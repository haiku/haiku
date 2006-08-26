/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <GroupLayout.h>

#include <LayoutItem.h>


// ItemLayoutData
struct BGroupLayout::ItemLayoutData {
	float	weight;

	ItemLayoutData()
		: weight(1)
	{
	}
};

// constructor
BGroupLayout::BGroupLayout(enum orientation orientation, float spacing)
	: BTwoDimensionalLayout(),
	  fOrientation(orientation)
{
	SetSpacing(spacing);
}

// destructor
BGroupLayout::~BGroupLayout()
{
}

// Spacing
float
BGroupLayout::Spacing() const
{
	return fHSpacing;
}

// SetSpacing
void
BGroupLayout::SetSpacing(float spacing)
{
	if (spacing != fHSpacing) {
		fHSpacing = spacing;
		fVSpacing = spacing;
		InvalidateLayout();
	}
}

// Orientation
orientation
BGroupLayout::Orientation() const
{
	return fOrientation;
}

// SetOrientation
void
BGroupLayout::SetOrientation(enum orientation orientation)
{
	if (orientation != fOrientation) {
		fOrientation = orientation;

		InvalidateLayout();
	}
}

// ItemWeight
float
BGroupLayout::ItemWeight(int32 index) const
{
	if (index < 0 || index >= CountItems())
		return 0;

	ItemLayoutData* data = _LayoutDataForItem(ItemAt(index));
	return (data ? data->weight : 0);
}

// SetItemWeight
void
BGroupLayout::SetItemWeight(int32 index, float weight)
{
	if (index < 0 || index >= CountItems())
		return;

	if (ItemLayoutData* data = _LayoutDataForItem(ItemAt(index)))
		data->weight = weight;

	InvalidateLayout();
}

// AddView	
BLayoutItem*
BGroupLayout::AddView(BView* child)
{
	return BTwoDimensionalLayout::AddView(child);
}

// AddView	
BLayoutItem*
BGroupLayout::AddView(int32 index, BView* child)
{
	return BTwoDimensionalLayout::AddView(index, child);
}

// AddView	
BLayoutItem*
BGroupLayout::AddView(BView* child, float weight)
{
	return AddView(-1, child, weight);
}

// AddView
BLayoutItem*
BGroupLayout::AddView(int32 index, BView* child, float weight)
{
	BLayoutItem* item = AddView(index, child);
	if (ItemLayoutData* data = _LayoutDataForItem(item))
		data->weight = weight;

	return item;
}

// AddItem
bool
BGroupLayout::AddItem(BLayoutItem* item)
{
	return BTwoDimensionalLayout::AddItem(item);
}

// AddItem
bool
BGroupLayout::AddItem(int32 index, BLayoutItem* item)
{
	return BTwoDimensionalLayout::AddItem(index, item);
}

// AddItem
bool
BGroupLayout::AddItem(BLayoutItem* item, float weight)
{
	return AddItem(-1, item, weight);
}

// AddItem
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

// ItemAdded
void
BGroupLayout::ItemAdded(BLayoutItem* item)
{
	item->SetLayoutData(new ItemLayoutData);
}

// ItemRemoved
void
BGroupLayout::ItemRemoved(BLayoutItem* item)
{
	if (ItemLayoutData* data = _LayoutDataForItem(item)) {
		item->SetLayoutData(NULL);
		delete data;
	}
}

// PrepareItems
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

// InternalCountColumns	
int32
BGroupLayout::InternalCountColumns()
{
	return (fOrientation == B_HORIZONTAL ? fVisibleItems.CountItems() : 1);
}

// InternalCountRows
int32
BGroupLayout::InternalCountRows()
{
	return (fOrientation == B_VERTICAL ? fVisibleItems.CountItems() : 1);
}

// GetColumnRowConstraints
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

// ItemDimensions
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

// _LayoutDataForItem
BGroupLayout::ItemLayoutData*
BGroupLayout::_LayoutDataForItem(BLayoutItem* item) const
{
	if (!item)
		return NULL;
	return (ItemLayoutData*)item->LayoutData();
}
