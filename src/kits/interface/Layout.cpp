/*
 * Copyright 2010, Haiku Inc.
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <Layout.h>

#include <new>

#include <Message.h>
#include <View.h>

#include "ViewLayoutItem.h"


using std::nothrow;

namespace {
	const char* kLayoutItemField = "BLayout:_items";
	const char* kLayoutDataField = "BLayout:_data";
}


// constructor
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
	ItemRemoved(item);
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

	if (err != B_OK)
		return err;
	
	if (deep) {
		int32 count = CountItems();
		for (int32 i = 0; i < count; i++) {
			err = archiver.AddArchivable(kLayoutItemField, ItemAt(i), deep);

			if (err == B_OK) {
				BMessage data;
				err = ArchiveLayoutData(&data, ItemAt(i));
				if (err == B_OK)
					err = into->AddMessage(kLayoutDataField, &data);
			}

			if (err != B_OK)
				return err;
		}
	}

	return archiver.Finish();
}


status_t
BLayout::AllUnarchived(const BMessage* from)
{
	status_t err = BArchivable::AllUnarchived(from);
	BUnarchiver unarchiver(from);

	if (err != B_OK)
		return err;

	for (int32 i = 0; ; i++) {
		BArchivable* retriever;
		err = unarchiver.FindArchivable(kLayoutItemField, i, &retriever);

		if (err == B_BAD_INDEX)
			break;

		if (err == B_OK) {
			BMessage layoutData;
			err = from->FindMessage(kLayoutDataField, i, &layoutData);

			if (err == B_OK) {
				BLayoutItem* item = dynamic_cast<BLayoutItem*>(retriever);
				err = RestoreItemAndData(&layoutData, item);
			}
		}

		if (err != B_OK)
			return err;
	}

	return B_OK;
}


status_t
BLayout::ArchiveLayoutData(BMessage* into, const BLayoutItem* of) const
{
	return B_OK;
}


status_t
BLayout::RestoreItemAndData(const BMessage* from, BLayoutItem* item)
{
	if (item && AddItem(item))
		return B_OK;
	return B_ERROR;
}


void
BLayout::ItemAdded(BLayoutItem* item)
{
}


void
BLayout::ItemRemoved(BLayoutItem* item)
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
