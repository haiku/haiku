/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		IngoWeinhold <bonefish@cs.tu-berlin.de>
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef LIST_H
#define LIST_H

#include <List.h>

template<class T, bool delete_on_destruction = true>
class List : protected BList {
 public:
	List(int32 count = 10)
		: BList(count) {}

	~List()
		{ MakeEmpty(); }

	// adding items
	inline void AddItem(T value)
		{ BList::AddItem((void*)value); }

	inline void AddItem(T value, int32 index)
		{ BList::AddItem((void*)value, index); }

	// information
	inline bool HasItem(T value) const
		{ return BList::HasItem((void*)value); }

	inline int32 IndexOf(T value) const
		{ return BList::IndexOf((void*)value); }

	inline bool IsEmpty() const
		{ return BList::IsEmpty(); }

	inline int32 CountItems() const
		{ return BList::CountItems(); }

	// retrieving items
	inline T ItemAt(int32 index) const
		{ return (T)BList::ItemAt(index); }

	inline T ItemAtFast(int32 index) const
		{ return (T)BList::ItemAtFast(index); }

	inline T FirstItem() const
		{ return (T)BList::FirstItem(); }

	inline T LastItem() const
		{ return (T)BList::LastItem(); }

	// removing items
	inline bool RemoveItem(T value)
		{ return BList::RemoveItem((void*)value); }

	inline T RemoveItem(int32 index)
		{ return (T)BList::RemoveItem(index); }

	inline bool RemoveItems(int32 index, int32 count)
		{ return BList::RemoveItems(index, count); }

	inline void MakeEmpty() {
		if (delete_on_destruction) {
			// delete all values
			int32 count = CountItems();
			for (int32 i = 0; i < count; i++)
				delete (T)BList::ItemAtFast(i);
		}
		BList::MakeEmpty();
	}
};

#endif	// LIST_H
