#pragma once

#include "common.h"
#include "value.h"




typedef struct {
	ObjString* key;
	Value value;
} Entry;

typedef struct {
	size_t count;
	size_t capacity;
	Entry* entries;
} Table;

// TODO: turn into Table class

void initTable(Table* table);
void freeTable(Table* table);
bool tableGet(Table* table, ObjString* key, Value* value);
bool tableSet(Table* table, ObjString* key, Value value);
bool tableDelete(Table* table, ObjString* key);
void tableAddAll(const Table* from, Table* to);
ObjString* tableFindString(Table* table, const char* chars, size_t length, uint32_t hash);
