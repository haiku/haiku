/*
 * Copyright 2009-2013, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef LIST_H
#define LIST_H


#include <new>
#include <stdlib.h>
#include <string.h>

#include <SupportDefs.h>


template <typename ItemType, bool PlainOldData, uint32 BlockSize = 8>
class List {
	typedef List<ItemType, PlainOldData, BlockSize> SelfType;
public:
	List()
		:
		fItems(NULL),
		fCount(0),
		fAllocatedCount(0)
	{
	}

	List(const SelfType& other)
		:
		fItems(NULL),
		fCount(0),
		fAllocatedCount(0)
	{
		*this = other;
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
		if (this == &other)
			return *this;

		if (PlainOldData) {
			if (_Resize(other.fCount))
				memcpy(fItems, other.fItems, fCount * sizeof(ItemType));
		} else {
			// Make sure to call destructors of old objects.
			// NOTE: Another option would be to use
			// ItemType::operator=(const ItemType& other), but then
			// we would need to be carefull which objects are already
			// initialized. Also ItemType would be required to implement the
			// operator, while doing it this way requires only a copy
			// constructor.
			_Resize(0);
			for (uint32 i = 0; i < other.fCount; i++) {
				if (!Add(other.ItemAtFast(i)))
					break;
			}
		}
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

	inline bool Add(const ItemType& copyFrom)
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

	inline bool Add(const ItemType& copyFrom, int32 index)
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

	ItemType*		fItems;
	ItemType		fNullItem;
	uint32			fCount;
	uint32			fAllocatedCount;
};


#endif // LIST_H
