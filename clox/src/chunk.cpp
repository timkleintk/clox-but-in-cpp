#include "chunk.h"

#include <cstdlib>
#include <iostream>

#include "memory.h"

Chunk::Chunk()
= default;

Chunk::~Chunk()
= default;

void Chunk::writeByte(const uint8_t byte, const size_t line)
{
	code.write(byte);
	while (lines.size() < code.size()) { lines.write(line); } // nts: optimization possible here
}


uint8_t Chunk::addConstant(const Value value)
{
	constants.write(value);
	return static_cast<uint8_t>(constants.size() - 1);
}

void Chunk::disassemble(const char* name) const
{
	printf("== %s ==\n", name);

	for (size_t offset = 0; offset < code.size();)
	{
		offset = disassembleInstruction(offset);
	}
}

