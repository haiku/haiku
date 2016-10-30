/*
 * Copyright 2010-2016 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 *		Adrien Destugues, pulkomandy@gmail.com
 */

#include <HttpTime.h>

#include <new>

#include <cstdio>


static const char* kDateFormats[] = {
	"%a, %d %b %Y %H:%M:%S",
	"%a, %d-%b-%Y %H:%M:%S",
	"%a, %d-%b-%Y %H:%M:%S GMT",
	"%a, %d-%b-%Y %H:%M:%S UTC",
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

	memset(&expireTime, 0, sizeof(struct tm));

	fDateFormat = B_HTTP_TIME_FORMAT_PARSED;
	unsigned int i;
	for (i = 0; i < sizeof(kDateFormats) / sizeof(const char*);
		i++) {
		const char* result = strptime(fDateString.String(), kDateFormats[i],
			&expireTime);

		// We need to parse the complete value for the "Expires" key.
		// Otherwise, we consider this to be a session cookie (or try another
		// one of the date formats).
		if (result == fDateString.String() + fDateString.Length()) {
			fDateFormat = i;
			break;
		}
	}

	// Did we identify some valid format?
	if (fDateFormat == B_HTTP_TIME_FORMAT_PARSED)
		return 0;

	// Now convert the struct tm from strptime into a BDateTime.
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
