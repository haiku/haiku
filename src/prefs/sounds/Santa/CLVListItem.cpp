//CLVListItem source file

//*** LICENSE ***
//ColumnListView, its associated classes and source code, and the other components of Santa's Gift Bag are
//being made publicly available and free to use in freeware and shareware products with a price under $25
//(I believe that shareware should be cheap). For overpriced shareware (hehehe) or commercial products,
//please contact me to negotiate a fee for use. After all, I did work hard on this class and invested a lot
//of time into it. That being said, DON'T WORRY I don't want much. It totally depends on the sort of project
//you're working on and how much you expect to make off it. If someone makes money off my work, I'd like to
//get at least a little something.  If any of the components of Santa's Gift Bag are is used in a shareware
//or commercial product, I get a free copy.  The source is made available so that you can improve and extend
//it as you need. In general it is best to customize your ColumnListView through inheritance, so that you
//can take advantage of enhancements and bug fixes as they become available. Feel free to distribute the 
//ColumnListView source, including modified versions, but keep this documentation and license with it.


//******************************************************************************************************
//**** PROJECT HEADER FILES
//******************************************************************************************************
#include <Region.h>


//******************************************************************************************************
//**** PROJECT HEADER FILES
//******************************************************************************************************
#include "CLVListItem.h"
#include "ColumnListView.h"
#include "CLVColumn.h"


//******************************************************************************************************
//**** CLVItem CLASS DEFINITION
//******************************************************************************************************
CLVListItem::CLVListItem(uint32 level, bool superitem, bool expanded, float minheight)
: BListItem(level, expanded),
fExpanderButtonRect(-1.0,-1.0,-1.0,-1.0),
fExpanderColumnRect(-1.0,-1.0,-1.0,-1.0)
{
	fSuperItem = superitem;
	fOutlineLevel = level;
	fMinHeight = minheight;
}


CLVListItem::~CLVListItem()
{ }


BRect CLVListItem::ItemColumnFrame(int32 column_index, ColumnListView* owner)
{
	BList* ColumnList = &owner->fColumnList;
	CLVColumn* ThisColumn = (CLVColumn*)ColumnList->ItemAt(column_index);
	if(!ThisColumn->IsShown())
		return BRect(-1,-1,-1,-1);

	int32 DisplayIndex = owner->IndexOf(this);	
	BRect itemRect;
	if(DisplayIndex >= 0)
		itemRect = owner->ItemFrame(DisplayIndex);
	else
		return BRect(-1,-1,-1,-1);

	//Figure out what the limit is for expanders pushing other columns
	float PushMax = 100000;
	if(ThisColumn->fPushedByExpander || (ThisColumn->fFlags & CLV_EXPANDER))
	{
		BList* DisplayList = &owner->fColumnDisplayList;
		int32 NumberOfColumns = DisplayList->CountItems();
		for(int32 Counter = 0; Counter < NumberOfColumns; Counter++)
		{
			CLVColumn* SomeColumn = (CLVColumn*)DisplayList->ItemAt(Counter);
			if((SomeColumn->fFlags & CLV_EXPANDER) || SomeColumn->fPushedByExpander)
				PushMax = SomeColumn->fColumnEnd;
		}
	}

	BRect ThisColumnRect = itemRect;
	ThisColumnRect.left = ThisColumn->fColumnBegin;
	ThisColumnRect.right = ThisColumn->fColumnEnd;

	if(ThisColumn->fFlags & CLV_EXPANDER)
	{
		ThisColumnRect.right += OutlineLevel() * 20.0;
		if(ThisColumnRect.right > PushMax)
			ThisColumnRect.right = PushMax;
	}
	else
	{
		if(ThisColumn->fPushedByExpander)
		{
			float Shift = OutlineLevel() * 20.0;
			ThisColumnRect.left += Shift;
			ThisColumnRect.right += Shift;
			if(Shift > 0.0 && ThisColumnRect.right > PushMax)
				ThisColumnRect.right = PushMax;
		}
	}

	if(ThisColumnRect.right >= ThisColumnRect.left)
		return ThisColumnRect;
	else
		return BRect(-1,-1,-1,-1);
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
				DrawItemColumn(owner, ThisColumnRect,
					((ColumnListView*)owner)->fColumnList.IndexOf(ThisColumn),complete);
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
				DrawItemColumn(owner, ThisColumnRect,
					((ColumnListView*)owner)->fColumnList.IndexOf(ThisColumn),complete);
		}
	}
	//Fill the area after all the columns (so the select highlight goes all the way across)
	ThisColumnRect.left = LastColumnEnd + 1.0;
	ThisColumnRect.right = owner->Bounds().right;
	if(ThisColumnRect.left <= ThisColumnRect.right && ClippingRegion.Intersects(ThisColumnRect))
		DrawItemColumn(owner, ThisColumnRect,-1,complete);
}


float CLVListItem::ExpanderShift(int32 column_index, ColumnListView* owner)
{
	CLVColumn* ThisColumn = owner->ColumnAt(column_index);
	if(!(ThisColumn->fPushedByExpander || ThisColumn->fFlags & CLV_EXPANDER))
		return 0.0;
	else
		return OutlineLevel() * 20.0;
}


void CLVListItem::Update(BView* owner, const BFont* font)
{
	BListItem::Update(owner,font);
	float ItemHeight = Height();
	if(ItemHeight < fMinHeight)
		ItemHeight = fMinHeight;
	SetHeight(ItemHeight);
}


void CLVListItem::ColumnWidthChanged(int32 column_index, float column_width, ColumnListView* the_view)
{
	//Get rid of a warning
	column_index = 0;
	column_width = 0;
	the_view = NULL;
}


void CLVListItem::FrameChanged(int32 column_index, BRect new_frame, ColumnListView* the_view)
{
	//Get rid of a warning
	column_index = 0;
	new_frame.left = 0;
	the_view = NULL;

}
