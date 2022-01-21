#pragma once

#include "common.h"

#define GROW_CAPACITY(capacity) \
	((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(type, pointer, oldCount, newCount) \
	(type*)reallocate(pointer, sizeof(type) * (oldCount), \
	sizeof(type) * (newCount))

template<typename T1, typename T2, typename T3>
constexpr auto FREE_ARRAY(T1 type, T2  pointer, T3  oldCount) { return reallocate(pointer, sizeof(type) * (oldCount), 0); }

void* reallocate(void* pointer, size_t oldSize, size_t newSize);

