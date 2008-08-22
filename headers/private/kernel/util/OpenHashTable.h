/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_UTIL_OPEN_HASH_TABLE_H
#define _KERNEL_UTIL_OPEN_HASH_TABLE_H


#include <KernelExport.h>
#include <util/kernel_cpp.h>


/*!
	The Definition template must have four methods: `HashKey', `Hash',
	`Compare' and `GetLink;. It must also define several types as shown in the
	following example:

	struct Foo : HashTableLink<Foo> {
		int bar;

		HashTableLink<Foo> otherLink;
	};

	struct HashTableDefinition {
		typedef int		KeyType;
		typedef	Foo		ValueType;

		HashTableDefinition(const HashTableDefinition&) {}

		size_t HashKey(int key) const { return key >> 1; }
		size_t Hash(Foo *value) const { return HashKey(value->bar); }
		bool Compare(int key, Foo *value) const { return value->bar == key; }
		HashTableLink<Foo> *GetLink(Foo *value) const { return value; }
	};
*/

template<typename Type>
struct HashTableLink {
	Type *fNext;
};

template<typename Definition, bool AutoExpand = true,
	bool CheckDuplicates = false>
class OpenHashTable {
public:
	typedef OpenHashTable<Definition, AutoExpand, CheckDuplicates> HashTable;
	typedef typename Definition::KeyType	KeyType;
	typedef typename Definition::ValueType	ValueType;

	static const size_t kMinimumSize = 8;

	// we use malloc() / free() for allocation. If in the future this
	// is revealed to be insufficient we can switch to a template based
	// allocator. All allocations are of power of 2 lengths.

	// regrowth factor: 200 / 256 = 78.125%
	//                   50 / 256 = 19.53125%

	OpenHashTable()
		:
		fTableSize(0),
		fItemCount(0),
		fTable(NULL)
	{
	}

	OpenHashTable(const Definition& definition)
		:
		fDefinition(definition),
		fTableSize(0),
		fItemCount(0),
		fTable(NULL)
	{
	}

	~OpenHashTable()
	{
		free(fTable);
	}

	status_t Init(size_t initialSize = kMinimumSize)
	{
		if (initialSize > 0 && !_Resize(initialSize))
			return B_NO_MEMORY;
		return B_OK;
	}

	ValueType *Lookup(const KeyType &key) const
	{
		if (fTableSize == 0)
			return NULL;

		size_t index = fDefinition.HashKey(key) & (fTableSize - 1);
		ValueType *slot = fTable[index];

		while (slot) {
			if (fDefinition.Compare(key, slot))
				break;
			slot = _Link(slot)->fNext;
		}

		return slot;
	}

	status_t Insert(ValueType *value)
	{
		if (fTableSize == 0) {
			if (!_Resize(kMinimumSize))
				return B_NO_MEMORY;
		} else if (AutoExpand && fItemCount >= (fTableSize * 200 / 256))
			_Resize(fTableSize * 2);

		InsertUnchecked(value);
		return B_OK;
	}

	void InsertUnchecked(ValueType *value)
	{
		if (CheckDuplicates && _ExhaustiveSearch(value))
			panic("Hash Table: value already in table.");

		_Insert(fTable, fTableSize, value);
		fItemCount++;
	}

	// TODO: a ValueType* Remove(const KeyType& key) method is missing

	bool Remove(ValueType *value)
	{
		if (!RemoveUnchecked(value))
			return false;

		if (AutoExpand && fTableSize > kMinimumSize
			&& fItemCount < (fTableSize * 50 / 256))
			_Resize(fTableSize / 2);

		return true;
	}

	bool RemoveUnchecked(ValueType *value)
	{
		size_t index = fDefinition.Hash(value) & (fTableSize - 1);
		ValueType *previous = NULL, *slot = fTable[index];

		while (slot) {
			ValueType *next = _Link(slot)->fNext;

			if (value == slot) {
				if (previous)
					_Link(previous)->fNext = next;
				else
					fTable[index] = next;
				break;
			}

			previous = slot;
			slot = next;
		}

		if (slot == NULL)
			return false;

		if (CheckDuplicates && _ExhaustiveSearch(value))
			panic("Hash Table: duplicate detected.");

		fItemCount--;
		return true;
	}

