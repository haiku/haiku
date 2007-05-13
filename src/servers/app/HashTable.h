/*
 * Copyright 2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef _HASH_TABLE_H_
#define _HASH_TABLE_H_


#include <SupportDefs.h>


class Hashable {
	public:
		virtual ~Hashable() {};
		virtual uint32 Hash() const = 0;
		virtual bool CompareTo(Hashable& hashable) const = 0;
};

class HashTable {
	public:
		HashTable(bool owning = false, int32 capacity = 100, float loadFactor = 0.75);
		~HashTable();

		void MakeEmpty(bool deleteValues = true);
		bool IsEmpty() const { return fCount == 0; }
		bool ContainsKey(Hashable& key) const { return _GetHashEntry(key) != NULL; }
		int32 CountItems() const { return fCount; }

		Hashable *GetValue(Hashable& key) const;

		bool AddItem(Hashable* value);
		Hashable *RemoveItem(Hashable& key);

	protected:
		struct entry;

		bool _Rehash();
		entry *_GetHashEntry(Hashable& key) const;

		entry**	fTable;
		int32	fCapacity, fCount, fThreshold;
		float	fLoadFactor;
		bool	fOwning;
};

#endif  /* HASHTABLE_H */
