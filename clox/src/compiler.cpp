#include "compiler.h"

#include "scanner.h"


void compile(const std::string& source)
{
	initScanner(source);
	int line = 1;

	while (true)
	{
		const Token token = scanToken();
		if (token.line != line)
		{
			printf("%4d ", line);
			line = token.line;
		}
		else
		{
			printf("   | ");
		}
		printf("%2d '%.*s'\n", token.type, static_cast<int>(token.length), token.start);

		if (token.type == TOKEN_EOF) break;
	}
}
