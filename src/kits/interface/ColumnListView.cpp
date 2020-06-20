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

#include "ColumnListView.h"

#include <typeinfo>

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>

#include <Application.h>
#include <Bitmap.h>
#include <ControlLook.h>
#include <Cursor.h>
#include <Debug.h>
#include <GraphicsDefs.h>
#include <LayoutUtils.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Region.h>
#include <ScrollBar.h>
#include <String.h>
#include <SupportDefs.h>
#include <Window.h>

#include <ObjectListPrivate.h>

#include "ObjectList.h"


#define DOUBLE_BUFFERED_COLUMN_RESIZE 1
#define SMART_REDRAW 1
#define DRAG_TITLE_OUTLINE 1
#define CONSTRAIN_CLIPPING_REGION 1
#define LOWER_SCROLLBAR 0


namespace BPrivate {

static const unsigned char kDownSortArrow8x8[] = {
	0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
	0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
	0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0xff
};

static const unsigned char kUpSortArrow8x8[] = {
	0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
	0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
	0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff
};

static const unsigned char kDownSortArrow8x8Invert[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x1f, 0x1f, 0x1f, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0x1f, 0xff, 0xff, 0xff, 0xff
};

static const unsigned char kUpSortArrow8x8Invert[] = {
	0xff, 0xff, 0xff, 0x1f, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x1f, 0x1f, 0x1f, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

static const float kTintedLineTint = 1.04;

static const float kMinTitleHeight = 16.0;
static const float kMinRowHeight = 16.0;
static const float kTitleSpacing = 1.4;
static const float kRowSpacing = 1.4;
static const float kLatchWidth = 15.0;

static const int32 kMaxDepth = 1024;
static const float kLeftMargin = kLatchWidth;
static const float kRightMargin = 8;
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
	typedef BView _inherited;
public:
								TitleView(BRect frame, OutlineView* outlineView,
									BList* visibleColumns, BList* sortColumns,
									BColumnListView* masterView,
									uint32 resizingMode);
	virtual						~TitleView();

			void				ColumnAdded(BColumn* column);
			void				ColumnResized(BColumn* column, float oldWidth);
			void				SetColumnVisible(BColumn* column, bool visible);

	virtual	void				Draw(BRect updateRect);
	virtual	void				ScrollTo(BPoint where);
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);
	virtual	void				MouseUp(BPoint where);
	virtual	void				FrameResized(float width, float height);

			void				MoveColumn(BColumn* column, int32 index);
			void				SetColumnFlags(column_flags flags);

			void				SetEditMode(bool state)
									{ fEditMode = state; }

			float				MarginWidth() const;

private:
			void				GetTitleRect(BColumn* column, BRect* _rect);
			int32				FindColumn(BPoint where, float* _leftEdge);
			void				FixScrollBar(bool scrollToFit);
			void				DragSelectedColumn(BPoint where);
			void				ResizeSelectedColumn(BPoint where,
									bool preferred = false);
			void				ComputeDragBoundries(BColumn* column,
									BPoint where);
			void				DrawTitle(BView* view, BRect frame,
									BColumn* column, bool depressed);

			float				_VirtualWidth() const;

			OutlineView*		fOutlineView;
			BList*				fColumns;
			BList*				fSortColumns;
//			float				fColumnsWidth;
			BRect				fVisibleRect;

#if DOUBLE_BUFFERED_COLUMN_RESIZE
			BBitmap*			fDrawBuffer;
			BView*				fDrawBufferView;
#endif

			enum {
				INACTIVE,
				RESIZING_COLUMN,
				PRESSING_COLUMN,
				DRAG_COLUMN_INSIDE_TITLE,
				DRAG_COLUMN_OUTSIDE_TITLE
			}					fCurrentState;

			BPopUpMenu*			fColumnPop;
			BColumnListView*	fMasterView;
			bool				fEditMode;
			int32				fColumnFlags;

	// State information for resizing/dragging
			BColumn*			fSelectedColumn;
			BRect				fSelectedColumnRect;
			bool				fResizingFirstColumn;
			BPoint				fClickPoint; // offset within cell
			float				fLeftDragBoundry;
			float				fRightDragBoundry;
			BPoint				fCurrentDragPosition;


			BBitmap*			fUpSortArrow;
			BBitmap*			fDownSortArrow;

			BCursor*			fResizeCursor;
			BCursor*			fMinResizeCursor;
			BCursor*			fMaxResizeCursor;
			BCursor*			fColumnMoveCursor;
};


class OutlineView : public BView {
	typedef BView _inherited;
public:
								OutlineView(BRect, BList* visibleColumns,
									BList* sortColumns,
									BColumnListView* listView);
	virtual						~OutlineView();

	virtual void				Draw(BRect);
	const 	BRect&				VisibleRect() const;

			void				RedrawColumn(BColumn* column, float leftEdge,
									bool isFirstColumn);
			void 				StartSorting();
			float				GetColumnPreferredWidth(BColumn* column);

			void				AddRow(BRow*, int32 index, BRow* TheRow);
			BRow*				CurrentSelection(BRow* lastSelected) const;
			void 				ToggleFocusRowSelection(bool selectRange);
			void 				ToggleFocusRowOpen();
			void 				ChangeFocusRow(bool up, bool updateSelection,
									bool addToCurrentSelection);
			void 				MoveFocusToVisibleRect();
			void 				ExpandOrCollapse(BRow* parent, bool expand);
			void 				RemoveRow(BRow*);
			BRowContainer*		RowList();
			void				UpdateRow(BRow*);
			bool				FindParent(BRow* row, BRow** _parent,
									bool* _isVisible);
			int32				IndexOf(BRow* row);
			void				Deselect(BRow*);
			void				AddToSelection(BRow*);
			void				DeselectAll();
			BRow*				FocusRow() const;
			void				SetFocusRow(BRow* row, bool select);
			BRow*				FindRow(float ypos, int32* _indent,
									float* _top);
			bool				FindRect(const BRow* row, BRect* _rect);
			void				ScrollTo(const BRow* row);

			void				Clear();
			void				SetSelectionMode(list_view_type type);
			list_view_type		SelectionMode() const;
			void				SetMouseTrackingEnabled(bool);
			void				FixScrollBar(bool scrollToFit);
			void				SetEditMode(bool state)
									{ fEditMode = state; }

	virtual void				FrameResized(float width, float height);
	virtual void				ScrollTo(BPoint where);
	virtual void				MouseDown(BPoint where);
	virtual void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);
	virtual void				MouseUp(BPoint where);
	virtual void				MessageReceived(BMessage* message);

private:
			bool				SortList(BRowContainer* list, bool isVisible);
	static	int32				DeepSortThreadEntry(void* outlineView);
			void				DeepSort();
			void				SelectRange(BRow* start, BRow* end);
			int32				CompareRows(BRow* row1, BRow* row2);
			void				AddSorted(BRowContainer* list, BRow* row);
			void				RecursiveDeleteRows(BRowContainer* list,
									bool owner);
			void				InvalidateCachedPositions();
			bool				FindVisibleRect(BRow* row, BRect* _rect);

			BList*				fColumns;
			BList*				fSortColumns;
			float				fItemsHeight;
			BRowContainer		fRows;
			BRect				fVisibleRect;

#if DOUBLE_BUFFERED_COLUMN_RESIZE
			BBitmap*			fDrawBuffer;
			BView*				fDrawBufferView;
#endif

			BRow*				fFocusRow;
			BRect				fFocusRowRect;
			BRow*				fRollOverRow;

			BRow				fSelectionListDummyHead;
			BRow*				fLastSelectedItem;
			BRow*				fFirstSelectedItem;

			thread_id			fSortThread;
			int32				fNumSorted;
			bool				fSortCancelled;

			enum CurrentState {
				INACTIVE,
				LATCH_CLICKED,
				ROW_CLICKED,
				DRAGGING_ROWS
			};

			CurrentState		fCurrentState;


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
			BPoint				fClickPoint;
			bool				fDragging;
			int32				fClickCount;
			BRow*				fTargetRow;
			float				fTargetRowTop;
			BRect				fLatchRect;
			float				fDropHighlightY;

	friend class RecursiveOutlineIterator;
};


class RecursiveOutlineIterator {
public:
								RecursiveOutlineIterator(
									BRowContainer* container,
									bool openBranchesOnly = true);

			BRow*				CurrentRow() const;
			int32				CurrentLevel() const;
			void				GoToNext();

private:
			struct {
				BRowContainer* fRowSet;
				int32 fIndex;
				int32 fDepth;
			}					fStack[kMaxDepth];

			int32				fStackIndex;
			BRowContainer*		fCurrentList;
			int32				fCurrentListIndex;
			int32				fCurrentListDepth;
			bool				fOpenBranchesOnly;
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


void
BColumn::MouseMoved(BColumnListView* /*parent*/, BRow* /*row*/,
	BField* /*field*/, BRect /*field_rect*/, BPoint/*point*/,
	uint32 /*buttons*/, int32 /*code*/)
{
}


void
BColumn::MouseDown(BColumnListView* /*parent*/, BRow* /*row*/,
	BField* /*field*/, BRect /*field_rect*/, BPoint /*point*/,
	uint32 /*buttons*/)
{
}


void
BColumn::MouseUp(BColumnListView* /*parent*/, BRow* /*row*/, BField* /*field*/)
{
}


// #pragma mark -


BRow::BRow()
	:
	fChildList(NULL),
	fIsExpanded(false),
	fHeight(std::max(kMinRowHeight,
		ceilf(be_plain_font->Size() * kRowSpacing))),
	fNextSelected(NULL),
	fPrevSelected(NULL),
	fParent(NULL),
	fList(NULL)
{
}


BRow::BRow(float height)
	:
	fChildList(NULL),
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
		BField* field = (BField*) fFields.RemoveItem((int32)0);
		if (field == 0)
			break;

		delete field;
	}
}


bool
BRow::HasLatch() const
{
	return fChildList != 0;
}


int32
BRow::CountFields() const
{
	return fFields.CountItems();
}


BField*
BRow::GetField(int32 index)
{
	return (BField*)fFields.ItemAt(index);
}


const BField*
BRow::GetField(int32 index) const
{
	return (const BField*)fFields.ItemAt(index);
}


void
BRow::SetField(BField* field, int32 logicalFieldIndex)
{
	if (fFields.ItemAt(logicalFieldIndex) != 0)
		delete (BField*)fFields.RemoveItem(logicalFieldIndex);

	if (NULL != fList) {
		ValidateField(field, logicalFieldIndex);
		Invalidate();
	}

	fFields.AddItem(field, logicalFieldIndex);
}


float
BRow::Height() const
{
	return fHeight;
}


bool
BRow::IsExpanded() const
{
	return fIsExpanded;
}


bool
BRow::IsSelected() const
{
	return fPrevSelected != NULL;
}


void
BRow::Invalidate()
{
	if (fList != NULL)
		fList->InvalidateRow(this);
}


void
BRow::ValidateFields() const
{
	for (int32 i = 0; i < CountFields(); i++)
		ValidateField(GetField(i), i);
}


void
BRow::ValidateField(const BField* field, int32 logicalFieldIndex) const
{
	// The Fields may be moved by the user, but the logicalFieldIndexes
	// do not change, so we need to map them over when checking the
	// Field types.
	BColumn* column = NULL;
	int32 items = fList->CountColumns();
	for (int32 i = 0 ; i < items; ++i) {
		column = fList->ColumnAt(i);
		if(column->LogicalFieldNum() == logicalFieldIndex )
			break;
	}

	if (column == NULL) {
		BString dbmessage("\n\n\tThe parent BColumnListView does not have "
			"\n\ta BColumn at the logical field index ");
		dbmessage << logicalFieldIndex << ".\n";
		puts(dbmessage.String());
	} else {
		if (!column->AcceptsField(field)) {
			BString dbmessage("\n\n\tThe BColumn of type ");
			dbmessage << typeid(*column).name() << "\n\tat logical field index "
				<< logicalFieldIndex << "\n\tdoes not support the field type "
				<< typeid(*field).name() << ".\n\n";
			debugger(dbmessage.String());
		}
	}
}


// #pragma mark -


BColumn::BColumn(float width, float minWidth, float maxWidth, alignment align)
	:
	fWidth(width),
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


float
BColumn::Width() const
{
	return fWidth;
}


void
BColumn::SetWidth(float width)
{
	fWidth = width;
}


float
BColumn::MinWidth() const
{
	return fMinWidth;
}


float
BColumn::MaxWidth() const
{
	return fMaxWidth;
}


void
BColumn::DrawTitle(BRect, BView*)
{
}


void
BColumn::DrawField(BField*, BRect, BView*)
{
}


int
BColumn::CompareFields(BField*, BField*)
{
	return 0;
}


void
BColumn::GetColumnName(BString* into) const
{
	*into = "(Unnamed)";
}


float
BColumn::GetPreferredWidth(BField* field, BView* parent) const
{
	return fWidth;
}


bool
BColumn::IsVisible() const
{
	return fVisible;
}


void
BColumn::SetVisible(bool visible)
{
	if (fList && (fVisible != visible))
		fList->SetColumnVisible(this, visible);
}


bool
BColumn::ShowHeading() const
{
	return fShowHeading;
}


void
BColumn::SetShowHeading(bool state)
{
	fShowHeading = state;
}


alignment
BColumn::Alignment() const
{
	return fAlignment;
}


void
BColumn::SetAlignment(alignment align)
{
	fAlignment = align;
}


bool
BColumn::WantsEvents() const
{
	return fWantsEvents;
}


void
BColumn::SetWantsEvents(bool state)
{
	fWantsEvents = state;
}


int32
BColumn::LogicalFieldNum() const
{
	return fFieldID;
}


bool
BColumn::AcceptsField(const BField*) const
{
	return true;
}


// #pragma mark -


BColumnListView::BColumnListView(BRect rect, const char* name,
	uint32 resizingMode, uint32 flags, border_style border,
	bool showHorizontalScrollbar)
	:
	BView(rect, name, resizingMode,
		flags | B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE),
	fStatusView(NULL),
	fSelectionMessage(NULL),
	fSortingEnabled(true),
	fLatchWidth(kLatchWidth),
	fBorderStyle(border),
	fShowingHorizontalScrollBar(showHorizontalScrollbar)
{
	_Init();
}


