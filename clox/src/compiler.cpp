#include "compiler.h"

#include "chunk.h"
#include "scanner.h"

struct Parser
{
	Token current;
	Token previous;
	bool hadError;
	bool panicMode;
};

enum Precedence
{
	PREC_NONE,
	PREC_ASSIGNMENT,  // =
	PREC_OR,          // or
	PREC_AND,         // and
	PREC_EQUALITY,    // == !=
	PREC_COMPARISON,  // < > <= >=
	PREC_TERM,        // + -
	PREC_FACTOR,      // * /
	PREC_UNARY,       // ! -
	PREC_CALL,        // . ()
	PREC_PRIMARY
};

using ParseFn = void(*)();

struct ParseRule
{
	ParseFn prefix;
	ParseFn inFix;
	Precedence precedence;
};

Parser parser;

Chunk* compilingChunk;

static Chunk& currentChunk()
{
	return *compilingChunk;
}

static void errorAt(const Token& token, const char* message)
{
	if (parser.panicMode) return;
	parser.panicMode = true;

	fprintf(stderr, "[line %d] Error", token.line);

	if (token.type == TOKEN_EOF)
	{
		fprintf(stderr, "at '%.*s'", static_cast<int>(token.length), token.start);
	}
	else if (token.type == TOKEN_ERROR)
	{
		// nothing
	}
	else
	{
		fprintf(stderr, " at '%.*s'", static_cast<int>(token.length), token.start);
	}

	fprintf(stderr, ": %s\n", message);
	parser.hadError = true;
}

static void error(const char* message)
{
	errorAt(parser.previous, message);
}

static void errorAtCurrent(const char* message)
{
	errorAt(parser.current, message);
}

static void advance()
{
	parser.previous = parser.current;

	while (true)
	{
		parser.current = scanToken();
		if (parser.current.type != TOKEN_ERROR) break;
		errorAtCurrent(parser.current.start);
	}
}

static void consume(const TokenType type, const char* message)
{
	if (parser.current.type == type)
	{
		advance();
		return;
	}

	errorAtCurrent(message);
}

static void emitByte(const uint8_t byte)
{
	currentChunk().writeByte(byte, parser.previous.line);
}

static void emitBytes(const uint8_t byte1, const uint8_t byte2)
{
	emitByte(byte1);
	emitByte(byte2);
}

static void emitReturn()
{
	emitByte(OP_RETURN);
}

static uint8_t makeConstant(const Value value)
{
	const int constant = currentChunk().addConstant(value);
	if (constant > UINT8_MAX)
	{
		error("Too many constants in one chunk.");
		return 0;
	}

	return static_cast<uint8_t>(constant);
}

static void emitConstant(const Value value)
{
	emitBytes(OP_CONSTANT, makeConstant(value));
}

static void endCompiler()
{
	emitReturn();
#ifdef DEBUG_PRINT_CODE
	if (!parser.hadError)
	{
		currentChunk().disassemble("code");
	}
#endif
}

static void expression();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static void binary()
{
	TokenType operatorType = parser.previous.type;
	ParseRule* rule = getRule(operatorType);
	parsePrecedence((Precedence)(rule->precedence + 1));

	switch (operatorType)
	{
	case TOKEN_BANG_EQUAL:    emitBytes(OP_EQUAL, OP_NOT); break;
	case TOKEN_EQUAL_EQUAL:	  emitByte(OP_EQUAL); break;
	case TOKEN_GREATER:		  emitByte(OP_GREATER); break;
	case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
	case TOKEN_LESS:		  emitByte(OP_LESS); break;
	case TOKEN_LESS_EQUAL:	  emitBytes(OP_GREATER, OP_NOT); break;
	case TOKEN_PLUS:	emitByte(OP_ADD); break;
	case TOKEN_MINUS:	emitByte(OP_SUBTRACT); break;
	case TOKEN_STAR:	emitByte(OP_MULTIPLY); break;
	case TOKEN_SLASH:	emitByte(OP_DIVIDE); break;
	default: return; // unreachable
	}
}

static void literal() {
	switch (parser.previous.type) {
	case TOKEN_FALSE: emitByte(OP_FALSE); break;
	case TOKEN_NIL: emitByte(OP_NIL); break;
	case TOKEN_TRUE: emitByte(OP_TRUE); break;
	default: return; // unreachable
	}
}

static void expression()
{
	parsePrecedence(PREC_ASSIGNMENT);
}

