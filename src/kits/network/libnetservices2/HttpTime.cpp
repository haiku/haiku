/*
 * Copyright 2010-2022 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 *		Adrien Destugues, pulkomandy@gmail.com
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */

#include <HttpTime.h>

#include <list>
#include <new>

#include <cstdio>

using namespace BPrivate::Network;


// The formats used should be, in order of preference (according to RFC2616,
// section 3.3):
// RFC1123 / RFC822: "Sun, 06 Nov 1994 08:49:37 GMT"
// RFC850          : "Sunday, 06-Nov-94 08:49:37 GMT" (obsoleted by RFC 1036)
// asctime         : "Sun Nov 6 08:49:37 1994"
//
// RFC1123 is the preferred one because it has 4 digit years.
//
// But of course in real life, all possible mixes of the formats are used.
// Believe it or not, it's even possible to find some website that gets this
// right and use one of the 3 formats above.
// Often seen variants are:
// - RFC1036 but with 4 digit year,
// - Missing or different timezone indicator
// - Invalid weekday
static const std::list<std::pair<BHttpTimeFormat, const char*>> kDateFormats = {
	// RFC822
	{BHttpTimeFormat::RFC1123, "%a, %d %b %Y %H:%M:%S GMT"}, // canonical
	{BHttpTimeFormat::RFC1123, "%a, %d %b %Y %H:%M:%S"}, // without timezone
	// Standard RFC850
	{BHttpTimeFormat::RFC850, "%A, %d-%b-%y %H:%M:%S GMT"}, // canonical
	{BHttpTimeFormat::RFC850, "%A, %d-%b-%y %H:%M:%S"}, // without timezone
	// RFC 850 with 4 digit year
	{BHttpTimeFormat::RFC850, "%a, %d-%b-%Y %H:%M:%S"}, // without timezone
	{BHttpTimeFormat::RFC850, "%a, %d-%b-%Y %H:%M:%S GMT"}, // with 4-digit year
	{BHttpTimeFormat::RFC850, "%a, %d-%b-%Y %H:%M:%S UTC"}, // "UTC" timezone
	// asctime
	{BHttpTimeFormat::AscTime, "%a %b %e %H:%M:%S %Y"},
};


// #pragma mark BHttpTime::InvalidInput


BHttpTime::InvalidInput::InvalidInput(const char* origin, BString input)
	:
	BError(origin),
	input(std::move(input))
{
}


const char*
BHttpTime::InvalidInput::Message() const noexcept
{
	if (input.IsEmpty())
		return "A HTTP timestamp cannot be empty";
	else
		return "The HTTP timestamp string does not match the expected format";
}


BString
BHttpTime::InvalidInput::DebugMessage() const
{
	BString output = BError::DebugMessage();
	if (!input.IsEmpty())
		output << ":\t " << input << "\n";
	return output;
}


// #pragma mark BHttpTime


BHttpTime::BHttpTime() noexcept
	:
	fDate(BDateTime::CurrentDateTime(B_GMT_TIME)),
	fDateFormat(BHttpTimeFormat::RFC1123)
{
}


BHttpTime::BHttpTime(BDateTime date)
	:
	fDate(date),
	fDateFormat(BHttpTimeFormat::RFC1123)
{
	if (!fDate.IsValid())
		throw InvalidInput(__PRETTY_FUNCTION__, "Invalid BDateTime object");
}


BHttpTime::BHttpTime(const BString& dateString)
	:
	fDate(0),
	fDateFormat(BHttpTimeFormat::RFC1123)
{
	_Parse(dateString);
}


// #pragma mark Date modification


void
BHttpTime::SetTo(const BString& string)
{
	_Parse(string);
}


void
BHttpTime::SetTo(BDateTime date)
{
	if (!date.IsValid())
		throw InvalidInput(__PRETTY_FUNCTION__, "Invalid BDateTime object");

	fDate = date;
	fDateFormat = BHttpTimeFormat::RFC1123;
}


// #pragma mark Date Access


BDateTime
BHttpTime::DateTime() const noexcept
{
	return fDate;
}


BHttpTimeFormat
BHttpTime::DateTimeFormat() const noexcept
{
	return fDateFormat;
}


BString
BHttpTime::ToString(BHttpTimeFormat outputFormat) const
{
	BString expirationFinal;
	struct tm expirationTm = {};
	expirationTm.tm_sec = fDate.Time().Second();
	expirationTm.tm_min = fDate.Time().Minute();
	expirationTm.tm_hour = fDate.Time().Hour();
	expirationTm.tm_mday = fDate.Date().Day();
	expirationTm.tm_mon = fDate.Date().Month() - 1;
	expirationTm.tm_year = fDate.Date().Year() - 1900;
	// strftime starts weekday count at 0 for Sunday,
	// while DayOfWeek starts at 1 for Monday and thus uses 7 for Sunday
	expirationTm.tm_wday = fDate.Date().DayOfWeek() % 7;
	expirationTm.tm_yday = 0;
	expirationTm.tm_isdst = 0;

	for (auto& [format, formatString]: kDateFormats) {
		if (format != outputFormat)
			continue;

		static const uint16 kTimetToStringMaxLength = 128;
		char expirationString[kTimetToStringMaxLength + 1];
		size_t strLength;

		strLength
			= strftime(expirationString, kTimetToStringMaxLength, formatString, &expirationTm);

		expirationFinal.SetTo(expirationString, strLength);
		break;
	}

	return expirationFinal;
}


void
BHttpTime::_Parse(const BString& dateString)
{
	if (dateString.Length() < 4)
		throw InvalidInput(__PRETTY_FUNCTION__, dateString);

	struct tm expireTime = {};

	bool found = false;
	for (auto& [format, formatString]: kDateFormats) {
		const char* result = strptime(dateString.String(), formatString, &expireTime);

		if (result == dateString.String() + dateString.Length()) {
			fDateFormat = format;
			found = true;
			break;
		}
	}

	// Did we identify some valid format?
	if (!found)
		throw InvalidInput(__PRETTY_FUNCTION__, dateString);

	// Now convert the struct tm from strptime into a BDateTime.
	BTime time(expireTime.tm_hour, expireTime.tm_min, expireTime.tm_sec);
	BDate date(expireTime.tm_year + 1900, expireTime.tm_mon + 1, expireTime.tm_mday);
	fDate = BDateTime(date, time);
}


// #pragma mark Convenience Functions


BDateTime
BPrivate::Network::parse_http_time(const BString& string)
{
	BHttpTime httpTime(string);
	return httpTime.DateTime();
}


BString
BPrivate::Network::format_http_time(BDateTime timestamp, BHttpTimeFormat outputFormat)
{
	BHttpTime httpTime(timestamp);
	return httpTime.ToString(outputFormat);
}
