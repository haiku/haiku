//Column list view source file

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
//**** SYSTEM HEADER FILES
//******************************************************************************************************
#include <Window.h>
#include <ClassInfo.h>


//******************************************************************************************************
//**** PROJECT HEADER FILES
//******************************************************************************************************
#include "ColumnListView.h"
#include "CLVColumnLabelView.h"
#include "CLVColumn.h"
#include "CLVListItem.h"


//******************************************************************************************************
//**** BITMAPS
//******************************************************************************************************
uint8 CLVRightArrowData[132] =
{
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0x00, 0x12, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0x00, 0x12, 0x12, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0x00, 0x12, 0x12, 0x12, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0x00, 0x12, 0x12, 0x12, 0x12, 0x00, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0x00, 0x12, 0x12, 0x12, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0x00, 0x12, 0x12, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0x00, 0x12, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};
uint8 CLVDownArrowData[132] =
{
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
	0xFF, 0x00, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x00, 0xFF, 0xFF,
	0xFF, 0xFF, 0x00, 0x12, 0x12, 0x12, 0x12, 0x12, 0x00, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0x00, 0x12, 0x12, 0x12, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x12, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};


//******************************************************************************************************
//**** ColumnListView CLASS DEFINITION
//******************************************************************************************************
CLVContainerView::CLVContainerView(ColumnListView* target, uint32 resizingMode, uint32 flags,
	bool horizontal, bool vertical, bool scroll_view_corner, border_style border) :
BetterScrollView(NULL,target,resizingMode,flags,horizontal,vertical,scroll_view_corner,border)
{
	IsBeingDestroyed = false;
};


CLVContainerView::~CLVContainerView()
{
	IsBeingDestroyed = true;
}


//******************************************************************************************************
//**** ColumnListView CLASS DEFINITION
//******************************************************************************************************
ColumnListView::ColumnListView(BRect Frame, CLVContainerView **ContainerView, const char *Name,
	uint32 ResizingMode, uint32 flags, list_view_type Type, bool hierarchical, bool horizontal,
	bool vertical, bool scroll_view_corner, border_style border, const BFont *LabelFont)
: BListView(Frame,Name,Type,B_FOLLOW_ALL_SIDES,flags),
fColumnList(6),
fColumnDisplayList(6),
fSortKeyList(6),
fFullItemList(32),
fRightArrow(BRect(0.0,0.0,10.0,10.0),B_COLOR_8_BIT,CLVRightArrowData,false,false),
fDownArrow(BRect(0.0,0.0,10.0,10.0),B_COLOR_8_BIT,CLVDownArrowData,false,false),
fHighlightTextOnly(false)
{
	fHierarchical = hierarchical;

	//Create the column titles bar view
	font_height FontAttributes;
	LabelFont->GetHeight(&FontAttributes);
	float fLabelFontHeight = ceil(FontAttributes.ascent) + ceil(FontAttributes.descent);
	float ColumnLabelViewBottom = Frame.top+1.0+fLabelFontHeight+3.0;
	fColumnLabelView = new CLVColumnLabelView(BRect(Frame.left,Frame.top,Frame.right,
		ColumnLabelViewBottom),this,LabelFont);

	//Create the container view
	EmbedInContainer(horizontal,vertical,scroll_view_corner,border,ResizingMode,flags);
	*ContainerView = fScrollView;

	//Complete the setup
	UpdateColumnSizesDataRectSizeScrollBars();
	fColumnLabelView->UpdateDragGroups();
	fExpanderColumn = -1;
	fCompare = NULL;
	fWatchingForDrag = false;
	fSelectedItemColorWindowActive = BeListSelectGrey;
	fSelectedItemColorWindowInactive = BeListSelectGrey;
	fWindowActive = false;
}


ColumnListView::~ColumnListView()
{
	//Delete all list columns
	int32 ColumnCount = fColumnList.CountItems();
	for(int32 Counter = ColumnCount-1; Counter >= 0; Counter--)
	{
		CLVColumn* Item = (CLVColumn*)fColumnList.RemoveItem(Counter);
		if(Item)
			delete Item;
	}
	//Remove and delete the container view if necessary
	if(!fScrollView->IsBeingDestroyed)
	{
		fScrollView->RemoveChild(this);
		delete fScrollView;
	}
}


CLVContainerView* ColumnListView::CreateContainer(bool horizontal, bool vertical, bool scroll_view_corner, 
	border_style border, uint32 ResizingMode, uint32 flags)
{
	return new CLVContainerView(this,ResizingMode,flags,horizontal,vertical,scroll_view_corner,border);
}


void ColumnListView::EmbedInContainer(bool horizontal, bool vertical, bool scroll_view_corner, border_style border,
	uint32 ResizingMode, uint32 flags)
{
	BRect ViewFrame = Frame();
	BRect LabelsFrame = fColumnLabelView->Frame();

	fScrollView = CreateContainer(horizontal,vertical,scroll_view_corner,border,ResizingMode,
		flags&(B_NAVIGABLE^0xFFFFFFFF));
	BRect NewFrame = Frame();

	//Resize the main view to make room for the CLVColumnLabelView
	ResizeTo(ViewFrame.right-ViewFrame.left,ViewFrame.bottom-LabelsFrame.bottom-1.0);
	MoveTo(NewFrame.left,NewFrame.top+(LabelsFrame.bottom-LabelsFrame.top+1.0));
	fColumnLabelView->MoveTo(NewFrame.left,NewFrame.top);

	//Add the ColumnLabelView
	fScrollView->AddChild(fColumnLabelView);

	//Remove and re-add the BListView so that it will draw after the CLVColumnLabelView
	fScrollView->RemoveChild(this);
	fScrollView->AddChild(this);
}


void ColumnListView::UpdateColumnSizesDataRectSizeScrollBars(bool scrolling_allowed)
{
	//Figure out the width
	float ColumnBegin;
	float ColumnEnd = -1.0;
	float DataWidth = 0.0;
	bool NextPushedByExpander = false;
	int32 NumberOfColumns = fColumnDisplayList.CountItems();
	for(int32 Counter = 0; Counter < NumberOfColumns; Counter++)
	{
		CLVColumn* Column = (CLVColumn*)fColumnDisplayList.ItemAt(Counter);
		if(NextPushedByExpander)
			Column->fPushedByExpander = true;
		else
			Column->fPushedByExpander = false;
		if(Column->IsShown())
		{
			float ColumnWidth = Column->Width();
			ColumnBegin = ColumnEnd + 1.0;
			ColumnEnd = ColumnBegin + ColumnWidth;
			Column->fColumnBegin = ColumnBegin;
			Column->fColumnEnd = ColumnEnd;
			DataWidth = Column->fColumnEnd;
			if(NextPushedByExpander)
				if(!(Column->fFlags & CLV_PUSH_PASS))
					NextPushedByExpander = false;
			if(Column->fFlags & CLV_EXPANDER)
				//Set the next column to be pushed
				NextPushedByExpander = true;
		}
	}

	//Figure out the height
	float DataHeight = 0.0;
	int32 NumberOfItems = CountItems();
	for(int32 Counter2 = 0; Counter2 < NumberOfItems; Counter2++)
		DataHeight += ItemAt(Counter2)->Height()+1.0;
	if(NumberOfItems > 0)
		DataHeight -= 1.0;

	//Update the scroll bars
	fScrollView->SetDataRect(BRect(0,0,DataWidth,DataHeight),scrolling_allowed);
}


