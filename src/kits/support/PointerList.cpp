/* 
** Copyright 2003-2004, Stefano Ceccherini (burton666@libero.it). All rights reserved.
**           2004, Michael Pfeiffer (laplace@users.sourceforge.net).
** Distributed under the terms of the OpenBeOS License.
**
** History
** 2003-2004  Initial implementation by Stefano Ceccerini.
** 2004/08/03 Testing, bug fixing and implementation of quick sort by Michael Pfeiffer.
*/

// TODO: Implement quick sort

// Note: Method Owning() is inlined in header file ObjectList.h

#include <List.h>


// Declaration of class _PointerList_ is inlined to be independet of the
// header file ObjectList.h

#ifndef __OBJECT_LIST__
class _PointerList_ : public BList {
public:
	_PointerList_(const _PointerList_ &list);
	_PointerList_(int32 itemsPerBlock = 20, bool owning = false);
	~_PointerList_();
	
	typedef void *(* GenericEachFunction)(void *, void *);
	typedef int (* GenericCompareFunction)(const void *, const void *);
	typedef int (* GenericCompareFunctionWithState)(const void *, const void *,
		void *);
	typedef int (* UnaryPredicateGlue)(const void *, void *);

	void *EachElement(GenericEachFunction, void *);
	void SortItems(GenericCompareFunction);
	void SortItems(GenericCompareFunctionWithState, void *state);
	void HSortItems(GenericCompareFunction);
	void HSortItems(GenericCompareFunctionWithState, void *state);
	
	void *BinarySearch(const void *, GenericCompareFunction) const;
	void *BinarySearch(const void *, GenericCompareFunctionWithState, void *state) const;

	int32 BinarySearchIndex(const void *, GenericCompareFunction) const;
	int32 BinarySearchIndex(const void *, GenericCompareFunctionWithState, void *state) const;
	int32 BinarySearchIndexByPredicate(const void *, UnaryPredicateGlue) const;

	bool Owning() const;
	bool ReplaceItem(int32, void *);

protected:
	bool owning;

};
#endif


// Forward declarations

static void *BinarySearch(const void *key, const void **items,
					int32 numItems,
					_PointerList_::GenericCompareFunction compareFunc,
					int32& index);

static void *BinarySearch(const void *key, const void **items,
					int32 numItems,
					_PointerList_::GenericCompareFunctionWithState compareFunc, void *state,
					int32& index);


// Implementation of class _PointerList_

_PointerList_::_PointerList_(int32 itemsPerBlock, bool own)
	:
	BList(itemsPerBlock),
	owning(own)
{
	
}


_PointerList_::_PointerList_(const _PointerList_ &list)
	:
	BList(list),
	owning(list.owning)
{
}


_PointerList_::~_PointerList_()
{
	// This is empty by design, the "owning" variable
	// is used by the BObjectList subclass
}


void *
_PointerList_::EachElement(GenericEachFunction function, void *arg)
{
	int32 numItems = CountItems();
	void *result = NULL;
	
	if (function) {
		for (int32 index = 0; index < numItems; index++) {
			result = function(ItemAtFast(index), arg);
			if (result != NULL)
				break;
		}
	}
	return result;
}


void
_PointerList_::SortItems(GenericCompareFunction compareFunc)
{

	if (compareFunc) {
		int32 numItems = CountItems();
		if (numItems > 1) {
			void **items = (void **)Items();
			for (int32 i = 0; i < numItems - 1; i++) {
				for (int32 j = 0; j < numItems - 1 - i; j++) {
					if (compareFunc(items[j + 1], items[j]) < 0) {
						void *tmp = items[j];
						items[j] = items[j + 1];
						items[j + 1] = tmp;
					}	
				}
			}
		}
	}
}


void
_PointerList_::SortItems(GenericCompareFunctionWithState compareFunc, void *state)
{
	if (compareFunc) {
		int32 numItems = CountItems();
		if (numItems > 1) {
			void **items = (void **)Items();
			for (int32 i = 0; i < numItems - 1; i++) {
				for (int32 j = 0; j < numItems - 1 - i; j++) {
					if (compareFunc(items[j + 1], items[j], state) < 0) {
						void *tmp = items[j];
						items[j] = items[j + 1];
						items[j + 1] = tmp;
					}	
				}
			}
		}
	}
}


