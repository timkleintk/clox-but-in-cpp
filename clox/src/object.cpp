#include "object.h"

#include <cstdio>
#include <cstring>

#include "memory.h"
#include "table.h"
#include "util.h"
#include "value.h"
#include "vm.h"



#define ALLOCATE_OBJ(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)

static Obj* allocateObject(size_t size, ObjType type)
{
	Obj* object = (Obj*)reallocate(nullptr, 0, size);
	object->type = type;
	object->isMarked = false;

	object->next = vm.objects;
	vm.objects = object;

#ifdef DEBUG_LOG_GC
	grey();
	printf("%p ", static_cast<void*>(object));
	white();
	printf("allocate %zu for %s\n", size, ObjTypeNames[type]);
#endif

	return object;
}

ObjClosure* newClosure(ObjFunction* function)
{
	ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue*, function->upvalueCount);
	for (int i = 0; i < function->upvalueCount; i++)
	{
		upvalues[i] = nullptr;
	}
	ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
	closure->function = function;
	closure->upvalues = upvalues;
	closure->upvalueCount = function->upvalueCount;
	return closure;
}

ObjFunction* newFunction()
{
	auto* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
	//auto* function = new(mem) ObjFunction(); this fucks up the object type enum
	//assert(mem == function);
	function->arity = 0;
	function->upvalueCount = 0;
	function->name = nullptr;
	auto* ptr = new (&function->chunk) Chunk();
	assert(ptr == &function->chunk);
	return function;
}

ObjNative* newNative(NativeFn function)
{
	ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
	native->function = function;
	return native;
}

static ObjString* allocateString(char* chars, size_t length, uint32_t hash)
{
	ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
	string->length = length;
	string->chars = chars;
	string->hash = hash;

	push(OBJ_VAL(string));
	vm.strings.set(string, NIL_VAL);
	pop();

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

ObjUpvalue* newUpvalue(Value* slot)
{
	ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
	upvalue->closed = NIL_VAL;
	upvalue->location = slot;
	upvalue->next = nullptr;
	return upvalue;
}

static void printFunction(ObjFunction* function)
{
	if (function->name == nullptr)
	{
		printf("<script>");
		return;
	}
	printf("<fn %s>", function->name->chars);
}

void printObject(Value value)
{
	switch (OBJ_TYPE(value))
	{
	case OBJ_CLOSURE:
		printFunction(AS_CLOSURE(value)->function);
		break;
	case OBJ_FUNCTION:
		printFunction(AS_FUNCTION(value));
		break;
	case OBJ_NATIVE:
		printf("<native fn>");
		break;
	case OBJ_STRING:
		printf("%s", AS_CSTRING(value));
		break;
	case OBJ_UPVALUE:
		printf("upvalue");
		break;
	default:
		printf("Unknown Object, could not print.");
		break;
	}
}
