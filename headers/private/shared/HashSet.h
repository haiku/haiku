/*
 * Copyright 2004-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef HASH_SET_H
#define HASH_SET_H

#include <util/OpenHashTable.h>

#include "AutoLocker.h"
#include "Locker.h"


// HashSetElement
template<typename Key>
class HashSetElement : public HashTableLink<HashSetElement<Key> > {
private:
	typedef HashSetElement<Key> Element;

public:
	HashSetElement()
		:
		fKey()
	{
	}

	HashSetElement(const Key& key)
		:
		fKey(key)
	{
	}

	Key		fKey;
};


// HashSetTableDefinition
template<typename Key>
struct HashSetTableDefinition {
	typedef Key					KeyType;
	typedef	HashSetElement<Key>	ValueType;

	size_t HashKey(const KeyType& key) const
		{ return key.GetHashCode(); }
	size_t Hash(const ValueType* value) const
		{ return HashKey(value->fKey); }
	bool Compare(const KeyType& key, const ValueType* value) const
		{ return value->fKey == key; }
	HashTableLink<ValueType>* GetLink(ValueType* value) const
		{ return value; }
};


// HashSet
template<typename Key>
class HashSet {
public:
	class Iterator {
	private:
		typedef HashSetElement<Key>	Element;
	public:
		Iterator(const Iterator& other)
			:
			fSet(other.fSet),
			fIterator(other.fIterator),
			fElement(other.fElement)
		{
		}

		bool HasNext() const
		{
			return fIterator.HasNext();
		}

		Key Next()
		{
			fElement = fIterator.Next();
			if (fElement == NULL)
				return Key();

			return fElement->fKey;
		}

		bool Remove()
		{
			if (fElement == NULL)
				return false;

			fSet->fTable.RemoveUnchecked(fElement);
			delete fElement;
			fElement = NULL;

			return true;
		}

		Iterator& operator=(const Iterator& other)
		{
			fSet = other.fSet;
			fIterator = other.fIterator;
			fElement = other.fElement;
			return *this;
		}

	private:
		Iterator(HashSet<Key>* set)
			:
			fSet(set),
			fIterator(set->fTable.GetIterator()),
			fElement(NULL)
		{
		}

	private:
		friend class HashMap<Key, Value>;
		typedef OpenHashTable<HashSetTableDefinition<Key> > ElementTable;

		HashSet<Key>*			fSet;
		ElementTable::Iterator	fIterator;
		Element*				fElement;

	private:
		friend class HashSet<Key>;
	};

	HashSet();
	~HashSet();

	status_t InitCheck() const;

	status_t Add(const Key& key);
	bool Remove(const Key& key);
	void Clear();
	bool Contains(const Key& key) const;

	int32 Size() const;

	Iterator GetIterator();

protected:
	typedef OpenHashTable<HashSetTableDefinition<Key> > ElementTable;
	typedef HashSetElement<Key>	Element;
	friend class Iterator;

protected:
	ElementTable	fTable;
};


// SynchronizedHashSet
template<typename Key>
class SynchronizedHashSet : public Locker {
public:
	typedef HashSet<Key>::Iterator Iterator;

	SynchronizedHashSet() : Locker("synchronized hash set")	{}
	~SynchronizedHashSet()	{ Lock(); }

	status_t InitCheck() const
	{
		return fSet.InitCheck();
	}

	status_t Add(const Key& key)
	{
		MapLocker locker(this);
		if (!locker.IsLocked())
			return B_ERROR;
		return fSet.Add(key);
	}

	bool Remove(const Key& key)
	{
		MapLocker locker(this);
		if (!locker.IsLocked())
			return false;
		return fSet.Remove(key);
	}

	void Clear()
	{
		MapLocker locker(this);
		fSet.Clear();
	}

	bool Contains(const Key& key) const
	{
		const Locker* lock = this;
		MapLocker locker(const_cast<Locker*>(lock));
		if (!locker.IsLocked())
			return false;
		return fSet.Contains(key);
	}

	int32 Size() const
	{
		const Locker* lock = this;
		MapLocker locker(const_cast<Locker*>(lock));
		return fSet.Size();
	}

	Iterator GetIterator()
	{
		return fSet.GetIterator();
	}

	// for debugging only
	const HashSet<Key>& GetUnsynchronizedSet() const	{ return fSet; }
	HashSet<Key>& GetUnsynchronizedSet()				{ return fSet; }

protected:
	typedef AutoLocker<Locker> MapLocker;

	HashSet<Key>	fSet;
};


// HashSet

// constructor
template<typename Key>
HashSet<Key>::HashSet()
	:
	fTable()
{
	fTable.Init();
}


// destructor
template<typename Key>
HashSet<Key>::~HashSet()
{
	Clear();
}


// InitCheck
template<typename Key>
status_t
HashSet<Key>::InitCheck() const
{
	return (fTable.TableSize() > 0 ? B_OK : B_NO_MEMORY);
}


// Add
template<typename Key>
status_t
HashSet<Key>::Add(const Key& key)
{
	Element* element = fTable.Lookup(key);
	if (element) {
		// already contains the value
		return B_OK;
	}

	// does not contain the key yet: create an element and add it
	element = new(std::nothrow) Element(key);
	if (!element)
		return B_NO_MEMORY;

	status_t error = fTable.Insert(element);
	if (error != B_OK)
		delete element;

	return error;
}


// Remove
template<typename Key>
bool
HashSet<Key>::Remove(const Key& key)
{
	Element* element = fTable.Lookup(key);
	if (element == NULL)
		return false;

	fTable.Remove(element);
	delete element;

	return true;
}


// Clear
template<typename Key, typename Value>
void
HashSet<Key>::Clear()
{
	// clear the table and delete the elements
	Element* element = fTable.Clear(true);
	while (element != NULL) {
		Element* next = element->fNext;
		delete element;
		element = next;
	}
}


// Contains
template<typename Key>
bool
HashSet<Key>::Contains(const Key& key) const
{
	return fTable.Lookup(key) != NULL;
}


// Size
template<typename Key>
int32
HashSet<Key>::Size() const
{
	return fTable.CountElements();
}

// GetIterator
template<typename Key>
HashSet<Key>::Iterator
HashSet<Key>::GetIterator()
{
	return Iterator(this);
}

// _FindElement
template<typename Key>
HashSet<Key>::Element *
HashSet<Key>::_FindElement(const Key& key) const
{
	Element* element = fTable.FindFirst(key.GetHashCode());
	while (element && element->fKey != key) {
		if (element->fNext >= 0)
			element = fTable.ElementAt(element->fNext);
		else
			element = NULL;
	}
	return element;
}


#endif	// HASH_SET_H
