// List.h
//
// Copyright (c) 2003, Ingo Weinhold (bonefish@cs.tu-berlin.de)
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
// 
// Except as contained in this notice, the name of a copyright holder shall
// not be used in advertising or otherwise to promote the sale, use or other
// dealings in this Software without prior written authorization of the
// copyright holder.

#ifndef LIST_H
#define LIST_H

#include <new>
#include <stdlib.h>
#include <string.h>

#include <SupportDefs.h>

template<typename ITEM>
class DefaultDefaultItemCreator {
public:
	static inline ITEM GetItem() { return ITEM(0); }
};

/*!
	\class List
	\brief A generic list implementation.
*/
template<typename ITEM,
		 typename DEFAULT_ITEM_SUPPLIER = DefaultDefaultItemCreator<ITEM> >
class List {
public:
	typedef ITEM					item_t;
	typedef List					list_t;

private:
	static item_t					sDefaultItem;
	static const size_t				kDefaultChunkSize = 10;
	static const size_t				kMaximalChunkSize = 1024 * 1024;

public:
	List(size_t chunkSize = kDefaultChunkSize);
	~List();

	inline const item_t &GetDefaultItem() const;
	inline item_t &GetDefaultItem();

	bool AddItem(const item_t &item, int32 index);
	bool AddItem(const item_t &item);
//	bool AddList(list_t *list, int32 index);
//	bool AddList(list_t *list);

	bool RemoveItem(const item_t &item);
	bool RemoveItem(int32 index);

	bool ReplaceItem(int32 index, const item_t &item);

	bool MoveItem(int32 oldIndex, int32 newIndex);

	void MakeEmpty();

	int32 CountItems() const;
	bool IsEmpty() const;
	const item_t &ItemAt(int32 index) const;
	item_t &ItemAt(int32 index);
	const item_t *Items() const;
	int32 IndexOf(const item_t &item) const;
	bool HasItem(const item_t &item) const;

	// debugging
	int32 GetCapacity() const	{ return fCapacity; }

private:
	inline static void _MoveItems(item_t* items, int32 offset, int32 count);
	bool _Resize(size_t count);

private:
	size_t	fCapacity;
	size_t	fChunkSize;
	int32	fItemCount;
	item_t	*fItems;
};

// sDefaultItem
template<typename ITEM, typename DEFAULT_ITEM_SUPPLIER>
typename List<ITEM, DEFAULT_ITEM_SUPPLIER>::item_t
	List<ITEM, DEFAULT_ITEM_SUPPLIER>::sDefaultItem(
		DEFAULT_ITEM_SUPPLIER::GetItem());

// constructor
template<typename ITEM, typename DEFAULT_ITEM_SUPPLIER>
List<ITEM, DEFAULT_ITEM_SUPPLIER>::List(size_t chunkSize)
	: fCapacity(0),
	  fChunkSize(chunkSize),
	  fItemCount(0),
	  fItems(NULL)
{
	if (fChunkSize == 0 || fChunkSize > kMaximalChunkSize)
		fChunkSize = kDefaultChunkSize;
	_Resize(0);
}

// destructor
template<typename ITEM, typename DEFAULT_ITEM_SUPPLIER>
List<ITEM, DEFAULT_ITEM_SUPPLIER>::~List()
{
	MakeEmpty();
	free(fItems);
}

// GetDefaultItem
template<typename ITEM, typename DEFAULT_ITEM_SUPPLIER>
inline
const typename List<ITEM, DEFAULT_ITEM_SUPPLIER>::item_t &
List<ITEM, DEFAULT_ITEM_SUPPLIER>::GetDefaultItem() const
{
	return sDefaultItem;
}

// GetDefaultItem
template<typename ITEM, typename DEFAULT_ITEM_SUPPLIER>
inline
typename List<ITEM, DEFAULT_ITEM_SUPPLIER>::item_t &
List<ITEM, DEFAULT_ITEM_SUPPLIER>::GetDefaultItem()
{
	return sDefaultItem;
}

