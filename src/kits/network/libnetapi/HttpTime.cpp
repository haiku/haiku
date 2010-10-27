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


#define PRINT(x) printf x


static const char* 	kRfc1123Format			= "%a, %d %b %Y %H:%M:%S GMT";
static const char* 	kCookieFormat			= "%a, %d-%b-%Y %H:%M:%S GMT";
static const char* 	kRfc1036Format			= "%A, %d-%b-%y %H:%M:%S GMT";
static const char* 	kAscTimeFormat			= "%a %d %b %H:%M:%S %Y";
static const uint16	kTimetToStringMaxLength	= 128;

using namespace BPrivate;


BHttpTime::BHttpTime()
	:
	fDate(0),
	fDateFormat(B_HTTP_TIME_FORMAT_PREFERRED)
{
}


BHttpTime::BHttpTime(time_t date)
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
BHttpTime::SetDate(time_t date)
{
	fDate = date;
}

	
// #pragma mark Date conversion


time_t
BHttpTime::Parse()
{
	struct tm expireTime;
	
	if (fDateString.Length() < 4)
		return 0;
		
	if (fDateString[3] == ',') {
		if (strptime(fDateString.String(), kRfc1123Format, &expireTime)
				== NULL) {
			strptime(fDateString.String(), kCookieFormat, &expireTime);
			fDateFormat = B_HTTP_TIME_FORMAT_COOKIE;
		} else
			fDateFormat = B_HTTP_TIME_FORMAT_RFC1123;
	} else if (fDateString[3] == ' ') {
		strptime(fDateString.String(), kRfc1036Format, &expireTime);
		fDateFormat = B_HTTP_TIME_FORMAT_RFC1036;
	} else {
		strptime(fDateString.String(), kAscTimeFormat, &expireTime);
		fDateFormat = B_HTTP_TIME_FORMAT_ASCTIME;
	}

//	return timegm(&expireTime);
// TODO: The above was used initially. See http://en.wikipedia.org/wiki/Time.h
// stippi: I don't know how Christophe had this code compiling initially,
// since Haiku does not appear to implement timegm().
return mktime(&expireTime);
}


BString
BHttpTime::ToString(int8 format)
{
	BString expirationFinal;
	struct tm* expirationTm = localtime(&fDate);
	char expirationString[kTimetToStringMaxLength + 1];
	size_t strLength;
	
	switch ((format == B_HTTP_TIME_FORMAT_PARSED)?fDateFormat:format) {
		default:
		case B_HTTP_TIME_FORMAT_RFC1123:
			strLength = strftime(expirationString, kTimetToStringMaxLength,
				kRfc1123Format, expirationTm);
			break;
			
		case B_HTTP_TIME_FORMAT_RFC1036:
			strLength = strftime(expirationString, kTimetToStringMaxLength,
				kRfc1036Format, expirationTm);
			break;
		
		case B_HTTP_TIME_FORMAT_ASCTIME:
			strLength = strftime(expirationString, kTimetToStringMaxLength,
				kAscTimeFormat, expirationTm);
			break;		
	}
	
	expirationFinal.SetTo(expirationString, strLength);	
	return expirationFinal;
}
