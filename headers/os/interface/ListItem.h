/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LIST_ITEM_H
#define _LIST_ITEM_H


#include <Archivable.h>
#include <Rect.h>

class BFont;
class BMessage;
class BOutlineListView;
class BView;


class BListItem : public BArchivable {
	public:
							BListItem(uint32 outlineLevel = 0, bool expanded = true);
							BListItem(BMessage* archive);
		virtual				~BListItem();

		virtual	status_t	Archive(BMessage* archive, bool deep = true) const;

		float				Height() const;
		float				Width() const;
		bool				IsSelected() const;
		void				Select();
		void				Deselect();

		virtual	void		SetEnabled(bool enabled);
		bool				IsEnabled() const;

		void				SetHeight(float height);
		void				SetWidth(float width);
		virtual	void		DrawItem(BView* owner, BRect frame,
								bool complete = false) = 0;
		virtual	void		Update(BView* owner, const BFont* font);

		virtual status_t	Perform(perform_code code, void* arg);

		bool				IsExpanded() const;
		void				SetExpanded(bool expanded);
		uint32				OutlineLevel() const;

	private:
		friend class BOutlineListView;

		bool				HasSubitems() const;

		virtual	void		_ReservedListItem1();
		virtual	void		_ReservedListItem2();

							BListItem(const BListItem& item);
		BListItem&			operator=(const BListItem& item);

		bool				IsItemVisible() const;
		void				SetItemVisible(bool visible);

	private:
		uint32				_reserved[1];
		BList*				fTemporaryList;
		float				fWidth;
		float				fHeight;
		uint32				fLevel;
		bool				fSelected;
		bool				fEnabled;
		bool				fExpanded;
		bool				fHasSubitems : 1;
		bool				fVisible : 1;
};

#include <StringItem.h>
	// to maintain source compatibility

#endif	// _LIST_ITEM_H
