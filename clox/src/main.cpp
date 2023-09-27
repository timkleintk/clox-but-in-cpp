#include <fstream>
#include <iostream>
#include <stack>

#include "chunk.h"
#include "common.h"
#include "debug.h"
#include "vm.h"

#include <stdlib.h>
#include <stdio.h>


static bool IsSourceComplete(const std::string& source)
{
	std::stack<char> braceBalance;
	char previous = 0;
	bool inComment = false;
	bool inString = false;

	// three newlines at the end should return true
	if (source.size() >= 3 && source.substr(source.size() - 3) == "\n\n\n") return true;

	for (const auto& c : source)
	{
		if (inString)
		{
			if (c == '"') inString = false;
		}
		else if (inComment)
		{
			if (c == '\n') inComment = false;
		}
		else
		{
			switch (c)
			{
			case '/':
				// peek second slash
				if (*(&c + 1) == '/') { inComment = true; }
				continue;
			case '"':
				inString = true;
				break;
			case '{':
			case '(':
				braceBalance.push(c);
				break;
			case '}':
			case ')':
			{
				const char complement = c == '}' ? '{' : '(';
				if (!braceBalance.empty() && braceBalance.top() == complement)
				{
					braceBalance.pop();
				}
				else
				{
					return true;
				}
				break;
			}
			default:
				break;
			}

			if (!std::isspace(c)) previous = c;
		}
	}

	if (inString) return false;
	if (braceBalance.empty())
	{
		if (previous == '}' || previous == ';' || previous == '\0') return true;
	}

	return false;

}

static void RunFile(const char* path)
{
	if (std::ifstream inputStream(path); inputStream.is_open())
	{
		const std::string source((std::istreambuf_iterator(inputStream)), std::istreambuf_iterator<char>());

		const InterpretResult result = interpret(source);

		if (result == INTERPRET_COMPILE_ERROR) exit(65);
		if (result == INTERPRET_RUNTIME_ERROR) exit(70);
	}
	else
	{
		std::cerr << "Couldn't open file \"" << path << "\".\n";
		exit(74);
	}
}

static void repl(const bool qualityOfLife)
{
	std::string source;
	std::string line;
	int lines = 1;

	while (true)
	{
		if (qualityOfLife)
		{
			std::cout << "  > ";
		}

		// get input
		std::getline(std::cin, source);

		// special commands
		if (std::cin.eof() || source == "exit") { break; }
		if (source == "clear") { system("CLS"); continue; }

		// multiline inputs
		while (!IsSourceComplete(source)) // disable multiline support for now
		{
			if (qualityOfLife)
			{
				printf("%-2d> ", ++lines);
			}
			std::getline(std::cin, line);
			source += "\n";
			source += line;
		}
		lines = 1;

		// interpret
		interpret(source);

		// for unit testing
		//if (!qualityOfLife)
		//{
		//	if (m_hadError) { exit(65); }
		//	if (m_hadRuntimeError) { exit(70); }
		//}

		//m_hadError = false;

	}
}

int main(const int argc, const char* argv[])
{
	initVM();
	if (argc == 1)
	{
		repl(true);
	}
	else if (argc == 2)
	{
		if (strcmp(argv[1], "test") == 0)
		{
			repl(false);
		}
		else
		{
			RunFile(argv[0]);
		}
	}
	else
	{
		fprintf(stderr, "Usage: clox [path]\n");
		exit(64);
	}

	freeVM();

	return 0;
}
