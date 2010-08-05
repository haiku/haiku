/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */


#include "ICUTimeBackend.h"

#include <new>

#include <string.h>

#include <unicode/timezone.h>

#include <ErrnoMaintainer.h>


namespace BPrivate {


extern "C" TimeBackend*
CreateInstance()
{
	return new(std::nothrow) ICUTimeBackend();
}


ICUTimeBackend::ICUTimeBackend()
{
}


ICUTimeBackend::~ICUTimeBackend()
{
}


void
ICUTimeBackend::Initialize(TimeDataBridge* dataBridge)
{
	fDataBridge = dataBridge;
}


void
ICUTimeBackend::TZSet(void)
{
	ErrnoMaintainer errnoMaintainer;

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
	icuTimeZone->getDisplayName(unicodeString);
	converter.SetTo(&fName);
	unicodeString.toUTF8(converter);

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
}


}	// namespace BPrivate