void ColumnListView::ColumnsChanged()
{
	//Any previous column dragging/resizing will get corrupted, so deselect
	if(fColumnLabelView->fColumnClicked)
		fColumnLabelView->fColumnClicked = NULL;

	//Update the internal sizes and grouping of the columns and sizes of drag groups
	UpdateColumnSizesDataRectSizeScrollBars();
	fColumnLabelView->UpdateDragGroups();
	fColumnLabelView->Invalidate();
	Invalidate();
}


bool ColumnListView::AddColumn(CLVColumn* Column)
//Adds a column to the ColumnListView at the end of the list.  Returns true if successful.
{
	int32 NumberOfColumns = fColumnList.CountItems();
	int32 DisplayIndex = NumberOfColumns;

	//Make sure a second Expander is not being added
	if(Column->fFlags & CLV_EXPANDER)
	{
		if(!fHierarchical)
			return false;
		for(int32 Counter = 0; Counter < NumberOfColumns; Counter++)
			if(((CLVColumn*)fColumnList.ItemAt(Counter))->fFlags & CLV_EXPANDER)
				return false;
		if(Column->IsShown())
			fExpanderColumn = NumberOfColumns;
	}

	//Make sure this column hasn't already been added to another ColumnListView
	if(Column->fParent != NULL)
		return false;

	//Check if this should be locked at the beginning or end, and adjust its position if necessary
	if(!Column->Flags() & CLV_LOCK_AT_END)
	{
		bool Repeat;
		if(Column->Flags() & CLV_LOCK_AT_BEGINNING)
		{
			//Move it to the beginning, after the last CLV_LOCK_AT_BEGINNING item
			DisplayIndex = 0;
			Repeat = true;
			while(Repeat && DisplayIndex < NumberOfColumns)
			{
				Repeat = false;
				CLVColumn* LastColumn = (CLVColumn*)fColumnDisplayList.ItemAt(DisplayIndex);
				if(LastColumn->Flags() & CLV_LOCK_AT_BEGINNING)
				{
					DisplayIndex++;
					Repeat = true;
				}
			}
		}
		else
		{
			//Make sure it isn't after a CLV_LOCK_AT_END item
			Repeat = true;
			while(Repeat && DisplayIndex > 0)
			{
				Repeat = false;
				CLVColumn* LastColumn = (CLVColumn*)fColumnDisplayList.ItemAt(DisplayIndex-1);
				if(LastColumn->Flags() & CLV_LOCK_AT_END)
				{
					DisplayIndex--;
					Repeat = true;
				}
			}
		}
	}

	//Add the column to the display list in the appropriate position
	fColumnDisplayList.AddItem(Column, DisplayIndex);

	//Add the column to the end of the column list
	fColumnList.AddItem(Column);

	//Tell the column it belongs to me now
	Column->fParent = this;

	//Set the scroll bars and tell views to update
	ColumnsChanged();
	return true;
}


bool ColumnListView::AddColumnList(BList* NewColumns)
//Adds a BList of CLVColumn's to the ColumnListView at the position specified, or at the end of the list
//if AtIndex == -1.  Returns true if successful.
{
	int32 NumberOfColumns = int32(fColumnList.CountItems());
	int32 NumberOfColumnsToAdd = int32(NewColumns->CountItems());

	//Make sure a second CLVExpander is not being added
	int32 Counter;
	int32 NumberOfExpanders = 0;
	for(Counter = 0; Counter < NumberOfColumns; Counter++)
		if(((CLVColumn*)fColumnList.ItemAt(Counter))->fFlags & CLV_EXPANDER)
			NumberOfExpanders++;
	int32 SetfExpanderColumnTo = -1;
	for(Counter = 0; Counter < NumberOfColumnsToAdd; Counter++)
	{
		CLVColumn* ThisColumn = (CLVColumn*)NewColumns->ItemAt(Counter);
		if(ThisColumn->fFlags & CLV_EXPANDER)
		{
			NumberOfExpanders++;
			if(ThisColumn->IsShown())
				SetfExpanderColumnTo = NumberOfColumns + Counter;
		}
	}
	if(NumberOfExpanders != 0 && !fHierarchical)
		return false;
	if(NumberOfExpanders > 1)
		return false;
	if(SetfExpanderColumnTo != -1)
		fExpanderColumn = SetfExpanderColumnTo;

	//Make sure none of these columns have already been added to a ColumnListView
	for(Counter = 0; Counter < NumberOfColumnsToAdd; Counter++)
		if(((CLVColumn*)NewColumns->ItemAt(Counter))->fParent != NULL)
			return false;
	//Make sure none of these columns are being added twice
	for(Counter = 0; Counter < NumberOfColumnsToAdd-1; Counter++)
		for(int32 Counter2 = Counter+1; Counter2 < NumberOfColumnsToAdd; Counter2++)
			if(NewColumns->ItemAt(Counter) == NewColumns->ItemAt(Counter2))
				return false;

	for(Counter = 0; Counter < NumberOfColumnsToAdd; Counter++)
	{
		CLVColumn* Column = (CLVColumn*)NewColumns->ItemAt(Counter);
		//Check if this should be locked at the beginning or end, and adjust its position if necessary
		int32 DisplayIndex = NumberOfColumns;
		if(!Column->Flags() & CLV_LOCK_AT_END)
		{
			bool Repeat;
			if(Column->Flags() & CLV_LOCK_AT_BEGINNING)
			{
				//Move it to the beginning, after the last CLV_LOCK_AT_BEGINNING item
				DisplayIndex = 0;
				Repeat = true;
				while(Repeat && DisplayIndex < NumberOfColumns)
				{
					Repeat = false;
					CLVColumn* LastColumn = (CLVColumn*)fColumnDisplayList.ItemAt(DisplayIndex);
					if(LastColumn->Flags() & CLV_LOCK_AT_BEGINNING)
					{
						DisplayIndex++;
						Repeat = true;
					}
				}
			}
			else
			{
				//Make sure it isn't after a CLV_LOCK_AT_END item
				Repeat = true;
				while(Repeat && DisplayIndex > 0)
				{
					Repeat = false;
					CLVColumn* LastColumn = (CLVColumn*)fColumnDisplayList.ItemAt(DisplayIndex-1);
					if(LastColumn->Flags() & CLV_LOCK_AT_END)
					{
						DisplayIndex--;
						Repeat = true;
					}
				}
			}
		}

		//Add the column to the display list in the appropriate position
		fColumnDisplayList.AddItem(Column, DisplayIndex);

		//Tell the column it belongs to me now
		Column->fParent = this;

		NumberOfColumns++;
	}

	//Add the columns to the end of the column list
	fColumnList.AddList(NewColumns);

	//Set the scroll bars and tell views to update
	ColumnsChanged();
	return true;
}


