/*
 * Copyright 2004-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Julun <host.haiku@gmx.de>
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "DateTime.h"


#include <time.h>


namespace BPrivate {


BTime::BTime()
	: fHour(-1),
	  fMinute(-1),
	  fSecond(-1)
{
}


BTime::BTime(int32 hour, int32 minute, int32 second)
	: fHour(hour),
	  fMinute(minute),
	  fSecond(second)
{
}


BTime::~BTime()
{
}


bool
BTime::IsValid() const
{
	if (fHour < 0 || fHour >= 24)
		return false;

	if (fMinute < 0 || fMinute >= 60)
		return false;

	if (fSecond < 0 || fSecond >= 60)
		return false;

	return true;
}


BTime
BTime::CurrentTime(time_type type)
{
	time_t timer;
	struct tm result;
	struct tm* timeinfo;

	time(&timer);

	if (type == B_GMT_TIME)
		timeinfo = gmtime_r(&timer, &result);
	else
		timeinfo = localtime_r(&timer, &result);

	int32 sec = timeinfo->tm_sec;
	return BTime(timeinfo->tm_hour, timeinfo->tm_min, (sec > 59) ? 59 : sec);
}


bool
BTime::SetTime(int32 hour, int32 minute, int32 second)
{
	fHour = hour;
	fMinute = minute;
	fSecond = second;

	return IsValid();
}


int32
BTime::Hour() const
{
	return fHour;
}


int32
BTime::Minute() const
{
	return fMinute;
}


int32
BTime::Second() const
{
	return fSecond;
}


//	#pragma mark -


BDate::BDate()
	: fDay(-1),
	  fYear(-1),
	  fMonth(-1)
{
}


BDate::BDate(int32 year, int32 month, int32 day)
	: fDay(day),
	  fYear(year),
	  fMonth(month)
{
}


BDate::~BDate()
{
}


bool
BDate::IsValid() const
{
	// fail if the year goes less 1583
	// 10/15/1582 start of Gregorian calendar
	if (fYear < 1583)
		return false;

	if (fMonth < 1 || fMonth > 12)
		return false;

	if (fDay < 1 || fDay > DaysInMonth())
		return false;

	return true;
}


BDate
BDate::CurrentDate(time_type type)
{
	time_t timer;
	struct tm result;
	struct tm* timeinfo;

	time(&timer);

	if (type == B_GMT_TIME)
		timeinfo = gmtime_r(&timer, &result);
	else
		timeinfo = localtime_r(&timer, &result);

	return BDate(timeinfo->tm_year + 1900, timeinfo->tm_mon +1, timeinfo->tm_mday);
}


bool
BDate::SetDate(int32 year, int32 month, int32 day)
{
	fDay = day;
	fYear = year;
	fMonth = month;

	return IsValid();
}


void
BDate::AddDays(int32 days)
{
	days += Day();
	if (days > 0) {
		while (days > DaysInMonth()) {
			days -= DaysInMonth();
			fMonth ++;
			if (fMonth == 13) {
				fMonth = 1;
				fYear ++;
			}
		}
	} else {
		while (days <= 0) {
			fMonth --;
			if (fMonth == 0) {
				fMonth = 12;
				fYear --;
			}
			days += DaysInMonth();
		}
	}
	fDay = days;
}


int32
BDate::Day() const
{
	return fDay;
}


int32
BDate::Year() const
{
	return fYear;
}


int32
BDate::Month() const
{
	return fMonth;
}


int32
BDate::WeekNumber() const
{
	/*
		This algorithm is taken from:
		Frequently Asked Questions about Calendars
		Version 2.8 Claus Tøndering 15 December 2005

		Note: it will work only within the Gregorian Calendar
	*/

	if (!IsValid())
		return 0;

	int32 a;
	int32 b;
	int32 s;
	int32 e;
	int32 f;

	if (fMonth > 0 && fMonth < 3) {
		a = fYear - 1;
		b = (a / 4) - (a / 100) + (a / 400);
		int32 c = ((a - 1) / 4) - ((a - 1) / 100) + ((a -1) / 400);
		s = b - c;
		e = 0;
		f = fDay - 1 + 31 * (fMonth - 1);
	} else if (fMonth >= 3 && fMonth <= 12) {
		a = fYear;
		b = (a / 4) - (a / 100) + (a / 400);
		int32 c = ((a - 1) / 4) - ((a - 1) / 100) + ((a -1) / 400);
		s = b - c;
		e = s + 1;
		f = fDay + ((153 * (fMonth - 3) + 2) / 5) + 58 + s;
	} else
		return 0;

	int32 g = (a + b) % 7;
	int32 d = (f + g - e) % 7;
	int32 n = f + 3 - d;

	int32 weekNumber;
	if (n < 0)
		weekNumber = 53 - (g -s) / 5;
	else if (n > 364 + s)
		weekNumber = 1;
	else
		weekNumber = n / 7 + 1;

	return weekNumber;
}


