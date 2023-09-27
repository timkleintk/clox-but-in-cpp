#include "compiler.h"

#include "chunk.h"
#include "object.h"
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

using ParseFn = void(*)(bool canAssign);

struct ParseRule {
	ParseFn prefix;
	ParseFn inFix;
	Precedence precedence;
};

struct Local {
	Token name;
	int depth;
};

enum FunctionType {
	TYPE_FUNCTION,
	TYPE_SCRIPT
};

struct Compiler {
	Compiler* enclosing;
	ObjFunction* function;
	FunctionType type;

	Local locals[UINT8_COUNT];
	int localCount;
	int scopeDepth;
};

Parser parser;
Compiler* current = nullptr;
Chunk* compilingChunk;

static Chunk& currentChunk()
{
	return current->function->chunk;
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

static bool check(TokenType type)
{
	return parser.current.type == type;
}

static bool match(TokenType type)
{
	if (!check(type)) return false;
	advance();
	return true;
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

static void emitLoop(size_t loopStart)
{
	emitByte(OP_LOOP);

	size_t offset = currentChunk().code.size() - loopStart + 2;
	if (offset > UINT16_MAX) error("Loop body too large.");

	emitByte((offset >> 8) & 0xff);
	emitByte(offset & 0xff);
}

// emits an instruction and returns the address/offset of where the address to jump to is going to be stored
static size_t emitJump(uint8_t instruction)
{
	emitByte(instruction);
	emitByte(0xff);
	emitByte(0xff);
	return currentChunk().code.size() - 2;
}

static void emitReturn()
{
	emitByte(OP_NIL);
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

static void patchJump(size_t offset)
{
	const int jump = static_cast<int>(currentChunk().code.size()) - static_cast<int>(offset) - 2;

	if (jump > UINT16_MAX)
	{
		error("Too much code to jump over");
	}

	currentChunk().code[offset] = (jump >> 8) & 0xff;
	currentChunk().code[offset + 1] = jump & 0xff;

}

static void initCompiler(Compiler* compiler, FunctionType type)
{
	compiler->enclosing = current;
	compiler->function = nullptr;
	compiler->type = type;
	compiler->localCount = 0;
	compiler->scopeDepth = 0;
	compiler->function = newFunction();
	current = compiler;
	if (type != TYPE_SCRIPT)
	{
		current->function->name = copyString(parser.previous.start, parser.previous.length);
	}

	Local* local = &current->locals[current->localCount++];
	local->depth = 0;
	local->name.start = "";
	local->name.length = 0;
}

static ObjFunction* endCompiler()
{
	auto& code = currentChunk().code;
	if (code.size() == 0 || code[code.size() - 1] != OP_RETURN)
	{
		emitReturn();
	}
	ObjFunction* function = current->function;
#ifdef DEBUG_PRINT_CODE
	if (!parser.hadError)
	{
		currentChunk().disassemble(function->name != nullptr ? function->name->chars : "<script>");
	}
#endif

	current = current->enclosing;
	return function;
}

static void beginScope()
{
	current->scopeDepth++;
}

static void endScope()
{
	current->scopeDepth--;

	while (current->localCount > 0 && current->locals[current->localCount - 1].depth > current->scopeDepth)
	{
		emitByte(OP_POP);
		current->localCount--;
	}
}

static void expression();
static void statement();
static void declaration();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static uint8_t identifierConstant(Token* name)
{
	return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

static bool identifiersEqual(Token* a, Token* b)
{
	if (a->length != b->length) return false;
	return memcmp(a->start, b->start, a->length) == 0;
}

static int resolveLocal(Compiler* compiler, Token* name)
{
	for (int i = compiler->localCount - 1; i >= 0; i--)
	{
		Local* local = &compiler->locals[i];
		if (identifiersEqual(name, &local->name))
		{
			if (local->depth == -1)
			{
				error("Can't read local variable in its own initializer.");
			}
			return i;
		}
	}

	return -1;
}

static void addLocal(Token name)
{
	if (current->localCount == UINT8_COUNT)
	{
		error("Too many local variables in function.");
		return;
	}

	Local* local = &current->locals[current->localCount++];
	local->name = name;
	local->depth = -1;
}

static void declareVariable()
{
	if (current->scopeDepth == 0) return;

	Token* name = &parser.previous;
	for (int i = current->localCount - 1; i >= 0; i--)
	{
		Local* local = &current->locals[i];
		if (local->depth != -1 && local->depth < current->scopeDepth)
		{
			break;
		}

		if (identifiersEqual(name, &local->name))
		{
			error("Already a variable with this name in this scope.");
		}
	}


	addLocal(*name);
}

static uint8_t parseVariable(const char* errorMessage)
{
	consume(TOKEN_IDENTIFIER, errorMessage);

	declareVariable();
	if (current->scopeDepth > 0) return 0;

	return identifierConstant(&parser.previous);
}

static void markInitialized()
{
	if (current->scopeDepth == 0) return;
	current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void defineVariable(uint8_t global)
{
	if (current->scopeDepth > 0) {
		markInitialized();
		return;
	}
	emitBytes(OP_DEFINE_GLOBAL, global);
}

static uint8_t argumentList()
{
	uint8_t argCount = 0;
	if (!check(TOKEN_RIGHT_PAREN))
	{
		do
		{
			expression();
			if (argCount == 255) { error("Can't have more than 255 arguments."); }
			argCount++;
		} while (match(TOKEN_COMMA));
	}
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
	return argCount;
}

static void and_(bool)
{
	size_t endJump = emitJump(OP_JUMP_IF_FALSE);

	emitByte(OP_POP);
	parsePrecedence(PREC_AND);

	patchJump(endJump);
}

static void binary(bool)
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

static void call(bool)
{
	uint8_t argCount = argumentList();
	emitBytes(OP_CALL, argCount);
}

static void literal(bool) {
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

static void block()
{
	while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
	{
		declaration();
	}

	consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void function(FunctionType type)
{
	Compiler compiler;
	initCompiler(&compiler, type);
	beginScope();

	consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");

	if (!check(TOKEN_RIGHT_PAREN))
	{
		do
		{
			current->function->arity++;
			if (current->function->arity > 255)
			{
				errorAtCurrent("Can't have more than 255 parameters.");
			}
			uint8_t constant = parseVariable("Expect parameter name.");
			defineVariable(constant);
		} while (match(TOKEN_COMMA));
	}


	consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
	consume(TOKEN_LEFT_BRACE, "Expect '{' before function body");
	block();

	ObjFunction* function = endCompiler();
	emitBytes(OP_CONSTANT, makeConstant(OBJ_VAL(function)));

}

static void funDeclaration()
{
	uint8_t global = parseVariable("Expect function name.");
	markInitialized();
	function(TYPE_FUNCTION);
	defineVariable(global);
}

static void varDeclaration()
{
	uint8_t global = parseVariable("Expect variable name.");

	if (match(TOKEN_EQUAL))
	{
		expression();
	}
	else
	{
		emitByte(OP_NIL);
	}
	consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration");

	defineVariable(global);
}

static void expressionStatement()
{
	expression();
	consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
	emitByte(OP_POP);
}

static void forStatement()
{
	beginScope();
	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
	if (match(TOKEN_SEMICOLON))
	{
		// No initializer
	}
	else if (match(TOKEN_VAR))
	{
		varDeclaration();
	}
	else
	{
		expressionStatement();
	}



	size_t loopStart = currentChunk().count();
	size_t exitJump = 0;
	if (!match(TOKEN_SEMICOLON))
	{
		expression();
		consume(TOKEN_SEMICOLON, "Expect ';' after loop condition");

		exitJump = emitJump(OP_JUMP_IF_FALSE);
		emitByte(OP_POP);
	}

	if (!match(TOKEN_RIGHT_PAREN))
	{
		size_t bodyJump = emitJump(OP_JUMP);
		size_t incrementStart = currentChunk().count();
		expression();
		emitByte(OP_POP);
		consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

		emitLoop(loopStart);
		loopStart = incrementStart;
		patchJump(bodyJump);
	}

	statement();
	emitLoop(loopStart);

	if (exitJump != 0) // exitJump can never be zero, unless the loop condition doesn't exist
	{
		patchJump(exitJump);
		emitByte(OP_POP);
	}

	endScope();
}

static void ifStatement()
{
	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

	size_t thenJump = emitJump(OP_JUMP_IF_FALSE);
	emitByte(OP_POP);
	statement();

	size_t elseJump = emitJump(OP_JUMP);

	patchJump(thenJump);
	emitByte(OP_POP);

	if (match(TOKEN_ELSE)) statement();
	patchJump(elseJump);
}

static void printStatement()
{
	expression();
	consume(TOKEN_SEMICOLON, "Expect ';' after value.");
	emitByte(OP_PRINT);
}

static void returnStatement()
{
	if (current->type == TYPE_SCRIPT)
	{
		error("Can't return from top-level code.");
	}

	if (match(TOKEN_SEMICOLON))
	{
		emitReturn();
	}
	else
	{
		expression();
		consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
		emitByte(OP_RETURN);
	}
}

static void whileStatement()
{
	size_t loopStart = currentChunk().code.size();
	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'");
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

	size_t exitJump = emitJump(OP_JUMP_IF_FALSE);
	emitByte(OP_POP);
	statement();
	emitLoop(loopStart);

	patchJump(exitJump);
	emitByte(OP_POP);
}

static void synchronize()
{
	parser.panicMode = false;

	while (parser.current.type != TOKEN_EOF)
	{
		// check if we've passed a semicolon or encounter the beginning of a new statement
		if (parser.previous.type == TOKEN_SEMICOLON) return;
		switch (parser.current.type)
		{
		case TOKEN_CLASS:
		case TOKEN_FUN:
		case TOKEN_VAR:
		case TOKEN_FOR:
		case TOKEN_IF:
		case TOKEN_WHILE:
		case TOKEN_PRINT:
		case TOKEN_RETURN:
			return;
		default:
			; // do nothing
		}

		advance();
	}

}

static void declaration()
{
	if (match(TOKEN_FUN))
	{
		funDeclaration();
	}
	else if (match(TOKEN_VAR))
	{
		varDeclaration();
	}
	else
	{
		statement();
	}

	if (parser.panicMode) synchronize();
}

static void statement()
{
	if (match(TOKEN_PRINT))
	{
		printStatement();
	}
	else if (match(TOKEN_FOR))
	{
		forStatement();
	}
	else if (match(TOKEN_IF))
	{
		ifStatement();
	}
	else if (match(TOKEN_RETURN))
	{
		returnStatement();
	}
	else if (match(TOKEN_WHILE))
	{
		whileStatement();
	}
	else if (match(TOKEN_LEFT_BRACE))
	{
		beginScope();
		block();
		endScope();
	}
	else
	{
		expressionStatement();
	}
}

static void grouping(bool)
{
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number(bool)
{
	double value = strtod(parser.previous.start, nullptr);
	emitConstant(NUMBER_VAL(value));
}

static void or_(bool)
{
	size_t elseJump = emitJump(OP_JUMP_IF_FALSE);
	size_t endJump = emitJump(OP_JUMP);

	patchJump(elseJump);
	emitByte(OP_POP);

	parsePrecedence(PREC_OR);
	patchJump(endJump);
}

static void string(bool) {
	emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

static void namedVariable(Token name, bool canAssign)
{
	uint8_t getOp, setOp;
	int arg = resolveLocal(current, &name);
	if (arg != -1)
	{
		getOp = OP_GET_LOCAL;
		setOp = OP_SET_LOCAL;
	}
	else
	{
		arg = identifierConstant(&name);
		getOp = OP_GET_GLOBAL;
		setOp = OP_SET_GLOBAL;
	}

	if (canAssign && match(TOKEN_EQUAL))
	{
		expression();
		emitBytes(setOp, static_cast<uint8_t>(arg));
	}
	else
	{
		emitBytes(getOp, static_cast<uint8_t>(arg));
	}
}

static void variable(bool canAssign)
{
	namedVariable(parser.previous, canAssign);
}

static void unary(bool)
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
	/*[TOKEN_LEFT_PAREN]   */ {grouping, call,	  PREC_CALL},
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
	/*[TOKEN_IDENTIFIER]   */ {variable, nullptr, PREC_NONE},
	/*[TOKEN_STRING]       */ {string,	nullptr, PREC_NONE},
	/*[TOKEN_NUMBER]       */ {number,   nullptr, PREC_NONE},
	/*[TOKEN_AND]          */ {nullptr,  and_,		PREC_AND},
	/*[TOKEN_CLASS]        */ {nullptr,  nullptr, PREC_NONE},
	/*[TOKEN_ELSE]         */ {nullptr,  nullptr, PREC_NONE},
	/*[TOKEN_FALSE]        */ {literal,  nullptr, PREC_NONE},
	/*[TOKEN_FOR]          */ {nullptr,  nullptr, PREC_NONE},
	/*[TOKEN_FUN]          */ {nullptr,  nullptr, PREC_NONE},
	/*[TOKEN_IF]           */ {nullptr,  nullptr, PREC_NONE},
	/*[TOKEN_NIL]          */ {literal,  nullptr, PREC_NONE},
	/*[TOKEN_OR]           */ {nullptr,  or_,		PREC_OR},
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

	bool canAssign = precedence <= PREC_ASSIGNMENT;
	prefixRule(canAssign);

	while (precedence <= getRule(parser.current.type)->precedence)
	{
		advance();
		const ParseFn infixRule = getRule(parser.previous.type)->inFix; // nts: merge these two lines
		infixRule(canAssign);
	}

	if (canAssign && match(TOKEN_EQUAL))
	{
		error("Invalid assignment target.");
	}

}

static ParseRule* getRule(const TokenType type)
{
	return &rules[type];
}

ObjFunction* compile(const std::string& source)
{
	initScanner(source);
	Compiler compiler;
	initCompiler(&compiler, TYPE_SCRIPT);

	parser.hadError = false;
	parser.panicMode = false;

	advance();

	while (!match(TOKEN_EOF))
	{
		declaration();
	}

	ObjFunction* function = endCompiler();
	return parser.hadError ? nullptr : function;
}
