/*
 * Copyright 2006-2010, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Adrien Destugues <pulkomandy@gmail.com>
 *		Axel Dörfler, axeld@pinc-software.de
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
									const char* id, const char* languageCode);
								LanguageListItem(
									const LanguageListItem& other);

			const BString&		ID() const { return fID; }
			const BString&		Code() const { return fCode; }

	virtual	void				DrawItem(BView* owner, BRect frame,
									bool complete = false);

protected:
			void				DrawItemWithTextOffset(BView* owner,
									BRect frame, bool complete,
									float textOffset);

private:
			BString				fID;
			BString				fCode;
};


class LanguageListItemWithFlag : public LanguageListItem {
public:
								LanguageListItemWithFlag(const char* text,
									const char* id, const char* languageCode,
									const char* countryCode = NULL);
								LanguageListItemWithFlag(
									const LanguageListItemWithFlag& other);
	virtual						~LanguageListItemWithFlag();

	virtual void				Update(BView* owner, const BFont* font);

	virtual	void				DrawItem(BView* owner, BRect frame,
									bool complete = false);

private:
			BString				fCountryCode;
			BBitmap*			fIcon;
};


class LanguageListView : public BOutlineListView {
public:
								LanguageListView(const char* name,
									list_view_type type);
	virtual						~LanguageListView();

			LanguageListItem*	ItemForLanguageID(const char* code,
									int32* _index = NULL) const;
			LanguageListItem*	ItemForLanguageCode(const char* code,
									int32* _index = NULL) const;

			void				SetDeleteMessage(BMessage* message);
			void				SetDragMessage(BMessage* message);
			void				SetGlobalDropTargetIndicator(bool isGlobal);

	virtual	void				Draw(BRect updateRect);
	virtual	bool 				InitiateDrag(BPoint point, int32 index,
									bool wasSelected);
	virtual	void 				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);
	virtual void				MouseUp(BPoint point);
	virtual	void 				AttachedToWindow();
	virtual	void 				MessageReceived(BMessage* message);
	virtual	void				KeyDown(const char* bytes, int32 numBytes);

private:
			bool				_AcceptsDragMessage(
									const BMessage* message) const;

private:
			int32				fDropIndex;
			BRect				fDropTargetHighlightFrame;
			bool				fGlobalDropTargetIndicator;
			BMessage*			fDeleteMessage;
			BMessage*			fDragMessage;
};


#endif	// LANGUAGE_LIST_VIEW_H