bool ColumnListView::RemoveColumn(CLVColumn* Column)
//Removes a CLVColumn from the ColumnListView.  Returns true if successful.
{
	if(!fColumnList.HasItem(Column))
		return false;
	int32 ColumnIndex = fSortKeyList.IndexOf(Column);
	if(ColumnIndex >= 0)
		fSortKeyList.RemoveItem(ColumnIndex);
		
	if(Column->fFlags & CLV_EXPANDER)
		fExpanderColumn = -1;

	//Remove Column from the column and display lists
	fColumnDisplayList.RemoveItem(Column);
	fColumnList.RemoveItem(Column);

	//Tell the column it has been removed
	Column->fParent = NULL;

	//Set the scroll bars and tell views to update
	ColumnsChanged();
	return true;
}


bool ColumnListView::RemoveColumns(CLVColumn* Column, int32 Count)
//Finds Column in ColumnList and removes Count columns and their data from the view and its items
{
	int32 ColumnIndex = fColumnList.IndexOf(Column);
	if(ColumnIndex < 0)
		return false;
	if(ColumnIndex + Count > fColumnList.CountItems())
		return false;

	//Remove columns from the column and display lists
	for(int32 Counter = ColumnIndex; Counter < ColumnIndex+Count; Counter++)
	{
		CLVColumn* ThisColumn = (CLVColumn*)fColumnList.ItemAt(Counter);
		fColumnDisplayList.RemoveItem(ThisColumn);

		int32 SortIndex = fSortKeyList.IndexOf(Column);
		if(SortIndex >= 0)
			fSortKeyList.RemoveItem(SortIndex);

		if(ThisColumn->fFlags & CLV_EXPANDER)
			fExpanderColumn = -1;

		//Tell the column it has been removed
		ThisColumn->fParent = NULL;
	}
	fColumnList.RemoveItems(ColumnIndex,Count);

	//Set the scroll bars and tell views to update
	ColumnsChanged();
	return true;
}


int32 ColumnListView::CountColumns() const
{
	return fColumnList.CountItems();
}


int32 ColumnListView::IndexOfColumn(CLVColumn* column) const
{
	return fColumnList.IndexOf(column);
}


CLVColumn* ColumnListView::ColumnAt(int32 column_index) const
{
	return (CLVColumn*)fColumnList.ItemAt(column_index);
}


bool ColumnListView::SetDisplayOrder(const int32* ColumnOrder)
//Sets the display order using a BList of CLVColumn's
{
	//Add the items to the display list in order
	fColumnDisplayList.MakeEmpty();
	int32 ColumnsToSet = fColumnList.CountItems();
	for(int32 Counter = 0; Counter < ColumnsToSet; Counter++)
	{
		if(ColumnOrder[Counter] >= ColumnsToSet)
			return false;
		for(int32 Counter2 = 0; Counter2 < Counter; Counter2++)
			if(ColumnOrder[Counter] == ColumnOrder[Counter2])
				return false;
		fColumnDisplayList.AddItem(fColumnList.ItemAt(ColumnOrder[Counter]));
	}

	//Update everything about the columns
	ColumnsChanged();

	//Let the program know that the display order changed.
	DisplayOrderChanged(ColumnOrder);
	return true;
}

void ColumnListView::ColumnWidthChanged(int32 ColumnIndex, float NewWidth)
{
	CLVColumn* ThisColumn = (CLVColumn*)fColumnList.ItemAt(ColumnIndex);
	if(ThisColumn->fFlags & CLV_TELL_ITEMS_WIDTH)
	{
		//Figure out what the limit is for expanders pushing other columns
		float PushMax = 100000;
		if(ThisColumn->fPushedByExpander || (ThisColumn->fFlags & CLV_EXPANDER))
		{
			int32 NumberOfColumns = fColumnDisplayList.CountItems();
			for(int32 Counter = 0; Counter < NumberOfColumns; Counter++)
			{
				CLVColumn* SomeColumn = (CLVColumn*)fColumnDisplayList.ItemAt(Counter);
				if((SomeColumn->fFlags & CLV_EXPANDER) || SomeColumn->fPushedByExpander)
					PushMax = SomeColumn->fColumnEnd;
			}
		}

		int32 number_of_items;
		if(fHierarchical)
			number_of_items = fFullItemList.CountItems();
		else
			number_of_items = CountItems();
		float ColumnEnd = ThisColumn->fColumnEnd;
		for(int32 Counter = 0; Counter < number_of_items; Counter++)
		{
			CLVListItem* ThisItem;
			if(fHierarchical)
				ThisItem = (CLVListItem*)fFullItemList.ItemAt(Counter);
			else
				ThisItem = (CLVListItem*)ItemAt(Counter);
			if(ThisColumn->fPushedByExpander || (ThisColumn->fFlags & CLV_EXPANDER))
			{
				float ColumnWidth = NewWidth;
				float ExpanderShift = ThisItem->OutlineLevel() * 20.0;
				if(ThisColumn->fFlags & CLV_EXPANDER)
				{
					if(ColumnEnd + ExpanderShift > PushMax)
						ColumnWidth += PushMax - ColumnEnd;
					else
						ColumnWidth += ExpanderShift;
				}
				else
				{
					if(ColumnEnd + ExpanderShift > PushMax)
					{
						ColumnWidth -= (ColumnEnd + ExpanderShift - PushMax);
						if(ColumnWidth < 0)
							ColumnWidth = 0;
					}
				}
				if(ExpanderShift > 0)
				{
					int32 DisplayIndex = IndexOf(ThisItem);
					if(DisplayIndex >= 0)
					{
						//The column scrolling scrolled some content that is encroaching into the next
						//column and needs to be invalidated now
						BRect InvalidateRect = ItemFrame(DisplayIndex);
						InvalidateRect.left = ColumnEnd;
						InvalidateRect.right = ColumnEnd + ExpanderShift;
						if(InvalidateRect.right > PushMax)
							InvalidateRect.right = PushMax;
						if(InvalidateRect.right >= InvalidateRect.left)
						Invalidate(InvalidateRect);
					}
				}
				ThisItem->ColumnWidthChanged(ColumnIndex,ColumnWidth,this);
			}
			else
				ThisItem->ColumnWidthChanged(ColumnIndex,NewWidth,this);
		}
	}
}