	/*!	If the table needs resizing, the number of bytes for the required
		allocation is returned. If no resizing is needed, 0 is returned.
	*/
	size_t ResizeNeeded() const
	{
		size_t size = fTableSize;
		if (size == 0 || fItemCount >= (size * 200 / 256)) {
			// grow table
			if (size == 0)
				size = kMinimumSize;
			while (fItemCount >= size * 200 / 256)
				size <<= 1;
		} else if (size > kMinimumSize && fItemCount < size * 50 / 256) {
			// shrink table
			while (fItemCount < size * 50 / 256)
				size >>= 1;
			if (size < kMinimumSize)
				size = kMinimumSize;
		}

		if (size == fTableSize)
			return 0;

		return size * sizeof(ValueType*);
	}

	/*!	Resizes the table using the given allocation. The allocation must not
		be \c NULL. It must be of size \a size, which must a value returned
		earlier by ResizeNeeded(). If the size requirements have changed in the
		meantime, the method free()s the given allocation and returns \c false.
		Otherwise \c true is returned.
	*/
	bool Resize(void* allocation, size_t size)
	{
		if (size != ResizeNeeded()) {
			free(allocation);
			return false;
		}

		_Resize((ValueType**)allocation, size / sizeof(ValueType*));
		return true;
	}

	class Iterator {
	public:
		Iterator(const HashTable *table)
			: fTable(table)
		{
			Rewind();
		}

		Iterator(const HashTable *table, size_t index, ValueType *value)
			: fTable(table), fIndex(index), fNext(value) {}

		bool HasNext() const { return fNext != NULL; }

		ValueType *Next()
		{
			ValueType *current = fNext;
			_GetNext();
			return current;
		}

		void Rewind()
		{
			// get the first one
			fIndex = 0;
			fNext = NULL;
			_GetNext();
		}

	protected:
		Iterator() {}

		void _GetNext()
		{
			if (fNext)
				fNext = fTable->_Link(fNext)->fNext;

			while (fNext == NULL && fIndex < fTable->fTableSize)
				fNext = fTable->fTable[fIndex++];
		}

		const HashTable *fTable;
		size_t fIndex;
		ValueType *fNext;
	};

	Iterator GetIterator() const { return Iterator(this); }

protected:
	// for g++ 2.95
	friend class Iterator;

	void _Insert(ValueType **table, size_t tableSize, ValueType *value)
	{
		size_t index = fDefinition.Hash(value) & (tableSize - 1);

		_Link(value)->fNext = table[index];
		table[index] = value;
	}

	bool _Resize(size_t newSize)
	{
		ValueType** newTable
			= (ValueType**)malloc(sizeof(ValueType*) * newSize);
		if (newTable == NULL)
			return false;

		_Resize(newTable, newSize);
		return true;
	}

	void _Resize(ValueType** newTable, size_t newSize)
	{
		for (size_t i = 0; i < newSize; i++)
			newTable[i] = NULL;

		if (fTable) {
			for (size_t i = 0; i < fTableSize; i++) {
				ValueType *bucket = fTable[i];
				while (bucket) {
					ValueType *next = _Link(bucket)->fNext;
					_Insert(newTable, newSize, bucket);
					bucket = next;
				}
			}

			free(fTable);
		}

		fTableSize = newSize;
		fTable = newTable;
	}

	HashTableLink<ValueType> *_Link(ValueType *bucket) const
	{
		return fDefinition.GetLink(bucket);
	}

	bool _ExhaustiveSearch(ValueType *value) const
	{
		for (size_t i = 0; i < fTableSize; i++) {
			ValueType *bucket = fTable[i];
			while (bucket) {
				if (bucket == value)
					return true;
				bucket = _Link(bucket)->fNext;
			}
		}

		return false;
	}

	Definition fDefinition;
	size_t fTableSize, fItemCount;
	ValueType **fTable;
};

#endif	// _KERNEL_UTIL_OPEN_HASH_TABLE_H
