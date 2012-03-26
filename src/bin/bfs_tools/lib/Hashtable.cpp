/* Hashtable - a general purpose hash table
**
** Copyright 2001 pinc Software. All Rights Reserved.
*/


#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "Hashtable.h"


/************************** standard string hash functions **************************/


unsigned int stringHash(const char *c)
{
	int len = strlen(c);

	return(*(int *)(c + len - 4));  // erstmal zum Testen
}


bool stringCompare(const char *a, const char *b)
{
	return(!strcmp(a, b));
}


// #pragma mark - Hashtable


Hashtable::Hashtable(int capacity, float loadFactor)
	:
	fIteratorIndex(-1)
{
	if (capacity < 0 || loadFactor <= 0)
		return;

	if (!capacity)
		capacity = 1;
	
	if (!(fTable = (struct Entry **)malloc(capacity * sizeof(void *))))
		return;
	memset(fTable,0,capacity * sizeof(void *));
	
	fThreshold = (int)(capacity * loadFactor);
	fModCount = 0;
	fLoadFactor = loadFactor;
	fCapacity = capacity;

	fHashFunc = (uint32 (*)(const void *))stringHash;
	fCompareFunc = (bool (*)(const void *, const void *))stringCompare;
}


Hashtable::~Hashtable()
{
	struct Entry **table = fTable;
	
	for(int32 index = fCapacity;--index >= 0;)
	{
		struct Entry *entry,*next;

		for(entry = table[index];entry;entry = next)
		{
			next = entry->next;
			delete entry;
		}
	}
	free(table);
}


void Hashtable::SetHashFunction(uint32 (*func)(const void *))
{
	fHashFunc = func;
}


void Hashtable::SetCompareFunction(bool (*func)(const void *, const void *))
{
	fCompareFunc = func;
}


bool Hashtable::IsEmpty() const
{
	return fCount == 0;
}


bool Hashtable::ContainsKey(const void *key)
{
	return GetHashEntry(key) ? true : false;
}


void *Hashtable::GetValue(const void *key)
{
	Entry *entry = GetHashEntry(key);

	return entry ? entry->value : NULL;
}


bool Hashtable::Put(const void *key, void *value)
{
	Entry *entry = GetHashEntry(key);
	int hash = fHashFunc(key);
	int index;

	if (entry)
		return true;

	fModCount++;
	if (fCount >= fThreshold)
		Rehash();

	index = hash % fCapacity;

	fTable[index] = new Entry(fTable[index], key, value);
	fCount++;

	return true;
}


void *Hashtable::Remove(const void *key)
{
	Entry **table,*entry,*prev;
	uint32 hash,(*func)(const void *);
	int32 index;

	table = fTable;
	hash = (func = fHashFunc)(key);
	index = hash % fCapacity;
	
	for(entry = table[index],prev = NULL;entry;entry = entry->next)
	{
		if ((func(entry->key) == hash) && fCompareFunc(entry->key,key))
		{
			void *value;

			fModCount++;
			if (prev)
				prev->next = entry->next;
			else
				table[index] = entry->next;
			
			fCount--;
			value = entry->value;
			delete entry;

			return value;
		}
		prev = entry;
	}
	return NULL;
}


status_t Hashtable::GetNextEntry(void **value)
{
	if (fIteratorIndex == -1)
	{
		fIteratorIndex = 0;
		fIteratorEntry = fTable[0];
	}
	else if (fIteratorEntry)
		fIteratorEntry = fIteratorEntry->next;

	// get next filled slot
	while (!fIteratorEntry && fIteratorIndex + 1 < fCapacity)
		fIteratorEntry = fTable[++fIteratorIndex];

	if (fIteratorEntry)
	{
		*value = fIteratorEntry->value;
		return B_OK;
	}
	
	return B_ENTRY_NOT_FOUND;
}


void Hashtable::Rewind()
{
	fIteratorIndex = -1;
}


void
Hashtable::MakeEmpty(int8 keyMode = HASH_EMPTY_NONE,int8 valueMode = HASH_EMPTY_NONE)
{
	fModCount++;

	for (int32 index = fCapacity; --index >= 0;) {
		Entry *entry, *next;

		for (entry = fTable[index]; entry; entry = next) {
			switch (keyMode) {
				case HASH_EMPTY_DELETE:
					// TODO: destructors are not called!
					delete (void*)entry->key;
					break;
				case HASH_EMPTY_FREE:
					free((void*)entry->key);
					break;
			}
			switch (valueMode) {
				case HASH_EMPTY_DELETE:
					// TODO: destructors are not called!
					delete entry->value;
					break;
				case HASH_EMPTY_FREE:
					free(entry->value);
					break;
			}
			next = entry->next;
			delete entry;
		}
		fTable[index] = NULL;
	}
	fCount = 0;
}


/** The hash table will be doubled in size, and rebuild.
 *  @return true on success
 */
 
bool Hashtable::Rehash()
{
	uint32 (*hashCode)(const void *) = fHashFunc;
	struct Entry **oldTable = fTable,**newtable;
	int oldCapacity = fCapacity;
	int newCapacity,i;

	newCapacity = oldCapacity * 2 + 1;
	if (!(newtable = (struct Entry **)malloc(newCapacity * sizeof(struct Entry *))))
		return false;
	memset(newtable,0,newCapacity*sizeof(struct Entry *));

	fModCount++;
	fThreshold = (int)(newCapacity * fLoadFactor);
	fTable = newtable;
	fCapacity = newCapacity;

	for (i = oldCapacity;i-- > 0;) {
		Entry *oldEntry,*entry = NULL;
		int index;

		for (oldEntry = oldTable[i];oldEntry;) {
			entry = oldEntry;  oldEntry = oldEntry->next;

			index = hashCode(entry->key) % newCapacity;
			entry->next = newtable[index];
			newtable[index] = entry;
		}
	}
	free(oldTable);

	return true;
}


Hashtable::Entry *Hashtable::GetHashEntry(const void *key)
{
	Entry **table,*entry;
	uint32 hash,(*func)(const void *);
	
	table = fTable;
	hash = (func = fHashFunc)(key);
	
	for(entry = table[hash % fCapacity];entry;entry = entry->next)
	{
		if ((func(entry->key) == hash) && fCompareFunc(entry->key,key))
			return entry;
	}
	return NULL;
}

