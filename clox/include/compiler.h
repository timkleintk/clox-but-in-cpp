#pragma once
#include <string>

#include "object.h"
#include "vm.h"

struct Chunk;
ObjFunction* compile(const std::string& source);
