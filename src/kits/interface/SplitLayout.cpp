/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <SplitLayout.h>

#include <stdio.h>

#include <LayoutItem.h>
#include <LayoutUtils.h>
#include <View.h>

#include "OneElementLayouter.h"
#include "SimpleLayouter.h"

// ItemLayoutInfo
class BSplitLayout::ItemLayoutInfo {
public:
	float		weight;
	BRect		layoutFrame;
	BSize		min;
	BSize		max;
	bool		isVisible;
	bool		isCollapsible;

	ItemLayoutInfo()
		: weight(1.0f),
		  layoutFrame(0, 0, -1, -1),
		  min(),
		  max(),
		  isVisible(true),
		  isCollapsible(true)
	{
	}
};

// ValueRange
class BSplitLayout::ValueRange {
public:
	int32 sumValue;	// including spacing
	int32 previousMin;
	int32 previousMax;
	int32 previousSize;
	int32 nextMin;
	int32 nextMax;
	int32 nextSize;
};

// SplitterItem
class BSplitLayout::SplitterItem : public BLayoutItem {
public:
	SplitterItem(BSplitLayout* layout)
		: fLayout(layout),
		  fFrame()
	{
	}

	virtual BSize MinSize()
	{
		if (fLayout->Orientation() == B_HORIZONTAL)
			return BSize(fLayout->SplitterSize() - 1, -1);
		else
			return BSize(-1, fLayout->SplitterSize() - 1);
	}

	virtual BSize MaxSize()
	{
		if (fLayout->Orientation() == B_HORIZONTAL)
			return BSize(fLayout->SplitterSize() - 1, B_SIZE_UNLIMITED);
		else
			return BSize(B_SIZE_UNLIMITED, fLayout->SplitterSize() - 1);
	}

	virtual BSize PreferredSize()
	{
		return MinSize();
	}

	virtual BAlignment Alignment()
	{
		return BAlignment(B_ALIGN_HORIZONTAL_CENTER, B_ALIGN_VERTICAL_CENTER);
	}

	virtual void SetExplicitMinSize(BSize size)
	{
		// not allowed
	}

	virtual void SetExplicitMaxSize(BSize size)
	{
		// not allowed
	}

	virtual void SetExplicitPreferredSize(BSize size)
	{
		// not allowed
	}

	virtual void SetExplicitAlignment(BAlignment alignment)
	{
		// not allowed
	}

	virtual bool IsVisible()
	{
		return true;
	}

	virtual void SetVisible(bool visible)
	{
		// not allowed
	}


	virtual BRect Frame()
	{
		return fFrame;
	}

	virtual void SetFrame(BRect frame)
	{
		fFrame = frame;
	}

private:
	BSplitLayout*	fLayout;
	BRect			fFrame;
};


// #pragma mark -


// constructor
BSplitLayout::BSplitLayout(enum orientation orientation,
		float spacing)
	: fOrientation(orientation),
	  fLeftInset(0),
	  fRightInset(0),
	  fTopInset(0),
	  fBottomInset(0),
	  fSplitterSize(3),
	  fSpacing(spacing),

	  fSplitterItems(),
	  fVisibleItems(),
	  fMin(),
	  fMax(),
	  fPreferred(),

	  fHorizontalLayouter(NULL),
	  fVerticalLayouter(NULL),
	  fHorizontalLayoutInfo(NULL),
	  fVerticalLayoutInfo(NULL),
	
	  fHeightForWidthItems(),
	  fHeightForWidthVerticalLayouter(NULL),
	  fHeightForWidthHorizontalLayoutInfo(NULL),

	  fLayoutValid(false),

	  fCachedHeightForWidthWidth(-2),
	  fHeightForWidthVerticalLayouterWidth(-2),
	  fCachedMinHeightForWidth(-1),
	  fCachedMaxHeightForWidth(-1),
	  fCachedPreferredHeightForWidth(-1),
	
	  fDraggingStartPoint(),
	  fDraggingStartValue(0),
	  fDraggingCurrentValue(0),
	  fDraggingSplitterIndex(-1)
{
}

// destructor
BSplitLayout::~BSplitLayout()
{
}

// SetInsets
void
BSplitLayout::SetInsets(float left, float top, float right, float bottom)
{
	fLeftInset = left;
	fTopInset = top;
	fRightInset = right;
	fBottomInset = bottom;

	InvalidateLayout();
}

// GetInsets
void
BSplitLayout::GetInsets(float* left, float* top, float* right, float* bottom)
{
	if (left)
		*left = fLeftInset;
	if (top)
		*top = fTopInset;
	if (right)
		*right = fRightInset;
	if (bottom)
		*bottom = fBottomInset;
}

