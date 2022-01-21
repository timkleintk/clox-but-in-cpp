#pragma once

#include "common.h"

enum OpCode
{
	OP_RETURN
};

struct Chunck
{
	Chunck();
	~Chunck();

	void init();
	void free();
	void write(uint8_t byte);

	size_t count;
	size_t capacity;
	uint8_t* code;
};