BColumnListView::BColumnListView(const char* name, uint32 flags,
	border_style border, bool showHorizontalScrollbar)
	:
	BView(name, flags | B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE),
	fStatusView(NULL),
	fSelectionMessage(NULL),
	fSortingEnabled(true),
	fLatchWidth(kLatchWidth),
	fBorderStyle(border),
	fShowingHorizontalScrollBar(showHorizontalScrollbar)
{
	_Init();
}


BColumnListView::~BColumnListView()
{
	while (BColumn* column = (BColumn*)fColumns.RemoveItem((int32)0))
		delete column;
}


bool
BColumnListView::InitiateDrag(BPoint, bool)
{
	return false;
}


void
BColumnListView::MessageDropped(BMessage*, BPoint)
{
}


void
BColumnListView::ExpandOrCollapse(BRow* row, bool Open)
{
	fOutlineView->ExpandOrCollapse(row, Open);
}


status_t
BColumnListView::Invoke(BMessage* message)
{
	if (message == 0)
		message = Message();

	return BInvoker::Invoke(message);
}


void
BColumnListView::ItemInvoked()
{
	Invoke();
}


void
BColumnListView::SetInvocationMessage(BMessage* message)
{
	SetMessage(message);
}


BMessage*
BColumnListView::InvocationMessage() const
{
	return Message();
}


uint32
BColumnListView::InvocationCommand() const
{
	return Command();
}


BRow*
BColumnListView::FocusRow() const
{
	return fOutlineView->FocusRow();
}


void
BColumnListView::SetFocusRow(int32 Index, bool Select)
{
	SetFocusRow(RowAt(Index), Select);
}


void
BColumnListView::SetFocusRow(BRow* row, bool Select)
{
	fOutlineView->SetFocusRow(row, Select);
}


void
BColumnListView::SetMouseTrackingEnabled(bool Enabled)
{
	fOutlineView->SetMouseTrackingEnabled(Enabled);
}


list_view_type
BColumnListView::SelectionMode() const
{
	return fOutlineView->SelectionMode();
}


void
BColumnListView::Deselect(BRow* row)
{
	fOutlineView->Deselect(row);
}


void
BColumnListView::AddToSelection(BRow* row)
{
	fOutlineView->AddToSelection(row);
}


void
BColumnListView::DeselectAll()
{
	fOutlineView->DeselectAll();
}


BRow*
BColumnListView::CurrentSelection(BRow* lastSelected) const
{
	return fOutlineView->CurrentSelection(lastSelected);
}


void
BColumnListView::SelectionChanged()
{
	if (fSelectionMessage)
		Invoke(fSelectionMessage);
}


void
BColumnListView::SetSelectionMessage(BMessage* message)
{
	if (fSelectionMessage == message)
		return;

	delete fSelectionMessage;
	fSelectionMessage = message;
}


BMessage*
BColumnListView::SelectionMessage()
{
	return fSelectionMessage;
}


uint32
BColumnListView::SelectionCommand() const
{
	if (fSelectionMessage)
		return fSelectionMessage->what;

	return 0;
}


void
BColumnListView::SetSelectionMode(list_view_type mode)
{
	fOutlineView->SetSelectionMode(mode);
}


void
BColumnListView::SetSortingEnabled(bool enabled)
{
	fSortingEnabled = enabled;
	fSortColumns.MakeEmpty();
	fTitleView->Invalidate();
		// erase sort indicators
}


bool
BColumnListView::SortingEnabled() const
{
	return fSortingEnabled;
}


void
BColumnListView::SetSortColumn(BColumn* column, bool add, bool ascending)
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


void
BColumnListView::ClearSortColumns()
{
	fSortColumns.MakeEmpty();
	fTitleView->Invalidate();
		// erase sort indicators
}


void
BColumnListView::AddStatusView(BView* view)
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


BView*
BColumnListView::RemoveStatusView()
{
	if (fStatusView) {
		float width = fStatusView->Bounds().Width();
		Window()->BeginViewTransaction();
		fStatusView->RemoveSelf();
		fHorizontalScrollBar->MoveBy(-width, 0);
		fHorizontalScrollBar->ResizeBy(width, 0);
		Window()->EndViewTransaction();
	}

	BView* view = fStatusView;
	fStatusView = 0;
	return view;
}


void
BColumnListView::AddColumn(BColumn* column, int32 logicalFieldIndex)
{
	ASSERT(column != NULL);

	column->fList = this;
	column->fFieldID = logicalFieldIndex;

	// sanity check -- if there is already a field with this ID, remove it.
	for (int32 index = 0; index < fColumns.CountItems(); index++) {
		BColumn* existingColumn = (BColumn*) fColumns.ItemAt(index);
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


void
BColumnListView::MoveColumn(BColumn* column, int32 index)
{
	ASSERT(column != NULL);
	fTitleView->MoveColumn(column, index);
}


void
BColumnListView::RemoveColumn(BColumn* column)
{
	if (fColumns.HasItem(column)) {
		SetColumnVisible(column, false);
		if (Window() != NULL)
			Window()->UpdateIfNeeded();
		fColumns.RemoveItem(column);
	}
}


int32
BColumnListView::CountColumns() const
{
	return fColumns.CountItems();
}


BColumn*
BColumnListView::ColumnAt(int32 field) const
{
	return (BColumn*) fColumns.ItemAt(field);
}


BColumn*
BColumnListView::ColumnAt(BPoint point) const
{
	float left = MAX(kLeftMargin, LatchWidth());

	for (int i = 0; BColumn* column = (BColumn*)fColumns.ItemAt(i); i++) {
		if (column == NULL || !column->IsVisible())
			continue;

		float right = left + column->Width();
		if (point.x >= left && point.x <= right)
			return column;

		left = right + 1;
	}

	return NULL;
}


void
BColumnListView::SetColumnVisible(BColumn* column, bool visible)
{
	fTitleView->SetColumnVisible(column, visible);
}


void
BColumnListView::SetColumnVisible(int32 index, bool isVisible)
{
	BColumn* column = ColumnAt(index);
	if (column != NULL)
		column->SetVisible(isVisible);
}


bool
BColumnListView::IsColumnVisible(int32 index) const
{
	BColumn* column = ColumnAt(index);
	if (column != NULL)
		return column->IsVisible();

	return false;
}


void
BColumnListView::SetColumnFlags(column_flags flags)
{
	fTitleView->SetColumnFlags(flags);
}


void
BColumnListView::ResizeColumnToPreferred(int32 index)
{
	BColumn* column = ColumnAt(index);
	if (column == NULL)
		return;

	// get the preferred column width
	float width = fOutlineView->GetColumnPreferredWidth(column);

	// set it
	float oldWidth = column->Width();
	column->SetWidth(width);

	fTitleView->ColumnResized(column, oldWidth);
	fOutlineView->Invalidate();
}


void
BColumnListView::ResizeAllColumnsToPreferred()
{
	int32 count = CountColumns();
	for (int32 i = 0; i < count; i++)
		ResizeColumnToPreferred(i);
}


const BRow*
BColumnListView::RowAt(int32 Index, BRow* parentRow) const
{
	if (parentRow == 0)
		return fOutlineView->RowList()->ItemAt(Index);

	return parentRow->fChildList ? parentRow->fChildList->ItemAt(Index) : NULL;
}


BRow*
BColumnListView::RowAt(int32 Index, BRow* parentRow)
{
	if (parentRow == 0)
		return fOutlineView->RowList()->ItemAt(Index);

	return parentRow->fChildList ? parentRow->fChildList->ItemAt(Index) : 0;
}


const BRow*
BColumnListView::RowAt(BPoint point) const
{
	float top;
	int32 indent;
	return fOutlineView->FindRow(point.y, &indent, &top);
}


BRow*
BColumnListView::RowAt(BPoint point)
{
	float top;
	int32 indent;
	return fOutlineView->FindRow(point.y, &indent, &top);
}


bool
BColumnListView::GetRowRect(const BRow* row, BRect* outRect) const
{
	return fOutlineView->FindRect(row, outRect);
}


bool
BColumnListView::FindParent(BRow* row, BRow** _parent, bool* _isVisible) const
{
	return fOutlineView->FindParent(row, _parent, _isVisible);
}


int32
BColumnListView::IndexOf(BRow* row)
{
	return fOutlineView->IndexOf(row);
}


int32
BColumnListView::CountRows(BRow* parentRow) const
{
	if (parentRow == 0)
		return fOutlineView->RowList()->CountItems();
	if (parentRow->fChildList)
		return parentRow->fChildList->CountItems();
	else
		return 0;
}


void
BColumnListView::AddRow(BRow* row, BRow* parentRow)
{
	AddRow(row, -1, parentRow);
}


void
BColumnListView::AddRow(BRow* row, int32 index, BRow* parentRow)
{
	row->fChildList = 0;
	row->fList = this;
	row->ValidateFields();
	fOutlineView->AddRow(row, index, parentRow);
}


void
BColumnListView::RemoveRow(BRow* row)
{
	fOutlineView->RemoveRow(row);
	row->fList = NULL;
}


void
BColumnListView::UpdateRow(BRow* row)
{
	fOutlineView->UpdateRow(row);
}


bool
BColumnListView::SwapRows(int32 index1, int32 index2, BRow* parentRow1,
	BRow* parentRow2)
{
	BRow* row1 = NULL;
	BRow* row2 = NULL;

	BRowContainer* container1 = NULL;
	BRowContainer* container2 = NULL;

	if (parentRow1 == NULL)
		container1 = fOutlineView->RowList();
	else
		container1 = parentRow1->fChildList;

	if (container1 == NULL)
		return false;

	if (parentRow2 == NULL)
		container2 = fOutlineView->RowList();
	else
		container2 = parentRow2->fChildList;

	if (container2 == NULL)
		return false;

	row1 = container1->ItemAt(index1);

	if (row1 == NULL)
		return false;

	row2 = container2->ItemAt(index2);

	if (row2 == NULL)
		return false;

	container1->ReplaceItem(index2, row1);
	container2->ReplaceItem(index1, row2);

	BRect rect1;
	BRect rect2;
	BRect rect;

	fOutlineView->FindRect(row1, &rect1);
	fOutlineView->FindRect(row2, &rect2);

	rect = rect1 | rect2;

	fOutlineView->Invalidate(rect);

	return true;
}


void
BColumnListView::ScrollTo(const BRow* row)
{
	fOutlineView->ScrollTo(row);
}


void
BColumnListView::ScrollTo(BPoint point)
{
	fOutlineView->ScrollTo(point);
}


void
BColumnListView::Clear()
{
	fOutlineView->Clear();
}


void
BColumnListView::InvalidateRow(BRow* row)
{
	BRect updateRect;
	GetRowRect(row, &updateRect);
	fOutlineView->Invalidate(updateRect);
}


// This method is deprecated.
void
BColumnListView::SetFont(const BFont* font, uint32 mask)
{
	fOutlineView->SetFont(font, mask);
	fTitleView->SetFont(font, mask);
}


void
BColumnListView::SetFont(ColumnListViewFont font_num, const BFont* font,
	uint32 mask)
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
	}
}


void
BColumnListView::GetFont(ColumnListViewFont font_num, BFont* font) const
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
	}
}


void
BColumnListView::SetColor(ColumnListViewColor colorIndex, const rgb_color color)
{
	if ((int)colorIndex < 0) {
		ASSERT(false);
		colorIndex = (ColumnListViewColor)0;
	}

	if ((int)colorIndex >= (int)B_COLOR_TOTAL) {
		ASSERT(false);
		colorIndex = (ColumnListViewColor)(B_COLOR_TOTAL - 1);
	}

	fColorList[colorIndex] = color;
	fCustomColors = true;
}


void
BColumnListView::ResetColors()
{
	fCustomColors = false;
	_UpdateColors();
	Invalidate();
}


rgb_color
BColumnListView::Color(ColumnListViewColor colorIndex) const
{
	if ((int)colorIndex < 0) {
		ASSERT(false);
		colorIndex = (ColumnListViewColor)0;
	}

	if ((int)colorIndex >= (int)B_COLOR_TOTAL) {
		ASSERT(false);
		colorIndex = (ColumnListViewColor)(B_COLOR_TOTAL - 1);
	}

	return fColorList[colorIndex];
}


void
BColumnListView::SetHighColor(rgb_color color)
{
	BView::SetHighColor(color);
//	fOutlineView->Invalidate();
		// Redraw with the new color.
		// Note that this will currently cause an infinite loop, refreshing
		// over and over. A better solution is needed.
}


void
BColumnListView::SetSelectionColor(rgb_color color)
{
	fColorList[B_COLOR_SELECTION] = color;
	fCustomColors = true;
}


void
BColumnListView::SetBackgroundColor(rgb_color color)
{
	fColorList[B_COLOR_BACKGROUND] = color;
	fCustomColors = true;
	fOutlineView->Invalidate();
		// repaint with new color
}


void
BColumnListView::SetEditColor(rgb_color color)
{
	fColorList[B_COLOR_EDIT_BACKGROUND] = color;
	fCustomColors = true;
}


const rgb_color
BColumnListView::SelectionColor() const
{
	return fColorList[B_COLOR_SELECTION];
}


const rgb_color
BColumnListView::BackgroundColor() const
{
	return fColorList[B_COLOR_BACKGROUND];
}


const rgb_color
BColumnListView::EditColor() const
{
	return fColorList[B_COLOR_EDIT_BACKGROUND];
}


BPoint
BColumnListView::SuggestTextPosition(const BRow* row,
	const BColumn* inColumn) const
{
	BRect rect(GetFieldRect(row, inColumn));

	font_height fh;
	fOutlineView->GetFontHeight(&fh);
	float baseline = floor(rect.top + fh.ascent
		+ (rect.Height() + 1 - (fh.ascent + fh.descent)) / 2);
	return BPoint(rect.left + 8, baseline);
}


BRect
BColumnListView::GetFieldRect(const BRow* row, const BColumn* inColumn) const
{
	BRect rect;
	GetRowRect(row, &rect);
	if (inColumn != NULL) {
		float leftEdge = MAX(kLeftMargin, LatchWidth());
		for (int index = 0; index < fColumns.CountItems(); index++) {
			BColumn* column = (BColumn*) fColumns.ItemAt(index);
			if (column == NULL || !column->IsVisible())
				continue;

			if (column == inColumn) {
				rect.left = leftEdge;
				rect.right = rect.left + column->Width();
				break;
			}

			leftEdge += column->Width() + 1;
		}
	}

	return rect;
}


void
BColumnListView::SetLatchWidth(float width)
{
	fLatchWidth = width;
	Invalidate();
}


float
BColumnListView::LatchWidth() const
{
	return fLatchWidth;
}

