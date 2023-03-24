#include "table.h"

#include <cstdlib>
#include <cstring>

#include "memory.h"
#include "object.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

void initTable(Table* table)
{
	table->count = 0;
	table->capacity = 0;
	table->entries = nullptr;
}

void freeTable(Table* table)
{
	FREE_ARRAY(Entry, table->entries, table->capacity);
	initTable(table);
}

static Entry* findEntry(Entry* entries, size_t capacity, const ObjString* key)
{
	uint32_t index = key->hash % capacity;
	Entry* tombstone = nullptr;

	for (;;)
	{
		Entry* entry = &entries[index];
		if (entry->key == nullptr)
		{
			if (IS_NIL(entry->value))
			{
				// empty entry
				return tombstone != nullptr ? tombstone : entry; // if we encountered a tombstone return that slot instead of the empty
			}

			// remember any tombstones that we pass
			if (tombstone == nullptr) tombstone = entry;
		}
		else if (entry->key == key)
		{
			// we found the key
			return entry;
		}
		index = (index + 1) % capacity;
	}
}

bool tableGet(Table* table, ObjString* key, Value* value)
{
	if (table->count == 0) return false;

	const Entry* entry = findEntry(table->entries, table->capacity, key);
	if (entry->key == nullptr) return false;

	*value = entry->value; // nts: why not return the value pointer and nullptr? I guess because we want to return a copy -> return const pointer?
	return true;
}

static void adjustCapacity(Table* table, size_t capacity)
{
	Entry* entries = ALLOCATE(Entry, capacity);
	for (size_t i = 0; i < capacity; i++)
	{
		entries[i].key = nullptr;
		entries[i].value = NIL_VAL;
	}

	table->count = 0;

	for (size_t i = 0; i < table->capacity; i++)
	{
		const Entry* entry = &table->entries[i];
		if (entry->key == nullptr) continue;

		Entry* dest = findEntry(entries, capacity, entry->key);
		dest->key = entry->key;
		dest->value = entry->value;
		table->count++;

	}

	FREE_ARRAY(Entry, table->entries, table->capacity);
	table->entries = entries;
	table->capacity = capacity;
}

bool tableSet(Table* table, ObjString* key, Value value)
{
	if (table->count + 1 > table->capacity * TABLE_MAX_LOAD)
	{
		const size_t capacity = GROW_CAPACITY(table->capacity);
		adjustCapacity(table, capacity);
	}
	Entry* entry = findEntry(table->entries, table->capacity, key);
	const bool isNewKey = entry->key == nullptr;
	if (isNewKey && IS_NIL(entry->value)) { table->count++; } // don't increment the count on tombstone usage

	entry->key = key;
	entry->value = value;
	return isNewKey;
}

bool tableDelete(Table* table, ObjString* key)
{
	if (table->count == 0) return false;

	// find the entry
	Entry* entry = findEntry(table->entries, table->capacity, key);
	if (entry->key == nullptr) return false;

	// place a tombstone in the entry
	entry->key = nullptr;
	entry->value = BOOL_VAL(true);
	return true;
}

ObjString* tableFindString(Table* table, const char* chars, size_t length, uint32_t hash)
{
	if (table->count == 0) return nullptr;

	uint32_t index = hash % table->capacity;
	for (;;)
	{
		const Entry* entry = &table->entries[index];
		if (entry->key == nullptr)
		{
			// stop if we find an empty non-tombstone entry
			if (IS_NIL(entry->value)) return nullptr;
		}
		else if (entry->key->length == length && entry->key->hash == hash && memcmp(entry->key->chars, chars, length) == 0)
		{
			// we found it
			return entry->key;
		}

		index = (index + 1) % table->capacity;
	}
}

void tableAddAll(Table* from, Table* to)
{
	for (size_t i = 0; i < from->capacity; i++)
	{
		const Entry* entry = &from->entries[i];
		if (entry->key != nullptr)
		{
			tableSet(to, entry->key, entry->value);
		}
	}
}
