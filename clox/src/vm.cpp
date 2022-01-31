#include "vm.h"

#include "chunk.h"
#include "compiler.h"

namespace
{
	VM vm;
}

static void resetStack()
{
	vm.stackTop = &vm.stack[0];
}

void initVM()
{
	resetStack();
}

void freeVM()
{}

[[maybe_unused]] static InterpretResult run()
{
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants[READ_BYTE()])
#define BINARY_OP(op) \
	do \
	{ \
		double b = pop(); \
		double a = pop(); \
		push (a op b); \
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
		case OP_ADD:		BINARY_OP(+); break;
		case OP_SUBTRACT:	BINARY_OP(-); break;
		case OP_MULTIPLY:	BINARY_OP(*); break;
		case OP_DIVIDE:		BINARY_OP(/); break;
		case OP_NEGATE:		push(-pop()); break;
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
	compile(source);
	return INTERPRET_OK;
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

