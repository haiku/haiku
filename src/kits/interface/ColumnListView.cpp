/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

/*******************************************************************************
/
/	File:			ColumnListView.cpp
/
/   Description:    Experimental multi-column list view.
/
/	Copyright 2000+, Be Incorporated, All Rights Reserved
/					 By Jeff Bush
/
*******************************************************************************/

#include <typeinfo>

#include <stdio.h>
#include <stdlib.h>

#include <Application.h>
#include <Bitmap.h>
#if _INCLUDES_CLASS_CURSOR
#include <Cursor.h>
#endif
#include <Debug.h>
#include <GraphicsDefs.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Region.h>
#include <ScrollBar.h>
#include <String.h>
#include <Window.h>

#include "ObjectList.h"
#include "ColumnListView.h"
#include "ColorTools.h"
/*
#ifndef _ARCHIVE_DEFS_H
#include <archive_defs.h>
#endif
*/
#define DOUBLE_BUFFERED_COLUMN_RESIZE 1
#define SMART_REDRAW 1
#define DRAG_TITLE_OUTLINE 1
#define CONSTRAIN_CLIPPING_REGION 1
#define LOWER_SCROLLBAR 1

namespace BPrivate {

static const unsigned char kResizeCursorData[] = {
	16, 1, 8, 8,
	0, 0, 1, 0x80, 1, 0x80, 1, 0x80, 9, 0x90, 0x19, 0x98, 0x39, 0x09c, 0x79, 0x9e,
	0x79, 0x9e, 0x39, 0x9c, 0x19, 0x98, 0x9, 0x90, 1, 0x80, 1, 0x80, 1, 0x80, 0, 0,
	3, 0xc0, 3, 0xc0, 3, 0xc0, 0xf, 0xf0, 0x1f, 0xf8, 0x3f, 0xfa, 0x7f, 0xfe, 0xff, 0xff,
	0xff, 0xff, 0x7f, 0xfe, 0x3f, 0xfa, 0x1f, 0xf8, 0xf, 0xf0, 3, 0xc0, 3, 0xc0, 3, 0xc0
};

static const unsigned char kMaxResizeCursorData[] = {
	16, 1, 8, 8,
	0, 0, 1, 0x80, 1, 0x80, 1, 0x80, 9, 0x80, 0x19, 0x80, 0x39, 0x80, 0x79, 0x80,
	0x79, 0x80, 0x39, 0x80, 0x19, 0x80, 0x9, 0x80, 1, 0x80, 1, 0x80, 1, 0x80, 0, 0,
	3, 0xc0, 3, 0xc0, 3, 0xc0, 0xf, 0xc0, 0x1f, 0xc0, 0x3f, 0xc0, 0x7f, 0xc0, 0xff, 0xc0,
	0xff, 0xc0, 0x7f, 0xc0, 0x3f, 0xc0, 0x1f, 0xc0, 0xf, 0xc0, 3, 0xc0, 3, 0xc0, 3, 0xc0
};

static const unsigned char kMinResizeCursorData[] = {
	16, 1, 8, 8,
	0, 0, 1, 0x80, 1, 0x80, 1, 0x80, 1, 0x90, 1, 0x98, 1, 0x09c, 1, 0x9e,
	1, 0x9e, 1, 0x9c, 1, 0x98, 1, 0x90, 1, 0x80, 1, 0x80, 1, 0x80, 0, 0,
	3, 0xc0, 3, 0xc0, 3, 0xc0, 3, 0xf0, 3, 0xf8, 3, 0xfa, 3, 0xfe, 3, 0xff,
	3, 0xff, 3, 0xfe, 3, 0xfa, 3, 0xf8, 3, 0xf0, 3, 0xc0, 3, 0xc0, 3, 0xc0
};

static const unsigned char kColumnMoveCursorData[] = {
	16, 1, 8, 8,
	1, 0x80, 3, 0xc0, 7, 0xe0, 0, 0, 0, 0, 0x20, 4, 0x61, 0x86, 0xe3, 0xc7,
	0xe3, 0xc7, 0x61, 0x86, 0x20, 4, 0, 0, 0, 0, 7, 0xe0, 3, 0xc0, 1, 0x80,
	1, 0x80, 3, 0xc0, 7, 0xe0, 0, 0, 0, 0, 0x20, 4, 0x61, 0x86, 0xe3, 0xc7,
	0xe3, 0xc7, 0x61, 0x86, 0x20, 4, 0, 0, 0, 0, 7, 0xe0, 3, 0xc0, 1, 0x80
};

static const unsigned char kUpSortArrow8x8[] = {
	0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
	0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
	0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0xff
};

static const unsigned char kDownSortArrow8x8[] = {
	0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
	0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
	0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff
};

static const unsigned char kUpSortArrow8x8Invert[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x1f, 0x1f, 0x1f, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0x1f, 0xff, 0xff, 0xff, 0xff
};

static const unsigned char kDownSortArrow8x8Invert[] = {
	0xff, 0xff, 0xff, 0x1f, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x1f, 0x1f, 0x1f, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

/*
static const rgb_color kTitleColor = {215, 215, 215, 255};
static const rgb_color kTitleTextColor = { 0, 0, 0, 255 };
static const rgb_color kDefaultBackgroundColor = {236, 236, 236, 255};
static const rgb_color kRowDividerColor = {148, 148, 148, 255};
static const rgb_color kDefaultSelectionColor = {255, 255, 255, 255};
static const rgb_color kDefaultEditColor = {180, 180, 180, 180};
static const rgb_color kNonFocusSelectionColor = {220, 220, 220, 255};
*/

static const float kTitleHeight = 17.0;
static const float kLatchWidth = 15.0;


static const rgb_color kColor[B_COLOR_TOTAL] =
{
    {236, 236, 236, 255},           // B_COLOR_BACKGROUND
    {  0,   0,   0, 255},           // B_COLOR_TEXT
    {148, 148, 148, 255},           // B_COLOR_ROW_DIVIDER
    {255, 255, 255, 255},           // B_COLOR_SELECTION
    {  0,   0,   0, 255},           // B_COLOR_SELECTION_TEXT
    {220, 220, 220, 255},           // B_COLOR_NON_FOCUS_SELECTION
    {180, 180, 180, 180},           // B_COLOR_EDIT_BACKGROUND
    {  0,   0,   0, 255},           // B_COLOR_EDIT_TEXT
    {215, 215, 215, 255},           // B_COLOR_HEADER_BACKGROUND
    {  0,   0,   0, 255},           // B_COLOR_HEADER_TEXT
    {  0,   0,   0, 255},           // B_COLOR_SEPARATOR_LINE
    {  0,   0,   0, 255},           // B_COLOR_SEPARATOR_BORDER
};

static const int32 kMaxDepth = 1024;
static const float kLeftMargin = kLatchWidth;
static const float kRightMargin = kLatchWidth;
static const float kBottomMargin = kLatchWidth;
static const float kOutlineLevelIndent = kLatchWidth;
static const float kColumnResizeAreaWidth = 10.0;
static const float kRowDragSensitivity = 5.0;
static const float kDoubleClickMoveSensitivity = 4.0;
static const float kSortIndicatorWidth = 9.0;
static const float kDropHighlightLineHeight = 2.0;

static const uint32 kToggleColumn = 'BTCL';

class BRowContainer : public BObjectList<BRow>
{
};

class TitleView : public BView {
public:
	TitleView(BRect, OutlineView*, BList *visibleColumns, BList *sortColumns,
		BColumnListView *masterView, uint32 resizingMode);
	~TitleView();
	void ColumnAdded(BColumn*);
	void SetColumnVisible(BColumn*, bool);

	virtual void Draw(BRect rect);
	virtual void ScrollTo(BPoint);
	virtual void MessageReceived(BMessage*);
	virtual void MouseDown(BPoint);
	virtual void MouseMoved(BPoint, uint32, const BMessage*);
	virtual void MouseUp(BPoint position);
	virtual void FrameResized(float width, float height);
	
	void MoveColumn(BColumn *column, int32 index);
	void SetColumnFlags(column_flags flags);

	void SetEditMode(bool state) { fEditMode = state; }

private:
	void GetTitleRect(BColumn*, BRect *out_rect);
	int32 FindColumn(BPoint, float *out_leftEdge);
	void FixScrollBar(bool scrollToFit);
	void DragSelectedColumn(BPoint);
	void ResizeSelectedColumn(BPoint);
	void ComputeDragBoundries(BColumn*, BPoint);
	void DrawTitle(BView*, BRect, BColumn*, bool depressed);

	OutlineView *fOutlineView;
	BList *fColumns;
	BList *fSortColumns;
	float fColumnsWidth;
	BRect fVisibleRect;
	
#if DOUBLE_BUFFERED_COLUMN_RESIZE
	BBitmap *fDrawBuffer;
	BView *fDrawBufferView;
#endif

	enum {
		INACTIVE,
		RESIZING_COLUMN,
		PRESSING_COLUMN,
		DRAG_COLUMN_INSIDE_TITLE,
		DRAG_COLUMN_OUTSIDE_TITLE
	} fCurrentState;

	BPopUpMenu *fColumnPop;
	BColumnListView *fMasterView;
	bool fEditMode;
	int32 fColumnFlags;

	// State information for resizing/dragging
	BColumn *fSelectedColumn;
	BRect fSelectedColumnRect;
	bool fResizingFirstColumn;
	BPoint fClickPoint;			// offset within cell
	float fLeftDragBoundry;
	float fRightDragBoundry;
	BPoint fCurrentDragPosition;

	
	BBitmap *fUpSortArrow;
	BBitmap *fDownSortArrow;
#if _INCLUDES_CLASS_CURSOR
	BCursor *fResizeCursor;
	BCursor *fMinResizeCursor;
	BCursor *fMaxResizeCursor;
	BCursor *fColumnMoveCursor;
#endif
	
	
	typedef BView _inherited;
};

class OutlineView : public BView {
public:
	OutlineView(BRect, BList *visibleColumns, BList *sortColumns, BColumnListView *listView);
	~OutlineView();

	virtual void				Draw(BRect);
	const 	BRect&				VisibleRect() const;

			void				RedrawColumn(BColumn *column, float leftEdge, bool isFirstColumn);
			void 				StartSorting();
	
			void				AddRow(BRow*, int32 index, BRow *TheRow);
			BRow*				CurrentSelection(BRow *lastSelected) const;
			void 				ToggleFocusRowSelection(bool selectRange);
			void 				ToggleFocusRowOpen();
			void 				ChangeFocusRow(bool up, bool updateSelection, bool addToCurrentSelection);
			void 				MoveFocusToVisibleRect();
			void 				ExpandOrCollapse(BRow *parent, bool expand);
			void 				RemoveRow(BRow*);
			BRowContainer*		RowList();
			void				UpdateRow(BRow*);
			bool				FindParent(BRow *row, BRow **out_parent, bool *out_isVisible);
			int32				IndexOf(BRow *row);
			void				Deselect(BRow*);
			void				AddToSelection(BRow*);
			void				DeselectAll();
			BRow*				FocusRow() const;
			void				SetFocusRow(BRow *row, bool select);
			BRow*				FindRow(float ypos, int32 *out_indent, float *out_top);
			bool				FindRect(const BRow *row, BRect *out_rect);
			void				ScrollTo(const BRow* Row);

			void				Clear();
			void				SetSelectionMode(list_view_type);
			list_view_type		SelectionMode() const;
			void				SetMouseTrackingEnabled(bool);
			void				FixScrollBar(bool scrollToFit);
			void				SetEditMode(bool state) { fEditMode = state; }

	virtual void				FrameResized(float width, float height);
	virtual void				ScrollTo(BPoint pt);
	virtual void				MouseDown(BPoint);
	virtual void				MouseMoved(BPoint, uint32, const BMessage*);
	virtual void				MouseUp(BPoint);
	virtual void				MessageReceived(BMessage*);

private:
			bool				SortList(BRowContainer *list, bool isVisible);
	static	int32				DeepSortThreadEntry(void *outlineView);
			void				DeepSort();
			void				SelectRange(BRow *start, BRow *end);
			int32				CompareRows(BRow *row1, BRow *row2);
			void				AddSorted(BRowContainer *list, BRow *row);
			void				RecursiveDeleteRows(BRowContainer *list, bool owner);
			void				InvalidateCachedPositions();
			bool				FindVisibleRect(BRow *row, BRect *out_rect);

	BList*			fColumns;
	BList*			fSortColumns;
	float			fItemsHeight;
	BRowContainer	fRows;
	BRect			fVisibleRect;

#if DOUBLE_BUFFERED_COLUMN_RESIZE
	BBitmap*		fDrawBuffer;
	BView*			fDrawBufferView;
#endif

	BRow*			fFocusRow;
	BRect			fFocusRowRect;
	BRow*			fRollOverRow;

	BRow			fSelectionListDummyHead;
	BRow*			fLastSelectedItem;
	BRow*			fFirstSelectedItem;

	thread_id		fSortThread;
	int32			fNumSorted;
	bool			fSortCancelled;

	enum CurrentState
	{
		INACTIVE,
		LATCH_CLICKED,
		ROW_CLICKED,
		DRAGGING_ROWS
	};
	
	CurrentState fCurrentState;
	

	BColumnListView*	fMasterView;
	list_view_type		fSelectionMode;
	bool				fTrackMouse;
	BField*				fCurrentField;
	BRow*				fCurrentRow;
	BColumn*			fCurrentColumn;
	bool				fMouseDown;
	BRect				fFieldRect;
	int32				fCurrentCode;
	bool				fEditMode;

	// State information for mouse/keyboard interaction
	BPoint fClickPoint;
	bool fDragging;
	int32 fClickCount;
	BRow *fTargetRow;
	float fTargetRowTop;
	BRect fLatchRect;
	float fDropHighlightY;

	friend class RecursiveOutlineIterator;
	typedef BView _inherited;
};

class RecursiveOutlineIterator {
public:
	RecursiveOutlineIterator(BRowContainer*, bool openBranchesOnly = true);
	BRow *CurrentRow() const;
	int32 CurrentLevel() const;
	void GoToNext();

private:
	struct {
		BRowContainer *fRowSet;
		int32 fIndex;
		int32 fDepth;
	} fStack[kMaxDepth];

	int32 fStackIndex;
	BRowContainer *fCurrentList;
	int32 fCurrentListIndex;
	int32 fCurrentListDepth;
	bool fOpenBranchesOnly;
};

}	// namespace BPrivate

using namespace BPrivate;

BField::BField()
{
}

BField::~BField()
{
}

// #pragma mark -

void BColumn::MouseMoved(BColumnListView */*parent*/, BRow */*row*/, BField */*field*/, 
						BRect /*field_rect*/, BPoint/*point*/, uint32 /*buttons*/, int32 /*code*/)
{
}

void BColumn::MouseDown( BColumnListView */*parent*/, BRow */*row*/, BField */*field*/,
						BRect /*field_rect*/, BPoint /*point*/, uint32 /*buttons*/)
{
}

void BColumn::MouseUp(BColumnListView */*parent*/, BRow */*row*/, BField */*field*/)
{
}

// #pragma mark -

BRow::BRow(float height)
	:	fChildList(NULL),
		fIsExpanded(false),
		fHeight(height),
		fNextSelected(NULL),
		fPrevSelected(NULL),
		fParent(NULL),
		fList(NULL)
{
}

BRow::~BRow()
{
	while (true) {
		BField *field = (BField*) fFields.RemoveItem(0L);
		if (field == 0)
			break;
		
		delete field;
	}		
}

bool BRow::HasLatch() const
{
	return fChildList != 0;
}

int32 BRow::CountFields() const
{
	return fFields.CountItems();
}

BField* BRow::GetField(int32 index)
{
	return (BField*) fFields.ItemAt(index);
}

const BField* BRow::GetField(int32 index) const
{
	return (const BField*) fFields.ItemAt(index);
}

void BRow::SetField(BField *field, int32 logicalFieldIndex)
{
	if (fFields.ItemAt(logicalFieldIndex) != 0)
		delete (BField*) fFields.RemoveItem(logicalFieldIndex);
	
	if( NULL != fList ) {
		ValidateField(field, logicalFieldIndex);
		BRect inv;
		fList->GetRowRect(this, &inv);
		fList->Invalidate(inv);
	}
	
	fFields.AddItem(field, logicalFieldIndex);
}

float BRow::Height() const
{
	return fHeight;
}

bool BRow::IsExpanded() const
{
	return fIsExpanded;
}

void 
BRow::ValidateFields() const
{
	for( int32 i = 0; i < CountFields(); i++ )
	{
		ValidateField(GetField(i), i);
	}
}

void 
BRow::ValidateField(const BField *field, int32 logicalFieldIndex) const
{
	// The Fields may be moved by the user, but the logicalFieldIndexes
	// do not change, so we need to map them over when checking the
	// Field types.
	BColumn* col = NULL;
	int32 items = fList->CountColumns();
	for( int32 i = 0 ; i < items; ++i )
	{
		col = fList->ColumnAt(i);
		if( col->LogicalFieldNum() == logicalFieldIndex )
			break;
	}
	
	if( NULL == col )
	{
		BString dbmessage("\n\n\tThe parent BColumnListView does not have "
		                  "\n\ta BColumn at the logical field index ");
		dbmessage << logicalFieldIndex << ".\n\n";
		printf(dbmessage.String());
	}
	else
	{
		if( false == col->AcceptsField(field) )
		{
			BString dbmessage("\n\n\tThe BColumn of type ");
			dbmessage << typeid(*col).name() << "\n\tat logical field index "
			          << logicalFieldIndex << "\n\tdoes not support the field type "
			          << typeid(*field).name() << ".\n\n";
			debugger(dbmessage.String());
		}
	}
}


// #pragma mark -

BColumn::BColumn(float width, float minWidth, float maxWidth, alignment align)
	:	fWidth(width),
		fMinWidth(minWidth),
		fMaxWidth(maxWidth),
		fVisible(true),
		fList(0),
		fShowHeading(true),
		fAlignment(align)
{
}

BColumn::~BColumn()
{
}

float BColumn::Width() const
{
	return fWidth;
}

void BColumn::SetWidth(float width)
{
	fWidth = width;
}

float BColumn::MinWidth() const
{
	return fMinWidth;
}

float BColumn::MaxWidth() const
{
	return fMaxWidth;
}

void BColumn::DrawTitle(BRect, BView*)
{
}

void BColumn::DrawField(BField*, BRect, BView*)
{
}

int BColumn::CompareFields(BField *, BField *)
{
	return 0;
}

void BColumn::GetColumnName(BString* into) const
{
	*into = "(Unnamed)";
}

bool BColumn::IsVisible() const
{
	return fVisible;
}

void BColumn::SetVisible(bool visible)
{
	if (fList && (fVisible != visible))
		fList->SetColumnVisible(this, visible);
}

bool BColumn::ShowHeading() const
{
	return fShowHeading;
}

void BColumn::SetShowHeading(bool state)
{
	fShowHeading = state;
}

alignment BColumn::Alignment() const
{
	return fAlignment;
}

void BColumn::SetAlignment(alignment align)
{
	fAlignment = align;
}

bool BColumn::WantsEvents() const
{
	return fWantsEvents;
}

void BColumn::SetWantsEvents(bool state)
{
	fWantsEvents = state;
}

int32 BColumn::LogicalFieldNum() const
{
	return fFieldID;
}

bool 
BColumn::AcceptsField(const BField*) const
{
	return true;
}


// #pragma mark -

BColumnListView::BColumnListView(BRect rect, const char *name, uint32 resizingMode,
	uint32 drawFlags, border_style border, bool showHorizontalScrollbar)
	:	BView(rect, name, resizingMode, drawFlags | B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE),
		fStatusView(0),
		fSelectionMessage(0),
		fSortingEnabled(true),
		fLatchWidth(kLatchWidth),
		fBorderStyle(border)
{
	SetViewColor(B_TRANSPARENT_32_BIT);

	BRect bounds(rect);
	bounds.OffsetTo(0, 0);
	
	for (int i = 0; i < (int)B_COLOR_TOTAL; i++)
	  fColorList[i] = kColor[i];
	
	BRect titleRect(bounds);
	titleRect.bottom = titleRect.top + kTitleHeight;
#if !LOWER_SCROLLBAR
	titleRect.right -= B_V_SCROLL_BAR_WIDTH + 1;
#endif

	BRect outlineRect(bounds);
	outlineRect.top = titleRect.bottom + 1.0;
	outlineRect.right -= B_V_SCROLL_BAR_WIDTH + 1;
	if(showHorizontalScrollbar)
		outlineRect.bottom -= B_H_SCROLL_BAR_HEIGHT + 1;

	BRect vScrollBarRect(bounds);
#if LOWER_SCROLLBAR
	vScrollBarRect.top += kTitleHeight;
#endif

	vScrollBarRect.left = vScrollBarRect.right - B_V_SCROLL_BAR_WIDTH;
	if(showHorizontalScrollbar)
		vScrollBarRect.bottom -= B_H_SCROLL_BAR_HEIGHT;

	BRect hScrollBarRect(bounds);
	hScrollBarRect.top = hScrollBarRect.bottom - B_H_SCROLL_BAR_HEIGHT;
	hScrollBarRect.right -= B_V_SCROLL_BAR_WIDTH;

	// Adjust stuff so the border will fit.
	if (fBorderStyle == B_PLAIN_BORDER) {
		titleRect.InsetBy(1, 0);
		titleRect.top++;
		outlineRect.InsetBy(1, 0);
		outlineRect.bottom--;

		vScrollBarRect.OffsetBy(-1, 0);
		vScrollBarRect.InsetBy(0, 1);
		hScrollBarRect.OffsetBy(0, -1);
		hScrollBarRect.InsetBy(1, 0);
	} else if (fBorderStyle == B_FANCY_BORDER) {
		titleRect.InsetBy(2, 0);
		titleRect.top += 2;
		outlineRect.InsetBy(2, 0);
		outlineRect.bottom -= 2;

		vScrollBarRect.OffsetBy(-2, 0);
		vScrollBarRect.InsetBy(0, 2);
		hScrollBarRect.OffsetBy(0, -2);
		hScrollBarRect.InsetBy(2, 0);
	}
	
	fOutlineView = new OutlineView(outlineRect, &fColumns, &fSortColumns, this);
	AddChild(fOutlineView);

	
	// Adapt to correct resizing mode
	uint32 fParentFlags = resizingMode;
	// Always follow LEFT_RIGHT
	uint32 fTitleFlags = B_FOLLOW_LEFT_RIGHT;
	
	if ((fParentFlags & B_FOLLOW_TOP) && (fParentFlags & ~B_FOLLOW_BOTTOM)) {
		fTitleFlags |= B_FOLLOW_TOP;
	}
	else if ((fParentFlags & B_FOLLOW_BOTTOM) && (fParentFlags & ~B_FOLLOW_TOP)) {
		fTitleFlags |= B_FOLLOW_BOTTOM;
	}
	
	fTitleView = new TitleView(titleRect, fOutlineView, &fColumns, &fSortColumns, this, fTitleFlags);


	AddChild(fTitleView);
	fVerticalScrollBar = new BScrollBar(vScrollBarRect, "vertical_scroll_bar",
		fOutlineView, 0.0, bounds.Height(), B_VERTICAL);
	AddChild(fVerticalScrollBar);
	fHorizontalScrollBar = new BScrollBar(hScrollBarRect, "horizontal_scroll_bar",
		fTitleView, 0.0, bounds.Width(), B_HORIZONTAL);
	AddChild(fHorizontalScrollBar);
	if(!showHorizontalScrollbar)
		fHorizontalScrollBar->Hide();
	fOutlineView->FixScrollBar(true);
}

BColumnListView::~BColumnListView()
{
	while (true) {
		BColumn *column = (BColumn*) fColumns.RemoveItem(0L);
		if (column == 0)
			break;
		
		delete column;
	}		
}

bool BColumnListView::InitiateDrag(BPoint, bool)
{
	return false;
}

void BColumnListView::MessageDropped(BMessage*, BPoint)
{
}

void BColumnListView::ExpandOrCollapse(BRow* Row, bool Open)
{
	fOutlineView->ExpandOrCollapse(Row, Open);
}

status_t BColumnListView::Invoke(BMessage *message)
{
	if (message == 0)
		message = Message();

	return BInvoker::Invoke(message);
}

void BColumnListView::ItemInvoked()
{
	Invoke();
}

void BColumnListView::SetInvocationMessage(BMessage *message)
{
	SetMessage(message);
}

BMessage* BColumnListView::InvocationMessage() const
{
	return Message();
}

uint32 BColumnListView::InvocationCommand() const
{
	return Command();
}

BRow* BColumnListView::FocusRow() const
{
	return fOutlineView->FocusRow();
}

void BColumnListView::SetFocusRow(int32 Index, bool Select)
{
	SetFocusRow(RowAt(Index), Select);
}

void BColumnListView::SetFocusRow(BRow* Row, bool Select)
{
	fOutlineView->SetFocusRow(Row, Select);
}

void BColumnListView::SetMouseTrackingEnabled(bool Enabled)
{
	fOutlineView->SetMouseTrackingEnabled(Enabled);	
}

list_view_type BColumnListView::SelectionMode() const
{
	return fOutlineView->SelectionMode();
}

void BColumnListView::Deselect(BRow *row)
{
	fOutlineView->Deselect(row);
}

void BColumnListView::AddToSelection(BRow *row)
{
	fOutlineView->AddToSelection(row);
}

void BColumnListView::DeselectAll()
{
	fOutlineView->DeselectAll();
}

BRow* BColumnListView::CurrentSelection(BRow *lastSelected) const
{
	return fOutlineView->CurrentSelection(lastSelected);
}

void BColumnListView::SelectionChanged()
{
	if (fSelectionMessage)
		Invoke(fSelectionMessage);
}

void BColumnListView::SetSelectionMessage(BMessage *message)
{
	if (fSelectionMessage == message)
		return;
		
	delete fSelectionMessage;
	fSelectionMessage = message;
}

BMessage* BColumnListView::SelectionMessage()
{
	return fSelectionMessage;
}

uint32 BColumnListView::SelectionCommand() const
{
	if (fSelectionMessage)
		return fSelectionMessage->what;
		
	return 0;
}

void BColumnListView::SetSelectionMode(list_view_type mode)
{
	fOutlineView->SetSelectionMode(mode);
}

void BColumnListView::SetSortingEnabled(bool enabled)
{
	fSortingEnabled = enabled;
	fSortColumns.MakeEmpty();
	fTitleView->Invalidate();	// Erase sort indicators
}

bool BColumnListView::SortingEnabled() const
{
	return fSortingEnabled;
}

void BColumnListView::SetSortColumn(BColumn *column, bool add, bool ascending)
{
	if (!SortingEnabled())
		return;

	if (!add)
		fSortColumns.MakeEmpty();

	if (!fSortColumns.HasItem(column))
		fSortColumns.AddItem(column);

	column->fSortAscending = ascending;
	fTitleView->Invalidate();	
	fOutlineView->StartSorting();
}

void BColumnListView::ClearSortColumns()
{
	fSortColumns.MakeEmpty();
	fTitleView->Invalidate();	// Erase sort indicators
}

void BColumnListView::AddStatusView(BView *view)
{
	BRect bounds = Bounds();
	float width = view->Bounds().Width();
	if (width > bounds.Width() / 2)
		width = bounds.Width() / 2;

	fStatusView = view;

	Window()->BeginViewTransaction();
	fHorizontalScrollBar->ResizeBy(-(width + 1), 0);
	fHorizontalScrollBar->MoveBy((width + 1), 0);
	AddChild(view);

	BRect viewRect(bounds);
	viewRect.right = width;
	viewRect.top = viewRect.bottom - B_H_SCROLL_BAR_HEIGHT;
	if (fBorderStyle == B_PLAIN_BORDER)
		viewRect.OffsetBy(1, -1);
	else if (fBorderStyle == B_FANCY_BORDER)
		viewRect.OffsetBy(2, -2);
	
	view->SetResizingMode(B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	view->ResizeTo(viewRect.Width(), viewRect.Height());
	view->MoveTo(viewRect.left, viewRect.top);
	Window()->EndViewTransaction();
}

BView* BColumnListView::RemoveStatusView()
{
	if (fStatusView) {
		float width = fStatusView->Bounds().Width();
		Window()->BeginViewTransaction();
		fStatusView->RemoveSelf();
		fHorizontalScrollBar->MoveBy(-width, 0);
		fHorizontalScrollBar->ResizeBy(width, 0);
		Window()->EndViewTransaction();
	}

	BView *view = fStatusView;
	fStatusView = 0;
	return view;
}
void BColumnListView::AddColumn(BColumn *column, int32 logicalFieldIndex)
{
	ASSERT(column != 0);

	column->fList = this;
	column->fFieldID = logicalFieldIndex;

	// sanity check.  If there is already a field with this ID, remove it.
	for (int32 index = 0; index < fColumns.CountItems(); index++) {
		BColumn *existingColumn = (BColumn*) fColumns.ItemAt(index);
		if (existingColumn && existingColumn->fFieldID == logicalFieldIndex) {
			RemoveColumn(existingColumn);
			break;
		}
	}

	if (column->Width() < column->MinWidth())
		column->SetWidth(column->MinWidth());
	else if (column->Width() > column->MaxWidth())
		column->SetWidth(column->MaxWidth());

	fColumns.AddItem((void*) column);
	fTitleView->ColumnAdded(column);
}

void BColumnListView::MoveColumn(BColumn *column, int32 index)
{
	ASSERT(column != 0);
	fTitleView->MoveColumn(column, index);
}

void BColumnListView::RemoveColumn(BColumn *column)
{
	if (fColumns.HasItem(column)) {
		SetColumnVisible(column, false);
		Window()->UpdateIfNeeded();
		fColumns.RemoveItem(column);
	}
}

int32 BColumnListView::CountColumns() const
{
	return fColumns.CountItems();
}

BColumn* BColumnListView::ColumnAt(int32 field) const
{
	return (BColumn*) fColumns.ItemAt(field);
}

void BColumnListView::SetColumnVisible(BColumn *column, bool visible)
{
	fTitleView->SetColumnVisible(column, visible);
}

void BColumnListView::SetColumnVisible(int32 index, bool isVisible)
{
	BColumn *column = ColumnAt(index);
	if (column)
		column->SetVisible(isVisible);
}

bool BColumnListView::IsColumnVisible(int32 index) const
{
	BColumn *column = ColumnAt(index);
	if (column)
		return column->IsVisible();

	return false;
}

void BColumnListView::SetColumnFlags(column_flags flags)
{
	fTitleView->SetColumnFlags(flags);
}

const BRow* BColumnListView::RowAt(int32 Index, BRow* ParentRow) const
{
	if (ParentRow == 0)
		return fOutlineView->RowList()->ItemAt(Index);	

	return ParentRow->fChildList ? ParentRow->fChildList->ItemAt(Index) : NULL;
}

BRow* BColumnListView::RowAt(int32 Index, BRow* ParentRow)
{
	if (ParentRow == 0)
		return fOutlineView->RowList()->ItemAt(Index);	

	return ParentRow->fChildList ? ParentRow->fChildList->ItemAt(Index) : 0;
}

const BRow* BColumnListView::RowAt(BPoint point) const
{
	float top;
	int32 indent;
	return fOutlineView->FindRow(point.y, &indent, &top);
}

BRow* BColumnListView::RowAt(BPoint point)
{
	float top;
	int32 indent;
	return fOutlineView->FindRow(point.y, &indent, &top);
}

bool BColumnListView::GetRowRect(const BRow *row, BRect *outRect) const
{
	return fOutlineView->FindRect(row, outRect);
}

bool BColumnListView::FindParent(BRow *row, BRow **out_parent, bool *out_isVisible) const
{
	return fOutlineView->FindParent(row, out_parent, out_isVisible);
}

int32 BColumnListView::IndexOf(BRow *row)
{
	return fOutlineView->IndexOf(row);
}

int32 BColumnListView::CountRows(BRow* ParentRow) const
{
	if (ParentRow == 0)
		return fOutlineView->RowList()->CountItems();	
	if (ParentRow->fChildList)
		return ParentRow->fChildList->CountItems();
	else
		return 0;
}

void BColumnListView::AddRow(BRow *row, BRow* ParentRow)
{
	AddRow(row, -1, ParentRow);
}

void BColumnListView::AddRow(BRow *row, int32 index, BRow* ParentRow)
{
	row->fChildList = 0;
	row->fList = this;
	row->ValidateFields();
	fOutlineView->AddRow(row, index, ParentRow);
}

void BColumnListView::RemoveRow(BRow *row)
{
	fOutlineView->RemoveRow(row);
	row->fList = NULL;
}

void BColumnListView::UpdateRow(BRow *row)
{
	fOutlineView->UpdateRow(row);
}

void BColumnListView::ScrollTo(const BRow* Row)
{
	fOutlineView->ScrollTo(Row);
}

void BColumnListView::Clear()
{
	fOutlineView->Clear();
}

void BColumnListView::SetFont(const BFont *font, uint32 mask)
{
	// This method is deprecated.
	fOutlineView->SetFont(font, mask);
	fTitleView->SetFont(font, mask);
}

void BColumnListView::SetFont(ColumnListViewFont font_num, const BFont* font, uint32 mask)
{
	switch (font_num) {
		case B_FONT_ROW:
			fOutlineView->SetFont(font, mask);
			break;
		  
		case B_FONT_HEADER:
			fTitleView->SetFont(font, mask);
			break;
			
		default:
			ASSERT(false);
			break;	
	};
}

void BColumnListView::GetFont(ColumnListViewFont font_num, BFont* font) const
{
	switch (font_num) {
		case B_FONT_ROW:
			fOutlineView->GetFont(font);
			break;
		  
		case B_FONT_HEADER:
			fTitleView->GetFont(font);
			break;
			
		default:
			ASSERT(false);
			break;	
	};
}

void BColumnListView::SetColor(ColumnListViewColor color_num, const rgb_color color)
{
	if ((int)color_num < 0)
	{
		ASSERT(false);
		color_num = (ColumnListViewColor) 0;
	}	
		
	if ((int)color_num >= (int)B_COLOR_TOTAL)
	{
		ASSERT(false);
		color_num = (ColumnListViewColor) (B_COLOR_TOTAL - 1);
	}
  
	fColorList[color_num] = color;
}

rgb_color BColumnListView::Color(ColumnListViewColor color_num) const
{
	if ((int)color_num < 0)
	{
		ASSERT(false);
		color_num = (ColumnListViewColor) 0;
	}	
		
	if ((int)color_num >= (int)B_COLOR_TOTAL)
	{
		ASSERT(false);
		color_num = (ColumnListViewColor) (B_COLOR_TOTAL - 1);
	}
  
	return fColorList[color_num];
}

void BColumnListView::SetHighColor(rgb_color color)
{
	BView::SetHighColor(color);
//	fOutlineView->Invalidate();	// Redraw things with the new color
								// Note that this will currently cause
								// an infinite loop, refreshing over and over.
								// A better solution is needed.
}

void BColumnListView::SetSelectionColor(rgb_color color)
{
	fColorList[B_COLOR_SELECTION] = color;
}

void BColumnListView::SetBackgroundColor(rgb_color color)
{
	fColorList[B_COLOR_BACKGROUND] = color;
	fOutlineView->Invalidate();	// Repaint with new color
}

void BColumnListView::SetEditColor(rgb_color color)
{
	fColorList[B_COLOR_EDIT_BACKGROUND] = color;
}

const rgb_color BColumnListView::SelectionColor() const
{
	return fColorList[B_COLOR_SELECTION];
}

const rgb_color BColumnListView::BackgroundColor() const
{
	return fColorList[B_COLOR_BACKGROUND];
}

const rgb_color BColumnListView::EditColor() const
{
	return fColorList[B_COLOR_EDIT_BACKGROUND];
}

BPoint BColumnListView::SuggestTextPosition(const BRow* row, const BColumn* inColumn) const
{
	BRect rect;
	GetRowRect(row, &rect);
	if (inColumn) {
		float leftEdge = MAX(kLeftMargin, LatchWidth());
		for (int index = 0; index < fColumns.CountItems(); index++) {
			BColumn *column = (BColumn*) fColumns.ItemAt(index);
			if (!column->IsVisible())
				continue;
			
			if (column == inColumn) {
				rect.left = leftEdge;
				rect.right = rect.left + column->Width();
				break;
			}
			
			leftEdge += column->Width() + 1;
		}
	}
	
	font_height fh;
	fOutlineView->GetFontHeight(&fh);
	float baseline = floor(rect.top + fh.ascent
							+ (rect.Height()+1-(fh.ascent+fh.descent))/2);
	return BPoint(rect.left + 8, baseline);
}

void BColumnListView::SetLatchWidth(float width)
{
	fLatchWidth = width;
	Invalidate();
}

float BColumnListView::LatchWidth() const
{
	return fLatchWidth;
}

void BColumnListView::DrawLatch(BView *view, BRect rect, LatchType position, BRow *)
{
	const int32 rectInset = 4;
	
	view->SetHighColor(0, 0, 0);
	
	// Make Square
	int32 sideLen = rect.IntegerWidth();
	if( sideLen > rect.IntegerHeight() )
	{
		sideLen = rect.IntegerHeight();
	}
	
	// Make Center
	int32 halfWidth  = rect.IntegerWidth() / 2;
	int32 halfHeight = rect.IntegerHeight() / 2;
	int32 halfSide   = sideLen / 2;
	
	float left = rect.left + halfWidth  - halfSide;
	float top  = rect.top  + halfHeight - halfSide;
	
	BRect itemRect(left, top, left + sideLen, top + sideLen);
	
	// Why it is a pixel high? I don't know.
	itemRect.OffsetBy(0, -1);
	
	itemRect.InsetBy(rectInset, rectInset);

	// Make it an odd number of pixels wide, the latch looks better this way
	if (1 == (itemRect.IntegerWidth() % 2))
	{
		itemRect.right  += 1;
		itemRect.bottom += 1;
	}
		
	switch (position) {
		case B_OPEN_LATCH:
			view->StrokeRect(itemRect);
			view->StrokeLine(BPoint(itemRect.left + 2, (itemRect.top + itemRect.bottom) / 2),
				BPoint(itemRect.right - 2, (itemRect.top + itemRect.bottom) / 2));
			break;
			
		case B_PRESSED_LATCH:
			view->StrokeRect(itemRect);
			view->StrokeLine(BPoint(itemRect.left + 2, (itemRect.top + itemRect.bottom) / 2),
				BPoint(itemRect.right - 2, (itemRect.top + itemRect.bottom) / 2));
			view->StrokeLine(BPoint((itemRect.left + itemRect.right) / 2, itemRect.top +  2),
				BPoint((itemRect.left + itemRect.right) / 2, itemRect.bottom - 2));
			view->InvertRect(itemRect);
			break;
			
		case B_CLOSED_LATCH:
			view->StrokeRect(itemRect);
			view->StrokeLine(BPoint(itemRect.left + 2, (itemRect.top + itemRect.bottom) / 2),
				BPoint(itemRect.right - 2, (itemRect.top + itemRect.bottom) / 2));
			view->StrokeLine(BPoint((itemRect.left + itemRect.right) / 2, itemRect.top +  2),
				BPoint((itemRect.left + itemRect.right) / 2, itemRect.bottom - 2));
			break;
	
		case B_NO_LATCH:
			// No drawing
			break;
	}
}

void BColumnListView::MakeFocus(bool isFocus)
{
	Invalidate();	// Redraw focus marks around view
	BView::MakeFocus(isFocus);
}

void BColumnListView::MessageReceived(BMessage *message)
{
	// Propagate mouse wheel messages down to child, so that it can
	// scroll.  Note we have done so, so we don't go into infinite
	// recursion if this comes back up here.
	if (message->what == B_MOUSE_WHEEL_CHANGED) {
		bool handled;
		if (message->FindBool("be:clvhandled", &handled) != B_OK) {
			message->AddBool("be:clvhandled", true);
			fOutlineView->MessageReceived(message);
			return;
		}
	}
	BView::MessageReceived(message);
}

void BColumnListView::KeyDown(const char *bytes, int32 numBytes)
{
	char c = bytes[0];
	switch (c) {
		case B_RIGHT_ARROW: 
		case B_LEFT_ARROW: {
			float  minVal, maxVal;
			fHorizontalScrollBar->GetRange(&minVal, &maxVal);
			float smallStep, largeStep;
			fHorizontalScrollBar->GetSteps(&smallStep, &largeStep);
			float oldVal = fHorizontalScrollBar->Value();
			float newVal = oldVal;

			if (c == B_LEFT_ARROW)
				newVal -= smallStep;
			else if (c == B_RIGHT_ARROW)
				newVal += smallStep;

			if (newVal < minVal)
				newVal = minVal;
			else if (newVal > maxVal)
				newVal = maxVal;

			fHorizontalScrollBar->SetValue(newVal);
			break;
		}

		case B_DOWN_ARROW:
			fOutlineView->ChangeFocusRow(false, (modifiers() & B_CONTROL_KEY) == 0,
				(modifiers() & B_SHIFT_KEY) != 0);
			break;

		case B_UP_ARROW:
			fOutlineView->ChangeFocusRow(true, (modifiers() & B_CONTROL_KEY) == 0,
				(modifiers() & B_SHIFT_KEY) != 0);
			break;
		
		case B_PAGE_UP:
		case B_PAGE_DOWN: {
			float minValue, maxValue;
			fVerticalScrollBar->GetRange(&minValue, &maxValue);
			float smallStep, largeStep;
			fVerticalScrollBar->GetSteps(&smallStep, &largeStep);
			float currentValue = fVerticalScrollBar->Value();
			float newValue = currentValue;
			
			if (c == B_PAGE_UP)
				newValue -= largeStep;
			else
				newValue += largeStep;
			
			if (newValue > maxValue)
				newValue = maxValue;
			else if (newValue < minValue)
				newValue = minValue;
				
			fVerticalScrollBar->SetValue(newValue);

			// Option + pgup or pgdn scrolls and changes the selection.
			if (modifiers() & B_OPTION_KEY)
				fOutlineView->MoveFocusToVisibleRect();
			
			break;
		}
			
		case B_ENTER:
			Invoke();
			break;

		case B_SPACE:
			fOutlineView->ToggleFocusRowSelection((modifiers() & B_SHIFT_KEY) != 0);
			break;

		case '+':
			fOutlineView->ToggleFocusRowOpen();			
			break;

		default:
			BView::KeyDown(bytes, numBytes);
	}
}

void BColumnListView::AttachedToWindow()
{
	if (!Messenger().IsValid())
		SetTarget(Window());
		
	if (SortingEnabled()) fOutlineView->StartSorting();	
}

void BColumnListView::WindowActivated(bool active)
{
	fOutlineView->Invalidate();
		// Focus and selection appearance changes with focus

	Invalidate(); 	// Redraw focus marks around view
	BView::WindowActivated(active);
}

void BColumnListView::Draw(BRect)
{
	BRect rect = Bounds();
	PushState();

	BRect cornerRect(rect.right - B_V_SCROLL_BAR_WIDTH, rect.bottom - B_H_SCROLL_BAR_HEIGHT,
		rect.right, rect.bottom);
	if (fBorderStyle == B_PLAIN_BORDER) {
		BView::SetHighColor(0, 0, 0); 
		StrokeRect(rect);
		cornerRect.OffsetBy(-1, -1); 
	} else if (fBorderStyle == B_FANCY_BORDER) {
		bool isFocus = IsFocus() && Window()->IsActive();

		if (isFocus)
			BView::SetHighColor(0, 0, 190);	// Need to find focus color programatically
		else
			BView::SetHighColor(255, 255, 255);

		StrokeRect(rect);
		if (!isFocus)
			BView::SetHighColor(184, 184, 184);
		else
			BView::SetHighColor(152, 152, 152);
			
		rect.InsetBy(1,1);
		StrokeRect(rect);
		cornerRect.OffsetBy(-2, -2);
	}
	
	BView::SetHighColor(215, 215, 215); // fills lower right rect between scroll bars
	FillRect(cornerRect);
	PopState();
}

void BColumnListView::SaveState(BMessage *msg)
{
	msg->MakeEmpty();

	// Damn compiler issuing l43m incorrect warnings.
	int i;
	for (i = 0; ;i++)
	{
		BColumn *col = (BColumn*) fColumns.ItemAt(i);
		if(!col)
			break;
		msg->AddInt32("ID",col->fFieldID);
		msg->AddFloat("width",col->fWidth);
		msg->AddBool("visible",col->fVisible);
	}

	msg->AddBool("sortingenabled",fSortingEnabled);

	if(fSortingEnabled)
	{
		for (i = 0; ;i++)
		{
			BColumn *col = (BColumn*) fSortColumns.ItemAt(i);
			if(!col)
				break;
			msg->AddInt32("sortID",col->fFieldID);
			msg->AddBool("sortascending",col->fSortAscending);
		}
	}
}

void BColumnListView::LoadState(BMessage *msg)
{
	for(int i=0;;i++)
	{
		int32 ID;
		if(B_OK!=msg->FindInt32("ID",i,&ID))
			break;
		for(int j=0;;j++)
		{
			BColumn *col = (BColumn*) fColumns.ItemAt(j);
			if(!col)
				break;
			if(col->fFieldID==ID)
			{
				// move this column to position 'i' and set its attributes
				MoveColumn(col,i);
				float f;
				if(B_OK==msg->FindFloat("width",i,&f))
					col->SetWidth(f);
				bool b;
				if(B_OK==msg->FindBool("visible",i,&b))
					col->SetVisible(b);
			}
		}
	}
	bool b;
	if(B_OK==msg->FindBool("sortingenabled",&b))
	{
		SetSortingEnabled(b);
		// Damn compiler issuing l43m incorrect warnings.
		for(int k=0;;k++)
		{
			int32 ID;
			if(B_OK!=msg->FindInt32("sortID", k, &ID))
				break;
			for(int j=0;;j++)
			{
				BColumn *col = (BColumn*) fColumns.ItemAt(j);
				if(!col)
					break;
				if(col->fFieldID==ID)
				{
					// add this column to the sort list
					bool val;
					if(B_OK==msg->FindBool("sortascending", k, &val))
						SetSortColumn(col, true, val);
				}
			}
		}
	}
}

void BColumnListView::SetEditMode(bool state)
{
	fOutlineView->SetEditMode(state);
	fTitleView->SetEditMode(state);
}

void BColumnListView::Refresh()
{
	if(LockLooper())
	{
		Invalidate();
		fOutlineView->FixScrollBar (true);
		fOutlineView->Invalidate();
		Window()->UpdateIfNeeded();
		UnlockLooper();
	}
}

// #pragma mark -


TitleView::TitleView(BRect rect, OutlineView *horizontalSlave, BList *visibleColumns,
		BList *sortColumns, BColumnListView *listView, uint32 resizingMode)
	:	BView(rect, "title_view", resizingMode, B_WILL_DRAW | B_FRAME_EVENTS),
		fOutlineView(horizontalSlave),
		fColumns(visibleColumns),
		fSortColumns(sortColumns),
		fColumnsWidth(0),
		fVisibleRect(rect.OffsetToCopy(0, 0)),
		fCurrentState(INACTIVE),
		fColumnPop(NULL),
		fMasterView(listView),
		fEditMode(false),
		fColumnFlags(B_ALLOW_COLUMN_MOVE|B_ALLOW_COLUMN_RESIZE|B_ALLOW_COLUMN_POPUP|B_ALLOW_COLUMN_REMOVE)
{
	SetViewColor(B_TRANSPARENT_32_BIT);
	
#if DOUBLE_BUFFERED_COLUMN_RESIZE
	// xxx this needs to be smart about the size of the backbuffer.
	BRect doubleBufferRect(0, 0, 600, 35);
	fDrawBuffer = new BBitmap(doubleBufferRect, B_RGB32, true);
	fDrawBufferView = new BView(doubleBufferRect, "double_buffer_view",
		B_FOLLOW_ALL_SIDES, 0);
	fDrawBuffer->Lock();
	fDrawBuffer->AddChild(fDrawBufferView);
	fDrawBuffer->Unlock();
#endif

	fUpSortArrow = new BBitmap(BRect(0, 0, 7, 7), B_COLOR_8_BIT);
	fDownSortArrow = new BBitmap(BRect(0, 0, 7, 7), B_COLOR_8_BIT);

	fUpSortArrow->SetBits((const void*) kUpSortArrow8x8, 64, 0, B_COLOR_8_BIT);
	fDownSortArrow->SetBits((const void*) kDownSortArrow8x8, 64, 0, B_COLOR_8_BIT);

#if _INCLUDES_CLASS_CURSOR
	fResizeCursor = new BCursor(kResizeCursorData);
	fMinResizeCursor = new BCursor(kMinResizeCursorData);
	fMaxResizeCursor = new BCursor(kMaxResizeCursorData);
	fColumnMoveCursor = new BCursor(kColumnMoveCursorData);
#endif
	FixScrollBar(true);
}

TitleView::~TitleView()
{
	delete fColumnPop;
	fColumnPop = NULL;
	
	fDrawBuffer->Lock();
	fDrawBufferView->RemoveSelf();
	fDrawBuffer->Unlock();
	delete fDrawBufferView;
	delete fDrawBuffer;
	delete fUpSortArrow;
	delete fDownSortArrow;

#if _INCLUDES_CLASS_CURSOR
	delete fResizeCursor;
	delete fMaxResizeCursor;
	delete fMinResizeCursor;
	delete fColumnMoveCursor;
#endif
}

void TitleView::ColumnAdded(BColumn *column)
{
	fColumnsWidth += column->Width() + 1;
	FixScrollBar(false);
	Invalidate();
}
// Could use a CopyBits here.
void TitleView::SetColumnVisible(BColumn *column, bool visible)
{
	if (column->fVisible == visible)
		return;

	// If setting it visible, do this first so we can find its position
	// to invalidate.  If hiding it, do it last.
	if (visible)
		column->fVisible = visible;
		
	BRect titleInvalid;
	GetTitleRect(column, &titleInvalid);

	// Now really set the visibility	
	column->fVisible = visible;
	
	if (visible)
		fColumnsWidth += column->Width();
	else
		fColumnsWidth -= column->Width();
	
	BRect outlineInvalid(fOutlineView->VisibleRect());
	outlineInvalid.left = titleInvalid.left;
	titleInvalid.right = outlineInvalid.right;

	Invalidate(titleInvalid);
	fOutlineView->Invalidate(outlineInvalid);
}

void TitleView::GetTitleRect(BColumn *findColumn, BRect *out_rect)
{
	float leftEdge = MAX(kLeftMargin, fMasterView->LatchWidth());
	int32 numColumns = fColumns->CountItems();
	for (int index = 0; index < numColumns; index++) {
		BColumn *column = (BColumn*) fColumns->ItemAt(index);
		if (!column->IsVisible())
			continue;

		if (column == findColumn) {
			out_rect->Set(leftEdge, 0, leftEdge + column->Width(), fVisibleRect.bottom);
			return;
		}
		
		leftEdge += column->Width() + 1;
	}

	TRESPASS();
}

int32 TitleView::FindColumn(BPoint position, float *out_leftEdge)
{
	float leftEdge = MAX(kLeftMargin, fMasterView->LatchWidth());
	int32 numColumns = fColumns->CountItems();
	for (int index = 0; index < numColumns; index++) {
		BColumn *column = (BColumn*) fColumns->ItemAt(index);
		if (!column->IsVisible())
			continue;

		if (leftEdge > position.x)
			break;

		if (position.x >= leftEdge && position.x <= leftEdge + column->Width()) {
			*out_leftEdge = leftEdge;
			return index;
		}

		leftEdge += column->Width() + 1;
	}

	return 0;
}

void TitleView::FixScrollBar(bool scrollToFit)
{
	BScrollBar *hScrollBar = ScrollBar(B_HORIZONTAL);
	if (hScrollBar) {
		float virtualWidth = fColumnsWidth + MAX(kLeftMargin, fMasterView->LatchWidth()) +
			kRightMargin * 2;

		if (virtualWidth > fVisibleRect.Width()) {
			hScrollBar->SetProportion(fVisibleRect.Width() / virtualWidth);

			// Perform the little trick if the user is scrolled over too far.
			// See OutlineView::FixScrollBar for a more in depth explanation
			float maxScrollBarValue = virtualWidth - fVisibleRect.Width();
			if (scrollToFit || hScrollBar->Value() <= maxScrollBarValue) {
				hScrollBar->SetRange(0.0, maxScrollBarValue);
				hScrollBar->SetSteps(50, fVisibleRect.Width());
			}
		} else if (hScrollBar->Value() == 0.0) 
			hScrollBar->SetRange(0.0, 0.0);	// disable scroll bar.
	}
}

void TitleView::DragSelectedColumn(BPoint position)
{
	float invalidLeft = fSelectedColumnRect.left;
	float invalidRight = fSelectedColumnRect.right;

	float leftEdge;
	int32 columnIndex = FindColumn(position, &leftEdge);
	fSelectedColumnRect.OffsetTo(leftEdge, 0);

	MoveColumn(fSelectedColumn, columnIndex);

	fSelectedColumn->fVisible = true;
	ComputeDragBoundries(fSelectedColumn, position);

	// Redraw the new column position
	GetTitleRect(fSelectedColumn, &fSelectedColumnRect);
	invalidLeft = MIN(fSelectedColumnRect.left, invalidLeft);
	invalidRight = MAX(fSelectedColumnRect.right, invalidRight);

	Invalidate(BRect(invalidLeft, 0, invalidRight, fVisibleRect.bottom));
	fOutlineView->Invalidate(BRect(invalidLeft, 0, invalidRight,
		fOutlineView->VisibleRect().bottom));

	DrawTitle(this, fSelectedColumnRect, fSelectedColumn, true);
}

void TitleView::MoveColumn(BColumn *column, int32 index)
{
	fColumns->RemoveItem((void*) column);
	
	if (-1 == index)
	{
		// Re-add the column at the end of the list.
		fColumns->AddItem((void*) column);
	}
	else
	{ 
		fColumns->AddItem((void*) column, index);
	}
}

void TitleView::SetColumnFlags(column_flags flags)
{
	fColumnFlags = flags;
}


void TitleView::ResizeSelectedColumn(BPoint position)
{
	float minWidth = fSelectedColumn->MinWidth();
	float maxWidth = fSelectedColumn->MaxWidth();
	
	float originalEdge = fSelectedColumnRect.left + fSelectedColumn->Width(); 
	if (position.x > fSelectedColumnRect.left + maxWidth)
		fSelectedColumn->SetWidth(maxWidth);
	else if (position.x < fSelectedColumnRect.left + minWidth)
		fSelectedColumn->SetWidth(minWidth);
	else
		fSelectedColumn->SetWidth(position.x - fSelectedColumnRect.left - 1);

	float dX = fSelectedColumnRect.left + fSelectedColumn->Width() - originalEdge;
	if (dX != 0) {
		BRect originalRect(originalEdge, 0, 1000000.0, fVisibleRect.Height());
		BRect movedRect(originalRect);
		movedRect.OffsetBy(dX, 0);

		// Update the size of the title column
		BRect sourceRect(0, 0, fSelectedColumn->Width(), fVisibleRect.Height());
		BRect destRect(sourceRect);
		destRect.OffsetBy(fSelectedColumnRect.left, 0);

#if DOUBLE_BUFFERED_COLUMN_RESIZE
		fDrawBuffer->Lock();
		fDrawBufferView->SetHighColor(fMasterView->Color(B_COLOR_HEADER_BACKGROUND));
		fDrawBufferView->FillRect(sourceRect);
		DrawTitle(fDrawBufferView, sourceRect, fSelectedColumn, false);
		fDrawBufferView->Sync();
		fDrawBuffer->Unlock();

		CopyBits(originalRect, movedRect);
		DrawBitmap(fDrawBuffer, sourceRect, destRect);
#else
		CopyBits(originalRect, movedRect);
		SetHighColor(fMasterView->Color(B_COLOR_HEADER_BACKGROUND));
		FillRect(destRect);
		DrawTitle(this, destRect, fSelectedColumn, false);
#endif

		// Update the body view
		BRect slaveSize = fOutlineView->VisibleRect();
		BRect slaveSource(originalRect);
		slaveSource.bottom = slaveSize.bottom;
		BRect slaveDest(movedRect);
		slaveDest.bottom = slaveSize.bottom;
		fOutlineView->CopyBits(slaveSource, slaveDest);
		fOutlineView->RedrawColumn(fSelectedColumn, fSelectedColumnRect.left,
			fResizingFirstColumn);
	
		fColumnsWidth += dX;

		// Update the cursor
#if _INCLUDES_CLASS_CURSOR
		if (fSelectedColumn->Width() == minWidth)
			SetViewCursor(fMinResizeCursor, false);
		else if (fSelectedColumn->Width() == maxWidth)
			SetViewCursor(fMaxResizeCursor, false);
		else			
			SetViewCursor(fResizeCursor, false);
#endif
	}
}

void TitleView::ComputeDragBoundries(BColumn *findColumn, BPoint )
{
	float previousColumnLeftEdge = -1000000.0;
	float nextColumnRightEdge = 1000000.0;
	
	bool foundColumn = false;
	float leftEdge = MAX(kLeftMargin, fMasterView->LatchWidth());
	int32 numColumns = fColumns->CountItems();
	for (int index = 0; index < numColumns; index++) {
		BColumn *column = (BColumn*) fColumns->ItemAt(index);
		if (!column->IsVisible())
			continue;
	
		if (column == findColumn) { 
			foundColumn = true;
			continue;
		}

		if (foundColumn) {
			nextColumnRightEdge = leftEdge + column->Width();
			break;
		} else
			previousColumnLeftEdge = leftEdge;

		leftEdge += column->Width() + 1;
	}
	
	float rightEdge = leftEdge + findColumn->Width();		

	fLeftDragBoundry = MIN(previousColumnLeftEdge + findColumn->Width(), leftEdge);
	fRightDragBoundry = MAX(nextColumnRightEdge, rightEdge);
}

void TitleView::DrawTitle(BView *view, BRect rect, BColumn *column, bool depressed)
{
	BRect drawRect;
	rgb_color borderColor = mix_color(fMasterView->Color(B_COLOR_HEADER_BACKGROUND), make_color(0, 0, 0), 128);
	rgb_color backgroundColor;

	rgb_color bevelHigh;
	rgb_color bevelLow;
	// Want exterior borders to overlap.
	rect.right += 1;
	drawRect = rect;
	drawRect.InsetBy(2, 2);
	if (depressed) {
		backgroundColor = mix_color(fMasterView->Color(B_COLOR_HEADER_BACKGROUND), make_color(0, 0, 0), 64);
		bevelHigh = mix_color(backgroundColor, make_color(0, 0, 0), 64);
		bevelLow = mix_color(backgroundColor, make_color(255, 255, 255), 128);
		drawRect.left++;
		drawRect.top++;
	} else {
		backgroundColor = fMasterView->Color(B_COLOR_HEADER_BACKGROUND);
		bevelHigh = mix_color(backgroundColor, make_color(255, 255, 255), 192);
		bevelLow = mix_color(backgroundColor, make_color(0, 0, 0), 64);
		drawRect.bottom--;
		drawRect.right--;
	}

	view->SetHighColor(borderColor);
	view->StrokeRect(rect);	
	view->BeginLineArray(4);
	view->AddLine(BPoint(rect.left+1, rect.top+1), BPoint(rect.right-1, rect.top+1), bevelHigh);
	view->AddLine(BPoint(rect.left+1, rect.top+1), BPoint(rect.left+1, rect.bottom-1), bevelHigh);
	view->AddLine(BPoint(rect.right-1, rect.top+1), BPoint(rect.right-1, rect.bottom-1), bevelLow);
	view->AddLine(BPoint(rect.left+2, rect.bottom-1), BPoint(rect.right-1, rect.bottom-1), bevelLow);	
	view->EndLineArray();

	font_height fh;
	GetFontHeight(&fh);
	
	float baseline = floor(drawRect.top + fh.ascent
							+ (drawRect.Height()+1-(fh.ascent+fh.descent))/2);
				   
	view->SetHighColor(backgroundColor);
	view->SetLowColor(backgroundColor);

	view->FillRect(rect.InsetByCopy(2, 2));


	// If no column given, nothing else to draw.
	if (!column)
		return;
	
	view->SetHighColor(fMasterView->Color(B_COLOR_HEADER_TEXT));

	BFont font;
	GetFont(&font);
	view->SetFont(&font);

	int sortIndex = fSortColumns->IndexOf(column);
	if (sortIndex >= 0) {
		// Draw sort notation.
		BPoint upperLeft(drawRect.right-kSortIndicatorWidth, baseline);
	
		if (fSortColumns->CountItems() > 1) {
			char str[256];
			sprintf(str, "%d", sortIndex + 1);
			const float w = view->StringWidth(str);
			upperLeft.x -= w;
			
			view->SetDrawingMode(B_OP_COPY);
			view->MovePenTo(BPoint(upperLeft.x + kSortIndicatorWidth, baseline));
			view->DrawString(str);
		}

		float bmh = fDownSortArrow->Bounds().Height()+1;
		
		view->SetDrawingMode(B_OP_OVER);
		
		if (column->fSortAscending) {
			BPoint leftTop(upperLeft.x, drawRect.top + (drawRect.IntegerHeight()-fDownSortArrow->Bounds().IntegerHeight())/2);
			view->DrawBitmapAsync(fDownSortArrow, leftTop); 
		} else {
			BPoint leftTop(upperLeft.x, drawRect.top + (drawRect.IntegerHeight()-fUpSortArrow->Bounds().IntegerHeight())/2);
			view->DrawBitmapAsync(fUpSortArrow, leftTop);
		}

		upperLeft.y = baseline-bmh+floor((fh.ascent+fh.descent-bmh)/2);
		if (upperLeft.y < drawRect.top) upperLeft.y = drawRect.top;

		// Adjust title stuff for sort indicator
		drawRect.right = upperLeft.x - 2;
	}

	if (drawRect.right > drawRect.left) {
#if CONSTRAIN_CLIPPING_REGION
		BRegion clipRegion;
		clipRegion.Set(drawRect);
		view->ConstrainClippingRegion(&clipRegion);
		view->PushState();
#endif
		view->MovePenTo(BPoint(drawRect.left+8, baseline));
		view->SetDrawingMode(B_OP_COPY);
		view->SetHighColor(fMasterView->Color(B_COLOR_HEADER_TEXT));
		column->DrawTitle(drawRect, view);

#if CONSTRAIN_CLIPPING_REGION
		view->PopState();
		view->ConstrainClippingRegion(NULL);
#endif
	}
}

void TitleView::Draw(BRect invalidRect)
{
	float columnLeftEdge = MAX(kLeftMargin, fMasterView->LatchWidth());
	for (int32 columnIndex = 0; columnIndex < fColumns->CountItems(); columnIndex++) {
		BColumn *column = (BColumn*) fColumns->ItemAt(columnIndex);
		if (!column->IsVisible())
			continue;

		if (columnLeftEdge > invalidRect.right)
			break;
			
		if (columnLeftEdge + column->Width() >= invalidRect.left) {
			BRect titleRect(columnLeftEdge, 0,
							columnLeftEdge + column->Width(), fVisibleRect.Height());
			DrawTitle(this, titleRect, column,
				(fCurrentState == DRAG_COLUMN_INSIDE_TITLE && fSelectedColumn == column));
		}

		columnLeftEdge += column->Width() + 1;
	}


	// Bevels for right title margin
	if (columnLeftEdge <= invalidRect.right) {
		BRect titleRect(columnLeftEdge, 0, Bounds().right+2, fVisibleRect.Height());
		DrawTitle(this, titleRect, NULL, false);
	}

	// Bevels for left title margin
	if (invalidRect.left < MAX(kLeftMargin, fMasterView->LatchWidth())) {
		BRect titleRect(0, 0, MAX(kLeftMargin, fMasterView->LatchWidth()) - 1, fVisibleRect.Height());
		DrawTitle(this, titleRect, NULL, false);
	}

#if DRAG_TITLE_OUTLINE	
	// (Internal) Column Drag Indicator 
	if (fCurrentState == DRAG_COLUMN_INSIDE_TITLE) {
		BRect dragRect(fSelectedColumnRect);
		dragRect.OffsetTo(fCurrentDragPosition.x - fClickPoint.x, 0);
		if (dragRect.Intersects(invalidRect)) {
			SetHighColor(0, 0, 255);
			StrokeRect(dragRect);	
		}
	}
#endif
}

void TitleView::ScrollTo(BPoint position)
{
	fOutlineView->ScrollBy(position.x - fVisibleRect.left, 0);
	fVisibleRect.OffsetTo(position.x, position.y);

	// Perform the little trick if the user is scrolled over too far.
	// See OutlineView::ScrollTo for a more in depth explanation
	float maxScrollBarValue = fColumnsWidth + MAX(kLeftMargin, fMasterView->LatchWidth()) +
		kRightMargin * 2 - fVisibleRect.Width();
	BScrollBar *hScrollBar = ScrollBar(B_HORIZONTAL);
	float min, max;
	hScrollBar->GetRange(&min, &max);
	if (max != maxScrollBarValue && position.x > maxScrollBarValue)
		FixScrollBar(true);

	_inherited::ScrollTo(position);
}

void TitleView::MessageReceived(BMessage *message)
{
	if (message->what == kToggleColumn) {
		int32 num;
		if (message->FindInt32("be:field_num", &num) == B_OK) {
			for (int index = 0; index < fColumns->CountItems(); index++) {
				BColumn *column = (BColumn*) fColumns->ItemAt(index);
				if (!column) continue;
				if (column->LogicalFieldNum() == num) {
					column->SetVisible(!column->IsVisible());
				}
			}
		}
		return;
	} else {
		BView::MessageReceived(message);
	}
}

void TitleView::MouseDown(BPoint position)
{
	if(fEditMode)
		return;
		
	int32 buttons=1;
	Window()->CurrentMessage()->FindInt32("buttons", &buttons);
	if (buttons == B_SECONDARY_MOUSE_BUTTON && (fColumnFlags & B_ALLOW_COLUMN_POPUP)) {
		// Right mouse button -- bring up menu to show/hide columns.
		if (!fColumnPop) fColumnPop = new BPopUpMenu("Columns", false, false);
		fColumnPop->RemoveItems(0, fColumnPop->CountItems(), true);
		BMessenger me(this);
		for (int index = 0; index < fColumns->CountItems(); index++) {
			BColumn *column = (BColumn*) fColumns->ItemAt(index);
			if (!column) continue;
			BString name;
			column->GetColumnName(&name);
			BMessage* msg = new BMessage(kToggleColumn);
			msg->AddInt32("be:field_num", column->LogicalFieldNum());
			BMenuItem* it = new BMenuItem(name.String(), msg);
			it->SetMarked(column->IsVisible());
			it->SetTarget(me);
			fColumnPop->AddItem(it);
		}
		BPoint screenPosition = ConvertToScreen(position);
		BRect sticky(screenPosition, screenPosition);
		sticky.InsetBy(-5, -5);
		fColumnPop->Go(ConvertToScreen(position), true, false, sticky, true);
		return;
	}
	
	fResizingFirstColumn = true;
	float leftEdge = MAX(kLeftMargin, fMasterView->LatchWidth());
	for (int index = 0; index < fColumns->CountItems(); index++) {
		BColumn *column = (BColumn*) fColumns->ItemAt(index);
		if (!column->IsVisible())
			continue;

		if (leftEdge > position.x + kColumnResizeAreaWidth / 2)
			break;

		//	Check for resizing a column
		float rightEdge = leftEdge + column->Width();			

		if(column->ShowHeading()) {
			if (position.x > rightEdge - kColumnResizeAreaWidth / 2
				&& position.x < rightEdge + kColumnResizeAreaWidth / 2
				&& column->MaxWidth() > column->MinWidth()
				&& (fColumnFlags & B_ALLOW_COLUMN_RESIZE)) {
				fCurrentState = RESIZING_COLUMN;
				fSelectedColumn = column;
				fSelectedColumnRect.Set(leftEdge, 0, rightEdge, fVisibleRect.Height());
				SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS |
					B_NO_POINTER_HISTORY);
				break;
			}

			fResizingFirstColumn = false;

			//	Check for clicking on a column.
			if (position.x > leftEdge && position.x < rightEdge) {		
				fCurrentState = PRESSING_COLUMN;
				fSelectedColumn = column;
				fSelectedColumnRect.Set(leftEdge, 0, rightEdge, fVisibleRect.Height());
				DrawTitle(this, fSelectedColumnRect, fSelectedColumn, true);
				fClickPoint = BPoint(position.x - fSelectedColumnRect.left, position.y
					- fSelectedColumnRect.top);
				SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS |
					B_NO_POINTER_HISTORY);
				break;
			}
		}
		leftEdge = rightEdge + 1;
	}
}

