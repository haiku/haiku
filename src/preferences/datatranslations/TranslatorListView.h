/*
 * Copyright 2002-2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Oliver Siebenmarck
 *		Andrew McCall, mccall@digitalparadise.co.uk
 *		Michael Wilber
 */
#ifndef TRANSLATOR_LIST_VIEW_H
#define TRANSLATOR_LIST_VIEW_H


#include <ListView.h>
#include <String.h>
#include <TranslationDefs.h>


class TranslatorItem : public BStringItem {
public:
							TranslatorItem(translator_id id, const char* name);
	virtual					~TranslatorItem();

			translator_id	ID() const { return fID; }
			const BString&	Supertype() const { return fSupertype; }

private:
			translator_id	fID;
			BString			fSupertype;
};


class TranslatorListView : public BListView {
public:
							TranslatorListView(const char* name,
								list_view_type type = B_SINGLE_SELECTION_LIST);
	virtual					~TranslatorListView();

			TranslatorItem*	TranslatorAt(int32 index) const;

	virtual	void			MessageReceived(BMessage* message);
	virtual	void			MouseMoved(BPoint point, uint32 transit, const BMessage* msg);

			void			SortItems();
};


#endif	// TRANSLATOR_LIST_VIEW_H
