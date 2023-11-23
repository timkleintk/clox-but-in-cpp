﻿#pragma once
#include <cstdint>
#include <string>

#include "table.h"
#include "value.h"

struct ObjUpvalue;
struct ObjClosure;
struct Chunk;

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

struct CallFrame
{
	ObjClosure* closure;
	uint8_t* ip;
	Value* slots;
};

struct VM
{
	VM() = default;
	~VM() = default;

	CallFrame frames[FRAMES_MAX];
	size_t frameCount;
	
	Value stack[STACK_MAX];
	Value* stackTop;
	Table globals;
	Table strings;
	ObjUpvalue* openUpvalues;

	size_t bytesAllocated;
	size_t nextGC;
	Obj* objects;

	int grayCount;
	int grayCapacity;
	Obj** grayStack;

};

enum InterpretResult
{
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR
};

extern VM vm;


void initVM();
void freeVM();

InterpretResult interpret(const std::string& source);

void push(Value value);
Value pop();
