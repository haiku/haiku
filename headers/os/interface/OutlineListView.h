/*******************************************************************************
/
/	File:			OutlineListView.h
/
/   Description:    BOutlineListView represents a "nestable" list view. 
/
/	Copyright 1997-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef _OUTLINE_LIST_VIEW_H
#define _OUTLINE_LIST_VIEW_H

#include <BeBuild.h>
#include <ListView.h>

class BListItem;

/*----------------------------------------------------------------*/
/*----- BOutlineListView class -----------------------------------*/

class BOutlineListView : public BListView {
public:
					BOutlineListView(BRect frame,
							const char * name,
							list_view_type type = B_SINGLE_SELECTION_LIST,
							uint32 resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
							uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS 
								| B_NAVIGABLE);
					BOutlineListView(BMessage *data);
virtual 			~BOutlineListView();

static	BArchivable *Instantiate(BMessage *data);
virtual	status_t	Archive(BMessage *data, bool deep = true) const;

virtual	void 		MouseDown(BPoint where);
virtual	void 		KeyDown(const char *bytes, int32 numBytes);
virtual	void		FrameMoved(BPoint new_position);
virtual	void		FrameResized(float new_width, float new_height);
virtual	void 		MouseUp(BPoint where);
	
virtual bool 		AddUnder(BListItem *item, BListItem *underItem);

virtual bool 		AddItem(BListItem *item);
virtual bool		AddItem(BListItem *item, int32 fullListIndex);
virtual bool		AddList(BList *newItems);
virtual bool		AddList(BList *newItems, int32 fullListIndex);

virtual bool		RemoveItem(BListItem *item);
virtual BListItem	*RemoveItem(int32 fullListIndex);
virtual bool		RemoveItems(int32 fullListIndex, int32 count);


/* The following calls operator on the full outlinelist */
		BListItem	*FullListItemAt(int32 fullListIndex) const;
		int32		FullListIndexOf(BPoint point) const;
		int32		FullListIndexOf(BListItem *item) const;
		BListItem	*FullListFirstItem() const;
		BListItem	*FullListLastItem() const;
		bool		FullListHasItem(BListItem *item) const;
		int32		FullListCountItems() const;
		int32		FullListCurrentSelection(int32 index = 0) const;
virtual	void		MakeEmpty();
		bool		FullListIsEmpty() const;
		void		FullListDoForEach(bool (*func)(BListItem *));
		void		FullListDoForEach(bool (*func)(BListItem *, void *), void*);

		BListItem	*Superitem(const BListItem *item);

		void 		Expand(BListItem *item);
		void 		Collapse(BListItem *item);
		
		bool		IsExpanded(int32 fullListIndex);

virtual BHandler	*ResolveSpecifier(BMessage *msg,
										int32 index,
										BMessage *specifier,
										int32 form,
										const char *property);
virtual status_t	GetSupportedSuites(BMessage *data);
virtual status_t	Perform(perform_code d, void *arg);

virtual void		ResizeToPreferred();
virtual void		GetPreferredSize(float *width, float *height);
virtual void		MakeFocus(bool state = true);
virtual void		AllAttached();
virtual void		AllDetached();
virtual void		DetachedFromWindow();



		void		FullListSortItems(int (*compareFunc)(const BListItem *,
									const BListItem *));
		void		SortItemsUnder(BListItem *underItem,
									bool oneLevelOnly,
									int (*compareFunc)(const BListItem *,
														const BListItem*));
		int32		CountItemsUnder(BListItem *under, bool oneLevelOnly) const;
		BListItem 	*EachItemUnder(BListItem *underItem,
									bool oneLevelOnly,
									BListItem *(*eachFunc)(BListItem *, void *),
									void *);
		BListItem 	*ItemUnderAt(BListItem *underItem,
								bool oneLevelOnly,
								int32 index) const;
		
protected:

virtual	bool		DoMiscellaneous(MiscCode code, MiscData * data);
virtual void		MessageReceived(BMessage *);

/*----- Private or reserved -----------------------------------------*/
private:
virtual	void		_ReservedOutlineListView1();
virtual	void		_ReservedOutlineListView2();
virtual	void		_ReservedOutlineListView3();
virtual	void		_ReservedOutlineListView4();

typedef BListView	_inherited;

		int32		FullListIndex(int32 index) const;
		int32		ListViewIndex(int32 index) const;

#if !_PR3_COMPATIBLE_
protected:
#endif
virtual	void 		ExpandOrCollapse(BListItem *underItem, bool expand);

private:

virtual BRect		LatchRect(BRect itemRect, int32 level) const;
virtual void		DrawLatch(BRect itemRect, int32 level, bool collapsed, 
						bool highlighted, bool misTracked);
virtual	void		DrawItem(BListItem *i, BRect cRect, bool complete = false);

		BListItem	*RemoveCommon(int32 fullListIndex);
		BListItem	*RemoveOne(int32 fullListIndex);

static	void 		TrackInLatchItem(void *);
static	void 		TrackOutLatchItem(void *);

		bool		OutlineSwapItems(int32 a, int32 b);
		bool		OutlineMoveItem(int32 from, int32 to);
		bool		OutlineReplaceItem(int32 index, BListItem *item);
		void		CommonMoveItems(int32 from, int32 count, int32 to);
		BListItem	*SuperitemForIndex(int32 fullListIndex, int32 level);
		int32		FindPreviousVisibleIndex(int32 fullListIndex);

		BList		fullList;
		uint32		_reserved[2];
};

/*----------------------------------------------------------------*/
/*----------------------------------------------------------------*/

#endif /* _OUTLINE_LIST_VIEW_H */
