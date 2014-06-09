/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 */

#include <HttpTime.h>

#include <new>

#include <cstdio>


static const char* kDateFormats[] = {
	"%a, %d %b %Y %H:%M:%S",
	"%a, %d-%b-%Y %H:%M:%S",
	"%A, %d-%b-%y %H:%M:%S",
	"%a %d %b %H:%M:%S %Y"
};

using namespace BPrivate;


BHttpTime::BHttpTime()
	:
	fDate(0),
	fDateFormat(B_HTTP_TIME_FORMAT_PREFERRED)
{
}


BHttpTime::BHttpTime(BDateTime date)
	:
	fDate(date),
	fDateFormat(B_HTTP_TIME_FORMAT_PREFERRED)
{
}


BHttpTime::BHttpTime(const BString& dateString)
	:
	fDateString(dateString),
	fDate(0),
	fDateFormat(B_HTTP_TIME_FORMAT_PREFERRED)
{
}

	
// #pragma mark Date modification


void
BHttpTime::SetString(const BString& string)
{
	fDateString = string;
}


void
BHttpTime::SetDate(BDateTime date)
{
	fDate = date;
}

	
// #pragma mark Date conversion


BDateTime
BHttpTime::Parse()
{
	struct tm expireTime;
	
	if (fDateString.Length() < 4)
		return 0;

	expireTime.tm_sec = 0;
	expireTime.tm_min = 0;
	expireTime.tm_hour = 0;
	expireTime.tm_mday = 0;
	expireTime.tm_mon = 0;
	expireTime.tm_year = 0;
	expireTime.tm_wday = 0;
	expireTime.tm_yday = 0;
	expireTime.tm_isdst = 0;
	
	fDateFormat = B_HTTP_TIME_FORMAT_PARSED;
	unsigned int i;
	for (i = 0; i < sizeof(kDateFormats) / sizeof(const char*);
		i++) {
		const char* result = strptime(fDateString.String(), kDateFormats[i],
			&expireTime);

		// Make sure we parsed enough of the date string, not just a small
		// part of it.
		if (result != NULL && result > fDateString.String() + 24) {
			fDateFormat = i;
			break;
		}
	}

	if (fDateFormat == B_HTTP_TIME_FORMAT_PARSED)
		return 0;

	BTime time(expireTime.tm_hour, expireTime.tm_min, expireTime.tm_sec);
	BDate date(expireTime.tm_year + 1900, expireTime.tm_mon + 1,
		expireTime.tm_mday);
	BDateTime dateTime(date, time);
	return dateTime;
}


BString
BHttpTime::ToString(int8 format)
{
	BString expirationFinal;
	struct tm expirationTm;
	expirationTm.tm_sec = fDate.Time().Second();
	expirationTm.tm_min = fDate.Time().Minute();
	expirationTm.tm_hour = fDate.Time().Hour();
	expirationTm.tm_mday = fDate.Date().Day();
	expirationTm.tm_mon = fDate.Date().Month() - 1;
	expirationTm.tm_year = fDate.Date().Year() - 1900;
	expirationTm.tm_wday = 0;
	expirationTm.tm_yday = 0;
	expirationTm.tm_isdst = 0;

	if (format == B_HTTP_TIME_FORMAT_PARSED)
		format = fDateFormat;

	if (format != B_HTTP_TIME_FORMAT_PARSED) {
		static const uint16 kTimetToStringMaxLength = 128;
		char expirationString[kTimetToStringMaxLength + 1];
		size_t strLength;
	
		strLength = strftime(expirationString, kTimetToStringMaxLength,
			kDateFormats[format], &expirationTm);
	
		expirationFinal.SetTo(expirationString, strLength);
	}
	return expirationFinal;
}
