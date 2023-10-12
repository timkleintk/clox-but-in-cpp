#include "memory.h"

#include <cstdlib>

#include "object.h"
#include "value.h"
#include "vm.h"

void* reallocate(void* pointer, size_t, const size_t newSize)
{
	if (newSize == 0)
	{
		free(pointer);
		return nullptr;
	}

	void* result = realloc(pointer, newSize);
	if (result == nullptr) { exit(1); }
	return result;
}

static void freeObject(Obj* object)
{
	switch (object->type)
	{
	case OBJ_CLOSURE:
	{
		ObjClosure* closure = reinterpret_cast<ObjClosure*>(object);
		FREE_ARRAY(ObjUpvalue*, closure->upvalues, closure->upvalueCount);
		FREE(ObjClosure, object);
		break;
	}
	case OBJ_FUNCTION:
	{
		// todo: make sure this is correct, especially the name field
		ObjFunction* function = reinterpret_cast<ObjFunction*>(object);
		function->chunk.~Chunk();
		FREE(ObjFunction, object);
		break;
	}
	case OBJ_NATIVE:
	{
		FREE(ObjNative, object);
		break;
	}
	case OBJ_STRING:
	{
		ObjString* string = reinterpret_cast<ObjString*>(object);
		FREE_ARRAY(char, string->chars, string->length + 1);
		FREE(ObjString, object);
		break;
	}
	case OBJ_UPVALUE:
	{
		FREE(ObjUpvalue, object);
		break;
	}
	}
}

void freeObjects()
{
	Obj* object = vm.objects;
	while (object != nullptr)
	{
		Obj* next = object->next;
		freeObject(object);
		object = next;
	}
}
