#include "debug.h"

#include <cstdio>
#include <iostream>

// nts: turn these into static functions taking in a "this" pointer?
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


size_t Chunk::disassembleInstruction(const size_t offset) const
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

	switch (const Op instruction = static_cast<Op>(code[offset]))
	{
	case OP_CONSTANT:
		return constantInstruction("OP_CONSTANT", offset);
	case OP_ADD:
		return simpleInstruction("OP_ADD", offset);
	case OP_SUBTRACT:
		return simpleInstruction("OP_SUBTRACT", offset);
	case OP_MULTIPLY:
		return simpleInstruction("OP_MULTIPLY", offset);
	case OP_DIVIDE:
		return simpleInstruction("OP_DIVIDE", offset);
	case OP_NEGATE:
		return simpleInstruction("OP_NEGATE", offset);
	case OP_RETURN:
		return simpleInstruction("OP_RETURN", offset);
	default:
		std::cout << "Unknown opcode " << static_cast<uint8_t>(instruction) << "\n";
		return offset + 1;
	}
}
