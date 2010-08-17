//Column list header source file


//******************************************************************************************************
//**** PROJECT HEADER FILES
//******************************************************************************************************
#define CLVColumn_CPP
#include <string.h>
#include "CLVColumn.h"
#include "ColumnListView.h"
#include "CLVColumnLabelView.h"


//******************************************************************************************************
//**** CLVColumn CLASS DEFINITION
//******************************************************************************************************
CLVColumn::CLVColumn(const char* label,BPopUpMenu * popup,float width,uint32 flags,float min_width)
{
    fPopup = popup;

	if(flags & CLV_EXPANDER)
	{
		label = NULL;
		width = 20.0;
		min_width = 20.0;
		flags &= CLV_NOT_MOVABLE | CLV_LOCK_AT_BEGINNING | CLV_HIDDEN | CLV_LOCK_WITH_RIGHT;
		flags |= CLV_EXPANDER | CLV_NOT_RESIZABLE | CLV_MERGE_WITH_RIGHT;
	}
	if(min_width < 4.0)
		min_width = 4.0;
	if(width < min_width)
		width = min_width;
	if(label)
	{
		fLabel = new char[strlen(label)+1];
		strcpy((char*)fLabel,label);
	}
	else
		fLabel = NULL;
	fWidth = width;
	fMinWidth = min_width;
	fFlags = flags;
	fPushedByExpander = false;
	fParent = NULL;
	fSortMode = NoSort;
}


CLVColumn::~CLVColumn()
{
	delete [] fLabel;
	if(fParent) fParent->RemoveColumn(this);
    delete fPopup;
}


float CLVColumn::Width() const
{
	return fWidth;
}


void CLVColumn::SetWidth(float width)
{
	if(width < fMinWidth)
		width = fMinWidth;
	if(width != fWidth)
	{
		float OldWidth = fWidth;
		fWidth = width;
		if(IsShown() && fParent)
		{
			BWindow* ParentWindow = fParent->Window();
			if(ParentWindow)
				ParentWindow->Lock();
			//Figure out the area after this column to scroll
			BRect ColumnViewBounds = fParent->fColumnLabelView->Bounds();
			BRect MainViewBounds = fParent->Bounds();
			BRect SourceArea = ColumnViewBounds;
			SourceArea.left = fColumnEnd+1.0;
			BRect DestArea = SourceArea;
			float Delta = width-OldWidth;
			DestArea.left += Delta;
			DestArea.right += Delta;
			float LimitShift;
			if(DestArea.right > ColumnViewBounds.right)
			{
				LimitShift = DestArea.right-ColumnViewBounds.right;
				DestArea.right -= LimitShift;
				SourceArea.right -= LimitShift;
			}
			if(DestArea.left < ColumnViewBounds.left)
			{
				LimitShift = ColumnViewBounds.left - DestArea.left;
				DestArea.left += LimitShift;
				SourceArea.left += LimitShift;
			}
			//Scroll the area that is being shifted
			if(ParentWindow)
				ParentWindow->UpdateIfNeeded();
			fParent->fColumnLabelView->CopyBits(SourceArea,DestArea);
			SourceArea.top = MainViewBounds.top;
			SourceArea.bottom = MainViewBounds.bottom;
			DestArea.top = MainViewBounds.top;
			DestArea.bottom = MainViewBounds.bottom;
			fParent->CopyBits(SourceArea,DestArea);

			//Invalidate the region that got revealed
			DestArea = ColumnViewBounds;
			if(width > OldWidth)
			{
				DestArea.left = fColumnEnd+1.0;
				DestArea.right = fColumnEnd+Delta;
			}
			else
			{
				DestArea.left = ColumnViewBounds.right+Delta+1.0;
				DestArea.right = ColumnViewBounds.right;
			}
			fParent->fColumnLabelView->Invalidate(DestArea);
			DestArea.top = MainViewBounds.top;
			DestArea.bottom = MainViewBounds.bottom;
			fParent->Invalidate(DestArea);

			//Invalidate the old or new resize handle as necessary
			DestArea = ColumnViewBounds;
			if(width > OldWidth)
				DestArea.left = fColumnEnd;
			else
				DestArea.left = fColumnEnd + Delta;
			DestArea.right = DestArea.left;
			fParent->fColumnLabelView->Invalidate(DestArea);

			//Update the column sizes, positions and group positions
			fParent->UpdateColumnSizesDataRectSizeScrollBars();
			fParent->fColumnLabelView->UpdateDragGroups();
			if(ParentWindow)
				ParentWindow->Unlock();
		}
		if(fParent)
			fParent->ColumnWidthChanged(fParent->fColumnList.IndexOf(this),fWidth);
	}
}


uint32 CLVColumn::Flags() const
{
	return fFlags;
}


bool CLVColumn::IsShown() const
{
	if(fFlags & CLV_HIDDEN)
		return false;
	else
		return true;
}


void CLVColumn::SetShown(bool Shown)
{
	bool shown = IsShown();
	if(shown != Shown)
	{
		if(Shown)
			fFlags &= 0xFFFFFFFF^CLV_HIDDEN;
		else
			fFlags |= CLV_HIDDEN;
		if(fParent)
		{
			float UpdateLeft = fColumnBegin;
			BWindow* ParentWindow = fParent->Window();
			if(ParentWindow)
				ParentWindow->Lock();
			fParent->UpdateColumnSizesDataRectSizeScrollBars();
			fParent->fColumnLabelView->UpdateDragGroups();
			if(Shown)
				UpdateLeft = fColumnBegin;
			BRect Area = fParent->fColumnLabelView->Bounds();
			Area.left = UpdateLeft;
			fParent->fColumnLabelView->Invalidate(Area);
			Area = fParent->Bounds();
			Area.left = UpdateLeft;
			fParent->Invalidate(Area);
			if(fFlags & CLV_EXPANDER)
			{
				if(!Shown)
					fParent->fExpanderColumn = -1;
				else
					fParent->fExpanderColumn = fParent->IndexOfColumn(this);
			}
			if(ParentWindow)
				ParentWindow->Unlock();
		}
	}
}


CLVSortMode CLVColumn::SortMode() const
{
	return fSortMode;
}

void CLVColumn::SetSortMode(CLVSortMode mode)
{
	if(fParent)
		fParent->SetSortMode(fParent->IndexOfColumn(this),mode);
	else
		fSortMode = mode;
}