void
_PointerList_::HSortItems(GenericCompareFunction sortFunction)
{
	const bool hasItems = !IsEmpty();
	void* firstItem = NULL;
	if (hasItems) {
		// append first item after sorting the list
		firstItem = ItemAt(0);
		RemoveItem((int32)0);
	}
	SortItems(sortFunction);
	if (hasItems) {
		AddItem(firstItem);
	}
}


void
_PointerList_::HSortItems(GenericCompareFunctionWithState sortFunc, void *state)
{
	const bool hasItems = !IsEmpty();
	void* firstItem = NULL;
	if (hasItems) {
		// append first item after sorting the list
		firstItem = ItemAt(0);
		RemoveItem((int32)0);
	}
	SortItems(sortFunc, state);
	if (hasItems) {
		AddItem(firstItem);
	}
}


void *
_PointerList_::BinarySearch(const void *key, GenericCompareFunction compareFunc) const
{
	int32 index;
	return ::BinarySearch(key, (const void **)Items(), CountItems(), compareFunc, index);
}


void *
_PointerList_::BinarySearch(const void *key,
			GenericCompareFunctionWithState compareFunc, void *state) const
{
	int32 index;
	return ::BinarySearch(key, (const void**)Items(), CountItems(), compareFunc, state, index);
}


int32
_PointerList_::BinarySearchIndex(const void *key, GenericCompareFunction compareFunc) const
{
	int32 index;
	::BinarySearch(key, (const void**)Items(), CountItems(), compareFunc, index);
	return index;
}


int32
_PointerList_::BinarySearchIndex(const void *key,
			GenericCompareFunctionWithState compareFunc, void *state) const
{
	int32 index;
	::BinarySearch(key, (const void**)Items(), CountItems(), compareFunc, state, index);
	return index;
}


int32
_PointerList_::BinarySearchIndexByPredicate(const void *key, UnaryPredicateGlue predicate) const
{
	void** items = (void**)Items();
	int32 low = 0;
	int32 high = CountItems()-1;
	int result = 0;
	int index = 0;
	while (low <= high) {
		index = (low + high) / 2;
		const void* item = items[index];
		result = predicate(item, (void*)key);
		if (result > 0) {
			high = index - 1;
		} else if (result < 0) {
			low = index + 1;
		} else {
			return index;
		}
	}

	if (result < 0) {
		index ++;
	}

	return -(index+1);
}

bool
_PointerList_::ReplaceItem(int32 index, void *newItem)
{
	if (index < 0 || index >= CountItems())
		return false;

	void **items = static_cast<void **>(Items());
	items[index] = newItem;

	return true;
}

// Implementation of static functions

static void * BinarySearch(const void* key, const void** items, int32 numItems,
	_PointerList_::GenericCompareFunction compareFunc, int32& index)
{
	int32 low = 0;
	int32 high = numItems-1;
	int result = 0;
	index = 0;
	while (low <= high) {
		index = (low + high) / 2;
		const void* item = items[index];
		result = compareFunc(key, item);
		if (result < 0) {
			high = index - 1;
		} else if (result > 0) {
			low = index + 1;
		} else {
			return (void*)item;
		}
	}
	
	if (result > 0) {
		index ++;
	}
	
	index = -(index+1);
	return NULL;
}
							
static void * BinarySearch(const void* key, const void** items, int32 numItems,
	_PointerList_::GenericCompareFunctionWithState compareFunc, void* state, int32& index)
{
	int32 low = 0;
	int32 high = numItems-1;
	int result = 0;
	index = 0;
	while (low <= high) {
		index = (low + high) / 2;
		const void* item = items[index];
		result = compareFunc(key, item, state);
		if (result < 0) {
			high = index - 1;
		} else if (result > 0) {
			low = index + 1;
		} else {
			return (void*)item;
		}
	}

	if (result > 0) {
		index ++;
	}

	index = -(index+1);
	return NULL;
}
