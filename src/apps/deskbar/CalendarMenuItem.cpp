/*
 * Copyright 2005-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#ifdef _SHOW_CALENDAR_MENU_ITEM
	// as specified in the makefile

#include "CalendarMenuItem.h"

#include <stdio.h>
#include <time.h>


static const int32 kDaysPerMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	//								  Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec

static const int32 kTitleFontSize = 9;

static const int32 kTitleGap = 3;
static const int32 kColumnGap = 9;
static const int32 kRowGap = 2;


static bool
is_leap_year(int32 year)
{
	return !(year % 4) && (year % 100 || !(year % 400));
}


static int32
days_per_month(struct tm &tm)
{
	int32 days = kDaysPerMonth[tm.tm_mon];
	return days + (tm.tm_mon == 1 && is_leap_year(tm.tm_year + 1900) ? 1 : 0);
}


static int32
first_weekday_of_month(struct tm &tm)
{
	return (tm.tm_wday - ((tm.tm_mday - 1) % 7) + 7) % 7;
}


//	#pragma mark -


CalendarMenuItem::CalendarMenuItem()
	: BMenuItem("calendar", NULL)
{
	SetEnabled(false);
}


CalendarMenuItem::~CalendarMenuItem()
{
}


void
CalendarMenuItem::DrawContent()
{
	time_t currentTime = time(NULL);
	struct tm tm;
	localtime_r(&currentTime, &tm);

	Menu()->PushState();
	Menu()->SetOrigin(ContentLocation());

	rgb_color todayBackgroundColor = tint_color(Menu()->LowColor(), B_LIGHTEN_2_TINT);
	rgb_color dayColor = tint_color(Menu()->HighColor(), B_DARKEN_2_TINT);
	rgb_color titleColor = Menu()->HighColor();

	BFont font;
	Menu()->GetFont(&font);
	font.SetSize(kTitleFontSize);
	Menu()->SetFont(&font, B_FONT_SIZE);

	// Draw month name & year

	Menu()->SetHighColor(titleColor);
	char text[64];
	strftime(text, sizeof(text), "%B, %Y", &tm);
	float width = Menu()->StringWidth(text);
	Menu()->DrawString(text, BPoint((fColumnWidth * 7 - width) / 2, fTitleHeight));

	// Draw weekdays

	for (tm.tm_wday = 0; tm.tm_wday < 7; tm.tm_wday++) {
		strftime(text, sizeof(text), "%a", &tm);

		width = Menu()->StringWidth(text);
		Menu()->DrawString(text, BPoint(fColumnWidth * tm.tm_wday + (fColumnWidth - width) / 2,
			2 * fTitleHeight + kTitleGap));
	}

	// Draw days

	Menu()->SetHighColor(dayColor);
	Menu()->SetFont(be_plain_font);

	Menu()->GetFont(&font);
	font.SetFace(B_BOLD_FACE);

	int32 total = fFirstWeekday + days_per_month(tm);
	int32 day = 1;
	int32 row = 1;
	for (int32 i = fFirstWeekday; i < total; i++) {
		int32 column = i % 7;
		bool today = day == tm.tm_mday;

		if (today)
			Menu()->SetFont(&font);

		sprintf(text, "%ld", day);
		width = Menu()->StringWidth(text);
		BPoint point(fColumnWidth * column + (fColumnWidth - width) / 2,
			2 * (fTitleHeight + kTitleGap) + row * fRowHeight);

		if (today) {
			// draw a rectangle around today's day number
			Menu()->SetLowColor(todayBackgroundColor);
			Menu()->FillRect(BRect(point.x - 2, point.y - 1 - fFontHeight,
				point.x + width + 1, point.y + 2), B_SOLID_LOW);

			Menu()->SetHighColor(dayColor);
		}

		Menu()->DrawString(text, point);

		if (today) {
			Menu()->SetLowColor(ui_color(B_MENU_BACKGROUND_COLOR));
			Menu()->SetFont(be_plain_font);
		}

		day++;
		if (column == 6)
			row++;
	}

	Menu()->PopState();
}


void
CalendarMenuItem::GetContentSize(float *_width, float *_height)
{
	time_t currentTime = time(NULL);
	struct tm tm;
	localtime_r(&currentTime, &tm);

	BFont font;
	font.SetSize(kTitleFontSize);
	font_height fontHeight;
	font.GetHeight(&fontHeight);
	fTitleHeight = ceilf(fontHeight.ascent + fontHeight.descent + fontHeight.leading);

	font = be_plain_font;
	font.GetHeight(&fontHeight);
	fRowHeight = ceilf(fontHeight.ascent + fontHeight.descent + fontHeight.leading + kRowGap);
	fFontHeight = ceilf(fontHeight.ascent);
	fColumnWidth = font.StringWidth("99") + kColumnGap;

	fFirstWeekday = first_weekday_of_month(tm);
	fRows = (fFirstWeekday + days_per_month(tm) + 6) / 7;

	if (_width)
		*_width = 7 * (fColumnWidth - 1);
	if (_height)
		*_height = fRows * fRowHeight + 2 * (fTitleHeight + 2 * kTitleGap);
}

#endif	// _SHOW_CALENDAR_MENU_ITEM
