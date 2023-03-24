#include "table.h"

#include <cstdlib>
#include <cstring>

#include "memory.h"
#include "object.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75


Table::~Table()
{
	FREE_ARRAY(Entry, m_entries, m_capacity);
}


// utility function ------------------------------------------------

static Entry* findEntry(Entry* entries, const size_t capacity, const ObjString* key)
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


// operations ------------------------------------------------------

bool Table::get(const ObjString* key, Value* out_value) const
{
	if (m_count == 0) return false;

	const Entry* entry = findEntry(m_entries, m_capacity, key);
	if (entry->key == nullptr) return false;

	// nts: why not return the value pointer and nullptr? I guess because we want to return a copy -> return const pointer?
	*out_value = entry->value;
	return true;
}

bool Table::set(ObjString* key, Value value)
{
	if (m_count + 1 > static_cast<size_t>(m_capacity * TABLE_MAX_LOAD))
	{
		const size_t capacity = GROW_CAPACITY(m_capacity);
		adjustCapacity(capacity);
	}
	Entry* entry = findEntry(m_entries, m_capacity, key);
	const bool isNewKey = entry->key == nullptr;
	if (isNewKey && IS_NIL(entry->value)) { m_count++; } // don't increment the count on tombstone usage

	entry->key = key;
	entry->value = value;
	return isNewKey;
}

bool Table::del(const ObjString* key) const
{
	if (m_count == 0) return false;

	// find the entry
	Entry* entry = findEntry(m_entries, m_capacity, key);
	if (entry->key == nullptr) return false;

	// place a tombstone in the entry
	entry->key = nullptr;
	entry->value = BOOL_VAL(true);

	return true;
}

// nts: should this take the argument "to" or "from"?
void Table::addAll(Table& to) const
{
	for (size_t i = 0; i < m_capacity; i++)
	{
		const auto& [key, value] = m_entries[i];
		if (key != nullptr)
		{
			to.set(key, value);
		}
	}
}

ObjString* Table::findString(const char* chars, size_t length, uint32_t hash) const
{
	if (m_count == 0) return nullptr;

	uint32_t index = hash % m_capacity;
	for (;;)
	{
		const Entry* entry = &m_entries[index];
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

		index = (index + 1) % m_capacity;
	}
}



void Table::adjustCapacity(size_t capacity)
{
	// allocate new entry array
	auto entries = ALLOCATE(Entry, capacity);

	// initialize values
	for (size_t i = 0; i < capacity; i++)
	{
		entries[i].key = nullptr;
		entries[i].value = NIL_VAL;
	}

	m_count = 0;

	for (size_t i = 0; i < m_capacity; i++)
	{
		const Entry* src = &m_entries[i];
		if (src->key == nullptr) continue;

		Entry* dest = findEntry(entries, capacity, src->key);
		dest->key = src->key;
		dest->value = src->value;
		m_count++;

	}

	FREE_ARRAY(Entry, m_entries, m_capacity);
	m_entries = entries;
	m_capacity = capacity;
}

