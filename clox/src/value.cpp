#include "value.h"

#include <cstdio>

bool valuesEqual(Value a, Value b)
{
	if (a.type != b.type) return false;
	switch (a.type)
	{
		// nts: this can become a very nice macro
	case VAL_BOOL: return a.as.boolean == b.as.boolean;
	case VAL_NIL: return true;
	case VAL_NUMBER: return a.as.number == b.as.number;
	default: return false;// unreachable
	}
}

void printValue(const Value& value)
{
	switch (value.type)
	{
	case VAL_BOOL: printf(AS_BOOL(value) ? "true" : "false"); break;
	case VAL_NIL: printf("nil"); break;
	case VAL_NUMBER: printf("%g", AS_NUMBER(value));
	default: return; // unreachable
	}
	
}

