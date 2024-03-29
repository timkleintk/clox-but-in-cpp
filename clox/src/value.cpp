﻿#include "value.h"

#include <cstdio>
#include <cstring>

#include "object.h"
//#include "memory.h" // book says I need this, but I don't

bool valuesEqual(Value a, Value b)
{
	if (a.type != b.type) return false;
	switch (a.type)
	{
		// nts: this can become a very nice macro
	case VAL_BOOL:		return a.as.boolean == b.as.boolean;
	case VAL_NIL:		return true;
	case VAL_NUMBER:	return a.as.number == b.as.number;
	case VAL_OBJ:		return AS_OBJ(a) == AS_OBJ(b);
	default: return false;// unreachable
	}
}

void printValue(const Value& value)
{
	switch (value.type)
	{
	case VAL_BOOL: printf(AS_BOOL(value) ? "true" : "false"); break;
	case VAL_NIL: printf("nil"); break;
	case VAL_NUMBER: printf("%g", AS_NUMBER(value)); break;
	case VAL_OBJ: printObject(value); break;
	default: return; // unreachable
	}
	
}