void
BColumnListView::DrawLatch(BView* view, BRect rect, LatchType position, BRow*)
{
	const int32 rectInset = 4;

	// make square
	int32 sideLen = rect.IntegerWidth();
	if (sideLen > rect.IntegerHeight())
		sideLen = rect.IntegerHeight();

	// make center
	int32 halfWidth  = rect.IntegerWidth() / 2;
	int32 halfHeight = rect.IntegerHeight() / 2;
	int32 halfSide   = sideLen / 2;

	float left = rect.left + halfWidth  - halfSide;
	float top  = rect.top  + halfHeight - halfSide;

	BRect itemRect(left, top, left + sideLen, top + sideLen);

	// Why it is a pixel high? I don't know.
	itemRect.OffsetBy(0, -1);

	itemRect.InsetBy(rectInset, rectInset);

	// make it an odd number of pixels wide, the latch looks better this way
	if ((itemRect.IntegerWidth() % 2) == 1) {
		itemRect.right += 1;
		itemRect.bottom += 1;
	}

	rgb_color highColor = view->HighColor();
	view->SetHighColor(0, 0, 0);

	switch (position) {
		case B_OPEN_LATCH:
			view->StrokeRect(itemRect);
			view->StrokeLine(
				BPoint(itemRect.left + 2,
					(itemRect.top + itemRect.bottom) / 2),
				BPoint(itemRect.right - 2,
					(itemRect.top + itemRect.bottom) / 2));
			break;

		case B_PRESSED_LATCH:
			view->StrokeRect(itemRect);
			view->StrokeLine(
				BPoint(itemRect.left + 2,
					(itemRect.top + itemRect.bottom) / 2),
				BPoint(itemRect.right - 2,
					(itemRect.top + itemRect.bottom) / 2));
			view->StrokeLine(
				BPoint((itemRect.left + itemRect.right) / 2,
					itemRect.top +  2),
				BPoint((itemRect.left + itemRect.right) / 2,
					itemRect.bottom - 2));
			view->InvertRect(itemRect);
			break;

		case B_CLOSED_LATCH:
			view->StrokeRect(itemRect);
			view->StrokeLine(
				BPoint(itemRect.left + 2,
					(itemRect.top + itemRect.bottom) / 2),
				BPoint(itemRect.right - 2,
					(itemRect.top + itemRect.bottom) / 2));
			view->StrokeLine(
				BPoint((itemRect.left + itemRect.right) / 2,
					itemRect.top +  2),
				BPoint((itemRect.left + itemRect.right) / 2,
					itemRect.bottom - 2));
			break;

		case B_NO_LATCH:
		default:
			// No drawing
			break;
	}

	view->SetHighColor(highColor);
}


void
BColumnListView::MakeFocus(bool isFocus)
{
	if (fBorderStyle != B_NO_BORDER) {
		// Redraw focus marks around view
		Invalidate();
		fHorizontalScrollBar->SetBorderHighlighted(isFocus);
		fVerticalScrollBar->SetBorderHighlighted(isFocus);
	}

	BView::MakeFocus(isFocus);
}


void
BColumnListView::MessageReceived(BMessage* message)
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
	} else if (message->what == B_COLORS_UPDATED) {
		// Todo: Is it worthwhile to optimize this?
		_UpdateColors();
	}

	BView::MessageReceived(message);
}


void
BColumnListView::KeyDown(const char* bytes, int32 numBytes)
{
	char key = bytes[0];
	switch (key) {
		case B_RIGHT_ARROW:
		case B_LEFT_ARROW:
		{
			if ((modifiers() & B_SHIFT_KEY) != 0) {
				float  minVal, maxVal;
				fHorizontalScrollBar->GetRange(&minVal, &maxVal);
				float smallStep, largeStep;
				fHorizontalScrollBar->GetSteps(&smallStep, &largeStep);
				float oldVal = fHorizontalScrollBar->Value();
				float newVal = oldVal;

				if (key == B_LEFT_ARROW)
					newVal -= smallStep;
				else if (key == B_RIGHT_ARROW)
					newVal += smallStep;

				if (newVal < minVal)
					newVal = minVal;
				else if (newVal > maxVal)
					newVal = maxVal;

				fHorizontalScrollBar->SetValue(newVal);
			} else {
				BRow* focusRow = fOutlineView->FocusRow();
				if (focusRow == NULL)
					break;

				bool isExpanded = focusRow->HasLatch()
					&& focusRow->IsExpanded();
				switch (key) {
					case B_LEFT_ARROW:
						if (isExpanded)
							fOutlineView->ToggleFocusRowOpen();
						else if (focusRow->fParent != NULL) {
							fOutlineView->DeselectAll();
							fOutlineView->SetFocusRow(focusRow->fParent, true);
							fOutlineView->ScrollTo(focusRow->fParent);
						}
						break;

					case B_RIGHT_ARROW:
						if (!isExpanded)
							fOutlineView->ToggleFocusRowOpen();
						else
							fOutlineView->ChangeFocusRow(false, true, false);
						break;
				}
			}
			break;
		}

		case B_DOWN_ARROW:
			fOutlineView->ChangeFocusRow(false,
				(modifiers() & B_CONTROL_KEY) == 0,
				(modifiers() & B_SHIFT_KEY) != 0);
			break;

		case B_UP_ARROW:
			fOutlineView->ChangeFocusRow(true,
				(modifiers() & B_CONTROL_KEY) == 0,
				(modifiers() & B_SHIFT_KEY) != 0);
			break;

		case B_PAGE_UP:
		case B_PAGE_DOWN:
		{
			float minValue, maxValue;
			fVerticalScrollBar->GetRange(&minValue, &maxValue);
			float smallStep, largeStep;
			fVerticalScrollBar->GetSteps(&smallStep, &largeStep);
			float currentValue = fVerticalScrollBar->Value();
			float newValue = currentValue;

			if (key == B_PAGE_UP)
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
			fOutlineView->ToggleFocusRowSelection(
				(modifiers() & B_SHIFT_KEY) != 0);
			break;

		case '+':
			fOutlineView->ToggleFocusRowOpen();
			break;

		default:
			BView::KeyDown(bytes, numBytes);
	}
}


void
BColumnListView::AttachedToWindow()
{
	if (!Messenger().IsValid())
		SetTarget(Window());

	if (SortingEnabled()) fOutlineView->StartSorting();
}


void
BColumnListView::WindowActivated(bool active)
{
	fOutlineView->Invalidate();
		// focus and selection appearance changes with focus

	Invalidate();
		// redraw focus marks around view
	BView::WindowActivated(active);
}


void
BColumnListView::Draw(BRect updateRect)
{
	BRect rect = Bounds();

	uint32 flags = 0;
	if (IsFocus() && Window()->IsActive())
		flags |= BControlLook::B_FOCUSED;

	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);

	BRect verticalScrollBarFrame;
	if (!fVerticalScrollBar->IsHidden())
		verticalScrollBarFrame = fVerticalScrollBar->Frame();

	BRect horizontalScrollBarFrame;
	if (!fHorizontalScrollBar->IsHidden())
		horizontalScrollBarFrame = fHorizontalScrollBar->Frame();

	if (fBorderStyle == B_NO_BORDER) {
		// We still draw the left/top border, but not focused.
		// The scrollbars cannot be displayed without frame and
		// it looks bad to have no frame only along the left/top
		// side.
		rgb_color borderColor = tint_color(base, B_DARKEN_2_TINT);
		SetHighColor(borderColor);
		StrokeLine(BPoint(rect.left, rect.bottom),
			BPoint(rect.left, rect.top));
		StrokeLine(BPoint(rect.left + 1, rect.top),
			BPoint(rect.right, rect.top));
	}

	be_control_look->DrawScrollViewFrame(this, rect, updateRect,
		verticalScrollBarFrame, horizontalScrollBarFrame,
		base, fBorderStyle, flags);

	if (fStatusView != NULL) {
		rect = Bounds();
		BRegion region(rect & fStatusView->Frame().InsetByCopy(-2, -2));
		ConstrainClippingRegion(&region);
		rect.bottom = fStatusView->Frame().top - 1;
		be_control_look->DrawScrollViewFrame(this, rect, updateRect,
			BRect(), BRect(), base, fBorderStyle, flags);
	}
}


void
BColumnListView::SaveState(BMessage* message)
{
	message->MakeEmpty();

	for (int32 i = 0; BColumn* column = (BColumn*)fColumns.ItemAt(i); i++) {
		message->AddInt32("ID", column->fFieldID);
		message->AddFloat("width", column->fWidth);
		message->AddBool("visible", column->fVisible);
	}

	message->AddBool("sortingenabled", fSortingEnabled);

	if (fSortingEnabled) {
		for (int32 i = 0; BColumn* column = (BColumn*)fSortColumns.ItemAt(i);
				i++) {
			message->AddInt32("sortID", column->fFieldID);
			message->AddBool("sortascending", column->fSortAscending);
		}
	}
}


void
BColumnListView::LoadState(BMessage* message)
{
	int32 id;
	for (int i = 0; message->FindInt32("ID", i, &id) == B_OK; i++) {
		for (int j = 0; BColumn* column = (BColumn*)fColumns.ItemAt(j); j++) {
			if (column->fFieldID == id) {
				// move this column to position 'i' and set its attributes
				MoveColumn(column, i);
				float width;
				if (message->FindFloat("width", i, &width) == B_OK)
					column->SetWidth(width);
				bool visible;
				if (message->FindBool("visible", i, &visible) == B_OK)
					column->SetVisible(visible);
			}
		}
	}
	bool b;
	if (message->FindBool("sortingenabled", &b) == B_OK) {
		SetSortingEnabled(b);
		for (int k = 0; message->FindInt32("sortID", k, &id) == B_OK; k++) {
			for (int j = 0; BColumn* column = (BColumn*)fColumns.ItemAt(j);
					j++) {
				if (column->fFieldID == id) {
					// add this column to the sort list
					bool value;
					if (message->FindBool("sortascending", k, &value) == B_OK)
						SetSortColumn(column, true, value);
				}
			}
		}
	}
}


void
BColumnListView::SetEditMode(bool state)
{
	fOutlineView->SetEditMode(state);
	fTitleView->SetEditMode(state);
}


void
BColumnListView::Refresh()
{
	if (LockLooper()) {
		Invalidate();
		fOutlineView->FixScrollBar (true);
		fOutlineView->Invalidate();
		Window()->UpdateIfNeeded();
		UnlockLooper();
	}
}


BSize
BColumnListView::MinSize()
{
	BSize size;
	size.width = 100;
	size.height = std::max(kMinTitleHeight,
		ceilf(be_plain_font->Size() * kTitleSpacing))
		+ 4 * B_H_SCROLL_BAR_HEIGHT;
	if (!fHorizontalScrollBar->IsHidden())
		size.height += fHorizontalScrollBar->Frame().Height() + 1;
	// TODO: Take border size into account

	return BLayoutUtils::ComposeSize(ExplicitMinSize(), size);
}


BSize
BColumnListView::PreferredSize()
{
	BSize size = MinSize();
	size.height += ceilf(be_plain_font->Size()) * 20;

	// return MinSize().width if there are no columns.
	int32 count = CountColumns();
	if (count > 0) {
		BRect titleRect;
		BRect outlineRect;
		BRect vScrollBarRect;
		BRect hScrollBarRect;
		_GetChildViewRects(Bounds(), titleRect, outlineRect, vScrollBarRect,
			hScrollBarRect);
		// Start with the extra width for border and scrollbars etc.
		size.width = titleRect.left - Bounds().left;
		size.width += Bounds().right - titleRect.right;

		// If we want all columns to be visible at their current width,
		// we also need to add the extra margin width that the TitleView
		// uses to compute its _VirtualWidth() for the horizontal scroll bar.
		size.width += fTitleView->MarginWidth();
		for (int32 i = 0; i < count; i++) {
			BColumn* column = ColumnAt(i);
			if (column != NULL)
				size.width += column->Width();
		}
	}

	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(), size);
}


BSize
BColumnListView::MaxSize()
{
	BSize size(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), size);
}


void
BColumnListView::LayoutInvalidated(bool descendants)
{
}


void
BColumnListView::DoLayout()
{
	if ((Flags() & B_SUPPORTS_LAYOUT) == 0)
		return;

	BRect titleRect;
	BRect outlineRect;
	BRect vScrollBarRect;
	BRect hScrollBarRect;
	_GetChildViewRects(Bounds(), titleRect, outlineRect, vScrollBarRect,
		hScrollBarRect);

	fTitleView->MoveTo(titleRect.LeftTop());
	fTitleView->ResizeTo(titleRect.Width(), titleRect.Height());

	fOutlineView->MoveTo(outlineRect.LeftTop());
	fOutlineView->ResizeTo(outlineRect.Width(), outlineRect.Height());

	fVerticalScrollBar->MoveTo(vScrollBarRect.LeftTop());
	fVerticalScrollBar->ResizeTo(vScrollBarRect.Width(),
		vScrollBarRect.Height());

	if (fStatusView != NULL) {
		BSize size = fStatusView->MinSize();
		if (size.height > B_H_SCROLL_BAR_HEIGHT)
			size.height = B_H_SCROLL_BAR_HEIGHT;
		if (size.width > Bounds().Width() / 2)
			size.width = floorf(Bounds().Width() / 2);

		BPoint offset(hScrollBarRect.LeftTop());

		if (fBorderStyle == B_PLAIN_BORDER) {
			offset += BPoint(0, 1);
		} else if (fBorderStyle == B_FANCY_BORDER) {
			offset += BPoint(-1, 2);
			size.height -= 1;
		}

		fStatusView->MoveTo(offset);
		fStatusView->ResizeTo(size.width, size.height);
		hScrollBarRect.left = offset.x + size.width + 1;
	}

	fHorizontalScrollBar->MoveTo(hScrollBarRect.LeftTop());
	fHorizontalScrollBar->ResizeTo(hScrollBarRect.Width(),
		hScrollBarRect.Height());

	fOutlineView->FixScrollBar(true);
}


