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
								~TimeZoneListItem();

			void				DrawItem(BView* owner, BRect frame,
									bool complete = false);

			bool				HasTimeZone() const;
			const BTimeZone&	TimeZone() const;
			const BString&		ID() const;
			const BString&		Name() const;
			int					OffsetFromGMT() const;

private:
			BBitmap*			fIcon;
			BTimeZone*			fTimeZone;
};


#endif	// _TIME_ZONE_LIST_ITEM_H
