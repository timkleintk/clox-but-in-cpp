#include "chunk.h"
#include "common.h"

int main(int, const char*[])
{
	Chunck chunck;
	chunck.init();
	chunck.write(OP_RETURN);
	chunck.free();


	return 0;
}
