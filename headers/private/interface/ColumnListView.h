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
/	File:			ColumnListView.h
/
/   Description:    Experimental multi-column list view.
/
/	Copyright 2000+, Be Incorporated, All Rights Reserved
/
*******************************************************************************/


#ifndef _COLUMN_LIST_VIEW_H
#define _COLUMN_LIST_VIEW_H

#include <BeBuild.h>
#include <View.h>
#include <List.h>
#include <Invoker.h>
#include <ListView.h>

class BScrollBar;

namespace BPrivate {

class OutlineView;
class TitleView;
class BRowContainer;
class RecursiveOutlineIterator;

}	// ns BPrivate

class BField;
class BRow;
class BColumn;
class BColumnListView;

enum LatchType {
	B_NO_LATCH					= 0,
	B_OPEN_LATCH				= 1,
	B_PRESSED_LATCH				= 2,
	B_CLOSED_LATCH				= 3
};

typedef enum {
	B_ALLOW_COLUMN_NONE			= 0,
	B_ALLOW_COLUMN_MOVE			= 1,
	B_ALLOW_COLUMN_RESIZE		= 2,
	B_ALLOW_COLUMN_POPUP		= 4,
	B_ALLOW_COLUMN_REMOVE		= 8
} column_flags;

enum ColumnListViewColor {
	B_COLOR_BACKGROUND			= 0,
	B_COLOR_TEXT				= 1,
	B_COLOR_ROW_DIVIDER			= 2,
	B_COLOR_SELECTION			= 3,
	B_COLOR_SELECTION_TEXT		= 4,
	B_COLOR_NON_FOCUS_SELECTION	= 5,
	B_COLOR_EDIT_BACKGROUND		= 6,
	B_COLOR_EDIT_TEXT			= 7,
	B_COLOR_HEADER_BACKGROUND	= 8,
	B_COLOR_HEADER_TEXT			= 9,
	B_COLOR_SEPARATOR_LINE		= 10,
	B_COLOR_SEPARATOR_BORDER	= 11,

	B_COLOR_TOTAL				= 12
};

enum ColumnListViewFont {
	B_FONT_ROW					= 0,
	B_FONT_HEADER				= 1,

	B_FONT_TOTAL				= 2
};


// A single row/column intersection in the list.
class BField {
public:
								BField();
	virtual						~BField();
};

// A single line in the list.  Each line contains a BField object
// for each column in the list, associated by their "logical field"
// index.  Hierarchies are formed by adding other BRow objects as
// a parent of a row, using the AddRow() function in BColumnListView().
class BRow {
public:
								BRow(float height = 16.0);
	virtual 					~BRow();
	virtual bool		 		HasLatch() const;

			int32				CountFields() const;
			BField*				GetField(int32 logicalFieldIndex);
	const	BField*				GetField(int32 logicalFieldIndex) const;
			void				SetField(BField* field,
									int32 logicalFieldIndex);

			float 				Height() const;
			bool 				IsExpanded() const;

private:
	// Blows up into the debugger if the validation fails.
			void				ValidateFields() const;
			void				ValidateField(const BField* field,
									int32 logicalFieldIndex) const;
private:
			BList				fFields;
			BPrivate::
			BRowContainer*		fChildList;
			bool				fIsExpanded;
			float				fHeight;
			BRow*				fNextSelected;
			BRow*				fPrevSelected;
			BRow*				fParent;
			BColumnListView*	fList;


	friend class BColumnListView;
	friend class BPrivate::RecursiveOutlineIterator;
	friend class BPrivate::OutlineView;
};

// Information about a single column in the list.  A column knows
// how to display the BField objects that occur at its location in
// each of the list's rows.  See ColumnTypes.h for particular
// subclasses of BField and BColumn that handle common data types.
class BColumn {
public:
								BColumn(float width, float minWidth,
									float maxWidth,
									alignment align = B_ALIGN_LEFT);
	virtual 					~BColumn();

			float				Width() const;
			void				SetWidth(float width);
			float				MinWidth() const;
			float				MaxWidth() const;

	virtual	void				DrawTitle(BRect rect, BView* targetView);
	virtual	void				DrawField(BField* field, BRect rect,
									BView* targetView);
	virtual	int					CompareFields(BField* field1, BField* field2);

