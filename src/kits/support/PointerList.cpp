/* 
** Copyright 2003-2004, Stefano Ceccherini (burton666@libero.it). All rights reserved.
**           2004, Michael Pfeiffer (laplace@users.sourceforge.net).
** Distributed under the terms of the OpenBeOS License.
**
** History
** 2003-2004  Initial implementation by Stefano Ceccerini.
** 2004/08/03 Testing, bug fixing and implementation of quick sort, refactoring
**            by Michael Pfeiffer.
*/

// Note: Method Owning() is inlined in header file ObjectList.h

#include <assert.h>
#include <List.h>


// Declaration of class _PointerList_ is inlined to be independet of the
// header file ObjectList.h

#ifndef __OBJECT_LIST__
class _PointerList_ : public BList {
public:
	_PointerList_(const _PointerList_ &list);
	_PointerList_(int32 itemsPerBlock = 20, bool owning = false);
	~_PointerList_();
	
	typedef void *(* GenericEachFunction)(void *item, void *arg);
	typedef int (* GenericCompareFunction)(const void *key, const void *item);
	typedef int (* GenericCompareFunctionWithState)(const void *key, const void *item,
		void *state);
	typedef int (* UnaryPredicateGlue)(const void *item, void *key);

	void *EachElement(GenericEachFunction, void *arg);
	void SortItems(GenericCompareFunction);
	void SortItems(GenericCompareFunctionWithState, void *state);
	void HSortItems(GenericCompareFunction);
	void HSortItems(GenericCompareFunctionWithState, void *state);
	
	void *BinarySearch(const void *key, GenericCompareFunction) const;
	void *BinarySearch(const void *key, GenericCompareFunctionWithState, void *state) const;

	int32 BinarySearchIndex(const void *key, GenericCompareFunction) const;
	int32 BinarySearchIndex(const void *key, GenericCompareFunctionWithState, void *state) const;
	int32 BinarySearchIndexByPredicate(const void *arg, UnaryPredicateGlue) const;

	bool Owning() const;
	bool ReplaceItem(int32, void *);

protected:
	bool owning;

};
#endif

class AbstractPointerListHelper 
{
public:
	AbstractPointerListHelper() {};
	
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

	void* BinarySearch(const void *key, const void **items, int32 numItems, int32 &index);
	void InsertionSort(void **items, int32 numItems);
	inline void InsertionSort(void **items, int32 low, int32 high);
	int32 ChoosePivot(void **items, int32 low, int32 high);
	int32 Partition(void **items, int32 low, int32 high, bool &isSorted);
	void QuickSort(void **items, int32 low, int32 high);
	
	// Method to be implemented by sub classes
	int virtual Compare(const void *key, const void* item) = 0;
};


void AbstractPointerListHelper::Swap(void **items, int32 i, int32 j)
{
	void *swap = items[i];
	items[i] = items[j];
	items[j] = swap;
}


int32 AbstractPointerListHelper::BinarySearchIndex(const void* key, const BList *list)
{
	int32 index;
	const void **items = static_cast<const void**>(list->Items());
	BinarySearch(key, items, list->CountItems(), index);
	return index;
}


void* AbstractPointerListHelper::BinarySearch(const void* key, const BList *list)
{
	int32 index;
	const void **items = static_cast<const void**>(list->Items());
	return BinarySearch(key, items, list->CountItems(), index);
}


void AbstractPointerListHelper::SortItems(BList *list)
{
	void **items = static_cast<void**>(list->Items());
	QuickSort(items, 0, list->CountItems()-1);
}


void AbstractPointerListHelper::HSortItems(BList *list)
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


void* AbstractPointerListHelper::BinarySearch(const void* key, const void** items, int32 numItems, int32& index)
{
	int32 low = 0;
	int32 high = numItems-1;
	int result = 0;
	index = 0;
	while (low <= high) {
		index = (low + high) / 2;
		const void* item = items[index];
		result = Compare(key, item);
		if (result < 0) {
			// key < item
			high = index - 1;
		} else if (result > 0) {
			// key > item
			low = index + 1;
		} else {
			// key == item
			return const_cast<void *>(item);
		}
	}
	// item not found
	if (result > 0) {
		// key > last item (= items[index])
		// insert position for key is after last item
		index ++;
	}

	index = -(index+1);
	return NULL;
}


