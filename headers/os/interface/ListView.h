/*
 * Copyright 2002-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LIST_VIEW_H
#define _LIST_VIEW_H


#include <Invoker.h>
#include <View.h>
#include <List.h>
#include <ListItem.h>


struct track_data;

enum list_view_type {
	B_SINGLE_SELECTION_LIST,
	B_MULTIPLE_SELECTION_LIST
};


class BListView : public BView, public BInvoker {
	public:
		BListView(BRect frame, const char* name,
			list_view_type type = B_SINGLE_SELECTION_LIST,
			uint32 resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
			uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE);
		BListView(BMessage* data);

		virtual ~BListView() ;

		static	BArchivable* Instantiate(BMessage* data);
		virtual status_t	Archive(BMessage* data, bool deep = true) const;
		virtual void		Draw(BRect updateRect);
		virtual void		MessageReceived(BMessage* msg);
		virtual void		MouseDown(BPoint where);
		virtual void		KeyDown(const char* bytes, int32 numBytes);
		virtual void		MakeFocus(bool state = true);
		virtual void		FrameResized(float newWidth, float newHeight);
		virtual void		TargetedByScrollView(BScrollView* scroller);
				void		ScrollTo(float x, float y);
		virtual void		ScrollTo(BPoint where);
		virtual bool		AddItem(BListItem* item);
		virtual bool		AddItem(BListItem* item, int32 atIndex);
		virtual bool		AddList(BList* newItems);
		virtual bool		AddList(BList* newItems, int32 atIndex);
		virtual bool		RemoveItem(BListItem* item);
		virtual BListItem*	RemoveItem(int32 index);
		virtual bool		RemoveItems(int32 index, int32 count);

		virtual void		SetSelectionMessage(BMessage* message);
		virtual void		SetInvocationMessage(BMessage* message);

				BMessage*	SelectionMessage() const;
				uint32		SelectionCommand() const;
				BMessage*	InvocationMessage() const;
				uint32		InvocationCommand() const;

		virtual void		SetListType(list_view_type type);
				list_view_type ListType() const;

				BListItem*	ItemAt(int32 index) const;
				int32		IndexOf(BPoint point) const;
				int32		IndexOf(BListItem* item) const;
				BListItem*	FirstItem() const;
				BListItem*	LastItem() const;
				bool		HasItem(BListItem* item) const;
				int32		CountItems() const;
		virtual void		MakeEmpty();
				bool		IsEmpty() const;
				void		DoForEach(bool (*func)(BListItem* item));
				void		DoForEach(bool (*func)(BListItem* item, void* arg), void* arg);
		const	BListItem**	Items() const;
				void		InvalidateItem(int32 index);
				void		ScrollToSelection();

				void		Select(int32 index, bool extend = false);
				void		Select(int32 from, int32 to, bool extend = false);
				bool		IsItemSelected(int32 index) const;
				int32		CurrentSelection(int32 index = 0) const;
		virtual status_t	Invoke(BMessage* message = NULL);

				void		DeselectAll();
				void		DeselectExcept(int32 exceptFrom, int32 exceptTo);
				void		Deselect(int32 index);

		virtual void		SelectionChanged();

				void		SortItems(int (*cmp)(const void*, const void*));

		/* These functions bottleneck through DoMiscellaneous() */
				bool		SwapItems(int32 a, int32 b);
				bool		MoveItem(int32 from, int32 to);
				bool		ReplaceItem(int32 index, BListItem* item);

		virtual void		AttachedToWindow();
		virtual void		FrameMoved(BPoint newPosition);

				BRect		ItemFrame(int32 index);

		virtual BHandler*	ResolveSpecifier(BMessage* message, int32 index,
								BMessage* specifier, int32 form, const char* property);
		virtual status_t	GetSupportedSuites(BMessage* data);

		virtual status_t	Perform(perform_code code, void* arg);

		virtual void		WindowActivated(bool state);
		virtual void		MouseUp(BPoint point);
		virtual void		MouseMoved(BPoint point, uint32 code,
								const BMessage *dragMessage);
		virtual void		DetachedFromWindow();
		virtual bool		InitiateDrag(BPoint point, int32 itemIndex,
								bool initialySelected);

		virtual void		ResizeToPreferred();
		virtual void		GetPreferredSize(float* _width, float* _height);
		virtual void		AllAttached();
		virtual void		AllDetached();

		virtual	BSize		MinSize();
		virtual	BSize		PreferredSize();

	protected:
		enum MiscCode { B_NO_OP, B_REPLACE_OP, B_MOVE_OP, B_SWAP_OP };
		union MiscData {
			struct Spare { int32 data[5]; };
			struct Replace { int32 index; BListItem *item; } replace;
			struct Move { int32 from; int32 to; } move;
			struct Swap { int32 a; int32 b; } swap;
		};

		virtual bool		DoMiscellaneous(MiscCode code, MiscData *data);

	private:
		friend class BOutlineListView;

		virtual	void		_ReservedListView2();
		virtual	void		_ReservedListView3();
		virtual	void		_ReservedListView4();

				BListView&	operator=(const BListView&);

				void		_InitObject(list_view_type type);
				void		_FixupScrollBar();
				void		_InvalidateFrom(int32 index);
				status_t	_PostMessage(BMessage* message);
				void		_FontChanged();
				int32		_RangeCheck(int32 index);
				bool		_Select(int32 index, bool extend);
				bool		_Select(int32 from, int32 to, bool extend);
				bool		_Deselect(int32 index);
//				void		_Deselect(int32 from, int32 to);
				bool		_DeselectAll(int32 exceptFrom, int32 exceptTo);
//				void		PerformDelayedSelect();
				bool		_TryInitiateDrag(BPoint where);
				int32		_CalcFirstSelected(int32 after);
				int32		_CalcLastSelected(int32 before);
		virtual void		DrawItem(BListItem* item, BRect itemRect,
								bool complete = false);
	
				bool		_SwapItems(int32 a, int32 b);
				bool		_MoveItem(int32 from, int32 to);
				bool		_ReplaceItem(int32 index, BListItem* item);
				void		_RescanSelection(int32 from, int32 to);

		BList				fList;
		list_view_type		fListType;
		int32				fFirstSelected;
		int32				fLastSelected;
		int32				fAnchorIndex;
		float				fWidth;
		BMessage*			fSelectMessage;
		BScrollView*		fScrollView;
		track_data*			fTrack;

		uint32				_reserved[3];
};

inline void
BListView::ScrollTo(float x, float y)
{
	ScrollTo(BPoint(x, y));
}

#endif /* _LIST_VIEW_H */