// _MoveItems
template<typename ITEM, typename DEFAULT_ITEM_SUPPLIER>
inline
void
List<ITEM, DEFAULT_ITEM_SUPPLIER>::_MoveItems(item_t* items, int32 offset, int32 count)
{
	if (count > 0 && offset != 0)
		memmove(items + offset, items, count * sizeof(item_t));
}

// AddItem
template<typename ITEM, typename DEFAULT_ITEM_SUPPLIER>
bool
List<ITEM, DEFAULT_ITEM_SUPPLIER>::AddItem(const item_t &item, int32 index)
{
	bool result = (index >= 0 && index <= fItemCount
				   && _Resize(fItemCount + 1));
	if (result) {
		_MoveItems(fItems + index, 1, fItemCount - index - 1);
		new(fItems + index) item_t(item);
	}
	return result;
}

// AddItem
template<typename ITEM, typename DEFAULT_ITEM_SUPPLIER>
bool
List<ITEM, DEFAULT_ITEM_SUPPLIER>::AddItem(const item_t &item)
{
	bool result = true;
	if ((int32)fCapacity > fItemCount) {
		new(fItems + fItemCount) item_t(item);
		fItemCount++;
	} else {
		if ((result = _Resize(fItemCount + 1)))
			new(fItems + (fItemCount - 1)) item_t(item);
	}
	return result;
}

// These don't use the copy constructor!
/*
// AddList
template<typename ITEM, typename DEFAULT_ITEM_SUPPLIER>
bool
List<ITEM, DEFAULT_ITEM_SUPPLIER>::AddList(list_t *list, int32 index)
{
	bool result = (list && index >= 0 && index <= fItemCount);
	if (result && list->fItemCount > 0) {
		int32 count = list->fItemCount;
		result = _Resize(fItemCount + count);
		if (result) {
			_MoveItems(fItems + index, count, fItemCount - index - count);
			memcpy(fItems + index, list->fItems,
				   list->fItemCount * sizeof(item_t));
		}
	}
	return result;
}

// AddList
template<typename ITEM, typename DEFAULT_ITEM_SUPPLIER>
bool
List<ITEM, DEFAULT_ITEM_SUPPLIER>::AddList(list_t *list)
{
	bool result = (list);
	if (result && list->fItemCount > 0) {
		int32 index = fItemCount;
		int32 count = list->fItemCount;
		result = _Resize(fItemCount + count);
		if (result) {
			memcpy(fItems + index, list->fItems,
				   list->fItemCount * sizeof(item_t));
		}
	}
	return result;
}
*/

// RemoveItem
template<typename ITEM, typename DEFAULT_ITEM_SUPPLIER>
bool
List<ITEM, DEFAULT_ITEM_SUPPLIER>::RemoveItem(const item_t &item)
{
	int32 index = IndexOf(item);
	bool result = (index >= 0);
	if (result)
		RemoveItem(index);
	return result;
}

// RemoveItem
template<typename ITEM, typename DEFAULT_ITEM_SUPPLIER>
bool
List<ITEM, DEFAULT_ITEM_SUPPLIER>::RemoveItem(int32 index)
{
	if (index >= 0 && index < fItemCount) {
		fItems[index].~item_t();
		_MoveItems(fItems + index + 1, -1, fItemCount - index - 1);
		_Resize(fItemCount - 1);
		return true;
	}
	return false;
}

// ReplaceItem
template<typename ITEM, typename DEFAULT_ITEM_SUPPLIER>
bool
List<ITEM, DEFAULT_ITEM_SUPPLIER>::ReplaceItem(int32 index, const item_t &item)
{
	if (index >= 0 && index < fItemCount) {
		fItems[index] = item;
		return true;
	}
	return false;
}

