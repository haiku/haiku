/*
 * Copyright 1999-2009 Jeremy Friesner
 * Copyright 2009-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 *		John Scipione, jscipione@gmail.com
 */
#ifndef CLV_COLUMN_LABEL_VIEW_H
#define CLV_COLUMN_LABEL_VIEW_H


#include <support/SupportDefs.h>
#include <InterfaceKit.h>


class ColumnListView;
class CLVColumn;


struct CLVDragGroup
{
	int32 groupStartDisplayListIndex;
		// indices in the column display list where this group starts
	int32 groupStopDisplayListIndex;
		// and finishes
	float groupBeginIndex;
		// -1.0 if whole group is hidden
	float groupEndIndex;
		// -1.0 if whole group is hidden
	CLVColumn* lastColumnShown;
	bool isAllLockBeginning;
	bool isAllLockEnd;
	bool isShown;
		// false if none of the columns in this group are shown
	uint32 flags;
		// Uses CLV_NOT_MOVABLE, CLV_LOCK_AT_BEGINNING, CLV_LOCK_AT_END
};


class CLVColumnLabelView : public BView
{
public:
							CLVColumnLabelView(BRect frame,
								ColumnListView* parent,
								const BFont* font);
							~CLVColumnLabelView();

		// BView overrides
			void			Draw(BRect UpdateRect);
			void			MouseDown(BPoint Point);
			void			MessageReceived(BMessage* message);

private:
		friend class ColumnListView;
		friend class CLVColumn;

			void			ShiftDragGroup(int32 newPosition);
			void			UpdateDragGroups();
			void			SetSnapMinMax();

			float			fFontAscent;
			BList*			fDisplayList;

		// column select and drag stuff
			CLVColumn*		fColumnClicked;
			BPoint			fPreviousMousePosition;
			BPoint			fMouseClickedPosition;
			bool			fColumnDragging;
			bool			fColumnResizing;
			BList			fDragGroupsList;
		// groups of CLVColumns that must drag together
			int32			fDragGroupIndex;
		// index into DragGroups of the group being dragged by user
			CLVDragGroup*	fDragGroup;
			CLVDragGroup*	fShownGroupBefore;
			CLVDragGroup*	fShownGroupAfter;
			int32			fSnapGroupBeforeIndex;
			int32			fSnapGroupAfterIndex;
		// index into DragGroups of fShownGroupBefore and
		// fShownGroupAfter, if the group the user is dragging is
		// allowed to snap there, otherwise -1
			float			fDragBoxMouseHoldOffset;
			float			fResizeMouseHoldOffset;
			float			fDragBoxWidth;
		// can include multiple columns; depends on CLV_LOCK_WITH_RIGHT
			float			fPreviousDragOutlineLeft;
			float			fPreviousDragOutlineRight;
			float			fSnapMin;
			float			fSnapMax;
		// -1.0 indicates the column can't snap in the given direction
			ColumnListView*	fParent;
};


#endif	// CLV_COLUMN_LABEL_VIEW_H
