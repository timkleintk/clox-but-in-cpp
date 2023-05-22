#include "debug.h"

#include <cstdio>
#include <iostream>

// nts: turn these into static functions taking in a "this" pointer?
size_t Chunk::simpleInstruction(const std::string& name, const size_t offset)
{
	std::cout << name << "\n";
	return offset + 1;
}

size_t Chunk::byteInstruction(const char* name, size_t offset) const
{
	uint8_t slot = code[offset + 1];
	printf("%-16s %4d\n", name, slot);
	return offset + 2;
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

#define SIMPLE_INSTRUCTION(name) case name: return simpleInstruction(#name,offset)

	switch (const Op instruction = static_cast<Op>(code[offset]))
	{
	case OP_CONSTANT:
		return constantInstruction("OP_CONSTANT", offset);
	SIMPLE_INSTRUCTION(OP_NIL);
	SIMPLE_INSTRUCTION(OP_TRUE);
	SIMPLE_INSTRUCTION(OP_FALSE);
	SIMPLE_INSTRUCTION(OP_POP);
	case OP_GET_LOCAL:
		return byteInstruction("OP_GET_LOCAL", offset);
	case OP_SET_LOCAL:
		return byteInstruction("OP_SET_LOCAL", offset);
	case OP_GET_GLOBAL: 
		return constantInstruction("OP_GET_GLOBAL",offset);
	case OP_DEFINE_GLOBAL:	
		return constantInstruction("OP_DEFINE_GLOBAL", offset);
	case OP_SET_GLOBAL:
		return constantInstruction("OP_SET_GLOBAL", offset);
	SIMPLE_INSTRUCTION(OP_EQUAL);
	SIMPLE_INSTRUCTION(OP_GREATER);
	SIMPLE_INSTRUCTION(OP_LESS);
	SIMPLE_INSTRUCTION(OP_ADD);
	SIMPLE_INSTRUCTION(OP_SUBTRACT);
	SIMPLE_INSTRUCTION(OP_MULTIPLY);
	SIMPLE_INSTRUCTION(OP_DIVIDE);
	SIMPLE_INSTRUCTION(OP_NOT);
	SIMPLE_INSTRUCTION(OP_NEGATE);
	SIMPLE_INSTRUCTION(OP_PRINT);
	SIMPLE_INSTRUCTION(OP_RETURN);
	default:
		std::cout << "Unknown opcode " << static_cast<uint8_t>(instruction) << "\n";
		return offset + 1;
	}

#undef SIMPLE_INSTRUCTION
}