// Spacing
float
BSplitLayout::Spacing() const
{
	return fSpacing;
}

// SetSpacing
void
BSplitLayout::SetSpacing(float spacing)
{
	if (spacing != fSpacing) {
		fSpacing = spacing;

		InvalidateLayout();
	}
}

// Orientation
orientation
BSplitLayout::Orientation() const
{
	return fOrientation;
}

// SetOrientation
void
BSplitLayout::SetOrientation(enum orientation orientation)
{
	if (orientation != fOrientation) {
		fOrientation = orientation;

		InvalidateLayout();
	}
}

// SplitterSize
float
BSplitLayout::SplitterSize() const
{
	return fSplitterSize;
}

// SetSplitterSize
void
BSplitLayout::SetSplitterSize(float size)
{
	if (size != fSplitterSize) {
		fSplitterSize = size;

		InvalidateLayout();
	}
}

// AddView
BLayoutItem*
BSplitLayout::AddView(BView* child)
{
	return BLayout::AddView(child);
}

// AddView
BLayoutItem*
BSplitLayout::AddView(int32 index, BView* child)
{
	return BLayout::AddView(index, child);
}

// AddView
BLayoutItem*
BSplitLayout::AddView(BView* child, float weight)
{
	return AddView(-1, child, weight);
}

// AddView
BLayoutItem*
BSplitLayout::AddView(int32 index, BView* child, float weight)
{
	BLayoutItem* item = AddView(index, child);
	if (item)
		SetItemWeight(item, weight);

	return item;
}

// AddItem
bool
BSplitLayout::AddItem(BLayoutItem* item)
{
	return BLayout::AddItem(item);
}

// AddItem
bool
BSplitLayout::AddItem(int32 index, BLayoutItem* item)
{
	return BLayout::AddItem(index, item);
}

// AddItem
bool
BSplitLayout::AddItem(BLayoutItem* item, float weight)
{
	return AddItem(-1, item, weight);
}

// AddItem
bool
BSplitLayout::AddItem(int32 index, BLayoutItem* item, float weight)
{
	bool success = AddItem(index, item);
	if (success)
		SetItemWeight(item, weight);

	return success;
}

// ItemWeight
float
BSplitLayout::ItemWeight(int32 index) const
{
	if (index < 0 || index >= CountItems())
		return 0;

	return ItemWeight(ItemAt(index));
}

// ItemWeight
float
BSplitLayout::ItemWeight(BLayoutItem* item) const
{
	if (ItemLayoutInfo* info = _ItemLayoutInfo(item))
		return info->weight;
	return 0;
}

// SetItemWeight
void
BSplitLayout::SetItemWeight(int32 index, float weight, bool invalidateLayout)
{
	if (index < 0 || index >= CountItems())
		return;

	BLayoutItem* item = ItemAt(index);
	SetItemWeight(item, weight);

	if (fHorizontalLayouter) {
		int32 visibleIndex = fVisibleItems.IndexOf(item);
		if (visibleIndex >= 0) {
			if (fOrientation == B_HORIZONTAL)
				fHorizontalLayouter->SetWeight(visibleIndex, weight);
			else
				fVerticalLayouter->SetWeight(visibleIndex, weight);
		}
	}

	if (invalidateLayout)
		InvalidateLayout();
}

// SetItemWeight
void
BSplitLayout::SetItemWeight(BLayoutItem* item, float weight)
{
	if (ItemLayoutInfo* info = _ItemLayoutInfo(item))
		info->weight = weight;
}

// SetCollapsible
void
BSplitLayout::SetCollapsible(bool collapsible)
{
	SetCollapsible(0, CountItems() - 1, collapsible);
}

// SetCollapsible
void
BSplitLayout::SetCollapsible(int32 index, bool collapsible)
{
	SetCollapsible(index, index, collapsible);
}

// SetCollapsible
void
BSplitLayout::SetCollapsible(int32 first, int32 last, bool collapsible)
{
	if (first < 0)
		first = 0;
	if (last < 0 || last > CountItems())
		last = CountItems() - 1;

	for (int32 i = first; i <= last; i++)
		_ItemLayoutInfo(ItemAt(i))->isCollapsible = collapsible;
}

// MinSize
BSize
BSplitLayout::MinSize()
{
	_ValidateMinMax();

	return _AddInsets(fMin);
}

// MaxSize
BSize
BSplitLayout::MaxSize()
{
	_ValidateMinMax();

	return _AddInsets(fMax);
}

// PreferredSize
BSize
BSplitLayout::PreferredSize()
{
	_ValidateMinMax();

	return _AddInsets(fPreferred);
}

// Alignment
BAlignment
BSplitLayout::Alignment()
{
	return BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT);
}

