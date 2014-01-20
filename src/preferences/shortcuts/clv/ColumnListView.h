/*
 * Copyright 1999-2009 Jeremy Friesner
 * Copyright 2009-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 *		John Scipione, jscipione@gmail.com
 */
#ifndef COLUMN_LIST_VIEW_H
#define COLUMN_LIST_VIEW_H


#include <ListView.h>

#include "Colors.h"
#include "CLVColumn.h"
#include "PrefilledBitmap.h"
#include "ScrollViewCorner.h"


class CLVListItem;
class CLVColumnLabelView;
class CLVFillerView;
class CLVContainerView;


typedef int (*CLVCompareFuncPtr)(const CLVListItem* item1,
	const CLVListItem* item2, int32 sort_key);


class ColumnListView : public BListView {
public:
								ColumnListView(BRect frame,
									BScrollView** containerView,
	// Used to get back a pointer to the container
	// view that will hold the ColumnListView, the
	// the CLVColumnLabelView, and the scrollbars.
	// If no scroll bars or border are asked for,
	// this will act like a plain BView container.
									const char* name = NULL,
									uint32 resizingMode
										= B_FOLLOW_LEFT | B_FOLLOW_TOP,
									uint32 flags
										= B_WILL_DRAW | B_FRAME_EVENTS
											| B_NAVIGABLE,
									list_view_type type
										= B_SINGLE_SELECTION_LIST,
									bool hierarchical = false,
									bool horizontal = true,
	// which scroll bars should I add, if any
									bool vertical = true,
									border_style border = B_NO_BORDER,
	// what type of border to add, if any
									const BFont* labelFont = be_plain_font);

	virtual							~ColumnListView();

			void					AddScrollViewCorner();

	//Archival stuff
	/*** Not implemented yet
									ColumnListView(BMessage* archive);
	static							ColumnListView* Instantiate(BMessage* data);
	virtual	status_t				Archive(BMessage* data, bool deep = true)
										const;
	***/

	virtual	void					MessageReceived(BMessage* message);

		// column setup functions
	virtual	bool					AddColumn(CLVColumn* column);
		// Note that a column may only be added to
		// one ColumnListView at a time, and may not
		// be added more than once to the same
		// ColumnListView without removing it
		// inbetween
	virtual	bool					AddColumnList(BList* newColumns);
	virtual	bool					RemoveColumn(CLVColumn* column);
	virtual	bool					RemoveColumns(CLVColumn* column,
										int32 Count);
		// Finds Column in ColumnList and removes Count columns and
		// their data from the view and its items
			int32					CountColumns() const;
			int32					IndexOfColumn(CLVColumn* column) const;
		CLVColumn*				ColumnAt(BPoint point) const;
		// Returns column located at point on screen --added by jaf
			CLVColumn*				ColumnAt(int32 columnIndex) const;
	virtual	bool					SetDisplayOrder(const int32* order);
		// Sets the display order: each int32 in the Order list specifies
		// the column index of the next column to display.  Note that this
		// DOES NOT get called if the user drags a column, so overriding
		// it will not inform you of user changes. If you need that info,
		// override DisplayOrderChanged instead. Also note that
		// SetDisplayOrder does call DisplayOrderChanged(false).
	virtual	void					ColumnWidthChanged(int32 columnIndex,
										float newWidth);
	virtual	void					DisplayOrderChanged(const int32* order);
		// Override this if you want to find out when the display order changed.
			int32*					DisplayOrder() const;
		// Gets the display order in the same format as that used by
		// SetDisplayOrder. The returned array belongs to the caller and
		// must be delete[]'d when done with it.
	virtual	void					SetSortKey(int32 columnIndex);
		// Set it to -1 to remove the sort key.
	virtual	void					AddSortKey(int32 columnIndex);
			void					ReverseSortMode(int32 columnIndex);
	virtual	void					SetSortMode(int32 columnIndex,
										CLVSortMode Mode);
			int32					Sorting(int32* SortKeys,
										CLVSortMode* SortModes) const;
		// Returns the number of used sort keys, and fills the provided
		// arrays with the sort keys by column index and sort modes,
		// in priority order.  The pointers should point to an array
		// int32 SortKeys[n], and an array CLVSortMode SortModes[n] where
		// n is the number of sortable columns in the ColumnListView.
		// Note: sorting will only occur if the key column is shown.
			void					SetSorting(int32 NumberOfKeys,
										int32* SortKeys,
										CLVSortMode* SortModes);
		// Sets the sorting parameters using the same format returned by sorting

		// BView overrides
	virtual	void					FrameResized(float newWidth,
										float newHeight);
	virtual	void					AttachedToWindow();
	virtual	void					ScrollTo(BPoint point);
	virtual	void					MouseDown(BPoint point);

