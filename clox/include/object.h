#pragma once

#include "common.h"
#include "value.h"

#define OBJ_TYPE(value)		(AS_OBJ(value)->type)

#define IS_STRING(value)	isObjType(value, OBJ_STRING)

#define AS_STRING(value)	((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)	(((ObjString*)AS_OBJ(value))->chars)

typedef enum {
	OBJ_STRING,
} ObjType;

struct Obj {
	ObjType type;
	struct Obj* next;
};

struct ObjString {
	Obj obj;
	size_t length;
	char* chars;
	uint32_t hash;
};

ObjString* takeString(char* chars, size_t length); // construct a string Obj and take ownership of the char array
ObjString* copyString(const char* chars, size_t length); // construct a string Obj with a copy of the char array
void printObject(Value value);

static inline bool isObjType(Value value, ObjType type) {
	return IS_OBJ(value) && value.as.obj->type == type;
}

