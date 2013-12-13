/*
 * Copyright 2010, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues <pulkomandy@pulkomandy.ath.cx>
 */
#ifndef _TIME_ZONE_LIST_ITEM_H
#define _TIME_ZONE_LIST_ITEM_H


#include <StringItem.h>


class BBitmap;
class BCountry;
class BTimeZone;


class TimeZoneListItem : public BStringItem {
public:
								TimeZoneListItem(const char* text,
									BCountry* country, BTimeZone* timeZone);
	virtual						~TimeZoneListItem();

	virtual	void				DrawItem(BView* owner, BRect frame,
									bool complete = false);

	virtual	void				Update(BView* owner, const BFont* font);

			bool				HasCountry() const
									{ return fCountry != NULL; };
			const BCountry&		Country() const { return *fCountry; };
			void				SetCountry(BCountry* country);

			bool				HasTimeZone() const
									{ return fTimeZone != NULL; };
			const BTimeZone&	TimeZone() const
									{ return *fTimeZone; };
			void				SetTimeZone(BTimeZone* timeZone);

			const BString&		ID() const;
			const BString&		Name() const;
			int					OffsetFromGMT() const;

private:
			void				_DrawItemWithTextOffset(BView* owner,
									BRect frame, bool complete,
									float textOffset);

			BCountry*			fCountry;
			BTimeZone*			fTimeZone;
			BBitmap*			fIcon;
};


#endif	// _TIME_ZONE_LIST_ITEM_H
