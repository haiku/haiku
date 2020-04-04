/*
 * Copyright 2002-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LIST_VIEW_H
#define _LIST_VIEW_H


#include <Invoker.h>
#include <List.h>
#include <ListItem.h>
#include <View.h>


struct track_data;


enum list_view_type {
	B_SINGLE_SELECTION_LIST,
	B_MULTIPLE_SELECTION_LIST
};


class BListView : public BView, public BInvoker {
public:
								BListView(BRect frame, const char* name,
									list_view_type type
										= B_SINGLE_SELECTION_LIST,
									uint32 resizeMask = B_FOLLOW_LEFT_TOP,
									uint32 flags = B_WILL_DRAW
										| B_FRAME_EVENTS | B_NAVIGABLE);
								BListView(const char* name,
									list_view_type type
										= B_SINGLE_SELECTION_LIST,
									uint32 flags = B_WILL_DRAW
										| B_FRAME_EVENTS | B_NAVIGABLE);
								BListView(list_view_type type
									= B_SINGLE_SELECTION_LIST);
								BListView(BMessage* data);

	virtual						~BListView();

	static	BArchivable*		Instantiate(BMessage* data);
	virtual status_t			Archive(BMessage* data,
									bool deep = true) const;

	virtual void				Draw(BRect updateRect);

	virtual void				AttachedToWindow();
	virtual void				DetachedFromWindow();
	virtual void				AllAttached();
	virtual void				AllDetached();
	virtual void				FrameResized(float newWidth, float newHeight);
	virtual void				FrameMoved(BPoint newPosition);
	virtual void				TargetedByScrollView(BScrollView* view);
	virtual void				WindowActivated(bool active);

	virtual void				MessageReceived(BMessage* message);
	virtual void				KeyDown(const char* bytes, int32 numBytes);
	virtual void				MouseDown(BPoint where);
	virtual void				MouseUp(BPoint where);
	virtual void				MouseMoved(BPoint where, uint32 code,
									const BMessage* dragMessage);

	virtual void				ResizeToPreferred();
	virtual void				GetPreferredSize(float *_width,
									float *_height);

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();

	virtual void				MakeFocus(bool state = true);

	virtual	void				SetFont(const BFont* font, uint32 mask
									= B_FONT_ALL);
	virtual void				ScrollTo(BPoint where);
	inline	void				ScrollTo(float x, float y);

	virtual bool				AddItem(BListItem* item);
	virtual bool				AddItem(BListItem* item, int32 atIndex);
	virtual bool				AddList(BList* newItems);
	virtual bool				AddList(BList* newItems, int32 atIndex);
	virtual bool				RemoveItem(BListItem* item);
	virtual BListItem*			RemoveItem(int32 index);
	virtual bool				RemoveItems(int32 index, int32 count);

	virtual void				SetSelectionMessage(BMessage* message);
	virtual void				SetInvocationMessage(BMessage* message);

			BMessage*			SelectionMessage() const;
			uint32				SelectionCommand() const;
			BMessage*			InvocationMessage() const;
			uint32				InvocationCommand() const;

	virtual void				SetListType(list_view_type type);
			list_view_type		ListType() const;

			BListItem*			ItemAt(int32 index) const;
			int32				IndexOf(BPoint point) const;
			int32				IndexOf(BListItem* item) const;
			BListItem*			FirstItem() const;
			BListItem*			LastItem() const;
			bool				HasItem(BListItem* item) const;
			int32				CountItems() const;
	virtual void				MakeEmpty();
			bool				IsEmpty() const;
			void				DoForEach(bool (*func)(BListItem* item));
			void				DoForEach(bool (*func)(BListItem* item,
									void* arg), void* arg);
			const BListItem**	Items() const;
			void				InvalidateItem(int32 index);
			void				ScrollToSelection();

			void				Select(int32 index, bool extend = false);
			void				Select(int32 from, int32 to,
									bool extend = false);
			bool				IsItemSelected(int32 index) const;
			int32				CurrentSelection(int32 index = 0) const;
	virtual status_t			Invoke(BMessage* message = NULL);

			void				DeselectAll();
			void				DeselectExcept(int32 exceptFrom,
									int32 exceptTo);
			void				Deselect(int32 index);

	virtual void				SelectionChanged();

	virtual bool				InitiateDrag(BPoint where, int32 index,
									bool wasSelected);

			void				SortItems(int (*cmp)(const void*,
									const void*));

	// These functions bottleneck through DoMiscellaneous()
			bool				SwapItems(int32 a, int32 b);
			bool				MoveItem(int32 from, int32 to);
			bool				ReplaceItem(int32 index, BListItem* item);

			BRect				ItemFrame(int32 index);

	virtual BHandler*			ResolveSpecifier(BMessage* message,
									int32 index, BMessage* specifier,
									int32 what, const char* property);
	virtual status_t			GetSupportedSuites(BMessage* data);

	virtual status_t			Perform(perform_code code, void* arg);

private:
	virtual	void				_ReservedListView2();
	virtual	void				_ReservedListView3();
	virtual	void				_ReservedListView4();

			BListView&			operator=(const BListView& other);

protected:
	enum MiscCode { B_NO_OP, B_REPLACE_OP, B_MOVE_OP, B_SWAP_OP };
	union MiscData {
		struct Spare { int32 data[5]; };
		struct Replace { int32 index; BListItem *item; } replace;
		struct Move { int32 from; int32 to; } move;
		struct Swap { int32 a; int32 b; } swap;
	};

	virtual bool				DoMiscellaneous(MiscCode code, MiscData* data);

private:
	friend class BOutlineListView;

			void				_InitObject(list_view_type type);
			void				_FixupScrollBar();
			void				_InvalidateFrom(int32 index);
			status_t			_PostMessage(BMessage* message);
			void				_UpdateItems();
			int32				_RangeCheck(int32 index);
			bool				_Select(int32 index, bool extend);
			bool				_Select(int32 from, int32 to, bool extend);
			bool				_Deselect(int32 index);
			bool				_DeselectAll(int32 exceptFrom, int32 exceptTo);
			int32				_CalcFirstSelected(int32 after);
			int32				_CalcLastSelected(int32 before);
			void				_RecalcItemTops(int32 start, int32 end = -1);

	virtual void				DrawItem(BListItem* item, BRect itemRect,
									bool complete = false);

			bool				_SwapItems(int32 a, int32 b);
			bool				_MoveItem(int32 from, int32 to);
			bool				_ReplaceItem(int32 index, BListItem* item);
			void				_RescanSelection(int32 from, int32 to);

private:
			BList				fList;
			list_view_type		fListType;
			int32				fFirstSelected;
			int32				fLastSelected;
			int32				fAnchorIndex;
			BMessage*			fSelectMessage;
			BScrollView*		fScrollView;
			track_data*			fTrack;

			uint32				_reserved[4];
};


inline void
BListView::ScrollTo(float x, float y)
{
	ScrollTo(BPoint(x, y));
}

#endif // _LIST_VIEW_H
