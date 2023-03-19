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
	case OBJ_STRING:
		{
		ObjString* string = reinterpret_cast<ObjString*>(object);
		FREE_ARRAY(char, string->chars, string->length + 1);
		FREE(ObjString, object);
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