void TitleView::MouseMoved(BPoint position, uint32 transit, const BMessage *)
{

	
	if(fEditMode)
		return;
	
	
	// Handle column manipulation
	switch (fCurrentState) {
		case RESIZING_COLUMN:
			ResizeSelectedColumn(position);
			break;

		case PRESSING_COLUMN: {
			if (abs((int32)(position.x - (fClickPoint.x + fSelectedColumnRect.left))) > kColumnResizeAreaWidth
				|| abs((int32)(position.y - (fClickPoint.y + fSelectedColumnRect.top))) > kColumnResizeAreaWidth) {
				// User has moved the mouse more than the tolerable amount,
				// initiate a drag.
				if (transit == B_INSIDE_VIEW || transit == B_ENTERED_VIEW) {
					if(fColumnFlags & B_ALLOW_COLUMN_MOVE) {
						fCurrentState = DRAG_COLUMN_INSIDE_TITLE;
						ComputeDragBoundries(fSelectedColumn, position);
#if _INCLUDES_CLASS_CURSOR
						SetViewCursor(fColumnMoveCursor, false);
#endif
#if DRAG_TITLE_OUTLINE
						BRect invalidRect(fSelectedColumnRect);
						invalidRect.OffsetTo(position.x - fClickPoint.x, 0);
						fCurrentDragPosition = position;
						Invalidate(invalidRect);
#endif
					}
				} else {
					if(fColumnFlags & B_ALLOW_COLUMN_REMOVE) {
						// Dragged outside view
						fCurrentState = DRAG_COLUMN_OUTSIDE_TITLE;
						fSelectedColumn->SetVisible(false);
						BRect dragRect(fSelectedColumnRect);
		
						// There is a race condition where the mouse may have moved by the
						// time we get to handle this message.  If the user drags a column very
						// quickly, this results in the annoying bug where the cursor is outside
						// of the rectangle that is being dragged around.
						// Call GetMouse with the checkQueue flag set to false so we
						// can get the most recent position of the mouse.  This minimizes
						// this problem (although it is currently not possible to completely
						// eliminate it).
						uint32 buttons;
						GetMouse(&position, &buttons, false);
						dragRect.OffsetTo(position.x - fClickPoint.x, position.y - dragRect.Height() / 2);
						BeginRectTracking(dragRect, B_TRACK_WHOLE_RECT);
					}
				}
			}

			break;
		}
			
		case DRAG_COLUMN_INSIDE_TITLE: {
			if (transit == B_EXITED_VIEW && (fColumnFlags & B_ALLOW_COLUMN_REMOVE)) {
				// Dragged outside view
				fCurrentState = DRAG_COLUMN_OUTSIDE_TITLE;
				fSelectedColumn->SetVisible(false);
				BRect dragRect(fSelectedColumnRect);

				// See explanation above.
				uint32 buttons;
				GetMouse(&position, &buttons, false);

				dragRect.OffsetTo(position.x - fClickPoint.x, position.y - fClickPoint.y);
				BeginRectTracking(dragRect, B_TRACK_WHOLE_RECT);
			} else if (position.x < fLeftDragBoundry || position.x > fRightDragBoundry)
				DragSelectedColumn(position);

#if DRAG_TITLE_OUTLINE		
			// Set up the invalid rect to include the rect for the previous position
			// of the drag rect, as well as the new one.
			BRect invalidRect(fSelectedColumnRect);
			invalidRect.OffsetTo(fCurrentDragPosition.x - fClickPoint.x, 0);
			if (position.x < fCurrentDragPosition.x)
				invalidRect.left -= fCurrentDragPosition.x - position.x;
			else
				invalidRect.right += position.x - fCurrentDragPosition.x;
				
			fCurrentDragPosition = position;
			Invalidate(invalidRect);
#endif
			break;
		}
		
		case DRAG_COLUMN_OUTSIDE_TITLE:
			if (transit == B_ENTERED_VIEW) {
				// Drag back into view
				EndRectTracking();
				fCurrentState = DRAG_COLUMN_INSIDE_TITLE;
				fSelectedColumn->SetVisible(true);
				DragSelectedColumn(position);
			} 

			break;
	
		case INACTIVE:
			// Check for cursor changes if we are over the resize area for
			// a column.
			BColumn *resizeColumn = 0;
			float leftEdge = MAX(kLeftMargin, fMasterView->LatchWidth());
			for (int index = 0; index < fColumns->CountItems(); index++) {
				BColumn *column = (BColumn*) fColumns->ItemAt(index);
				if (!column->IsVisible())
					continue;
		
				if (leftEdge > position.x + kColumnResizeAreaWidth / 2)
					break;
		
				float rightEdge = leftEdge + column->Width();			
				if (position.x > rightEdge - kColumnResizeAreaWidth / 2
					&& position.x < rightEdge + kColumnResizeAreaWidth / 2
					&& column->MaxWidth() > column->MinWidth()) {
					resizeColumn = column;
					break;
				}
		
				leftEdge = rightEdge + 1;
			}

			// Update the cursor
#if _INCLUDES_CLASS_CURSOR
			if (resizeColumn) {
				if (resizeColumn->Width() == resizeColumn->MinWidth())
					SetViewCursor(fMinResizeCursor, false);
				else if (resizeColumn->Width() == resizeColumn->MaxWidth())
					SetViewCursor(fMaxResizeCursor, false);
				else			
					SetViewCursor(fResizeCursor, false);
			} else
				SetViewCursor(B_CURSOR_SYSTEM_DEFAULT, false);
#endif
			break;
	}
}

