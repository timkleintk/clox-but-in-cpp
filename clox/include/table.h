#pragma once

#include "common.h"
#include "value.h"




typedef struct {
	ObjString* key;
	Value value;
} Entry;

class Table {
public:
	Table() = default;
	~Table();

	// operations
	bool get(const ObjString* key, Value* out_value) const;
	// returns true if the key is new to the table
	bool set(ObjString* key, Value value);
	bool del(const ObjString* key);
	void addAll(Table& to) const;

	ObjString* findString(const char* chars, size_t length, uint32_t hash) const;
	void removeWhite();
	void mark();
	// getters
	size_t count() const { return m_count; }
	size_t capacity() const { return m_capacity; }

private:

	void adjustCapacity(size_t capacity);


	size_t m_count = 0;
	size_t m_capacity = 0;
	Entry* m_entries = nullptr;
};


