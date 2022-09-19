#pragma once

#include <cstdio>
#include <string>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"


template<class Type>
constexpr Type* allocate_obj(ObjType type) {return allocateObject }


static ObjString* allocateString(char* chars, int length) {
	ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
	string->length = length;
	string->chars = chars;
	return string;
}

ObjString* copyString(const char* chars, int length) {
	char* heapChars = ALLOCATE(char, lenght + 1);
	memcpy(heapChars, chars, length);
	heapChars[length] = '\0';
	return allocateString(heapChars, length);
}


