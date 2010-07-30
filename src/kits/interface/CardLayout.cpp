/*
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <CardLayout.h>

#include <LayoutItem.h>
#include <Message.h>
#include <View.h>


namespace {
	const char* kVisibleItemField = "BCardLayout:visibleItem";
}


BCardLayout::BCardLayout()
	:
	BLayout(),
	fMin(0, 0),
	fMax(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED),
	fPreferred(0, 0),
	fVisibleItem(NULL),
	fMinMaxValid(false)
{
}


BCardLayout::BCardLayout(BMessage* from)
	:
	BLayout(BUnarchiver::PrepareArchive(from)),
	fMin(0, 0),
	fMax(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED),
	fPreferred(0, 0),
	fVisibleItem(NULL),
	fMinMaxValid(false)
{
	BUnarchiver(from).Finish();
}


BCardLayout::~BCardLayout()
{
}


BLayoutItem*
BCardLayout::VisibleItem() const
{
	return fVisibleItem;
}


int32
BCardLayout::VisibleIndex() const
{
	return IndexOfItem(fVisibleItem);
}


void
BCardLayout::SetVisibleItem(int32 index)
{
	SetVisibleItem(ItemAt(index));
}


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


BSize
BCardLayout::MinSize()
{
	_ValidateMinMax();
	return fMin;
}


BSize
BCardLayout::MaxSize()
{
	_ValidateMinMax();
	return fMax;
}


BSize
BCardLayout::PreferredSize()
{
	_ValidateMinMax();
	return fPreferred;
}


BAlignment
BCardLayout::Alignment()
{
	return BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT);
}


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


void
BCardLayout::InvalidateLayout()
{
	BLayout::InvalidateLayout();

	fMinMaxValid = false;
}


void
BCardLayout::LayoutView()
{
	_ValidateMinMax();

	BSize size = View()->Bounds().Size();
	size.width = max_c(size.width, fMin.width);
	size.height = max_c(size.height, fMin.height);

	if (fVisibleItem != NULL)
		fVisibleItem->AlignInFrame(BRect(0, 0, size.width, size.height));
}


status_t
BCardLayout::Archive(BMessage* into, bool deep) const
{
	BArchiver archiver(into);
	status_t err = BLayout::Archive(into, deep);

	if (err == B_OK && deep)
		err = into->AddInt32(kVisibleItemField, IndexOfItem(fVisibleItem));

	return archiver.Finish(err);
}


status_t
BCardLayout::AllUnarchived(const BMessage* from)
{
	status_t err = BLayout::AllUnarchived(from);
	if (err != B_OK)
		return err;

	int32 visibleIndex;
	err = from->FindInt32(kVisibleItemField, &visibleIndex);
	if (err == B_OK)
		SetVisibleItem(visibleIndex);

	return err;
}


BArchivable*
BCardLayout::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "BCardLayout"))
		return new BCardLayout(from);
	return NULL;
}
	

bool
BCardLayout::ItemAdded(BLayoutItem* item, int32 atIndex)
{
	item->SetVisible(false);
	return true;
}


void
BCardLayout::ItemRemoved(BLayoutItem* item, int32 fromIndex)
{
	if (fVisibleItem == item) {
		BLayoutItem* newVisibleItem = NULL;
		SetVisibleItem(newVisibleItem);
	}
}


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

	if (BView* view = View())
		view->ResetLayoutInvalidation();
}