	virtual void				MouseMoved(BColumnListView* parent, BRow* row,
									BField* field, BRect fieldRect,
									BPoint point, uint32 buttons, int32 code);
	virtual void				MouseDown(BColumnListView* parent, BRow* row,
									BField* field, BRect fieldRect,
									BPoint point, uint32 buttons);
	virtual	void				MouseUp(BColumnListView* parent, BRow* row,
									BField* field);

	virtual	void				GetColumnName(BString* into) const;
	virtual	float				GetPreferredWidth(BField* field,
									BView* parent) const;

			bool				IsVisible() const;
			void				SetVisible(bool);

			bool				WantsEvents() const;
			void				SetWantsEvents(bool);

			bool				ShowHeading() const;
			void				SetShowHeading(bool);

			alignment			Alignment() const;
			void				SetAlignment(alignment);

			int32				LogicalFieldNum() const;

	/*!
		\param field The BField derivative to validate.

			Implement this function on your BColumn derivatives to validate
			BField derivatives that your BColumn will be drawing/manipulating.

			This function will be called when BFields are added to the Column,
			use dynamic_cast<> to determine if it is of a kind that your
			BColumn know how ot handle. return false if it is not.

			\note The debugger will be called if you return false from here
			with information about what type of BField and BColumn and the
			logical field index where it occured.

			\note Do not call the inherited version of this, it just returns
			true;
	  */
	virtual	bool				AcceptsField(const BField* field) const;

private:
			float				fWidth;
			float 				fMinWidth;
			float				fMaxWidth;
			bool				fVisible;
			int32				fFieldID;
			BColumnListView*	fList;
			bool				fSortAscending;
			bool				fWantsEvents;
			bool				fShowHeading;
			alignment			fAlignment;

	friend class BPrivate::OutlineView;
	friend class BColumnListView;
	friend class BPrivate::TitleView;
};

// The column list view class.
class BColumnListView : public BView, public BInvoker {
public:
								BColumnListView(BRect rect,
									const char* name, uint32 resizingMode,
									uint32 flags, border_style = B_NO_BORDER,
									bool showHorizontalScrollbar = true);
								BColumnListView(const char* name,
									uint32 flags, border_style = B_NO_BORDER,
									bool showHorizontalScrollbar = true);
	virtual						~BColumnListView();

	// Interaction
	virtual	bool				InitiateDrag(BPoint, bool wasSelected);
	virtual	void				MessageDropped(BMessage*, BPoint point);
	virtual	void				ExpandOrCollapse(BRow* row, bool expand);
	virtual	status_t			Invoke(BMessage* message = NULL);
	virtual	void				ItemInvoked();
	virtual	void				SetInvocationMessage(BMessage* message);
			BMessage* 			InvocationMessage() const;
			uint32 				InvocationCommand() const;
			BRow* 				FocusRow() const;
			void 				SetFocusRow(int32 index, bool select = false);
			void 				SetFocusRow(BRow* row, bool select = false);
			void 				SetMouseTrackingEnabled(bool);

	// Selection
			list_view_type		SelectionMode() const;
			void 				Deselect(BRow* row);
			void 				AddToSelection(BRow* row);
			void 				DeselectAll();
			BRow*				CurrentSelection(BRow* lastSelected = 0) const;
	virtual	void				SelectionChanged();
	virtual	void				SetSelectionMessage(BMessage* message);
			BMessage*			SelectionMessage();
			uint32				SelectionCommand() const;
			void				SetSelectionMode(list_view_type type);
				// list_view_type is defined in ListView.h.

	// Sorting
			void				SetSortingEnabled(bool);
			bool				SortingEnabled() const;
			void				SetSortColumn(BColumn* column, bool add,
									bool ascending);
			void				ClearSortColumns();

	// The status view is a little area in the lower left hand corner.
			void				AddStatusView(BView* view);
			BView*				RemoveStatusView();