void ColumnListView::DisplayOrderChanged(const int32* order)
{
	int num_columns = fColumnList.CountItems();

	//Figure out what the limit is for expanders pushing other columns
	float PushMax = 100000;
	CLVColumn* ThisColumn;
	if(fHierarchical)
		for(int32 Counter = 0; Counter < num_columns; Counter++)
		{
			ThisColumn = (CLVColumn*)fColumnDisplayList.ItemAt(Counter);
			if((ThisColumn->fFlags & CLV_EXPANDER) || ThisColumn->fPushedByExpander)
				PushMax = ThisColumn->fColumnEnd;
		}

	int32 number_of_items;
	if(fHierarchical)
		number_of_items = fFullItemList.CountItems();
	else
		number_of_items = CountItems();
	for(int column = 0; column < num_columns; column++)
	{
		ThisColumn = (CLVColumn*)fColumnList.ItemAt(column);
		if(ThisColumn->fFlags & CLV_TELL_ITEMS_WIDTH)
		{
			float ColumnLeft = ThisColumn->fColumnBegin;
			float ColumnRight = ThisColumn->fColumnEnd;
			for(int32 item_index = 0; item_index < number_of_items; item_index++)
			{
				CLVListItem* item;
				if(fHierarchical)
					item = (CLVListItem*)fFullItemList.ItemAt(item_index);
				else
					item = (CLVListItem*)ItemAt(item_index);
				int32 DisplayIndex = IndexOf(item);	
				if(DisplayIndex >= 0 && ThisColumn->IsShown())
				{
					BRect ThisColumnRect = ItemFrame(DisplayIndex);
					ThisColumnRect.left = ColumnLeft;
					ThisColumnRect.right = ColumnRight;
					if(ThisColumn->fFlags & CLV_EXPANDER)
					{
						ThisColumnRect.right += item->OutlineLevel() * 20.0;
						if(ThisColumnRect.right > PushMax)
							ThisColumnRect.right = PushMax;
					}
					else
					{
						if(ThisColumn->fPushedByExpander)
						{
							float Shift = item->OutlineLevel() * 20.0;
							ThisColumnRect.left += Shift;
							ThisColumnRect.right += Shift;
							if(Shift > 0.0 && ThisColumnRect.right > PushMax)
								ThisColumnRect.right = PushMax;
						}
					}
				
					if(ThisColumnRect.right >= ThisColumnRect.left)
						item->FrameChanged(column,ThisColumnRect,this);
					else
						item->FrameChanged(column,BRect(-1,-1,-1,-1),this);
				}
				else
					item->FrameChanged(column,BRect(-1,-1,-1,-1),this);
			}
		}
	}
	//Get rid of a warning:
	order = NULL;
}


void ColumnListView::GetDisplayOrder(int32* order) const
{
	int32 ColumnsInList = fColumnList.CountItems();
	for(int32 Counter = 0; Counter < ColumnsInList; Counter++)
		order[Counter] = int32(fColumnList.IndexOf(fColumnDisplayList.ItemAt(Counter)));
}


void ColumnListView::SetSortKey(int32 ColumnIndex, bool doSort)
{
	CLVColumn* Column;
	if(ColumnIndex >= 0)
	{
		Column = (CLVColumn*)fColumnList.ItemAt(ColumnIndex);
		if(!(Column->Flags()&CLV_SORT_KEYABLE))
			return;
	}
	else
		Column = NULL;
	if(fSortKeyList.ItemAt(0) != Column || Column == NULL)
	{
		BRect LabelBounds = fColumnLabelView->Bounds();
		//Need to remove old sort keys and erase all the old underlines
		int32 SortKeyCount = fSortKeyList.CountItems();
		for(int32 Counter = 0; Counter < SortKeyCount; Counter++)
		{
			CLVColumn* UnderlineColumn = (CLVColumn*)fSortKeyList.ItemAt(Counter);
			if(UnderlineColumn->fSortMode != NoSort)
				fColumnLabelView->Invalidate(BRect(UnderlineColumn->fColumnBegin,LabelBounds.top,
					UnderlineColumn->fColumnEnd,LabelBounds.bottom));
		}
		fSortKeyList.MakeEmpty();

		if(Column)
		{
			fSortKeyList.AddItem(Column);
			if(Column->fSortMode == NoSort)
				SetSortMode(ColumnIndex,Ascending);
			if(doSort)
			{
				SortItems();
				//Need to draw new underline
				fColumnLabelView->Invalidate(BRect(Column->fColumnBegin,LabelBounds.top,Column->fColumnEnd,
					LabelBounds.bottom));
			}
		}
	}

	SortingChanged();
}


void ColumnListView::AddSortKey(int32 ColumnIndex, bool doSort)
{
	CLVColumn* Column;
	if(ColumnIndex >= 0)
	{
		Column = (CLVColumn*)fColumnList.ItemAt(ColumnIndex);
		if(!(Column->Flags()&CLV_SORT_KEYABLE))
			return;
	}
	else
		Column = NULL;
	if(Column && !fSortKeyList.HasItem(Column))
	{
		BRect LabelBounds = fColumnLabelView->Bounds();
		fSortKeyList.AddItem(Column);
		if(Column->fSortMode == NoSort)
			SetSortMode(ColumnIndex,Ascending);
		if(doSort)
		{
			SortItems();
			//Need to draw new underline
			fColumnLabelView->Invalidate(BRect(Column->fColumnBegin,LabelBounds.top,Column->fColumnEnd,
				LabelBounds.bottom));
		}
	}

	SortingChanged();
}


void ColumnListView::SetSortMode(int32 ColumnIndex,CLVSortMode Mode, bool doSort)
{
	CLVColumn* Column;
	if(ColumnIndex >= 0)
	{
		Column = (CLVColumn*)fColumnList.ItemAt(ColumnIndex);
		if(!(Column->Flags()&CLV_SORT_KEYABLE))
			return;
	}
	else
		return;
	if(Column->fSortMode != Mode)
	{
		BRect LabelBounds = fColumnLabelView->Bounds();
		Column->fSortMode = Mode;
		if(Mode == NoSort && fSortKeyList.HasItem(Column))
			fSortKeyList.RemoveItem(Column);
		if(doSort)
		{
			SortItems();
			//Need to draw or erase underline
			fColumnLabelView->Invalidate(BRect(Column->fColumnBegin,LabelBounds.top,Column->fColumnEnd,
				LabelBounds.bottom));
		}
	}

	SortingChanged();
}


void ColumnListView::ReverseSortMode(int32 ColumnIndex, bool doSort)
{
	CLVColumn* Column;
	if(ColumnIndex >= 0)
	{
		Column = (CLVColumn*)fColumnList.ItemAt(ColumnIndex);
		if(!(Column->Flags()&CLV_SORT_KEYABLE))
			return;
	}
	else
		return;
	if(Column->fSortMode == Ascending)
		SetSortMode(ColumnIndex,Descending,doSort);
	else if(Column->fSortMode == Descending)
		SetSortMode(ColumnIndex,NoSort,doSort);
	else if(Column->fSortMode == NoSort)
		SetSortMode(ColumnIndex,Ascending,doSort);
}


int32 ColumnListView::GetSorting(int32* SortKeys, CLVSortMode* SortModes) const
{
	int32 NumberOfKeys = fSortKeyList.CountItems();
	for(int32 Counter = 0; Counter < NumberOfKeys; Counter++)
	{
		CLVColumn* Column = (CLVColumn*)fSortKeyList.ItemAt(Counter);
		SortKeys[Counter] = IndexOfColumn(Column);
		SortModes[Counter] = Column->SortMode();
	}
	return NumberOfKeys;
}


