/*
 * Copyright 1999-2009 Jeremy Friesner
 * Copyright 2009-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 *		John Scipione, jscipione@gmail.com
 */


#define ColumnListView_CPP

#include "ColumnListView.h"
#include "CLVColumnLabelView.h"
#include "CLVColumn.h"
#include "CLVListItem.h"

#include <stdio.h>
#include <interface/Rect.h>


uint8 CLVRightArrowData[132] = {
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

uint8 CLVDownArrowData[132] = {
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


class CLVContainerView : public BScrollView
{
public:
							CLVContainerView(char* name, BView* target,
								uint32 resizingMode, uint32 flags,
								bool horizontal, bool vertical,
								border_style border);
	virtual					~CLVContainerView();

			bool			IsBeingDestroyed() const
								{ return fIsBeingDestroyed; };

private:
			bool			fIsBeingDestroyed;
};


CLVContainerView::CLVContainerView(char* name, BView* target,
	uint32 resizingMode, uint32 flags, bool horizontal, bool vertical,
	border_style border)
	:
	BScrollView(name, target, resizingMode, flags, horizontal, vertical,
		border)
{
	fIsBeingDestroyed = false;
}


CLVContainerView::~CLVContainerView()
{
	fIsBeingDestroyed = true;
}


ColumnListView::ColumnListView(BRect frame, BScrollView** containerView,
	const char* name, uint32 resizingMode, uint32 flags, list_view_type type,
	bool hierarchical, bool horizontal, bool vertical, border_style border,
	const BFont* labelFont)
	:
	BListView(frame, name, type, B_FOLLOW_ALL_SIDES, flags | B_PULSE_NEEDED),
	fHierarchical(hierarchical),
	fColumnList(6),
	fColumnDisplayList(6),
	fDataWidth(0),
	fDataHeight(0),
	fPageWidth(0),
	fPageHeight(0),
	fSortKeyList(6),
	fRightArrow(BRect(0, 0, 10, 10), B_RGBA32, CLVRightArrowData, false, false),
	fDownArrow(BRect(0, 0, 10, 10), B_RGBA32, CLVDownArrowData, false, false),
	fFullItemList(32),
	fSelectedColumn(-1),
	fEditMessage(NULL)
{
	// create the column titles bar view
	font_height fontAttributes;
	labelFont->GetHeight(&fontAttributes);
	float fLabelFontHeight = ceil(fontAttributes.ascent)
		+ ceil(fontAttributes.descent);
	float columnLabelViewBottom = frame.top + 1 + fLabelFontHeight + 3;
	fColumnLabelView = new CLVColumnLabelView(BRect(frame.left, frame.top,
		frame.right, columnLabelViewBottom), this, labelFont);

	// create the container view
	CreateContainer(horizontal, vertical, border, resizingMode, flags);
	*containerView = fScrollView;

	// complete the setup
	ShiftDragGroup();
	fColumnLabelView->UpdateDragGroups();
	fExpanderColumn = -1;
	fCompareFunction = NULL;
}


ColumnListView::~ColumnListView()
{
	// delete all list columns
	int32 ColumnCount = fColumnList.CountItems();
	for (int32 i = ColumnCount - 1; i >= 0; i--) {
		CLVColumn* item = (CLVColumn*)fColumnList.RemoveItem(i);
		if (item != NULL)
			delete item;
	}

	// remove and delete the container view if necessary
	if (!fScrollView->IsBeingDestroyed()) {
		fScrollView->RemoveChild(this);
		delete fScrollView;
	}

	delete fEditMessage;
}


void
ColumnListView::CreateContainer(bool horizontal, bool vertical,
	border_style border, uint32 resizingMode, uint32 flags)
{
	BRect frame = Frame();
	BRect labelsFrame = fColumnLabelView->Frame();

	fScrollView = new CLVContainerView(NULL, this, resizingMode, flags,
		horizontal, vertical, border);
	BRect newFrame = Frame();
	// resize the main view to make room for the CLVColumnLabelView
	ResizeTo(frame.right - frame.left, frame.bottom -
		labelsFrame.bottom - 1.0);
	MoveTo(newFrame.left, newFrame.top
		+ (labelsFrame.bottom - labelsFrame.top + 1.0));
	fColumnLabelView->MoveTo(newFrame.left, newFrame.top);

	// add the ColumnLabelView
	fScrollView->AddChild(fColumnLabelView);

	// remove and re-add the BListView so that it will draw after
	// the CLVColumnLabelView
	fScrollView->RemoveChild(this);
	fScrollView->AddChild(this);

	fFillerView = NULL;
}


void
ColumnListView::AddScrollViewCorner()
{
	BPoint farCorner = fScrollView->Bounds().RightBottom();
	fFillerView = new ScrollViewCorner(farCorner.x - B_V_SCROLL_BAR_WIDTH,
		farCorner.y - B_H_SCROLL_BAR_HEIGHT);
	fScrollView->AddChild(fFillerView);
}


void
ColumnListView::ShiftDragGroup()
{
	// figure out the width
	float columnBegin;
	float columnEnd = -1.0;
	fDataWidth = 0.0;
	bool nextPushedByExpander = false;
	int32 columnCount = fColumnDisplayList.CountItems();
	for (int32 i = 0; i < columnCount; i++) {
		CLVColumn* column = (CLVColumn*)fColumnDisplayList.ItemAt(i);
		if (nextPushedByExpander)
			column->fPushedByExpander = true;
		else
			column->fPushedByExpander = false;

		if (column->IsShown()) {
			float columnWidth = column->Width();
			columnBegin = columnEnd + 1.0;
			columnEnd = columnBegin + columnWidth;
			column->fColumnBegin = columnBegin;
			column->fColumnEnd = columnEnd;
			fDataWidth = column->fColumnEnd;
			if (nextPushedByExpander && !(column->fFlags & CLV_PUSH_PASS))
				nextPushedByExpander = false;

			if (column->fFlags & CLV_EXPANDER) {
				// set the next column to be pushed
				nextPushedByExpander = true;
			}
		}
	}

	// figure out the height
	fDataHeight = 0.0;
	int32 itemCount = CountItems();
	for (int32 i = 0; i < itemCount; i++)
		fDataHeight += ItemAt(i)->Height() + 1.0;

	if (itemCount > 0)
		fDataHeight -= 1.0;

	// update the scroll bars
	UpdateScrollBars();
}


void
ColumnListView::UpdateScrollBars()
{
	if (fScrollView != NULL) {
		// figure out the bounds and scroll if necessary
		BRect bounds;
		float deltaX;
		float deltaY;
		do {
			bounds = Bounds();
			//Figure out the width of the page rectangle
			fPageWidth = fDataWidth;
			fPageHeight = fDataHeight;
			//If view runs past the end, make more visible at the beginning
			deltaX = 0.0;
			if (bounds.right > fDataWidth && bounds.left > 0) {
				deltaX = bounds.right - fDataWidth;
				if (deltaX > bounds.left)
					deltaX = bounds.left;
			}
			deltaY = 0.0;
			if (bounds.bottom > fDataHeight && bounds.top > 0) {
				deltaY = bounds.bottom - fDataHeight;
				if (deltaY > bounds.top)
					deltaY = bounds.top;
			}
			if (deltaX != 0.0 || deltaY != 0.0) {
				ScrollTo(BPoint(bounds.left - deltaX, bounds.top - deltaY));
				bounds = Bounds();
			}
			if (bounds.right-bounds.left > fDataWidth)
				fPageWidth = bounds.right;

			if (bounds.bottom-bounds.top > fDataHeight)
				fPageHeight = bounds.bottom;
		} while (deltaX != 0.0 || deltaY != 0.0);

		// figure out the ratio of the bounds rectangle width or height
		// to the page rectangle width or height
		float widthRatio = (bounds.right - bounds.left) / fPageWidth;
		float heightRatio = (bounds.bottom - bounds.top) / fPageHeight;

		BScrollBar* horizontalScrollBar = fScrollView->ScrollBar(B_HORIZONTAL);
		BScrollBar* verticalScrollBar = fScrollView->ScrollBar(B_VERTICAL);
		// set the scroll bar ranges and proportions.  If the whole document
		// is visible, inactivate the slider
		if (horizontalScrollBar != NULL) {
			if (widthRatio >= 1.0 && bounds.left == 0.0)
				horizontalScrollBar->SetRange(0.0, 0.0);
			else {
				horizontalScrollBar->SetRange(0.0, fPageWidth
					- (bounds.right - bounds.left));
			}
			horizontalScrollBar->SetProportion(widthRatio);
			// set the step values
			horizontalScrollBar->SetSteps(20.0, bounds.right - bounds.left);
		}

		if (verticalScrollBar != NULL) {
			if (heightRatio >= 1.0 && bounds.top == 0.0) {
				verticalScrollBar->SetRange(0.0, 0.0);
				if (fFillerView)
					fFillerView->SetViewColor(BeInactiveControlGrey);
			} else {
				verticalScrollBar->SetRange(0.0,
					fPageHeight - (bounds.bottom-bounds.top));
				if (fFillerView != NULL)
					fFillerView->SetViewColor(BeBackgroundGrey);
			}
			verticalScrollBar->SetProportion(heightRatio);
		}
	}
}


void
ColumnListView::ColumnsChanged()
{
	// any previous column dragging/resizing will get corrupted, so deselect
	if (fColumnLabelView->fColumnClicked)
		fColumnLabelView->fColumnClicked = NULL;

	// update the internal sizes and grouping of the columns and sizes
	// of drag groups
	ShiftDragGroup();
	fColumnLabelView->UpdateDragGroups();
	fColumnLabelView->Invalidate();
	Invalidate();
}


// Adds a column to the ColumnListView at the end of the list.
// Returns true if successful.
bool
ColumnListView::AddColumn(CLVColumn* column)
{
	int32 columnCount = fColumnList.CountItems();
	int32 displayIndex = columnCount;

	// make sure a second Expander is not being added
	if ((column->fFlags & CLV_EXPANDER) != 0) {
		if (!fHierarchical)
			return false;

		for (int32 i = 0; i < columnCount; i++) {
			if ((((CLVColumn*)fColumnList.ItemAt(i))->fFlags & CLV_EXPANDER)
					!= 0) {
				return false;
			}
		}

		if (column->IsShown())
			fExpanderColumn = columnCount;
	}

	// make sure this column hasn't already been added to another ColumnListView
	if (column->fParent != NULL)
		return false;

	BWindow* parentWindow = Window();
	if (parentWindow != NULL)
		parentWindow->Lock();

	// check if this should be locked at the beginning or end, and adjust
	// its position if necessary
	if ((column->Flags() & CLV_LOCK_AT_END) == 0) {
		bool shouldRepeat;
		if ((column->Flags() & CLV_LOCK_AT_BEGINNING) != 0) {
			// move it to the beginning, after the last
			// CLV_LOCK_AT_BEGINNING item
			displayIndex = 0;
			shouldRepeat = true;
			while (shouldRepeat && displayIndex < columnCount) {
				shouldRepeat = false;
				CLVColumn* lastColumn
					= (CLVColumn*)fColumnDisplayList.ItemAt(displayIndex);
				if ((lastColumn->Flags() & CLV_LOCK_AT_BEGINNING) != 0) {
					displayIndex++;
					shouldRepeat = true;
				}
			}
		} else {
			// make sure it isn't after a CLV_LOCK_AT_END item
			shouldRepeat = true;
			while (shouldRepeat && displayIndex > 0)
			{
				shouldRepeat = false;
				CLVColumn* lastColumn
					= (CLVColumn*)fColumnDisplayList.ItemAt(displayIndex - 1);
				if ((lastColumn->Flags() & CLV_LOCK_AT_END) != 0) {
					displayIndex--;
					shouldRepeat = true;
				}
			}
		}
	}

	// add the column to the display list in the appropriate position
	fColumnDisplayList.AddItem(column, displayIndex);

	// add the column to the end of the column list
	fColumnList.AddItem(column);

	// tell the column it belongs to me now
	column->fParent = this;

	// set the scroll bars and tell views to update
	ColumnsChanged();
	if (parentWindow != NULL)
		parentWindow->Unlock();

	return true;
}


// Adds a BList of CLVColumn's to the ColumnListView at the position specified,
// or at the end of the list if AtIndex == -1.  Returns true if successful.
bool
ColumnListView::AddColumnList(BList* newColumns)
{
	int32 columnCount = int32(fColumnList.CountItems());
	int32 columnCountToAdd = int32(newColumns->CountItems());

	// make sure a second CLVExpander is not being added
	int32 expanderCount = 0;
	for (int32 i = 0; i < columnCount; i++) {
		if ((((CLVColumn*)fColumnList.ItemAt(i))->fFlags & CLV_EXPANDER) != 0)
			expanderCount++;
	}

	int32 expandercolumnIndex = -1;
	for (int32 i = 0; i < columnCountToAdd; i++) {
		CLVColumn* current = (CLVColumn*)newColumns->ItemAt(i);
		if ((current->fFlags & CLV_EXPANDER) != 0) {
			expanderCount++;
			if (current->IsShown())
				expandercolumnIndex = columnCount + i;
		}
	}

	if (expanderCount != 0 && !fHierarchical)
		return false;

	if (expanderCount > 1)
		return false;

	if (expandercolumnIndex != -1)
		fExpanderColumn = expandercolumnIndex;

	// make sure none of these columns have already been added
	// to a ColumnListView
	for (int32 i = 0; i < columnCountToAdd; i++) {
		if (((CLVColumn*)newColumns->ItemAt(i))->fParent != NULL)
			return false;
	}

	// make sure none of these columns are being added twice
	for (int32 i = 0; i < columnCountToAdd - 1; i++) {
		for (int32 j = i + 1; j < columnCountToAdd; j++) {
			if (newColumns->ItemAt(i) == newColumns->ItemAt(j))
				return false;
		}
	}

	BWindow* parentWindow = Window();
	if (parentWindow != NULL)
		parentWindow->Lock();

	for (int32 i = 0; i < columnCountToAdd; i++) {
		CLVColumn* column = (CLVColumn*)newColumns->ItemAt(i);
		// check if this should be locked at the beginning or end,
		// and adjust its position if necessary
		int32 displayIndex = columnCount;
		if ((column->Flags() & CLV_LOCK_AT_END) == 0) {
			bool shouldRepeat;
			if ((column->Flags() & CLV_LOCK_AT_BEGINNING) != 0) {
				// move it to the beginning, after the last
				// CLV_LOCK_AT_BEGINNING item
				displayIndex = 0;
				shouldRepeat = true;
				while (shouldRepeat && displayIndex < columnCount) {
					shouldRepeat = false;
					CLVColumn* lastColumn
						= (CLVColumn*)fColumnDisplayList.ItemAt(displayIndex);
					if ((lastColumn->Flags() & CLV_LOCK_AT_BEGINNING) != 0) {
						displayIndex++;
						shouldRepeat = true;
					}
				}
			} else {
				// make sure it isn't after a CLV_LOCK_AT_END item
				shouldRepeat = true;
				while (shouldRepeat && displayIndex > 0) {
					shouldRepeat = false;
					CLVColumn* lastColumn
						= (CLVColumn*)fColumnDisplayList.ItemAt(
							displayIndex - 1);
					if ((lastColumn->Flags() & CLV_LOCK_AT_END) != 0) {
						displayIndex--;
						shouldRepeat = true;
					}
				}
			}
		}

		// add the column to the display list in the appropriate position
		fColumnDisplayList.AddItem(column, displayIndex);

		// tell the column it belongs to me now
		column->fParent = this;

		columnCount++;
	}

	// add the columns to the end of the column list
	fColumnList.AddList(newColumns);

	// set the scroll bars and tell views to update
	ColumnsChanged();
	if (parentWindow != NULL)
		parentWindow->Unlock();

	return true;
}


// Removes a CLVColumn from the ColumnListView.
// Returns true if successful.
bool
ColumnListView::RemoveColumn(CLVColumn* column)
{
	if (!fColumnList.HasItem(column))
		return false;

	int32 columnIndex = fSortKeyList.IndexOf(column);
	if (columnIndex >= 0)
		fSortKeyList.RemoveItem(columnIndex);

	if ((column->fFlags & CLV_EXPANDER) != 0)
		fExpanderColumn = -1;

	BWindow* parentWindow = Window();
	if (parentWindow != NULL)
		parentWindow->Lock();

	// remove column from the column and display lists
	fColumnDisplayList.RemoveItem(column);
	fColumnList.RemoveItem(column);

	//tell the column it has been removed
	column->fParent = NULL;

	// set the scroll bars and tell views to update
	ColumnsChanged();
	if (parentWindow != NULL)
		parentWindow->Unlock();

	return true;
}


// Finds column in columnList and removes Count columns and their data
// from the view and its items.
bool
ColumnListView::RemoveColumns(CLVColumn* column, int32 count)
{
	BWindow* parentWindow = Window();
	if (parentWindow != NULL)
		parentWindow->Lock();

	int32 columnIndex = fColumnList.IndexOf(column);
	if (columnIndex < 0) {
		if (parentWindow != NULL)
			parentWindow->Unlock();

		return false;
	}

	if (columnIndex + count >= fColumnList.CountItems()) {
		if (parentWindow != NULL)
			parentWindow->Unlock();

		return false;
	}

	for (int32 i = columnIndex; i < columnIndex + count; i++) {
		// remove columns from the column and display lists
		CLVColumn* current = (CLVColumn*)fColumnList.ItemAt(i);
		fColumnDisplayList.RemoveItem(current);

		int32 sortIndex = fSortKeyList.IndexOf(column);
		if (sortIndex >= 0)
			fSortKeyList.RemoveItem(sortIndex);

		if ((current->fFlags & CLV_EXPANDER) != 0)
			fExpanderColumn = -1;

		// tell the column it has been removed
		current->fParent = NULL;
	}
	fColumnList.RemoveItems(columnIndex, count);

	// set the scroll bars and tell views to update
	ColumnsChanged();
	if (parentWindow != NULL)
		parentWindow->Unlock();

	return true;
}

void
ColumnListView::SetEditMessage(BMessage* message, BMessenger target)
{
	delete fEditMessage;
	fEditMessage = message;
	fEditTarget = target;
}


void
ColumnListView::KeyDown(const char* bytes, int32 numBytes)
{
	int colDiff = 0;
	bool metaKeysPressed = false;

	// find out if any meta-keys are pressed
	int32 q;
	if (Window()->CurrentMessage()->FindInt32("modifiers", &q) == B_NO_ERROR) {
		metaKeysPressed = ((q & (B_SHIFT_KEY | B_COMMAND_KEY | B_CONTROL_KEY
			| B_OPTION_KEY)) != 0);
	}

	if (numBytes > 0) {
		switch (*bytes)
		{
			case B_LEFT_ARROW:
			if (metaKeysPressed == false)
			{
				colDiff = -1;
				break;
			}

			case B_RIGHT_ARROW:
			if (metaKeysPressed == false)
			{
				colDiff = 1;
				break;
			}

			case B_UP_ARROW:
			case B_DOWN_ARROW:
			if (metaKeysPressed == false)
			{
				BListView::KeyDown(bytes, numBytes);
				break;
			}

			default:
			if (fEditMessage != NULL)
			{
				BMessage temp(*fEditMessage);
				temp.AddInt32("column", fSelectedColumn);
				temp.AddInt32("row", CurrentSelection());
				temp.AddString("bytes", bytes);

				int32 key;
				if (Window()->CurrentMessage()->FindInt32("key", &key)
						== B_NO_ERROR) {
					temp.AddInt32("key", key);
				}

				fEditTarget.SendMessage(&temp);
			}
			break;
		}
	}

	if (colDiff != 0) {
		// we need to move the highlighted column by (colDiff) columns
		// if possible.
		int displayColumnCount = fColumnDisplayList.CountItems();
		int currentColumn = fSelectedColumn;
			// currentColumn is an ACTUAL index.
		if (currentColumn == -1) {
			// no current column selected?
			currentColumn = (colDiff > 0) ? GetActualIndexOf(0)
				: GetActualIndexOf(displayColumnCount-1);
				// go to an edge
		} else {
			// go to the display column adjacent to the current column's
			// display column.
			int32 currentDisplayIndex = GetDisplayIndexOf(currentColumn);
			if (currentDisplayIndex < 0)
				currentDisplayIndex = 0;

			currentDisplayIndex += colDiff;

			if (currentDisplayIndex < 0)
				currentDisplayIndex = displayColumnCount - 1;

			if (currentDisplayIndex >= displayColumnCount)
				currentDisplayIndex = 0;

			currentColumn = GetActualIndexOf(currentDisplayIndex);
		}

		SetSelectedColumnIndex(currentColumn);
	}
}


int32
ColumnListView::CountColumns() const
{
	return fColumnList.CountItems();
}


int32
ColumnListView::IndexOfColumn(CLVColumn* column) const
{
	return fColumnList.IndexOf(column);
}


CLVColumn*
ColumnListView::ColumnAt(int32 columnIndex) const
{
	return (CLVColumn*)fColumnList.ItemAt(columnIndex);
}


CLVColumn*
ColumnListView::ColumnAt(BPoint point) const
{
	int32 columnCount = fColumnList.CountItems();
	for (int32 i = 0; i < columnCount; i++) {
		CLVColumn* col = (CLVColumn*)fColumnList.ItemAt(i);
		if ((point.x >= col->fColumnBegin) && (point.x <= col->fColumnEnd))
			return col;
	}

	return NULL;
}


//Sets the display order using a BList of CLVColumn's
bool
ColumnListView::SetDisplayOrder(const int32* ColumnOrder)
{
	BWindow* parentWindow = Window();
	if (parentWindow != NULL)
		parentWindow->Lock();

	// add the items to the display list in order
	fColumnDisplayList.MakeEmpty();
	int32 columnCount = fColumnList.CountItems();
	for (int32 i = 0; i < columnCount; i++) {
		if (ColumnOrder[i] >= columnCount) {
			if (parentWindow != NULL)
				parentWindow->Unlock();

			return false;
		}

		for (int32 j = 0; j < i; j++) {
			if (ColumnOrder[i] == ColumnOrder[j]) {
				if (parentWindow != NULL)
					parentWindow->Unlock();

				return false;
			}
		}
		fColumnDisplayList.AddItem(fColumnList.ItemAt(ColumnOrder[i]));
	}

	// update everything about the columns
	ColumnsChanged();

	// let the program know that the display order changed.
	if (parentWindow != NULL)
		parentWindow->Unlock();

	DisplayOrderChanged(ColumnOrder);

	return true;
}


void
ColumnListView::ColumnWidthChanged(int32 columnIndex, float newWidth)
{
	Invalidate();
}


void
ColumnListView::DisplayOrderChanged(const int32* order)
{
}


int32*
ColumnListView::DisplayOrder() const
{
	int32 columnCount = fColumnList.CountItems();
	int32* list = new int32[columnCount];
	BWindow* parentWindow = Window();
	if (parentWindow != NULL)
		parentWindow->Lock();

	for (int32 i = 0; i < columnCount; i++)
		list[i] = int32(fColumnList.IndexOf(fColumnDisplayList.ItemAt(i)));

	if (parentWindow != NULL)
		parentWindow->Unlock();

	return list;
}


void
ColumnListView::SetSortKey(int32 columnIndex)
{
	CLVColumn* column;
	if (columnIndex >= 0) {
		column = (CLVColumn*)fColumnList.ItemAt(columnIndex);
		if ((column->Flags() & CLV_SORT_KEYABLE) == 0)
			return;
	} else
		column = NULL;

	if (fSortKeyList.ItemAt(0) != column || column == NULL) {
		BWindow* parentWindow = Window();
		if (parentWindow != NULL)
			parentWindow->Lock();

		BRect labelBounds = fColumnLabelView->Bounds();
		// need to remove old sort keys and erase all the old underlines
		int32 sortKeyCount = fSortKeyList.CountItems();
		for (int32 i = 0; i < sortKeyCount; i++) {
			CLVColumn* underlineColumn = (CLVColumn*)fSortKeyList.ItemAt(i);
			if (underlineColumn->fSortMode != SORT_MODE_NONE) {
				fColumnLabelView->Invalidate(
				BRect(underlineColumn->fColumnBegin,
					labelBounds.top, underlineColumn->fColumnEnd,
					labelBounds.bottom));
			}
		}
		fSortKeyList.MakeEmpty();

		if (column != NULL) {
			fSortKeyList.AddItem(column);
			if (column->fSortMode == SORT_MODE_NONE)
				SetSortMode(columnIndex, SORT_MODE_ASCENDING);

			SortItems();
			// need to draw new underline
			fColumnLabelView->Invalidate(BRect(column->fColumnBegin,
				labelBounds.top, column->fColumnEnd, labelBounds.bottom));
		}
		if (parentWindow != NULL)
			parentWindow->Unlock();
	}
}


void
ColumnListView::AddSortKey(int32 columnIndex)
{
	CLVColumn* column;
	if (columnIndex >= 0) {
		column = (CLVColumn*)fColumnList.ItemAt(columnIndex);
		if ((column->Flags() & CLV_SORT_KEYABLE) == 0)
			return;
	} else
		column = NULL;

	if (column != NULL && !fSortKeyList.HasItem(column)) {
		BWindow* parentWindow = Window();
		if (parentWindow != NULL)
			parentWindow->Lock();

		BRect labelBounds = fColumnLabelView->Bounds();
		fSortKeyList.AddItem(column);
		if (column->fSortMode == SORT_MODE_NONE)
			SetSortMode(columnIndex, SORT_MODE_ASCENDING);

		SortItems();

		// need to draw new underline
		fColumnLabelView->Invalidate(BRect(column->fColumnBegin,
			labelBounds.top, column->fColumnEnd, labelBounds.bottom));

		if (parentWindow != NULL)
			parentWindow->Unlock();
	}
}


void
ColumnListView::SetSortMode(int32 columnIndex, CLVSortMode sortMode)
{
	CLVColumn* column;
	if (columnIndex >= 0) {
		column = (CLVColumn*)fColumnList.ItemAt(columnIndex);
		if ((column->Flags() & CLV_SORT_KEYABLE) == 0)
			return;
	} else
		return;

	if (column->fSortMode != sortMode) {
		BWindow* parentWindow = Window();
		if (parentWindow != NULL)
			parentWindow->Lock();

		BRect labelBounds = fColumnLabelView->Bounds();
		column->fSortMode = sortMode;
		if (sortMode == SORT_MODE_NONE && fSortKeyList.HasItem(column))
			fSortKeyList.RemoveItem(column);

		SortItems();
		// need to draw or erase underline

		fColumnLabelView->Invalidate(BRect(column->fColumnBegin,
			labelBounds.top, column->fColumnEnd, labelBounds.bottom));
		if (parentWindow != NULL)
			parentWindow->Unlock();
	}
}


void
ColumnListView::ReverseSortMode(int32 columnIndex)
{
	CLVColumn* column;
	if (columnIndex >= 0) {
		column = (CLVColumn*)fColumnList.ItemAt(columnIndex);
		if ((column->Flags() & CLV_SORT_KEYABLE) == 0)
			return;
	} else
		return;

	if (column->fSortMode == SORT_MODE_ASCENDING)
		SetSortMode(columnIndex, SORT_MODE_DESCENDING);
	else if (column->fSortMode == SORT_MODE_DESCENDING)
		SetSortMode(columnIndex, SORT_MODE_NONE);
	else if (column->fSortMode == SORT_MODE_NONE)
		SetSortMode(columnIndex, SORT_MODE_ASCENDING);
}


int32
ColumnListView::Sorting(int32* sortKeys, CLVSortMode* sortModes) const
{
	BWindow* parentWindow = Window();
	if (parentWindow != NULL)
		parentWindow->Lock();

	int32 keyCount = fSortKeyList.CountItems();
	for (int32 i = 0; i < keyCount; i++) {
		CLVColumn* Column = (CLVColumn*)fSortKeyList.ItemAt(i);
		sortKeys[i] = IndexOfColumn(Column);
		sortModes[i] = Column->SortMode();
	}

	if (parentWindow != NULL)
		parentWindow->Unlock();

	return keyCount;
}

void
ColumnListView::Pulse()
{
	int32 curSel = CurrentSelection();
	if (curSel >= 0) {
		CLVListItem* item = (CLVListItem*) ItemAt(curSel);
		item->Pulse(this);
	}
}


void
ColumnListView::SetSorting(int32 keyCount, int32* sortKeys,
	CLVSortMode* sortModes)
{
	BWindow* parentWindow = Window();
	if (parentWindow != NULL)
		parentWindow->Lock();

	// need to remove old sort keys and erase all the old underlines
	BRect labelBounds = fColumnLabelView->Bounds();
	int32 sortKeyCount = fSortKeyList.CountItems();
	for (int32 i = 0; i < sortKeyCount; i++) {
		CLVColumn* underlineColumn = (CLVColumn*)fSortKeyList.ItemAt(i);
		if (underlineColumn->fSortMode != SORT_MODE_NONE) {
			fColumnLabelView->Invalidate(BRect(underlineColumn->fColumnBegin,
				labelBounds.top, underlineColumn->fColumnEnd,
				labelBounds.bottom));
		}
	}
	fSortKeyList.MakeEmpty();

	for (int32 i = 0; i < keyCount; i++) {
		if (i == 0)
			SetSortKey(sortKeys[0]);
		else
			AddSortKey(sortKeys[i]);

		SetSortMode(sortKeys[i], sortModes[i]);
	}

	if (parentWindow != NULL)
		parentWindow->Unlock();
}


void
ColumnListView::FrameResized(float newWidth, float newHeight)
{
	ShiftDragGroup();
	uint32 itemCount = CountItems();
	BFont font;
	GetFont(&font);
	for (uint32 i = 0; i < itemCount; i++)
		ItemAt(i)->Update(this, &font);

	BListView::FrameResized(newWidth, newHeight);
}


void
ColumnListView::AttachedToWindow()
{
	// Hack to work around app_server bug
	BListView::AttachedToWindow();
	ShiftDragGroup();
}


void
ColumnListView::ScrollTo(BPoint point)
{
	BListView::ScrollTo(point);
	fColumnLabelView->ScrollTo(BPoint(point.x, 0.0));
}

int32
ColumnListView::GetActualIndexOf(int32 displayIndex) const
{
	if ((displayIndex < 0) || (displayIndex >= fColumnDisplayList.CountItems()))
		return -1;

	return (int32)fColumnList.IndexOf(fColumnDisplayList.ItemAt(displayIndex));
}


int32
ColumnListView::GetDisplayIndexOf(int32 realIndex) const
{
	if ((realIndex < 0) || (realIndex >= fColumnList.CountItems()))
		return -1;

	return (int32)fColumnDisplayList.IndexOf(fColumnList.ItemAt(realIndex));
}


// Set a new (actual) column index as the selected index.
// Call with arg -1 to unselect all.
// Gotta change the fSelectedColumn on all entries.
// There is undoubtedly a more efficient way to do this!  --jaf
void
ColumnListView :: SetSelectedColumnIndex(int32 col)
{
	if (fSelectedColumn != col) {
		fSelectedColumn = col;

		int32 numRows = fFullItemList.CountItems();
		for (int32 j = 0; j < numRows; j++) {
			((CLVListItem *)fFullItemList.ItemAt(j))->fSelectedColumn
				= fSelectedColumn;
		}

		// update current row if necessary
		int32 selectedIndex = CurrentSelection();
		if (selectedIndex != -1)
			InvalidateItem(selectedIndex);
   }
}


void
ColumnListView::MouseDown(BPoint point)
{
    int prevColumn = fSelectedColumn;
	int32 columnCount = fColumnDisplayList.CountItems();
	float xleft = point.x;
	for (int32 i = 0; i < columnCount; i++) {
		CLVColumn* column = (CLVColumn*)fColumnDisplayList.ItemAt(i);
		if (column->IsShown()) {
			if (xleft > 0) {
				xleft -= column->Width();
				if (xleft <= 0) {
					SetSelectedColumnIndex(GetActualIndexOf(i));
					break;
				}
			}
		}
	}

	int32 itemIndex = IndexOf(point);
	if (itemIndex >= 0) {
		CLVListItem* clickedItem = (CLVListItem*)BListView::ItemAt(itemIndex);
		if (clickedItem->fIsSuperItem) {
			if (clickedItem->fExpanderButtonRect.Contains(point)) {
				if (clickedItem->IsExpanded())
					Collapse(clickedItem);
				else
					Expand(clickedItem);

				return;
			}
		}
	}


	// if secondary mouse click, hoist up the popup-menu
	const char* selectedText = NULL;
	CLVColumn* column = ColumnAt(fSelectedColumn);
	if (column != NULL) {
		BPopUpMenu* popup = column->GetPopup();
		if (popup != NULL) {
			BMessage* message = Window()->CurrentMessage();
			int32 buttons;
			if ((message->FindInt32("buttons", &buttons) == B_NO_ERROR)
				&& (buttons == B_SECONDARY_MOUSE_BUTTON)) {
				BPoint where(point);
				Select(IndexOf(where));
				ConvertToScreen(&where);
				BMenuItem* result = popup->Go(where, false);
				if (result != NULL)
					selectedText = result->Label();
			}
		}
	}

	int32 previousRow = CurrentSelection();
	BListView::MouseDown(point);

	int32 currentRow = CurrentSelection();
	if ((fEditMessage != NULL) && (selectedText != NULL
		|| (fSelectedColumn == prevColumn && currentRow == previousRow))) {
		// send mouse message...
		BMessage temp(*fEditMessage);
		temp.AddInt32("column", fSelectedColumn);
		temp.AddInt32("row", CurrentSelection());
		if (selectedText)
			temp.AddString("text", selectedText);
		else
			temp.AddInt32("mouseClick", 0);

		fEditTarget.SendMessage(&temp);
	}
}


bool
ColumnListView::AddUnder(CLVListItem* item, CLVListItem* superitem)
{
	if (!fHierarchical)
		return false;

	// find the superitem in the full list and display list (if shown)
	int32 superItemPosition = fFullItemList.IndexOf(superitem);
	if (superItemPosition < 0)
		return false;

	uint32 SuperitemLevel = superitem->fOutlineLevel;

	// add the item under the superitem in the full list
	int32 itemPosition = superItemPosition + 1;
	item->fOutlineLevel = SuperitemLevel + 1;
	while (true) {
		CLVListItem* temp = (CLVListItem*)fFullItemList.ItemAt(itemPosition);
		if (temp) {
			if (temp->fOutlineLevel > SuperitemLevel)
				itemPosition++;
			else
				break;
		}
		else
			break;
	}

	return AddItemPrivate(item, itemPosition);
}


bool
ColumnListView::AddItem(CLVListItem* item, int32 fullListIndex)
{
	return AddItemPrivate(item, fullListIndex);
}


bool
ColumnListView::AddItem(CLVListItem* item)
{
	if (fHierarchical)
		return AddItemPrivate(item, fFullItemList.CountItems());
	else
		return AddItemPrivate(item, CountItems());
}


bool
ColumnListView::AddItem(BListItem* item, int32 fullListIndex)
{
	return BListView::AddItem(item, fullListIndex);
}


bool
ColumnListView::AddItem(BListItem* item)
{
	return BListView::AddItem(item);
}


bool
ColumnListView::AddItemPrivate(CLVListItem* item, int32 fullListIndex)
{
	item->fSelectedColumn = fSelectedColumn;

	if (fHierarchical) {
		uint32 itemLevel = item->OutlineLevel();

		// figure out whether it is visible (should it be added to visible list)
		bool isVisible = true;

		// find the item that contains it in the full list
		int32 superItemPosition;
		if (itemLevel == 0)
			superItemPosition = -1;
		else
			superItemPosition = fullListIndex - 1;

		CLVListItem* superItem;
		while (superItemPosition >= 0) {
			superItem = (CLVListItem*)fFullItemList.ItemAt(superItemPosition);
			if (superItem != NULL) {
				if (superItem->fOutlineLevel >= itemLevel)
					superItemPosition--;
				else
					break;
			} else
				return false;
		}

		if (superItemPosition >= 0 && superItem != NULL) {
			if (!superItem->IsExpanded()) {
				// superItem's contents aren't visible
				isVisible = false;
			}
			if (!HasItem(superItem)) {
				// superItem itself isn't showing
				isVisible = false;
			}
		}

		// add the item to the full list
		if (!fFullItemList.AddItem(item, fullListIndex))
			return false;
		else {
			// add the item to the display list
			if (isVisible) {
				// find the previous item, or -1 if the item I'm adding
				// will be the first one
				int32 PreviousitemPosition = fullListIndex - 1;
				CLVListItem* previousItem;
				while (PreviousitemPosition >= 0) {
					previousItem
						= (CLVListItem*)fFullItemList.ItemAt(
							PreviousitemPosition);
					if (previousItem != NULL && HasItem(previousItem))
						break;
					else
						PreviousitemPosition--;
				}

				// add the item after the previous item, or first on the list
				bool itemAdded;
				if (PreviousitemPosition >= 0)
					itemAdded = BListView::AddItem((BListItem*)item,
						IndexOf(previousItem) + 1);
				else
					itemAdded = BListView::AddItem((BListItem*)item, 0);

				if (itemAdded == false)
					fFullItemList.RemoveItem(item);

				return itemAdded;
			}

			return true;
		}
	} else
		return BListView::AddItem(item, fullListIndex);
}


bool
ColumnListView::AddList(BList* newItems)
{
	if (fHierarchical)
		return AddListPrivate(newItems, fFullItemList.CountItems());
	else
		return AddListPrivate(newItems, CountItems());
}


bool
ColumnListView::AddList(BList* newItems, int32 fullListIndex)
{
	return AddListPrivate(newItems, fullListIndex);
}


bool
ColumnListView::AddListPrivate(BList* newItems, int32 fullListIndex)
{
	int32 itemCount = newItems->CountItems();
	for (int32 count = 0; count < itemCount; count++) {
		if (!AddItemPrivate((CLVListItem*)newItems->ItemAt(count),
				fullListIndex + count)) {
			return false;
		}
	}

	return true;
}


bool
ColumnListView::RemoveItem(CLVListItem* item)
{
	if (item == NULL || !fFullItemList.HasItem(item))
		return false;

	if (fHierarchical) {
		int32 itemsToRemove = 1 + FullListNumberOfSubitems(item);
		return RemoveItems(fFullItemList.IndexOf(item), itemsToRemove);
	} else
		return BListView::RemoveItem((BListItem*)item);
}


BListItem*
ColumnListView::RemoveItem(int32 fullListIndex)
{
	if (fHierarchical) {
		CLVListItem* item = (CLVListItem*)fFullItemList.ItemAt(fullListIndex);
		if (item)
		{
			int32 itemsToRemove = 1 + FullListNumberOfSubitems(item);
			if (RemoveItems(fullListIndex, itemsToRemove))
				return item;
			else
				return NULL;
		}
		else
			return NULL;
	} else
		return BListView::RemoveItem(fullListIndex);
}


bool
ColumnListView::RemoveItems(int32 fullListIndex, int32 count)
{
	CLVListItem* item;
	if (fHierarchical) {
		uint32 LastSuperitemLevel = UINT32_MAX;
		int32 i;
		int32 DisplayitemsToRemove = 0;
		int32 firstDisplayItemToRemove = -1;

		for (i = fullListIndex; i < fullListIndex + count; i++) {
			item = FullListItemAt(i);
			if (item->fOutlineLevel < LastSuperitemLevel)
				LastSuperitemLevel = item->fOutlineLevel;

			if (BListView::HasItem((BListItem*)item)) {
				DisplayitemsToRemove++;
				if (firstDisplayItemToRemove == -1)
					firstDisplayItemToRemove = BListView::IndexOf(item);
			}
		}

		while (true) {
			item = FullListItemAt(i);
			if (item != NULL && item->fOutlineLevel > LastSuperitemLevel) {
				count++;
				i++;
				if (BListView::HasItem((BListItem*)item)) {
					DisplayitemsToRemove++;
					if (firstDisplayItemToRemove == -1) {
						firstDisplayItemToRemove
							= BListView::IndexOf((BListItem*)item);
					}
				}
			} else
				break;
		}

		while (DisplayitemsToRemove > 0) {
			if (BListView::RemoveItem(firstDisplayItemToRemove) == NULL)
				return false;

			DisplayitemsToRemove--;
		}

		return fFullItemList.RemoveItems(fullListIndex, count);
	} else
		return BListView::RemoveItems(fullListIndex, count);
}


bool
ColumnListView::RemoveItem(BListItem* item)
{
	return BListView::RemoveItem(item);
}


CLVListItem*
ColumnListView::FullListItemAt(int32 fullListIndex) const
{
	return (CLVListItem*)fFullItemList.ItemAt(fullListIndex);
}


int32
ColumnListView::FullListIndexOf(const CLVListItem* item) const
{
	return fFullItemList.IndexOf((CLVListItem*)item);
}


int32
ColumnListView::FullListIndexOf(BPoint point) const
{
	int32 DisplayListIndex = IndexOf(point);
	CLVListItem* item = (CLVListItem*)ItemAt(DisplayListIndex);

	return item != NULL ? FullListIndexOf(item) : -1;
}


CLVListItem*
ColumnListView::FullListFirstItem() const
{
	return (CLVListItem*)fFullItemList.FirstItem();
}


CLVListItem*
ColumnListView::FullListLastItem() const
{
	return (CLVListItem*)fFullItemList.LastItem();
}


bool
ColumnListView::FullListHasItem(const CLVListItem* item) const
{
	return fFullItemList.HasItem((CLVListItem*)item);
}


int32
ColumnListView::FullListCountItems() const
{
	return fFullItemList.CountItems();
}


void
ColumnListView::MakeEmpty()
{
	fFullItemList.MakeEmpty();
	BListView::MakeEmpty();
}


void
ColumnListView::MakeEmptyPrivate()
{
	fFullItemList.MakeEmpty();
	BListView::MakeEmpty();
}


bool
ColumnListView::FullListIsEmpty() const
{
	return fFullItemList.IsEmpty();
}


int32
ColumnListView::FullListCurrentSelection(int32 index) const
{
	return FullListIndexOf((CLVListItem*)ItemAt(CurrentSelection(index)));
}


void
ColumnListView::FullListDoForEach(bool (*func)(CLVListItem*))
{
	int32 itemCount = fFullItemList.CountItems();
	for (int32 i = 0; i < itemCount; i++) {
		if (func((CLVListItem*)fFullItemList.ItemAt(i)) == true)
			return;
	}
}


void
ColumnListView::FullListDoForEach(bool (*func)(CLVListItem*, void*),
	void* arg2)
{
	int32 itemCount = fFullItemList.CountItems();
	for (int32 i = 0; i < itemCount; i++) {
		if (func((CLVListItem*)fFullItemList.ItemAt(i), arg2) == true)
			return;
	}
}


CLVListItem*
ColumnListView::Superitem(const CLVListItem* item) const
{
	int32 superItemPosition;
	uint32 itemLevel = item->fOutlineLevel;

	if (itemLevel == 0)
		superItemPosition = -1;
	else
		superItemPosition = fFullItemList.IndexOf((CLVListItem*)item) - 1;

	CLVListItem* superItem = NULL;
	while (superItemPosition >= 0) {
		superItem = (CLVListItem*)fFullItemList.ItemAt(superItemPosition);
		if (superItem != NULL) {
			if (superItem->fOutlineLevel >= itemLevel)
				superItemPosition--;
			else
				break;
		} else
			return NULL;
	}

	return superItemPosition >= 0 ? superItem : NULL;
}


int32
ColumnListView::FullListNumberOfSubitems(const CLVListItem* item) const
{
	if (!fHierarchical)
		return 0;

	int32 itemPosition = FullListIndexOf(item);
	int32 subitemPosition;
	uint32 SuperitemLevel = item->fOutlineLevel;
	if (itemPosition >= 0) {
		for (subitemPosition = itemPosition + 1; subitemPosition >= 1;
			subitemPosition++) {
			CLVListItem* item = FullListItemAt(subitemPosition);
			if (item == NULL || item->fOutlineLevel <= SuperitemLevel)
				break;
		}
	} else
		return 0;

	return subitemPosition - itemPosition - 1;
}


void
ColumnListView::Expand(CLVListItem* item)
{
	BWindow* parentWindow = Window();
	if (parentWindow != NULL)
		parentWindow->Lock();

	if (!item->fIsSuperItem)
		item->fIsSuperItem = true;

	if (item->IsExpanded()) {
		if (parentWindow != NULL)
			parentWindow->Unlock();

		return;
	}

	item->SetExpanded(true);
	if (!fHierarchical) {
		if (parentWindow != NULL)
			parentWindow->Unlock();

		return;
	}

	int32 displayIndex = IndexOf(item);
	if (displayIndex >= 0) {
		if (fExpanderColumn >= 0) {
			// change the state of the arrow
			item->DrawItemColumn(this, item->fExpanderColumnRect,
				fExpanderColumn, (fExpanderColumn == fSelectedColumn), true);
			SetDrawingMode(B_OP_OVER);
			DrawBitmap(&fDownArrow, BRect(0.0, 0.0,
				item->fExpanderButtonRect.right -
					item->fExpanderButtonRect.left, 10.0),
				item->fExpanderButtonRect);
			SetDrawingMode(B_OP_COPY);
		}

		// add the items under it
		int32 fullListIndex = fFullItemList.IndexOf(item);
		uint32 itemLevel = item->fOutlineLevel;
		int32 i = fullListIndex + 1;
		int32 addPosition = displayIndex + 1;
		while (true) {
			CLVListItem* nextItem = (CLVListItem*)fFullItemList.ItemAt(i);
			if (nextItem == NULL)
				break;

			if (nextItem->fOutlineLevel > itemLevel) {
				BListView::AddItem((BListItem*)nextItem, addPosition++);
				if (nextItem->fIsSuperItem && !nextItem->IsExpanded()) {
					// the item I just added is collapsed, so skip over all
					// its children
					uint32 SkipLevel = nextItem->fOutlineLevel + 1;
					while (true) {
						i++;
						nextItem = (CLVListItem*)fFullItemList.ItemAt(i);
						if (nextItem == NULL)
							break;

						if (nextItem->fOutlineLevel < SkipLevel)
							break;
					}
				} else
					i++;
			} else
				break;
		}
	}

	if (parentWindow != NULL)
		parentWindow->Unlock();
}


void
ColumnListView::Collapse(CLVListItem* item)
{
	BWindow* parentWindow = Window();
	if (parentWindow != NULL)
		parentWindow->Lock();

	if (!item->fIsSuperItem)
		item->fIsSuperItem = true;

	if (!(item->IsExpanded())) {
		if (parentWindow != NULL)
			parentWindow->Unlock();

		return;
	}

	item->SetExpanded(false);
	if (!fHierarchical) {
		if (parentWindow != NULL)
			parentWindow->Unlock();
		return;
	}

	int32 displayIndex = IndexOf((BListItem*)item);
	if (displayIndex >= 0) {
		if (fExpanderColumn >= 0) {
			// change the state of the arrow
			item->DrawItemColumn(this, item->fExpanderColumnRect,
				fExpanderColumn, (fExpanderColumn == fSelectedColumn), true);
			SetDrawingMode(B_OP_OVER);
			DrawBitmap(&fRightArrow, BRect(0.0, 0.0,
				item->fExpanderButtonRect.right -
					item->fExpanderButtonRect.left,
				10.0), item->fExpanderButtonRect);
			SetDrawingMode(B_OP_COPY);
		}

		// remove the items under it
		uint32 itemLevel = item->fOutlineLevel;
		int32 nextItemIndex = displayIndex + 1;
		while (true) {
			CLVListItem* nextItem = (CLVListItem*)ItemAt(nextItemIndex);
			if (nextItem != NULL) {
				if (nextItem->fOutlineLevel > itemLevel)
					BListView::RemoveItem(nextItemIndex);
				else
					break;
			} else
				break;
		}
	}

	if (parentWindow != NULL)
		parentWindow->Unlock();
}


bool
ColumnListView::IsExpanded(int32 fullListIndex) const
{
	BListItem* item = (BListItem*)fFullItemList.ItemAt(fullListIndex);
	return item != NULL ? item->IsExpanded() : false;
}


void
ColumnListView::SetSortFunction(CLVCompareFuncPtr compare)
{
	fCompareFunction = compare;
}


void
ColumnListView::SortItems()
{
	BWindow* parentWindow = Window();
	if (parentWindow != NULL)
		parentWindow->Lock();

	BList newList;
	int32 itemCount;
	if (!fHierarchical)
		itemCount = CountItems();
	else
		itemCount = fFullItemList.CountItems();

	if (itemCount == 0) {
		if (parentWindow != NULL)
			parentWindow->Unlock();

		return;
	}

	int32 i;
	if (!fHierarchical) {
		// plain sort, remember the list context for each item
		for (i = 0; i < itemCount; i++)
			((CLVListItem*)ItemAt(i))->fSortingContextCLV = this;
		// do the actual sort
		BListView::SortItems((int (*)(const void*,
			const void*))ColumnListView::PlainBListSortFunc);
	} else {
		// block-by-block sort
		SortFullListSegment(0, 0, &newList);
		fFullItemList = newList;
		// remember the list context for each item
		for (i = 0; i < itemCount; i++) {
			((CLVListItem*)fFullItemList.ItemAt(i))->fSortingContextBList
				= &newList;
		}
		// do the actual sort
		BListView::SortItems((int (*)(const void*,
			const void*))ColumnListView::HierarchicalBListSortFunc);
	}

	if (parentWindow != NULL)
		parentWindow->Unlock();
}


int
ColumnListView::PlainBListSortFunc(BListItem** firstItem,
	BListItem** secondItem)
{
	CLVListItem* item1 = (CLVListItem*)*firstItem;
	CLVListItem* item2 = (CLVListItem*)*secondItem;
	ColumnListView* sortingContext = item1->fSortingContextCLV;
	int32 sortDepth = sortingContext->fSortKeyList.CountItems();
	int compareResult = 0;

	if (sortingContext->fCompareFunction) {
		for (int32 SortIteration = 0;
				SortIteration < sortDepth && compareResult == 0;
				SortIteration++) {
			CLVColumn* column = (CLVColumn*)sortingContext->fSortKeyList.ItemAt(
				SortIteration);
			compareResult = sortingContext->fCompareFunction(item1, item2,
				sortingContext->fColumnList.IndexOf(column));
			if (column->fSortMode == SORT_MODE_DESCENDING)
				compareResult = 0 - compareResult;
		}
	}

	return compareResult;
}


int
ColumnListView::HierarchicalBListSortFunc(BListItem** firstItem,
	BListItem** secondItem)
{
	CLVListItem* item1 = (CLVListItem*)*firstItem;
	CLVListItem* item2 = (CLVListItem*)*secondItem;
	if (item1->fSortingContextBList->IndexOf(item1)
			< item1->fSortingContextBList->IndexOf(item2)) {
		return -1;
	} else
		return 1;
}


void
ColumnListView::SortFullListSegment(int32 originalListStartIndex,
	int32 insertionPoint, BList* newList)
{
	// identify and sort the items at this level
	BList* itemCount = SortItemCount(originalListStartIndex);
	int32 newItemsStopIndex = insertionPoint + itemCount->CountItems();
	newList->AddList(itemCount, insertionPoint);
	delete itemCount;

	// identify and sort the subitems
	for (int32 i = insertionPoint; i < newItemsStopIndex; i++) {
		CLVListItem* item = (CLVListItem*)newList->ItemAt(i);
		CLVListItem* nextItem = (CLVListItem*)fFullItemList.ItemAt(
			fFullItemList.IndexOf(item) + 1);
		if (item->fIsSuperItem && nextItem != NULL
			&& item->fOutlineLevel < nextItem->fOutlineLevel) {
			int32 OldListSize = newList->CountItems();
			SortFullListSegment(fFullItemList.IndexOf(item) + 1,
				i + 1, newList);
			int32 newListSize = newList->CountItems();
			newItemsStopIndex += newListSize - OldListSize;
			i += newListSize - OldListSize;
		}
	}
}


BList*
ColumnListView::SortItemCount(int32 originalListStartIndex)
{
	uint32 level = ((CLVListItem*)fFullItemList.ItemAt(
		originalListStartIndex))->fOutlineLevel;

	// create a new BList of the items in this level
	int32 i = originalListStartIndex;
	int32 itemCount = 0;
	BList* itemsList = new BList(16);
	while (true) {
		CLVListItem* item = (CLVListItem*)fFullItemList.ItemAt(i);
		if (item == NULL)
			break;

		uint32 itemLevel = item->fOutlineLevel;
		if (itemLevel == level) {
			itemsList->AddItem(item);
			itemCount++;
		} else if (itemLevel < level)
			break;

		i++;
	}

	// sort the BList of the items in this level
	CLVListItem** sortArray = new CLVListItem*[itemCount];
	CLVListItem** levelListItems = (CLVListItem**)itemsList->Items();
	for (i = 0; i < itemCount; i++)
		sortArray[i] = levelListItems[i];

	itemsList->MakeEmpty();
	SortListArray(sortArray, itemCount);
	for (i = 0; i < itemCount; i++)
		itemsList->AddItem(sortArray[i]);

	delete[] sortArray;

	return itemsList;
}


void
ColumnListView::SortListArray(CLVListItem** sortArray, int32 itemCount)
{
	int32 sortDepth = fSortKeyList.CountItems();
	for (int32 i = 0; i < itemCount - 1; i++) {
		for (int32 j = i + 1; j < itemCount; j++) {
			int compareResult = 0;
			if (fCompareFunction) {
				for (int32 SortIteration = 0;
					SortIteration < sortDepth && compareResult == 0;
					SortIteration++) {
					CLVColumn* Column = (CLVColumn*)fSortKeyList.ItemAt(
						SortIteration);
					compareResult = fCompareFunction(sortArray[i],
						sortArray[j], fColumnList.IndexOf(Column));
					if (Column->fSortMode == SORT_MODE_DESCENDING)
						compareResult = 0 - compareResult;
				}
			}

			if (compareResult == 1) {
				CLVListItem* Temp = sortArray[i];
				sortArray[i] = sortArray[j];
				sortArray[j] = Temp;
			}
		}
	}
}


void
ColumnListView::MessageReceived(BMessage* message)
{
	switch(message->what) {
		case B_UNMAPPED_KEY_DOWN:
			if (fEditMessage != NULL) {
				BMessage temp(*fEditMessage);
				temp.AddInt32("column", fSelectedColumn);
				temp.AddInt32("row", CurrentSelection());

				int32 key;
				if (message->FindInt32("key", &key) == B_NO_ERROR)
					temp.AddInt32("unmappedkey", key);
				fEditTarget.SendMessage(&temp);
			}
			break;

		default:
			BListView::MessageReceived(message);
	}
}