// HasHeightForWidth
bool
BSplitLayout::HasHeightForWidth()
{
	_ValidateMinMax();

	return !fHeightForWidthItems.IsEmpty();
}

// GetHeightForWidth
void
BSplitLayout::GetHeightForWidth(float width, float* min, float* max,
	float* preferred)
{
	if (!HasHeightForWidth())
		return;

	float innerWidth = _SubtractInsets(BSize(width, 0)).width;
	_InternalGetHeightForWidth(innerWidth, false, min, max, preferred);
	_AddInsets(min, max, preferred);
}

// InvalidateLayout
void
BSplitLayout::InvalidateLayout()
{
	_InvalidateLayout(true);
}

// LayoutView
void
BSplitLayout::LayoutView()
{
	_ValidateMinMax();

	// layout the elements
	BSize size = _SubtractInsets(BSize(View()->Bounds()));
	fHorizontalLayouter->Layout(fHorizontalLayoutInfo, size.width);

	Layouter* verticalLayouter;
	if (HasHeightForWidth()) {
		float minHeight, maxHeight, preferredHeight;
		_InternalGetHeightForWidth(size.width, true, &minHeight, &maxHeight,
			&preferredHeight);
		size.height = max_c(size.height, minHeight);
		verticalLayouter = fHeightForWidthVerticalLayouter;
	} else
		verticalLayouter = fVerticalLayouter;
	
	verticalLayouter->Layout(fVerticalLayoutInfo, size.height);

	float xOffset = fLeftInset;
	float yOffset = fTopInset;
	float splitterWidth = 0;	// pixel counts, no distances
	float splitterHeight = 0;	//
	float xSpacing = 0;
	float ySpacing = 0;
	if (fOrientation == B_HORIZONTAL) {
		splitterWidth = fSplitterSize;
		splitterHeight = size.height + 1;
		xSpacing = fSpacing;
	} else {
		splitterWidth = size.width + 1;
		splitterHeight = fSplitterSize;
		ySpacing = fSpacing;
	}

	int itemCount = CountItems();
	for (int i = 0; i < itemCount; i++) {
		// layout the splitter
		if (i > 0) {
			SplitterItem* splitterItem = _SplitterItemAt(i - 1);

			_LayoutItem(splitterItem, BRect(xOffset, yOffset,
				xOffset + splitterWidth - 1, yOffset + splitterHeight - 1),
				true);

			if (fOrientation == B_HORIZONTAL)
				xOffset += splitterWidth + xSpacing;
			else
				yOffset += splitterHeight + ySpacing;
		}

		// layout the item
		BLayoutItem* item = ItemAt(i);
		int32 visibleIndex = fVisibleItems.IndexOf(item);
		if (visibleIndex < 0) {
			_LayoutItem(item, BRect(), false);
			continue;
		}

		// get the dimensions of the item
		float width = fHorizontalLayoutInfo->ElementSize(visibleIndex);
		float height = fVerticalLayoutInfo->ElementSize(visibleIndex);

		// place the component
		_LayoutItem(item, BRect(xOffset, yOffset, xOffset + width,
			yOffset + height), true);

		if (fOrientation == B_HORIZONTAL)
			xOffset += width + xSpacing + 1;
		else
			yOffset += height + ySpacing + 1;
	}

	fLayoutValid = true;
}

// SplitterItemFrame
BRect
BSplitLayout::SplitterItemFrame(int32 index) const
{
	if (SplitterItem* item = _SplitterItemAt(index))
		return item->Frame();
	return BRect();
}

// StartDraggingSplitter
bool
BSplitLayout::StartDraggingSplitter(BPoint point)
{
	StopDraggingSplitter();

	// Layout must be valid. Bail out, if it isn't.
	if (!fLayoutValid)
		return false;

	// Things shouldn't be draggable, if we have a >= max layout.
	BSize size = _SubtractInsets(BSize(View()->Frame()));
	if (fOrientation == B_HORIZONTAL && size.width >= fMax.width
		|| fOrientation == B_VERTICAL && size.height >= fMax.height) {
		return false;
	}

	int32 splitterCount = fSplitterItems.CountItems();
	for (int32 i = 0; i < splitterCount; i++) {
		SplitterItem* splitItem = _SplitterItemAt(i);
		BRect frame = splitItem->Frame();
		if (frame.Contains(point)) {
			fDraggingStartPoint = View()->ConvertToScreen(point);
			fDraggingStartValue = _SplitterValue(i);
			fDraggingCurrentValue = fDraggingStartValue;
			fDraggingSplitterIndex = i;

			return true;
		}
	}

	return false;
}

