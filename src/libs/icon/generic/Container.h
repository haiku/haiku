/*
 * Copyright 2006-2007, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */
#ifndef CONTAINER_H
#define CONTAINER_H


#include <stdio.h>
#include <string.h>

#include <List.h>
#include <OS.h>
#include <Referenceable.h>

#include "IconBuild.h"

class BReferenceable;


_BEGIN_ICON_NAMESPACE


template<class Type>
class ContainerListener {
 public:
								ContainerListener();
	virtual						~ContainerListener();

	virtual	void				ItemAdded(Type* item, int32 index) = 0;
	virtual	void				ItemRemoved(Type* item) = 0;
};


/*!
	Wraps \c BList and provides some additional features.

	-# It allows listeners to listen when an item is added or removed.
	-# It can take ownership of the objects that it stores.
	-# It casts list items into the correct type.
*/
// TODO: some of these features are provided by BObjectList
template<class Type>
class Container {
 public:
								Container(bool ownsItems);
	virtual						~Container();

			bool				AddItem(Type* item);
			bool				AddItem(Type* item, int32 index);
			bool				RemoveItem(Type* item);
			Type*				RemoveItem(int32 index);

			void				MakeEmpty();

			int32				CountItems() const;
			bool				HasItem(Type* item) const;
			int32				IndexOf(Type* item) const;

			Type*				ItemAt(int32 index) const;
			Type*				ItemAtFast(int32 index) const;

			bool				AddListener(ContainerListener<Type>* listener);
			bool				RemoveListener(ContainerListener<Type>* listener);

 private:
			void				_NotifyItemAdded(Type* item, int32 index) const;
			void				_NotifyItemRemoved(Type* item) const;

 private:
			BList				fItems;
			bool				fOwnsItems;

			BList				fListeners;
};


template<class Type>
ContainerListener<Type>::ContainerListener() {}


template<class Type>
ContainerListener<Type>::~ContainerListener() {}


template<class Type>
Container<Type>::Container(bool ownsItems)
	: fItems(16),
	  fOwnsItems(ownsItems),
	  fListeners(2)
{
}


template<class Type>
Container<Type>::~Container()
{
	int32 count = fListeners.CountItems();
	if (count > 0) {
		debugger("~Container() - there are still"
				 "listeners attached\n");
	}
	MakeEmpty();
}


// #pragma mark -


template<class Type>
bool
Container<Type>::AddItem(Type* item)
{
	return AddItem(item, CountItems());
}


template<class Type>
bool
Container<Type>::AddItem(Type* item, int32 index)
{
	if (!item)
		return false;

	// prevent adding the same item twice
	if (HasItem(item))
		return false;

	if (fItems.AddItem((void*)item, index)) {
		_NotifyItemAdded(item, index);
		return true;
	}

	fprintf(stderr, "Container::AddItem() - out of memory!\n");
	return false;
}


template<class Type>
bool
Container<Type>::RemoveItem(Type* item)
{
	if (fItems.RemoveItem((void*)item)) {
		_NotifyItemRemoved(item);
		return true;
	}

	return false;
}


template<class Type>
Type*
Container<Type>::RemoveItem(int32 index)
{
	Type* item = (Type*)fItems.RemoveItem(index);
	if (item) {
		_NotifyItemRemoved(item);
	}

	return item;
}


template<class Type>
void
Container<Type>::MakeEmpty()
{
	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		Type* item = ItemAtFast(i);
		_NotifyItemRemoved(item);
#ifdef ICON_O_MATIC
		if (fOwnsItems)
			item->ReleaseReference();
#else
		if (fOwnsItems)
			delete item;
#endif
	}
	fItems.MakeEmpty();
}


// #pragma mark -


template<class Type>
int32
Container<Type>::CountItems() const
{
	return fItems.CountItems();
}


template<class Type>
bool
Container<Type>::HasItem(Type* item) const
{
	return fItems.HasItem(item);
}


template<class Type>
int32
Container<Type>::IndexOf(Type* item) const
{
	return fItems.IndexOf((void*)item);
}


template<class Type>
Type*
Container<Type>::ItemAt(int32 index) const
{
	return (Type*)fItems.ItemAt(index);
}


template<class Type>
Type*
Container<Type>::ItemAtFast(int32 index) const
{
	return (Type*)fItems.ItemAtFast(index);
}


// #pragma mark -


template<class Type>
bool
Container<Type>::AddListener(ContainerListener<Type>* listener)
{
	if (listener && !fListeners.HasItem(listener))
		return fListeners.AddItem(listener);
	return false;
}


template<class Type>
bool
Container<Type>::RemoveListener(ContainerListener<Type>* listener)
{
	return fListeners.RemoveItem(listener);
}


// #pragma mark -


template<class Type>
void
Container<Type>::_NotifyItemAdded(Type* item, int32 index) const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		ContainerListener<Type>* listener
			= (ContainerListener<Type>*)listeners.ItemAtFast(i);
		listener->ItemAdded(item, index);
	}
}


template<class Type>
void
Container<Type>::_NotifyItemRemoved(Type* item) const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		ContainerListener<Type>* listener
			= (ContainerListener<Type>*)listeners.ItemAtFast(i);
		listener->ItemRemoved(item);
	}
}


_END_ICON_NAMESPACE


#endif	// CONTAINER_H
