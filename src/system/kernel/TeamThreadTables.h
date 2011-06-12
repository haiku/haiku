/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_TEAM_THREAD_TABLES_H
#define KERNEL_TEAM_THREAD_TABLES_H


#include <thread_types.h>


namespace BKernel {


template<typename Element>
struct TeamThreadTable {
public:
	typedef typename Element::id_type		id_type;
	typedef typename Element::iterator_type	IteratorEntry;

	struct Iterator {
		Iterator()
			:
			fNext(NULL)
		{
		}

		Iterator(IteratorEntry* nextEntry)
		{
			_SetNext(nextEntry);
		}

		bool HasNext() const
		{
			return fNext != NULL;
		}

		Element* Next()
		{
			Element* result = fNext;
			if (result != NULL)
				_SetNext(result->GetDoublyLinkedListLink()->next);

			return result;
		}

	private:
		void _SetNext(IteratorEntry* entry)
		{
			while (entry != NULL) {
				if (entry->id >= 0) {
					fNext = static_cast<Element*>(entry);
					return;
				}

				entry = entry->GetDoublyLinkedListLink()->next;
			}

			fNext = NULL;
		}

	private:
		Element*	fNext;
	};

public:
	TeamThreadTable()
		:
		fNextSerialNumber(1)
	{
	}

	status_t Init(size_t initialSize)
	{
		return fTable.Init(initialSize);
	}

	void Insert(Element* element)
	{
		element->serial_number = fNextSerialNumber++;
		fTable.InsertUnchecked(element);
		fList.Add(element);
	}

	void Remove(Element* element)
	{
		fTable.RemoveUnchecked(element);
		fList.Remove(element);
	}

	Element* Lookup(id_type id, bool visibleOnly = true) const
	{
		Element* element = fTable.Lookup(id);
		return element != NULL && (!visibleOnly || element->visible)
			? element : NULL;
	}

	/*! Gets an iterator.
		The iterator iterates through all, including invisible, entries!
	*/
	Iterator GetIterator()
	{
		return Iterator(fList.Head());
	}

	void InsertIteratorEntry(IteratorEntry* entry)
	{
		// add to front
		entry->id = -1;
		entry->visible = false;
		fList.Add(entry, false);
	}

	void RemoveIteratorEntry(IteratorEntry* entry)
	{
		fList.Remove(entry);
	}

	Element* NextElement(IteratorEntry* entry, bool visibleOnly = true)
	{
		if (entry == fList.Tail())
			return NULL;

		IteratorEntry* nextEntry = entry;

		while (true) {
			nextEntry = nextEntry->GetDoublyLinkedListLink()->next;
			if (nextEntry == NULL) {
				// end of list -- requeue entry at the end and return NULL
				fList.Remove(entry);
				fList.Add(entry);
				return NULL;
			}

			if (nextEntry->id >= 0 && (!visibleOnly || nextEntry->visible)) {
				// found an element -- requeue entry after element
				Element* element = static_cast<Element*>(nextEntry);
				fList.Remove(entry);
				fList.InsertAfter(nextEntry, entry);
				return element;
			}
		}
	}

private:
	struct HashDefinition {
		typedef id_type		KeyType;
		typedef	Element		ValueType;

		size_t HashKey(id_type key) const
		{
			return key;
		}

		size_t Hash(Element* value) const
		{
			return HashKey(value->id);
		}

		bool Compare(id_type key, Element* value) const
		{
			return value->id == key;
		}

		Element*& GetLink(Element* value) const
		{
			return value->hash_next;
		}
	};

	typedef BOpenHashTable<HashDefinition> ElementTable;
	typedef DoublyLinkedList<IteratorEntry> List;

private:
	ElementTable	fTable;
	List			fList;
	int64			fNextSerialNumber;
};


}	// namespace BKernel


#endif	// KERNEL_TEAM_THREAD_TABLES_H