int32 AbstractPointerListHelper::ChoosePivot(void **items, int32 low, int32 high)
{
	if (kPivotThreshold <= kQuickSortThreshold || 
		high - low > kPivotThreshold) {
		assert(high - low > kPivotThreshold);
		// choose the middle item of three items
 		int32 mid = (low + high) / 2;

		void* first = items[low];
		void* middle = items[mid];
		void* last = items[high];

		if (Compare(first, middle) <= 0) {
			// first <= middle
			if (Compare(middle, last) <= 0) {
				// first <= middle <= last
				return mid;
			}
			// first <= middle and last < middle
			if (Compare(first, last) <= 0) {
				// first <= last < middle
				return high;
			} 
			// last < first <= middle 
			return low;
		}
		// middle < first
		if (Compare(first, last) <= 0) {
			// middle < first <= last
			return low;
		}
		// middle < first and last < first
		if (Compare(middle, last) <= 0) {
			// middle <= last < first
			return high;
		}
		// last < middle < first
		return mid;		
	} else {
		// choose the middle element to avoid O(n^2) for an already sorted list
		return (low + high) / 2;
	}	
}


int32 AbstractPointerListHelper::Partition(void **items, int32 low, int32 high, bool& isSorted)
{
	assert(low < high);
	int32 left  = low; 
	int32 right = high;
	int32 pivot = ChoosePivot(items, low, high);
	void *pivotItem = items[pivot];

	// Optimization: Check if all items are equal. We get this almost for free.
	// Searching the first item that does not belong to the left list has to
	// be done anyway.
	int32 result;
	isSorted = true;
	// Search first item in left part that does not belong to this part
	// (where item > pivotItem)
	while (left < right && (result = Compare(items[left], pivotItem)) <= 0) {
		left ++;
		if (result != 0) {
			isSorted = false;
			break;
		}	
	}
	if (isSorted && left == right && Compare(items[right], pivotItem) == 0) {
		return low;
	}
	// End of optimization
	isSorted = false;

	// pivot element has to be first element in list
	if (low != pivot) {
		Swap(items, low, pivot);
	}
	
	// now partion the array in a left part where item <= pivotItem
	// and a right part where item > pivotItem
	do {
		// search first item in left part that does not belong to this part
		// (where item > pivotItem)
		while (left < right && Compare(items[left], pivotItem) <= 0) {
			left ++;
		}
		// search first item (from right to left) in right part that does not belong 
		// to this part (where item <= pivotItem). This holds at least for pivot
		// element at top of list! No array bounds check needed!
		while (Compare(items[right], pivotItem) > 0) {
			right --;
		}
		if (left < right) {
			// now swap the items to the proper part
			Swap(items, left, right);
		}
	} while (left < right);
	// place pivotItem between left and right part
	items[low] = items[right];
	items[right] = pivotItem;
	return right;
}


void AbstractPointerListHelper::InsertionSort(void **items, int32 numItems)
{
	for (int32 i = 1; i < numItems; i ++) {
		// treat list[0 .. i-1] as sorted
		void* item = items[i];
		// insert item at right place in list[0..i]
		int32 j = i;
		void* prev = items[j-1];
		while (Compare(prev, item) > 0) {
			items[j] = prev;
			j --;
			if (j <= 0) break;
			prev = items[j-1];
		}
		items[j] = item;
	}
}


void AbstractPointerListHelper::InsertionSort(void **items, int32 low, int32 high)
{
	InsertionSort(&items[low], high - low + 1);
}


void AbstractPointerListHelper::QuickSort(void **items, int32 low, int32 high)
{
	if (low < high) {
		if (high - low < kQuickSortThreshold) {
			InsertionSort(items, low, high);
		} else {		
			bool isSorted;
			int pivot = Partition(items, low, high, isSorted);
			if (isSorted) return;

			QuickSort(items, low, pivot - 1);
			QuickSort(items, pivot + 1, high);
		}
	}	
}


class PointerListHelper : public AbstractPointerListHelper
{
public:
	PointerListHelper(_PointerList_::GenericCompareFunction compareFunc)
		: fCompareFunc(compareFunc)
	{
		// nothing to do
	}
	
	int Compare(const void *a, const void *b) {
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

	int Compare(const void *a, const void *b) {
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

	int Compare(const void *arg, const void *item) {
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
_PointerList_::SortItems(GenericCompareFunctionWithState compareFunc, void *state)
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
_PointerList_::HSortItems(GenericCompareFunctionWithState compareFunc, void *state)
{
	PointerListHelperWithState helper(compareFunc, state);
	helper.HSortItems(this);	
}


void *
_PointerList_::BinarySearch(const void *key, GenericCompareFunction compareFunc) const
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
_PointerList_::BinarySearchIndex(const void *key, GenericCompareFunction compareFunc) const
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
_PointerList_::BinarySearchIndexByPredicate(const void *key, UnaryPredicateGlue predicate) const
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

