#pragma once

#include "chunk.h"
#include "common.h"
#include "value.h"

#define OBJ_TYPE(value)		(AS_OBJ(value)->type)

#define IS_FUNCTION(value)	isObjType(value, OBJ_FUNCTION)
#define IS_NATIVE(value)	isObjType(value, OBJ_NATIVE)
#define IS_STRING(value)	isObjType(value, OBJ_STRING)

#define AS_FUNCTION(value)	((ObjFunction*)AS_OBJ(value))
#define AS_NATIVE(value)	(((ObjNative*)AS_OBJ(value))->function)
#define AS_STRING(value)	((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)	(((ObjString*)AS_OBJ(value))->chars)

typedef enum {
	OBJ_FUNCTION,
	OBJ_NATIVE,
	OBJ_STRING,
} ObjType;

struct Obj {
	ObjType type;
	Obj* next;
};

struct ObjFunction
{
	Obj obj;
	int arity;
	Chunk chunk;
	ObjString* name;
};

typedef Value(*NativeFn)(int argCount, Value* args);

struct ObjNative
{
	Obj obj;
	NativeFn function;
};

struct ObjString {
	Obj obj;
	size_t length;
	char* chars;
	uint32_t hash;
};

ObjFunction* newFunction();
ObjNative* newNative(NativeFn function);
ObjString* takeString(char* chars, size_t length); // construct a string Obj and take ownership of the char array
ObjString* copyString(const char* chars, size_t length); // construct a string Obj with a copy of the char array
void printObject(Value value);

static inline bool isObjType(Value value, ObjType type) {
	return IS_OBJ(value) && value.as.obj->type == type;
}