void
BColumnListView::_Init()
{
	SetViewColor(B_TRANSPARENT_32_BIT);

	BRect bounds(Bounds());
	if (bounds.Width() <= 0)
		bounds.right = 100;

	if (bounds.Height() <= 0)
		bounds.bottom = 100;

	fCustomColors = false;
	_UpdateColors();

	BRect titleRect;
	BRect outlineRect;
	BRect vScrollBarRect;
	BRect hScrollBarRect;
	_GetChildViewRects(bounds, titleRect, outlineRect, vScrollBarRect,
		hScrollBarRect);

	fOutlineView = new OutlineView(outlineRect, &fColumns, &fSortColumns, this);
	AddChild(fOutlineView);


	fTitleView = new TitleView(titleRect, fOutlineView, &fColumns,
		&fSortColumns, this, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	AddChild(fTitleView);

	fVerticalScrollBar = new BScrollBar(vScrollBarRect, "vertical_scroll_bar",
		fOutlineView, 0.0, bounds.Height(), B_VERTICAL);
	AddChild(fVerticalScrollBar);

	fHorizontalScrollBar = new BScrollBar(hScrollBarRect,
		"horizontal_scroll_bar", fTitleView, 0.0, bounds.Width(), B_HORIZONTAL);
	AddChild(fHorizontalScrollBar);

	if (!fShowingHorizontalScrollBar)
		fHorizontalScrollBar->Hide();

	fOutlineView->FixScrollBar(true);
}


void
BColumnListView::_UpdateColors()
{
	if (fCustomColors)
		return;

	fColorList[B_COLOR_BACKGROUND] = ui_color(B_LIST_BACKGROUND_COLOR);
	fColorList[B_COLOR_TEXT] = ui_color(B_LIST_ITEM_TEXT_COLOR);
	fColorList[B_COLOR_ROW_DIVIDER] = tint_color(
		ui_color(B_LIST_SELECTED_BACKGROUND_COLOR), B_DARKEN_2_TINT);
	fColorList[B_COLOR_SELECTION] = ui_color(B_LIST_SELECTED_BACKGROUND_COLOR);
	fColorList[B_COLOR_SELECTION_TEXT] =
		ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR);

	// For non focus selection uses the selection color as BListView
	fColorList[B_COLOR_NON_FOCUS_SELECTION] =
		ui_color(B_LIST_SELECTED_BACKGROUND_COLOR);

	// edit mode doesn't work very well
	fColorList[B_COLOR_EDIT_BACKGROUND] = tint_color(
		ui_color(B_LIST_SELECTED_BACKGROUND_COLOR), B_DARKEN_1_TINT);
	fColorList[B_COLOR_EDIT_BACKGROUND].alpha = 180;

	// Unused color
	fColorList[B_COLOR_EDIT_TEXT] = ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR);

	fColorList[B_COLOR_HEADER_BACKGROUND] = ui_color(B_PANEL_BACKGROUND_COLOR);
	fColorList[B_COLOR_HEADER_TEXT] = ui_color(B_PANEL_TEXT_COLOR);

	// Unused colors
	fColorList[B_COLOR_SEPARATOR_LINE] = ui_color(B_LIST_ITEM_TEXT_COLOR);
	fColorList[B_COLOR_SEPARATOR_BORDER] = ui_color(B_LIST_ITEM_TEXT_COLOR);
}


void
BColumnListView::_GetChildViewRects(const BRect& bounds, BRect& titleRect,
	BRect& outlineRect, BRect& vScrollBarRect, BRect& hScrollBarRect)
{
	titleRect = bounds;
	titleRect.bottom = titleRect.top + std::max(kMinTitleHeight,
		ceilf(be_plain_font->Size() * kTitleSpacing));
#if !LOWER_SCROLLBAR
	titleRect.right -= B_V_SCROLL_BAR_WIDTH;
#endif

	outlineRect = bounds;
	outlineRect.top = titleRect.bottom + 1.0;
	outlineRect.right -= B_V_SCROLL_BAR_WIDTH;
	if (fShowingHorizontalScrollBar)
		outlineRect.bottom -= B_H_SCROLL_BAR_HEIGHT;

	vScrollBarRect = bounds;
#if LOWER_SCROLLBAR
	vScrollBarRect.top += std::max(kMinTitleHeight,
		ceilf(be_plain_font->Size() * kTitleSpacing));
#endif

	vScrollBarRect.left = vScrollBarRect.right - B_V_SCROLL_BAR_WIDTH;
	if (fShowingHorizontalScrollBar)
		vScrollBarRect.bottom -= B_H_SCROLL_BAR_HEIGHT;

	hScrollBarRect = bounds;
	hScrollBarRect.top = hScrollBarRect.bottom - B_H_SCROLL_BAR_HEIGHT;
	hScrollBarRect.right -= B_V_SCROLL_BAR_WIDTH;

	// Adjust stuff so the border will fit.
	if (fBorderStyle == B_PLAIN_BORDER || fBorderStyle == B_NO_BORDER) {
		titleRect.InsetBy(1, 0);
		titleRect.OffsetBy(0, 1);
		outlineRect.InsetBy(1, 1);
	} else if (fBorderStyle == B_FANCY_BORDER) {
		titleRect.InsetBy(2, 0);
		titleRect.OffsetBy(0, 2);
		outlineRect.InsetBy(2, 2);

		vScrollBarRect.OffsetBy(-1, 0);
#if LOWER_SCROLLBAR
		vScrollBarRect.top += 2;
		vScrollBarRect.bottom -= 1;
#else
		vScrollBarRect.InsetBy(0, 1);
#endif
		hScrollBarRect.OffsetBy(0, -1);
		hScrollBarRect.InsetBy(1, 0);
	}
}


// #pragma mark -


TitleView::TitleView(BRect rect, OutlineView* horizontalSlave,
	BList* visibleColumns, BList* sortColumns, BColumnListView* listView,
	uint32 resizingMode)
	:
	BView(rect, "title_view", resizingMode, B_WILL_DRAW | B_FRAME_EVENTS),
	fOutlineView(horizontalSlave),
	fColumns(visibleColumns),
	fSortColumns(sortColumns),
//	fColumnsWidth(0),
	fVisibleRect(rect.OffsetToCopy(0, 0)),
	fCurrentState(INACTIVE),
	fColumnPop(NULL),
	fMasterView(listView),
	fEditMode(false),
	fColumnFlags(B_ALLOW_COLUMN_MOVE | B_ALLOW_COLUMN_RESIZE
		| B_ALLOW_COLUMN_POPUP | B_ALLOW_COLUMN_REMOVE)
{
	SetViewColor(B_TRANSPARENT_COLOR);

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

	fUpSortArrow = new BBitmap(BRect(0, 0, 7, 7), B_CMAP8);
	fDownSortArrow = new BBitmap(BRect(0, 0, 7, 7), B_CMAP8);

	fUpSortArrow->SetBits((const void*) kUpSortArrow8x8, 64, 0, B_CMAP8);
	fDownSortArrow->SetBits((const void*) kDownSortArrow8x8, 64, 0, B_CMAP8);

	fResizeCursor = new BCursor(B_CURSOR_ID_RESIZE_EAST_WEST);
	fMinResizeCursor = new BCursor(B_CURSOR_ID_RESIZE_EAST);
	fMaxResizeCursor = new BCursor(B_CURSOR_ID_RESIZE_WEST);
	fColumnMoveCursor = new BCursor(B_CURSOR_ID_MOVE);

	FixScrollBar(true);
}


TitleView::~TitleView()
{
	delete fColumnPop;
	fColumnPop = NULL;

#if DOUBLE_BUFFERED_COLUMN_RESIZE
	delete fDrawBuffer;
#endif
	delete fUpSortArrow;
	delete fDownSortArrow;

	delete fResizeCursor;
	delete fMaxResizeCursor;
	delete fMinResizeCursor;
	delete fColumnMoveCursor;
}


void
TitleView::ColumnAdded(BColumn* column)
{
//	fColumnsWidth += column->Width();
	FixScrollBar(false);
	Invalidate();
}


void
TitleView::ColumnResized(BColumn* column, float oldWidth)
{
//	fColumnsWidth += column->Width() - oldWidth;
	FixScrollBar(false);
	Invalidate();
}


void
TitleView::SetColumnVisible(BColumn* column, bool visible)
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

//	if (visible)
//		fColumnsWidth += column->Width();
//	else
//		fColumnsWidth -= column->Width();

	BRect outlineInvalid(fOutlineView->VisibleRect());
	outlineInvalid.left = titleInvalid.left;
	titleInvalid.right = outlineInvalid.right;

	Invalidate(titleInvalid);
	fOutlineView->Invalidate(outlineInvalid);

	FixScrollBar(false);
}


void
TitleView::GetTitleRect(BColumn* findColumn, BRect* _rect)
{
	float leftEdge = MAX(kLeftMargin, fMasterView->LatchWidth());
	int32 numColumns = fColumns->CountItems();
	for (int index = 0; index < numColumns; index++) {
		BColumn* column = (BColumn*) fColumns->ItemAt(index);
		if (!column->IsVisible())
			continue;

		if (column == findColumn) {
			_rect->Set(leftEdge, 0, leftEdge + column->Width(),
				fVisibleRect.bottom);
			return;
		}

		leftEdge += column->Width() + 1;
	}

	TRESPASS();
}


int32
TitleView::FindColumn(BPoint position, float* _leftEdge)
{
	float leftEdge = MAX(kLeftMargin, fMasterView->LatchWidth());
	int32 numColumns = fColumns->CountItems();
	for (int index = 0; index < numColumns; index++) {
		BColumn* column = (BColumn*) fColumns->ItemAt(index);
		if (!column->IsVisible())
			continue;

		if (leftEdge > position.x)
			break;

		if (position.x >= leftEdge
			&& position.x <= leftEdge + column->Width()) {
			*_leftEdge = leftEdge;
			return index;
		}

		leftEdge += column->Width() + 1;
	}

	return 0;
}


void
TitleView::FixScrollBar(bool scrollToFit)
{
	BScrollBar* hScrollBar = ScrollBar(B_HORIZONTAL);
	if (hScrollBar == NULL)
		return;

	float virtualWidth = _VirtualWidth();

	if (virtualWidth > fVisibleRect.Width()) {
		hScrollBar->SetProportion(fVisibleRect.Width() / virtualWidth);

		// Perform the little trick if the user is scrolled over too far.
		// See OutlineView::FixScrollBar for a more in depth explanation
		float maxScrollBarValue = virtualWidth - fVisibleRect.Width();
		if (scrollToFit || hScrollBar->Value() <= maxScrollBarValue) {
			hScrollBar->SetRange(0.0, maxScrollBarValue);
			hScrollBar->SetSteps(50, fVisibleRect.Width());
		}
	} else if (hScrollBar->Value() == 0.0) {
		// disable scroll bar.
		hScrollBar->SetRange(0.0, 0.0);
	}
}


void
TitleView::DragSelectedColumn(BPoint position)
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


void
TitleView::MoveColumn(BColumn* column, int32 index)
{
	fColumns->RemoveItem((void*) column);

	if (-1 == index) {
		// Re-add the column at the end of the list.
		fColumns->AddItem((void*) column);
	} else {
		fColumns->AddItem((void*) column, index);
	}
}


void
TitleView::SetColumnFlags(column_flags flags)
{
	fColumnFlags = flags;
}


float
TitleView::MarginWidth() const
{
	return MAX(kLeftMargin, fMasterView->LatchWidth()) + kRightMargin;
}


void
TitleView::ResizeSelectedColumn(BPoint position, bool preferred)
{
	float minWidth = fSelectedColumn->MinWidth();
	float maxWidth = fSelectedColumn->MaxWidth();

	float oldWidth = fSelectedColumn->Width();
	float originalEdge = fSelectedColumnRect.left + oldWidth;
	if (preferred) {
		float width = fOutlineView->GetColumnPreferredWidth(fSelectedColumn);
		fSelectedColumn->SetWidth(width);
	} else if (position.x > fSelectedColumnRect.left + maxWidth)
		fSelectedColumn->SetWidth(maxWidth);
	else if (position.x < fSelectedColumnRect.left + minWidth)
		fSelectedColumn->SetWidth(minWidth);
	else
		fSelectedColumn->SetWidth(position.x - fSelectedColumnRect.left - 1);

	float dX = fSelectedColumnRect.left + fSelectedColumn->Width()
		 - originalEdge;
	if (dX != 0) {
		float columnHeight = fVisibleRect.Height();
		BRect originalRect(originalEdge, 0, 1000000.0, columnHeight);
		BRect movedRect(originalRect);
		movedRect.OffsetBy(dX, 0);

		// Update the size of the title column
		BRect sourceRect(0, 0, fSelectedColumn->Width(), columnHeight);
		BRect destRect(sourceRect);
		destRect.OffsetBy(fSelectedColumnRect.left, 0);

#if DOUBLE_BUFFERED_COLUMN_RESIZE
		fDrawBuffer->Lock();
		DrawTitle(fDrawBufferView, sourceRect, fSelectedColumn, false);
		fDrawBufferView->Sync();
		fDrawBuffer->Unlock();

		CopyBits(originalRect, movedRect);
		DrawBitmap(fDrawBuffer, sourceRect, destRect);
#else
		CopyBits(originalRect, movedRect);
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

//		fColumnsWidth += dX;

		// Update the cursor
		if (fSelectedColumn->Width() == minWidth)
			SetViewCursor(fMinResizeCursor, true);
		else if (fSelectedColumn->Width() == maxWidth)
			SetViewCursor(fMaxResizeCursor, true);
		else
			SetViewCursor(fResizeCursor, true);

		ColumnResized(fSelectedColumn, oldWidth);
	}
}