void TitleView::MouseUp(BPoint position)
{
	if(fEditMode)
		return;
		
	switch (fCurrentState) {
		case RESIZING_COLUMN:
			ResizeSelectedColumn(position);
			fCurrentState = INACTIVE;
			FixScrollBar(false);
#if _INCLUDES_CLASS_CURSOR
			SetViewCursor(B_CURSOR_SYSTEM_DEFAULT, false);
#endif
			break;

		case PRESSING_COLUMN: {
			if (fMasterView->SortingEnabled()) {
				if (fSortColumns->HasItem(fSelectedColumn)) {
					if ((modifiers() & B_CONTROL_KEY) == 0 && fSortColumns->CountItems() > 1) {
						fSortColumns->MakeEmpty();
						fSortColumns->AddItem(fSelectedColumn);
					}
	
					fSelectedColumn->fSortAscending = !fSelectedColumn->fSortAscending;
				} else {
					if ((modifiers() & B_CONTROL_KEY) == 0)
						fSortColumns->MakeEmpty();
	
					fSortColumns->AddItem(fSelectedColumn);
					fSelectedColumn->fSortAscending = true;
				}
				
				fOutlineView->StartSorting();
			}
			
			fCurrentState = INACTIVE;		
			Invalidate();
			break;
		}

		case DRAG_COLUMN_INSIDE_TITLE:
			fCurrentState = INACTIVE;

#if DRAG_TITLE_OUTLINE
			Invalidate();	// xxx Can make this smaller
#else
			Invalidate(fSelectedColumnRect);
#endif
#if _INCLUDES_CLASS_CURSOR
			SetViewCursor(B_CURSOR_SYSTEM_DEFAULT, false);
#endif
			break;
		
		case DRAG_COLUMN_OUTSIDE_TITLE:
			fCurrentState = INACTIVE;
			EndRectTracking();
#if _INCLUDES_CLASS_CURSOR
			SetViewCursor(B_CURSOR_SYSTEM_DEFAULT, false);
#endif
			break;
			
		default:
			;
	}
}

