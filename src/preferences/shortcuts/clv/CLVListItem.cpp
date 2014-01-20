/*
 * Copyright 1999-2009 Jeremy Friesner
 * Copyright 2009-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 *		John Scipione, jscipione@gmail.com
 */


#define CLVListItem_CPP

#include <stdio.h>

#include "CLVListItem.h"
#include "ColumnListView.h"
#include "CLVColumn.h"
#include "PrefilledBitmap.h"

#include "InterfaceKit.h"


CLVListItem::CLVListItem(uint32 level, bool superitem, bool expanded,
	float minheight)
	:
	BListItem(level, expanded),
	fExpanderButtonRect(-1.0, -1.0, -1.0, -1.0),
	fExpanderColumnRect(-1.0, -1.0, -1.0, -1.0),
	fSelectedColumn(-1)
{
	fIsSuperItem = superitem;
	fOutlineLevel = level;
	fMinHeight = minheight;
}


CLVListItem::~CLVListItem()
{
}


bool
CLVListItem::IsSuperItem() const
{
	return fIsSuperItem;
}


void
CLVListItem::SetSuperItem(bool superitem)
{
	fIsSuperItem = superitem;
}


uint32
CLVListItem::OutlineLevel() const
{
	return fOutlineLevel;
}


void
CLVListItem::SetOutlineLevel(uint32 level)
{
	fOutlineLevel = level;
}


void
CLVListItem::Pulse(BView* owner)
{
}

void
CLVListItem::DrawItem(BView* owner, BRect itemRect, bool complete)
{
	BList* displayList = &((ColumnListView*)owner)->fColumnDisplayList;
	int32 columnCount = displayList->CountItems();
	float PushMax = itemRect.right;
	CLVColumn* column;
	BRect columnRect = itemRect;
	float expanderDelta = OutlineLevel() * 20.0;
	// figure out what the limit is for expanders pushing other columns
	for (int32 i = 0; i < columnCount; i++) {
		column = (CLVColumn*)displayList->ItemAt(i);
		if((column->fFlags & CLV_EXPANDER) || column->fPushedByExpander)
			PushMax = column->fColumnEnd;
	}

	BRegion ClippingRegion;
	if (!complete)
		owner->GetClippingRegion(&ClippingRegion);
	else
		ClippingRegion.Set(itemRect);

	float LastColumnEnd = -1.0;

	// draw the columns
	for (int32 Counter = 0; Counter < columnCount; Counter++) {
		column = (CLVColumn*)displayList->ItemAt(Counter);
		if(!column->IsShown())
			continue;

		columnRect.left = column->fColumnBegin;
		columnRect.right = LastColumnEnd = column->fColumnEnd;
		float shift = 0.0;
		if ((column->fFlags & CLV_EXPANDER) || column->fPushedByExpander)
			shift = expanderDelta;

		if (column->fFlags & CLV_EXPANDER) {
			columnRect.right += shift;
			if(columnRect.right > PushMax)
				columnRect.right = PushMax;

			fExpanderColumnRect = columnRect;
			if (ClippingRegion.Intersects(columnRect)) {
				// give the programmer a chance to do his kind of
				// highlighting if the item is selected
				int32 actualIndex
					= ((ColumnListView*)owner)->fColumnList.IndexOf(column);
				DrawItemColumn(owner, columnRect, actualIndex,
					(fSelectedColumn == actualIndex), complete);
				if (fIsSuperItem) {
					// draw the expander, clip manually
					float top
						= ceilf((columnRect.bottom-columnRect.top - 10.0)
							/ 2.0);
					float left = column->fColumnEnd + shift - 3.0 - 10.0;
					float rightClip = left + 10.0 - columnRect.right;
					if (rightClip < 0.0)
						rightClip = 0.0;

					BBitmap* arrow;
					if (IsExpanded())
						arrow = &((ColumnListView*)owner)->fDownArrow;
					else
						arrow = &((ColumnListView*)owner)->fRightArrow;

					if (left <= columnRect.right) {
						fExpanderButtonRect.Set(left, columnRect.top + top,
							left + 10.0 - rightClip, columnRect.top + top + 10.0);
						owner->SetDrawingMode(B_OP_OVER);
						owner->DrawBitmap(arrow,
							BRect(0.0, 0.0, 10.0 - rightClip, 10.0),
							fExpanderButtonRect);
						owner->SetDrawingMode(B_OP_COPY);
					} else
						fExpanderButtonRect.Set(-1.0, -1.0, -1.0, -1.0);
				}
			}
		} else {
			columnRect.left += shift;
			columnRect.right += shift;
			if (shift > 0.0 && columnRect.right > PushMax)
				columnRect.right = PushMax;

			if (columnRect.right >= columnRect.left
				&& ClippingRegion.Intersects(columnRect))
			{
			    int32 actualIndex
					= ((ColumnListView*)owner)->fColumnList.IndexOf(column);
				DrawItemColumn(owner, columnRect, actualIndex,
					(fSelectedColumn == actualIndex), complete);
	        }
		}
	}

	// fill the area after all the columns (so the select highlight goes
	// all the way across)
	columnRect.left = LastColumnEnd + 1.0;
	columnRect.right = owner->Bounds().right;
	if (columnRect.left <= columnRect.right
		&& ClippingRegion.Intersects(columnRect)) {
		DrawItemColumn(owner, columnRect, -1, false, complete);
	}
}


float
CLVListItem::ExpanderShift(int32 columnIndex, BView* owner)
{
	BList* displayList = &((ColumnListView*)owner)->fColumnDisplayList;
	CLVColumn* column = (CLVColumn*)displayList->ItemAt(columnIndex);
	float expanderDelta = OutlineLevel() * 20.0f;
	if (!column->fPushedByExpander)
		expanderDelta = 0.0;

	return expanderDelta;
}


void
CLVListItem::Update(BView* owner, const BFont* font)
{
	BListItem::Update(owner, font);
	float itemHeight = Height();
	if (itemHeight < fMinHeight)
		itemHeight = fMinHeight;

	SetWidth(((ColumnListView*)owner)->fPageWidth);
	SetHeight(itemHeight);
}
