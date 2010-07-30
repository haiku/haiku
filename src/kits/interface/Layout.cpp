/*
 * Copyright 2010, Haiku Inc.
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <Layout.h>

#include <syslog.h>
#include <new>

#include <Message.h>
#include <View.h>

#include "ViewLayoutItem.h"


using std::nothrow;


namespace {
	const char* const kLayoutItemField = "BLayout:items";
}


BLayout::BLayout()
	:
	fView(NULL),
	fItems(20)
{
}


BLayout::BLayout(BMessage* from)
	:
	BArchivable(BUnarchiver::PrepareArchive(from)),
	fView(NULL),
	fItems(20)
{
	BUnarchiver unarchiver(from);

	int32 i = 0;
	while (unarchiver.EnsureUnarchived(kLayoutItemField, i++) == B_OK)
		;
}


BLayout::~BLayout()
{
	// this deletes all items
	SetView(NULL);
}


BView*
BLayout::View() const
{
	return fView;
}


BLayoutItem*
BLayout::AddView(BView* child)
{
	return AddView(-1, child);
}


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


bool
BLayout::AddItem(BLayoutItem* item)
{
	return AddItem(-1, item);
}


bool
BLayout::AddItem(int32 index, BLayoutItem* item)
{
	if (!fView || !item || fItems.HasItem(item))
		return false;

	// if the item refers to a BView, we make sure it is added to the parent
	// view
	bool addedView = false;
	BView* view = item->View();
	if (view && view->fParent != fView
		&& !(addedView = fView->_AddChild(view, NULL)))
		return false;

	// validate the index
	if (index < 0 || index > fItems.CountItems())
		index = fItems.CountItems();

	if (fItems.AddItem(item, index) && ItemAdded(item, index)) {
		item->SetLayout(this);
		InvalidateLayout();
		return true;
	} else {
		// this check is necessary so that if an addition somewhere other
		// than the end of the list fails, we don't remove the wrong item
		if (fItems.ItemAt(index) == item)
			fItems.RemoveItem(index);
		if (addedView)
			view->_RemoveSelf();
		return false;
	}
}


bool
BLayout::RemoveView(BView* child)
{
	bool removed = false;

	// a view can have any number of layout items - we need to remove them all
	for (int32 i = fItems.CountItems(); i-- > 0;) {
		BLayoutItem* item = ItemAt(i);

		if (item->View() != child)
			continue;

		RemoveItem(i);
		removed = true;
		delete item;
	}

	return removed;
}


bool
BLayout::RemoveItem(BLayoutItem* item)
{
	int32 index = IndexOfItem(item);
	return (index >= 0 ? RemoveItem(index) : false);
}


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
	ItemRemoved(item, index);
	InvalidateLayout();

	return item;
}


BLayoutItem*
BLayout::ItemAt(int32 index) const
{
	return (BLayoutItem*)fItems.ItemAt(index);
}


int32
BLayout::CountItems() const
{
	return fItems.CountItems();
}


int32
BLayout::IndexOfItem(const BLayoutItem* item) const
{
	return fItems.IndexOf(item);
}


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


void
BLayout::InvalidateLayout()
{
	if (fView)
		fView->InvalidateLayout();
}


status_t
BLayout::Archive(BMessage* into, bool deep) const
{
	BArchiver archiver(into);
	status_t err = BArchivable::Archive(into, deep);

	if (deep) {
		int32 count = CountItems();
		for (int32 i = 0; i < count && err == B_OK; i++) {
			BLayoutItem* item = ItemAt(i);
			err = archiver.AddArchivable(kLayoutItemField, item, deep);

			if (err == B_OK) {
				err = ItemArchived(into, item, i);
				if (err != B_OK)
					syslog(LOG_ERR, "ItemArchived() failed at index: %d.", i);
			}
		}
	}

	return archiver.Finish(err);
}


status_t
BLayout::AllUnarchived(const BMessage* from)
{
	BUnarchiver unarchiver(from);
	status_t err = BArchivable::AllUnarchived(from);
	if (err != B_OK)
		return err;

	int32 itemCount;
	unarchiver.ArchiveMessage()->GetInfo(kLayoutItemField, NULL, &itemCount);
	for (int32 i = 0; i < itemCount && err == B_OK; i++) {
		BLayoutItem* item;
		err = unarchiver.FindObject(kLayoutItemField,
			i, BUnarchiver::B_DONT_ASSUME_OWNERSHIP, item);
		if (err != B_OK)
			return err;

		if (!fItems.AddItem(item, i) || !ItemAdded(item, i)) {
			fItems.RemoveItem(i);
			return B_ERROR;
		}

		err = ItemUnarchived(from, item, i);
		if (err != B_OK) {
			fItems.RemoveItem(i);
			ItemRemoved(item, i);	
			return err;
		}

		item->SetLayout(this);
		unarchiver.AssumeOwnership(item);
	}

	InvalidateLayout();
	return err;
}


status_t
BLayout::ItemArchived(BMessage* into, BLayoutItem* item, int32 index) const
{
	return B_OK;
}


status_t
BLayout::ItemUnarchived(const BMessage* from, BLayoutItem* item, int32 index)
{
	return B_OK;
}


bool
BLayout::ItemAdded(BLayoutItem* item, int32 atIndex)
{
	return true;
}


void
BLayout::ItemRemoved(BLayoutItem* item, int32 fromIndex)
{
}


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
