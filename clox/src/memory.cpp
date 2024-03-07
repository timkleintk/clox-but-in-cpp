#include "memory.h"

#include <cstdlib>
#include <Windows.h>

#include "compiler.h"
#include "object.h"
#include "util.h"
#include "value.h"
#include "vm.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif

#define GC_HEAP_GROW_FACTOR 2;

void* reallocate(void* pointer, size_t oldSize, size_t newSize)
{
	vm.bytesAllocated += newSize - oldSize;
	if (newSize > oldSize)
	{
#ifdef DEBUG_STRESS_GC
		collectGarbage();
#else
		if (vm.bytesAllocated > vm.nextGC)
		{
			collectGarbage();
		}
#endif
	}

	if (newSize == 0)
	{
		free(pointer);
		return nullptr;
	}

	void* result = realloc(pointer, newSize);
	if (result == nullptr) { exit(1); }
	return result;
}

void markObject(Obj* object)
{
	if (object == nullptr) return;
	if (object->isMarked) return;
#ifdef DEBUG_LOG_GC
	printf("%p mark ", static_cast<void*>(object));
	printValue(OBJ_VAL(object));
	printf("\n");
#endif
	object->isMarked = true;

	if (vm.grayCapacity < vm.grayCount + 1)
	{
		vm.grayCapacity = GROW_CAPACITY(vm.grayCapacity);
		vm.grayStack = static_cast<Obj**>(realloc(vm.grayStack, sizeof(Obj*) * vm.grayCapacity));
		if (vm.grayStack == nullptr) exit(1);
	}

	vm.grayStack[vm.grayCount++] = object;
}

void markValue(Value value)
{
	if (IS_OBJ(value)) markObject(AS_OBJ(value));
}

static void markArray(Blob<Value>& array)
{
	for (size_t i = 0; i < array.size(); i++)
	{
		markValue(array[i]);
	}
}

static void blackenObject(Obj* object)
{
#ifdef DEBUG_LOG_GC
	printf("%p blacken ", reinterpret_cast<void*>(object));
	printValue(OBJ_VAL(object));
	printf("\n");
#endif

	switch (object->type)
	{
	case OBJ_CLASS:
	{
		ObjClass* klass = reinterpret_cast<ObjClass*>(object);
		markObject(reinterpret_cast<Obj*>(klass->name));
		break;
	}
	case OBJ_CLOSURE:
	{

		ObjClosure* closure = reinterpret_cast<ObjClosure*>(object);
		markObject(reinterpret_cast<Obj*>(closure->function));
		for (int i = 0; i < closure->upvalueCount; i++)
		{
			markObject(reinterpret_cast<Obj*>(closure->upvalues[i]));
		}
		break;
	}
	case OBJ_FUNCTION:
	{
		ObjFunction* function = reinterpret_cast<ObjFunction*>(object);
		markObject(reinterpret_cast<Obj*>(function->name));
		markArray(function->chunk.constants);
		break;
	}
	case OBJ_INSTANCE:
	{
		ObjInstance* instance = reinterpret_cast<ObjInstance*>(object);
		markObject((Obj*)instance->klass);
		instance->fields.mark();
		break;
	}
	case OBJ_UPVALUE:
		markValue(reinterpret_cast<ObjUpvalue*>(object)->closed);
		break;
	case OBJ_NATIVE:
	case OBJ_STRING:
		break;
	}
}

static void freeObject(Obj* object)
{
#ifdef DEBUG_LOG_GC
	printf("%p free type %s: ", (void*)object, ObjTypeNames[object->type]);
	printObject(OBJ_VAL(object));
	printf("\n");
#endif
	switch (object->type)
	{
	case OBJ_CLASS:
	{
		FREE(ObjClass, object);
		break;
	}
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
	case OBJ_INSTANCE:
	{
		ObjInstance* instance = reinterpret_cast<ObjInstance*>(object);
		instance->fields.~Table();
		FREE(ObjInstance, object);
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

static void markRoots()
{
	for (Value* slot = vm.stack; slot < vm.stackTop; slot++)
	{
		markValue(*slot);
	}

	for (size_t i = 0; i < vm.frameCount; i++)
	{
		markObject(reinterpret_cast<Obj*>(vm.frames[i].closure));
	}

	for (ObjUpvalue* upvalue = vm.openUpvalues; upvalue != nullptr; upvalue = upvalue->next)
	{
		markObject(reinterpret_cast<Obj*>(upvalue));
	}

	vm.globals.mark();

	markCompilerRoots();
}

static void traceReferences()
{
	while (vm.grayCount > 0)
	{
		Obj* object = vm.grayStack[--vm.grayCount];
		blackenObject(object);
	}
}

static void sweep()
{
	Obj* previous = nullptr;
	Obj* object = vm.objects;
	while (object != nullptr)
	{
		if (object->isMarked)
		{
			object->isMarked = false;
			previous = object;
			object = object->next;
		}
		else
		{
			Obj* unreached = object;
			object = object->next;
			if (previous != nullptr)
			{
				previous->next = object;
			}
			else
			{
				vm.objects = object;
			}
			freeObject(unreached);
		}
	}
}

void collectGarbage()
{
#ifdef DEBUG_LOG_GC
	grey();
	printf("-- gc begin\n");
	size_t before = vm.bytesAllocated;
#endif

	markRoots();
	traceReferences();
	vm.strings.removeWhite();
	sweep();

	vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
	printf("-- gc end\n");
	if (before != vm.bytesAllocated) white();

	printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
		before - vm.bytesAllocated, before, vm.bytesAllocated, vm.nextGC);
	white();
#endif
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

	free(vm.grayStack);
}

