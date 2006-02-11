/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _STRING_ITEM_H
#define _STRING_ITEM_H


#include <ListItem.h>


class BStringItem : public BListItem {
	public:
							BStringItem(const char* text, uint32 outlineLevel = 0,
								bool expanded = true);
							BStringItem(BMessage* archive);
		virtual				~BStringItem();

		static BArchivable*	Instantiate(BMessage* archive);
		virtual status_t	Archive(BMessage* archive, bool deep = true) const;

		virtual	void		DrawItem(BView* owner, BRect frame, bool complete = false);
		virtual	void		SetText(const char* text);
		const char*			Text() const;

		virtual	void		Update(BView* owner, const BFont* font);

		virtual status_t	Perform(perform_code code, void* arg);

	private:
		virtual	void		_ReservedStringItem1();
		virtual	void		_ReservedStringItem2();

							BStringItem(const BStringItem& item);
		BStringItem&		operator=(const BStringItem& item);

		char*				fText;
		float				fBaselineOffset;
		uint32				_reserved[2];
};

#endif	// _STRING_ITEM_H
