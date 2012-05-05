/*
 * Copyright 2006, Haiku.
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

#ifdef LIB_LAYOUT
#  include <layout.h>
#endif

#include "MouseWheelFilter.h"
#include "Observer.h"

enum
{
	FLAGS_NONE			= 0x00,
	FLAGS_TINTED_LINE	= 0x01,
	FLAGS_FOCUSED		= 0x02,
};

// portion of the listviews height that triggers autoscrolling
// when the mouse is over it with a dragmessage
#define SCROLL_AREA			0.1

class BMessageFilter;
class BMessageRunner;
class BScrollView;
class Selectable;
class Selection;

// SimpleItem
class SimpleItem : public BStringItem
{
 public:
							SimpleItem(const char* name);
		virtual				~SimpleItem();

		virtual	void		Draw(BView* owner, BRect frame,
								 uint32 flags);
		virtual	void		DrawBackground(BView* owner, BRect frame,
								  uint32 flags);

							// let the item know what's going on
/*		virtual	void		AttachedToListView(SimpleListView* owner);
		virtual	void		DetachedFromListView(SimpleListView* owner);

		virtual	void		SetItemFrame(BRect frame);*/

 private:

};

// DragSortableListView
class DragSortableListView : public MouseWheelTarget,
							 public BListView,
							 public Observer {
 public:
							DragSortableListView(BRect frame,
												 const char* name,
												 list_view_type type
														= B_SINGLE_SELECTION_LIST,
												 uint32 resizingMode
														= B_FOLLOW_LEFT
														  | B_FOLLOW_TOP,
												 uint32 flags
														= B_WILL_DRAW
														  | B_NAVIGABLE
														  | B_FRAME_EVENTS);
	virtual					~DragSortableListView();

	// BListView interface
	virtual	void			AttachedToWindow();
	virtual	void			DetachedFromWindow();
	virtual	void			FrameResized(float width, float height);
//	virtual	void			MakeFocus(bool focused);
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

	// MouseWheelTarget interface
	virtual	bool			MouseWheelChanged(float x, float y);

	// Observer interface (watching Selection)
	virtual	void			ObjectChanged(const Observable* object);

	// DragSortableListView
	virtual	void			SetDragCommand(uint32 command);
	virtual	void			ModifiersChanged();	// called by window
	virtual	void			DoubleClicked(int32 index) {}

	virtual	void			SetItemFocused(int32 index);

	virtual	bool			AcceptDragMessage(const BMessage* message) const;
	virtual	bool			HandleDropMessage(const BMessage* message,
								int32 dropIndex);
	virtual	void			SetDropTargetRect(const BMessage* message,
								BPoint where);

							// autoscrolling
			void			SetAutoScrolling(bool enable);
			bool			DoesAutoScrolling() const;
			BScrollView*	ScrollView() const
								{ return fScrollView; }
			void			ScrollTo(int32 index);

	virtual	void			MoveItems(BList& items, int32 toIndex);
	virtual	void			CopyItems(BList& items, int32 toIndex);
	virtual	void			RemoveItemList(BList& indices);
			void			RemoveSelected(); // uses RemoveItemList()
	virtual	bool			DeleteItem(int32 index);

							// selection
			void			SetSelection(Selection* selection);

	virtual	int32			IndexOfSelectable(Selectable* selectable) const;
	virtual	Selectable*		SelectableFor(BListItem* item) const;

			void			SetDeselectAllInGlobalSelection(bool deselect);

			void			SelectAll();
			int32			CountSelectedItems() const;
	virtual	void			SelectionChanged();

	virtual	BListItem*		CloneItem(int32 atIndex) const = 0;
	virtual	void			DrawListItem(BView* owner, int32 index,
										 BRect itemFrame) const = 0;
	virtual	void			MakeDragMessage(BMessage* message) const = 0;

 private:
			void			_RemoveDropAnticipationRect();
			void			_SetDragMessage(const BMessage* message);

	BRect					fDropRect;
	BMessage				fDragMessageCopy;
	BMessageFilter*			fMouseWheelFilter;
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

	Selection*				fSelection;
	bool					fSyncingToSelection;
	bool					fModifyingSelection;
};

// SimpleListView
class SimpleListView : 
					   #ifdef LIB_LAYOUT
					   public MView,
					   #endif
					   public DragSortableListView {
 public:
							SimpleListView(BRect frame,
										   BMessage* selectionChangeMessage = NULL);
							SimpleListView(BRect frame,
										   const char* name,
										   BMessage* selectionChangeMessage = NULL,
										   list_view_type type
												= B_MULTIPLE_SELECTION_LIST,
										   uint32 resizingMode
												= B_FOLLOW_ALL_SIDES,
										   uint32 flags
												= B_WILL_DRAW | B_NAVIGABLE
												  | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE);
							~SimpleListView();

	#ifdef LIB_LAYOUT
							// MView
	virtual	minimax			layoutprefs();
	virtual	BRect			layout(BRect frame);
	#endif
							// BListView
	virtual	void			DetachedFromWindow();
	virtual void			MessageReceived(BMessage* message);

	virtual	BListItem*		CloneItem(int32 atIndex) const;
	virtual	void			DrawListItem(BView* owner, int32 index,
										 BRect itemFrame) const;
	virtual	void			MakeDragMessage(BMessage* message) const;

 protected:
			void			_MakeEmpty();

 private:

	BMessage*				fSelectionChangeMessage;
};

#endif // LIST_VIEWS_H
