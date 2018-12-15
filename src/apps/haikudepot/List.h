/*
 * Copyright 2009-2013, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2018, Andrew Lindesay <apl@lindesay.co.nz>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef LIST_H
#define LIST_H


#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SupportDefs.h>


#define BINARY_SEARCH_LINEAR_THRESHOLD 4


template <typename ItemType, bool PlainOldData, uint32 BlockSize = 8>
class List {
	typedef List<ItemType, PlainOldData, BlockSize> SelfType;
	typedef int32 (*CompareItemFn)(const ItemType& one, const ItemType& two);
	typedef int32 (*CompareContextFn)(const void* context,
		const ItemType& item);
public:
	List()
		:
		fItems(NULL),
		fCount(0),
		fAllocatedCount(0),
		fCompareItemsFunction(NULL),
		fCompareContextFunction(NULL)
	{
	}

	List(CompareItemFn compareItemsFunction,
		CompareContextFn compareContextFunction)
		:
		fItems(NULL),
		fCount(0),
		fAllocatedCount(0),
		fCompareItemsFunction(compareItemsFunction),
		fCompareContextFunction(compareContextFunction)
	{
	}

	List(const SelfType& other)
		:
		fItems(NULL),
		fCount(0),
		fAllocatedCount(0),
		fCompareItemsFunction(other.fCompareItemsFunction),
		fCompareContextFunction(other.fCompareContextFunction)
	{
		_AddAllVerbatim(other);
	}

	virtual ~List()
	{
		if (!PlainOldData) {
			// Make sure to call destructors of old objects.
			_Resize(0);
		}
		free(fItems);
	}

	SelfType& operator=(const SelfType& other)
	{
		if (this != &other)
			_AddAllVerbatim(other);
		return *this;
	}


	bool operator==(const SelfType& other) const
	{
		if (this == &other)
			return true;

		if (fCount != other.fCount)
			return false;
		if (fCount == 0)
			return true;

		if (PlainOldData) {
			return memcmp(fItems, other.fItems,
				fCount * sizeof(ItemType)) == 0;
		} else {
			for (uint32 i = 0; i < other.fCount; i++) {
				if (ItemAtFast(i) != other.ItemAtFast(i))
					return false;
			}
		}
		return true;
	}

	bool operator!=(const SelfType& other) const
	{
		return !(*this == other);
	}

	inline void Clear()
	{
		_Resize(0);
	}

	inline bool IsEmpty() const
	{
		return fCount == 0;
	}

	inline int32 CountItems() const
	{
		return fCount;
	}

/*! Note that the use of this method will depend on the list being sorted.
*/

	inline int32 Search(const void* context) const
	{
		if (fCount == 0 || fCompareContextFunction == NULL)
			return -1;

		return _BinarySearchBounded(context, 0, fCount - 1);
	}

	/*! This function will add the item into the list.  If the list is sorted
	    then the item will be insert in order.  If the list is not sorted then
	    the item will be inserted at the end of the list.
	*/

	inline bool Add(const ItemType& copyFrom)
	{
		if (fCompareItemsFunction != NULL) {
			return _AddOrdered(copyFrom);
		}

		return _AddTail(copyFrom);
	}

	inline bool Add(const ItemType& copyFrom, int32 index)
	{
		// if the list is sorted then ignore the index and just insert in
		// order.
		if (fCompareItemsFunction != NULL) {
			return _AddOrdered(copyFrom);
		}

		return _AddAtIndex(copyFrom, index);
	}

	inline bool Remove()
	{
		if (fCount > 0) {
			_Resize(fCount - 1);
			return true;
		}
		return false;
	}

	inline bool Remove(int32 index)
	{
		if (index < 0 || index >= (int32)fCount)
			return false;

		if (!PlainOldData) {
			ItemType* object = fItems + index;
			object->~ItemType();
		}

		int32 nextIndex = index + 1;
		if ((int32)fCount > nextIndex) {
			memcpy(fItems + index, fItems + nextIndex,
				(fCount - nextIndex) * sizeof(ItemType));
		}

		fCount--;
		return true;
	}

	inline bool Remove(const ItemType& item)
	{
		return Remove(IndexOf(item));
	}

	inline bool Replace(int32 index, const ItemType& copyFrom)
	{
		if (fCompareItemsFunction != NULL) {
			bool result = Remove(index);
			_AddOrdered(copyFrom);
			return result;
		}

		if (index < 0 || index >= (int32)fCount)
			return false;

		ItemType* item = fItems + index;
		// Initialize the new object from the original.
		if (!PlainOldData) {
			item->~ItemType();
			new (item) ItemType(copyFrom);
		} else
			*item = copyFrom;
		return true;
	}

	inline const ItemType& ItemAt(int32 index) const
	{
		if (index < 0 || index >= (int32)fCount)
			return fNullItem;
		return ItemAtFast(index);
	}

	inline const ItemType& ItemAtFast(int32 index) const
	{
		return *(fItems + index);
	}

	inline const ItemType& LastItem() const
	{
		if (fCount == 0)
			return fNullItem;
		return ItemAt((int32)fCount - 1);
	}

	inline int32 IndexOf(const ItemType& item) const
	{
		for (uint32 i = 0; i < fCount; i++) {
			if (ItemAtFast(i) == item)
				return i;
		}
		return -1;
	}

	inline bool Contains(const ItemType& item) const
	{
		return IndexOf(item) >= 0;
	}

