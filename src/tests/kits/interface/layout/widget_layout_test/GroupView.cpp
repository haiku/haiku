/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "GroupView.h"

#include <stdio.h>

#include <LayoutUtils.h>


// #pragma mark - GroupView


struct GroupView::LayoutInfo {
	int32	min;
	int32	max;
	int32	preferred;
	int32	size;

	LayoutInfo()
		: min(0),
		  max(B_SIZE_UNLIMITED),
		  preferred(0)
	{
	}

	void AddConstraints(float addMin, float addMax, float addPreferred)
	{
		if (addMin >= min)
			min = (int32)addMin + 1;
		if (addMax <= max)
			max = (int32)addMax + 1;
		if (addPreferred >= preferred)
			preferred = (int32)addPreferred + 1;
	}

	void Normalize()
	{
		if (max < min)
			max = min;
		if (preferred < min)
			preferred = min;
		if (preferred > max)
			preferred = max;
	}
};


GroupView::GroupView(enum orientation orientation, int32 lineCount)
	: View(BRect(0, 0, 0, 0)),
	  fOrientation(orientation),
	  fLineCount(lineCount),
	  fColumnSpacing(0),
	  fRowSpacing(0),
	  fInsets(0, 0, 0, 0),
	  fMinMaxValid(false),
	  fColumnInfos(NULL),
	  fRowInfos(NULL)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	if (fLineCount < 1)
		fLineCount = 1;
}


GroupView::~GroupView()
{
	delete fColumnInfos;
	delete fRowInfos;
}


void
GroupView::SetSpacing(float horizontal, float vertical)
{
	if (horizontal != fColumnSpacing || vertical != fRowSpacing) {
		fColumnSpacing = horizontal;
		fRowSpacing = vertical;

		InvalidateLayout();
	}
}


void
GroupView::SetInsets(float left, float top, float right, float bottom)
{
	BRect newInsets(left, top, right, bottom);
	if (newInsets != fInsets) {
		fInsets = newInsets;
		InvalidateLayout();
	}
}


BSize
GroupView::MinSize()
{
	_ValidateMinMax();
	return _AddInsetsAndSpacing(BSize(fMinWidth - 1, fMinHeight - 1));
}


BSize
GroupView::MaxSize()
{
	_ValidateMinMax();
	return _AddInsetsAndSpacing(BSize(fMaxWidth - 1, fMaxHeight - 1));
}


BSize
GroupView::PreferredSize()
{
	_ValidateMinMax();
	return _AddInsetsAndSpacing(BSize(fPreferredWidth - 1,
		fPreferredHeight - 1));
}


BAlignment
GroupView::Alignment()
{
	return BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT);
}


void
GroupView::InvalidateLayout()
{
	fMinMaxValid = false;
	View::InvalidateLayout();
}


void
GroupView::Layout()
{
//printf("%p->GroupView::Layout()\n", this);
	_ValidateMinMax();
		// actually a little late already

	BSize size = _SubtractInsetsAndSpacing(Size());
	_LayoutLine(size.IntegerWidth() + 1, fColumnInfos, fColumnCount);
	_LayoutLine(size.IntegerHeight() + 1, fRowInfos, fRowCount);

	// layout children
	BPoint location = fInsets.LeftTop();
	for (int32 column = 0; column < fColumnCount; column++) {
		LayoutInfo& columnInfo = fColumnInfos[column];
		location.y = fInsets.top;
		for (int32 row = 0; row < fRowCount; row++) {
			View* child = _ChildAt(column, row);
			if (!child)
				continue;

			// get the grid cell frame
			BRect cellFrame(location,
				BSize(columnInfo.size - 1, fRowInfos[row].size - 1));

			// align the child frame in the grid cell
			BRect childFrame = BLayoutUtils::AlignInFrame(cellFrame,
				child->MaxSize(), child->Alignment());

			// layout child
			child->SetFrame(childFrame);

			location.y += fRowInfos[row].size + fRowSpacing;
		}

		location.x += columnInfo.size + fColumnSpacing;
	}
//printf("%p->GroupView::Layout() done\n", this);
}


void
GroupView::_ValidateMinMax()
{
	if (fMinMaxValid)
		return;

//printf("%p->GroupView::_ValidateMinMax()\n", this);
	delete fColumnInfos;
	delete fRowInfos;

	fColumnCount = _ColumnCount();
	fRowCount = _RowCount();

	fColumnInfos = new LayoutInfo[fColumnCount];
	fRowInfos = new LayoutInfo[fRowCount];

	// collect the children's min/max constraints
	for (int32 column = 0; column < fColumnCount; column++) {
		for (int32 row = 0; row < fRowCount; row++) {
			View* child = _ChildAt(column, row);
			if (!child)
				continue;

			BSize min = child->MinSize();
			BSize max = child->MaxSize();
			BSize preferred = child->PreferredSize();

			// apply constraints to column/row info
			fColumnInfos[column].AddConstraints(min.width, max.width,
				preferred.width);
			fRowInfos[row].AddConstraints(min.height, max.height,
				preferred.height);
		}
	}

	// normalize the column/row constraints and compute sum min/max
	fMinWidth = 0;
	fMinHeight = 0;
	fMaxWidth = 0;
	fMaxHeight = 0;
	fPreferredWidth = 0;
	fPreferredHeight = 0;

	for (int32 column = 0; column < fColumnCount; column++) {
		fColumnInfos[column].Normalize();
		fMinWidth = BLayoutUtils::AddSizesInt32(fMinWidth,
			fColumnInfos[column].min);
		fMaxWidth = BLayoutUtils::AddSizesInt32(fMaxWidth,
			fColumnInfos[column].max);
		fPreferredWidth = BLayoutUtils::AddSizesInt32(fPreferredWidth,
			fColumnInfos[column].preferred);
//printf("  column %ld: min: %ld, max: %ld, preferred: %ld\n", column, fColumnInfos[column].min, fColumnInfos[column].max, fColumnInfos[column].preferred);
	}

	for (int32 row = 0; row < fRowCount; row++) {
		fRowInfos[row].Normalize();
		fMinHeight = BLayoutUtils::AddSizesInt32(fMinHeight,
			fRowInfos[row].min);
		fMaxHeight = BLayoutUtils::AddSizesInt32(fMaxHeight,
			fRowInfos[row].max);
		fPreferredHeight = BLayoutUtils::AddSizesInt32(fPreferredHeight,
			fRowInfos[row].preferred);
//printf("  row %ld: min: %ld, max: %ld, preferred: %ld\n", row, fRowInfos[row].min, fRowInfos[row].max, fRowInfos[row].preferred);
	}

	fMinMaxValid = true;
//printf("%p->GroupView::_ValidateMinMax() done\n", this);
}


