#pragma once
#include <string>

#include "object.h"

struct Chunk;
bool compile(const std::string& source, Chunk&);