private:
	inline int32 _BinarySearchLinearBounded(
		const void* context,
		int32 start, int32 end) const
	{
		for(int32 i = start; i <= end; i++) {
			if (fCompareContextFunction(context, ItemAtFast(i)) == 0)
				return i;
		}

		return -1;
	}

	inline int32 _BinarySearchBounded(
		const void* context, int32 start, int32 end) const
	{
		if (end - start < BINARY_SEARCH_LINEAR_THRESHOLD)
			return _BinarySearchLinearBounded(context, start, end);

		int32 mid = start + ((end - start) >> 1);

		if (fCompareContextFunction(context, ItemAtFast(mid)) >= 0)
			return _BinarySearchBounded(context, mid, end);
		return _BinarySearchBounded(context, start, mid - 1);
	}

	inline void _AddAllVerbatim(const SelfType& other)
	{
		if (PlainOldData) {
			if (_Resize(other.fCount))
				memcpy(fItems, other.fItems, fCount * sizeof(ItemType));
		} else {
			// Make sure to call destructors of old objects.
			// NOTE: Another option would be to use
			// ItemType::operator=(const ItemType& other), but then
			// we would need to be careful which objects are already
			// initialized. Also ItemType would be required to implement the
			// operator, while doing it this way requires only a copy
			// constructor.
			_Resize(0);
			for (uint32 i = 0; i < other.fCount; i++) {
				if (!Add(other.ItemAtFast(i)))
					break;
			}
		}
	}


	inline bool _AddOrderedLinearBounded(
		const ItemType& copyFrom, int32 start, int32 end)
	{
		for(int32 i = start; i <= (end + 1); i++) {
			bool greaterBefore = (i == start)
				|| (fCompareItemsFunction(copyFrom, ItemAtFast(i - 1)) > 0);

			if (greaterBefore) {
				bool lessAfter = (i == end + 1)
					|| (fCompareItemsFunction(copyFrom, ItemAtFast(i)) <= 0);

				if (lessAfter)
					return _AddAtIndex(copyFrom, i);
			}
		}

		printf("illegal state; unable to insert item into list\n");
		exit(EXIT_FAILURE);
	}

	inline bool _AddOrderedBounded(
		const ItemType& copyFrom, int32 start, int32 end)
	{
		if(end - start < BINARY_SEARCH_LINEAR_THRESHOLD)
			return _AddOrderedLinearBounded(copyFrom, start, end);

		int32 mid = start + ((end - start) >> 1);

		if (fCompareItemsFunction(copyFrom, ItemAtFast(mid)) >= 0)
			return _AddOrderedBounded(copyFrom, mid, end);
		return _AddOrderedBounded(copyFrom, start, mid - 1);
	}

	inline bool _AddTail(const ItemType& copyFrom)
	{
		if (_Resize(fCount + 1)) {
			ItemType* item = fItems + fCount - 1;
			// Initialize the new object from the original.
			if (!PlainOldData)
				new (item) ItemType(copyFrom);
			else
				*item = copyFrom;
			return true;
		}
		return false;
	}


	inline bool _AddAtIndex(const ItemType& copyFrom, int32 index)
	{
		if (index < 0 || index > (int32)fCount)
			return false;

		if (!_Resize(fCount + 1))
			return false;

		int32 nextIndex = index + 1;
		if ((int32)fCount > nextIndex)
			memmove(fItems + nextIndex, fItems + index,
				(fCount - nextIndex) * sizeof(ItemType));

		ItemType* item = fItems + index;
		if (!PlainOldData)
			new (item) ItemType(copyFrom);
		else
			*item = copyFrom;

		return true;
	}


	inline bool _AddOrdered(const ItemType& copyFrom)
	{
		// special case
		if (fCount == 0
			|| fCompareItemsFunction(copyFrom, ItemAtFast(fCount - 1)) > 0) {
			return _AddTail(copyFrom);
		}

		return _AddOrderedBounded(copyFrom, 0, fCount - 1);
	}

	inline bool _Resize(uint32 count)
	{
		if (count > fAllocatedCount) {
			uint32 allocationCount = (count + BlockSize - 1)
				/ BlockSize * BlockSize;
			ItemType* items = reinterpret_cast<ItemType*>(
				realloc(fItems, allocationCount * sizeof(ItemType)));
			if (items == NULL)
				return false;
			fItems = items;

			fAllocatedCount = allocationCount;
		} else if (count < fCount) {
			if (!PlainOldData) {
				// Uninit old objects so that we can re-use them when
				// appending objects without the need to re-allocate.
				for (uint32 i = count; i < fCount; i++) {
					ItemType* object = fItems + i;
					object->~ItemType();
				}
			}
		}
		fCount = count;
		return true;
	}

	ItemType*			fItems;
	ItemType			fNullItem;
	uint32				fCount;
	uint32				fAllocatedCount;
	CompareItemFn		fCompareItemsFunction;
	CompareContextFn	fCompareContextFunction;
};


#endif // LIST_H