void TitleView::FrameResized(float width, float height)
{
	fVisibleRect.right = fVisibleRect.left + width;
	fVisibleRect.bottom = fVisibleRect.top + height;
	FixScrollBar(true);
}

// #pragma mark -

OutlineView::OutlineView(BRect rect, BList *visibleColumns, BList *sortColumns,
	BColumnListView *listView)
	:	BView(rect, "outline_view", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS),
		fColumns(visibleColumns),
		fSortColumns(sortColumns),
		fItemsHeight(24.),
		fVisibleRect(rect.OffsetToCopy(0, 0)),
		fFocusRow(0),
		fRollOverRow(0),
		fLastSelectedItem(0),
		fFirstSelectedItem(0),
		fSortThread(B_BAD_THREAD_ID),
		fCurrentState(INACTIVE),
		fMasterView(listView),
		fSelectionMode(B_MULTIPLE_SELECTION_LIST),
		fTrackMouse(false),
		fCurrentField(0),
		fCurrentRow(0),
		fCurrentColumn(0),
		fMouseDown(false),
		fCurrentCode(B_OUTSIDE_VIEW),
		fEditMode(false),
		fDragging(false),
		fClickCount(0),
		fDropHighlightY(-1)
{
	SetViewColor(B_TRANSPARENT_32_BIT);

#if DOUBLE_BUFFERED_COLUMN_RESIZE
	// xxx this needs to be smart about the size of the buffer.  Also, the buffer can
	// be shared with the title's buffer.
	BRect doubleBufferRect(0, 0, 600, 35);
	fDrawBuffer = new BBitmap(doubleBufferRect, B_RGB32, true);
	fDrawBufferView = new BView(doubleBufferRect, "double_buffer_view",
		B_FOLLOW_ALL_SIDES, 0);
	fDrawBuffer->Lock();
	fDrawBuffer->AddChild(fDrawBufferView);
	fDrawBuffer->Unlock();
#endif

	FixScrollBar(true);
	fSelectionListDummyHead.fNextSelected = &fSelectionListDummyHead;
	fSelectionListDummyHead.fPrevSelected = &fSelectionListDummyHead;
}

