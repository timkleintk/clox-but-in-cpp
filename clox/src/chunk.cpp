#include "chunk.h"

#include <cstdlib>
#include <iostream>

#include "memory.h"
#include "object.h"
#include "util.h"
#include "vm.h"

Chunk::Chunk()
= default;

Chunk::~Chunk()
= default;

void Chunk::writeByte(const uint8_t byte, const size_t line)
{
	code.write(byte);
	while (lines.size() < code.size()) { lines.write(line); } // nts: optimization possible here
}


int Chunk::addConstant(const Value value)
{
	push(value);
	constants.write(value);
	pop();
	return static_cast<int>(constants.size() - 1);
}

void Chunk::disassemble(const char* name) const
{
	printf("== %s ==\n", name);
	if (constants.size() > 0)
	{
		grey();
		printf("constants: ");
		for (size_t i = 0; i < constants.size(); i++)
		{
			printf("[ ");
			if (IS_OBJ(constants[i]) && IS_STRING(constants[i])) { printf("\""); }
			printValue(constants[i]);
			if (IS_OBJ(constants[i]) && IS_STRING(constants[i])) { printf("\""); }
			printf(" ]");
		}
		printf("\n");
	}

	for (size_t offset = 0; offset < code.size();)
	{
		offset = disassembleInstruction(offset);
	}
	printf("\n");
}

size_t Chunk::count() const
{
	return code.size();
}