void
TitleView::ComputeDragBoundries(BColumn* findColumn, BPoint)
{
	float previousColumnLeftEdge = -1000000.0;
	float nextColumnRightEdge = 1000000.0;

	bool foundColumn = false;
	float leftEdge = MAX(kLeftMargin, fMasterView->LatchWidth());
	int32 numColumns = fColumns->CountItems();
	for (int index = 0; index < numColumns; index++) {
		BColumn* column = (BColumn*) fColumns->ItemAt(index);
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

	fLeftDragBoundry = MIN(previousColumnLeftEdge + findColumn->Width(),
		leftEdge);
	fRightDragBoundry = MAX(nextColumnRightEdge, rightEdge);
}


void
TitleView::DrawTitle(BView* view, BRect rect, BColumn* column, bool depressed)
{
	BRect drawRect;
	drawRect = rect;

	font_height fh;
	GetFontHeight(&fh);

	float baseline = floor(drawRect.top + fh.ascent
		+ (drawRect.Height() + 1 - (fh.ascent + fh.descent)) / 2);

	BRect bgRect = rect;

	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	view->SetHighColor(tint_color(base, B_DARKEN_2_TINT));
	view->StrokeLine(bgRect.LeftBottom(), bgRect.RightBottom());

	bgRect.bottom--;
	bgRect.right--;

	if (depressed)
		base = tint_color(base, B_DARKEN_1_TINT);

	be_control_look->DrawButtonBackground(view, bgRect, rect, base, 0,
		BControlLook::B_TOP_BORDER | BControlLook::B_BOTTOM_BORDER);

	view->SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
		B_DARKEN_2_TINT));
	view->StrokeLine(rect.RightTop(), rect.RightBottom());

	// If no column given, nothing else to draw.
	if (column == NULL)
		return;

	view->SetHighColor(fMasterView->Color(B_COLOR_HEADER_TEXT));

	BFont font;
	GetFont(&font);
	view->SetFont(&font);

	int sortIndex = fSortColumns->IndexOf(column);
	if (sortIndex >= 0) {
		// Draw sort notation.
		BPoint upperLeft(drawRect.right - kSortIndicatorWidth, baseline);

		if (fSortColumns->CountItems() > 1) {
			char str[256];
			sprintf(str, "%d", sortIndex + 1);
			const float w = view->StringWidth(str);
			upperLeft.x -= w;

			view->SetDrawingMode(B_OP_COPY);
			view->MovePenTo(BPoint(upperLeft.x + kSortIndicatorWidth,
				baseline));
			view->DrawString(str);
		}

		float bmh = fDownSortArrow->Bounds().Height()+1;

		view->SetDrawingMode(B_OP_OVER);

		if (column->fSortAscending) {
			BPoint leftTop(upperLeft.x, drawRect.top + (drawRect.IntegerHeight()
				- fDownSortArrow->Bounds().IntegerHeight()) / 2);
			view->DrawBitmapAsync(fDownSortArrow, leftTop);
		} else {
			BPoint leftTop(upperLeft.x, drawRect.top + (drawRect.IntegerHeight()
				- fUpSortArrow->Bounds().IntegerHeight()) / 2);
			view->DrawBitmapAsync(fUpSortArrow, leftTop);
		}

		upperLeft.y = baseline - bmh + floor((fh.ascent + fh.descent - bmh) / 2);
		if (upperLeft.y < drawRect.top)
			upperLeft.y = drawRect.top;

		// Adjust title stuff for sort indicator
		drawRect.right = upperLeft.x - 2;
	}

	if (drawRect.right > drawRect.left) {
#if CONSTRAIN_CLIPPING_REGION
		BRegion clipRegion(drawRect);
		view->PushState();
		view->ConstrainClippingRegion(&clipRegion);
#endif
		view->MovePenTo(BPoint(drawRect.left + 8, baseline));
		view->SetDrawingMode(B_OP_OVER);
		view->SetHighColor(fMasterView->Color(B_COLOR_HEADER_TEXT));
		column->DrawTitle(drawRect, view);

#if CONSTRAIN_CLIPPING_REGION
		view->PopState();
#endif
	}
}


float
TitleView::_VirtualWidth() const
{
	float width = MarginWidth();

	int32 count = fColumns->CountItems();
	for (int32 i = 0; i < count; i++) {
		BColumn* column = reinterpret_cast<BColumn*>(fColumns->ItemAt(i));
		if (column->IsVisible())
			width += column->Width();
	}

	return width;
}


