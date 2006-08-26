/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <CardLayout.h>

#include <LayoutItem.h>
#include <View.h>

// constructor
BCardLayout::BCardLayout()
	: BLayout(),
	  fMin(0, 0),
	  fMax(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED),
	  fPreferred(0, 0),
	  fVisibleItem(NULL),
	  fMinMaxValid(false)
{
}

// destructor
BCardLayout::~BCardLayout()
{
}

// VisibleItem
BLayoutItem*
BCardLayout::VisibleItem() const
{
	return fVisibleItem;
}

// VisibleIndex
int32
BCardLayout::VisibleIndex() const
{
	return IndexOfItem(fVisibleItem);
}

// SetVisibleItem
void
BCardLayout::SetVisibleItem(int32 index)
{
	SetVisibleItem(ItemAt(index));
}

// SetVisibleItem
void
BCardLayout::SetVisibleItem(BLayoutItem* item)
{
	if (item == fVisibleItem)
		return;

	if (item != NULL && IndexOfItem(item) < 0)
		return;
	
	if (fVisibleItem != NULL)
		fVisibleItem->SetVisible(false);

	fVisibleItem = item;

	if (fVisibleItem != NULL) {
		fVisibleItem->SetVisible(true);

		LayoutView();
	}
}

// MinSize
BSize
BCardLayout::MinSize()
{
	_ValidateMinMax();
	return fMin;
}

// MaxSize
BSize
BCardLayout::MaxSize()
{
	_ValidateMinMax();
	return fMax;
}

// PreferredSize
BSize
BCardLayout::PreferredSize()
{
	_ValidateMinMax();
	return fPreferred;
}

// Alignment
BAlignment
BCardLayout::Alignment()
{
	return BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT);
}

// HasHeightForWidth
bool
BCardLayout::HasHeightForWidth()
{
	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		if (ItemAt(i)->HasHeightForWidth())
			return true;
	}
	
	return false;
}

// GetHeightForWidth
void
BCardLayout::GetHeightForWidth(float width, float* min, float* max,
	float* preferred)
{
	_ValidateMinMax();

	// init with useful values
	float minHeight = fMin.height;
	float maxHeight = fMax.height;
	float preferredHeight = fPreferred.height;

	// apply the items' constraints
	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		BLayoutItem* item = ItemAt(i);
		if (item->HasHeightForWidth()) {
			float itemMinHeight;
			float itemMaxHeight;
			float itemPreferredHeight;
			item->GetHeightForWidth(width, &itemMinHeight, &itemMaxHeight,
				&itemPreferredHeight);
			minHeight = max_c(minHeight, itemMinHeight);
			maxHeight = min_c(maxHeight, itemMaxHeight);
			preferredHeight = min_c(preferredHeight, itemPreferredHeight);
		}
	}

	// adjust max and preferred, if necessary
	maxHeight = max_c(maxHeight, minHeight);
	preferredHeight = max_c(preferredHeight, minHeight);
	preferredHeight = min_c(preferredHeight, maxHeight);

	if (min)
		*min = minHeight;
	if (max)
		*max = maxHeight;
	if (preferred)
		*preferred = preferredHeight;
}

// InvalidateLayout
void
BCardLayout::InvalidateLayout()
{
	BLayout::InvalidateLayout();
	
	fMinMaxValid = false;
}

// LayoutView
void
BCardLayout::LayoutView()
{
	_ValidateMinMax();

	BSize size = BSize(View()->Bounds());
	size.width = max_c(size.width, fMin.width);
	size.height = max_c(size.height, fMin.height);

	if (fVisibleItem != NULL)
		fVisibleItem->AlignInFrame(BRect(0, 0, size.width, size.height));
}

// ItemAdded
void
BCardLayout::ItemAdded(BLayoutItem* item)
{
	item->SetVisible(false);
}

// ItemRemoved
void
BCardLayout::ItemRemoved(BLayoutItem* item)
{
	if (fVisibleItem == item) {
		BLayoutItem* newVisibleItem = NULL;
		SetVisibleItem(newVisibleItem);
	}
}

// _ValidateMinMax
void
BCardLayout::_ValidateMinMax()
{
	if (fMinMaxValid)
		return;

	fMin.width = 0;
	fMin.height = 0;
	fMax.width = B_SIZE_UNLIMITED;
	fMax.height = B_SIZE_UNLIMITED;
	fPreferred.width = 0;
	fPreferred.height = 0;

	int32 itemCount = CountItems();
	for (int32 i = 0; i < itemCount; i++) {
		BLayoutItem* item = ItemAt(i);

		BSize min = item->MinSize();
		BSize max = item->MaxSize();
		BSize preferred = item->PreferredSize();

		fMin.width = max_c(fMin.width, min.width);
		fMin.height = max_c(fMin.height, min.height);

		fMax.width = min_c(fMax.width, max.width);
		fMax.height = min_c(fMax.height, max.height);

		fPreferred.width = max_c(fPreferred.width, preferred.width);
		fPreferred.height = max_c(fPreferred.height, preferred.height);
	}

	fMax.width = max_c(fMax.width, fMin.width);
	fMax.height = max_c(fMax.height, fMin.height);

	fPreferred.width = max_c(fPreferred.width, fMin.width);
	fPreferred.height = max_c(fPreferred.height, fMin.height);
	fPreferred.width = min_c(fPreferred.width, fMax.width);
	fPreferred.height = min_c(fPreferred.height, fMax.height);
	
	fMinMaxValid = true;
}