int32
BDate::DayOfWeek() const
{
	/*
		This algorithm is taken from:
		Frequently Asked Questions about Calendars
		Version 2.8 Claus Tøndering 15 December 2005

		Note: it will work only within the Gregorian Calendar
	*/

	if (!IsValid())
		return -1;

	int32 a = (14 - fMonth) / 12;
	int32 y = fYear - a;
	int32 m = fMonth + 12 * a - 2;
	int32 d = (fDay + y + (y / 4) - (y / 100) + (y / 400) + ((31 * m) / 12)) % 7;

	return d;
}


int32
BDate::DayOfYear() const
{
	/*
		Note: this function might fail for 1582...

		http://en.wikipedia.org/wiki/Gregorian_calendar:
			  The last day of the Julian calendar was Thursday October 4, 1582
			  and this was followed by the first day of the Gregorian calendar,
			  Friday October 15, 1582 (the cycle of weekdays was not affected).
	*/

	if (!IsValid())
		return -1;

	const int kFirstDayOfMonth[2][12] = {
		{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 },
		{ 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 }
	};

	if (IsLeapYear(fYear))
		return kFirstDayOfMonth[1][fMonth -1] + fDay;

	return kFirstDayOfMonth[0][fMonth -1] + fDay;
}


bool
BDate::IsLeapYear(int32 year) const
{
	return year % 400 == 0 || year % 4 == 0 && year % 100 != 0;
}


int32
BDate::DaysInYear() const
{
	if (!IsValid())
		return 0;

	return IsLeapYear(fYear) ? 366 : 365;
}


int32
BDate::DaysInMonth() const
{
	if (fMonth == 2 && IsLeapYear(fYear))
		return 29;

	const int32 daysInMonth[12] =
		{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	return daysInMonth[fMonth -1];
}


BString
BDate::ShortDayName(int32 day) const
{
	if (day < 1 || day > 7)
		return BString();

	tm tm_struct;
	memset(&tm_struct, 0, sizeof(tm));
	tm_struct.tm_wday = day == 7 ? 0 : day;

	char buffer[256];
	strftime(buffer, sizeof(buffer), "%a", &tm_struct);

	return BString(buffer);
}


BString
BDate::ShortMonthName(int32 month) const
{
	if (month < 1 || month > 12)
		return BString();

	tm tm_struct;
	memset(&tm_struct, 0, sizeof(tm));
	tm_struct.tm_mon = month -1;

	char buffer[256];
	strftime(buffer, sizeof(buffer), "%b", &tm_struct);

	return BString(buffer);
}


BString
BDate::LongDayName(int32 day) const
{
	if (day < 1 || day > 7)
		return BString();

	tm tm_struct;
	memset(&tm_struct, 0, sizeof(tm));
	tm_struct.tm_wday = day == 7 ? 0 : day;

	char buffer[256];
	strftime(buffer, sizeof(buffer), "%A", &tm_struct);

	return BString(buffer);
}


BString
BDate::LongMonthName(int32 month) const
{
	if (month < 1 || month > 12)
		return BString();

	tm tm_struct;
	memset(&tm_struct, 0, sizeof(tm));
	tm_struct.tm_mon = month -1;

	char buffer[256];
	strftime(buffer, sizeof(buffer), "%B", &tm_struct);

	return BString(buffer);
}


//	#pragma mark -


BDateTime::BDateTime(const BDate &date, const BTime &time)
	: fDate(date),
	  fTime(time)
{
}


BDateTime::~BDateTime()
{
}


bool
BDateTime::IsValid() const
{
	return fDate.IsValid() && fTime.IsValid();
}


BDateTime
BDateTime::CurrentDateTime(time_type type)
{
	BDate date = BDate::CurrentDate(type);
	BTime time = BTime::CurrentTime(type);

	return BDateTime(date, time);
}


void
BDateTime::SetDateTime(const BDate &date, const BTime &time)
{
	fDate = date;
	fTime = time;
}


BDate
BDateTime::Date() const
{
	return fDate;
}


void
BDateTime::SetDate(const BDate &date)
{
	fDate = date;
}


BTime
BDateTime::Time() const
{
	return fTime;
}


void
BDateTime::SetTime(const BTime &time)
{
	fTime = time;
}


uint32
BDateTime::Time_t() const
{
	tm tm_struct;

	tm_struct.tm_hour = fTime.Hour();
	tm_struct.tm_min = fTime.Minute();
	tm_struct.tm_sec = fTime.Second();

	tm_struct.tm_year = fDate.Year() - 1900;
	tm_struct.tm_mon = fDate.Month() - 1;
	tm_struct.tm_mday = fDate.Day();

	// set less 0 as we wan't use it
	tm_struct.tm_isdst = -1;

	// return secs_since_jan1_1970 or -1 on error
	return uint32(mktime(&tm_struct));
}

}	//namespace BPrivate