// DragSplitter
bool
BSplitLayout::DragSplitter(BPoint point)
{
	if (fDraggingSplitterIndex < 0)
		return false;

	point = View()->ConvertToScreen(point);

	int32 valueDiff;
	if (fOrientation == B_HORIZONTAL)
		valueDiff = int32(point.x - fDraggingStartPoint.x);
	else
		valueDiff = int32(point.y - fDraggingStartPoint.y);

	return _SetSplitterValue(fDraggingSplitterIndex,
		fDraggingStartValue + valueDiff);
}

// StopDraggingSplitter
bool
BSplitLayout::StopDraggingSplitter()
{
	if (fDraggingSplitterIndex < 0)
		return false;

	// update the item weights
	_UpdateSplitterWeights();

	fDraggingSplitterIndex = -1;

	return true;
}

// DraggedSplitter
int32
BSplitLayout::DraggedSplitter() const
{
	return fDraggingSplitterIndex;
}

// ItemAdded
void
BSplitLayout::ItemAdded(BLayoutItem* item)
{
	if (CountItems() > 1) {
		SplitterItem* splitterItem = new SplitterItem(this);
		SetItemWeight(splitterItem, 0);
		fSplitterItems.AddItem(splitterItem);
	}

	SetItemWeight(item, 1);
}

// ItemRemoved
void
BSplitLayout::ItemRemoved(BLayoutItem* item)
{
	if (fSplitterItems.CountItems() > 0) {
		SplitterItem* splitterItem = (SplitterItem*)fSplitterItems.RemoveItem(
			fSplitterItems.CountItems() - 1);
		delete (ItemLayoutInfo*)splitterItem->LayoutData();
		delete splitterItem;
	}

	delete (ItemLayoutInfo*)item->LayoutData();
	item->SetLayoutData(NULL);
}

// _InvalidateLayout
void
BSplitLayout::_InvalidateLayout(bool invalidateView)
{
	if (invalidateView)
		BLayout::InvalidateLayout();

	delete fHorizontalLayouter;
	delete fVerticalLayouter;
	delete fHorizontalLayoutInfo;
	delete fVerticalLayoutInfo;

	fHorizontalLayouter = NULL;
	fVerticalLayouter = NULL;
	fHorizontalLayoutInfo = NULL;
	fVerticalLayoutInfo = NULL;

	_InvalidateCachedHeightForWidth();

	fLayoutValid = false;
}

// _InvalidateCachedHeightForWidth
void
BSplitLayout::_InvalidateCachedHeightForWidth()
{
	delete fHeightForWidthVerticalLayouter;
	delete fHeightForWidthHorizontalLayoutInfo;

	fHeightForWidthVerticalLayouter = NULL;
	fHeightForWidthHorizontalLayoutInfo = NULL;

	fCachedHeightForWidthWidth = -2;
	fHeightForWidthVerticalLayouterWidth = -2;
}

// _SplitterItemAt
BSplitLayout::SplitterItem*
BSplitLayout::_SplitterItemAt(int32 index) const
{
	return (SplitterItem*)fSplitterItems.ItemAt(index);
}

// _GetSplitterValueRange
void
BSplitLayout::_GetSplitterValueRange(int32 index, ValueRange& range)
{
	ItemLayoutInfo* previousInfo = _ItemLayoutInfo(ItemAt(index));
	ItemLayoutInfo* nextInfo = _ItemLayoutInfo(ItemAt(index + 1));
	if (fOrientation == B_HORIZONTAL) {
		range.previousMin = (int32)previousInfo->min.width + 1;
		range.previousMax = (int32)previousInfo->max.width + 1;
		range.previousSize = previousInfo->layoutFrame.IntegerWidth() + 1;
		range.nextMin = (int32)nextInfo->min.width + 1;
		range.nextMax = (int32)nextInfo->max.width + 1;
		range.nextSize = nextInfo->layoutFrame.IntegerWidth() + 1;
	} else {
		range.previousMin = (int32)previousInfo->min.height + 1;
		range.previousMax = (int32)previousInfo->max.height + 1;
		range.previousSize = previousInfo->layoutFrame.IntegerHeight() + 1;
		range.nextMin = (int32)nextInfo->min.height + 1;
		range.nextMax = (int32)nextInfo->max.height + 1;
		range.nextSize = (int32)nextInfo->layoutFrame.IntegerHeight() + 1;
	}

	range.sumValue = range.previousSize + range.nextSize;
	if (previousInfo->isVisible)
		range.sumValue += (int32)fSpacing;
	if (nextInfo->isVisible)
		range.sumValue += (int32)fSpacing;
}

// _SplitterValue
int32
BSplitLayout::_SplitterValue(int32 index) const
{
	ItemLayoutInfo* info = _ItemLayoutInfo(ItemAt(index));
	if (info && info->isVisible) {
		if (fOrientation == B_HORIZONTAL)
			return info->layoutFrame.IntegerWidth() + 1 + (int32)fSpacing;
		else
			return info->layoutFrame.IntegerHeight() + 1 + (int32)fSpacing;
	} else
		return 0;
}

