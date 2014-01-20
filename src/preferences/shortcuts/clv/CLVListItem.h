/*
 * Copyright 1999-2009 Jeremy Friesner
 * Copyright 2009-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 *		John Scipione, jscipione@gmail.com
 */
#ifndef CLV_LIST_ITEM_H
#define CLV_LIST_ITEM_H


#include <interface/ListItem.h>


class ColumnListView;


class CLVListItem : public BListItem
{
public:
									CLVListItem(uint32 level = 0,
										bool superitem = false,
										bool expanded = false,
										float minheight = 0.0);
	virtual							~CLVListItem();

		//Archival stuff
		/* Not implemented yet
									CLVItem(BMessage* archive);
	static	CLVItem*				Instantiate(BMessage* data);
	virtual	status_t				Archive(BMessage* data, bool deep = true)
										const;
		*/

	virtual	void					DrawItemColumn(BView* owner,
										BRect columnRect,
										int32 columnIndex,
										bool columnSelected,
										bool complete) = 0;
		// columnIndex (0-N) is based on the order in which the columns
		// were added to the ColumnListView, not the display order.
		// An index of -1 indicates that the program needs to draw a blank
		// area beyond the last column. The main purpose is to allow the
		// highlighting bar to continue all the way to the end of the'
		// ColumnListView, even after the end of the last column.

	virtual	void					DrawItem(BView* owner, BRect itemRect,
										bool complete);
		// In general, you don't need or want to override DrawItem().
			float					ExpanderShift(int32 columnIndex,
										BView* owner);
	virtual	void					Update(BView* owner, const BFont* font);
			bool					IsSuperItem() const;
			void					SetSuperItem(bool superitem);
			uint32					OutlineLevel() const;
			void					SetOutlineLevel(uint32 level);

	virtual	void					Pulse(BView* owner);
		// Called periodically when this item is selected.

			int32					GetSelectedColumn() const
										{ return fSelectedColumn; }
			void					SetSelectedColumn(int32 i)
										{ fSelectedColumn = i; }

	private:
		friend class ColumnListView;

			bool					fIsSuperItem;
			uint32					fOutlineLevel;
			float					fMinHeight;
			BRect					fExpanderButtonRect;
			BRect					fExpanderColumnRect;
			BList*					fSortingContextBList;
			ColumnListView*			fSortingContextCLV;
			int32					fSelectedColumn;
};


#endif	// CLV_LIST_ITEM_H
