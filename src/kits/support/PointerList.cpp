/* 
** Copyright 2003-2004, Stefano Ceccherini (burton666@libero.it). All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

// TODO: we need to include this as _PointerList_ is declared there.
// I wonder if we should add a PointerList.h file, then ObjectList.h would contain
// a copy of the declaration
#include <ObjectList.h>

#include <stdlib.h>

// TODO: I used bubble sort, which is notably the slowest possible sorting algorithm
// Why ? Laziness. Fix that!
// Test the class.


typedef int (* GenericCompareFunction)(const void *, const void *);
typedef int (* GenericCompareFunctionWithState)(const void *, const void *, void *);

	
static void *BSearch(const void *key, const void *start,
					size_t numItems, size_t size,
					GenericCompareFunction compareFunc);

static void *BSearchWithState(const void *key, const void *start,
							size_t numItems, size_t size,
							GenericCompareFunctionWithState compareFunc, void *state);


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
	//TODO: Does "H" stand for "Heap" ? Should we use Heap Sort here ?
	SortItems(sortFunction);
}


void
_PointerList_::HSortItems(GenericCompareFunctionWithState sortFunc, void *state)
{
	//TODO: Same as above
	SortItems(sortFunc, state);
}


void *
_PointerList_::BinarySearch(const void *key, GenericCompareFunction compareFunc) const
{
	return BSearch(key, Items(), CountItems(), sizeof(void *), compareFunc);
}


void *
_PointerList_::BinarySearch(const void *key,
			GenericCompareFunctionWithState compareFunc, void *state) const
{
	return BSearchWithState(key, Items(), CountItems(), sizeof(void *), compareFunc, state);
}


int32
_PointerList_::BinarySearchIndex(const void *key, GenericCompareFunction compareFunc) const
{
	void *item = BSearch(key, Items(), CountItems(), sizeof(void *), compareFunc);
	
	if (item)
		return IndexOf(item);

	return -1;
}


int32
_PointerList_::BinarySearchIndex(const void *key,
			GenericCompareFunctionWithState compareFunc, void *state) const
{
	void *item = BSearchWithState(key, Items(), CountItems(),
									sizeof(void *), compareFunc, state);
	
	if (item)
		return IndexOf(item);

	return -1;
}


int32
_PointerList_::BinarySearchIndexByPredicate(const void *key, UnaryPredicateGlue predicate) const
{
	int32 numItems = CountItems();
	int32 result = -1;
	for (int32 index = 0; index < numItems; index++) {
		result = predicate(ItemAtFast(index), (void *)key);
		if (result)
			break;
	}
	
	return result;
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


static void *
BSearch(const void *key, const void *start,
					size_t numItems, size_t size,
					GenericCompareFunction compareFunc)

{
	const char *base = (const char *)start;
	const void *p;

	int low;
	int high;
	int result;
	size_t i;
	for (low = -1, high = numItems; high - low > 1;) {
		i = (high + low) >> 1;
		p = (void *)(base + (i * size));
		
		result = compareFunc(key, *(void **)p);
		if (result == 0)
			break;
		else if (result < 0)
			high = i;
        else
			low  = i;
    }
    
	if (result == 0)
		return *(void **)p;

	return NULL;
}


static void *
BSearchWithState(const void *key, const void *start,
							size_t numItems, size_t size,
							GenericCompareFunctionWithState compareFunc, void *state)
{
		const char *base = (const char *)start;
	const void *p;

	int low;
	int high;
	int result;
	size_t i;
	for (low = -1, high = numItems; high - low > 1;) {
		i = (high + low) >> 1;
		p = (void *)(base + (i * size));
		
		result = compareFunc(key, *(void **)p, state);
		if (result == 0)
			break;
		else if (result < 0)
			high = i;
        else
			low  = i;
    }
    
	if (result == 0)
		return *(void **)p;

	return NULL;
}