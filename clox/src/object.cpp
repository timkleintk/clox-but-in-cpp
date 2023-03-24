#include "object.h"

#include <cstdio>
#include <cstring>

#include "memory.h"
#include "table.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)

static Obj* allocateObject(size_t size, ObjType type)
{
	Obj* object = (Obj*)reallocate(nullptr, 0, size);
	object->type = type;

	object->next = vm.objects;
	vm.objects = object;

	return object;
}

static ObjString* allocateString(char* chars, size_t length, uint32_t hash)
{
	ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
	string->length = length;
	string->chars = chars;
	string->hash = hash;
	vm.strings.set(string, NIL_VAL);
	return string;
}


uint32_t hashString(const char* key, const size_t length)
{
	uint32_t hash = 2166136261u;
	for (size_t i = 0; i < length; i++)
	{
		hash ^= static_cast<uint8_t>(key[i]);
		hash *= 16777619;
	}
	return hash;
}

ObjString* takeString(char* chars, const size_t length)
{
	const uint32_t hash = hashString(chars, length);
	ObjString* interned = vm.strings.findString(chars, length, hash);
	if (interned != nullptr)
	{
		FREE_ARRAY(char, chars, length + 1);
		return interned;
	}
	return allocateString(chars, length, hash);
}

ObjString* copyString(const char* chars, size_t length)
{
	uint32_t hash = hashString(chars, length);
	ObjString* interned = vm.strings.findString(chars, length, hash);
	if (interned != nullptr) { return interned; }

	char* heapChars = ALLOCATE(char, length + 1);
	memcpy(heapChars, chars, length);
	heapChars[length] = '\0';
	return allocateString(heapChars, length, hash);
}

void printObject(Value value)
{
	switch (OBJ_TYPE(value))
	{
	case OBJ_STRING:
		printf("%s", AS_CSTRING(value));
		break;
	default:
		printf("Unknown Object, could not print.");
		break;
	}
}
