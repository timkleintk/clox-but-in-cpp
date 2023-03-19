#pragma once
#include <string>

//#include "object.h" // book says I need this, but I don't think so

struct Chunk;
bool compile(const std::string& source, Chunk&);
