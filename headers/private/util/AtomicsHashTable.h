/*
 * Copyright 2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_UTIL_ATOMICS_HASH_TABLE_H
#define _KERNEL_UTIL_ATOMICS_HASH_TABLE_H


#include <util/OpenHashTable.h>
#include <util/atomic.h>


// AtomicsHashTable extends BOpenHashTable with some atomic operations.

template<typename Definition, bool AutoExpand = true,
	bool CheckDuplicates = false>
class AtomicsHashTable : public BOpenHashTable<Definition,
	AutoExpand, CheckDuplicates> {
public:
	typedef BOpenHashTable<Definition, AutoExpand, CheckDuplicates> HashTable;

	typedef typename Definition::KeyType KeyType;
	typedef typename Definition::ValueType ValueType;

	AtomicsHashTable()
		: HashTable() {}

	AtomicsHashTable(const Definition& definition)
		: HashTable(definition) {}

	/*! \brief Inserts a value atomically.

		If there's another item with an identical key, the new value will not
		be inserted, and that value will be returned instead. If there's no
		other value with the same key, the passed value will be inserted,
		and NULL will be returned.

		The caller is responsible for ensuring that no Remove()s or Resize()s
		are invoked concurrently with this method.
	*/
	ValueType* InsertAtomic(ValueType* value)
	{
		KeyType key = HashTable::fDefinition.Key(value);
		size_t index = HashTable::fDefinition.Hash(value) & (HashTable::fTableSize - 1);
		HashTable::_Link(value) = NULL;

		ValueType** link = &HashTable::fTable[index];
		while (true) {
			ValueType* existing = atomic_pointer_get(link);
			if (existing == NULL) {
				existing = atomic_pointer_test_and_set(link, value, existing);
				if (existing == NULL) {
					size_t& count = HashTable::fItemCount;
					sizeof(size_t) == 4 ?
						atomic_add((int32*)&count, 1) :
						atomic_add64((int64*)&count, 1);
					return NULL;
				}
			}

			if (HashTable::fDefinition.Compare(key, existing))
				return existing;

			link = &HashTable::_Link(existing);
		}
	}

	bool ResizeIfNeeded()
	{
		size_t resizeNeeded = HashTable::ResizeNeeded();
		if (resizeNeeded == 0)
			return true;

		return HashTable::_Resize(resizeNeeded);
	}
};


#endif	// _KERNEL_UTIL_ATOMICS_HASH_TABLE_H