OutlineView::~OutlineView()
{
	fDrawBuffer->Lock();
	fDrawBufferView->RemoveSelf();
	fDrawBuffer->Unlock();
	delete fDrawBufferView;
	delete fDrawBuffer;

	Clear();
}

void OutlineView::Clear()
{
	DeselectAll();	// Make sure selection list doesn't point to deleted rows!
	RecursiveDeleteRows(&fRows, false);
	Invalidate();
	fItemsHeight = 24.;
	FixScrollBar(true);
}

void OutlineView::SetSelectionMode(list_view_type mode)
{
	DeselectAll();	
	fSelectionMode = mode;
}

list_view_type OutlineView::SelectionMode() const
{
	return fSelectionMode;
}

void OutlineView::Deselect(BRow *row)
{
	if (row) {
		if (row->fNextSelected != 0) {
			row->fNextSelected->fPrevSelected = row->fPrevSelected;
			row->fPrevSelected->fNextSelected = row->fNextSelected;
			row->fNextSelected = 0;
			row->fPrevSelected = 0;
			Invalidate();
		}
	}
}

void OutlineView::AddToSelection(BRow *row)
{
	if (row) {
		if (row->fNextSelected == 0) {
			if (fSelectionMode == B_SINGLE_SELECTION_LIST)
				DeselectAll();
			
			row->fNextSelected = fSelectionListDummyHead.fNextSelected;
			row->fPrevSelected = &fSelectionListDummyHead;
			row->fNextSelected->fPrevSelected = row;
			row->fPrevSelected->fNextSelected = row;
			
			BRect invalidRect;
			if (FindVisibleRect(row, &invalidRect))
				Invalidate(invalidRect);
		}
	}
}

void OutlineView::RecursiveDeleteRows(BRowContainer* List, bool IsOwner)
{
	if (List) {
		while (true) {
			BRow *row = List->RemoveItemAt(0L);
			if (row == 0)
				break;
				
			if (row->fChildList)
				RecursiveDeleteRows(row->fChildList, true);
	
			delete row;
		}
	
		if (IsOwner)
			delete List;
	}
}


void OutlineView::RedrawColumn(BColumn *column, float leftEdge, bool isFirstColumn)
{
	if (column) {
		font_height fh;
		GetFontHeight(&fh);
		float line = 0.0;
		for (RecursiveOutlineIterator iterator(&fRows); iterator.CurrentRow();
			line += iterator.CurrentRow()->Height() + 1, iterator.GoToNext()) {
			BRow *row = iterator.CurrentRow();
			float rowHeight = row->Height();
			if (line > fVisibleRect.bottom)
				break;
	
			if (line + rowHeight >= fVisibleRect.top) {
				BRect sourceRect(0, 0, column->Width(), rowHeight);
				BRect destRect(leftEdge, line, leftEdge + column->Width(), line + rowHeight);
	
	#if DOUBLE_BUFFERED_COLUMN_RESIZE
				fDrawBuffer->Lock();
				if (row->fNextSelected != 0) {
					if(fEditMode) {
						fDrawBufferView->SetHighColor(fMasterView->Color(B_COLOR_EDIT_BACKGROUND));
						fDrawBufferView->SetLowColor(fMasterView->Color(B_COLOR_EDIT_BACKGROUND));
					} else {
						fDrawBufferView->SetHighColor(fMasterView->Color(B_COLOR_SELECTION));
						fDrawBufferView->SetLowColor(fMasterView->Color(B_COLOR_SELECTION));
					}
				} else {
					fDrawBufferView->SetHighColor(fMasterView->Color(B_COLOR_BACKGROUND));
					fDrawBufferView->SetLowColor(fMasterView->Color(B_COLOR_BACKGROUND));
				}
				BFont	font;
				GetFont(&font);
				fDrawBufferView->SetFont(&font);
				fDrawBufferView->FillRect(sourceRect);
	
				if (isFirstColumn) {
					// If this is the first column, double buffer drawing the latch too.
					destRect.left += iterator.CurrentLevel() * kOutlineLevelIndent
						- fMasterView->LatchWidth();
					sourceRect.left += iterator.CurrentLevel() * kOutlineLevelIndent
						- fMasterView->LatchWidth();
	
					LatchType pos = B_NO_LATCH;
					if (row->HasLatch())
						pos = row->fIsExpanded ? B_OPEN_LATCH : B_CLOSED_LATCH;					
	
					BRect latchRect(sourceRect);
					latchRect.right = latchRect.left + fMasterView->LatchWidth();
					fMasterView->DrawLatch(fDrawBufferView, latchRect, pos, row);
				}
	
				BField *field = row->GetField(column->fFieldID);
				if (field) {
					BRect fieldRect(sourceRect);
					if (isFirstColumn)
						fieldRect.left += fMasterView->LatchWidth();
	
		#if CONSTRAIN_CLIPPING_REGION
					BRegion clipRegion;
					clipRegion.Set(fieldRect);
					fDrawBufferView->ConstrainClippingRegion(&clipRegion);
					fDrawBufferView->PushState();
		#endif
					fDrawBufferView->SetHighColor(fMasterView->Color(B_COLOR_TEXT));
					float baseline = floor(fieldRect.top + fh.ascent
											+ (fieldRect.Height()+1-(fh.ascent+fh.descent))/2);
					fDrawBufferView->MovePenTo(fieldRect.left + 8, baseline);
					column->DrawField(field, fieldRect, fDrawBufferView);
		#if CONSTRAIN_CLIPPING_REGION
					fDrawBufferView->PopState();
					fDrawBufferView->ConstrainClippingRegion(NULL);
		#endif
				}
	
				if (fFocusRow == row) {
					if(!fEditMode) {
						fDrawBufferView->SetHighColor(0, 0, 0);
						fDrawBufferView->StrokeRect(BRect(-1, sourceRect.top, 10000.0, sourceRect.bottom - 1));
					}
				}
	
				fDrawBufferView->SetHighColor(fMasterView->Color(B_COLOR_ROW_DIVIDER));
		//		StrokeLine(BPoint(0, line + rowHeight - 2), BPoint(Bounds().Width(), line + rowHeight - 2));
		//		StrokeLine(BPoint(0, line + rowHeight - 1), BPoint(Bounds().Width(), line + rowHeight - 1));
				fDrawBufferView->StrokeLine(BPoint(0, rowHeight), BPoint(Bounds().Width(), rowHeight));
	
				fDrawBufferView->Sync();
				fDrawBuffer->Unlock();
				SetDrawingMode(B_OP_OVER);
				DrawBitmap(fDrawBuffer, sourceRect, destRect);
	
	#else
	
				if (row->fNextSelected != 0) {
					if(fEditMode) {
						SetHighColor(fMasterView->Color(B_COLOR_EDIT_BACKGROUND));
						SetLowColor(fMasterView->Color(B_COLOR_EDIT_BACKGROUND));
					}
					else {
						SetHighColor(fMasterView->Color(B_COLOR_SELECTION));
						SetLowColor(fMasterView->Color(B_COLOR_SELECTION));
					}
				} else {
					SetHighColor(fMasterView->Color(B_COLOR_BACKGROUND));
					SetLowColor(fMasterView->Color(B_COLOR_BACKGROUND));
				}
	
				FillRect(destRect);
	
				BField *field = row->GetField(column->fFieldID);
				if (field) {
		#if CONSTRAIN_CLIPPING_REGION
					BRegion clipRegion;
					clipRegion.Set(destRect);
					ConstrainClippingRegion(&clipRegion);
					PushState();
		#endif
					SetHighColor(fColorList[B_TEXT_COLOR]);
					float baseline = floor(destRect.top + fh.ascent
											+ (destRect.Height()+1-(fh.ascent+fh.descent))/2);
					MovePenTo(destRect.left + 8, baseline);
					column->DrawField(field, destRect, this);
		#if CONSTRAIN_CLIPPING_REGION
					PopState();
					ConstrainClippingRegion(NULL);
		#endif
				}
	
				if (fFocusRow == row) {
					if(!fEditMode) {
						SetHighColor(0, 0, 0);
						StrokeRect(BRect(0, destRect.top, 10000.0, destRect.bottom - 1));
					}
				}
	
				rgb_color color = HighColor();
				SetHighColor(fMasterView->Color(B_COLOR_ROW_DIVIDER));
		//		StrokeLine(BPoint(0, line + rowHeight - 2), BPoint(Bounds().Width(), line + rowHeight - 2));
		//		StrokeLine(BPoint(0, line + rowHeight - 1), BPoint(Bounds().Width(), line + rowHeight - 1));
				StrokeLine(BPoint(0, line + rowHeight), BPoint(Bounds().Width(), line + rowHeight));
				SetHighColor(color);
	#endif
			}
		}
	}
}

