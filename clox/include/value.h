#pragma once

#include "common.h"

typedef enum {
	VAL_BOOL,
	VAL_NIL,
	VAL_NUMBER,
} ValueType;

typedef struct {
	ValueType type;
	union {
		bool boolean;
		double number;
	} as;
} Value;

// nts: where is value array?

bool valuesEqual(Value a, Value b);

#define IS_BOOL(value)		((value).type == VAL_BOOL)
#define IS_NIL(value)		((value).type == VAL_NIL)
#define IS_NUMBER(value)	((value).type == VAL_NUMBER)

// nts: these defines don't seem much better than just typing .as.boolean
#define AS_BOOL(value)		((value).as.boolean)
#define AS_NUMBER(value)	((value).as.number)

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

void printValue(const Value& value);
