/* 
** Copyright 2003-2004, Stefano Ceccherini (burton666@libero.it). All rights reserved.
**           2004, Michael Pfeiffer (laplace@users.sourceforge.net).
** Distributed under the terms of the Haiku License.
**
** History
** 2003-2004  Initial implementation by Stefano Ceccerini.
** 2004/08/03 Testing, bug fixing and implementation of quick sort, refactoring
**            by Michael Pfeiffer.
*/

// TODO: 
//   - Rewrite to use STL
//   - Include ObjectList.h
//   - Test if building with jam works

// Note: Method Owning() is inlined in header file ObjectList.h

#include <ObjectList.h>

#include <algorithm>
#include <assert.h>
#include <functional>
#include <string.h>

#include <List.h>


using namespace std;


// TODO: The implementation of _PointerList_ should be completely rewritten to
// use STL in a more efficent way!

struct comparator;


class AbstractPointerListHelper {
public:
	AbstractPointerListHelper() {};
	virtual ~AbstractPointerListHelper();
	
	/** 
		Returns the index of the item that matches key or
	    a negative number. Then -(index+1) is the insert position 
	    of the item not in list.
	*/
	int32 BinarySearchIndex(const void *key, const BList *list);
	/** 
		Returns the item that matches key or NULL if the item could 
		not be found in the list.
	*/
	void* BinarySearch(const void *key, const BList *list);
	/**
		Sorts the items in list.
	*/
	void SortItems(BList *list);
	/**
		Removes the first item in list and appends it at the bottom of 
		the list and sorts all items but the last item.
	*/
	void HSortItems(BList *list);

	friend struct comparator;

private:
	enum {
		// Use insertion sort if number of elements in list is less than 
		// kQuickSortThreshold.
		kQuickSortThreshold = 11,
		// Use simple pivot element computation if number of elements in
		// list is less than kPivotThreshold.
		kPivotThreshold     = 5,
	}; 

	// Methods that do the actual work:
	inline void Swap(void **items, int32 i, int32 j);

	void* BinarySearch(const void *key, const void **items, int32 numItems,
			int32 &index);
	void QuickSort(void **items, int32 low, int32 high);
	
	// Method to be implemented by sub classes
	int virtual Compare(const void *key, const void* item) = 0;
};

struct comparator : public binary_function<const void*, const void*, bool>
{
	comparator(AbstractPointerListHelper* helper) : helper(helper) {}
	
	bool operator()(const void* a, const void* b) {
		return helper->Compare(b, a) > 0;
	}
	
	AbstractPointerListHelper* helper;
};


AbstractPointerListHelper::~AbstractPointerListHelper()
{
}


void
AbstractPointerListHelper::Swap(void **items, int32 i, int32 j)
{
	void *swap = items[i];
	items[i] = items[j];
	items[j] = swap;
}


int32
AbstractPointerListHelper::BinarySearchIndex(const void *key, const BList *list)
{
	int32 index;
	const void **items = static_cast<const void**>(list->Items());
	BinarySearch(key, items, list->CountItems(), index);
	return index;
}


void *
AbstractPointerListHelper::BinarySearch(const void *key, const BList *list)
{
	int32 index;
	const void **items = static_cast<const void**>(list->Items());
	return BinarySearch(key, items, list->CountItems(), index);
}


void
AbstractPointerListHelper::SortItems(BList *list)
{
	void **items = static_cast<void**>(list->Items());
	QuickSort(items, 0, list->CountItems()-1);
}


void
AbstractPointerListHelper::HSortItems(BList *list)
{
	void **items = static_cast<void**>(list->Items());
	int32 numItems = list->CountItems();
	if (numItems > 1) {
		// swap last with first item
		Swap(items, 0, numItems-1);
	}
	// sort all items but last item
	QuickSort(items, 0, numItems-2);
}


void *
AbstractPointerListHelper::BinarySearch(const void *key, const void **items,
	int32 numItems, int32 &index)
{
	const void** end = &items[numItems];
	const void** found = lower_bound(items, end, key, comparator(this));
	index = found - items;
	if (index != numItems && Compare(key, *found) == 0) {
		return const_cast<void*>(*found);
	} else {
		index = -(index + 1);
		return NULL;
	}
}


