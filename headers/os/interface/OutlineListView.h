/*
 * Copyright 2006-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _OUTLINE_LIST_VIEW_H
#define _OUTLINE_LIST_VIEW_H


#include <ListView.h>

class BListItem;


class BOutlineListView : public BListView {
public:
								BOutlineListView(BRect frame, const char* name,
									list_view_type type
										= B_SINGLE_SELECTION_LIST,
									uint32 resizeMode
										= B_FOLLOW_LEFT | B_FOLLOW_TOP,
									uint32 flags = B_WILL_DRAW
										| B_FRAME_EVENTS | B_NAVIGABLE);
								BOutlineListView(const char* name,
									list_view_type type
										= B_SINGLE_SELECTION_LIST,
									uint32 flags = B_WILL_DRAW
										| B_FRAME_EVENTS | B_NAVIGABLE);
								BOutlineListView(BMessage* archive);
	virtual 					~BOutlineListView();

	static	BArchivable*		Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;

	virtual	void				MouseDown(BPoint where);
	virtual	void				KeyDown(const char* bytes, int32 numBytes);
	virtual	void				FrameMoved(BPoint newPosition);
	virtual	void				FrameResized(float newWidth, float newHeight);
	virtual	void				MouseUp(BPoint where);

	virtual bool				AddUnder(BListItem* item,
									BListItem* underItem);

	virtual bool				AddItem(BListItem* item);
	virtual bool				AddItem(BListItem* item, int32 fullListIndex);
	virtual bool				AddList(BList* newItems);
	virtual bool				AddList(BList* newItems, int32 fullListIndex);

	virtual bool				RemoveItem(BListItem* item);
	virtual BListItem*			RemoveItem(int32 fullListIndex);
	virtual bool				RemoveItems(int32 fullListIndex, int32 count);

			BListItem*			FullListItemAt(int32 fullListIndex) const;
			int32				FullListIndexOf(BPoint point) const;
			int32				FullListIndexOf(BListItem* item) const;
			BListItem*			FullListFirstItem() const;
			BListItem*			FullListLastItem() const;
			bool				FullListHasItem(BListItem* item) const;
			int32				FullListCountItems() const;
			int32				FullListCurrentSelection(
									int32 index = 0) const;

	virtual	void				MakeEmpty();
			bool				FullListIsEmpty() const;
			void				FullListDoForEach(
									bool (*func)(BListItem* item));
			void				FullListDoForEach(
									bool (*func)(BListItem* item, void*),
									void*);

			BListItem*			Superitem(const BListItem* item);

			void				Expand(BListItem* item);
			void				Collapse(BListItem* item);

			bool				IsExpanded(int32 fullListIndex);

	virtual BHandler*			ResolveSpecifier(BMessage* message,
									int32 index, BMessage* specifier,
									int32 what, const char* property);
	virtual status_t			GetSupportedSuites(BMessage* data);
	virtual status_t			Perform(perform_code code, void* data);

	virtual void				ResizeToPreferred();
	virtual void				GetPreferredSize(float* _width,
									float* _height);
	virtual void				MakeFocus(bool focus = true);
	virtual void				AllAttached();
	virtual void				AllDetached();
	virtual void				DetachedFromWindow();

			void				FullListSortItems(int (*compareFunc)(
										const BListItem* first,
										const BListItem* second));
			void				SortItemsUnder(BListItem* underItem,
									bool oneLevelOnly, int (*compareFunc)(
										const BListItem* first,
										const BListItem* second));
			int32				CountItemsUnder(BListItem* under,
									bool oneLevelOnly) const;
			BListItem*			EachItemUnder(BListItem* underItem,
									bool oneLevelOnly, BListItem* (*eachFunc)(
										BListItem* item, void* arg),
									void* arg);
			BListItem*			ItemUnderAt(BListItem* underItem,
									bool oneLevelOnly, int32 index) const;

protected:
	virtual	bool				DoMiscellaneous(MiscCode code, MiscData* data);
	virtual void				MessageReceived(BMessage* message);

private:
	virtual	void				_ReservedOutlineListView1();
	virtual	void				_ReservedOutlineListView2();
	virtual	void				_ReservedOutlineListView3();
	virtual	void				_ReservedOutlineListView4();

protected:
	virtual	void				ExpandOrCollapse(BListItem* underItem,
									bool expand);
	virtual BRect				LatchRect(BRect itemRect, int32 level) const;
	virtual void				DrawLatch(BRect itemRect, int32 level,
									bool collapsed, bool highlighted,
									bool misTracked);
	virtual	void				DrawItem(BListItem* item, BRect itemRect,
									bool complete = false);

private:
			int32				_FullListIndex(int32 index) const;

			void				_PopulateTree(BList* tree, BList& target,
									int32& firstIndex, bool onlyVisible);
			void				_SortTree(BList* tree, bool oneLevelOnly,
									int (*compareFunc)(const BListItem* a,
										const BListItem* b));
			void				_DestructTree(BList* tree);
			BList*				_BuildTree(BListItem* underItem, int32& index);

			void				_CullInvisibleItems(BList &list);
			bool				_SwapItems(int32 first, int32 second);
			BListItem*			_RemoveItem(BListItem* item,
									int32 fullListIndex);

			BListItem*			_SuperitemForIndex(int32 fullListIndex,
									int32 level, int32* _superIndex = NULL);
			int32				_FindPreviousVisibleIndex(int32 fullListIndex);

private:
			BList				fFullList;

			uint32				_reserved[2];
};

#endif // _OUTLINE_LIST_VIEW_H
