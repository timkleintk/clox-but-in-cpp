#include "debug.h"

#include <cstdio>
#include <iostream>


size_t Chunk::simpleInstruction(const std::string& name, const size_t offset)
{
	std::cout << name << "\n";
	return offset + 1;
}


size_t Chunk::constantInstruction(const char* name, size_t offset) const
{
	const uint8_t constant = code[offset + 1];
	printf("%-16s %4d '", name, constant);
	printValue(constants[constant]);
	printf("'\n");

	return offset + 2;
}


size_t Chunk::disassembleInstruction(size_t offset) const
{
	// nts: printf vs std::cout?
	printf("%04zu ", offset);

	if (offset > 0 && lines[offset] == lines[offset - 1])
	{
		printf("   | ");
	}
	else
	{
		printf("%4llu ", lines[offset]);
	}

	switch (const uint8_t instruction = code[offset])
	{
	case OP_CONSTANT:
		return constantInstruction("OP_CONSTANT", offset);
	case OP_RETURN:
		return simpleInstruction("OP_RETURN", offset);
	default:
		std::cout << "Unknown opcode " << instruction << "\n";
		return offset + 1;
	}
}
