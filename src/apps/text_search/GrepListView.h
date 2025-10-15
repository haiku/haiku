/*
 * Copyright (c) 1998-2007 Matthijs Hollemans
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef GREP_LIST_VIEW_H
#define GREP_LIST_VIEW_H

#include <Entry.h>
#include <ListItem.h>
#include <OutlineListView.h>
#include <PopUpMenu.h>


class ResultItem : public BStringItem {
public:
								ResultItem(const entry_ref& ref);

			entry_ref			ref;
};


class GrepListView : public BOutlineListView {
public:
								GrepListView();

	virtual void				AttachedToWindow();
	virtual	void				MouseDown(BPoint where);

			ResultItem*			FindItem(const entry_ref& ref,
									int32* _index) const;

			ResultItem*			RemoveResults(const entry_ref& ref,
									bool completeItem);

private:
			void				_CreatePopUpMenu();
			BPopUpMenu*			fContextMenu;
};


#endif // GREP_LIST_VIEW_H
