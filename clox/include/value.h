﻿#pragma once

#include "common.h"

typedef struct Obj Obj;
typedef struct ObjString ObjString;

typedef enum {
	VAL_BOOL,
	VAL_NIL,
	VAL_NUMBER,
	VAL_OBJ,
} ValueType;

typedef struct {
	ValueType type;
	union {
		bool boolean;
		double number;
		Obj* obj;
	} as;
} Value;

// nts: where is value array?

bool valuesEqual(Value a, Value b);

#define IS_BOOL(value)		((value).type == VAL_BOOL)
#define IS_NIL(value)		((value).type == VAL_NIL)
#define IS_NUMBER(value)	((value).type == VAL_NUMBER)
#define IS_OBJ(value)		((value).type == VAL_OBJ)

// this is how the book does it
/*
#define BOOL_VAL(value)		((Value){VAL_BOOL, {.boolean = value}})
#define	NIL_VAL				((Value){VAL_NIL, {.number = 0}}
#define NUMBER_VAL(value)	((Value){VAL_NUMBER, {.number = value}})
*/
// but I think this works better:
#define BOOL_VAL(value)		Value{VAL_BOOL, {.boolean = value}}
#define	NIL_VAL				Value{VAL_NIL, {.number = 0}}
#define NUMBER_VAL(value)	Value{VAL_NUMBER, {.number = value}}
#define OBJ_VAL(object)		Value{VAL_OBJ, {.obj = static_cast<Obj*>(object)}}

void printValue(const Value& value);