		// List functions
	virtual	bool					AddUnder(CLVListItem*,
										CLVListItem* superitem);
	virtual	bool					AddItem(CLVListItem*,
										int32 fullListIndex);
	virtual	bool					AddItem(CLVListItem*);
	virtual	bool					AddList(BList* newItems);
		// This must be a BList of CLVListItem*'s, NOT BListItem*'s
	virtual	bool					AddList(BList* newItems,
										int32 fullListIndex);
		//This must be a BList of CLVListItem*'s, NOT BListItem*'s
	virtual	bool					AddItem(BListItem*, int32 fullListIndex);
		// unhide
	virtual	bool					AddItem(BListItem*);
		// unhide
	virtual	bool					RemoveItem(CLVListItem* item);
	virtual	BListItem*				RemoveItem(int32 fullListIndex);
		// actually returns CLVListItem
	virtual	bool					RemoveItems(int32 fullListIndex,
										int32 count);
	virtual	bool					RemoveItem(BListItem* item);
		// unhide
	virtual	void					MakeEmpty();
			CLVListItem*			FullListItemAt(int32 fullListIndex) const;
			int32					FullListIndexOf(const CLVListItem* item)
										const;
			int32					FullListIndexOf(BPoint point) const;
			CLVListItem*			FullListFirstItem() const;
			CLVListItem*			FullListLastItem() const;
			bool					FullListHasItem(const CLVListItem* item)
										const;
			int32					FullListCountItems() const;
			bool					FullListIsEmpty() const;
			int32					FullListCurrentSelection(int32 index = 0)
										const;
			void					FullListDoForEach(
										bool (*func)(CLVListItem*));
			void					FullListDoForEach(bool (*func)(
										CLVListItem*, void*), void* arg2);
			CLVListItem*			Superitem(const CLVListItem* item) const;
			int32					FullListNumberOfSubitems(
										const CLVListItem* item) const;
	virtual	void					Expand(CLVListItem* item);
	virtual	void					Collapse(CLVListItem* item);
			bool					IsExpanded(int32 fullListIndex) const;
			void					SetSortFunction(CLVCompareFuncPtr compare);
			void					SortItems();

	virtual	void					KeyDown(const char* bytes, int32 numBytes);

			void					SetEditMessage(BMessage* message,
										BMessenger target);
		// Sets a BMessage that will be sent every time a key is pressed,
		// or the mouse is clicked in the active cell. (message) becomes
		// property of this ColumnListView.
        // Copies of (message) will be sent to (target).

	virtual	void					Pulse();
        // Used to make the cursor blink on the string column...

			int32					GetSelectedColumn() const
										{ return fSelectedColumn; }

	private:
		friend class CLVMainView;
		friend class CLVColumn;
		friend class CLVColumnLabelView;
		friend class CLVListItem;

			int32					GetActualIndexOf(int32 displayIndex) const;
		// Returns the "real" index of the given display index,
		// or -1 if there is none.

			int32					GetDisplayIndexOf(int32 actualIndex) const;
		// Returns the display index of the given "real" index,
		// or -1 if there is none.

			void					SetSelectedColumnIndex(
										int32 selectedcolumnIndex);
        // Call this to change fSelectedColumn to a new value properly.

			void					ShiftDragGroup();
			void					UpdateScrollBars();
			void					ColumnsChanged();
			void					CreateContainer(bool horizontal,
										bool vertical, border_style border,
										uint32 resizingMode, uint32 flags);
			void					SortListArray(CLVListItem** sortArray,
										int32 itemCount);
			void					MakeEmptyPrivate();
			bool					AddListPrivate(BList* newItems,
										int32 fullListIndex);
			bool					AddItemPrivate(CLVListItem* item,
										int32 fullListIndex);
			void					SortFullListSegment(
										int32 originalListStartIndex,
										int32 insertionPoint, BList* newList);
			BList*					SortItemCount(
										int32 originalListStartIndex);
	static	int						PlainBListSortFunc(BListItem** firstItem,
										BListItem** secondItem);
	static	int						HierarchicalBListSortFunc(
										BListItem** firstItem,
										BListItem** secondItem);

			CLVColumnLabelView*		fColumnLabelView;
			CLVContainerView*		fScrollView;
			ScrollViewCorner*		fFillerView;
			bool					fHierarchical;
			BList					fColumnList;
			BList					fColumnDisplayList;
			float					fDataWidth;
			float					fDataHeight;
			float					fPageWidth;
			float					fPageHeight;
			BList					fSortKeyList;
		// list contains CLVColumn pointers
			PrefilledBitmap			fRightArrow;
			PrefilledBitmap			fDownArrow;
			BList					fFullItemList;
			int32					fExpanderColumn;
			CLVCompareFuncPtr		fCompareFunction;
		// added by jaf
			int32					fSelectedColumn;
		// actual index of the column that contains the active cell.
			BMessage*				fEditMessage;
		// if non-NULL, sent on keypress or when active cell is clicked.
			BMessenger				fEditTarget;
		// target for fEditMessage.
};


#endif	// COLUMN_LIST_VIEW_H
