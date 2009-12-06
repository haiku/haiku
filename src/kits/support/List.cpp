/*
 * Copyright 2001-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		The Storage kit Team
 *		Isaac Yonemoto
 *		Rene Gollent
 *		Stephan Aßmus
 */

//!	BList class provides storage for pointers. Not thread safe.


#include <List.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// helper function
static inline void
move_items(void** items, int32 offset, int32 count)
{
	if (count > 0 && offset != 0)
		memmove(items + offset, items, count * sizeof(void*));
}


BList::BList(int32 count)
	:
	fObjectList(NULL),
	fPhysicalSize(0),
	fItemCount(0),
	fBlockSize(count),
	fResizeThreshold(0)
{
	if (fBlockSize <= 0)
		fBlockSize = 1;
	_ResizeArray(fItemCount);
}


BList::BList(const BList& anotherList)
	:
	fObjectList(NULL),
	fPhysicalSize(0),
	fItemCount(0),
	fBlockSize(anotherList.fBlockSize)
{
	*this = anotherList;
}


BList::~BList()
{
	free(fObjectList);
}


BList&
BList::operator=(const BList& list)
{
	if (&list != this) {
		fBlockSize = list.fBlockSize;
		if (_ResizeArray(list.fItemCount)) {
			fItemCount = list.fItemCount;
			memcpy(fObjectList, list.fObjectList, fItemCount * sizeof(void*));
		}
	}

	return *this;
}


bool
BList::operator==(const BList& list) const
{
	if (&list == this)
		return true;

	if (list.fItemCount != fItemCount)
		return false;

	if (fItemCount > 0) {
		return memcmp(fObjectList, list.fObjectList,
			fItemCount * sizeof(void*)) == 0;
	}

	return true;
}


bool
BList::operator!=(const BList& list) const
{
	return !(*this == list);
}


bool
BList::AddItem(void* item, int32 index)
{
	if (index < 0 || index > fItemCount)
		return false;

	bool result = true;

	if (fItemCount + 1 > fPhysicalSize)
		result = _ResizeArray(fItemCount + 1);
	if (result) {
		++fItemCount;
		move_items(fObjectList + index, 1, fItemCount - index - 1);
		fObjectList[index] = item;
	}
	return result;
}


bool
BList::AddItem(void* item)
{
	bool result = true;
	if (fPhysicalSize > fItemCount) {
		fObjectList[fItemCount] = item;
		++fItemCount;
	} else {
		if ((result = _ResizeArray(fItemCount + 1))) {
			fObjectList[fItemCount] = item;
			++fItemCount;
		}
	}
	return result;
}


bool
BList::AddList(const BList* list, int32 index)
{
	bool result = (list && index >= 0 && index <= fItemCount);
	if (result && list->fItemCount > 0) {
		int32 count = list->fItemCount;
		if (fItemCount + count > fPhysicalSize)
			result = _ResizeArray(fItemCount + count);
		if (result) {
			fItemCount += count;
			move_items(fObjectList + index, count, fItemCount - index - count);
			memcpy(fObjectList + index, list->fObjectList,
				list->fItemCount * sizeof(void*));
		}
	}
	return result;
}


bool
BList::AddList(const BList* list)
{
	bool result = (list != NULL);
	if (result && list->fItemCount > 0) {
		int32 index = fItemCount;
		int32 count = list->fItemCount;
		if (fItemCount + count > fPhysicalSize)
			result = _ResizeArray(fItemCount + count);
		if (result) {
			fItemCount += count;
			memcpy(fObjectList + index, list->fObjectList,
				list->fItemCount * sizeof(void*));
		}
	}
	return result;
}


bool
BList::RemoveItem(void* item)
{
	int32 index = IndexOf(item);
	bool result = (index >= 0);
	if (result)
		RemoveItem(index);
	return result;
}


void*
BList::RemoveItem(int32 index)
{
	void* item = NULL;
	if (index >= 0 && index < fItemCount) {
		item = fObjectList[index];
		move_items(fObjectList + index + 1, -1, fItemCount - index - 1);
		--fItemCount;
		if (fItemCount <= fResizeThreshold)
			_ResizeArray(fItemCount);
	}
	return item;
}


bool
BList::RemoveItems(int32 index, int32 count)
{
	bool result = (index >= 0 && index <= fItemCount);
	if (result) {
		if (index + count > fItemCount)
			count = fItemCount - index;
		if (count > 0) {
			move_items(fObjectList + index + count, -count,
					   fItemCount - index - count);
			fItemCount -= count;
			if (fItemCount <= fResizeThreshold)
				_ResizeArray(fItemCount);
		} else
			result = false;
	}
	return result;
}


bool
BList::ReplaceItem(int32 index, void* newItem)
{
	bool result = false;

	if (index >= 0 && index < fItemCount) {
		fObjectList[index] = newItem;
		result = true;
	}
	return result;
}


void
BList::MakeEmpty()
{
	fItemCount = 0;
	_ResizeArray(0);
}


// #pragma mark - Reordering items.


void
BList::SortItems(int (*compareFunc)(const void*, const void*))
{
	if (compareFunc)
		qsort(fObjectList, fItemCount, sizeof(void*), compareFunc);
}