void
GroupView::_LayoutLine(int32 size, LayoutInfo* infos, int32 infoCount)
{
	BList infosToLayout;
	for (int32 i = 0; i < infoCount; i++) {
		infos[i].size = 0;
		infosToLayout.AddItem(infos + i);
	}

	// Distribute the available space over the infos. Each iteration we
	// try to distribute the remaining space evenly. We respect min and
	// max constraints, though, add up the space we failed to assign
	// due to the constraints, and use the sum as remaining space for the
	// next iteration (can be negative). Then we ignore infos that can't
	// shrink or grow anymore (depending on what would be needed).
	while (!infosToLayout.IsEmpty()) {
		BList canShrinkInfos;
		BList canGrowInfos;

		int32 sizeDiff = 0;
		int32 infosToLayoutCount = infosToLayout.CountItems();
		for (int32 i = 0; i < infosToLayoutCount; i++) {
			LayoutInfo* info = (LayoutInfo*)infosToLayout.ItemAt(i);
			info->size += (i + 1) * size / infosToLayoutCount
				- i * size / infosToLayoutCount;
			if (info->size < info->min) {
				sizeDiff -= info->min - info->size;
				info->size = info->min;
			} else if (info->size > info->max) {
				sizeDiff += info->size - info->max;
				info->size = info->max;
			}

			if (info->size > info->min)
				canShrinkInfos.AddItem(info);
			if (info->size < info->max)
				canGrowInfos.AddItem(info);
		}

		size = sizeDiff;
		if (size == 0)
			break;

		if (size > 0)
			infosToLayout = canGrowInfos;
		else
			infosToLayout = canShrinkInfos;
	}

	// If unassigned space is remaining, that means, that the group has
	// been resized beyond its max size. We distribute the excess space
	// evenly.
	if (size > 0) {
		for (int32 i = 0; i < infoCount; i++) {
			infos[i].size += (i + 1) * size / infoCount
				- i * size / infoCount;
		}
	}
}


BSize
GroupView::_AddInsetsAndSpacing(BSize size)
{
	size.width = BLayoutUtils::AddDistances(size.width,
		fInsets.left + fInsets.right - 1
		+ (fColumnCount - 1) * fColumnSpacing);
	size.height = BLayoutUtils::AddDistances(size.height,
		fInsets.top + fInsets.bottom - 1
		+ (fRowCount - 1) * fRowSpacing);
	return size;
}


BSize
GroupView::_SubtractInsetsAndSpacing(BSize size)
{
	size.width = BLayoutUtils::SubtractDistances(size.width,
		fInsets.left + fInsets.right - 1
		+ (fColumnCount - 1) * fColumnSpacing);
	size.height = BLayoutUtils::SubtractDistances(size.height,
		fInsets.top + fInsets.bottom - 1
		+ (fRowCount - 1) * fRowSpacing);
	return size;
}

int32
GroupView::_RowCount() const
{
	int32 childCount = CountChildren();
	int32 count;
	if (fOrientation == B_HORIZONTAL)
		count = min_c(fLineCount, childCount);
	else
		count = (childCount + fLineCount - 1) / fLineCount;

	return max_c(count, 1);
}


int32
GroupView::_ColumnCount() const
{
	int32 childCount = CountChildren();
	int32 count;
	if (fOrientation == B_HORIZONTAL)
		count = (childCount + fLineCount - 1) / fLineCount;
	else
		count = min_c(fLineCount, childCount);

	return max_c(count, 1);
}


View*
GroupView::_ChildAt(int32 column, int32 row) const
{
	if (fOrientation == B_HORIZONTAL)
		return ChildAt(column * fLineCount + row);
	else
		return ChildAt(row * fLineCount + column);
}


// #pragma mark - Glue


Glue::Glue()
	: View()
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


// #pragma mark -  Strut


Strut::Strut(float pixelWidth, float pixelHeight)
	: View(),
	  fSize(pixelWidth >= 0 ? pixelWidth - 1 : B_SIZE_UNSET,
	  	pixelHeight >= 0 ? pixelHeight - 1 : B_SIZE_UNSET)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


BSize
Strut::MinSize()
{
	return BLayoutUtils::ComposeSize(fSize, BSize(-1, -1));
}


BSize
Strut::MaxSize()
{
	return BLayoutUtils::ComposeSize(fSize,
		BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
}


// #pragma mark - HStrut


HStrut::HStrut(float width)
	: Strut(width, -1)
{
}


// #pragma mark - VStrut


VStrut::VStrut(float height)
	: Strut(-1, height)
{
}
