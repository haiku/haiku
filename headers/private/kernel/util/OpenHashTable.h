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
#include <util/kernel_cpp.h>

// the Definition template must have four methods: `HashKey', `Hash',
// `Compare' and `GetLink;. It must also define several types as shown in the
// following example:
//
// struct Foo : HashTableLink<Foo> {
//	int bar;
//
//	HashTableLink<Foo> otherLink;
// };
//
// struct HashTableDefinition {
//	typedef void	ParentType;
//	typedef int		KeyType;
//	typedef	Foo		ValueType;
//
//	HashTableDefinition(void *parent) {}
//
//	size_t HashKey(int key) { return key >> 1; }
//	size_t Hash(Foo *value) { return HashKey(value->bar); }
//	bool Compare(int key, Foo *value) { return value->bar == key; }
//  HashTableLink<Foo> *GetLink(Foo *value) { return value; }
// };

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

	static const size_t kMinimumSize = 32;

	// we use new [] / delete [] for allocation. If in the future this
	// is revealed to be insufficient we can switch to a template based
	// allocator. All allocations are of power of 2 lengths.

	// regrowth factor: 200 / 256 = 78.125%
	//                   50 / 256 = 19.53125%

	OpenHashTable(size_t initialSize = kMinimumSize)
		: fItemCount(0), fTable(NULL)
	{
		if (initialSize < kMinimumSize)
			initialSize = kMinimumSize;

		_Resize(initialSize);
	}

	OpenHashTable(typename Definition::ParentType *parent,
		size_t initialSize = kMinimumSize)
		: fDefinition(parent), fItemCount(0), fTable(NULL)
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
		size_t index = fDefinition.HashKey(key) & (fTableSize - 1);
		ValueType *slot = fTable[index];

		while (slot) {
			if (fDefinition.Compare(key, slot))
				break;
			slot = _Link(slot)->fNext;
		}

		return slot;
	}

	void Insert(ValueType *value)
	{
		if (AutoExpand && fItemCount >= (fTableSize * 200 / 256))
			_Resize(fTableSize * 2);

		InsertUnchecked(value);
	}

	void InsertUnchecked(ValueType *value)
	{
		if (CheckDuplicates) {
			for (size_t i = 0; i < fTableSize; i++) {
				ValueType *bucket = fTable[i];
				while (bucket) {
					if (bucket == value)
						panic("Hash Table: value already in table.");
					bucket = _Link(bucket)->fNext;
				}
			}
		}

		_Insert(fTable, fTableSize, value);
		fItemCount++;
	}

	void Remove(ValueType *value)
	{
		RemoveUnchecked(value);

		if (AutoExpand && fTableSize > kMinimumSize
			&& fItemCount < (fTableSize * 50 / 256))
			_Resize(fTableSize / 2);
	}

	void RemoveUnchecked(ValueType *value)
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

		if (CheckDuplicates) {
			for (size_t i = 0; i < fTableSize; i++) {
				ValueType *bucket = fTable[i];
				while (bucket) {
					if (bucket == value)
						panic("Hash Table: duplicate detected.");
					bucket = _Link(bucket)->fNext;
				}
			}
		}

		fItemCount--;
	}

	class Iterator {
	public:
		Iterator(const HashTable *table)
			: fTable(table), fIndex(0), fCurrent(NULL), fNext(NULL)
		{
			Rewind();
		}

		bool HasNext() const { return fNext != NULL; }

		ValueType *Next()
		{
			ValueType *current = fCurrent;
			fCurrent = fNext;
			_GetNext();
			return current;
		}

		void Rewind()
		{
			// get the first one
			fIndex = 0;
			fCurrent = NULL;
			_GetNext();
			fCurrent = fNext;
			// get the proper next one
			if (fCurrent)
				_GetNext();
		}

	private:
		void _GetNext()
		{
			if (fCurrent) {
				fNext = fTable->_Link(fCurrent)->fNext;
			} else {
				fNext = NULL;
			}

			while (fNext == NULL && fIndex < fTable->fTableSize)
				fNext = fTable->fTable[fIndex++];
		}

		const HashTable *fTable;
		size_t fIndex;
		ValueType *fCurrent, *fNext;
	};

	Iterator GetIterator() const { return Iterator(this); }

private:
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
		ValueType **newTable = new ValueType *[newSize];
		if (newTable == NULL)
			return false;

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

			delete [] fTable;
		}

		fTableSize = newSize;
		fTable = newTable;
		return true;
	}

	HashTableLink<ValueType> *_Link(ValueType *bucket) const
	{
		return fDefinition.GetLink(bucket);
	}

	Definition fDefinition;
	size_t fTableSize, fItemCount;
	ValueType **fTable;
};

#endif
