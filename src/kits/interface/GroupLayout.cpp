/*
 * Copyright 2010, Haiku, Inc.
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <GroupLayout.h>

#include <ControlLook.h>
#include <LayoutItem.h>
#include <Message.h>

#include <new>


using std::nothrow;


namespace {
	const char* const kItemWeightField = "BGroupLayout:item:weight";
	const char* const kVerticalField = "BGroupLayout:vertical";
}


struct BGroupLayout::ItemLayoutData {
	float	weight;

	ItemLayoutData()
		: weight(1)
	{
	}
};


BGroupLayout::BGroupLayout(enum orientation orientation, float spacing)
	:
	BTwoDimensionalLayout(),
	fOrientation(orientation)
{
	SetSpacing(spacing);
}


BGroupLayout::BGroupLayout(BMessage* from)
	:
	BTwoDimensionalLayout(from)
{
	bool isVertical;
	if (from->FindBool(kVerticalField, &isVertical) != B_OK)
		isVertical = false;
	fOrientation = isVertical ? B_VERTICAL : B_HORIZONTAL;
}


BGroupLayout::~BGroupLayout()
{
}


float
BGroupLayout::Spacing() const
{
	return fHSpacing;
}


void
BGroupLayout::SetSpacing(float spacing)
{
	spacing = BControlLook::ComposeSpacing(spacing);
	if (spacing != fHSpacing) {
		fHSpacing = spacing;
		fVSpacing = spacing;
		InvalidateLayout();
	}
}


orientation
BGroupLayout::Orientation() const
{
	return fOrientation;
}


void
BGroupLayout::SetOrientation(enum orientation orientation)
{
	if (orientation != fOrientation) {
		fOrientation = orientation;

		InvalidateLayout();
	}
}


float
BGroupLayout::ItemWeight(int32 index) const
{
	if (index < 0 || index >= CountItems())
		return 0;

	ItemLayoutData* data = _LayoutDataForItem(ItemAt(index));
	return (data ? data->weight : 0);
}


void
BGroupLayout::SetItemWeight(int32 index, float weight)
{
	if (index < 0 || index >= CountItems())
		return;

	if (ItemLayoutData* data = _LayoutDataForItem(ItemAt(index)))
		data->weight = weight;

	InvalidateLayout();
}


BLayoutItem*
BGroupLayout::AddView(BView* child)
{
	return BTwoDimensionalLayout::AddView(child);
}


BLayoutItem*
BGroupLayout::AddView(int32 index, BView* child)
{
	return BTwoDimensionalLayout::AddView(index, child);
}


BLayoutItem*
BGroupLayout::AddView(BView* child, float weight)
{
	return AddView(-1, child, weight);
}


BLayoutItem*
BGroupLayout::AddView(int32 index, BView* child, float weight)
{
	BLayoutItem* item = AddView(index, child);
	if (ItemLayoutData* data = _LayoutDataForItem(item))
		data->weight = weight;

	return item;
}


bool
BGroupLayout::AddItem(BLayoutItem* item)
{
	return BTwoDimensionalLayout::AddItem(item);
}


bool
BGroupLayout::AddItem(int32 index, BLayoutItem* item)
{
	return BTwoDimensionalLayout::AddItem(index, item);
}


bool
BGroupLayout::AddItem(BLayoutItem* item, float weight)
{
	return AddItem(-1, item, weight);
}


bool
BGroupLayout::AddItem(int32 index, BLayoutItem* item, float weight)
{
	bool success = AddItem(index, item);
	if (success) {
		if (ItemLayoutData* data = _LayoutDataForItem(item))
			data->weight = weight;
	}

	return success;
}


status_t
BGroupLayout::Archive(BMessage* into, bool deep) const
{
	BArchiver archiver(into);
	status_t err = BTwoDimensionalLayout::Archive(into, deep);

	if (err == B_OK)
		err = into->AddBool(kVerticalField, fOrientation == B_VERTICAL);

	return archiver.Finish(err);
}


status_t
BGroupLayout::AllArchived(BMessage* into) const
{
	return BTwoDimensionalLayout::AllArchived(into);
}


status_t
BGroupLayout::AllUnarchived(const BMessage* from)
{
	return BTwoDimensionalLayout::AllUnarchived(from);
}


BArchivable*
BGroupLayout::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "BGroupLayout"))
		return new(nothrow) BGroupLayout(from);
	return NULL;
}


status_t
BGroupLayout::ItemArchived(BMessage* into,
	BLayoutItem* item, int32 index) const
{
	return into->AddFloat(kItemWeightField, _LayoutDataForItem(item)->weight);
}


status_t
BGroupLayout::ItemUnarchived(const BMessage* from,
	BLayoutItem* item, int32 index)
{
	float weight;
	status_t err = from->FindFloat(kItemWeightField, index, &weight);

	if (err == B_OK)
		_LayoutDataForItem(item)->weight = weight;

	return err;
}


bool
BGroupLayout::ItemAdded(BLayoutItem* item, int32 atIndex)
{
	item->SetLayoutData(new(nothrow) ItemLayoutData);
	return item->LayoutData() != NULL;
}


void
BGroupLayout::ItemRemoved(BLayoutItem* item, int32 fromIndex)
{
	if (ItemLayoutData* data = _LayoutDataForItem(item)) {
		item->SetLayoutData(NULL);
		delete data;
	}
}


void
BGroupLayout::PrepareItems(enum orientation orientation)
{
	// filter the visible items
	fVisibleItems.MakeEmpty();
	int32 itemCount = CountItems();
	for (int i = 0; i < itemCount; i++) {
		BLayoutItem* item = ItemAt(i);
		if (item->IsVisible())
			fVisibleItems.AddItem(item);
	}
}


int32
BGroupLayout::InternalCountColumns()
{
	return (fOrientation == B_HORIZONTAL ? fVisibleItems.CountItems() : 1);
}


int32
BGroupLayout::InternalCountRows()
{
	return (fOrientation == B_VERTICAL ? fVisibleItems.CountItems() : 1);
}


void
BGroupLayout::GetColumnRowConstraints(enum orientation orientation, int32 index,
	ColumnRowConstraints* constraints)
{
	if (index >= 0 && index < fVisibleItems.CountItems()) {
		BLayoutItem* item = (BLayoutItem*)fVisibleItems.ItemAt(index);
		constraints->min = -1;
		constraints->max = B_SIZE_UNLIMITED;
		if (ItemLayoutData* data = _LayoutDataForItem(item))
			constraints->weight = data->weight;
		else
			constraints->weight = 1;
	}
}


void
BGroupLayout::GetItemDimensions(BLayoutItem* item, Dimensions* dimensions)
{
	int32 index = fVisibleItems.IndexOf(item);
	if (index < 0)
		return;

	if (fOrientation == B_HORIZONTAL) {
		dimensions->x = index;
		dimensions->y = 0;
		dimensions->width = 1;
		dimensions->height = 1;
	} else {
		dimensions->x = 0;
		dimensions->y = index;
		dimensions->width = 1;
		dimensions->height = 1;
	}
}


BGroupLayout::ItemLayoutData*
BGroupLayout::_LayoutDataForItem(BLayoutItem* item) const
{
	if (!item)
		return NULL;
	return (ItemLayoutData*)item->LayoutData();
}


status_t
BGroupLayout::Perform(perform_code code, void* _data)
{
	return BTwoDimensionalLayout::Perform(code, _data);
}


void BGroupLayout::_ReservedGroupLayout1() {}
void BGroupLayout::_ReservedGroupLayout2() {}
void BGroupLayout::_ReservedGroupLayout3() {}
void BGroupLayout::_ReservedGroupLayout4() {}
void BGroupLayout::_ReservedGroupLayout5() {}
void BGroupLayout::_ReservedGroupLayout6() {}
void BGroupLayout::_ReservedGroupLayout7() {}
void BGroupLayout::_ReservedGroupLayout8() {}
void BGroupLayout::_ReservedGroupLayout9() {}
void BGroupLayout::_ReservedGroupLayout10() {}