// _LayoutItem
void
BSplitLayout::_LayoutItem(BLayoutItem* item, BRect frame, bool visible)
{
	// update the layout frame
	ItemLayoutInfo* info = _ItemLayoutInfo(item);
	info->isVisible = visible;
	if (visible)
		info->layoutFrame = frame;
	else
		info->layoutFrame = BRect(0, 0, -1, -1);

	// update min/max
	info->min = item->MinSize();
	info->max = item->MaxSize();

	if (item->HasHeightForWidth()) {
		BSize size = _SubtractInsets(BSize(View()->Frame()));
		float minHeight, maxHeight;
		item->GetHeightForWidth(size.width, &minHeight, &maxHeight, NULL);
		info->min.height = max_c(info->min.height, minHeight);
		info->max.height = min_c(info->max.height, maxHeight);
	}

	// layout the item
	if (visible)
		item->AlignInFrame(frame);
}

// _LayoutItem
void
BSplitLayout::_LayoutItem(BLayoutItem* item, ItemLayoutInfo* info)
{
	// update the visibility of the item
	bool isVisible = item->IsVisible();
	bool visibilityChanged = (info->isVisible != isVisible);
	if (visibilityChanged)
		item->SetVisible(info->isVisible);
	
	// nothing more to do, if the item is not visible
	if (!info->isVisible)
		return;

	item->AlignInFrame(info->layoutFrame);

	// if the item became visible, we need to update its internal layout
	if (visibilityChanged) {
		if (BView* itemView = item->View())
			itemView->Layout(false);
	}
}

