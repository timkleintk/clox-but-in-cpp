#pragma once

#include <string>

#include "blob.h"
#include "common.h"
#include "value.h"

enum Op : uint8_t
{
	OP_CONSTANT,
	OP_ADD,
	OP_SUBTRACT,
	OP_MULTIPLY,
	OP_DIVIDE,
	OP_NEGATE,
	OP_RETURN
};

struct Chunk
{
	Chunk();
	~Chunk();

	//template<typename T>
	//void writeCode(T code, const size_t line) { writeCode(static_cast<uint8_t>(code), line); }
	//void writeOp(Op op, size_t line) { writeByte(static_cast<uint8_t>(op), line); }
	//void WriteConstant(size_t constant, size_t line) { writeByte(static_cast<uint8_t>(constant), line); }
	void writeByte(uint8_t byte, size_t line);
	int addConstant(Value value);

	void disassemble(const char* name) const;
	size_t disassembleInstruction(size_t offset) const;

	Blob<uint8_t> code;
	Blob<size_t> lines;
	Blob<Value> constants;

private:

	static size_t simpleInstruction(const std::string& name, size_t offset);
	size_t constantInstruction(const char* name, size_t offset) const;

};


