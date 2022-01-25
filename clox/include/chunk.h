#pragma once

#include <string>

#include "blob.h"
#include "common.h"
#include "value.h"

enum OpCode
{
	OP_RETURN,
	OP_CONSTANT
};

struct Chunk
{
	Chunk();
	~Chunk();

	template<typename T>
	void writeCode(T byte, const size_t line) { writeCode(static_cast<uint8_t>(byte), line); }
	void writeCode(uint8_t byte, size_t line);
	size_t addConstant(Value value);

	void disassemble(const char* name) const;


private:
	Blob<uint8_t> code;
	Blob<size_t> lines;
	Blob<Value> constants;

	size_t disassembleInstruction(size_t offset) const;

	static size_t simpleInstruction(const std::string& name, size_t offset);
	size_t constantInstruction(const char* name, size_t offset) const;

};
