/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <Layout.h>

#include <new>

#include <View.h>

#include "ViewLayoutItem.h"

using std::nothrow;


// constructor
BLayout::BLayout()
	: fView(NULL),
	  fItems(20)
{
}

// destructor
BLayout::~BLayout()
{
	// this deletes all items
	SetView(NULL);
}

// View
BView*
BLayout::View() const
{
	return fView;
}

// AddView
BLayoutItem*
BLayout::AddView(BView* child)
{
	return AddView(-1, child);
}

// AddView
BLayoutItem*
BLayout::AddView(int32 index, BView* child)
{
	if (BViewLayoutItem* item = new(nothrow) BViewLayoutItem(child)) {
		if (AddItem(index, item))
			return item;
		delete item;
	}
	return NULL;
}

// AddItem
bool
BLayout::AddItem(BLayoutItem* item)
{
	return AddItem(-1, item);
}

// AddItem
bool
BLayout::AddItem(int32 index, BLayoutItem* item)
{
	if (!fView || !item || fItems.HasItem(item))
		return false;
	
	// if the item refers to a BView, we make sure, it is added to the parent
	// view
	BView* view = item->View();
	if (view && view->fParent != fView && !fView->_AddChild(view, NULL))
		return false;

	// validate the index
	if (index < 0 || index > fItems.CountItems())
		index = fItems.CountItems();

	fItems.AddItem(item, index);
	ItemAdded(item);
	item->SetLayout(this);
	InvalidateLayout();

	return true;
}

// RemoveView
bool
BLayout::RemoveView(BView* child)
{
	int32 index = IndexOfView(child);
	if (index >= 0) {
		if (BLayoutItem* item = RemoveItem(index)) {
			delete item;
			return true;
		}
	}

	return false;
}

// RemoveItem
bool
BLayout::RemoveItem(BLayoutItem* item)
{
	int32 index = IndexOfItem(item);
	return (index >= 0 ? RemoveItem(index) : false);
}

// RemoveItem
BLayoutItem*
BLayout::RemoveItem(int32 index)
{
	if (index < 0 || index >= fItems.CountItems())
		return NULL;

	BLayoutItem* item = (BLayoutItem*)fItems.RemoveItem(index);

	// if the item refers to a BView, we make sure, it is removed from the
	// parent view
	BView* view = item->View();
	if (view && view->fParent == fView)
		view->_RemoveSelf();

	item->SetLayout(NULL);
	ItemRemoved(item);
	InvalidateLayout();

	return item;
}

// ItemAt
BLayoutItem*
BLayout::ItemAt(int32 index) const
{
	return (BLayoutItem*)fItems.ItemAt(index);
}

// CountItems
int32
BLayout::CountItems() const
{
	return fItems.CountItems();
}

// IndexOfItem
int32
BLayout::IndexOfItem(BLayoutItem* item) const
{
	return fItems.IndexOf(item);
}

// IndexOfView
int32
BLayout::IndexOfView(BView* child) const
{
	int itemCount = fItems.CountItems();
	for (int32 i = 0; i < itemCount; i++) {
		BLayoutItem* item = (BLayoutItem*)fItems.ItemAt(i);
		if (dynamic_cast<BViewLayoutItem*>(item) && item->View() == child)
			return i;
	}

	return -1;
}

// InvalidateLayout
void
BLayout::InvalidateLayout()
{
	if (fView)
		fView->InvalidateLayout();
}

// ItemAdded
void
BLayout::ItemAdded(BLayoutItem* item)
{
}

// ItemRemoved
void
BLayout::ItemRemoved(BLayoutItem* item)
{
}

// SetView
void
BLayout::SetView(BView* view)
{
	if (view != fView) {
		fView = NULL;

		// remove and delete all items
		for (int32 i = CountItems() - 1; i >= 0; i--)
			delete RemoveItem(i);

		fView = view;

		InvalidateLayout();
	}
}