void OutlineView::Draw(BRect invalidBounds)
{
#if SMART_REDRAW
	BRegion invalidRegion;
	GetClippingRegion(&invalidRegion);
#endif

	font_height fh;
	GetFontHeight(&fh);
	
	float line = 0.0;
	int32 numColumns = fColumns->CountItems();
	for (RecursiveOutlineIterator iterator(&fRows); iterator.CurrentRow();
		iterator.GoToNext()) {
		BRow *row = iterator.CurrentRow();
		if (line > invalidBounds.bottom)
			break;
		
		float rowHeight = row->Height();
		
		if (line > invalidBounds.top - rowHeight) {
			bool isFirstColumn = true;
			float fieldLeftEdge = MAX(kLeftMargin, fMasterView->LatchWidth());
			for (int columnIndex = 0; columnIndex < numColumns; columnIndex++) {
				BColumn *column = (BColumn*) fColumns->ItemAt(columnIndex);
				if (!column->IsVisible())
					continue;
				
				if (!isFirstColumn && fieldLeftEdge > invalidBounds.right)
					break;
					
				if (fieldLeftEdge + column->Width() >= invalidBounds.left) {
					BRect fullRect(fieldLeftEdge, line,
						fieldLeftEdge + column->Width(), line + rowHeight);

					bool clippedFirstColumn = false;
						// This happens when a column is indented past the
						// beginning of the next column.

					if (row->fNextSelected != 0) {
						if (Window()->IsActive()) {
							if(fEditMode)
								SetHighColor(fMasterView->Color(B_COLOR_EDIT_BACKGROUND));
							else
								SetHighColor(fMasterView->Color(B_COLOR_SELECTION));
						}
						else
							SetHighColor(fMasterView->Color(B_COLOR_NON_FOCUS_SELECTION));
					} else
						SetHighColor(fMasterView->Color(B_COLOR_BACKGROUND));

					BRect destRect(fullRect);
					if (isFirstColumn) {
						fullRect.left -= fMasterView->LatchWidth();
						destRect.left += iterator.CurrentLevel() * kOutlineLevelIndent;
						if (destRect.left >= destRect.right) {
							// clipped 
							FillRect(BRect(0, line, fieldLeftEdge + column->Width(),
								line + rowHeight));
							clippedFirstColumn = true;
						}

			
						FillRect(BRect(0, line, MAX(kLeftMargin, fMasterView->LatchWidth()), line + row->Height()));
					}


#if SMART_REDRAW
					if (!clippedFirstColumn && invalidRegion.Intersects(fullRect)) 
#else
					if (!clippedFirstColumn) 
#endif
					{
						FillRect(fullRect);	// Using color set above

						// Draw the latch widget if it has one.
						if (isFirstColumn) {
							if (row == fTargetRow && fCurrentState == LATCH_CLICKED) {
								// Note that this only occurs if the user is holding
								// down a latch while items are added in the background.
								BPoint pos;
								uint32 buttons;
								GetMouse(&pos, &buttons);
								if (fLatchRect.Contains(pos))
									fMasterView->DrawLatch(this, fLatchRect, B_PRESSED_LATCH,
										fTargetRow);
								else
									fMasterView->DrawLatch(this, fLatchRect,
										row->fIsExpanded ? B_OPEN_LATCH : B_CLOSED_LATCH,
										fTargetRow);
							} else {
								LatchType pos = B_NO_LATCH;
								if (row->HasLatch())
									pos = row->fIsExpanded ? B_OPEN_LATCH : B_CLOSED_LATCH;

								fMasterView->DrawLatch(this, BRect(destRect.left -
									fMasterView->LatchWidth(),
									destRect.top, destRect.left, destRect.bottom), pos,
									row);
							}
						}
	
						if (row->fNextSelected != 0) {
							if (Window()->IsActive()) {
								if(fEditMode)  
									SetLowColor(fMasterView->Color(B_COLOR_EDIT_BACKGROUND));
								else
									SetLowColor(fMasterView->Color(B_COLOR_SELECTION));
							}
							else
								SetLowColor(fMasterView->Color(B_COLOR_NON_FOCUS_SELECTION));
						} else
							SetLowColor(fMasterView->Color(B_COLOR_BACKGROUND));

						SetHighColor(fMasterView->HighColor());
							// The master view just holds the high color for us.

						BField *field = row->GetField(column->fFieldID);
						if (field) {
#if CONSTRAIN_CLIPPING_REGION
							BRegion clipRegion;
							clipRegion.Set(destRect);
							ConstrainClippingRegion(&clipRegion);
							PushState();
#endif
							SetHighColor(fMasterView->Color(B_COLOR_TEXT));
							float baseline = floor(destRect.top + fh.ascent
													+ (destRect.Height()+1-(fh.ascent+fh.descent))/2);
							MovePenTo(destRect.left + 8, baseline);
							column->DrawField(field, destRect, this);
#if CONSTRAIN_CLIPPING_REGION
							PopState();
							ConstrainClippingRegion(NULL);
#endif
						}
					}
				}		
	
				isFirstColumn = false;
				fieldLeftEdge += column->Width() + 1;
			}
	
			if (fieldLeftEdge <= invalidBounds.right) {
				if (row->fNextSelected != 0) {
					if (Window()->IsActive()) {
						if(fEditMode)
							SetHighColor(fMasterView->Color(B_COLOR_EDIT_BACKGROUND));
						else
							SetHighColor(fMasterView->Color(B_COLOR_SELECTION));
					}
					else
						SetHighColor(fMasterView->Color(B_COLOR_NON_FOCUS_SELECTION));
				} else
					SetHighColor(fMasterView->Color(B_COLOR_BACKGROUND));

				FillRect(BRect(fieldLeftEdge, line, invalidBounds.right,
					line + rowHeight));
			}
		}
	
		if (fFocusRow == row && fMasterView->IsFocus() && Window()->IsActive()) {
			if(!fEditMode) {
				SetHighColor(0, 0, 0);
				StrokeRect(BRect(0, line, 10000.0, line + rowHeight - 1));
			}
		}
		rgb_color color = HighColor();
		SetHighColor(fMasterView->Color(B_COLOR_ROW_DIVIDER));
//		StrokeLine(BPoint(0, line + rowHeight - 2), BPoint(Bounds().Width(), line + rowHeight - 2));
//		StrokeLine(BPoint(0, line + rowHeight - 1), BPoint(Bounds().Width(), line + rowHeight - 1));
		StrokeLine(BPoint(0, line + rowHeight), BPoint(Bounds().Width(), line + rowHeight));
		SetHighColor(color);
		line += rowHeight + 1;
	}

	if (line <= invalidBounds.bottom) {
		SetHighColor(fMasterView->Color(B_COLOR_BACKGROUND));
		FillRect(BRect(invalidBounds.left, line, invalidBounds.right, invalidBounds.bottom));
	}

	// Draw the drop target line	
	if (fDropHighlightY != -1)
		InvertRect(BRect(0, fDropHighlightY - kDropHighlightLineHeight / 2, 1000000,
			fDropHighlightY + kDropHighlightLineHeight / 2));
}

BRow* OutlineView::FindRow(float ypos, int32 *out_rowIndent, float *out_top)
{
	if (out_rowIndent && out_top) {
		float line = 0.0;
		for (RecursiveOutlineIterator iterator(&fRows); iterator.CurrentRow();
			iterator.GoToNext()) {
			BRow *row = iterator.CurrentRow();
			if (line > ypos)
				break;
	
			float rowHeight = row->Height();
			if (ypos <= line + rowHeight) {
				*out_top = line;
				*out_rowIndent = iterator.CurrentLevel();
				return row;
			}
	
			line += rowHeight + 1;
		}
	}
	return 0;
}

void OutlineView::SetMouseTrackingEnabled(bool enabled)
{
	fTrackMouse = enabled;
	if (!enabled && fDropHighlightY != -1) {
		InvertRect(BRect(0, fDropHighlightY - kDropHighlightLineHeight / 2, 1000000,
			fDropHighlightY + kDropHighlightLineHeight / 2));	// Erase the old target line
		fDropHighlightY = -1;
	}
}


//
// Note that this interaction is not totally safe.  If items are added to
// the list in the background, the widget rect will be incorrect, possibly
// resulting in drawing glitches.  The code that adds items needs to be a little smarter
// about invalidating state.
// 
void OutlineView::MouseDown(BPoint position)
{
	if(!fEditMode)
		fMasterView->MakeFocus(true);

	// Check to see if the user is clicking on a widget to open a section
	// of the list.
	bool reset_click_count = false;
	int32 indent;
	float rowTop; 
	BRow *row = FindRow(position.y, &indent, &rowTop);
	if (row) {

		// Update fCurrentField
		bool handle_field = false;
		BField *new_field = 0;
		BRow *new_row = 0;
		BColumn *new_column = 0;
		BRect new_rect;

		if(position.y >=0 ) {
			if(position.x >=0 ) {
				float x=0;
				for(int32 c=0;c<fMasterView->CountColumns();c++) {
					new_column = fMasterView->ColumnAt(c);
					if (!new_column->IsVisible())
						continue;
					if((MAX(kLeftMargin, fMasterView->LatchWidth())+x)+new_column->Width() >= position.x) {
						if(new_column->WantsEvents()) {
							new_field = row->GetField(c);
							new_row = row;
							FindRect(new_row,&new_rect);
							new_rect.left = MAX(kLeftMargin, fMasterView->LatchWidth()) + x;
							new_rect.right = new_rect.left + new_column->Width() - 1;	
							handle_field = true;
						}
						break;
					}
					x += new_column->Width();
				}				
			}
		}

		// Handle mouse down
		if(handle_field) {
			fMouseDown = true;
			fFieldRect = new_rect;
			fCurrentColumn = new_column;
			fCurrentRow = new_row;
			fCurrentField = new_field;
			fCurrentCode = B_INSIDE_VIEW;
			fCurrentColumn->MouseDown(fMasterView,fCurrentRow,fCurrentField,fFieldRect,position,1);
		}

		if(!fEditMode) {

			fTargetRow = row;
			fTargetRowTop = rowTop;
			FindVisibleRect(fFocusRow, &fFocusRowRect);

			float leftWidgetBoundry = indent * kOutlineLevelIndent + MAX(kLeftMargin, fMasterView->LatchWidth()) -
				fMasterView->LatchWidth();
			fLatchRect.Set(leftWidgetBoundry, rowTop, leftWidgetBoundry +
				fMasterView->LatchWidth(), rowTop + row->Height());
			if (fLatchRect.Contains(position) && row->HasLatch()) {
				fCurrentState = LATCH_CLICKED;
				if (fTargetRow->fNextSelected != 0) {
					if(fEditMode)
						SetHighColor(fMasterView->Color(B_COLOR_EDIT_BACKGROUND));
					else
						SetHighColor(fMasterView->Color(B_COLOR_SELECTION));
				}
				else
					SetHighColor(fMasterView->Color(B_COLOR_BACKGROUND));

				FillRect(fLatchRect);	
				if (fLatchRect.Contains(position))
					fMasterView->DrawLatch(this, fLatchRect, B_PRESSED_LATCH, row);
				else
					fMasterView->DrawLatch(this, fLatchRect, fTargetRow->fIsExpanded
						? B_OPEN_LATCH : B_CLOSED_LATCH, row);

			} else {
				Invalidate(fFocusRowRect);
				fFocusRow = fTargetRow;
				FindVisibleRect(fFocusRow, &fFocusRowRect);

				ASSERT(fTargetRow != 0);

				if ((modifiers() & B_CONTROL_KEY) == 0)
					DeselectAll();

				if ((modifiers() & B_SHIFT_KEY) != 0 && fFirstSelectedItem != 0
					&& fSelectionMode == B_MULTIPLE_SELECTION_LIST) {
					SelectRange(fFirstSelectedItem, fTargetRow);
				}
				else {
					if (fTargetRow->fNextSelected != 0) {
						// Unselect row
						fTargetRow->fNextSelected->fPrevSelected = fTargetRow->fPrevSelected;
						fTargetRow->fPrevSelected->fNextSelected = fTargetRow->fNextSelected;
						fTargetRow->fPrevSelected = 0;
						fTargetRow->fNextSelected = 0;
						fFirstSelectedItem = NULL;
					} else {
						// Select row
						if (fSelectionMode == B_SINGLE_SELECTION_LIST)
							DeselectAll();
					
						fTargetRow->fNextSelected = fSelectionListDummyHead.fNextSelected;
						fTargetRow->fPrevSelected = &fSelectionListDummyHead;
						fTargetRow->fNextSelected->fPrevSelected = fTargetRow;
						fTargetRow->fPrevSelected->fNextSelected = fTargetRow;
						fFirstSelectedItem = fTargetRow;
					}
	
					Invalidate(BRect(fVisibleRect.left, fTargetRowTop, fVisibleRect.right,
						fTargetRowTop + fTargetRow->Height()));
				}

				fCurrentState = ROW_CLICKED;
				if (fLastSelectedItem != fTargetRow)
					reset_click_count = true;
				fLastSelectedItem = fTargetRow;
				fMasterView->SelectionChanged();

			}
		}

		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS |
			B_NO_POINTER_HISTORY);

	} else if (fFocusRow != 0) {
		// User clicked in open space, unhighlight focus row.
		fFocusRow = 0;
		Invalidate(fFocusRowRect);
	}
	
	// We stash the click counts here because the 'clicks' field
	// is not in the CurrentMessage() when MouseUp is called... ;(
	if (reset_click_count)
		fClickCount = 1;
	else
		Window()->CurrentMessage()->FindInt32("clicks", &fClickCount);
	fClickPoint = position;
	
} // end of MouseDown()

