#include "vm.h"

#include <cstdarg>
//#include <cstring> // book says I need this, but I don't

#include "chunk.h"
#include "compiler.h"
#include "object.h"

VM vm;

static void resetStack()
{
	vm.stackTop = &vm.stack[0];
}

static void runtimeError(const char* format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fputs("\n", stderr);

	// nts: the book says to write this, but I fucked around so I have to do something else, possible BUG!
	// size_t instruction = vm.ip - vm.chunk->code - 1;
	// size_t instruction = vm.ip - reinterpret_cast<uint8_t*>( & (vm.chunk->code) - 1);
	size_t instruction = vm.ip - &vm.chunk->code[0] - 1; // index of the instruction within the current chunk (?)
	int line = static_cast<int>(vm.chunk->lines[instruction]);
	fprintf(stderr, "[line %d] in script\n", line);
	resetStack();

}

void initVM()
{
	resetStack();
	vm.objects = nullptr;
}

void freeVM()
{
	freeObjects();
}

static Value peek(int distance) {
	return vm.stackTop[-1 - distance];
}

static bool isFalsey(Value value) {
	return IS_NIL(value) || IS_BOOL(value) && value.as.boolean;
}

static void concatenate()
{
	ObjString* b = AS_STRING(pop());
	ObjString* a = AS_STRING(pop());

	size_t length = a->length + b->length;
	char* chars = ALLOCATE(char, length + 1);
	memcpy(chars, a->chars, a->length);
	memcpy(chars + a->length, b->chars, b->length);
	chars[length] = '\0';

	ObjString* result = takeString(chars, length);
	push(OBJ_VAL(result));
}

[[maybe_unused]] static InterpretResult run()
{
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants[READ_BYTE()])
#define BINARY_OP(valueType, op) \
	do { \
		if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
			runtimeError("Operands must be numbers."); \
			return INTERPRET_RUNTIME_ERROR;\
		} \
		double b = AS_NUMBER(pop()); \
		double a = AS_NUMBER(pop()); \
		push (valueType(a op b)); \
	} while  (false)

	for (;;)
	{
#ifdef DEBUG_TRACE_EXECUTION
		printf("          ");
		for (Value* slot = &vm.stack[0]; slot < vm.stackTop; slot++)
		{
			printf("[ ");
			printValue(*slot);
			printf(" ]");
		}
		printf("\n");
		vm.chunk->disassembleInstruction(vm.ip - &vm.chunk->code[0]);
#endif

		switch (Op instruction = static_cast<Op>(READ_BYTE()))
		{
		case OP_CONSTANT:
		{
			const Value constant = READ_CONSTANT();
			push(constant);
			break;
		}
		case OP_NIL: push(NIL_VAL); break;
		case OP_TRUE: push(BOOL_VAL(true)); break;
		case OP_FALSE: push(BOOL_VAL(false)); break;
		case OP_EQUAL:
			Value r = pop();
			Value l = pop();
			push(BOOL_VAL(valuesEqual(l, r)));
			break;
		case OP_GREATER:	BINARY_OP(BOOL_VAL, > ); break;
		case OP_LESS:		BINARY_OP(BOOL_VAL, < ); break;

		case OP_ADD: {
			if (IS_STRING(peek(0)) && IS_STRING(peek(1)))
			{
				concatenate();
			}
			else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1)))
			{
				double b = AS_NUMBER(pop());
				double a = AS_NUMBER(pop());
				push(NUMBER_VAL(a + b));
			}
			else
			{
				runtimeError("Operands must be two numbers or two strings.");
				return INTERPRET_RUNTIME_ERROR;
			}
			break;
		}
		case OP_SUBTRACT:	BINARY_OP(NUMBER_VAL, -); break;
		case OP_MULTIPLY:	BINARY_OP(NUMBER_VAL, *); break;
		case OP_DIVIDE:		BINARY_OP(NUMBER_VAL, / ); break;
		case OP_NOT:
			push(BOOL_VAL(isFalsey(pop())));
			break;
		case OP_NEGATE:
			if (!IS_NUMBER(peek(0))) {
				runtimeError("Operand must be a number.");
				return INTERPRET_RUNTIME_ERROR;
			}
			push(NUMBER_VAL(-AS_NUMBER(pop())));
			break;
		case OP_RETURN:
		{
			printValue(pop());
			printf("\n");
			return INTERPRET_OK;
		}
		default:
			break;
		}
	}
#undef READ_CONSTANT
#undef READ_BYTE
}

InterpretResult interpret(const std::string& source)
{
	Chunk chunk;

	if (!compile(source, chunk))
	{
		return INTERPRET_COMPILE_ERROR;
	}

	vm.chunk = &chunk;
	vm.ip = &vm.chunk->code[0];

	const InterpretResult result = run();

	return result;
}

void push(Value value)
{
	*vm.stackTop = value; // nts: move?
	vm.stackTop++;
}

Value pop()
{
	vm.stackTop--;
	return *vm.stackTop;
}


