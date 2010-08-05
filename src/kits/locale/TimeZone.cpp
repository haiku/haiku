/*
 * Copyright (c) 2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Adrien Destugues <pulkomandy@pulkomandy.ath.cx>
 * 		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include <TimeZone.h>

#include <unicode/timezone.h>
#include <ICUWrapper.h>


const char* BTimeZone::kNameOfGmtZone = "GMT";


BTimeZone::BTimeZone(const char* zoneCode)
{
	SetTo(zoneCode);
}


BTimeZone::~BTimeZone()
{
}


const BString&
BTimeZone::Name() const
{
	return fName;
}


const BString&
BTimeZone::DaylightSavingName() const
{
	return fDaylightSavingName;
}


const BString&
BTimeZone::ShortName() const
{
	return fShortName;
}


const BString&
BTimeZone::ShortDaylightSavingName() const
{
	return fShortDaylightSavingName;
}


const BString&
BTimeZone::Code() const
{
	return fCode;
}


int
BTimeZone::OffsetFromGMT() const
{
	return fOffsetFromGMT;
}


bool
BTimeZone::SupportsDaylightSaving() const
{
	return fSupportsDaylightSaving;
}


status_t
BTimeZone::InitCheck() const
{
	return fInitStatus;
}


status_t
BTimeZone::SetTo(const char* zoneCode)
{
	TimeZone* icuTimeZone;
	if (zoneCode == NULL || zoneCode[0] == '\0')
		icuTimeZone = TimeZone::createDefault();
	else
		icuTimeZone = TimeZone::createTimeZone(zoneCode);

	UnicodeString unicodeString;
	icuTimeZone->getID(unicodeString);
	BStringByteSink converter(&fCode);
	unicodeString.toUTF8(converter);

	unicodeString.remove();
	icuTimeZone->getDisplayName(false, TimeZone::LONG, unicodeString);
	converter.SetTo(&fName);
	unicodeString.toUTF8(converter);

	unicodeString.remove();
	icuTimeZone->getDisplayName(true, TimeZone::LONG, unicodeString);
	converter.SetTo(&fDaylightSavingName);
	unicodeString.toUTF8(converter);

	unicodeString.remove();
	icuTimeZone->getDisplayName(false, TimeZone::SHORT, unicodeString);
	converter.SetTo(&fShortName);
	unicodeString.toUTF8(converter);

	unicodeString.remove();
	icuTimeZone->getDisplayName(true, TimeZone::SHORT, unicodeString);
	converter.SetTo(&fShortDaylightSavingName);
	unicodeString.toUTF8(converter);

	fSupportsDaylightSaving = icuTimeZone->useDaylightTime();

	int32_t rawOffset;
	int32_t dstOffset;
	UDate nowMillis = 1000 * (double)time(NULL);

	UErrorCode error = U_ZERO_ERROR;
	icuTimeZone->getOffset(nowMillis, FALSE, rawOffset, dstOffset, error);
	if (!U_SUCCESS(error)) {
		fOffsetFromGMT = 0;
		fInitStatus = B_ERROR;
	} else {
		fOffsetFromGMT = (rawOffset + dstOffset) / 1000;
			// we want seconds, not ms (which ICU gives us)
		fInitStatus = B_OK;
	}

	delete icuTimeZone;

	return fInitStatus;
}
