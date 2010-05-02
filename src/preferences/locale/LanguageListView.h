/*
 * Copyright 2006-2010, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Adrien Destugues <pulkomandy@gmail.com>
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */
#ifndef LANGUAGE_LIST_VIEW_H
#define LANGUAGE_LIST_VIEW_H


#include <OutlineListView.h>
#include <StringItem.h>
#include <String.h>


class LanguageListItem : public BStringItem {
public:
							LanguageListItem(const char* text,
								const char* code);
	virtual					~LanguageListItem();

			const BString&	LanguageCode() { return fLanguageCode; }
			void			DrawItem(BView* owner, BRect frame,
								bool complete = false);

private:
			BString			fLanguageCode;
			BBitmap*		fIcon;
};


class LanguageListView : public BOutlineListView {
public:
							LanguageListView(const char* name,
								list_view_type type);
	virtual					~LanguageListView();

			bool 			InitiateDrag(BPoint point, int32 index,
								bool wasSelected);
			void 			MouseMoved(BPoint where, uint32 transit,
								const BMessage* msg);
			void 			MoveItems(BList& items, int32 index);
			void 			MoveItemFrom(BOutlineListView* origin, int32 index,
								int32 dropSpot = 0);
			void 			AttachedToWindow();
			void 			MessageReceived (BMessage* message);
private:
			int32			fDropIndex;
			BMessage*		fMsgPrefLanguagesChanged;
};


#endif	// LANGUAGE_LIST_VIEW_H