void OutlineView::MouseMoved(BPoint position, uint32 /*transit*/, const BMessage */*message*/)
{
	if(!fMouseDown) {
		// Update fCurrentField
		bool handle_field = false;
		BField *new_field = 0;
		BRow *new_row = 0;
		BColumn *new_column = 0;
		BRect new_rect(0,0,0,0);
		if(position.y >=0 ) {
			float top;
			int32 indent;
			BRow *row = FindRow(position.y, &indent, &top);
			if(row && position.x >=0 ) {
				float x=0;
				for(int32 c=0;c<fMasterView->CountColumns();c++) {
					new_column = fMasterView->ColumnAt(c);
					if (!new_column->IsVisible())
						continue;
					if((MAX(kLeftMargin, fMasterView->LatchWidth())+x)+new_column->Width() > position.x) {
						if(new_column->WantsEvents()) {
							new_field = row->GetField(c);
							new_row = row;
							FindRect(new_row,&new_rect);
							new_rect.left = MAX(kLeftMargin, fMasterView->LatchWidth()) + x;
							new_rect.right = new_rect.left + new_column->Width() - 1;	
							handle_field = true;
						}
						break;
					} 
					x += new_column->Width();
				}				
			}
		}

		// Handle mouse moved
		if(handle_field) {
			if(new_field != fCurrentField) {
				if(fCurrentField) {
					fCurrentColumn->MouseMoved(fMasterView, fCurrentRow, 
						fCurrentField, fFieldRect, position, 0, fCurrentCode = B_EXITED_VIEW);
				}
				fCurrentColumn = new_column;
				fCurrentRow = new_row;
				fCurrentField = new_field;
				fFieldRect = new_rect;
				if(fCurrentField) {
					fCurrentColumn->MouseMoved(fMasterView, fCurrentRow, 
						fCurrentField, fFieldRect, position, 0, fCurrentCode = B_ENTERED_VIEW);
				}
			} else {
				if(fCurrentField) {
					fCurrentColumn->MouseMoved(fMasterView, fCurrentRow, 
						fCurrentField, fFieldRect, position, 0, fCurrentCode = B_INSIDE_VIEW);
				}
			}
		} else {
			if(fCurrentField) {
				fCurrentColumn->MouseMoved(fMasterView, fCurrentRow, 
						fCurrentField, fFieldRect, position, 0, fCurrentCode = B_EXITED_VIEW);
				fCurrentField = 0;
				fCurrentColumn = 0;
				fCurrentRow = 0;
			}
		}
	} else {
		if(fCurrentField) {
			if(fFieldRect.Contains(position)) {
				if (   fCurrentCode == B_OUTSIDE_VIEW	
					|| fCurrentCode == B_EXITED_VIEW	
				) {	
					fCurrentColumn->MouseMoved(fMasterView, fCurrentRow, 
						fCurrentField, fFieldRect, position, 1, fCurrentCode = B_ENTERED_VIEW);							
				} else {
					fCurrentColumn->MouseMoved(fMasterView, fCurrentRow, 
						fCurrentField, fFieldRect, position, 1, fCurrentCode = B_INSIDE_VIEW);							
				}
			} else {
				if (   fCurrentCode == B_INSIDE_VIEW
					|| fCurrentCode == B_ENTERED_VIEW
				) {
					fCurrentColumn->MouseMoved(fMasterView, fCurrentRow, 
						fCurrentField, fFieldRect, position, 1, fCurrentCode = B_EXITED_VIEW);							
				} else {
					fCurrentColumn->MouseMoved(fMasterView, fCurrentRow, 
						fCurrentField, fFieldRect, position, 1, fCurrentCode = B_OUTSIDE_VIEW);							
				}
			}
		}
	}

	if(!fEditMode) {

		switch (fCurrentState) {
			case LATCH_CLICKED:
				if (fTargetRow->fNextSelected != 0) {
					if(fEditMode)
						SetHighColor(fMasterView->Color(B_COLOR_EDIT_BACKGROUND));
					else
						SetHighColor(fMasterView->Color(B_COLOR_SELECTION));
				}
				else
					SetHighColor(fMasterView->Color(B_COLOR_BACKGROUND));
				
				FillRect(fLatchRect);	
				if (fLatchRect.Contains(position))
					fMasterView->DrawLatch(this, fLatchRect, B_PRESSED_LATCH, fTargetRow);
				else
					fMasterView->DrawLatch(this, fLatchRect, fTargetRow->fIsExpanded
						? B_OPEN_LATCH : B_CLOSED_LATCH, fTargetRow);
			
				break;
		
			
			case ROW_CLICKED:
				if (abs((int)(position.x - fClickPoint.x)) > kRowDragSensitivity
					|| abs((int)(position.y - fClickPoint.y)) > kRowDragSensitivity) {
					fCurrentState = DRAGGING_ROWS;
					fMasterView->InitiateDrag(fClickPoint, fTargetRow->fNextSelected != 0);
				}
				break;

			case DRAGGING_ROWS:
#if 0
				// falls through...
#else
				if (fTrackMouse /*&& message*/) {
					if (fVisibleRect.Contains(position)) {
						float top;
						int32 indent;
						BRow *target = FindRow(position.y, &indent, &top);
						if(target)
							SetFocusRow(target,true);
					}
				}
				break;
#endif

			default: {
	
				if (fTrackMouse /*&& message*/) {
					// Draw a highlight line...
					if (fVisibleRect.Contains(position)) {
						float top;
						int32 indent;
						BRow *target = FindRow(position.y, &indent, &top);
						if(target==fRollOverRow)
							break;
						if(fRollOverRow)
						{
							BRect rect;
							FindRect(fRollOverRow, &rect);
							Invalidate(rect);
						}
						fRollOverRow=target;
#if 0
						SetFocusRow(fRollOverRow,false);
#else
						PushState();
						SetDrawingMode(B_OP_BLEND);
						SetHighColor(255,255,255,255);
						BRect rect;
						FindRect(fRollOverRow, &rect);
						rect.bottom -= 1.0;
						FillRect(rect);
						PopState();
#endif
					} else {
						if(fRollOverRow)
						{
							BRect rect;
							FindRect(fRollOverRow, &rect);
							Invalidate(rect);
							fRollOverRow = NULL;
						}
					}
				}
			}
		}
	}		
} // end of MouseMoved()

void OutlineView::MouseUp(BPoint position)
{
	if(fCurrentField) {
		fCurrentColumn->MouseUp(fMasterView,fCurrentRow,fCurrentField);
		fMouseDown = false;
	}

	if(!fEditMode) {
		switch (fCurrentState) {
			case LATCH_CLICKED:
				if (fLatchRect.Contains(position))
					fMasterView->ExpandOrCollapse(fTargetRow, !fTargetRow->fIsExpanded);
				
				Invalidate(fLatchRect);
				fCurrentState = INACTIVE;			
				break;
			
			case ROW_CLICKED:
				if (fClickCount > 1
					&& abs((int)fClickPoint.x - (int)position.x) < kDoubleClickMoveSensitivity
					&& abs((int)fClickPoint.y - (int)position.y) < kDoubleClickMoveSensitivity) {
					fMasterView->ItemInvoked();
				}
				fCurrentState = INACTIVE;
				break;		
			
			case DRAGGING_ROWS:
				fCurrentState = INACTIVE;
				// Falls through

			default:
				if (fDropHighlightY != -1) {
					InvertRect(BRect(0, fDropHighlightY - kDropHighlightLineHeight / 2, 1000000,
						fDropHighlightY + kDropHighlightLineHeight / 2));	// Erase the old target line
					fDropHighlightY = -1;
				}
		}
	}
} // end of MouseUp()

void OutlineView::MessageReceived(BMessage *message)
{
	if (message->WasDropped()) {
		fMasterView->MessageDropped(message, ConvertFromScreen(message->DropPoint()));
	} else {
		BView::MessageReceived(message);
	}
}

void OutlineView::ChangeFocusRow(bool up, bool updateSelection, bool addToCurrentSelection)
{
	int32 indent;
	float top;
	float newRowPos = 0;
	float verticalScroll = 0;
	
	if (fFocusRow) {
		// A row currently has the focus, get information about it
		newRowPos = fFocusRowRect.top + (up ? -4 : fFocusRow->Height() + 4);
		if (newRowPos < fVisibleRect.top + 20)
			verticalScroll = newRowPos - 20;
		else if (newRowPos > fVisibleRect.bottom - 20)
			verticalScroll = newRowPos - fVisibleRect.Height() + 20;
	} else
		newRowPos = fVisibleRect.top + 2;
			// no row is currently focused, set this to the top of the window
			// so we will select the first visible item in the list.

	BRow *newRow = FindRow(newRowPos, &indent, &top);
	if (newRow) {
		if (fFocusRow) {
			fFocusRowRect.right = 10000;
			Invalidate(fFocusRowRect);
		}
		fFocusRow = newRow;
		fFocusRowRect.top = top;
		fFocusRowRect.left = 0;
		fFocusRowRect.right = 10000;
		fFocusRowRect.bottom = fFocusRowRect.top + fFocusRow->Height();
		Invalidate(fFocusRowRect);

		if (updateSelection) {
			if (!addToCurrentSelection || fSelectionMode == B_SINGLE_SELECTION_LIST)
				DeselectAll();

			if (fFocusRow->fNextSelected == 0) {
				fFocusRow->fNextSelected = fSelectionListDummyHead.fNextSelected;
				fFocusRow->fPrevSelected = &fSelectionListDummyHead;
				fFocusRow->fNextSelected->fPrevSelected = fFocusRow;
				fFocusRow->fPrevSelected->fNextSelected = fFocusRow;
			}
		
			fLastSelectedItem = fFocusRow;
		}
	} else
		Invalidate(fFocusRowRect);

	if (verticalScroll != 0) {
		BScrollBar *vScrollBar = ScrollBar(B_VERTICAL);
		float min, max;
		vScrollBar->GetRange(&min, &max);
		if (verticalScroll < min)
			verticalScroll = min;
		else if (verticalScroll > max)
			verticalScroll = max;

		vScrollBar->SetValue(verticalScroll);
	}
	
	if (newRow && updateSelection)
		fMasterView->SelectionChanged();
}

void OutlineView::MoveFocusToVisibleRect()
{
	fFocusRow = 0;
	ChangeFocusRow(true, true, false);
}

BRow* OutlineView::CurrentSelection(BRow *lastSelected) const
{
	BRow *row;
	if (lastSelected == 0)
		row = fSelectionListDummyHead.fNextSelected;
	else
		row = lastSelected->fNextSelected;
		
		
	if (row == &fSelectionListDummyHead)
		row = 0;

	return row;
}

void OutlineView::ToggleFocusRowSelection(bool selectRange)
{
	if (fFocusRow == 0)
		return;
		
	if (selectRange && fSelectionMode == B_MULTIPLE_SELECTION_LIST)
		SelectRange(fLastSelectedItem, fFocusRow);
	else {
		if (fFocusRow->fNextSelected != 0) {
			// Unselect row
			fFocusRow->fNextSelected->fPrevSelected = fFocusRow->fPrevSelected;
			fFocusRow->fPrevSelected->fNextSelected = fFocusRow->fNextSelected;
			fFocusRow->fPrevSelected = 0;
			fFocusRow->fNextSelected = 0;
		} else {
			// Select row
			if (fSelectionMode == B_SINGLE_SELECTION_LIST)
				DeselectAll();

			fFocusRow->fNextSelected = fSelectionListDummyHead.fNextSelected;
			fFocusRow->fPrevSelected = &fSelectionListDummyHead;
			fFocusRow->fNextSelected->fPrevSelected = fFocusRow;
			fFocusRow->fPrevSelected->fNextSelected = fFocusRow;
		}
	}

	fLastSelectedItem = fFocusRow;
	fMasterView->SelectionChanged();
	Invalidate(fFocusRowRect);
}

void OutlineView::ToggleFocusRowOpen()
{
	if (fFocusRow)
		fMasterView->ExpandOrCollapse(fFocusRow, !fFocusRow->fIsExpanded);
}


// xxx Could use CopyBits here to speed things up.
void OutlineView::ExpandOrCollapse(BRow* ParentRow, bool Expand)
{
	if (ParentRow) {
	
		if (ParentRow->fIsExpanded == Expand) 
			return;

		ParentRow->fIsExpanded = Expand;
	
		BRect parentRect;
		if (FindRect(ParentRow, &parentRect)) {
			// Determine my new height
			float subTreeHeight = 0.0;
			if (ParentRow->fIsExpanded)
				for (RecursiveOutlineIterator iterator(ParentRow->fChildList);
				     iterator.CurrentRow();
				     iterator.GoToNext()
				    )
				{
					subTreeHeight += iterator.CurrentRow()->Height()+1;
				}
			else
				for (RecursiveOutlineIterator iterator(ParentRow->fChildList);
				     iterator.CurrentRow();
				     iterator.GoToNext()
				    )
				{
					subTreeHeight -= iterator.CurrentRow()->Height()+1;
				}
			fItemsHeight += subTreeHeight;
	
			// Adjust focus row if necessary.
			if (FindRect(fFocusRow, &fFocusRowRect) == false) {
				// focus row is in a subtree that has collapsed, move it up to the parent.
				fFocusRow = ParentRow;
				FindRect(fFocusRow, &fFocusRowRect);
			}
			
			Invalidate(BRect(0, parentRect.top, fVisibleRect.right, fVisibleRect.bottom));
			FixScrollBar(false);
		}
	}
	
}

void OutlineView::RemoveRow(BRow *row)
{
	if (row) {
		BRow *parentRow;
		bool parentIsVisible;
		float subTreeHeight = row->Height();
		if (FindParent(row, &parentRow, &parentIsVisible)) {
			// adjust height
			if (parentIsVisible && (parentRow == 0 || parentRow->fIsExpanded)) {
				if (row->fIsExpanded) {
					for (RecursiveOutlineIterator iterator(row->fChildList);
						iterator.CurrentRow(); iterator.GoToNext())
						subTreeHeight += iterator.CurrentRow()->Height();
				}
			}
		}
		if (parentRow) {
			if (parentRow->fIsExpanded)
				fItemsHeight -= subTreeHeight + 1;
		}
		else {
			fItemsHeight -= subTreeHeight + 1;
		}
		FixScrollBar(false);
		if (parentRow)
			parentRow->fChildList->RemoveItem(row);
		else
			fRows.RemoveItem(row);		
		
		if (parentRow != 0 && parentRow->fChildList->CountItems() == 0) {
			delete parentRow->fChildList;
			parentRow->fChildList = 0;
			if (parentIsVisible)
				Invalidate();	// xxx crude way of redrawing latch	
		}

		if (parentIsVisible && (parentRow == 0 || parentRow->fIsExpanded))
			Invalidate();	// xxx make me smarter.				


		// Adjust focus row if necessary.
		if (fFocusRow && FindRect(fFocusRow, &fFocusRowRect) == false) {
			// focus row is in a subtree that is gone, move it up to the parent.
			fFocusRow = parentRow;
			if (fFocusRow)
				FindRect(fFocusRow, &fFocusRowRect);
		}
		
		// Remove this from the selection if necessary
		if (row->fNextSelected != 0) {
			row->fNextSelected->fPrevSelected = row->fPrevSelected;
			row->fPrevSelected->fNextSelected = row->fNextSelected;
			row->fPrevSelected = 0;
			row->fNextSelected = 0;
			fMasterView->SelectionChanged();
		}

		fCurrentColumn = 0;
		fCurrentRow = 0;
		fCurrentField = 0;
	}
}

BRowContainer* OutlineView::RowList()
{
	return &fRows;
}

void OutlineView::UpdateRow(BRow *row)
{
	if (row) {
		// Determine if this row has changed its sort order
		BRow *parentRow = NULL;
		bool parentIsVisible = false;
		FindParent(row, &parentRow, &parentIsVisible);
		
		BRowContainer* list = (parentRow == NULL) ? &fRows : parentRow->fChildList;
		
		if(list) {
			int32 rowIndex = list->IndexOf(row);
			ASSERT(rowIndex >= 0);
			ASSERT(list->ItemAt(rowIndex) == row);
			
			bool rowMoved = false;
			if (rowIndex > 0 && CompareRows(list->ItemAt(rowIndex - 1), row) > 0)
				rowMoved = true;
					
			if (rowIndex < list->CountItems() - 1 && CompareRows(list->ItemAt(rowIndex + 1),
				row) < 0)
				rowMoved = true;
					
			if (rowMoved) {
				// Sort location of this row has changed.
				// Remove and re-add in the right spot
				SortList(list, parentIsVisible && (parentRow == NULL || parentRow->fIsExpanded));
			} else if (parentIsVisible && (parentRow == NULL || parentRow->fIsExpanded)) {
				BRect invalidRect;
				if (FindVisibleRect(row, &invalidRect))
					Invalidate(invalidRect);
			}
		}
	}
}