// _SetSplitterValue
bool
BSplitLayout::_SetSplitterValue(int32 index, int32 value)
{
	// if both items are collapsed, nothing can be dragged
	BLayoutItem* previousItem = ItemAt(index);
	BLayoutItem* nextItem = ItemAt(index + 1);
	ItemLayoutInfo* previousInfo = _ItemLayoutInfo(previousItem);
	ItemLayoutInfo* nextInfo = _ItemLayoutInfo(nextItem);
	ItemLayoutInfo* splitterInfo = _ItemLayoutInfo(_SplitterItemAt(index));
	bool previousVisible = previousInfo->isVisible;
	bool nextVisible = nextInfo->isVisible;
	if (!previousVisible && !nextVisible)
		return false;

	ValueRange range;
	_GetSplitterValueRange(index, range);

	value = max_c(min_c(value, range.sumValue), -(int32)fSpacing);

	int32 previousSize = value - (int32)fSpacing;
	int32 nextSize = range.sumValue - value - (int32)fSpacing;

	// Note: While this collapsed-check is mathmatically correct (i.e. we
	// collapse an item, if it would become smaller than half its minimum
	// size), we might want to change it, since for the user it looks like
	// collapsing happens earlier. The reason being that the only visual mark
	// the user has is the mouse cursor which indeed hasn't crossed the middle
	// of the item yet.
	bool previousCollapsed = (previousSize <= range.previousMin / 2)
		&& previousInfo->isCollapsible;
	bool nextCollapsed = (nextSize <= range.nextMin / 2)
		&& nextInfo->isCollapsible;
	if (previousCollapsed && nextCollapsed) {
		// we cannot collapse both items; we have to decide for one
		if (previousSize < nextSize) {
			// collapse previous
			nextCollapsed = false;
			nextSize = range.sumValue - (int32)fSpacing;
		} else {
			// collapse next
			previousCollapsed = false;
			previousSize = range.sumValue - (int32)fSpacing;
		}
	}

	if (previousCollapsed || nextCollapsed) {
		// one collapsed item -- check whether that violates the constraints
		// of the other one
		int32 availableSpace = range.sumValue - (int32)fSpacing;
		if (previousCollapsed) {
			if (availableSpace < range.nextMin
				|| availableSpace > range.nextMax) {
				// we cannot collapse the previous item
				previousCollapsed = false;
			}
		} else {
			if (availableSpace < range.previousMin
				|| availableSpace > range.previousMax) {
				// we cannot collapse the next item
				nextCollapsed = false;
			}
		}
	}

	if (!(previousCollapsed || nextCollapsed)) {
		// no collapsed item -- check whether there is a close solution
		previousSize = value - (int32)fSpacing;
		nextSize = range.sumValue - value - (int32)fSpacing;

		if (range.previousMin + range.nextMin + 2 * fSpacing > range.sumValue) {
			// we don't have enough space to uncollapse both items
			int32 availableSpace = range.sumValue - (int32)fSpacing;
			if (previousSize < nextSize && availableSpace >= range.nextMin
				&& availableSpace <= range.nextMax
				&& previousInfo->isCollapsible) {
				previousCollapsed = true;
			} else if (availableSpace >= range.previousMin
				&& availableSpace <= range.previousMax
				&& nextInfo->isCollapsible) {
				nextCollapsed = true;
			} else if (availableSpace >= range.nextMin
				&& availableSpace <= range.nextMax
				&& previousInfo->isCollapsible) {
				previousCollapsed = true;
			} else {
				if (previousSize < nextSize && previousInfo->isCollapsible) {
					previousCollapsed = true;
				} else if (nextInfo->isCollapsible) {
					nextCollapsed = true;
				} else {
					// Neither item is collapsible although there's not enough
					// space: Give them both their minimum size.
					previousSize = range.previousMin;
					nextSize = range.nextMin;
				}
			}

		} else {
			// there is enough space for both items
			// make sure the min constraints are satisfied
			if (previousSize < range.previousMin) {
				previousSize = range.previousMin;
				nextSize = range.sumValue - previousSize - 2 * (int32)fSpacing;
			} else if (nextSize < range.nextMin) {
				nextSize = range.nextMin;
				previousSize = range.sumValue - nextSize - 2 * (int32)fSpacing;
			}

			// if we can, also satisfy the max constraints
			if (range.previousMax + range.nextMax + 2 * (int32)fSpacing
					>= range.sumValue) {
				if (previousSize > range.previousMax) {
					previousSize = range.previousMax;
					nextSize = range.sumValue - previousSize
						- 2 * (int32)fSpacing;
				} else if (nextSize > range.nextMax) {
					nextSize = range.nextMax;
					previousSize = range.sumValue - nextSize
						- 2 * (int32)fSpacing;
				}
			}
		}
	}

	// compute the size for one collapsed item; for none collapsed item we
	// already have correct values
	if (previousCollapsed || nextCollapsed) {
		int32 availableSpace = range.sumValue - (int32)fSpacing;
		if (previousCollapsed) {
			previousSize = 0;
			nextSize = availableSpace;
		} else {
			previousSize = availableSpace;
			nextSize = 0;
		}
	}

	int32 newValue = previousSize + (previousCollapsed ? 0 : (int32)fSpacing);
	if (newValue == fDraggingCurrentValue) {
		// nothing changed
		return false;
	}

	// something changed: we need to recompute the layout
	int32 baseOffset = -fDraggingCurrentValue;
		// offset to the current splitter position
	int32 splitterOffset = baseOffset + newValue;
	int32 nextOffset = splitterOffset + (int32)fSplitterSize + (int32)fSpacing;

	BRect splitterFrame(splitterInfo->layoutFrame);
	if (fOrientation == B_HORIZONTAL) {
		// horizontal layout
		// previous item
		float left = splitterFrame.left + baseOffset;
		previousInfo->layoutFrame.Set(
			left,
			splitterFrame.top,
			left + previousSize - 1,
			splitterFrame.bottom);

		// next item
		left = splitterFrame.left + nextOffset;
		nextInfo->layoutFrame.Set(
			left,
			splitterFrame.top,
			left + nextSize - 1,
			splitterFrame.bottom);

		// splitter
		splitterInfo->layoutFrame.left += splitterOffset;
		splitterInfo->layoutFrame.right += splitterOffset;
	} else {
		// vertical layout
		// previous item
		float top = splitterFrame.top + baseOffset;
		previousInfo->layoutFrame.Set(
			splitterFrame.left,
			top,
			splitterFrame.right,
			top + previousSize - 1);

		// next item
		top = splitterFrame.top + nextOffset;
		nextInfo->layoutFrame.Set(
			splitterFrame.left,
			top,
			splitterFrame.right,
			top + nextSize - 1);

		// splitter
		splitterInfo->layoutFrame.top += splitterOffset;
		splitterInfo->layoutFrame.bottom += splitterOffset;
	}

	previousInfo->isVisible = !previousCollapsed;
	nextInfo->isVisible = !nextCollapsed;

	bool heightForWidth = (fOrientation == B_HORIZONTAL && HasHeightForWidth());

	// If the item visibility is to be changed, we need to update the splitter
	// values now, since the visibility change will cause an invalidation.
	if (previousVisible != previousInfo->isVisible
		|| nextVisible != nextInfo->isVisible || heightForWidth) {
		_UpdateSplitterWeights();
	}

	// If we have height for width items, we need to invalidate the previous
	// and the next item. Actually we would only need to invalidate height for
	// width items, but since non height for width items might be aligned with
	// height for width items, we need to trigger a layout that creates a
	// context that spans all aligned items.
	// We invalidate already here, so that changing the items' size won't cause
	// an immediate relayout.
	if (heightForWidth) {
		previousItem->InvalidateLayout();
		nextItem->InvalidateLayout();
	}

	// do the layout
	_LayoutItem(previousItem, previousInfo);
	_LayoutItem(_SplitterItemAt(index), splitterInfo);
	_LayoutItem(nextItem, nextInfo);

	fDraggingCurrentValue = newValue;

	return true;
}

