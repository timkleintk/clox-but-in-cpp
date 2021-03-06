#pragma once
#include <cstdint>
#include <string>

#include "value.h"

struct Chunk;

#define STACK_MAX 256

struct VM
{
	VM() = default;
	~VM() = default;
	
	Chunk* chunk = nullptr;
	uint8_t* ip = nullptr;
	Value stack[STACK_MAX];
	Value* stackTop;
};

enum InterpretResult
{
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR
};


void initVM();
void freeVM();

InterpretResult interpret(const std::string& source);

void push(Value value);
Value pop();
