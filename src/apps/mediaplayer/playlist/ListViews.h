/*
 * Copyright 2006-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef LIST_VIEWS_H
#define LIST_VIEWS_H

#include <ListItem.h>
#include <ListView.h>
#include <Message.h>

enum {
	FLAGS_NONE			= 0x00,
	FLAGS_TINTED_LINE	= 0x01,
	FLAGS_FOCUSED		= 0x02,
};

// portion of the listviews height that triggers autoscrolling
// when the mouse is over it with a dragmessage
#define SCROLL_AREA			0.1

class BMessageRunner;
class BMessageFilter;
class InterfaceWindow;
class BScrollView;

// SimpleItem
class SimpleItem : public BStringItem {
 public:
							SimpleItem(const char* name);
		virtual				~SimpleItem();

		virtual	void		Draw(BView* owner, BRect frame,
								 uint32 flags);
		virtual	void		DrawBackground(BView* owner, BRect frame,
								  uint32 flags);
};

// DragSortableListView
class DragSortableListView : public BListView {
 public:
							DragSortableListView(BRect frame, const char* name,
								list_view_type type = B_SINGLE_SELECTION_LIST,
								uint32 resizingMode = B_FOLLOW_LEFT
									| B_FOLLOW_TOP,
								uint32 flags = B_WILL_DRAW | B_NAVIGABLE
									| B_FRAME_EVENTS);
	virtual					~DragSortableListView();

	// BListView interface
	virtual	void			AttachedToWindow();
	virtual	void			DetachedFromWindow();
	virtual	void			FrameResized(float width, float height);
	virtual	void			Draw(BRect updateRect);
	virtual	void			ScrollTo(BPoint where);
	virtual	void			TargetedByScrollView(BScrollView* scrollView);
	virtual	bool			InitiateDrag(BPoint point, int32 index,
										 bool wasSelected);
	virtual void			MessageReceived(BMessage* message);
	virtual	void			KeyDown(const char* bytes, int32 numBytes);
	virtual	void			MouseDown(BPoint where);
	virtual void			MouseMoved(BPoint where, uint32 transit,
									   const BMessage* dragMessage);
	virtual void			MouseUp(BPoint where);
	virtual	void			WindowActivated(bool active);
	virtual void			DrawItem(BListItem *item, BRect itemFrame,
									 bool complete = false);

	// DragSortableListView
	virtual	void			SetDragCommand(uint32 command);
	virtual	void			ModifiersChanged();	// called by window
	virtual	void			DoubleClicked(int32 index) {}

	virtual	void			SetItemFocused(int32 index);

	virtual	bool			AcceptDragMessage(const BMessage* message) const;
	virtual	void			SetDropTargetRect(const BMessage* message,
											  BPoint where);

	// autoscrolling
			void			SetAutoScrolling(bool enable);
			bool			DoesAutoScrolling() const;
			BScrollView*	ScrollView() const
								{ return fScrollView; }
			void			ScrollTo(int32 index);

			bool			MouseWheelChanged(float x, float y);

	virtual	void			MoveItems(const BList& indices, int32 toIndex);
	virtual	void			CopyItems(const BList& indices, int32 toIndex);
	virtual	void			RemoveItemList(const BList& indices);

			void			GetSelectedItems(BList& indices);
			void			RemoveSelected(); // uses RemoveItemList()
			void			RemoveAll(); // uses RemoveItemList()
			int32			CountSelectedItems() const;
			void			SelectAll();
	virtual	bool			DeleteItem(int32 index);

	virtual	BListItem*		CloneItem(int32 atIndex) const = 0;
	virtual	void			DrawListItem(BView* owner, int32 index,
										 BRect itemFrame) const = 0;
	virtual	void			MakeDragMessage(BMessage* message) const = 0;

 private:
			void			_RemoveDropAnticipationRect();
			void			_SetDragMessage(const BMessage* message);

	BRect					fDropRect;
	BMessage				fDragMessageCopy;
	BMessageRunner*			fScrollPulse;
	BPoint					fLastMousePos;

 protected:
			void			_SetDropAnticipationRect(BRect r);
			void			_SetDropIndex(int32 index);

	int32					fDropIndex;
	BListItem*				fLastClickedItem;
	BScrollView*			fScrollView;
	uint32					fDragCommand;
	int32					fFocusedIndex;
};

// SimpleListView
class SimpleListView : public DragSortableListView {
 public:
							SimpleListView(BRect frame,
								BMessage* selectionChangeMessage = NULL);
							SimpleListView(BRect frame, const char* name,
								BMessage* selectionChangeMessage = NULL,
								list_view_type type = B_MULTIPLE_SELECTION_LIST,
								uint32 resizingMode = B_FOLLOW_ALL_SIDES,
								uint32 flags = B_WILL_DRAW | B_NAVIGABLE
									| B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE);
							~SimpleListView();

	// DragSortableListView interface
	virtual void			MessageReceived(BMessage* message);
	virtual	void			SelectionChanged();

	virtual	BListItem*		CloneItem(int32 atIndex) const;
	virtual	void			DrawListItem(BView* owner, int32 index,
										 BRect itemFrame) const;
	virtual	void			MakeDragMessage(BMessage* message) const;

 private:

	BMessage*				fSelectionChangeMessage;
};

#endif // LIST_VIEWS_H
