#include "chunk.h"
#include "common.h"
#include "debug.h"
#include "vm.h"


int main(const int argc, const char* argv[])
{
	initVM();

	Chunk chunk;

	chunk.writeByte(OP_CONSTANT);
	chunk.writeByte(chunk.addConstant(1.2));

	chunk.writeByte(OP_CONSTANT);
	chunk.writeByte(chunk.addConstant(3.4));

	chunk.writeByte(OP_ADD);

	chunk.writeByte(OP_CONSTANT);
	chunk.writeByte(chunk.addConstant(5.6));

	chunk.writeByte(OP_DIVIDE);

	chunk.writeByte(OP_NEGATE);

	chunk.writeByte(OP_RETURN);

	chunk.disassemble("test chunk");

	interpret(&chunk);

	freeVM();

	(void)getchar();
	return 0;
}