// _ItemLayoutInfo
BSplitLayout::ItemLayoutInfo*
BSplitLayout::_ItemLayoutInfo(BLayoutItem* item) const
{
	ItemLayoutInfo* info = (ItemLayoutInfo*)item->LayoutData();
	if (!info) {
		info = new ItemLayoutInfo();
		item->SetLayoutData(info);
	}

	return info;
}

// _UpdateSplitterWeights
void
BSplitLayout::_UpdateSplitterWeights()
{
	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		float weight;
		if (fOrientation == B_HORIZONTAL)
			weight = _ItemLayoutInfo(ItemAt(i))->layoutFrame.Width() + 1;
		else
			weight = _ItemLayoutInfo(ItemAt(i))->layoutFrame.Height() + 1;

		SetItemWeight(i, weight, false);
	}

	// Just updating the splitter weights is fine in principle. The next
	// LayoutView() will use the correct values. But, if our orientation is
	// vertical, the cached height for width info needs to be flushed, or the
	// obsolete cached values will be used.
	if (fOrientation == B_VERTICAL)
		_InvalidateCachedHeightForWidth();
}

// _ValidateMinMax
void
BSplitLayout::_ValidateMinMax()
{
	if (fHorizontalLayouter != NULL)
		return;

	fLayoutValid = false;

	fVisibleItems.MakeEmpty();
	fHeightForWidthItems.MakeEmpty();

	_InvalidateCachedHeightForWidth();
	
	// filter the visible items
	int32 itemCount = CountItems();
	for (int32 i = 0; i < itemCount; i++) {
		BLayoutItem* item = ItemAt(i);
		if (item->IsVisible())
			fVisibleItems.AddItem(item);

		// Add "height for width" items even, if they aren't visible. Otherwise
		// we may get our parent into trouble, since we could change from
		// "height for width" to "not height for width".
		if (item->HasHeightForWidth())
			fHeightForWidthItems.AddItem(item);
	}
	itemCount = fVisibleItems.CountItems();		

	// create the layouters
	Layouter* itemLayouter = new SimpleLayouter(itemCount, 0);

	if (fOrientation == B_HORIZONTAL) {
		fHorizontalLayouter = itemLayouter;
		fVerticalLayouter = new OneElementLayouter();
	} else {
		fHorizontalLayouter = new OneElementLayouter();
		fVerticalLayouter = itemLayouter;
	}

	// tell the layouters about our constraints
	if (itemCount > 0) {
		for (int32 i = 0; i < itemCount; i++) {
			BLayoutItem* item = (BLayoutItem*)fVisibleItems.ItemAt(i);
			BSize min = item->MinSize();
			BSize max = item->MaxSize();
			BSize preferred = item->PreferredSize();

			fHorizontalLayouter->AddConstraints(i, 1, min.width, max.width,
				preferred.width);
			fVerticalLayouter->AddConstraints(i, 1, min.height, max.height,
				preferred.height);

			float weight = ItemWeight(item);
			fHorizontalLayouter->SetWeight(i, weight);
			fVerticalLayouter->SetWeight(i, weight);
		}
	}

	fMin.width = fHorizontalLayouter->MinSize();
	fMin.height = fVerticalLayouter->MinSize();
	fMax.width = fHorizontalLayouter->MaxSize();
	fMax.height = fVerticalLayouter->MaxSize();
	fPreferred.width = fHorizontalLayouter->PreferredSize();
	fPreferred.height = fVerticalLayouter->PreferredSize();

	fHorizontalLayoutInfo = fHorizontalLayouter->CreateLayoutInfo();
	if (fHeightForWidthItems.IsEmpty())
		fVerticalLayoutInfo = fVerticalLayouter->CreateLayoutInfo();
}

