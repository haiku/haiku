/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */
#ifndef _KERNEL_UTIL_MULTI_HASH_TABLE_H
#define _KERNEL_UTIL_MULTI_HASH_TABLE_H


#include <KernelExport.h>
#include <util/kernel_cpp.h>
#include <util/OpenHashTable.h>


// MultiHashTable is a container which acts a bit like multimap<>
// but with hash table semantics.

// refer to OpenHashTable.h for how to use this container.

template<typename Definition, bool AutoExpand = true,
	bool CheckDuplicates = false>
class MultiHashTable : private BOpenHashTable<Definition,
	AutoExpand, CheckDuplicates> {
public:
	typedef BOpenHashTable<Definition, AutoExpand, CheckDuplicates> HashTable;
	typedef MultiHashTable<Definition, AutoExpand, CheckDuplicates> MultiTable;

	typedef typename HashTable::Iterator Iterator;
	typedef typename Definition::KeyType KeyType;
	typedef typename Definition::ValueType ValueType;

	MultiHashTable()
		: HashTable() {}

	MultiHashTable(const Definition& definition)
		: HashTable(definition) {}

	status_t Init(size_t initialSize = HashTable::kMinimumSize)
	{
		return HashTable::Init(initialSize);
	}

	void Insert(ValueType *value)
	{
		if (AutoExpand
			&& HashTable::fItemCount >= (HashTable::fTableSize * 200 / 256))
			_Resize(HashTable::fTableSize * 2);

		InsertUnchecked(value);
	}

	void InsertUnchecked(ValueType *value)
	{
		_Insert(HashTable::fTable, HashTable::fTableSize, value);
		HashTable::fItemCount++;
	}

	bool Remove(ValueType *value)
	{
		if (!HashTable::RemoveUnchecked(value))
			return false;

		if (AutoExpand && HashTable::fTableSize > HashTable::kMinimumSize
			&& HashTable::fItemCount < (HashTable::fTableSize * 50 / 256))
			_Resize(HashTable::fTableSize / 2);

		return true;
	}

	Iterator GetIterator() const { return HashTable::GetIterator(); }

	class ValueIterator : protected Iterator {
	public:
		ValueIterator(const HashTable *table, size_t index, ValueType *value)
			: fOriginalIndex(index), fOriginalValue(value)
		{
			Iterator::fTable = table;
			Rewind();
		}

		bool HasNext() const
		{
			if (Iterator::fNext == NULL)
				return false;
			if (Iterator::fNext == fOriginalValue)
				return true;
			return ((const MultiTable *)Iterator::fTable)->_Definition().CompareValues(
				fOriginalValue, Iterator::fNext);
		}

		void Rewind()
		{
			Iterator::fIndex = fOriginalIndex + 1;
			Iterator::fNext = fOriginalValue;
		}

		ValueType *Next() { return Iterator::Next(); }

	private:
		size_t fOriginalIndex;
		ValueType *fOriginalValue;
	};

	ValueIterator Lookup(const KeyType &key) const
	{
		size_t index = 0;
		ValueType *slot = NULL;
		if (HashTable::fTableSize > 0) {
			index = HashTable::fDefinition.HashKey(key)
				& (HashTable::fTableSize - 1);
			slot = HashTable::fTable[index];
		}

		while (slot) {
			if (HashTable::fDefinition.Compare(key, slot))
				break;
			slot = HashTable::_Link(slot);
		}

		if (slot == NULL)
			return ValueIterator(this, HashTable::fTableSize, NULL);

		return ValueIterator(this, index, slot);
	}

private:
	// for g++ 2.95
	friend class ValueIterator;

	const Definition &_Definition() const { return HashTable::fDefinition; }

	void _Insert(ValueType **table, size_t tableSize, ValueType *value)
	{
		size_t index = HashTable::fDefinition.Hash(value) & (tableSize - 1);

		ValueType *previous;

		// group values with the same key
		for (previous = table[index]; previous
				&& !HashTable::fDefinition.CompareValues(previous, value);
				previous = HashTable::_Link(previous));

		if (previous) {
			HashTable::_Link(value) = HashTable::_Link(previous);
			HashTable::_Link(previous) = value;
		} else {
			HashTable::_Link(value) = table[index];
			table[index] = value;
		}
	}

	// TODO use BOpenHashTable's _Resize
	bool _Resize(size_t newSize)
	{
		ValueType **newTable = new ValueType *[newSize];
		if (newTable == NULL)
			return false;

		for (size_t i = 0; i < newSize; i++)
			newTable[i] = NULL;

		if (HashTable::fTable) {
			for (size_t i = 0; i < HashTable::fTableSize; i++) {
				ValueType *bucket = HashTable::fTable[i];
				while (bucket) {
					ValueType *next = HashTable::_Link(bucket);
					_Insert(newTable, newSize, bucket);
					bucket = next;
				}
			}

			delete [] HashTable::fTable;
		}

		HashTable::fTableSize = newSize;
		HashTable::fTable = newTable;
		return true;
	}
};

#endif	// _KERNEL_UTIL_MULTI_HASH_TABLE_H