void ColumnListView::SetSorting(int32 NumberOfKeys, int32* SortKeys, CLVSortMode* SortModes, bool doSort)
{

	//Need to remove old sort keys and erase all the old underlines
	BRect LabelBounds = fColumnLabelView->Bounds();
	int32 SortKeyCount = fSortKeyList.CountItems();
	for(int32 Counter = 0; Counter < SortKeyCount; Counter++)
	{
		CLVColumn* UnderlineColumn = (CLVColumn*)fSortKeyList.ItemAt(Counter);
		if(UnderlineColumn->fSortMode != NoSort)
			fColumnLabelView->Invalidate(BRect(UnderlineColumn->fColumnBegin,LabelBounds.top,
				UnderlineColumn->fColumnEnd,LabelBounds.bottom));
	}
	fSortKeyList.MakeEmpty();

	for(int32 Counter = 0; Counter < NumberOfKeys; Counter++)
	{
		if(Counter == 0)
			SetSortKey(SortKeys[0],false);
		else
			AddSortKey(SortKeys[Counter],false);
		SetSortMode(SortKeys[Counter],SortModes[Counter],doSort && Counter == NumberOfKeys-1);
	}

	SortingChanged();
}


void ColumnListView::SortingChanged()
{ }


void ColumnListView::FrameResized(float width, float height)
{
	UpdateColumnSizesDataRectSizeScrollBars();
	BListView::FrameResized(width,height);
}


void ColumnListView::ScrollTo(BPoint point)
{
	BListView::ScrollTo(point);
	fColumnLabelView->ScrollTo(BPoint(point.x,0.0));
}


void ColumnListView::MouseDown(BPoint where)
{
	//BListView::MouseDown gives undesirable behavior during click-and drag (it reselects instead of
	//doing nothing to allow for initiating drag & drop), so I need to replace the selection behavior,
	//and while I'm at it, I'll just do an InitiateDrag hook function.
	BMessage* message = Window()->CurrentMessage();
	int32 item_index = IndexOf(where);
	int32 modifier_keys = message->FindInt32("modifiers");
	int32 clicks = message->FindInt32("clicks");
	if(message->FindInt32("buttons") != B_PRIMARY_MOUSE_BUTTON)
		return;
	if(item_index >= 0)
	{
		CLVListItem* clicked_item = (CLVListItem*)BListView::ItemAt(item_index);
		if(clicked_item->IsSuperItem() && clicked_item->ExpanderRectContains(where))
		{
			if(clicked_item->IsExpanded())
				Collapse(clicked_item);
			else
				Expand(clicked_item);
		}
		else
		{
			if(!clicked_item->IsSelected())
			{
				//Clicked a new item...select it.
				list_view_type type = ListType();
				if((modifier_keys & B_SHIFT_KEY) && type != B_SINGLE_SELECTION_LIST)
				{
					//If shift held down, expand the selection to include all intervening items.
					int32 min_selection = 0x7FFFFFFF;
					int32 max_selection = -1;
					int32 selection_index = 0;
					int32 selection_item_index;
					while((selection_item_index = CurrentSelection(selection_index)) != -1)
					{
						if(min_selection > selection_item_index)
							min_selection = selection_item_index;
						if(max_selection < selection_item_index)
							max_selection = selection_item_index;
						selection_index++;
					}
					if(min_selection == 0x7FFFFFFF)
						Select(item_index,false);
					else
					{
						if(min_selection > item_index)
							min_selection = item_index;
						if(max_selection < item_index)
							max_selection = item_index;
						Select(min_selection,max_selection,false);
					}
				}
				else if((modifier_keys & B_OPTION_KEY) && type != B_SINGLE_SELECTION_LIST)
					//If option held down, expand the selection to include just it.
					Select(item_index,true);
				else
					//If neither key held down, select this item alone.
					Select(item_index,false);

				//Also watch for drag of the new selection
				fLastMouseDown = where;
				fNoKeyMouseDownItemIndex = -1;
				fWatchingForDrag = true;
			}
			else
			{
				//Clicked an already selected item...
				if(modifier_keys & B_OPTION_KEY)
					//if option held down, remove it.
					Deselect(item_index);
				else if(modifier_keys & B_SHIFT_KEY)
				{
					//If shift held down, ignore it and just watch for drag.
					fLastMouseDown = where;
					fNoKeyMouseDownItemIndex = -1;
					fWatchingForDrag = true;
				}
				else
				{
					if(clicks > 1)
						Invoke(InvocationMessage());
					else
					{
						//If neither key held down, watch for drag, but if no drag and just a click on
						//this item, select it alone at mouse up.
						fLastMouseDown = where;
						fNoKeyMouseDownItemIndex = item_index;
						fWatchingForDrag = true;
					}
				}
			}
			if(fWatchingForDrag)
				SetMouseEventMask(B_POINTER_EVENTS,B_NO_POINTER_HISTORY);
		}
	}
	else
	{
		//Clicked outside of any items.  If no shift or option key, deselect all.
		if((!(modifier_keys & B_SHIFT_KEY)) && (!(modifier_keys & B_OPTION_KEY)))
			DeselectAll();
	}
}


void ColumnListView::MouseUp(BPoint where)
{
	if(fWatchingForDrag && fNoKeyMouseDownItemIndex != -1)
		Select(fNoKeyMouseDownItemIndex,false);
	fNoKeyMouseDownItemIndex = -1;
	fWatchingForDrag = false;
}


void ColumnListView::MouseMoved(BPoint where, uint32 code, const BMessage *message)
{
	if(fWatchingForDrag && (where.x<fLastMouseDown.x-4 || where.x>fLastMouseDown.x+4 ||
		where.y<fLastMouseDown.y-4 || where.y>fLastMouseDown.y+4))
	{
		InitiateDrag(fLastMouseDown,IndexOf(fLastMouseDown),true);
		fNoKeyMouseDownItemIndex = -1;
		fWatchingForDrag = false;
		SetMouseEventMask(0);
	}
}


void ColumnListView::WindowActivated(bool active)
{
	fWindowActive = active;
	if(fSelectedItemColorWindowActive.red != fSelectedItemColorWindowInactive.red ||
		fSelectedItemColorWindowActive.green != fSelectedItemColorWindowInactive.green ||
		fSelectedItemColorWindowActive.blue != fSelectedItemColorWindowInactive.blue ||
		fSelectedItemColorWindowActive.alpha != fSelectedItemColorWindowInactive.alpha)
	{
		int32 index=0;
		int32 selected;
		while((selected = CurrentSelection(index)) >= 0)
		{
			InvalidateItem(selected);
			index++;
		}
	}
	BListView::WindowActivated(active);
}


bool ColumnListView::AddUnder(BListItem* a_item, BListItem* a_superitem)
{
	if(!fHierarchical)
		return false;

	//Get the CLVListItems
	CLVListItem* item = cast_as(a_item,CLVListItem);
	CLVListItem* superitem cast_as(a_superitem,CLVListItem);
	if(item == NULL || superitem == NULL)
		return false;

	//Find the superitem in the full list and display list (if shown)
	int32 SuperItemPos = fFullItemList.IndexOf(superitem);
	if(SuperItemPos < 0)
		return false;
	uint32 SuperItemLevel = superitem->fOutlineLevel;

	//Add the item under the superitem in the full list
	int32 ItemPos = SuperItemPos + 1;
	item->fOutlineLevel = SuperItemLevel + 1;
	while(true)
	{
		CLVListItem* Temp = (CLVListItem*)fFullItemList.ItemAt(ItemPos);
		if(Temp)
		{
			if(Temp->fOutlineLevel > SuperItemLevel)
				ItemPos++;
			else
				break;
		}
		else
			break;
	}
	return AddItemPrivate(item,ItemPos);
}