// _InternalGetHeightForWidth
void
BSplitLayout::_InternalGetHeightForWidth(float width, bool realLayout,
	float* minHeight, float* maxHeight, float* preferredHeight)
{
	if (realLayout && fHeightForWidthVerticalLayouterWidth != width
		|| !realLayout && fCachedHeightForWidthWidth != width) {
		// The general strategy is to clone the vertical layouter, which only
		// knows the general min/max constraints, do a horizontal layout for the
		// given width, and add the children's height for width constraints to
		// the cloned vertical layouter. If this method is invoked internally,
		// we keep the cloned vertical layouter, for it will be used for doing
		// the layout. Otherwise we just drop it after we've got the height for
		// width info.

		// clone the vertical layouter and get the horizontal layout info to be used
		LayoutInfo* horizontalLayoutInfo = NULL;
		Layouter* verticalLayouter = fVerticalLayouter->CloneLayouter();
		if (realLayout) {
			horizontalLayoutInfo = fHorizontalLayoutInfo;
			delete fHeightForWidthVerticalLayouter;
			fHeightForWidthVerticalLayouter = verticalLayouter;
			delete fVerticalLayoutInfo;
			fVerticalLayoutInfo = verticalLayouter->CreateLayoutInfo();
			fHeightForWidthVerticalLayouterWidth = width;
		} else {
			if (fHeightForWidthHorizontalLayoutInfo == NULL) {
				delete fHeightForWidthHorizontalLayoutInfo;
				fHeightForWidthHorizontalLayoutInfo
					= fHorizontalLayouter->CreateLayoutInfo();
			}
			horizontalLayoutInfo = fHeightForWidthHorizontalLayoutInfo;
		}
		
		// do the horizontal layout (already done when doing this for the real
		// layout)
		if (!realLayout)
			fHorizontalLayouter->Layout(horizontalLayoutInfo, width);

		// add the children's height for width constraints
		int32 count = fHeightForWidthItems.CountItems();
		for (int32 i = 0; i < count; i++) {
			BLayoutItem* item = (BLayoutItem*)fHeightForWidthItems.ItemAt(i);
			int32 index = fVisibleItems.IndexOf(item);
			if (index >= 0) {
				float itemMinHeight, itemMaxHeight, itemPreferredHeight;
				item->GetHeightForWidth(
					horizontalLayoutInfo->ElementSize(index),
					&itemMinHeight, &itemMaxHeight, &itemPreferredHeight);
				verticalLayouter->AddConstraints(index, 1, itemMinHeight,
					itemMaxHeight, itemPreferredHeight);
			}
		}

		// get the height for width info
		fCachedHeightForWidthWidth = width;
		fCachedMinHeightForWidth = verticalLayouter->MinSize();
		fCachedMaxHeightForWidth = verticalLayouter->MaxSize();
		fCachedPreferredHeightForWidth = verticalLayouter->PreferredSize();
	}

	if (minHeight)
		*minHeight = fCachedMinHeightForWidth;
	if (maxHeight)
		*maxHeight = fCachedMaxHeightForWidth;
	if (preferredHeight)
		*preferredHeight = fCachedPreferredHeightForWidth;
}

// _SplitterSpace
float
BSplitLayout::_SplitterSpace() const
{
	int32 splitters = fSplitterItems.CountItems();
	float space = 0;
	if (splitters > 0) {
		space = (fVisibleItems.CountItems() + splitters - 1) * fSpacing
			+ splitters * fSplitterSize;
	}

	return space;
}

// _AddInsets
BSize
BSplitLayout::_AddInsets(BSize size)
{
	size.width = BLayoutUtils::AddDistances(size.width,
		fLeftInset + fRightInset - 1);
	size.height = BLayoutUtils::AddDistances(size.height,
		fTopInset + fBottomInset - 1);

	float spacing = _SplitterSpace();
	if (fOrientation == B_HORIZONTAL)
		size.width = BLayoutUtils::AddDistances(size.width, spacing - 1);
	else 
		size.height = BLayoutUtils::AddDistances(size.height, spacing - 1);

	return size;
}

// _AddInsets
void
BSplitLayout::_AddInsets(float* minHeight, float* maxHeight,
	float* preferredHeight)
{
	float insets = fTopInset + fBottomInset - 1;
	if (fOrientation == B_VERTICAL)
		insets += _SplitterSpace();
	if (minHeight)
		*minHeight = BLayoutUtils::AddDistances(*minHeight, insets);
	if (maxHeight)
		*maxHeight = BLayoutUtils::AddDistances(*maxHeight, insets);
	if (preferredHeight)
		*preferredHeight = BLayoutUtils::AddDistances(*preferredHeight, insets);
}

// _SubtractInsets
BSize
BSplitLayout::_SubtractInsets(BSize size)
{
	size.width = BLayoutUtils::SubtractDistances(size.width,
		fLeftInset + fRightInset - 1);
	size.height = BLayoutUtils::SubtractDistances(size.height,
		fTopInset + fBottomInset - 1);

	float spacing = _SplitterSpace();
	if (fOrientation == B_HORIZONTAL)
		size.width = BLayoutUtils::SubtractDistances(size.width, spacing - 1);
	else 
		size.height = BLayoutUtils::SubtractDistances(size.height, spacing - 1);

	return size;
}