void
AbstractPointerListHelper::QuickSort(void **items, int32 low, int32 high)
{
	if (low <= high) {
		sort(&items[low], &items[high+1], comparator(this));
	}
}


class PointerListHelper : public AbstractPointerListHelper {
public:
	PointerListHelper(_PointerList_::GenericCompareFunction compareFunc)
		: fCompareFunc(compareFunc)
	{
		// nothing to do
	}
	
	int Compare(const void *a, const void *b)
	{
		return fCompareFunc(a, b);
	}
	
private:
	_PointerList_::GenericCompareFunction fCompareFunc;
};


class PointerListHelperWithState : public AbstractPointerListHelper
{
public:
	PointerListHelperWithState(
		_PointerList_::GenericCompareFunctionWithState compareFunc, 
		void* state)
		: fCompareFunc(compareFunc)
		, fState(state)
	{
		// nothing to do
	}

	int Compare(const void *a, const void *b)
	{
		return fCompareFunc(a, b, fState);
	}

private:
	_PointerList_::GenericCompareFunctionWithState fCompareFunc;
	void* fState;
};


class PointerListHelperUsePredicate : public AbstractPointerListHelper
{
public:
	PointerListHelperUsePredicate(
		_PointerList_::UnaryPredicateGlue predicate)
		: fPredicate(predicate)
	{
		// nothing to do
	}

	int Compare(const void *arg, const void *item)
	{
		// need to adapt arguments and return value
		return -fPredicate(item, const_cast<void *>(arg));
	}

private:
	_PointerList_::UnaryPredicateGlue fPredicate;
};


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


// Note: function pointers must not be NULL!!!

void *
_PointerList_::EachElement(GenericEachFunction function, void *arg)
{
	int32 numItems = CountItems();
	void *result = NULL;
	
	for (int32 index = 0; index < numItems; index++) {
		result = function(ItemAtFast(index), arg);
		if (result != NULL)
			break;
	}

	return result;
}


void
_PointerList_::SortItems(GenericCompareFunction compareFunc)
{
	PointerListHelper helper(compareFunc);
	helper.SortItems(this);
}


void
_PointerList_::SortItems(GenericCompareFunctionWithState compareFunc,
	void *state)
{
	PointerListHelperWithState helper(compareFunc, state);
	helper.SortItems(this);	
}


void
_PointerList_::HSortItems(GenericCompareFunction compareFunc)
{
	PointerListHelper helper(compareFunc);
	helper.HSortItems(this);
}


void
_PointerList_::HSortItems(GenericCompareFunctionWithState compareFunc,
	void *state)
{
	PointerListHelperWithState helper(compareFunc, state);
	helper.HSortItems(this);	
}


void *
_PointerList_::BinarySearch(const void *key,
	GenericCompareFunction compareFunc) const
{
	PointerListHelper helper(compareFunc);
	return helper.BinarySearch(key, this);
}


void *
_PointerList_::BinarySearch(const void *key,
			GenericCompareFunctionWithState compareFunc, void *state) const
{
	PointerListHelperWithState helper(compareFunc, state);
	return helper.BinarySearch(key, this);	
}


int32
_PointerList_::BinarySearchIndex(const void *key,
	GenericCompareFunction compareFunc) const
{
	PointerListHelper helper(compareFunc);
	return helper.BinarySearchIndex(key, this);
}


int32
_PointerList_::BinarySearchIndex(const void *key,
			GenericCompareFunctionWithState compareFunc, void *state) const
{
	PointerListHelperWithState helper(compareFunc, state);
	return helper.BinarySearchIndex(key, this);	
}


int32
_PointerList_::BinarySearchIndexByPredicate(const void *key,
	UnaryPredicateGlue predicate) const
{
	PointerListHelperUsePredicate helper(predicate);
	return helper.BinarySearchIndex(key, this);		
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


bool
_PointerList_::MoveItem(int32 from, int32 to)
{
	if (from == to)
		return true;

	void* fromItem = ItemAt(from);
	void* toItem = ItemAt(to);
	if (fromItem == NULL || toItem == NULL)
		return false;

	void** items = static_cast<void**>(Items());
	if (from < to)
		memmove(items + from, items + from + 1, (to - from) * sizeof(void*));
	else
		memmove(items + to + 1, items + to, (from - to) * sizeof(void*));

	items[to] = fromItem;
	return true;
}


