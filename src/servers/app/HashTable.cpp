/*
 * Copyright 2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "HashTable.h"

#include <new>

#include <stdlib.h>
//#include <stdarg.h>
#include <string.h>

using std::nothrow;

struct HashTable::entry {
	entry*		next;
	Hashable*	value;
};


HashTable::HashTable(bool owning, int32 capacity, float loadFactor)
	:
	fTable(NULL),
	fCount(0),
	fThreshold(0),
	fOwning(owning)
{
	if (capacity < 10)
		capacity = 10;
	if (loadFactor <= 0.3)
		loadFactor = 0.3;

	fLoadFactor = loadFactor;
	fCapacity = capacity;
}


HashTable::~HashTable()
{
	MakeEmpty(fOwning);
}


void
HashTable::MakeEmpty(bool deleteValues)
{
	if (fTable == NULL)
		return;

	for (int32 index = fCapacity; --index >= 0;) {
		struct entry *entry, *next;

		for (entry = fTable[index]; entry != NULL; entry = next) {
			next = entry->next;

			if (deleteValues)
				delete entry->value;
			delete entry;
		}
	}

	free(fTable);
	fTable = NULL;
}


Hashable *
HashTable::GetValue(Hashable& key) const
{
	struct entry* entry = _GetHashEntry(key);

	return entry != NULL ? entry->value : NULL;
}


bool
HashTable::AddItem(Hashable *value)
{
	struct entry *entry = _GetHashEntry(*value);
	int32 hash = value->Hash();
	int32 index;

	// already in hash?
	if (entry != NULL)
		return true;

	if (fCount >= fThreshold)
		_Rehash();

	index = hash % fCapacity;

	entry = new (nothrow) HashTable::entry;
	if (entry == NULL)
		return false;

	entry->value = value;
	entry->next = fTable[index];
	fTable[index] = entry;
	fCount++;
	return true;
}


Hashable *
HashTable::RemoveItem(Hashable& key)
{
	struct entry* previous = NULL;
	struct entry* entry;
	uint32 hash;
	int32 index;

	if (fTable == NULL)
		return NULL;

	hash = key.Hash();
	index = hash % fCapacity;

	for (entry = fTable[index]; entry != NULL; entry = entry->next) {
		if (entry->value->Hash() == hash && entry->value->CompareTo(key)) {
			// found value in array
			Hashable* value;

			if (previous)
				previous->next = entry->next;
			else
				fTable[index] = entry->next;

			fCount--;
			value = entry->value;
			delete entry;
			return value;
		}

		previous = entry;
	}
	return NULL;
}


bool
HashTable::_Rehash()
{
	struct entry** newTable;
	int32 oldCapacity = fCapacity;
	int32 newCapacity, i;

	if (fCount != 0)
		newCapacity = oldCapacity * 2 + 1;
	else
		newCapacity = fCapacity;

	newTable = (struct entry **)malloc(newCapacity * sizeof(struct entry *));
	if (newTable == NULL)
		return false;

	memset(newTable, 0, newCapacity * sizeof(struct entry *));

	if (fTable != NULL) {
		// repopulate the entries into the new array
		for (i = fCapacity; i-- > 0;) {
			struct entry* entry;
			struct entry* next;

			for (entry = fTable[i]; entry != NULL; entry = next) {
				next = entry->next;

				int32 index = entry->value->Hash() % newCapacity;
				entry->next = newTable[index];
				newTable[index] = entry;
			}
		}

		free(fTable);
	}

	fTable = newTable;
	fCapacity = newCapacity;
	fThreshold = int32(newCapacity * fLoadFactor);

	return true;
}


struct HashTable::entry *
HashTable::_GetHashEntry(Hashable& key) const
{
	struct entry* entry;
	uint32 hash = key.Hash();

	if (fTable == NULL)
		return NULL;

	for (entry = fTable[hash % fCapacity]; entry != NULL; entry = entry->next) {
		if (entry->value->Hash() == hash && entry->value->CompareTo(key))
			return entry;
	}

	return NULL;
}

