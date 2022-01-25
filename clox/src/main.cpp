#include "chunk.h"
#include "common.h"
#include "debug.h"


int main(int, const char*[])
{
	Chunk chunk;

	const size_t constant = chunk.addConstant(1.2);

	chunk.writeCode(OP_CONSTANT, 123);
	chunk.writeCode(constant, 123);

	chunk.writeCode(OP_RETURN, 123);

	chunk.disassemble("test chunk");


	(void)getchar();
	return 0;
}
