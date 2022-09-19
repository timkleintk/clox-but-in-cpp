#pragma once

#include "common.h"
#include "value.h"

#define OBJ_TYPE(value) ((value).as.obj->type)

#define IS_STRING(value) isObjType(value, OBJ_STRING)

constexpr ObjString* as_string(const Value& value) { return reinterpret_cast<ObjString*>(value.as.obj);}
constexpr char* as_cstring(const Value& value) { return as_string(value)->chars;}

typedef enum {
	OBJ_STRING,
} ObjType;

struct Obj {
	ObjType type;
};

struct ObjString {
	Obj obj;
	int length;
	char* chars;
};

ObjString* copyString(const char* chars, int length);

static inline bool isObjType(Value value, ObjType type) {
	return IS_OBJ(value) && value.as.obj->type == type;
}