bool
BList::SwapItems(int32 indexA, int32 indexB)
{
	bool result = false;

	if (indexA >= 0 && indexA < fItemCount
		&& indexB >= 0 && indexB < fItemCount) {

		void* tmpItem = fObjectList[indexA];
		fObjectList[indexA] = fObjectList[indexB];
		fObjectList[indexB] = tmpItem;

		result = true;
	}

	return result;
}


/*! This moves a list item from posititon a to position b, moving the
	appropriate block of list elements to make up for the move. For example,
	in the array:
	A B C D E F G H I J
		Moveing 1(B)->6(G) would result in this:
	A C D E F G B H I J
*/
bool
BList::MoveItem(int32 fromIndex, int32 toIndex)
{
	if ((fromIndex >= fItemCount) || (toIndex >= fItemCount) || (fromIndex < 0)
		|| (toIndex < 0)) {
		return false;
	}

	void* tmpMover = fObjectList[fromIndex];
	if (fromIndex < toIndex) {
		memmove(fObjectList + fromIndex, fObjectList + fromIndex + 1,
			(toIndex - fromIndex) * sizeof(void*));
	} else if (fromIndex > toIndex) {
		memmove(fObjectList + toIndex + 1, fObjectList + toIndex,
			(fromIndex - toIndex) * sizeof(void*));
	};
	fObjectList[toIndex] = tmpMover;

	return true;
}


// #pragma mark - Retrieving items.


void*
BList::ItemAt(int32 index) const
{
	void *item = NULL;
	if (index >= 0 && index < fItemCount)
		item = fObjectList[index];
	return item;
}


void*
BList::FirstItem() const
{
	void *item = NULL;
	if (fItemCount > 0)
		item = fObjectList[0];
	return item;
}


void*
BList::ItemAtFast(int32 index) const
{
	return fObjectList[index];
}


void*
BList::Items() const
{
	return fObjectList;
}


void*
BList::LastItem() const
{
	void* item = NULL;
	if (fItemCount > 0)
		item = fObjectList[fItemCount - 1];
	return item;
}


// #pragma mark - Querying the list.


bool
BList::HasItem(void* item) const
{
	return (IndexOf(item) >= 0);
}


bool
BList::HasItem(const void* item) const
{
	return (IndexOf(item) >= 0);
}


int32
BList::IndexOf(void* item) const
{
	for (int32 i = 0; i < fItemCount; i++) {
		if (fObjectList[i] == item)
			return i;
	}
	return -1;
}


int32
BList::IndexOf(const void* item) const
{
	for (int32 i = 0; i < fItemCount; i++) {
		if (fObjectList[i] == item)
			return i;
	}
	return -1;
}


int32
BList::CountItems() const
{
	return fItemCount;
}


bool
BList::IsEmpty() const
{
	return fItemCount == 0;
}


// #pragma mark - Iterating over the list.

/*!	Iterate a function over the whole list. If the function outputs a true
	value, then the process is terminated.
*/
void
BList::DoForEach(bool (*func)(void*))
{
	if (func == NULL)
		return;

	bool terminate = false;
	int32 index = 0;

	while ((!terminate) && (index < fItemCount)) {
		terminate = func(fObjectList[index]);
		index++;
	}
}


/*!	Iterate a function over the whole list. If the function outputs a true
	value, then the process is terminated. This version takes an additional
	argument which is passed to the function.
*/
void
BList::DoForEach(bool (*func)(void*, void*), void* arg)
{
	if (func == NULL)
		return;

	bool terminate = false; int32 index = 0;
	while ((!terminate) && (index < fItemCount)) {
		terminate = func(fObjectList[index], arg);
		index++;
	}
}


#if (__GNUC__ == 2)

// This is somewhat of a hack for backwards compatibility -
// the reason these functions are defined this way rather than simply
// being made private members is that if they are included, then whenever
// gcc encounters someone calling AddList() with a non-const BList pointer,
// it will try to use the private version and fail with a compiler error.

// obsolete AddList(BList* list, int32 index) and AddList(BList* list)
// AddList
extern "C" bool
AddList__5BListP5BListl(BList* self, BList* list, int32 index)
{
	return self->AddList((const BList*)list, index);
}

// AddList
extern "C" bool
AddList__5BListP5BList(BList* self, BList* list)
{
	return self->AddList((const BList*)list);
}
#endif

// FBC
void BList::_ReservedList1() {}
void BList::_ReservedList2() {}


/*!	Resizes fObjectList to be large enough to contain count items.
*/
bool
BList::_ResizeArray(int32 count)
{
	bool result = true;
	// calculate the new physical size
	// by doubling the existing size
	// until we can hold at least count items
	int32 newSize = fPhysicalSize > 0 ? fPhysicalSize : fBlockSize;
	int32 targetSize = count;
	if (targetSize <= 0)
		targetSize = fBlockSize;
	if (targetSize > fPhysicalSize) {
		while (newSize < targetSize)
			newSize <<= 1;
	} else if (targetSize <= fResizeThreshold) {
		newSize = fResizeThreshold;
	}

	// resize if necessary
	if (newSize != fPhysicalSize) {
		void** newObjectList
			= (void**)realloc(fObjectList, newSize * sizeof(void*));
		if (newObjectList) {
			fObjectList = newObjectList;
			fPhysicalSize = newSize;
			// set our lower bound to either 1/4
			//of the current physical size, or 0
			fResizeThreshold = fPhysicalSize >> 2 >= fBlockSize
				? fPhysicalSize >> 2 : 0;
		} else
			result = false;
	}
	return result;
}

