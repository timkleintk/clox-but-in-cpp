﻿#include "scanner.h"


struct Scanner
{
	const char* start;
	const char* current;
	int line;
};

Scanner scanner;

void initScanner(const std::string& source)
{
	scanner.start = source.c_str();
	scanner.current = scanner.start;
	scanner.line = 1;
}

static bool isAlpha(const char c)
{
	return (c >= 'a' && c <= 'z') ||
		(c >= 'A' && c <= 'Z') ||
		c == '_';
}


static bool isAtEnd()
{
	return *scanner.current == '\0';
}

static char advance()
{
	return *scanner.current++;
}

static char peek()
{
	return *scanner.current;
}

static char peekNext()
{
	if (isAtEnd()) return '\0';
	return scanner.current[1];
}

static bool match(char expected)
{
	if (isAtEnd()) return false;
	if (*scanner.current != expected) return false;
	scanner.current++;
	return true;
}

static Token makeToken(const TokenType type)
{
	return { type,	scanner.start,	static_cast<size_t>(scanner.current - scanner.start),	scanner.line };
}

static Token errorToken(const char* message)
{
	return { TOKEN_ERROR, message, strlen(message), scanner.line };
}

static void skipWhitespace()
{
	while (true)
	{
		switch (peek())
		{
		case ' ':
		case '\r':
		case '\t':
			advance();
			break;
		case '\n':
			scanner.line++;
			advance();
			break;
		case '/':
			if (peekNext() == '/')
			{
				while (peek() != '\n' && !isAtEnd()) advance();
				break;
			}
			return;
		default:
			return;
		}
	}
}

static TokenType identifierType()
{
	auto checkKeyword = [](const size_t start, const size_t length, const char* rest, const TokenType type)
	{
		if (static_cast<size_t>(scanner.current - scanner.start) == start + length && memcmp(scanner.start + start, rest, length) == 0)
		{
			return type;
		}
		return TOKEN_IDENTIFIER;
	};

	switch (*scanner.start)
	{
	case 'a': return checkKeyword(1, 2, "nd", TOKEN_AND);
	case 'c': return checkKeyword(1, 4, "lass", TOKEN_CLASS);
	case 'e': return checkKeyword(1, 3, "lse", TOKEN_ELSE);
	case 'f':
		if (scanner.current - scanner.start > 1)
		{
			switch (scanner.start[1])
			{
			case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
			case 'o': return checkKeyword(2, 1, "r", TOKEN_FOR);
			case 'u': return checkKeyword(2, 1, "n", TOKEN_FUN);
			}
		}
		break;
	case 'i': return checkKeyword(1, 1, "f", TOKEN_IF);
	case 'n': return checkKeyword(1, 2, "il", TOKEN_NIL);
	case 'o': return checkKeyword(1, 1, "r", TOKEN_OR);
	case 'p': return checkKeyword(1, 4, "rint", TOKEN_PRINT);
	case 'r': return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
	case 's': return checkKeyword(1, 4, "uper", TOKEN_SUPER);
	case 't':
		if (scanner.current - scanner.start > 1)
		{
			switch (scanner.start[1])
			{
			case 'h': return checkKeyword(2, 2, "is", TOKEN_THIS);
			case 'r': return checkKeyword(2, 2, "ue", TOKEN_TRUE);
			}
		}
		break;
	case 'v': return checkKeyword(1, 2, "ar", TOKEN_VAR);
	case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);
	}

	return TOKEN_IDENTIFIER;
}

static Token identifier()
{
	while (isAlpha(peek()) || isdigit(peek())) advance();
	return makeToken(identifierType());
}

static Token string()
{
	while (peek() != '"' && !isAtEnd())
	{
		if (peek() == '\n') scanner.line++;
		advance();
	}

	if (isAtEnd()) return errorToken("Unterminated string.");

	// the closing quote
	advance();
	return makeToken(TOKEN_STRING);
}

static Token number()
{
	while (isdigit(peek())) advance();

	if (peek() == '.' && isdigit(peekNext()))
	{
		advance();

		while (isdigit(peek())) advance();
	}

	return makeToken(TOKEN_NUMBER);
}

Token scanToken()
{
	skipWhitespace();
	scanner.start = scanner.current;

	if (isAtEnd()) return makeToken(TOKEN_EOF);

	const char c = advance();
	if (isAlpha(c)) return identifier();
	if (isdigit(c)) return number();
	switch (c)
	{
	case '(': return makeToken(TOKEN_LEFT_PAREN);
	case ')': return makeToken(TOKEN_RIGHT_PAREN);
	case '{': return makeToken(TOKEN_LEFT_BRACE);
	case '}': return makeToken(TOKEN_RIGHT_BRACE);
	case ';': return makeToken(TOKEN_SEMICOLON);
	case ',': return makeToken(TOKEN_COMMA);
	case '.': return makeToken(TOKEN_DOT);
	case '-': return makeToken(TOKEN_MINUS);
	case '+': return makeToken(TOKEN_PLUS);
	case '/': return makeToken(TOKEN_SLASH);
	case '*': return makeToken(TOKEN_STAR);
	case '!':
		return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
	case '=':
		return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
	case '<':
		return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
	case '>':
		return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);

	case '"':
		return string();
	}

	return errorToken("Unexpected character.");
}