bool ColumnListView::AddItem(BListItem* a_item, int32 fullListIndex)
{
	//Get the CLVListItems
	CLVListItem* item = cast_as(a_item,CLVListItem);
	if(item == NULL)
		return false;
	return AddItemPrivate(item,fullListIndex);
}


bool ColumnListView::AddItem(BListItem* a_item)
{
	//Get the CLVListItems
	CLVListItem* item = cast_as(a_item,CLVListItem);
	if(item == NULL)
		return false;
	if(fHierarchical)
		return AddItemPrivate(item,fFullItemList.CountItems());
	else
		return AddItemPrivate(item,CountItems());
}


bool ColumnListView::AddItemPrivate(CLVListItem* item, int32 fullListIndex)
{
	if(fHierarchical)
	{
		uint32 ItemLevel = item->OutlineLevel();

		//Figure out whether it is visible (should it be added to visible list)
		bool Visible = true;

		//Find the item that contains it in the full list
		int32 SuperItemPos;
		if(ItemLevel == 0)
			SuperItemPos = -1;
		else
			SuperItemPos = fullListIndex - 1;
		CLVListItem* SuperItem;
		while(SuperItemPos >= 0)
		{
			SuperItem = (CLVListItem*)fFullItemList.ItemAt(SuperItemPos);
			if(SuperItem)
			{
				if(SuperItem->fOutlineLevel >= ItemLevel)
					SuperItemPos--;
				else
					break;
			}
			else
				return false;
		}
		if(SuperItemPos >= 0 && SuperItem)
		{
			if(!SuperItem->IsExpanded())
				//SuperItem's contents aren't visible
				Visible = false;
			if(!HasItem(SuperItem))
				//SuperItem itself isn't showing
				Visible = false;
		}

		//Add the item to the full list
		if(!fFullItemList.AddItem(item,fullListIndex))
			return false;
		else
		{
			//Add the item to the display list
			if(Visible)
			{
				//Find the previous item, or -1 if the item I'm adding will be the first one
				int32 PreviousItemPos = fullListIndex - 1;
				CLVListItem* PreviousItem;
				while(PreviousItemPos >= 0)
				{
					PreviousItem = (CLVListItem*)fFullItemList.ItemAt(PreviousItemPos);
					if(PreviousItem && HasItem(PreviousItem))
						break;
					else
						PreviousItemPos--;
				}

				//Add the item after the previous item, or first on the list
				bool Result;
				if(PreviousItemPos >= 0)
					Result = BListView::AddItem((BListItem*)item,IndexOf(PreviousItem)+1);
				else
					Result = BListView::AddItem((BListItem*)item,0);
				if(Result == false)
					fFullItemList.RemoveItem(item);
				return Result;
			}
			return true;
		}
	}
	else
		return BListView::AddItem(item,fullListIndex);
}


bool ColumnListView::AddList(BList* newItems)
{
	if(fHierarchical)
		return AddHierarchicalListPrivate(newItems,fFullItemList.CountItems());
	else
		return BListView::AddList(newItems);
}


bool ColumnListView::AddList(BList* newItems, int32 fullListIndex)
{
	if(fHierarchical)
		return AddHierarchicalListPrivate(newItems,fullListIndex);
	else
		return BListView::AddList(newItems,fullListIndex);
}


bool ColumnListView::EfficientAddListHierarchical(BList* newItems, int32 fullListIndex, bool expanded)
{
	if(!fHierarchical)
		return false;
	if(expanded)
	{
		int32 index = 0;
		if(fullListIndex > 0)
			index = IndexOf((CLVListItem*)fFullItemList.ItemAt(fullListIndex-1))+1;
		if(!BListView::AddList(newItems,index))
			return false;
	}
	int32 numItems = newItems->CountItems();
	void** items = (void**)newItems->Items();
	for(int32 i=0; i<numItems; i++)
		items[i] = (void*)(CLVListItem*)(BListItem*)items[i];	//Convert to CLVListItems
	bool result = fFullItemList.AddList(newItems,fullListIndex);
	for(int32 i=0; i<numItems; i++)
		items[i] = (void*)(BListItem*)(CLVListItem*)items[i];	//Convert back to BListItems
	return result;
}


bool ColumnListView::AddHierarchicalListPrivate(BList* newItems, int32 fullListIndex)
{
	int32 NumberOfItems = newItems->CountItems();
	for(int32 count = 0; count < NumberOfItems; count++)
		if(!AddItemPrivate((CLVListItem*)(BListItem*)newItems->ItemAt(count),fullListIndex+count))
			return false;
	return true;
}


bool ColumnListView::RemoveItem(BListItem* a_item)
{
	//Get the CLVListItems
	CLVListItem* item = cast_as(a_item,CLVListItem);
	if(item == NULL)
		return false;
	if(fHierarchical)
	{
		if(!fFullItemList.HasItem(item))
			return false;
		int32 ItemsToRemove = 1 + FullListNumberOfSubitems(item);
		return RemoveItems(fFullItemList.IndexOf(item),ItemsToRemove);
	}
	else
		return BListView::RemoveItem((BListItem*)item);
}


BListItem* ColumnListView::RemoveItem(int32 fullListIndex)
{
	if(fHierarchical)
	{
		CLVListItem* TheItem = (CLVListItem*)fFullItemList.ItemAt(fullListIndex);
		if(TheItem)
		{
			int32 ItemsToRemove = 1 + FullListNumberOfSubitems(TheItem);
			if(RemoveItems(fullListIndex,ItemsToRemove))
				return TheItem;
			else
				return NULL;
		}
		else
			return NULL;
	}
	else
		return BListView::RemoveItem(fullListIndex);
}


bool ColumnListView::RemoveItems(int32 fullListIndex, int32 count)
{
	CLVListItem* TheItem;
	if(fHierarchical)
	{
		uint32 LastSuperItemLevel = ULONG_MAX;
		int32 Counter;
		int32 DisplayItemsToRemove = 0;
		int32 FirstDisplayItemToRemove = -1;
		for(Counter = fullListIndex; Counter < fullListIndex+count; Counter++)
		{
			TheItem = FullListItemAt(Counter);
			if(TheItem->fOutlineLevel < LastSuperItemLevel)
				LastSuperItemLevel = TheItem->fOutlineLevel;
			if(BListView::HasItem((BListItem*)TheItem))
			{
				DisplayItemsToRemove++;
				if(FirstDisplayItemToRemove == -1)
					FirstDisplayItemToRemove = BListView::IndexOf(TheItem);
			}
		}
		while(true)
		{
			TheItem = FullListItemAt(Counter);
			if(TheItem && TheItem->fOutlineLevel > LastSuperItemLevel)
			{
				count++;
				Counter++;
				if(BListView::HasItem((BListItem*)TheItem))
				{
					DisplayItemsToRemove++;
					if(FirstDisplayItemToRemove == -1)
						FirstDisplayItemToRemove = BListView::IndexOf((BListItem*)TheItem);
				}
			}
			else
				break;
		}
		while(DisplayItemsToRemove > 0)
		{
			if(BListView::RemoveItem(FirstDisplayItemToRemove) == NULL)
				return false;
			DisplayItemsToRemove--;
		}
		return fFullItemList.RemoveItems(fullListIndex,count);
	}
	else
		return BListView::RemoveItems(fullListIndex,count);
}