	// Column Manipulation
			void				AddColumn(BColumn* column,
									int32 logicalFieldIndex);
			void				MoveColumn(BColumn* column, int32 index);
			void				RemoveColumn(BColumn* column);
			int32				CountColumns() const;
			BColumn*			ColumnAt(int32 index) const;
			BColumn*			ColumnAt(BPoint point) const;
			void				SetColumnVisible(BColumn* column,
									bool isVisible);
			void				SetColumnVisible(int32, bool);
			bool				IsColumnVisible(int32) const;
			void				SetColumnFlags(column_flags flags);
			void				ResizeColumnToPreferred(int32 index);
			void				ResizeAllColumnsToPreferred();

	// Row manipulation
			const BRow*			RowAt(int32 index, BRow *parent = 0) const;
			BRow*				RowAt(int32 index, BRow *parent = 0);
			const BRow*			RowAt(BPoint) const;
			BRow*				RowAt(BPoint);
			bool				GetRowRect(const BRow* row, BRect* _rect) const;
			bool				FindParent(BRow* row, BRow** _parent,
									bool *_isVisible) const;
			int32				IndexOf(BRow* row);
			int32				CountRows(BRow* parent = 0) const;
			void				AddRow(BRow* row, BRow* parent = NULL);
			void				AddRow(BRow* row, int32 index,
									BRow* parent = NULL);

			void				ScrollTo(const BRow* Row);
			void				ScrollTo(BPoint point);

	// Does not delete row or children at this time.
	// todo: Make delete row and children
			void				RemoveRow(BRow* row);

			void				UpdateRow(BRow* row);
			void				Clear();

	// Appearance (DEPRECATED)
			void				GetFont(BFont* font) const
									{ BView::GetFont(font); }
	virtual	void				SetFont(const BFont* font,
									uint32 mask = B_FONT_ALL);
	virtual	void				SetHighColor(rgb_color);
			void				SetSelectionColor(rgb_color);
			void				SetBackgroundColor(rgb_color);
			void				SetEditColor(rgb_color);
			const rgb_color		SelectionColor() const;
			const rgb_color		BackgroundColor() const;
			const rgb_color		EditColor() const;

	// Appearance (NEW STYLE)
			void				SetColor(ColumnListViewColor colorIndex,
									rgb_color color);
			void				SetFont(ColumnListViewFont fontIndex,
									const BFont* font,
									uint32 mask = B_FONT_ALL);
			rgb_color			Color(ColumnListViewColor colorIndex) const;
			void				GetFont(ColumnListViewFont fontIndex,
									BFont* font) const;

			BPoint				SuggestTextPosition(const BRow* row,
									const BColumn* column = NULL) const;

			void				SetLatchWidth(float width);
			float				LatchWidth() const;
	virtual	void				DrawLatch(BView* view, BRect frame,
									LatchType type, BRow* row);
	virtual	void				MakeFocus(bool isfocus = true);
			void				SaveState(BMessage* archive);
			void				LoadState(BMessage* archive);

			BView*				ScrollView() const
									{ return (BView*)fOutlineView; }
			void				SetEditMode(bool state);
			void				Refresh();

	virtual BSize				MinSize();
	virtual BSize				PreferredSize();
	virtual BSize				MaxSize();


protected:
	virtual	void 				MessageReceived(BMessage* message);
	virtual	void 				KeyDown(const char* bytes, int32 numBytes);
	virtual	void 				AttachedToWindow();
	virtual	void 				WindowActivated(bool active);
	virtual	void 				Draw(BRect updateRect);

	virtual	void				LayoutInvalidated(bool descendants = false);
	virtual	void				DoLayout();

private:
			void				_Init(bool showHorizontalScrollbar);
			void				_GetChildViewRects(const BRect& bounds,
									bool showHorizontalScrollBar,
									BRect& titleRect, BRect& outlineRect,
									BRect& vScrollBarRect,
									BRect& hScrollBarRect);

			rgb_color 			fColorList[B_COLOR_TOTAL];
			BPrivate::TitleView* fTitleView;
			BPrivate::OutlineView* fOutlineView;
			BList 				fColumns;
			BScrollBar*			fHorizontalScrollBar;
			BScrollBar* 		fVerticalScrollBar;
			BList				fSortColumns;
			BView*				fStatusView;
			BMessage*			fSelectionMessage;
			bool				fSortingEnabled;
			float				fLatchWidth;
			border_style		fBorderStyle;
};

#endif // _COLUMN_LIST_VIEW_H
