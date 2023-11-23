#pragma once

#include "common.h"

#include "chunk.h"
#include "value.h"

#define OBJ_TYPE(value)		(AS_OBJ(value)->type)

#define IS_CLOSURE(value)	isObjType(value, OBJ_CLOSURE)
#define IS_FUNCTION(value)	isObjType(value, OBJ_FUNCTION)
#define IS_NATIVE(value)	isObjType(value, OBJ_NATIVE)
#define IS_STRING(value)	isObjType(value, OBJ_STRING)

#define AS_CLOSURE(value)	((ObjClosure*)AS_OBJ(value))
#define AS_FUNCTION(value)	((ObjFunction*)AS_OBJ(value))
#define AS_NATIVE(value)	(((ObjNative*)AS_OBJ(value))->function)
#define AS_STRING(value)	((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)	(((ObjString*)AS_OBJ(value))->chars)

typedef enum {
	OBJ_CLOSURE,
	OBJ_FUNCTION,
	OBJ_NATIVE,
	OBJ_STRING,
	OBJ_UPVALUE,
} ObjType;

#ifdef DEBUG_LOG_GC
inline const char* ObjTypeNames[] = {
	"OBJ_CLOSURE",
	"OBJ_FUNCTION",
	"OBJ_NATIVE",
	"OBJ_STRING",
	"OBJ_UPVALUE"
};
#endif

struct Obj {
	ObjType type;
	bool isMarked;
	Obj* next;
};

struct ObjFunction
{
	Obj obj;
	int arity;
	int upvalueCount;
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

struct ObjUpvalue
{
	Obj obj;
	Value* location;
	Value closed;
	ObjUpvalue* next;
};

struct ObjClosure
{
	Obj obj;
	ObjFunction* function;
	ObjUpvalue** upvalues;
	int upvalueCount;
};

ObjClosure* newClosure(ObjFunction* function);
ObjFunction* newFunction();
ObjNative* newNative(NativeFn function);
ObjString* takeString(char* chars, size_t length); // construct a string Obj and take ownership of the char array
ObjString* copyString(const char* chars, size_t length); // construct a string Obj with a copy of the char array
ObjUpvalue* newUpvalue(Value* slot);
void printObject(Value value);

static inline bool isObjType(Value value, ObjType type) {
	return IS_OBJ(value) && value.as.obj->type == type;
}