CLVListItem* ColumnListView::FullListItemAt(int32 fullListIndex) const
{
	return (CLVListItem*)fFullItemList.ItemAt(fullListIndex);
}


int32 ColumnListView::FullListIndexOf(const CLVListItem* item) const
{
	return fFullItemList.IndexOf((CLVListItem*)item);
}


int32 ColumnListView::FullListIndexOf(BPoint point) const
{
	int32 DisplayListIndex = IndexOf(point);
	CLVListItem* TheItem = (CLVListItem*)ItemAt(DisplayListIndex);
	if(TheItem)
		return FullListIndexOf(TheItem);
	else
		return -1;
}


CLVListItem* ColumnListView::FullListFirstItem() const
{
	return (CLVListItem*)fFullItemList.FirstItem();
}


CLVListItem* ColumnListView::FullListLastItem() const
{
	return (CLVListItem*)fFullItemList.LastItem();
}


bool ColumnListView::FullListHasItem(const CLVListItem* item) const
{
	return fFullItemList.HasItem((CLVListItem*)item);
}


int32 ColumnListView::FullListCountItems() const
{
	return fFullItemList.CountItems();
}


void ColumnListView::MakeEmpty()
{
	fFullItemList.MakeEmpty();
	BListView::MakeEmpty();
}


void ColumnListView::MakeEmptyPrivate()
{
	fFullItemList.MakeEmpty();
	BListView::MakeEmpty();
}


bool ColumnListView::FullListIsEmpty() const
{
	return fFullItemList.IsEmpty();
}


int32 ColumnListView::FullListCurrentSelection(int32 index) const
{
	int32 Selection = CurrentSelection(index);
	CLVListItem* SelectedItem = (CLVListItem*)ItemAt(Selection);
	return FullListIndexOf(SelectedItem);
}


void ColumnListView::FullListDoForEach(bool (*func)(CLVListItem*))
{
	int32 NumberOfItems = fFullItemList.CountItems();
	for(int32 Counter = 0; Counter < NumberOfItems; Counter++)
		if(func((CLVListItem*)fFullItemList.ItemAt(Counter)) == true)
			return;
}


void ColumnListView::FullListDoForEach(bool (*func)(CLVListItem*, void*), void* arg2)
{
	int32 NumberOfItems = fFullItemList.CountItems();
	for(int32 Counter = 0; Counter < NumberOfItems; Counter++)
		if(func((CLVListItem*)fFullItemList.ItemAt(Counter),arg2) == true)
			return;
}


CLVListItem* ColumnListView::Superitem(const CLVListItem* item) const
{
	int32 SuperItemPos;
	uint32 ItemLevel = item->fOutlineLevel;
	if(ItemLevel == 0)
		SuperItemPos = -1;
	else
		SuperItemPos = fFullItemList.IndexOf((CLVListItem*)item) - 1;
	CLVListItem* SuperItem;
	while(SuperItemPos >= 0)
	{
		SuperItem = (CLVListItem*)fFullItemList.ItemAt(SuperItemPos);
		if(SuperItem)
		{
			if(SuperItem->fOutlineLevel >= ItemLevel)
				SuperItemPos--;
			else
				break;
		}
		else
			return NULL;
	}
	if(SuperItemPos >= 0)
		return SuperItem;
	else
		return NULL;
}


int32 ColumnListView::FullListNumberOfSubitems(const CLVListItem* item) const
{
	if(!fHierarchical)
		return 0;
	int32 ItemPos = FullListIndexOf(item);
	int32 SubItemPos;
	uint32 SuperItemLevel = item->fOutlineLevel;
	if(ItemPos >= 0)
	{
		for(SubItemPos = ItemPos + 1; SubItemPos >= 1; SubItemPos++)
		{
			CLVListItem* TheItem = FullListItemAt(SubItemPos);
			if(TheItem == NULL || TheItem->fOutlineLevel <= SuperItemLevel)
				break;
		}
	}
	else
		return 0;
	return SubItemPos-ItemPos-1;
}


void ColumnListView::Expand(CLVListItem* item)
{
	if(!(item->fSuperItem))
		item->fSuperItem = true;
	if(item->IsExpanded())
		return;
	item->SetExpanded(true);
	if(!fHierarchical)
		return;

	int32 DisplayIndex = IndexOf(item);
	if(DisplayIndex >= 0)
	{
		if(fExpanderColumn >= 0)
		{
			//Change the state of the arrow
			item->DrawItemColumn(this,item->fExpanderColumnRect,fExpanderColumn,true);
			SetDrawingMode(B_OP_OVER);
			DrawBitmap(&fDownArrow, BRect(0.0,0.0,item->fExpanderButtonRect.right-
				item->fExpanderButtonRect.left,10.0),item->fExpanderButtonRect);
			SetDrawingMode(B_OP_COPY);
		}

		//Add the items under it
		int32 FullListIndex = fFullItemList.IndexOf(item);
		uint32 ItemLevel = item->fOutlineLevel;
		int32 Counter = FullListIndex + 1;
		int32 AddPos = DisplayIndex + 1;
		while(true)
		{
			CLVListItem* NextItem = (CLVListItem*)fFullItemList.ItemAt(Counter);
			if(NextItem == NULL)
				break;
			if(NextItem->fOutlineLevel > ItemLevel)
			{
				BListView::AddItem((BListItem*)NextItem,AddPos++);
				if(NextItem->fSuperItem && !NextItem->IsExpanded())
				{
					//The item I just added is collapsed, so skip all its children
					uint32 SkipLevel = NextItem->fOutlineLevel + 1;
					while(true)
					{
						Counter++;
						NextItem = (CLVListItem*)fFullItemList.ItemAt(Counter);
						if(NextItem == NULL)
							break;
						if(NextItem->fOutlineLevel < SkipLevel)
							break;
					}
				}
				else
					Counter++;
			}
			else
				break;
		}
	}
}


void ColumnListView::Collapse(CLVListItem* item)
{
	if(!(item->fSuperItem))
		item->fSuperItem = true;
	if(!(item->IsExpanded()))
		return;
	item->SetExpanded(false);
	if(!fHierarchical)
		return;

	int32 DisplayIndex = IndexOf((BListItem*)item);
	if(DisplayIndex >= 0)
	{
		if(fExpanderColumn >= 0)
		{
			//Change the state of the arrow
			item->DrawItemColumn(this,item->fExpanderColumnRect,fExpanderColumn,true);
			SetDrawingMode(B_OP_OVER);
			DrawBitmap(&fRightArrow, BRect(0.0,0.0,item->fExpanderButtonRect.right-
				item->fExpanderButtonRect.left,10.0),item->fExpanderButtonRect);
			SetDrawingMode(B_OP_COPY);
		}

		//Remove the items under it
		uint32 ItemLevel = item->fOutlineLevel;
		int32 NextItemIndex = DisplayIndex+1;
		while(true)
		{
			CLVListItem* NextItem = (CLVListItem*)ItemAt(NextItemIndex);
			if(NextItem)
			{
				if(NextItem->fOutlineLevel > ItemLevel)
					BListView::RemoveItem(NextItemIndex);
				else
					break;
			}
			else
				break;
		}
	}
}


