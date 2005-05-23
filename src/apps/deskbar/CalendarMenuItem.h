/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef CALENDAR_MENU_ITEM_H
#define CALENDAR_MENU_ITEM_H


#include <MenuItem.h>


class CalendarMenuItem : public BMenuItem {
	public:
		CalendarMenuItem();
		~CalendarMenuItem();

		virtual void DrawContent();
		virtual void GetContentSize(float *_width, float *_height);

	private:
		float	fTitleHeight, fRowHeight, fColumnWidth, fFontHeight;
		int32	fRows, fFirstWeekday;
};

#endif	/* CALENDAR_MENU_ITEM_H */
