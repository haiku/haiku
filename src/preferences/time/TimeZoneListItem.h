/*
 * Copyright 2010, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues <pulkomandy@pulkomandy.ath.cx>
 */

#ifndef __TIMEZONELISTITEM_H__
#define __TIMEZONELISTITEM_H__


#include <StringItem.h>


class BBitmap;
class BCountry;
class BTimeZone;


class TimeZoneListItem : public BStringItem {
	public:
					TimeZoneListItem(const char* text, BCountry* country,
						BTimeZone* timeZone);
					~TimeZoneListItem();

		void		DrawItem(BView* owner, BRect frame, bool complete = false);
		void		Code(char* buffer);

	private:
		BBitmap*	fIcon;
		BTimeZone*	fTimeZone;
};

#endif
