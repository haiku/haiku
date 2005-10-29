/*******************************************************************************
/
/	File:			ListItem.h
/
/   Description:    BListView represents a one-dimensional list view. 
/
/	Copyright 1996-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef _LIST_ITEM_H
#define _LIST_ITEM_H

#include <BeBuild.h>
#include <Archivable.h>
#include <Rect.h>

class BFont;
class BMessage;
class BOutlineListView;
class BView;

/*----------------------------------------------------------------*/
/*----- BListItem class ------------------------------------------*/

class BListItem : public BArchivable {
public:
					BListItem(uint32 outlineLevel = 0, bool expanded = true);
					BListItem(BMessage *data);
virtual				~BListItem();
virtual	status_t	Archive(BMessage *data, bool deep = true) const;

		float		Height() const;
		float		Width() const;
		bool		IsSelected() const;
		void		Select();
		void		Deselect();

virtual	void		SetEnabled(bool on);
		bool		IsEnabled() const;

		void		SetHeight(float height);
		void		SetWidth(float width);
virtual	void		DrawItem(BView *owner,
							BRect bounds,
							bool complete = false) = 0;
virtual	void		Update(BView *owner, const BFont *font);

virtual status_t	Perform(perform_code d, void *arg);

		bool 		IsExpanded() const;
		void 		SetExpanded(bool expanded);
		uint32 		OutlineLevel() const;

/*----- Private or reserved -----------------------------------------*/
private:
friend class BOutlineListView;

		bool 		HasSubitems() const;

virtual	void		_ReservedListItem1();
virtual	void		_ReservedListItem2();

					BListItem(const BListItem &);
		BListItem	&operator=(const BListItem &);

		/* calls used by BOutlineListView*/
		bool 		IsItemVisible() const;
		void 		SetItemVisible(bool);

		uint32		_reserved[2];
		float		fWidth;
		float		fHeight;
		uint32 		fLevel;
		bool		fSelected;
		bool		fEnabled;
		bool 		fExpanded;
		bool 		fHasSubitems : 1;
		bool 		fVisible : 1;
};


/*-------------------------------------------------------------*/

#include <StringItem.h> /* to maintain compatibility */

#endif /* _LIST_ITEM_H */