static void grouping()
{
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number()
{
	double value = strtod(parser.previous.start, nullptr);
	emitConstant(NUMBER_VAL(value));
}

static void unary()
{
	TokenType operatorType = parser.previous.type;

	// compile the operand
	parsePrecedence(PREC_UNARY);

	switch (operatorType)
	{
	case TOKEN_BANG: emitByte(OP_NOT); break;
	case TOKEN_MINUS: emitByte(OP_NEGATE); break;
	default: return;
	}
}

// this table describes what to do when you encounter a certain token as a prefix and as an infix as well as the precedense the whole infix expression would have

ParseRule rules[] = {
	/*[TOKEN_LEFT_PAREN]   */ {grouping, nullptr, PREC_NONE},
	/*[TOKEN_RIGHT_PAREN]  */ {nullptr,  nullptr, PREC_NONE},
	/*[TOKEN_LEFT_BRACE]   */ {nullptr,  nullptr, PREC_NONE},
	/*[TOKEN_RIGHT_BRACE]  */ {nullptr,  nullptr, PREC_NONE},
	/*[TOKEN_COMMA]        */ {nullptr,  nullptr, PREC_NONE},
	/*[TOKEN_DOT]          */ {nullptr,  nullptr, PREC_NONE},
	/*[TOKEN_MINUS]        */ {unary,    binary,  PREC_TERM},
	/*[TOKEN_PLUS]         */ {nullptr,  binary,  PREC_TERM},
	/*[TOKEN_SEMICOLON]    */ {nullptr,  nullptr, PREC_NONE},
	/*[TOKEN_SLASH]        */ {nullptr,  binary,  PREC_FACTOR},
	/*[TOKEN_STAR]         */ {nullptr,  binary,  PREC_FACTOR},
	/*[TOKEN_BANG]         */ {unary,    nullptr, PREC_NONE},
	/*[TOKEN_BANG_EQUAL]   */ {nullptr,  binary,  PREC_EQUALITY},
	/*[TOKEN_EQUAL]        */ {nullptr,  nullptr, PREC_NONE},
	/*[TOKEN_EQUAL_EQUAL]  */ {nullptr,  binary,  PREC_EQUALITY},
	/*[TOKEN_GREATER]      */ {nullptr,  binary,  PREC_COMPARISON},
	/*[TOKEN_GREATER_EQUAL]*/ {nullptr,  binary,  PREC_COMPARISON},
	/*[TOKEN_LESS]         */ {nullptr,  binary,  PREC_COMPARISON},
	/*[TOKEN_LESS_EQUAL]   */ {nullptr,  binary,  PREC_COMPARISON},
	/*[TOKEN_IDENTIFIER]   */ {nullptr,  nullptr, PREC_NONE},
	/*[TOKEN_STRING]       */ {nullptr,  nullptr, PREC_NONE},
	/*[TOKEN_NUMBER]       */ {number,   nullptr, PREC_NONE},
	/*[TOKEN_AND]          */ {nullptr,  nullptr, PREC_NONE},
	/*[TOKEN_CLASS]        */ {nullptr,  nullptr, PREC_NONE},
	/*[TOKEN_ELSE]         */ {nullptr,  nullptr, PREC_NONE},
	/*[TOKEN_FALSE]        */ {literal,  nullptr, PREC_NONE},
	/*[TOKEN_FOR]          */ {nullptr,  nullptr, PREC_NONE},
	/*[TOKEN_FUN]          */ {nullptr,  nullptr, PREC_NONE},
	/*[TOKEN_IF]           */ {nullptr,  nullptr, PREC_NONE},
	/*[TOKEN_NIL]          */ {literal,  nullptr, PREC_NONE},
	/*[TOKEN_OR]           */ {nullptr,  nullptr, PREC_NONE},
	/*[TOKEN_PRINT]        */ {nullptr,  nullptr, PREC_NONE},
	/*[TOKEN_RETURN]       */ {nullptr,  nullptr, PREC_NONE},
	/*[TOKEN_SUPER]        */ {nullptr,  nullptr, PREC_NONE},
	/*[TOKEN_THIS]         */ {nullptr,  nullptr, PREC_NONE},
	/*[TOKEN_TRUE]         */ {literal,  nullptr, PREC_NONE},
	/*[TOKEN_VAR]          */ {nullptr,  nullptr, PREC_NONE},
	/*[TOKEN_WHILE]        */ {nullptr,  nullptr, PREC_NONE},
	/*[TOKEN_ERROR]        */ {nullptr,  nullptr, PREC_NONE},
	/*[TOKEN_EOF]          */ {nullptr,  nullptr, PREC_NONE},
};

static void parsePrecedence(Precedence precedence)
{
	advance();
	const ParseFn prefixRule = getRule(parser.previous.type)->prefix;
	if (prefixRule == nullptr)
	{
		error("expect expression.");
		return;
	}

	prefixRule();

	while (precedence <= getRule(parser.current.type)->precedence)
	{
		advance();
		const ParseFn infixRule = getRule(parser.previous.type)->inFix; // nts: merge these two lines
		infixRule();
	}
}

static ParseRule* getRule(const TokenType type)
{
	return &rules[type];
}

bool compile(const std::string& source, Chunk& chunk)
{
	initScanner(source);

	compilingChunk = &chunk;

	parser.hadError = false;
	parser.panicMode = false;

	advance();
	expression();
	consume(TOKEN_EOF, "Expect end of expression.");

	endCompiler();
	return !parser.hadError;
}
