/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */


#ifndef _OPEN_HASH_TABLE_H_
#define _OPEN_HASH_TABLE_H_

#include <KernelExport.h>

// the Definition template must have three methods: `HashKey', `Hash' and
// `Compare'. It must also define several types as shown in the following
// example:
//
// struct Foo {
//	int bar;
// };
//
// struct HashTableDefinition {
//	typedef void *	ParentType;
//	typedef int		KeyType;
//	typedef	Foo		ValueType;
//
//	static size_t HashKey(void *parent, int key) { return key >> 1; }
//	static size_t Hash(void *parent, Foo *value) { return HashKey(value->bar); }
//	static bool Compare(void *parent, int key, Foo *value)
//		{ return value->bar == key; }
// };

// This hash table implementation uses open addressing vs. the more common
// chaining. This approach is advantageous as the number of expected collisions
// is the same (property of the hash function) while not wasting one additional
// word per item and having better cache locality. The usage of quadratic
// probing reduces the effectiveness of cache locality but prevents clustering.
template<typename Definition, bool CheckDuplicates = false>
class OpenHashTable {
public:
	typedef typename Definition::ParentType	ParentType;
	typedef typename Definition::KeyType	KeyType;
	typedef typename Definition::ValueType	ValueType;

	static const size_t kMinimumSize = 32;

	// we use new [] / delete [] for allocation. If in the future this
	// is revealed to be insufficient we can switch to a template based
	// allocator. All allocations are of power of 2 lengths.

	// regrowth factor: 200 / 256 = 78.125%
	//                   50 / 256 = 19.53125%

	OpenHashTable(const ParentType &parent, size_t initialSize = kMinimumSize)
		: fParent(parent), fItemCount(0), fTable(NULL),
			fDeletedToken((ValueType *)(((char *)0) - 1))
	{
		if (initialSize < kMinimumSize)
			initialSize = kMinimumSize;

		_Resize(initialSize);
	}

	~OpenHashTable()
	{
		delete [] fTable;
	}

	status_t InitCheck() const { return fTable ? B_OK : B_NO_MEMORY; }

	ValueType *Lookup(const KeyType &key) const
	{
		size_t index = Definition::HashKey(fParent, key) & (fTableSize - 1);
		size_t f = 0;

		while (true) {
			ValueType *slot = fTable[index];

			if (slot == NULL)
				return NULL;
			else if (!_IsDeleted(slot)
					&& Definition::Compare(fParent, key, slot))
				return slot;

			index = _NextSlot(f, index, fTableSize);
		}
	}

	bool Insert(ValueType *value)
	{
		if (fItemCount >= (fTableSize * 200 / 256)) {
			if (!_Resize(fTableSize * 2))
				return false;
		}

		InsertUnchecked(value);
		return true;
	}

	void InsertUnchecked(ValueType *value)
	{
		if (CheckDuplicates) {
			for (size_t i = 0; i < fTableSize; i++) {
				if (fTable[i] == value)
					panic("HashTable: item already in table");
			}
		}

		ValueType *previous = _Insert(fTable, fTableSize, value);
		if (_IsDeleted(previous))
			fDeletedCount--;
		fItemCount++;
	}

	void Remove(ValueType *value)
	{
		RemoveUnchecked(value);

		if (fTableSize > kMinimumSize && fItemCount < (fTableSize * 50 / 256))
			_Resize(fTableSize / 2);
	}

	void RemoveUnchecked(ValueType *value)
	{
		size_t index = Definition::Hash(fParent, value) & (fTableSize - 1);
		size_t f = 0;

		while (true) {
			if (fTable[index] == value) {
				fTable[index] = (ValueType *)fDeletedToken;
				break;
			}

			index = _NextSlot(f, index, fTableSize);
		}

		if (CheckDuplicates) {
			for (size_t i = 0; i < fTableSize; i++) {
				if (fTable[i] == value)
					panic("HashTable: item removed, but still in table.");
			}
		}

		fItemCount--;
		fDeletedCount++;
	}

private:
	ValueType *_Insert(ValueType **table, size_t tableSize, ValueType *value)
	{
		size_t index = Definition::Hash(fParent, value) & (tableSize - 1);
		size_t f = 0;

		while (true) {
			if (table[index] == NULL || table[index] == fDeletedToken) {
				ValueType *previous = table[index];
				table[index] = value;
				return previous;
			}

			index = _NextSlot(f, index, tableSize);
		}

		return NULL;
	}

	static size_t _NextSlot(size_t &f, size_t index, size_t tableSize)
	{
		// quadratic probing
		f++;
		return (index + f) & (tableSize - 1);
	}

	bool _IsDeleted(ValueType *value) const
	{
		return value == fDeletedToken;
	}

	bool _Resize(size_t newSize)
	{
		ValueType **newTable = new ValueType *[newSize];
		if (newTable == NULL)
			return false;

		for (size_t i = 0; i < newSize; i++)
			newTable[i] = NULL;

		if (fTable) {
			for (size_t i = 0; i < fTableSize; i++) {
				if (fTable[i] && !_IsDeleted(fTable[i]))
					_Insert(newTable, newSize, fTable[i]);
			}

			delete [] fTable;
		}

		fTableSize = newSize;
		fDeletedCount = 0;
		fTable = newTable;
		return true;
	}

	ParentType fParent;
	size_t fTableSize, fItemCount, fDeletedCount;
	ValueType **fTable;

	const ValueType *fDeletedToken;
};

#endif
