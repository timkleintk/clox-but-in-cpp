#include "debug.h"

#include <windows.h>

#include "object.h"

#include <cstdio>
#include <iostream>

#include "util.h"

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

size_t Chunk::jumpInstruction(const char* name, int sign, size_t offset) const
{
	uint16_t jump = (uint16_t)(code[offset + 1] << 8);
	jump |= code[offset + 2];
	printf("%-16s %4llu -> %llu\n", name, offset, offset + 3 + sign * jump);
	return offset + 3;
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
	grey();
	printf("%04llu ", offset);

	grey();
	if (offset > 0 && lines[offset] == lines[offset - 1])
	{
		printf("   | ");
	}
	else
	{
		printf("%4llu ", lines[offset]);
	}
	white();

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
		return constantInstruction("OP_GET_GLOBAL", offset);
	case OP_DEFINE_GLOBAL:
		return constantInstruction("OP_DEFINE_GLOBAL", offset);
	case OP_SET_GLOBAL:
		return constantInstruction("OP_SET_GLOBAL", offset);
	case OP_GET_UPVALUE:
		return byteInstruction("OP_GET_UPVALUE", offset);
	case OP_SET_UPVALUE:
		return byteInstruction("OP_SET_UPVALUE", offset);
	case OP_GET_PROPERTY:
		return constantInstruction("OP_GET_PROPERTY", offset);
	case OP_SET_PROPERTY:
		return constantInstruction("OP_SET_PROPERTY", offset);
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
	case OP_JUMP:
		return jumpInstruction("OP_JUMP", 1, offset);
	case OP_JUMP_IF_FALSE:
		return jumpInstruction("OP_JUMP_IF_FALSE", 1, offset);
	case OP_LOOP:
		return jumpInstruction("OP_LOOP", -1, offset);
	case OP_CALL:
		return byteInstruction("OP_CALL", offset);
	case OP_CLOSURE:
	{
		offset++;
		uint8_t constant = code[offset++];
		printf("%-16s %4d ", "OP_CLOSURE", constant);
		printValue(constants[constant]);
		printf("\n");

		ObjFunction* function = AS_FUNCTION(constants[constant]);
		for (int j = 0; j < function->upvalueCount; j++)
		{
			int isLocal = code[offset++];
			int index = code[offset++];
			grey();
			printf("%04llu      ", offset - 2);
			white();
			printf("|                     %s %d\n", isLocal ? "local" : "upvalue", index);
		}

		return offset;
	}
	SIMPLE_INSTRUCTION(OP_CLOSE_UPVALUE);
	SIMPLE_INSTRUCTION(OP_RETURN);
	case OP_CLASS:
		return constantInstruction("OP_CLASS", offset);
	default:
		std::cout << "Unknown opcode " << static_cast<uint8_t>(instruction) << "\n";
		return offset + 1;
	}

#undef SIMPLE_INSTRUCTION
}