void OutlineView::AddRow(BRow* Row, int32 Index, BRow* ParentRow)
{
	if(Row) {
		Row->fParent = ParentRow;
	
		if (fMasterView->SortingEnabled()) {
			// Ignore index here.
			if (ParentRow) {
				if (ParentRow->fChildList == 0)
					ParentRow->fChildList = new BRowContainer;
				
				AddSorted(ParentRow->fChildList, Row);
			} else
				AddSorted(&fRows, Row);
		} else {
			// Note, a -1 index implies add to end if sorting is not enabled
			if (ParentRow) {
				if (ParentRow->fChildList == 0)
					ParentRow->fChildList = new BRowContainer;
				
				if (Index < 0 || Index > ParentRow->fChildList->CountItems())
					ParentRow->fChildList->AddItem(Row);
				else	
					ParentRow->fChildList->AddItem(Row, Index);
			} else {
				if (Index < 0 || Index >= fRows.CountItems())
					fRows.AddItem(Row);
				else	
					fRows.AddItem(Row, Index);
			}
		}
				
		if (ParentRow == 0 || ParentRow->fIsExpanded)
			fItemsHeight += Row->Height() + 1;
	
		FixScrollBar(false);
	
		BRect newRowRect;
		bool newRowIsInOpenBranch = FindRect(Row, &newRowRect);
	
		if (fFocusRow && fFocusRowRect.top > newRowRect.bottom) {
			// The focus row has moved.
			Invalidate(fFocusRowRect);
			FindRect(fFocusRow, &fFocusRowRect);
			Invalidate(fFocusRowRect);
		}
	
		if (newRowIsInOpenBranch) {
			if (fCurrentState == INACTIVE) {
				if (newRowRect.bottom < fVisibleRect.top) {
					// The new row is totally above the current viewport, move
					// everything down and redraw the first line.
					BRect source(fVisibleRect);
					BRect dest(fVisibleRect);
					source.bottom -= Row->Height() + 1;
					dest.top += Row->Height() + 1;
					CopyBits(source, dest);
					Invalidate(BRect(fVisibleRect.left, fVisibleRect.top, fVisibleRect.right,
						fVisibleRect.top + newRowRect.Height()));
				} else if (newRowRect.top < fVisibleRect.bottom) {
					// New item is somewhere in the current region.  Scroll everything
					// beneath it down and invalidate just the new row rect.
					BRect source(fVisibleRect.left, newRowRect.top, fVisibleRect.right,
						fVisibleRect.bottom - newRowRect.Height());
					BRect dest(source);
					dest.OffsetBy(0, newRowRect.Height() + 1);
					CopyBits(source, dest);
					Invalidate(newRowRect);
				} // otherwise, this is below the currently visible region
			} else {
				// Adding the item may have caused the item that the user is currently
				// selected to move.  This would cause annoying drawing and interaction
				// bugs, as the position of that item is cached.  If this happens, resize
				// the scroll bar, then scroll back so the selected item is in view.
				BRect targetRect;
				if (FindRect(fTargetRow, &targetRect)) {
					float delta = targetRect.top - fTargetRowTop;
					if (delta != 0) {
						// This causes a jump because ScrollBy will copy a chunk of the view.
						// Since the actual contents of the view have been offset, we don't
						// want this, we just want to change the virtual origin of the window.
						// Constrain the clipping region so everything is clipped out so no
						// copy occurs.
						//
						//	xxx this currently doesn't work if the scroll bars aren't enabled.
						//  everything will still move anyway.  A minor annoyance.
						BRegion emptyRegion;
						ConstrainClippingRegion(&emptyRegion);
						PushState();
						ScrollBy(0, delta);
						PopState();
						ConstrainClippingRegion(NULL);
			
						fTargetRowTop += delta;
						fClickPoint.y += delta;
						fLatchRect.OffsetBy(0, delta);
					}
				}
			} 
		}
	
		// If the parent was previously childless, it will need to have a latch drawn.
		BRect parentRect;
		if (ParentRow && ParentRow->fChildList->CountItems() == 1
			&& FindVisibleRect(ParentRow, &parentRect))
			Invalidate(parentRect);
	}
}


void OutlineView::FixScrollBar(bool scrollToFit)
{
	BScrollBar *vScrollBar = ScrollBar(B_VERTICAL);
	if (vScrollBar) {
		if (fItemsHeight > fVisibleRect.Height()) {
			float maxScrollBarValue = (fItemsHeight + kBottomMargin) - fVisibleRect.Height();
			vScrollBar->SetProportion(fVisibleRect.Height() / (fItemsHeight + kBottomMargin));

			// If the user is scrolled down too far when makes the range smaller, the list
			// will jump suddenly, which is undesirable.  In this case, don't fix the scroll
			// bar here. In ScrollTo, it checks to see if this has occured, and will
			// fix the scroll bars sneakily if the user has scrolled up far enough.
			if (scrollToFit || vScrollBar->Value() <= maxScrollBarValue) {
				vScrollBar->SetRange(0.0, maxScrollBarValue);
				vScrollBar->SetSteps(20.0, fVisibleRect.Height());
			}
		} else if (vScrollBar->Value() == 0.0) 
			vScrollBar->SetRange(0.0, 0.0);		// disable scroll bar.
	}
}

void OutlineView::AddSorted(BRowContainer *list, BRow *row)
{
	if (list && row) {
		// Find general vicinity with binary search.
		int32 lower = 0;
		int32 upper = list->CountItems()-1;
		while( lower < upper ) {
			int32 middle = lower + (upper-lower+1)/2;
			int32 cmp = CompareRows(row, list->ItemAt(middle));
			if( cmp < 0 ) upper = middle-1;
			else if( cmp > 0 ) lower = middle+1;
			else lower = upper = middle;
		}
		
		// At this point, 'upper' and 'lower' at the last found item.
		// Arbitrarily use 'upper' and determine the final insertion
		// point -- either before or after this item.
		if( upper < 0 ) upper = 0;
		else if( upper < list->CountItems() ) {
			if( CompareRows(row, list->ItemAt(upper)) > 0 ) upper++;
		}
		
		if (upper >= list->CountItems())
			list->AddItem(row);				// Adding to end.
		else
			list->AddItem(row, upper);		// Insert
	}
}

int32 OutlineView::CompareRows(BRow *row1, BRow *row2)
{
	int32 itemCount (fSortColumns->CountItems());
	if (row1 && row2) {
		for (int32 index = 0; index < itemCount; index++) {
			BColumn *column = (BColumn*) fSortColumns->ItemAt(index);
			int comp = 0;
			BField *field1 = (BField*) row1->GetField(column->fFieldID);
			BField *field2 = (BField*) row2->GetField(column->fFieldID);
			if (field1 && field2)
				comp = column->CompareFields(field1, field2);
	
			if (!column->fSortAscending)
				comp = -comp;
			
			if (comp != 0)
				return comp;
		}
	}
	return 0;
}

void OutlineView::FrameResized(float width, float height)
{
	fVisibleRect.right = fVisibleRect.left + width;
	fVisibleRect.bottom = fVisibleRect.top + height;
	FixScrollBar(true);
	_inherited::FrameResized(width, height);
}

void OutlineView::ScrollTo(BPoint position)
{
	fVisibleRect.OffsetTo(position.x, position.y);

	// In FixScrollBar, we might not have been able to change the size of
	// the scroll bar because the user was scrolled down too far.  Take
	// this opportunity to sneak it in if we can.
	BScrollBar *vScrollBar = ScrollBar(B_VERTICAL);
	float maxScrollBarValue = (fItemsHeight + kBottomMargin) - fVisibleRect.Height();
	float min, max;
	vScrollBar->GetRange(&min, &max);
	if (max != maxScrollBarValue && position.y > maxScrollBarValue)
		FixScrollBar(true);
	
	_inherited::ScrollTo(position);
}

const BRect& OutlineView::VisibleRect() const
{
	return fVisibleRect;
}


bool OutlineView::FindVisibleRect(BRow *row, BRect *out_rect)
{
	if (row && out_rect) {
		float line = 0.0;
		for (RecursiveOutlineIterator iterator(&fRows); iterator.CurrentRow();
			iterator.GoToNext()) {
			if (line > fVisibleRect.bottom)
				break;
				
			if (iterator.CurrentRow() == row) {
				out_rect->Set(fVisibleRect.left, line, fVisibleRect.right, line + row->Height());
				return true;
			}
		
			line += iterator.CurrentRow()->Height() + 1;
		}
	}
	return false;
}

bool OutlineView::FindRect(const BRow *row, BRect *out_rect)
{
	float line = 0.0;
	for (RecursiveOutlineIterator iterator(&fRows); iterator.CurrentRow();
		iterator.GoToNext()) {
		if (iterator.CurrentRow() == row) {
			out_rect->Set(fVisibleRect.left, line, fVisibleRect.right, line + row->Height());
			return true;
		}
	
		line += iterator.CurrentRow()->Height() + 1;
	}

	return false;
}


void OutlineView::ScrollTo(const BRow* Row)
{
	BRect rect;
	if( true == FindRect(Row, &rect) )
	{
		ScrollTo(BPoint(rect.left, rect.top));
	}
}


void OutlineView::DeselectAll()
{
	// Invalidate all selected rows
	float line = 0.0;
	for (RecursiveOutlineIterator iterator(&fRows); iterator.CurrentRow();
		iterator.GoToNext()) {
		if (line > fVisibleRect.bottom)
			break;
			
		BRow *row = iterator.CurrentRow();
		if (line + row->Height() > fVisibleRect.top) {
			if (row->fNextSelected != 0)
				Invalidate(BRect(fVisibleRect.left, line, fVisibleRect.right,
					line + row->Height()));
		}
	
		line += row->Height() + 1;
	}

	// Set items not selected
	while (fSelectionListDummyHead.fNextSelected != &fSelectionListDummyHead) {
		BRow *row = fSelectionListDummyHead.fNextSelected;
		row->fNextSelected->fPrevSelected = row->fPrevSelected;
		row->fPrevSelected->fNextSelected = row->fNextSelected;
		row->fNextSelected = 0;
		row->fPrevSelected = 0;
	}
}

BRow* OutlineView::FocusRow() const
{
	return fFocusRow;
}

void OutlineView::SetFocusRow(BRow* Row, bool Select)
{
	if (Row)
	{
		if(Select)
			AddToSelection(Row);
			
		if(fFocusRow == Row)
			return;
			
		Invalidate(fFocusRowRect); // invalidate previous
	
		fTargetRow = fFocusRow = Row;

		FindVisibleRect(fFocusRow, &fFocusRowRect);
		Invalidate(fFocusRowRect); // invalidate current

		fFocusRowRect.right = 10000;
		fMasterView->SelectionChanged();
	}
}

bool OutlineView::SortList(BRowContainer *list, bool isVisible)
{
	if (list) {
		// Shellsort
		BRow **items = (BRow**) list->AsBList()->Items();
		int32 numItems = list->CountItems();
		int h;
		for (h = 1; h < numItems / 9; h = 3 * h + 1)
			;
			
		for (;h > 0; h /= 3) {
			for (int step = h; step < numItems; step++) {
				BRow *temp = items[step];
				int i;
				for (i = step - h; i >= 0; i -= h) {
					if (CompareRows(temp, items[i]) < 0)
						items[i + h] = items[i];
					else
						break;
				}
				
				items[i + h] = temp;
			}
		}
		
		if (isVisible) {
			Invalidate();
	
			InvalidateCachedPositions();
			int lockCount = Window()->CountLocks();
			for (int i = 0; i < lockCount; i++) 
				Window()->Unlock();
	
			while (lockCount--)
				if (!Window()->Lock())
					return false;	// Window is gone...
		}
	}
	return true;
}

int32 OutlineView::DeepSortThreadEntry(void *_outlineView)
{
	((OutlineView*) _outlineView)->DeepSort();
	return 0;
}

void OutlineView::DeepSort()
{
	struct stack_entry {
		bool isVisible;
		BRowContainer *list;
		int32 listIndex;
	} stack[kMaxDepth];
	int32 stackTop = 0;

	stack[stackTop].list = &fRows;
	stack[stackTop].isVisible = true;
	stack[stackTop].listIndex = 0;
	fNumSorted = 0;

	if (Window()->Lock() == false)
		return;

	bool doneSorting = false;
	while (!doneSorting && !fSortCancelled) {

		stack_entry *currentEntry = &stack[stackTop];

		// xxx Can make the invalidate area smaller by finding the rect for the
		// parent item and using that as the top of the invalid rect.

		bool haveLock = SortList(currentEntry->list, currentEntry->isVisible);
		if (!haveLock)
			return ;	// window is gone.

		// Fix focus rect.
		InvalidateCachedPositions();
		if (fCurrentState != INACTIVE)
			fCurrentState = INACTIVE;	// sorry...

		// next list.
		bool foundNextList = false;
		while (!foundNextList && !fSortCancelled) {
			for (int32 index = currentEntry->listIndex; index < currentEntry->list->CountItems();
				index++) {
				BRow *parentRow = currentEntry->list->ItemAt(index);
				BRowContainer *childList = parentRow->fChildList;
				if (childList != 0) {
					currentEntry->listIndex = index + 1;
					stackTop++;
					ASSERT(stackTop < kMaxDepth);
					stack[stackTop].listIndex = 0;
					stack[stackTop].list = childList;
					stack[stackTop].isVisible = (currentEntry->isVisible && parentRow->fIsExpanded);
					foundNextList = true;
					break;
				}
			}
		
			if (!foundNextList) {
				// back up
				if (--stackTop < 0) {
					doneSorting = true;
					break;
				}
				
				currentEntry = &stack[stackTop];
			}
		}
	}

	Window()->Unlock();
}

void OutlineView::StartSorting()
{
	// If this view is not yet attached to a window, don't start a sort thread!
	if (Window() == NULL)
		return;
			
	if (fSortThread != B_BAD_THREAD_ID) {
		thread_info tinfo;
		if (get_thread_info(fSortThread, &tinfo) == B_OK) {
			// Unlock window so this won't deadlock (sort thread is probably
			// waiting to lock window).

			int lockCount = Window()->CountLocks();
			for (int i = 0; i < lockCount; i++) 
				Window()->Unlock();
	
			fSortCancelled = true;
			int32 status;
			wait_for_thread(fSortThread, &status);

			while (lockCount--)
				if (!Window()->Lock())
					return ;	// Window is gone...
		}
	}

	fSortCancelled = false;
	fSortThread = spawn_thread(DeepSortThreadEntry, "sort_thread", B_NORMAL_PRIORITY, this);
	resume_thread(fSortThread);
}

void OutlineView::SelectRange(BRow *start, BRow *end)
{
	if (!start || !end)
		return;

	if (start == end)	// start is always selected when this is called
		return;

	RecursiveOutlineIterator iterator(&fRows, false);
	while (iterator.CurrentRow() != 0) {
		if (iterator.CurrentRow() == end) {
			// reverse selection, swap to fix special case
			BRow *temp = start;
			start = end;
			end = temp;
			break;
		} else if (iterator.CurrentRow() == start)
			break;
	
		iterator.GoToNext();
	}

	while (true) {
		BRow *row = iterator.CurrentRow();
		if(row) {
			if (row->fNextSelected == 0) {
				row->fNextSelected = fSelectionListDummyHead.fNextSelected;
				row->fPrevSelected = &fSelectionListDummyHead;
				row->fNextSelected->fPrevSelected = row;
				row->fPrevSelected->fNextSelected = row;
			}
		} else
			break;

		if (row == end)
			break;
		
		iterator.GoToNext();
	}

	Invalidate();  // xxx make invalidation smaller
}

bool OutlineView::FindParent(BRow *row, BRow **outParent, bool *out_parentIsVisible)
{
	bool result = false;
	if (row && outParent) {
		*outParent = row->fParent;
	
		// Walk up the parent chain to determine if this row is visible
		bool isVisible = true;
		for (BRow *currentRow = row->fParent; currentRow; currentRow = currentRow->fParent) {
			if (!currentRow->fIsExpanded) {
				isVisible = false;
				break;
			}
		}
		
		if (out_parentIsVisible) *out_parentIsVisible = isVisible;
		result = (NULL != *outParent);
	}
	
	return result;
}

int32 OutlineView::IndexOf(BRow *row)
{
	if (row) {
		if (row->fParent == 0)
			return fRows.IndexOf(row);
	
		ASSERT(row->fParent->fChildList);
		return row->fParent->fChildList->IndexOf(row);
	}
	
	return B_ERROR;
}

void OutlineView::InvalidateCachedPositions()
{
	if (fFocusRow)
		FindRect(fFocusRow, &fFocusRowRect);
}

// #pragma mark -


RecursiveOutlineIterator::RecursiveOutlineIterator(BRowContainer *list, bool openBranchesOnly)
	:	fStackIndex(0),
		fCurrentListIndex(0),
		fCurrentListDepth(0),
		fOpenBranchesOnly(openBranchesOnly)
{
	if (list == 0 || list->CountItems() == 0)
		fCurrentList = 0;
	else
		fCurrentList = list;
}

BRow* RecursiveOutlineIterator::CurrentRow() const
{
	if (fCurrentList == 0)
		return 0;
		
	return fCurrentList->ItemAt(fCurrentListIndex);
}

void RecursiveOutlineIterator::GoToNext()
{
	if (fCurrentList == 0)
		return;
	if (fCurrentListIndex < 0 || fCurrentListIndex >= fCurrentList->CountItems()) {
		fCurrentList = 0;
		return;
	}
	
	BRow *currentRow = fCurrentList->ItemAt(fCurrentListIndex);
	if(currentRow) {
		if (currentRow->fChildList && (currentRow->fIsExpanded || !fOpenBranchesOnly)
			&& currentRow->fChildList->CountItems() > 0) {
			// Visit child.
			// Put current list on the stack if it needs to be revisited.
			if (fCurrentListIndex < fCurrentList->CountItems() - 1) {
				fStack[fStackIndex].fRowSet = fCurrentList;		
				fStack[fStackIndex].fIndex = fCurrentListIndex + 1;
				fStack[fStackIndex].fDepth = fCurrentListDepth;
				fStackIndex++;
			}
	
			fCurrentList = currentRow->fChildList;
			fCurrentListIndex = 0;
			fCurrentListDepth++;
		} else if (fCurrentListIndex < fCurrentList->CountItems() - 1)
			fCurrentListIndex++; // next item in current list
		else if (--fStackIndex >= 0) {
			fCurrentList = fStack[fStackIndex].fRowSet;
			fCurrentListIndex = fStack[fStackIndex].fIndex;
			fCurrentListDepth = fStack[fStackIndex].fDepth;
		} else
			fCurrentList = 0;
	}
}

int32 RecursiveOutlineIterator::CurrentLevel() const
{
	return fCurrentListDepth;
}