bool ColumnListView::IsExpanded(int32 fullListIndex) const
{
	BListItem* TheItem = (BListItem*)fFullItemList.ItemAt(fullListIndex);
	if(TheItem)
		return TheItem->IsExpanded();
	else
		return false;
}


void ColumnListView::SetSortFunction(CLVCompareFuncPtr compare)
{
	fCompare = compare;
}


void ColumnListView::SortItems()
{
	int32 NumberOfItems;
	if(!fHierarchical)
		NumberOfItems = CountItems();
	else
		NumberOfItems = fFullItemList.CountItems();
	if(NumberOfItems == 0)
		return;
	int32 Counter;
	if(!fHierarchical)
	{
		//Plain sort
		//Remember the list context for each item
		for(Counter = 0; Counter < NumberOfItems; Counter++)
			((CLVListItem*)ItemAt(Counter))->fSortingContextCLV = this;
		//Do the actual sort
		BListView::SortItems((int (*)(const void*, const void*))ColumnListView::PlainBListSortFunc);
	}
	else
	{
		//Block-by-block sort
		BList NewList;
		SortFullListSegment(0,0,&NewList);
		fFullItemList = NewList;
		//Remember the list context for each item
		for(Counter = 0; Counter < NumberOfItems; Counter++)
			((CLVListItem*)fFullItemList.ItemAt(Counter))->fSortingContextBList = &fFullItemList;
		//Do the actual sort
		BListView::SortItems((int (*)(const void*, const void*))ColumnListView::HierarchicalBListSortFunc);
	}
}


int ColumnListView::PlainBListSortFunc(BListItem** a_item1, BListItem** a_item2)
{
	CLVListItem* item1 = (CLVListItem*)*a_item1;
	CLVListItem* item2 = (CLVListItem*)*a_item2;
	ColumnListView* SortingContext = item1->fSortingContextCLV;
	int32 SortDepth = SortingContext->fSortKeyList.CountItems();
	int CompareResult = 0;
	if(SortingContext->fCompare)
		for(int32 SortIteration = 0; SortIteration < SortDepth && CompareResult == 0; SortIteration++)
		{
			CLVColumn* Column = (CLVColumn*)SortingContext->fSortKeyList.ItemAt(SortIteration);
			CompareResult = SortingContext->fCompare(item1,item2,SortingContext->fColumnList.IndexOf(Column));
			if(Column->fSortMode == Descending)
				CompareResult = 0-CompareResult;
		}
	return CompareResult;
}


int ColumnListView::HierarchicalBListSortFunc(BListItem** a_item1, BListItem** a_item2)
{
	CLVListItem* item1 = (CLVListItem*)*a_item1;
	CLVListItem* item2 = (CLVListItem*)*a_item2;
	if(item1->fSortingContextBList->IndexOf(item1) < item1->fSortingContextBList->IndexOf(item2))
		return -1;
	else
		return 1;
}


void ColumnListView::SortFullListSegment(int32 OriginalListStartIndex, int32 InsertionPoint,
	BList* NewList)
{
	//Identify and sort the items at this level
	BList* ItemsInThisLevel = SortItemsInThisLevel(OriginalListStartIndex);
	int32 NewItemsStopIndex = InsertionPoint + ItemsInThisLevel->CountItems();
	NewList->AddList(ItemsInThisLevel,InsertionPoint);
	delete ItemsInThisLevel;

	//Identify and sort the subitems
	for(int32 Counter = InsertionPoint; Counter < NewItemsStopIndex; Counter++)
	{
		CLVListItem* ThisItem = (CLVListItem*)NewList->ItemAt(Counter);
		CLVListItem* NextItem = (CLVListItem*)fFullItemList.ItemAt(fFullItemList.IndexOf(ThisItem)+1);
		if(ThisItem->IsSuperItem() && NextItem && ThisItem->fOutlineLevel < NextItem->fOutlineLevel)
		{
			int32 OldListSize = NewList->CountItems();
			SortFullListSegment(fFullItemList.IndexOf(ThisItem)+1,Counter+1,NewList);
			int32 NewListSize = NewList->CountItems();
			NewItemsStopIndex += NewListSize - OldListSize;
			Counter += NewListSize - OldListSize;
		}
	}
}


BList* ColumnListView::SortItemsInThisLevel(int32 OriginalListStartIndex)
{
	uint32 ThisLevel = ((CLVListItem*)fFullItemList.ItemAt(OriginalListStartIndex))->fOutlineLevel;

	//Create a new BList of the items in this level
	int32 Counter = OriginalListStartIndex;
	int32 ItemsInThisLevel = 0;
	BList* ThisLevelItems = new BList(16);
	while(true)
	{
		CLVListItem* ThisItem = (CLVListItem*)fFullItemList.ItemAt(Counter);
		if(ThisItem == NULL)
			break;
		uint32 ThisItemLevel = ThisItem->fOutlineLevel;
		if(ThisItemLevel == ThisLevel)
		{
			ThisLevelItems->AddItem(ThisItem);
			ItemsInThisLevel++;
		}
		else if(ThisItemLevel < ThisLevel)
			break;
		Counter++;
	}

	//Sort the BList of the items in this level
	CLVListItem** SortArray = new CLVListItem*[ItemsInThisLevel];
	CLVListItem** ListItems = (CLVListItem**)ThisLevelItems->Items();
	for(Counter = 0; Counter < ItemsInThisLevel; Counter++)
		SortArray[Counter] = ListItems[Counter];
	SortListArray(SortArray,ItemsInThisLevel);
	for(Counter = 0; Counter < ItemsInThisLevel; Counter++)
		ListItems[Counter] = SortArray[Counter];
	delete[] SortArray;
	return ThisLevelItems;
}


void ColumnListView::SortListArray(CLVListItem** SortArray, int32 NumberOfItems)
{
	if(fCompare == NULL)
		//No sorting function
		return;
	int32 SortDepth = fSortKeyList.CountItems();
	for(int32 Counter1 = 0; Counter1 < NumberOfItems-1; Counter1++)
		for(int32 Counter2 = Counter1+1; Counter2 < NumberOfItems; Counter2++)
		{
			int CompareResult = 0;
			for(int32 SortIteration = 0; SortIteration < SortDepth && CompareResult == 0; SortIteration++)
			{
				CLVColumn* Column = (CLVColumn*)fSortKeyList.ItemAt(SortIteration);
				CompareResult = fCompare(SortArray[Counter1],SortArray[Counter2],fColumnList.IndexOf(Column));
				if(CompareResult > 0)
					CompareResult = 1;
				else if(CompareResult < 0)
					CompareResult = -1;
				if(Column->fSortMode == Descending)
					CompareResult = 0-CompareResult;
			}
			if(CompareResult == 1)
			{
				CLVListItem* Temp = SortArray[Counter1];
				SortArray[Counter1] = SortArray[Counter2];
				SortArray[Counter2] = Temp;
			}
		}
}