void
TitleView::Draw(BRect invalidRect)
{
	float columnLeftEdge = MAX(kLeftMargin, fMasterView->LatchWidth());
	for (int32 columnIndex = 0; columnIndex < fColumns->CountItems();
		columnIndex++) {

		BColumn* column = (BColumn*) fColumns->ItemAt(columnIndex);
		if (!column->IsVisible())
			continue;

		if (columnLeftEdge > invalidRect.right)
			break;

		if (columnLeftEdge + column->Width() >= invalidRect.left) {
			BRect titleRect(columnLeftEdge, 0,
				columnLeftEdge + column->Width(), fVisibleRect.Height());
			DrawTitle(this, titleRect, column,
				(fCurrentState == DRAG_COLUMN_INSIDE_TITLE
				&& fSelectedColumn == column));
		}

		columnLeftEdge += column->Width() + 1;
	}


	// bevels for right title margin
	if (columnLeftEdge <= invalidRect.right) {
		BRect titleRect(columnLeftEdge, 0, Bounds().right + 2,
			fVisibleRect.Height());
		DrawTitle(this, titleRect, NULL, false);
	}

	// bevels for left title margin
	if (invalidRect.left < MAX(kLeftMargin, fMasterView->LatchWidth())) {
		BRect titleRect(0, 0, MAX(kLeftMargin, fMasterView->LatchWidth()) - 1,
			fVisibleRect.Height());
		DrawTitle(this, titleRect, NULL, false);
	}

#if DRAG_TITLE_OUTLINE
	// (internal) column drag indicator
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


void
TitleView::ScrollTo(BPoint position)
{
	fOutlineView->ScrollBy(position.x - fVisibleRect.left, 0);
	fVisibleRect.OffsetTo(position.x, position.y);

	// Perform the little trick if the user is scrolled over too far.
	// See OutlineView::ScrollTo for a more in depth explanation
	float maxScrollBarValue = _VirtualWidth() - fVisibleRect.Width();
	BScrollBar* hScrollBar = ScrollBar(B_HORIZONTAL);
	float min, max;
	hScrollBar->GetRange(&min, &max);
	if (max != maxScrollBarValue && position.x > maxScrollBarValue)
		FixScrollBar(true);

	_inherited::ScrollTo(position);
}


void
TitleView::MessageReceived(BMessage* message)
{
	if (message->what == kToggleColumn) {
		int32 num;
		if (message->FindInt32("be:field_num", &num) == B_OK) {
			for (int index = 0; index < fColumns->CountItems(); index++) {
				BColumn* column = (BColumn*) fColumns->ItemAt(index);
				if (column == NULL)
					continue;

				if (column->LogicalFieldNum() == num)
					column->SetVisible(!column->IsVisible());
			}
		}
		return;
	}

	BView::MessageReceived(message);
}


void
TitleView::MouseDown(BPoint position)
{
	if (fEditMode)
		return;

	int32 buttons = 1;
	Window()->CurrentMessage()->FindInt32("buttons", &buttons);
	if (buttons == B_SECONDARY_MOUSE_BUTTON
		&& (fColumnFlags & B_ALLOW_COLUMN_POPUP)) {
		// Right mouse button -- bring up menu to show/hide columns.
		if (fColumnPop == NULL)
			fColumnPop = new BPopUpMenu("Columns", false, false);

		fColumnPop->RemoveItems(0, fColumnPop->CountItems(), true);
		BMessenger me(this);
		for (int index = 0; index < fColumns->CountItems(); index++) {
			BColumn* column = (BColumn*) fColumns->ItemAt(index);
			if (column == NULL)
				continue;

			BString name;
			column->GetColumnName(&name);
			BMessage* message = new BMessage(kToggleColumn);
			message->AddInt32("be:field_num", column->LogicalFieldNum());
			BMenuItem* item = new BMenuItem(name.String(), message);
			item->SetMarked(column->IsVisible());
			item->SetTarget(me);
			fColumnPop->AddItem(item);
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
		BColumn* column = (BColumn*)fColumns->ItemAt(index);
		if (column == NULL || !column->IsVisible())
			continue;

		if (leftEdge > position.x + kColumnResizeAreaWidth / 2)
			break;

		// check for resizing a column
		float rightEdge = leftEdge + column->Width();

		if (column->ShowHeading()) {
			if (position.x > rightEdge - kColumnResizeAreaWidth / 2
				&& position.x < rightEdge + kColumnResizeAreaWidth / 2
				&& column->MaxWidth() > column->MinWidth()
				&& (fColumnFlags & B_ALLOW_COLUMN_RESIZE) != 0) {

				int32 clicks = 0;
				fSelectedColumn = column;
				fSelectedColumnRect.Set(leftEdge, 0, rightEdge,
					fVisibleRect.Height());
				Window()->CurrentMessage()->FindInt32("clicks", &clicks);
				if (clicks == 2 || buttons == B_TERTIARY_MOUSE_BUTTON) {
					ResizeSelectedColumn(position, true);
					fCurrentState = INACTIVE;
					break;
				}
				fCurrentState = RESIZING_COLUMN;
				fClickPoint = BPoint(position.x - rightEdge - 1,
					position.y - fSelectedColumnRect.top);
				SetMouseEventMask(B_POINTER_EVENTS,
					B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY);
				break;
			}

			fResizingFirstColumn = false;

			// check for clicking on a column
			if (position.x > leftEdge && position.x < rightEdge) {
				fCurrentState = PRESSING_COLUMN;
				fSelectedColumn = column;
				fSelectedColumnRect.Set(leftEdge, 0, rightEdge,
					fVisibleRect.Height());
				DrawTitle(this, fSelectedColumnRect, fSelectedColumn, true);
				fClickPoint = BPoint(position.x - fSelectedColumnRect.left,
					position.y - fSelectedColumnRect.top);
				SetMouseEventMask(B_POINTER_EVENTS,
					B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY);
				break;
			}
		}
		leftEdge = rightEdge + 1;
	}
}


void
TitleView::MouseMoved(BPoint position, uint32 transit,
	const BMessage* dragMessage)
{
	if (fEditMode)
		return;

	// Handle column manipulation
	switch (fCurrentState) {
		case RESIZING_COLUMN:
			ResizeSelectedColumn(position - BPoint(fClickPoint.x, 0));
			break;

		case PRESSING_COLUMN: {
			if (abs((int32)(position.x - (fClickPoint.x
					+ fSelectedColumnRect.left))) > kColumnResizeAreaWidth
				|| abs((int32)(position.y - (fClickPoint.y
					+ fSelectedColumnRect.top))) > kColumnResizeAreaWidth) {
				// User has moved the mouse more than the tolerable amount,
				// initiate a drag.
				if (transit == B_INSIDE_VIEW || transit == B_ENTERED_VIEW) {
					if(fColumnFlags & B_ALLOW_COLUMN_MOVE) {
						fCurrentState = DRAG_COLUMN_INSIDE_TITLE;
						ComputeDragBoundries(fSelectedColumn, position);
						SetViewCursor(fColumnMoveCursor, true);
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

						// There is a race condition where the mouse may have
						// moved by the time we get to handle this message.
						// If the user drags a column very quickly, this
						// results in the annoying bug where the cursor is
						// outside of the rectangle that is being dragged
						// around.  Call GetMouse with the checkQueue flag set
						// to false so we can get the most recent position of
						// the mouse.  This minimizes this problem (although
						// it is currently not possible to completely eliminate
						// it).
						uint32 buttons;
						GetMouse(&position, &buttons, false);
						dragRect.OffsetTo(position.x - fClickPoint.x,
							position.y - dragRect.Height() / 2);
						BeginRectTracking(dragRect, B_TRACK_WHOLE_RECT);
					}
				}
			}

			break;
		}

		case DRAG_COLUMN_INSIDE_TITLE: {
			if (transit == B_EXITED_VIEW
				&& (fColumnFlags & B_ALLOW_COLUMN_REMOVE)) {
				// Dragged outside view
				fCurrentState = DRAG_COLUMN_OUTSIDE_TITLE;
				fSelectedColumn->SetVisible(false);
				BRect dragRect(fSelectedColumnRect);

				// See explanation above.
				uint32 buttons;
				GetMouse(&position, &buttons, false);

				dragRect.OffsetTo(position.x - fClickPoint.x,
					position.y - fClickPoint.y);
				BeginRectTracking(dragRect, B_TRACK_WHOLE_RECT);
			} else if (position.x < fLeftDragBoundry
				|| position.x > fRightDragBoundry) {
				DragSelectedColumn(position - BPoint(fClickPoint.x, 0));
			}

#if DRAG_TITLE_OUTLINE
			// Set up the invalid rect to include the rect for the previous
			// position of the drag rect, as well as the new one.
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
				DragSelectedColumn(position - BPoint(fClickPoint.x, 0));
			}

			break;

		case INACTIVE:
			// Check for cursor changes if we are over the resize area for
			// a column.
			BColumn* resizeColumn = 0;
			float leftEdge = MAX(kLeftMargin, fMasterView->LatchWidth());
			for (int index = 0; index < fColumns->CountItems(); index++) {
				BColumn* column = (BColumn*) fColumns->ItemAt(index);
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
			if (resizeColumn) {
				if (resizeColumn->Width() == resizeColumn->MinWidth())
					SetViewCursor(fMinResizeCursor, true);
				else if (resizeColumn->Width() == resizeColumn->MaxWidth())
					SetViewCursor(fMaxResizeCursor, true);
				else
					SetViewCursor(fResizeCursor, true);
			} else
				SetViewCursor(B_CURSOR_SYSTEM_DEFAULT, true);
			break;
	}
}


void
TitleView::MouseUp(BPoint position)
{
	if (fEditMode)
		return;

	switch (fCurrentState) {
		case RESIZING_COLUMN:
			ResizeSelectedColumn(position - BPoint(fClickPoint.x, 0));
			fCurrentState = INACTIVE;
			FixScrollBar(false);
			break;

		case PRESSING_COLUMN: {
			if (fMasterView->SortingEnabled()) {
				if (fSortColumns->HasItem(fSelectedColumn)) {
					if ((modifiers() & B_CONTROL_KEY) == 0
						&& fSortColumns->CountItems() > 1) {
						fSortColumns->MakeEmpty();
						fSortColumns->AddItem(fSelectedColumn);
					}

					fSelectedColumn->fSortAscending
						= !fSelectedColumn->fSortAscending;
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
			SetViewCursor(B_CURSOR_SYSTEM_DEFAULT, true);
			break;

		case DRAG_COLUMN_OUTSIDE_TITLE:
			fCurrentState = INACTIVE;
			EndRectTracking();
			SetViewCursor(B_CURSOR_SYSTEM_DEFAULT, true);
			break;

		default:
			;
	}
}


void
TitleView::FrameResized(float width, float height)
{
	fVisibleRect.right = fVisibleRect.left + width;
	fVisibleRect.bottom = fVisibleRect.top + height;
	FixScrollBar(true);
}


// #pragma mark - OutlineView


OutlineView::OutlineView(BRect rect, BList* visibleColumns, BList* sortColumns,
	BColumnListView* listView)
	:
	BView(rect, "outline_view", B_FOLLOW_ALL_SIDES,
		B_WILL_DRAW | B_FRAME_EVENTS),
	fColumns(visibleColumns),
	fSortColumns(sortColumns),
	fItemsHeight(0.0),
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
	SetViewColor(B_TRANSPARENT_COLOR);

#if DOUBLE_BUFFERED_COLUMN_RESIZE
	// TODO: This needs to be smart about the size of the buffer.
	// Also, the buffer can be shared with the title's buffer.
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
#if DOUBLE_BUFFERED_COLUMN_RESIZE
	delete fDrawBuffer;
#endif

	Clear();
}


void
OutlineView::Clear()
{
	DeselectAll();
		// Make sure selection list doesn't point to deleted rows!
	RecursiveDeleteRows(&fRows, false);
	fItemsHeight = 0.0;
	FixScrollBar(true);
	Invalidate();
}


void
OutlineView::SetSelectionMode(list_view_type mode)
{
	DeselectAll();
	fSelectionMode = mode;
}


list_view_type
OutlineView::SelectionMode() const
{
	return fSelectionMode;
}


void
OutlineView::Deselect(BRow* row)
{
	if (row == NULL)
		return;

	if (row->fNextSelected != 0) {
		row->fNextSelected->fPrevSelected = row->fPrevSelected;
		row->fPrevSelected->fNextSelected = row->fNextSelected;
		row->fNextSelected = 0;
		row->fPrevSelected = 0;
		Invalidate();
	}
}


void
OutlineView::AddToSelection(BRow* row)
{
	if (row == NULL)
		return;

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


void
OutlineView::RecursiveDeleteRows(BRowContainer* list, bool isOwner)
{
	if (list == NULL)
		return;

	while (true) {
		BRow* row = list->RemoveItemAt(0L);
		if (row == 0)
			break;

		if (row->fChildList)
			RecursiveDeleteRows(row->fChildList, true);

		delete row;
	}

	if (isOwner)
		delete list;
}


void
OutlineView::RedrawColumn(BColumn* column, float leftEdge, bool isFirstColumn)
{
	// TODO: Remove code duplication (private function which takes a view
	// pointer, pass "this" in non-double buffered mode)!
	// Watch out for sourceRect versus destRect though!
	if (!column)
		return;

	font_height fh;
	GetFontHeight(&fh);
	float line = 0.0;
	bool tintedLine = true;
	for (RecursiveOutlineIterator iterator(&fRows); iterator.CurrentRow();
		line += iterator.CurrentRow()->Height() + 1, iterator.GoToNext()) {

		BRow* row = iterator.CurrentRow();
		float rowHeight = row->Height();
		if (line > fVisibleRect.bottom)
			break;
		tintedLine = !tintedLine;

		if (line + rowHeight >= fVisibleRect.top) {
#if DOUBLE_BUFFERED_COLUMN_RESIZE
			BRect sourceRect(0, 0, column->Width(), rowHeight);
#endif
			BRect destRect(leftEdge, line, leftEdge + column->Width(),
				line + rowHeight);

			rgb_color highColor;
			rgb_color lowColor;
			if (row->fNextSelected != 0) {
				if (fEditMode) {
					highColor = fMasterView->Color(B_COLOR_EDIT_BACKGROUND);
					lowColor = fMasterView->Color(B_COLOR_EDIT_BACKGROUND);
				} else {
					highColor = fMasterView->Color(B_COLOR_SELECTION);
					lowColor = fMasterView->Color(B_COLOR_SELECTION);
				}
			} else {
				highColor = fMasterView->Color(B_COLOR_BACKGROUND);
				lowColor = fMasterView->Color(B_COLOR_BACKGROUND);
			}
			if (tintedLine)
				lowColor = tint_color(lowColor, kTintedLineTint);


#if DOUBLE_BUFFERED_COLUMN_RESIZE
			fDrawBuffer->Lock();

			fDrawBufferView->SetHighColor(highColor);
			fDrawBufferView->SetLowColor(lowColor);

			BFont font;
			GetFont(&font);
			fDrawBufferView->SetFont(&font);
			fDrawBufferView->FillRect(sourceRect, B_SOLID_LOW);

			if (isFirstColumn) {
				// If this is the first column, double buffer drawing the latch
				// too.
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

			BField* field = row->GetField(column->fFieldID);
			if (field) {
				BRect fieldRect(sourceRect);
				if (isFirstColumn)
					fieldRect.left += fMasterView->LatchWidth();

	#if CONSTRAIN_CLIPPING_REGION
				BRegion clipRegion(fieldRect);
				fDrawBufferView->PushState();
				fDrawBufferView->ConstrainClippingRegion(&clipRegion);
	#endif
				fDrawBufferView->SetHighColor(fMasterView->Color(
					row->fNextSelected ? B_COLOR_SELECTION_TEXT
						: B_COLOR_TEXT));
				float baseline = floor(fieldRect.top + fh.ascent
					+ (fieldRect.Height() + 1 - (fh.ascent+fh.descent)) / 2);
				fDrawBufferView->MovePenTo(fieldRect.left + 8, baseline);
				column->DrawField(field, fieldRect, fDrawBufferView);
	#if CONSTRAIN_CLIPPING_REGION
				fDrawBufferView->PopState();
	#endif
			}

			if (fFocusRow == row && !fEditMode && fMasterView->IsFocus()
				&& Window()->IsActive()) {
				fDrawBufferView->SetHighColor(fMasterView->Color(
					B_COLOR_ROW_DIVIDER));
				fDrawBufferView->StrokeRect(BRect(-1, sourceRect.top,
					10000.0, sourceRect.bottom));
			}

			fDrawBufferView->Sync();
			fDrawBuffer->Unlock();
			SetDrawingMode(B_OP_COPY);
			DrawBitmap(fDrawBuffer, sourceRect, destRect);

#else

			SetHighColor(highColor);
			SetLowColor(lowColor);
			FillRect(destRect, B_SOLID_LOW);

			BField* field = row->GetField(column->fFieldID);
			if (field) {
	#if CONSTRAIN_CLIPPING_REGION
				BRegion clipRegion(destRect);
				PushState();
				ConstrainClippingRegion(&clipRegion);
	#endif
				SetHighColor(fMasterView->Color(row->fNextSelected
					? B_COLOR_SELECTION_TEXT : B_COLOR_TEXT));
				float baseline = floor(destRect.top + fh.ascent
					+ (destRect.Height() + 1 - (fh.ascent + fh.descent)) / 2);
				MovePenTo(destRect.left + 8, baseline);
				column->DrawField(field, destRect, this);
	#if CONSTRAIN_CLIPPING_REGION
				PopState();
	#endif
			}

			if (fFocusRow == row && !fEditMode && fMasterView->IsFocus()
				&& Window()->IsActive()) {
				SetHighColor(fMasterView->Color(B_COLOR_ROW_DIVIDER));
				StrokeRect(BRect(0, destRect.top, 10000.0, destRect.bottom));
			}
#endif
		}
	}
}


void
OutlineView::Draw(BRect invalidBounds)
{
#if SMART_REDRAW
	BRegion invalidRegion;
	GetClippingRegion(&invalidRegion);
#endif

	font_height fh;
	GetFontHeight(&fh);

	float line = 0.0;
	bool tintedLine = true;
	int32 numColumns = fColumns->CountItems();
	for (RecursiveOutlineIterator iterator(&fRows); iterator.CurrentRow();
		iterator.GoToNext()) {
		BRow* row = iterator.CurrentRow();
		if (line > invalidBounds.bottom)
			break;

		tintedLine = !tintedLine;
		float rowHeight = row->Height();

		if (line >= invalidBounds.top - rowHeight) {
			bool isFirstColumn = true;
			float fieldLeftEdge = MAX(kLeftMargin, fMasterView->LatchWidth());

			// setup background color
			rgb_color lowColor;
			if (row->fNextSelected != 0) {
				if (Window()->IsActive()) {
					if (fEditMode)
						lowColor = fMasterView->Color(B_COLOR_EDIT_BACKGROUND);
					else
						lowColor = fMasterView->Color(B_COLOR_SELECTION);
				}
				else
					lowColor = fMasterView->Color(B_COLOR_NON_FOCUS_SELECTION);
			} else
				lowColor = fMasterView->Color(B_COLOR_BACKGROUND);
			if (tintedLine)
				lowColor = tint_color(lowColor, kTintedLineTint);

			for (int columnIndex = 0; columnIndex < numColumns; columnIndex++) {
				BColumn* column = (BColumn*) fColumns->ItemAt(columnIndex);
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

					SetHighColor(lowColor);

					BRect destRect(fullRect);
					if (isFirstColumn) {
						fullRect.left -= fMasterView->LatchWidth();
						destRect.left += iterator.CurrentLevel()
							* kOutlineLevelIndent;
						if (destRect.left >= destRect.right) {
							// clipped
							FillRect(BRect(0, line, fieldLeftEdge
								+ column->Width(), line + rowHeight));
							clippedFirstColumn = true;
						}

						FillRect(BRect(0, line, MAX(kLeftMargin,
							fMasterView->LatchWidth()), line + row->Height()));
					}


#if SMART_REDRAW
					if (!clippedFirstColumn
						&& invalidRegion.Intersects(fullRect)) {
#else
					if (!clippedFirstColumn) {
#endif
						FillRect(fullRect);	// Using color set above

						// Draw the latch widget if it has one.
						if (isFirstColumn) {
							if (row == fTargetRow
								&& fCurrentState == LATCH_CLICKED) {
								// Note that this only occurs if the user is
								// holding down a latch while items are added
								// in the background.
								BPoint pos;
								uint32 buttons;
								GetMouse(&pos, &buttons);
								if (fLatchRect.Contains(pos)) {
									fMasterView->DrawLatch(this, fLatchRect,
										B_PRESSED_LATCH, fTargetRow);
								} else {
									fMasterView->DrawLatch(this, fLatchRect,
										row->fIsExpanded ? B_OPEN_LATCH
											: B_CLOSED_LATCH, fTargetRow);
								}
							} else {
								LatchType pos = B_NO_LATCH;
								if (row->HasLatch())
									pos = row->fIsExpanded ? B_OPEN_LATCH
										: B_CLOSED_LATCH;

								fMasterView->DrawLatch(this,
									BRect(destRect.left
										- fMasterView->LatchWidth(),
									destRect.top, destRect.left,
									destRect.bottom), pos, row);
							}
						}

						SetHighColor(fMasterView->HighColor());
							// The master view just holds the high color for us.
						SetLowColor(lowColor);

						BField* field = row->GetField(column->fFieldID);
						if (field) {
#if CONSTRAIN_CLIPPING_REGION
							BRegion clipRegion(destRect);
							PushState();
							ConstrainClippingRegion(&clipRegion);
#endif
							SetHighColor(fMasterView->Color(
								row->fNextSelected ? B_COLOR_SELECTION_TEXT
								: B_COLOR_TEXT));
							float baseline = floor(destRect.top + fh.ascent
								+ (destRect.Height() + 1
								- (fh.ascent+fh.descent)) / 2);
							MovePenTo(destRect.left + 8, baseline);
							column->DrawField(field, destRect, this);
#if CONSTRAIN_CLIPPING_REGION
							PopState();
#endif
						}
					}
				}

				isFirstColumn = false;
				fieldLeftEdge += column->Width() + 1;
			}

			if (fieldLeftEdge <= invalidBounds.right) {
				SetHighColor(lowColor);
				FillRect(BRect(fieldLeftEdge, line, invalidBounds.right,
					line + rowHeight));
			}
		}

		// indicate the keyboard focus row
		if (fFocusRow == row && !fEditMode && fMasterView->IsFocus()
			&& Window()->IsActive()) {
			SetHighColor(fMasterView->Color(B_COLOR_ROW_DIVIDER));
			StrokeRect(BRect(0, line, 10000.0, line + rowHeight));
		}

		line += rowHeight + 1;
	}

	if (line <= invalidBounds.bottom) {
		// fill background below last item
		SetHighColor(fMasterView->Color(B_COLOR_BACKGROUND));
		FillRect(BRect(invalidBounds.left, line, invalidBounds.right,
			invalidBounds.bottom));
	}

	// Draw the drop target line
	if (fDropHighlightY != -1) {
		InvertRect(BRect(0, fDropHighlightY - kDropHighlightLineHeight / 2,
			1000000, fDropHighlightY + kDropHighlightLineHeight / 2));
	}
}


BRow*
OutlineView::FindRow(float ypos, int32* _rowIndent, float* _top)
{
	if (_rowIndent && _top) {
		float line = 0.0;
		for (RecursiveOutlineIterator iterator(&fRows); iterator.CurrentRow();
			iterator.GoToNext()) {

			BRow* row = iterator.CurrentRow();
			if (line > ypos)
				break;

			float rowHeight = row->Height();
			if (ypos <= line + rowHeight) {
				*_top = line;
				*_rowIndent = iterator.CurrentLevel();
				return row;
			}

			line += rowHeight + 1;
		}
	}

	return NULL;
}

void OutlineView::SetMouseTrackingEnabled(bool enabled)
{
	fTrackMouse = enabled;
	if (!enabled && fDropHighlightY != -1) {
		// Erase the old target line
		InvertRect(BRect(0, fDropHighlightY - kDropHighlightLineHeight / 2,
			1000000, fDropHighlightY + kDropHighlightLineHeight / 2));
		fDropHighlightY = -1;
	}
}


//
// Note that this interaction is not totally safe.  If items are added to
// the list in the background, the widget rect will be incorrect, possibly
// resulting in drawing glitches.  The code that adds items needs to be a little smarter
// about invalidating state.
//
void
OutlineView::MouseDown(BPoint position)
{
	if (!fEditMode)
		fMasterView->MakeFocus(true);

	// Check to see if the user is clicking on a widget to open a section
	// of the list.
	bool reset_click_count = false;
	int32 indent;
	float rowTop;
	BRow* row = FindRow(position.y, &indent, &rowTop);
	if (row != NULL) {

		// Update fCurrentField
		bool handle_field = false;
		BField* new_field = 0;
		BRow* new_row = 0;
		BColumn* new_column = 0;
		BRect new_rect;

		if (position.y >= 0) {
			if (position.x >= 0) {
				float x = 0;
				for (int32 c = 0; c < fMasterView->CountColumns(); c++) {
					new_column = fMasterView->ColumnAt(c);
					if (!new_column->IsVisible())
						continue;
					if ((MAX(kLeftMargin, fMasterView->LatchWidth()) + x)
						+ new_column->Width() >= position.x) {
						if (new_column->WantsEvents()) {
							new_field = row->GetField(c);
							new_row = row;
							FindRect(new_row,&new_rect);
							new_rect.left = MAX(kLeftMargin,
								fMasterView->LatchWidth()) + x;
							new_rect.right = new_rect.left
								+ new_column->Width() - 1;
							handle_field = true;
						}
						break;
					}
					x += new_column->Width();
				}
			}
		}

		// Handle mouse down
		if (handle_field) {
			fMouseDown = true;
			fFieldRect = new_rect;
			fCurrentColumn = new_column;
			fCurrentRow = new_row;
			fCurrentField = new_field;
			fCurrentCode = B_INSIDE_VIEW;
			BMessage* message = Window()->CurrentMessage();
			int32 buttons = 1;
			message->FindInt32("buttons", &buttons);
			fCurrentColumn->MouseDown(fMasterView, fCurrentRow,
				fCurrentField, fFieldRect, position, buttons);
		}

		if (!fEditMode) {

			fTargetRow = row;
			fTargetRowTop = rowTop;
			FindVisibleRect(fFocusRow, &fFocusRowRect);

			float leftWidgetBoundry = indent * kOutlineLevelIndent
				+ MAX(kLeftMargin, fMasterView->LatchWidth())
				- fMasterView->LatchWidth();
			fLatchRect.Set(leftWidgetBoundry, rowTop, leftWidgetBoundry
				+ fMasterView->LatchWidth(), rowTop + row->Height());
			if (fLatchRect.Contains(position) && row->HasLatch()) {
				fCurrentState = LATCH_CLICKED;
				if (fTargetRow->fNextSelected != 0)
					SetHighColor(fMasterView->Color(B_COLOR_SELECTION));
				else
					SetHighColor(fMasterView->Color(B_COLOR_BACKGROUND));

				FillRect(fLatchRect);
				if (fLatchRect.Contains(position)) {
					fMasterView->DrawLatch(this, fLatchRect, B_PRESSED_LATCH,
						row);
				} else {
					fMasterView->DrawLatch(this, fLatchRect,
						fTargetRow->fIsExpanded ? B_OPEN_LATCH
						: B_CLOSED_LATCH, row);
				}
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
						fTargetRow->fNextSelected->fPrevSelected
							= fTargetRow->fPrevSelected;
						fTargetRow->fPrevSelected->fNextSelected
							= fTargetRow->fNextSelected;
						fTargetRow->fPrevSelected = 0;
						fTargetRow->fNextSelected = 0;
						fFirstSelectedItem = NULL;
					} else {
						// Select row
						if (fSelectionMode == B_SINGLE_SELECTION_LIST)
							DeselectAll();

						fTargetRow->fNextSelected
							= fSelectionListDummyHead.fNextSelected;
						fTargetRow->fPrevSelected
							= &fSelectionListDummyHead;
						fTargetRow->fNextSelected->fPrevSelected = fTargetRow;
						fTargetRow->fPrevSelected->fNextSelected = fTargetRow;
						fFirstSelectedItem = fTargetRow;
					}

					Invalidate(BRect(fVisibleRect.left, fTargetRowTop,
						fVisibleRect.right,
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
		FindVisibleRect(fFocusRow, &fFocusRowRect);
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

}


void
OutlineView::MouseMoved(BPoint position, uint32 /*transit*/,
	const BMessage* /*dragMessage*/)
{
	if (!fMouseDown) {
		// Update fCurrentField
		bool handle_field = false;
		BField* new_field = 0;
		BRow* new_row = 0;
		BColumn* new_column = 0;
		BRect new_rect(0,0,0,0);
		if (position.y >=0 ) {
			float top;
			int32 indent;
			BRow* row = FindRow(position.y, &indent, &top);
			if (row && position.x >=0 ) {
				float x=0;
				for (int32 c=0;c<fMasterView->CountColumns();c++) {
					new_column = fMasterView->ColumnAt(c);
					if (!new_column->IsVisible())
						continue;
					if ((MAX(kLeftMargin,
						fMasterView->LatchWidth()) + x) + new_column->Width()
						> position.x) {

						if(new_column->WantsEvents()) {
							new_field = row->GetField(c);
							new_row = row;
							FindRect(new_row,&new_rect);
							new_rect.left = MAX(kLeftMargin,
								fMasterView->LatchWidth()) + x;
							new_rect.right = new_rect.left
								+ new_column->Width() - 1;
							handle_field = true;
						}
						break;
					}
					x += new_column->Width();
				}
			}
		}

		// Handle mouse moved
		if (handle_field) {
			if (new_field != fCurrentField) {
				if (fCurrentField) {
					fCurrentColumn->MouseMoved(fMasterView, fCurrentRow,
						fCurrentField, fFieldRect, position, 0,
						fCurrentCode = B_EXITED_VIEW);
				}
				fCurrentColumn = new_column;
				fCurrentRow = new_row;
				fCurrentField = new_field;
				fFieldRect = new_rect;
				if (fCurrentField) {
					fCurrentColumn->MouseMoved(fMasterView, fCurrentRow,
						fCurrentField, fFieldRect, position, 0,
						fCurrentCode = B_ENTERED_VIEW);
				}
			} else {
				if (fCurrentField) {
					fCurrentColumn->MouseMoved(fMasterView, fCurrentRow,
						fCurrentField, fFieldRect, position, 0,
						fCurrentCode = B_INSIDE_VIEW);
				}
			}
		} else {
			if (fCurrentField) {
				fCurrentColumn->MouseMoved(fMasterView, fCurrentRow,
					fCurrentField, fFieldRect, position, 0,
					fCurrentCode = B_EXITED_VIEW);
				fCurrentField = 0;
				fCurrentColumn = 0;
				fCurrentRow = 0;
			}
		}
	} else {
		if (fCurrentField) {
			if (fFieldRect.Contains(position)) {
				if (fCurrentCode == B_OUTSIDE_VIEW
					|| fCurrentCode == B_EXITED_VIEW) {
					fCurrentColumn->MouseMoved(fMasterView, fCurrentRow,
						fCurrentField, fFieldRect, position, 1,
						fCurrentCode = B_ENTERED_VIEW);
				} else {
					fCurrentColumn->MouseMoved(fMasterView, fCurrentRow,
						fCurrentField, fFieldRect, position, 1,
						fCurrentCode = B_INSIDE_VIEW);
				}
			} else {
				if (fCurrentCode == B_INSIDE_VIEW
					|| fCurrentCode == B_ENTERED_VIEW) {
					fCurrentColumn->MouseMoved(fMasterView, fCurrentRow,
						fCurrentField, fFieldRect, position, 1,
						fCurrentCode = B_EXITED_VIEW);
				} else {
					fCurrentColumn->MouseMoved(fMasterView, fCurrentRow,
						fCurrentField, fFieldRect, position, 1,
						fCurrentCode = B_OUTSIDE_VIEW);
				}
			}
		}
	}

	if (!fEditMode) {

		switch (fCurrentState) {
			case LATCH_CLICKED:
				if (fTargetRow->fNextSelected != 0)
					SetHighColor(fMasterView->Color(B_COLOR_SELECTION));
				else
					SetHighColor(fMasterView->Color(B_COLOR_BACKGROUND));

				FillRect(fLatchRect);
				if (fLatchRect.Contains(position)) {
					fMasterView->DrawLatch(this, fLatchRect, B_PRESSED_LATCH,
						fTargetRow);
				} else {
					fMasterView->DrawLatch(this, fLatchRect,
						fTargetRow->fIsExpanded ? B_OPEN_LATCH : B_CLOSED_LATCH,
						fTargetRow);
				}
				break;

			case ROW_CLICKED:
				if (abs((int)(position.x - fClickPoint.x)) > kRowDragSensitivity
					|| abs((int)(position.y - fClickPoint.y))
						> kRowDragSensitivity) {
					fCurrentState = DRAGGING_ROWS;
					fMasterView->InitiateDrag(fClickPoint,
						fTargetRow->fNextSelected != 0);
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
						BRow* target = FindRow(position.y, &indent, &top);
						if (target)
							SetFocusRow(target, true);
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
						BRow* target = FindRow(position.y, &indent, &top);
						if (target == fRollOverRow)
							break;
						if (fRollOverRow) {
							BRect rect;
							FindRect(fRollOverRow, &rect);
							Invalidate(rect);
						}
						fRollOverRow = target;
#if 0
						SetFocusRow(fRollOverRow,false);
#else
						PushState();
						SetDrawingMode(B_OP_BLEND);
						SetHighColor(255, 255, 255, 255);
						BRect rect;
						FindRect(fRollOverRow, &rect);
						rect.bottom -= 1.0;
						FillRect(rect);
						PopState();
#endif
					} else {
						if (fRollOverRow) {
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
}


void
OutlineView::MouseUp(BPoint position)
{
	if (fCurrentField) {
		fCurrentColumn->MouseUp(fMasterView, fCurrentRow, fCurrentField);
		fMouseDown = false;
	}

	if (fEditMode)
		return;

	switch (fCurrentState) {
		case LATCH_CLICKED:
			if (fLatchRect.Contains(position)) {
				fMasterView->ExpandOrCollapse(fTargetRow,
					!fTargetRow->fIsExpanded);
			}

			Invalidate(fLatchRect);
			fCurrentState = INACTIVE;
			break;

		case ROW_CLICKED:
			if (fClickCount > 1
				&& abs((int)fClickPoint.x - (int)position.x)
					< kDoubleClickMoveSensitivity
				&& abs((int)fClickPoint.y - (int)position.y)
					< kDoubleClickMoveSensitivity) {
				fMasterView->ItemInvoked();
			}
			fCurrentState = INACTIVE;
			break;

		case DRAGGING_ROWS:
			fCurrentState = INACTIVE;
			// Falls through

		default:
			if (fDropHighlightY != -1) {
				InvertRect(BRect(0,
					fDropHighlightY - kDropHighlightLineHeight / 2,
					1000000, fDropHighlightY + kDropHighlightLineHeight / 2));
					// Erase the old target line
				fDropHighlightY = -1;
			}
	}
}


void
OutlineView::MessageReceived(BMessage* message)
{
	if (message->WasDropped()) {
		fMasterView->MessageDropped(message,
			ConvertFromScreen(message->DropPoint()));
	} else {
		BView::MessageReceived(message);
	}
}


void
OutlineView::ChangeFocusRow(bool up, bool updateSelection,
	bool addToCurrentSelection)
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

	BRow* newRow = FindRow(newRowPos, &indent, &top);
	if (newRow) {
		if (fFocusRow) {
			fFocusRowRect.right = 10000;
			Invalidate(fFocusRowRect);
		}
		BRow* oldFocusRow = fFocusRow;
		fFocusRow = newRow;
		fFocusRowRect.top = top;
		fFocusRowRect.left = 0;
		fFocusRowRect.right = 10000;
		fFocusRowRect.bottom = fFocusRowRect.top + fFocusRow->Height();
		Invalidate(fFocusRowRect);

		if (updateSelection) {
			if (!addToCurrentSelection
				|| fSelectionMode == B_SINGLE_SELECTION_LIST) {
				DeselectAll();
			}

			// if the focus row isn't selected, add it to the selection
			if (fFocusRow->fNextSelected == 0) {
				fFocusRow->fNextSelected
					= fSelectionListDummyHead.fNextSelected;
				fFocusRow->fPrevSelected = &fSelectionListDummyHead;
				fFocusRow->fNextSelected->fPrevSelected = fFocusRow;
				fFocusRow->fPrevSelected->fNextSelected = fFocusRow;
			} else if (oldFocusRow != NULL
				&& fSelectionListDummyHead.fNextSelected == oldFocusRow
				&& (((IndexOf(oldFocusRow->fNextSelected)
						< IndexOf(oldFocusRow)) == up)
					|| fFocusRow == oldFocusRow->fNextSelected)) {
					// if the focus row is selected, if:
					// 1. the previous focus row is last in the selection
					// 2a. the next selected row is now the focus row
					// 2b. or the next selected row is beyond the focus row
					//	   in the move direction
					// then deselect the previous focus row
				fSelectionListDummyHead.fNextSelected
					= oldFocusRow->fNextSelected;
				if (fSelectionListDummyHead.fNextSelected != NULL) {
					fSelectionListDummyHead.fNextSelected->fPrevSelected
						= &fSelectionListDummyHead;
					oldFocusRow->fNextSelected = NULL;
				}
				oldFocusRow->fPrevSelected = NULL;
			}

			fLastSelectedItem = fFocusRow;
		}
	} else
		Invalidate(fFocusRowRect);

	if (verticalScroll != 0) {
		BScrollBar* vScrollBar = ScrollBar(B_VERTICAL);
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


void
OutlineView::MoveFocusToVisibleRect()
{
	fFocusRow = 0;
	ChangeFocusRow(true, true, false);
}


BRow*
OutlineView::CurrentSelection(BRow* lastSelected) const
{
	BRow* row;
	if (lastSelected == 0)
		row = fSelectionListDummyHead.fNextSelected;
	else
		row = lastSelected->fNextSelected;


	if (row == &fSelectionListDummyHead)
		row = 0;

	return row;
}


void
OutlineView::ToggleFocusRowSelection(bool selectRange)
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


void
OutlineView::ToggleFocusRowOpen()
{
	if (fFocusRow)
		fMasterView->ExpandOrCollapse(fFocusRow, !fFocusRow->fIsExpanded);
}


void
OutlineView::ExpandOrCollapse(BRow* parentRow, bool expand)
{
	// TODO: Could use CopyBits here to speed things up.

	if (parentRow == NULL)
		return;

	if (parentRow->fIsExpanded == expand)
		return;

	parentRow->fIsExpanded = expand;

	BRect parentRect;
	if (FindRect(parentRow, &parentRect)) {
		// Determine my new height
		float subTreeHeight = 0.0;
		if (parentRow->fIsExpanded)
			for (RecursiveOutlineIterator iterator(parentRow->fChildList);
			     iterator.CurrentRow();
			     iterator.GoToNext()
			    )
			{
				subTreeHeight += iterator.CurrentRow()->Height()+1;
			}
		else
			for (RecursiveOutlineIterator iterator(parentRow->fChildList);
			     iterator.CurrentRow();
			     iterator.GoToNext()
			    )
			{
				subTreeHeight -= iterator.CurrentRow()->Height()+1;
			}
		fItemsHeight += subTreeHeight;

		// Adjust focus row if necessary.
		if (FindRect(fFocusRow, &fFocusRowRect) == false) {
			// focus row is in a subtree that has collapsed,
			// move it up to the parent.
			fFocusRow = parentRow;
			FindRect(fFocusRow, &fFocusRowRect);
		}

		Invalidate(BRect(0, parentRect.top, fVisibleRect.right,
			fVisibleRect.bottom));
		FixScrollBar(false);
	}
}

void
OutlineView::RemoveRow(BRow* row)
{
	if (row == NULL)
		return;

	BRow* parentRow;
	bool parentIsVisible;
	FindParent(row, &parentRow, &parentIsVisible);
		// NOTE: This could be a root row without a parent, in which case
		// it is always visible, though.

	// Adjust height for the visible sub-tree that is going to be removed.
	float subTreeHeight = 0.0f;
	if (parentIsVisible && (parentRow == NULL || parentRow->fIsExpanded)) {
		// The row itself is visible at least.
		subTreeHeight = row->Height() + 1;
		if (row->fIsExpanded) {
			// Adjust for the height of visible sub-items as well.
			// (By default, the iterator follows open branches only.)
			for (RecursiveOutlineIterator iterator(row->fChildList);
				iterator.CurrentRow(); iterator.GoToNext())
				subTreeHeight += iterator.CurrentRow()->Height() + 1;
		}
		BRect invalid;
		if (FindRect(row, &invalid)) {
			invalid.bottom = Bounds().bottom;
			if (invalid.IsValid())
				Invalidate(invalid);
		}
	}

	fItemsHeight -= subTreeHeight;

	FixScrollBar(false);
	int32 indent = 0;
	float top = 0.0;
	if (FindRow(fVisibleRect.top, &indent, &top) == NULL && ScrollBar(B_VERTICAL) != NULL) {
		// after removing this row, no rows are actually visible any more,
		// force a scroll to make them visible again
		if (fItemsHeight > fVisibleRect.Height())
			ScrollBy(0.0, fItemsHeight - fVisibleRect.Height() - Bounds().top);
		else
			ScrollBy(0.0, -Bounds().top);
	}
	if (parentRow != NULL) {
		parentRow->fChildList->RemoveItem(row);
		if (parentRow->fChildList->CountItems() == 0) {
			delete parentRow->fChildList;
			parentRow->fChildList = 0;
			// It was the last child row of the parent, which also means the
			// latch disappears.
			BRect parentRowRect;
			if (parentIsVisible && FindRect(parentRow, &parentRowRect))
				Invalidate(parentRowRect);
		}
	} else
		fRows.RemoveItem(row);

	// Adjust focus row if necessary.
	if (fFocusRow && !FindRect(fFocusRow, &fFocusRowRect)) {
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


BRowContainer*
OutlineView::RowList()
{
	return &fRows;
}


void
OutlineView::UpdateRow(BRow* row)
{
	if (row) {
		// Determine if this row has changed its sort order
		BRow* parentRow = NULL;
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


void
OutlineView::AddRow(BRow* row, int32 Index, BRow* parentRow)
{
	if (!row)
		return;

	row->fParent = parentRow;

	if (fMasterView->SortingEnabled() && !fSortColumns->IsEmpty()) {
		// Ignore index here.
		if (parentRow) {
			if (parentRow->fChildList == NULL)
				parentRow->fChildList = new BRowContainer;

			AddSorted(parentRow->fChildList, row);
		} else
			AddSorted(&fRows, row);
	} else {
		// Note, a -1 index implies add to end if sorting is not enabled
		if (parentRow) {
			if (parentRow->fChildList == 0)
				parentRow->fChildList = new BRowContainer;

			if (Index < 0 || Index > parentRow->fChildList->CountItems())
				parentRow->fChildList->AddItem(row);
			else
				parentRow->fChildList->AddItem(row, Index);
		} else {
			if (Index < 0 || Index >= fRows.CountItems())
				fRows.AddItem(row);
			else
				fRows.AddItem(row, Index);
		}
	}

	if (parentRow == 0 || parentRow->fIsExpanded)
		fItemsHeight += row->Height() + 1;

	FixScrollBar(false);

	BRect newRowRect;
	bool newRowIsInOpenBranch = FindRect(row, &newRowRect);

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
				source.bottom -= row->Height() + 1;
				dest.top += row->Height() + 1;
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

	// If the parent was previously childless, it will need to have a latch
	// drawn.
	BRect parentRect;
	if (parentRow && parentRow->fChildList->CountItems() == 1
		&& FindVisibleRect(parentRow, &parentRect))
		Invalidate(parentRect);
}


void
OutlineView::FixScrollBar(bool scrollToFit)
{
	BScrollBar* vScrollBar = ScrollBar(B_VERTICAL);
	if (vScrollBar) {
		if (fItemsHeight > fVisibleRect.Height()) {
			float maxScrollBarValue = fItemsHeight - fVisibleRect.Height();
			vScrollBar->SetProportion(fVisibleRect.Height() / fItemsHeight);

			// If the user is scrolled down too far when making the range smaller, the list
			// will jump suddenly, which is undesirable.  In this case, don't fix the scroll
			// bar here. In ScrollTo, it checks to see if this has occured, and will
			// fix the scroll bars sneakily if the user has scrolled up far enough.
			if (scrollToFit || vScrollBar->Value() <= maxScrollBarValue) {
				vScrollBar->SetRange(0.0, maxScrollBarValue);
				vScrollBar->SetSteps(20.0, fVisibleRect.Height());
			}
		} else if (vScrollBar->Value() == 0.0 || fItemsHeight == 0.0)
			vScrollBar->SetRange(0.0, 0.0);		// disable scroll bar.
	}
}


void
OutlineView::AddSorted(BRowContainer* list, BRow* row)
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


int32
OutlineView::CompareRows(BRow* row1, BRow* row2)
{
	int32 itemCount (fSortColumns->CountItems());
	if (row1 && row2) {
		for (int32 index = 0; index < itemCount; index++) {
			BColumn* column = (BColumn*) fSortColumns->ItemAt(index);
			int comp = 0;
			BField* field1 = (BField*) row1->GetField(column->fFieldID);
			BField* field2 = (BField*) row2->GetField(column->fFieldID);
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


void
OutlineView::FrameResized(float width, float height)
{
	fVisibleRect.right = fVisibleRect.left + width;
	fVisibleRect.bottom = fVisibleRect.top + height;
	FixScrollBar(true);
	_inherited::FrameResized(width, height);
}


void
OutlineView::ScrollTo(BPoint position)
{
	fVisibleRect.OffsetTo(position.x, position.y);

	// In FixScrollBar, we might not have been able to change the size of
	// the scroll bar because the user was scrolled down too far.  Take
	// this opportunity to sneak it in if we can.
	BScrollBar* vScrollBar = ScrollBar(B_VERTICAL);
	float maxScrollBarValue = fItemsHeight - fVisibleRect.Height();
	float min, max;
	vScrollBar->GetRange(&min, &max);
	if (max != maxScrollBarValue && position.y > maxScrollBarValue)
		FixScrollBar(true);

	_inherited::ScrollTo(position);
}


const BRect&
OutlineView::VisibleRect() const
{
	return fVisibleRect;
}


bool
OutlineView::FindVisibleRect(BRow* row, BRect* _rect)
{
	if (row && _rect) {
		float line = 0.0;
		for (RecursiveOutlineIterator iterator(&fRows); iterator.CurrentRow();
			iterator.GoToNext()) {

			if (iterator.CurrentRow() == row) {
				_rect->Set(fVisibleRect.left, line, fVisibleRect.right,
					line + row->Height());
				return line <= fVisibleRect.bottom;
			}

			line += iterator.CurrentRow()->Height() + 1;
		}
	}
	return false;
}


bool
OutlineView::FindRect(const BRow* row, BRect* _rect)
{
	float line = 0.0;
	for (RecursiveOutlineIterator iterator(&fRows); iterator.CurrentRow();
		iterator.GoToNext()) {
		if (iterator.CurrentRow() == row) {
			_rect->Set(fVisibleRect.left, line, fVisibleRect.right,
				line + row->Height());
			return true;
		}

		line += iterator.CurrentRow()->Height() + 1;
	}

	return false;
}


void
OutlineView::ScrollTo(const BRow* row)
{
	BRect rect;
	if (FindRect(row, &rect)) {
		BRect bounds = Bounds();
		if (rect.top < bounds.top)
			ScrollTo(BPoint(bounds.left, rect.top));
		else if (rect.bottom > bounds.bottom)
			ScrollBy(0, rect.bottom - bounds.bottom);
	}
}


void
OutlineView::DeselectAll()
{
	// Invalidate all selected rows
	float line = 0.0;
	for (RecursiveOutlineIterator iterator(&fRows); iterator.CurrentRow();
		iterator.GoToNext()) {
		if (line > fVisibleRect.bottom)
			break;

		BRow* row = iterator.CurrentRow();
		if (line + row->Height() > fVisibleRect.top) {
			if (row->fNextSelected != 0)
				Invalidate(BRect(fVisibleRect.left, line, fVisibleRect.right,
					line + row->Height()));
		}

		line += row->Height() + 1;
	}

	// Set items not selected
	while (fSelectionListDummyHead.fNextSelected != &fSelectionListDummyHead) {
		BRow* row = fSelectionListDummyHead.fNextSelected;
		row->fNextSelected->fPrevSelected = row->fPrevSelected;
		row->fPrevSelected->fNextSelected = row->fNextSelected;
		row->fNextSelected = 0;
		row->fPrevSelected = 0;
	}
}


BRow*
OutlineView::FocusRow() const
{
	return fFocusRow;
}


void
OutlineView::SetFocusRow(BRow* row, bool Select)
{
	if (row) {
		if (Select)
			AddToSelection(row);

		if (fFocusRow == row)
			return;

		Invalidate(fFocusRowRect); // invalidate previous

		fTargetRow = fFocusRow = row;

		FindVisibleRect(fFocusRow, &fFocusRowRect);
		Invalidate(fFocusRowRect); // invalidate current

		fFocusRowRect.right = 10000;
		fMasterView->SelectionChanged();
	}
}


bool
OutlineView::SortList(BRowContainer* list, bool isVisible)
{
	if (list) {
		// Shellsort
		BRow** items
			= (BRow**) BObjectList<BRow>::Private(list).AsBList()->Items();
		int32 numItems = list->CountItems();
		int h;
		for (h = 1; h < numItems / 9; h = 3 * h + 1)
			;

		for (;h > 0; h /= 3) {
			for (int step = h; step < numItems; step++) {
				BRow* temp = items[step];
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


int32
OutlineView::DeepSortThreadEntry(void* _outlineView)
{
	((OutlineView*) _outlineView)->DeepSort();
	return 0;
}


void
OutlineView::DeepSort()
{
	struct stack_entry {
		bool isVisible;
		BRowContainer* list;
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

		stack_entry* currentEntry = &stack[stackTop];

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
				BRow* parentRow = currentEntry->list->ItemAt(index);
				BRowContainer* childList = parentRow->fChildList;
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


void
OutlineView::StartSorting()
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


void
OutlineView::SelectRange(BRow* start, BRow* end)
{
	if (!start || !end)
		return;

	if (start == end)	// start is always selected when this is called
		return;

	RecursiveOutlineIterator iterator(&fRows, false);
	while (iterator.CurrentRow() != 0) {
		if (iterator.CurrentRow() == end) {
			// reverse selection, swap to fix special case
			BRow* temp = start;
			start = end;
			end = temp;
			break;
		} else if (iterator.CurrentRow() == start)
			break;

		iterator.GoToNext();
	}

	while (true) {
		BRow* row = iterator.CurrentRow();
		if (row) {
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


bool
OutlineView::FindParent(BRow* row, BRow** outParent, bool* outParentIsVisible)
{
	bool result = false;
	if (row != NULL && outParent != NULL) {
		*outParent = row->fParent;

		if (outParentIsVisible != NULL) {
			// Walk up the parent chain to determine if this row is visible
			*outParentIsVisible = true;
			for (BRow* currentRow = row->fParent; currentRow != NULL;
				currentRow = currentRow->fParent) {
				if (!currentRow->fIsExpanded) {
					*outParentIsVisible = false;
					break;
				}
			}
		}

		result = *outParent != NULL;
	}

	return result;
}


int32
OutlineView::IndexOf(BRow* row)
{
	if (row) {
		if (row->fParent == 0)
			return fRows.IndexOf(row);

		ASSERT(row->fParent->fChildList);
		return row->fParent->fChildList->IndexOf(row);
	}

	return B_ERROR;
}


void
OutlineView::InvalidateCachedPositions()
{
	if (fFocusRow)
		FindRect(fFocusRow, &fFocusRowRect);
}


float
OutlineView::GetColumnPreferredWidth(BColumn* column)
{
	float preferred = 0.0;
	for (RecursiveOutlineIterator iterator(&fRows); BRow* row =
		iterator.CurrentRow(); iterator.GoToNext()) {
		BField* field = row->GetField(column->fFieldID);
		if (field) {
			float width = column->GetPreferredWidth(field, this)
				+ iterator.CurrentLevel() * kOutlineLevelIndent;
			preferred = max_c(preferred, width);
		}
	}

	BString name;
	column->GetColumnName(&name);
	preferred = max_c(preferred, StringWidth(name));

	// Constrain to preferred width. This makes the method do a little
	// more than asked, but it's for convenience.
	if (preferred < column->MinWidth())
		preferred = column->MinWidth();
	else if (preferred > column->MaxWidth())
		preferred = column->MaxWidth();

	return preferred;
}


// #pragma mark -


RecursiveOutlineIterator::RecursiveOutlineIterator(BRowContainer* list,
	bool openBranchesOnly)
	:
	fStackIndex(0),
	fCurrentListIndex(0),
	fCurrentListDepth(0),
	fOpenBranchesOnly(openBranchesOnly)
{
	if (list == 0 || list->CountItems() == 0)
		fCurrentList = 0;
	else
		fCurrentList = list;
}


BRow*
RecursiveOutlineIterator::CurrentRow() const
{
	if (fCurrentList == 0)
		return 0;

	return fCurrentList->ItemAt(fCurrentListIndex);
}


void
RecursiveOutlineIterator::GoToNext()
{
	if (fCurrentList == 0)
		return;
	if (fCurrentListIndex < 0 || fCurrentListIndex >= fCurrentList->CountItems()) {
		fCurrentList = 0;
		return;
	}

	BRow* currentRow = fCurrentList->ItemAt(fCurrentListIndex);
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


int32
RecursiveOutlineIterator::CurrentLevel() const
{
	return fCurrentListDepth;
}