// MoveItem
template<typename ITEM, typename DEFAULT_ITEM_SUPPLIER>
bool
List<ITEM, DEFAULT_ITEM_SUPPLIER>::MoveItem(int32 oldIndex, int32 newIndex)
{
	if (oldIndex >= 0 && oldIndex < fItemCount
		&& newIndex >= 0 && newIndex <= fItemCount) {
		if (oldIndex < newIndex - 1) {
			item_t item = fItems[oldIndex];
			_MoveItems(fItems + oldIndex + 1, -1, newIndex - oldIndex - 1);
			fItems[newIndex] = item;
		} else if (oldIndex > newIndex) {
			item_t item = fItems[oldIndex];
			_MoveItems(fItems + newIndex, 1, oldIndex - newIndex);
			fItems[newIndex] = item;
		}
		return true;
	}
	return false;
}

// MakeEmpty
template<typename ITEM, typename DEFAULT_ITEM_SUPPLIER>
void
List<ITEM, DEFAULT_ITEM_SUPPLIER>::MakeEmpty()
{
	for (int32 i = 0; i < fItemCount; i++)
		fItems[i].~item_t();
	_Resize(0);
}

// CountItems
template<typename ITEM, typename DEFAULT_ITEM_SUPPLIER>
int32
List<ITEM, DEFAULT_ITEM_SUPPLIER>::CountItems() const
{
	return fItemCount;
}

// IsEmpty
template<typename ITEM, typename DEFAULT_ITEM_SUPPLIER>
bool
List<ITEM, DEFAULT_ITEM_SUPPLIER>::IsEmpty() const
{
	return (fItemCount == 0);
}

// ItemAt
template<typename ITEM, typename DEFAULT_ITEM_SUPPLIER>
const typename List<ITEM, DEFAULT_ITEM_SUPPLIER>::item_t &
List<ITEM, DEFAULT_ITEM_SUPPLIER>::ItemAt(int32 index) const
{
	if (index >= 0 && index < fItemCount)
		return fItems[index];
	return sDefaultItem;
}

// ItemAt
template<typename ITEM, typename DEFAULT_ITEM_SUPPLIER>
typename List<ITEM, DEFAULT_ITEM_SUPPLIER>::item_t &
List<ITEM, DEFAULT_ITEM_SUPPLIER>::ItemAt(int32 index)
{
	if (index >= 0 && index < fItemCount)
		return fItems[index];
	return sDefaultItem;
}

// Items
template<typename ITEM, typename DEFAULT_ITEM_SUPPLIER>
const typename List<ITEM, DEFAULT_ITEM_SUPPLIER>::item_t *
List<ITEM, DEFAULT_ITEM_SUPPLIER>::Items() const
{
	return fItems;
}

// IndexOf
template<typename ITEM, typename DEFAULT_ITEM_SUPPLIER>
int32
List<ITEM, DEFAULT_ITEM_SUPPLIER>::IndexOf(const item_t &item) const
{
	for (int32 i = 0; i < fItemCount; i++) {
		if (fItems[i] == item)
			return i;
	}
	return -1;
}

// HasItem
template<typename ITEM, typename DEFAULT_ITEM_SUPPLIER>
bool
List<ITEM, DEFAULT_ITEM_SUPPLIER>::HasItem(const item_t &item) const
{
	return (IndexOf(item) >= 0);
}

// _Resize
template<typename ITEM, typename DEFAULT_ITEM_SUPPLIER>
bool
List<ITEM, DEFAULT_ITEM_SUPPLIER>::_Resize(size_t count)
{
	bool result = true;
	// calculate the new capacity
	int32 newSize = count;
	if (newSize <= 0)
		newSize = 1;
	newSize = ((newSize - 1) / fChunkSize + 1) * fChunkSize;
	// resize if necessary
	if ((size_t)newSize != fCapacity) {
		item_t* newItems
			= (item_t*)realloc(fItems, newSize * sizeof(item_t));
		if (newItems) {
			fItems = newItems;
			fCapacity = newSize;
		} else
			result = false;
	}
	if (result)
		fItemCount = count;
	return result;
}

#endif	// LIST_H
