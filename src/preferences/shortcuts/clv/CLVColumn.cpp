/*
 * Copyright 1999-2009 Jeremy Friesner
 * Copyright 2009-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 *		John Scipione, jscipione@gmail.com
 */


#define CLVColumn_CPP

#include <string.h>

#include "CLVColumn.h"
#include "ColumnListView.h"
#include "CLVColumnLabelView.h"


CLVColumn::CLVColumn(const char* label, BPopUpMenu* popup, float width,
	uint32 flags, float minWidth)
{
	fPopup = popup;

	if (flags & CLV_EXPANDER) {
		label = NULL;
		width = 20.0;
		minWidth = 20.0;
		flags &= CLV_NOT_MOVABLE | CLV_LOCK_AT_BEGINNING | CLV_HIDDEN
			| CLV_LOCK_WITH_RIGHT;
		flags |= CLV_EXPANDER | CLV_NOT_RESIZABLE | CLV_MERGE_WITH_RIGHT;
	}

	if (minWidth < 4.0)
		minWidth = 4.0;

	if (width < minWidth)
		width = minWidth;

	if (label != NULL) {
		fLabel = new char[strlen(label) + 1];
		strcpy((char*)fLabel, label);
	} else
		fLabel = NULL;

	fWidth = width;
	fMinWidth = minWidth;
	fFlags = flags;
	fPushedByExpander = false;
	fParent = NULL;
	fSortMode = SORT_MODE_NONE;
}


CLVColumn::~CLVColumn()
{
	delete[] fLabel;
	if (fParent != NULL)
		fParent->RemoveColumn(this);

	delete fPopup;
}


float
CLVColumn::Width() const
{
	return fWidth;
}


void
CLVColumn::SetWidth(float width)
{
	if(width < fMinWidth)
		width = fMinWidth;

	if(width != fWidth) {
		float oldWidth = fWidth;
		fWidth = width;
		if (IsShown() && fParent != NULL) {
			BWindow* parentWindow = fParent->Window();
			if (parentWindow != NULL)
				parentWindow->Lock();

			// figure out the area after this column to scroll
			BRect ColumnViewBounds = fParent->fColumnLabelView->Bounds();
			BRect MainViewBounds = fParent->Bounds();
			BRect sourceArea = ColumnViewBounds;
			sourceArea.left = fColumnEnd + 1.0;
			BRect destArea = sourceArea;
			float delta = width-oldWidth;
			destArea.left += delta;
			destArea.right += delta;
			float LimitShift;
			if (destArea.right > ColumnViewBounds.right) {
				LimitShift = destArea.right-ColumnViewBounds.right;
				destArea.right -= LimitShift;
				sourceArea.right -= LimitShift;
			}
			if (destArea.left < ColumnViewBounds.left) {
				LimitShift = ColumnViewBounds.left - destArea.left;
				destArea.left += LimitShift;
				sourceArea.left += LimitShift;
			}

			// scroll the area that is being shifted
			if(parentWindow)
				parentWindow->UpdateIfNeeded();

			fParent->fColumnLabelView->CopyBits(sourceArea, destArea);
			sourceArea.top = MainViewBounds.top;
			sourceArea.bottom = MainViewBounds.bottom;
			destArea.top = MainViewBounds.top;
			destArea.bottom = MainViewBounds.bottom;
			fParent->CopyBits(sourceArea, destArea);

			// invalidate the region that got revealed
			destArea = ColumnViewBounds;
			if (width > oldWidth) {
				destArea.left = fColumnEnd + 1.0;
				destArea.right = fColumnEnd + delta;
			} else {
				destArea.left = ColumnViewBounds.right + delta + 1.0;
				destArea.right = ColumnViewBounds.right;
			}
			fParent->fColumnLabelView->Invalidate(destArea);
			destArea.top = MainViewBounds.top;
			destArea.bottom = MainViewBounds.bottom;
			fParent->Invalidate(destArea);

			// invalidate the old or new resize handle as necessary
			destArea = ColumnViewBounds;
			if (width > oldWidth)
				destArea.left = fColumnEnd;
			else
				destArea.left = fColumnEnd + delta;

			destArea.right = destArea.left;
			fParent->fColumnLabelView->Invalidate(destArea);

			// update the column sizes, positions and group positions
			fParent->ShiftDragGroup();
			fParent->fColumnLabelView->UpdateDragGroups();
			if(parentWindow)
				parentWindow->Unlock();
		}
		if (fParent != NULL) {
			fParent->ColumnWidthChanged(fParent->fColumnList.IndexOf(this),
				fWidth);
		}
	}
}


uint32
CLVColumn::Flags() const
{
	return fFlags;
}


bool
CLVColumn::IsShown() const
{
	if ((fFlags & CLV_HIDDEN) != 0)
		return false;
	else
		return true;
}


void
CLVColumn::SetShown(bool show)
{
	bool shown = IsShown();
	if (shown != show) {
		if (show)
			fFlags &= 0xFFFFFFFF^CLV_HIDDEN;
		else
			fFlags |= CLV_HIDDEN;

		if (fParent != NULL) {
			float updateLeft = fColumnBegin;
			BWindow* parentWindow = fParent->Window();
			if (parentWindow != NULL)
				parentWindow->Lock();

			fParent->ShiftDragGroup();
			fParent->fColumnLabelView->UpdateDragGroups();
			if (show)
				updateLeft = fColumnBegin;

			BRect area = fParent->fColumnLabelView->Bounds();
			area.left = updateLeft;
			fParent->fColumnLabelView->Invalidate(area);
			area = fParent->Bounds();
			area.left = updateLeft;
			fParent->Invalidate(area);
			if ((fFlags & CLV_EXPANDER) != 0) {
				if (!show)
					fParent->fExpanderColumn = -1;
				else
					fParent->fExpanderColumn = fParent->IndexOfColumn(this);
			}
			if (parentWindow != NULL)
				parentWindow->Unlock();
		}
	}
}


CLVSortMode
CLVColumn::SortMode() const
{
	return fSortMode;
}


void
CLVColumn::SetSortMode(CLVSortMode mode)
{
	if (fParent != NULL)
		fParent->SetSortMode(fParent->IndexOfColumn(this), mode);
	else
		fSortMode = mode;
}
