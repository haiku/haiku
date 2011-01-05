/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HOLIDAY_H_
#define _HOLIDAY_H_


#include <DateTime.h>


namespace BPrivate {


class BHoliday {
public:
	virtual						~BHoliday() { };
};


class BEasterHoliday : public BHoliday {
public:
	static	BDate				EasterSunday(int32 year);
	static	BDate				AscensionDay(int32 year);
	static	BDate				PentecostDay(int32 year);
};


/*!
	Returns the date for easter sunday for the \c year.
*/
/*static*/ BDate
BEasterHoliday::EasterSunday(int32 year)
{
	// http://bloggingabout.net/blogs/jschreuder/archive/2005/06/24/7019.aspx
	int32 gold = year % 19;
	int32 century = year / 100;
	int32 h = (century - (int32)(century / 4)
		- (int32)((century * 8 + 13) / 25) + 19 * gold + 15) % 30;
	int32 i = h - (int32)(h / 28) * (1 - (int32)(h / 28)
		* (int32)(29 / (h + 1)) * (int32)((21 - gold) / 11));

	int32 day = i - ((year + (int32)(year / 4) + i + 2 - century
		+ (int32)(century / 4)) % 7) + 28;
	BDate date;
	if (day > 31)
		date.SetDate(year, 4, day - 31);
	else
		date.SetDate(year, 3, day);
	return date;
}


/*!
	Returns the date for ascension day for the \c year.
*/
/*static*/ BDate
BEasterHoliday::AscensionDay(int32 year)
{
	BDate date = EasterSunday(year);
	date.AddDays(39);
	return date;
}


/*!
	Returns the date for pentecost day for the \c year.
*/
/*static*/ BDate
BEasterHoliday::PentecostDay(int32 year)
{
	BDate date = EasterSunday(year);
	date.AddDays(49);
	return date;
}


}	// namespace BPrivate


using BPrivate::BHoliday;
using BPrivate::BEasterHoliday;


#endif	// _HOLIDAY_H_
