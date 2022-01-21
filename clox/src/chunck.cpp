#include "chunk.h"

#include "memory.h"

Chunck::Chunck()
{
	
}

Chunck::~Chunck()
{
	free();
}


void Chunck::init()
{
	count = 0;
	capacity = 0;
	code = nullptr;
}

void Chunck::free()
{
	FreeArray(code, capacity);
	init();
}

void Chunck::write(uint8_t byte)
{
	if (capacity < count + 1)
	{
		const size_t oldCapacity = capacity;
		capacity = GrowCapacity(oldCapacity);
		code = GrowArray(code, oldCapacity, capacity);
	}

	code[count++] = byte;
}

