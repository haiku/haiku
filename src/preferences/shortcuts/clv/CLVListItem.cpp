//CLVListItem source file


//******************************************************************************************************
//**** PROJECT HEADER FILES
//******************************************************************************************************
#define CLVListItem_CPP
#include <stdio.h>
#include "CLVListItem.h"
#include "ColumnListView.h"
#include "CLVColumn.h"
#include "PrefilledBitmap.h"

#include "InterfaceKit.h"

//******************************************************************************************************
//**** CLVItem CLASS DEFINITION
//******************************************************************************************************
CLVListItem::CLVListItem(uint32 level, bool superitem, bool expanded, float minheight)
: BListItem(level, expanded),
fExpanderButtonRect(-1.0,-1.0,-1.0,-1.0),
fExpanderColumnRect(-1.0,-1.0,-1.0,-1.0),
_selectedColumn(-1)
{
	fSuperItem = superitem;
	fOutlineLevel = level;
	fMinHeight = minheight;
}


CLVListItem::~CLVListItem()
{ }


bool CLVListItem::IsSuperItem() const
{
	return fSuperItem;
}


void CLVListItem::SetSuperItem(bool superitem)
{
	fSuperItem = superitem;
}


uint32 CLVListItem::OutlineLevel() const
{
	return fOutlineLevel;
}

void CLVListItem::SetOutlineLevel(uint32 level)
{
	fOutlineLevel = level;
}

void CLVListItem::Pulse(BView * owner)
{
    // empty
}

void CLVListItem::DrawItem(BView* owner, BRect itemRect, bool complete)
{
	BList* DisplayList = &((ColumnListView*)owner)->fColumnDisplayList;
	int32 NumberOfColumns = DisplayList->CountItems();
	float PushMax = itemRect.right;
	CLVColumn* ThisColumn;
	BRect ThisColumnRect = itemRect;
	float ExpanderDelta = OutlineLevel() * 20.0;
	//Figure out what the limit is for expanders pushing other columns
	for(int32 Counter = 0; Counter < NumberOfColumns; Counter++)
	{
		ThisColumn = (CLVColumn*)DisplayList->ItemAt(Counter);
		if((ThisColumn->fFlags & CLV_EXPANDER) || ThisColumn->fPushedByExpander)
			PushMax = ThisColumn->fColumnEnd;
	}
	BRegion ClippingRegion;
	if(!complete)
		owner->GetClippingRegion(&ClippingRegion);
	else
		ClippingRegion.Set(itemRect);
	float LastColumnEnd = -1.0;

	//Draw the columns
	for(int32 Counter = 0; Counter < NumberOfColumns; Counter++)
	{
		ThisColumn = (CLVColumn*)DisplayList->ItemAt(Counter);
		if(!ThisColumn->IsShown())
			continue;
		ThisColumnRect.left = ThisColumn->fColumnBegin;
		ThisColumnRect.right = LastColumnEnd = ThisColumn->fColumnEnd;
		float Shift = 0.0;
		if((ThisColumn->fFlags & CLV_EXPANDER) || ThisColumn->fPushedByExpander)
			Shift = ExpanderDelta;
		if(ThisColumn->fFlags & CLV_EXPANDER)
		{
			ThisColumnRect.right += Shift;
			if(ThisColumnRect.right > PushMax)
				ThisColumnRect.right = PushMax;
			fExpanderColumnRect = ThisColumnRect;
			if(ClippingRegion.Intersects(ThisColumnRect))
			{
				//Give the programmer a chance to do his kind of highlighting if the item is selected
				int32 actualIndex = ((ColumnListView*)owner)->fColumnList.IndexOf(ThisColumn);
				DrawItemColumn(owner, ThisColumnRect, actualIndex, (_selectedColumn == actualIndex), complete);
				if(fSuperItem)
				{
					//Draw the expander, clip manually
					float TopOffset = ceil((ThisColumnRect.bottom-ThisColumnRect.top-10.0)/2.0);
					float LeftOffset = ThisColumn->fColumnEnd + Shift - 3.0 - 10.0;
					float RightClip = LeftOffset + 10.0 - ThisColumnRect.right;
					if(RightClip < 0.0)
						RightClip = 0.0;
					BBitmap* Arrow;
					if(IsExpanded())
						Arrow = &((ColumnListView*)owner)->fDownArrow;
					else
						Arrow = &((ColumnListView*)owner)->fRightArrow;
					if(LeftOffset <= ThisColumnRect.right)
					{
						fExpanderButtonRect.Set(LeftOffset,ThisColumnRect.top+TopOffset,
							LeftOffset+10.0-RightClip,ThisColumnRect.top+TopOffset+10.0);
						owner->SetDrawingMode(B_OP_OVER);
						owner->DrawBitmap(Arrow, BRect(0.0,0.0,10.0-RightClip,10.0),fExpanderButtonRect);
						owner->SetDrawingMode(B_OP_COPY);
					}
					else
						fExpanderButtonRect.Set(-1.0,-1.0,-1.0,-1.0);
				}
			}
		}
		else
		{
			ThisColumnRect.left += Shift;
			ThisColumnRect.right += Shift;
			if(Shift > 0.0 && ThisColumnRect.right > PushMax)
				ThisColumnRect.right = PushMax;
			if(ThisColumnRect.right >= ThisColumnRect.left && ClippingRegion.Intersects(ThisColumnRect)) 
			{
			    int32 actualIndex = ((ColumnListView*)owner)->fColumnList.IndexOf(ThisColumn);
				DrawItemColumn(owner, ThisColumnRect, actualIndex, (_selectedColumn == actualIndex), complete);
	        }
		}
	}
	//Fill the area after all the columns (so the select highlight goes all the way across)
	ThisColumnRect.left = LastColumnEnd + 1.0;
	ThisColumnRect.right = owner->Bounds().right;
	if(ThisColumnRect.left <= ThisColumnRect.right && ClippingRegion.Intersects(ThisColumnRect))
    {
		DrawItemColumn(owner, ThisColumnRect,-1, false, complete); 
	}
}


float CLVListItem::ExpanderShift(int32 column_index, BView* owner)
{
	BList* DisplayList = &((ColumnListView*)owner)->fColumnDisplayList;
	CLVColumn* ThisColumn = (CLVColumn*)DisplayList->ItemAt(column_index);
	float ExpanderDelta = OutlineLevel() * 20.0;
	if(!ThisColumn->fPushedByExpander)
		ExpanderDelta = 0.0;
	return ExpanderDelta;
}


void CLVListItem::Update(BView* owner, const BFont* font)
{
	BListItem::Update(owner,font);
	float ItemHeight = Height();
	if(ItemHeight < fMinHeight)
		ItemHeight = fMinHeight;
	SetWidth(((ColumnListView*)owner)->fPageWidth);
	SetHeight(ItemHeight);
}
