// List.cpp
// Just here to be able to compile and test BResources.
// To be replaced by the OpenBeOS version to be provided by the IK Team.

#include "List.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// constructor
BList::BList(int32 count)
	  :		 fObjectList(NULL),
			 fPhysicalSize(0),
			 fItemCount(0),
			 fBlockSize(count)
{
	if (fBlockSize <= 0)
		fBlockSize = 1;
	Resize(fItemCount);
}

// copy constructor
BList::BList(const BList& anotherList)
	  :		 fObjectList(NULL),
			 fPhysicalSize(0),
			 fItemCount(0),
			 fBlockSize(anotherList.fBlockSize)
{
	*this = anotherList;
}

// destructor
BList::~BList()
{
	free(fObjectList);
}

// helper function
static inline
void
move_items(void** items, int32 offset, int32 count)
{
	if (count > 0 && offset != 0)
		memmove(items + offset, items, count * sizeof(void*));
}

// AddItem
bool
BList::AddItem(void *item, int32 index)
{
	bool result = (index >= 0 && index <= fItemCount
				   && Resize(fItemCount + 1));
	if (result) {
		move_items(fObjectList + index, 1, fItemCount - index - 1);
		fObjectList[index] = item;
	}
	return result;
}

// AddItem
bool
BList::AddItem(void *item)
{
	bool result = true;
	if ((int32)fPhysicalSize > fItemCount) {
		fObjectList[fItemCount] = item;
		fItemCount++;
	} else {
		if ((result = Resize(fItemCount + 1)))
			fObjectList[fItemCount - 1] = item;
	}
	return result;
}

// AddList
bool
BList::AddList(BList *list, int32 index)
{
	bool result = (list && index >= 0 && index <= fItemCount);
	if (result && list->fItemCount > 0) {
		int32 count = list->fItemCount;
		result = Resize(fItemCount + count);
		if (result) {
			move_items(fObjectList + index, count, fItemCount - index - count);
			memcpy(fObjectList + index, list->fObjectList,
				   list->fItemCount * sizeof(void *));
		}
	}
	return result;
}

// AddList
bool
BList::AddList(BList *list)
{
	bool result = (list);
	if (result && list->fItemCount > 0) {
		int32 index = fItemCount;
		int32 count = list->fItemCount;
		result = Resize(fItemCount + count);
		if (result) {
			memcpy(fObjectList + index, list->fObjectList,
				   list->fItemCount * sizeof(void *));
		}
	}
	return result;
}

// CountItems
int32
BList::CountItems() const
{
	return fItemCount;
}

// DoForEach
void
BList::DoForEach(bool (*func)(void *))
{
	if (func) {
		for (int32 i = 0; i < fItemCount; i++)
			(*func)(fObjectList[i]);
	}
}

// DoForEach
void
BList::DoForEach(bool (*func)(void *, void *), void *arg2)
{
	if (func) {
		for (int32 i = 0; i < fItemCount; i++)
			(*func)(fObjectList[i], arg2);
	}
}

// FirstItem
void *
BList::FirstItem() const
{
	void *item = NULL;
	if (fItemCount > 0)
		item = fObjectList[0];
	return item;
}

// HasItem
bool
BList::HasItem(void *item) const
{
	return (IndexOf(item) >= 0);
}

// IndexOf
int32
BList::IndexOf(void *item) const
{
	for (int32 i = 0; i < fItemCount; i++) {
		if (fObjectList[i] == item)
			return i;
	}
	return -1;
}

// IsEmpty
bool
BList::IsEmpty() const
{
	return (fItemCount == 0);
}

// ItemAt
void *
BList::ItemAt(int32 index) const
{
	void *item = NULL;
	if (index >= 0 && index < fItemCount)
		item = fObjectList[index];
	return item;
}

// Items
void *
BList::Items() const
{
	return fObjectList;
}

// LastItem
void *
BList::LastItem() const
{
	void *item = NULL;
	if (fItemCount > 0)
		item = fObjectList[fItemCount - 1];
	return item;
}

// MakeEmpty
void
BList::MakeEmpty()
{
	Resize(0);
}

// RemoveItem
bool
BList::RemoveItem(void *item)
{
	int32 index = IndexOf(item);
	bool result = (index >= 0);
	if (result)
		RemoveItem(index);
	return result;
}

// RemoveItem
void *
BList::RemoveItem(int32 index)
{
	void *item = NULL;
	if (index >= 0 && index < fItemCount) {
		item = fObjectList[index];
		move_items(fObjectList + index + 1, -1, fItemCount - index - 1);
		Resize(fItemCount - 1);
	}
	return item;
}

// RemoveItems
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
			Resize(fItemCount - count);
		} else
			result = false;
	}
	return result;
}

// SortItems
void
BList::SortItems(int (*compareFunc)(const void *, const void *))
{
	if (compareFunc)
		qsort(fObjectList, fItemCount, sizeof(void *), compareFunc);
}

// =
BList&
BList::operator =(const BList &list)
{
	fBlockSize = list.fBlockSize;
	Resize(list.fItemCount);
	memcpy(fObjectList, list.fObjectList, fItemCount * sizeof(void*));
	return *this;
}

// reserved
void
BList::_ReservedList1()
{
}

// reserved
void
BList::_ReservedList2()
{
}

// Resize
//
// Resizes fObjectList to be large enough to contain count items.
// fItemCount is adjusted accordingly.
bool
BList::Resize(int32 count)
{
	bool result = true;
	// calculate the new physical size
	int32 newSize = count;
	if (newSize <= 0)
		newSize = 1;
	newSize = ((newSize - 1) / fBlockSize + 1) * fBlockSize;
	// resize if necessary
	if ((size_t)newSize != fPhysicalSize) {
		void** newObjectList
			= (void**)realloc(fObjectList, newSize * sizeof(void*));
		if (newObjectList) {
			fObjectList = newObjectList;
			fPhysicalSize = newSize;
		} else
			result = false;
	}
	if (result)
		fItemCount = count;
	return result;
}

