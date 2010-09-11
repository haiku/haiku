/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */


#include "ICUTimeConversion.h"

#include <math.h>
#include <strings.h>

#include <unicode/gregocal.h>

#include <AutoDeleter.h>


namespace BPrivate {


ICUTimeConversion::ICUTimeConversion(const ICUTimeData& timeData)
	:
	fTimeData(timeData),
	fDataBridge(NULL)
{
	fTimeZoneID[0] = '\0';
}


ICUTimeConversion::~ICUTimeConversion()
{
}


void
ICUTimeConversion::Initialize(TimeConversionDataBridge* dataBridge)
{
	fDataBridge = dataBridge;
}


status_t
ICUTimeConversion::TZSet(const char* timeZoneID)
{
	// nothing to do if the given name matches any name of the current timezone
	if (strcasecmp(fTimeZoneID, timeZoneID) == 0)
		return B_OK;

	strlcpy(fTimeZoneID, timeZoneID, sizeof(fTimeZoneID));

	ObjectDeleter<TimeZone> icuTimeZone = TimeZone::createTimeZone(fTimeZoneID);
	if (icuTimeZone.Get() == NULL)
		return B_NO_MEMORY;

	int32_t rawOffset;
	int32_t dstOffset;
	UDate nowMillis = 1000 * (UDate)time(NULL);
	UErrorCode icuStatus = U_ZERO_ERROR;
	icuTimeZone->getOffset(nowMillis, FALSE, rawOffset, dstOffset, icuStatus);
	if (!U_SUCCESS(icuStatus)) {
		*fDataBridge->addrOfTimezone = 0;
		*fDataBridge->addrOfDaylight = false;
		strcpy(fDataBridge->addrOfTZName[0], "GMT");
		strcpy(fDataBridge->addrOfTZName[1], "GMT");

		return B_ERROR;
	}

	*fDataBridge->addrOfTimezone = -1 * (rawOffset + dstOffset) / 1000;
		// we want seconds, not the ms that ICU gives us

	*fDataBridge->addrOfDaylight = icuTimeZone->useDaylightTime();

	for (int i = 0; i < 2; ++i) {
		UnicodeString icuString;
		icuTimeZone->getDisplayName(i == 1, TimeZone::SHORT_COMMONLY_USED,
			fTimeData.ICULocale(), icuString);
		CheckedArrayByteSink byteSink(fDataBridge->addrOfTZName[i],
			sizeof(fTimeZoneID));
		icuString.toUTF8(byteSink);

		// make sure to canonicalize "GMT+00:00" to just "GMT"
		if (strcmp(fDataBridge->addrOfTZName[i], "GMT+00:00") == 0)
			fDataBridge->addrOfTZName[i][3] = '\0';
	}

	return B_OK;
}


status_t
ICUTimeConversion::Localtime(const time_t* inTime, struct tm* tmOut)
{
	ObjectDeleter<TimeZone> icuTimeZone = TimeZone::createTimeZone(fTimeZoneID);
	if (icuTimeZone.Get() == NULL)
		return B_NO_MEMORY;

	tmOut->tm_zone = fTimeZoneID;
	return _FillTmValues(icuTimeZone.Get(), inTime, tmOut);
}


status_t
ICUTimeConversion::Gmtime(const time_t* inTime, struct tm* tmOut)
{
	const TimeZone* icuTimeZone = TimeZone::getGMT();
		// no delete - doesn't belong to us

	return _FillTmValues(icuTimeZone, inTime, tmOut);
}


status_t
ICUTimeConversion::Mktime(struct tm* inOutTm, time_t& timeOut)
{
	ObjectDeleter<TimeZone> icuTimeZone = TimeZone::createTimeZone(fTimeZoneID);
	if (icuTimeZone.Get() == NULL)
		return B_NO_MEMORY;

	UErrorCode icuStatus = U_ZERO_ERROR;
	GregorianCalendar calendar(*icuTimeZone.Get(), fTimeData.ICULocale(),
		icuStatus);
	if (!U_SUCCESS(icuStatus))
		return B_ERROR;

	calendar.setLenient(TRUE);
	calendar.set(inOutTm->tm_year + 1900, inOutTm->tm_mon, inOutTm->tm_mday,
		inOutTm->tm_hour, inOutTm->tm_min, inOutTm->tm_sec);

	UDate timeInMillis = calendar.getTime(icuStatus);
	if (!U_SUCCESS(icuStatus))
		return B_ERROR;
	timeOut = (time_t)((int64_t)timeInMillis / 1000);

	return _FillTmValues(icuTimeZone.Get(), &timeOut, inOutTm);
}


status_t
ICUTimeConversion::_FillTmValues(const TimeZone* icuTimeZone,
	const time_t* inTime, struct tm* tmOut)
{
	UErrorCode icuStatus = U_ZERO_ERROR;
	GregorianCalendar calendar(*icuTimeZone, fTimeData.ICULocale(), icuStatus);
	if (!U_SUCCESS(icuStatus))
		return B_ERROR;

	calendar.setTime(1000 * (UDate)*inTime, icuStatus);
	if (!U_SUCCESS(icuStatus))
		return B_ERROR;

	tmOut->tm_sec = calendar.get(UCAL_SECOND, icuStatus);
	if (!U_SUCCESS(icuStatus))
		return B_ERROR;
	tmOut->tm_min = calendar.get(UCAL_MINUTE, icuStatus);
	if (!U_SUCCESS(icuStatus))
		return B_ERROR;
	tmOut->tm_hour = calendar.get(UCAL_HOUR_OF_DAY, icuStatus);
	if (!U_SUCCESS(icuStatus))
		return B_ERROR;
	tmOut->tm_mday = calendar.get(UCAL_DAY_OF_MONTH, icuStatus);
	if (!U_SUCCESS(icuStatus))
		return B_ERROR;
	tmOut->tm_mon = calendar.get(UCAL_MONTH, icuStatus);
	if (!U_SUCCESS(icuStatus))
		return B_ERROR;
	tmOut->tm_year = calendar.get(UCAL_YEAR, icuStatus) - 1900;
	if (!U_SUCCESS(icuStatus))
		return B_ERROR;
	tmOut->tm_wday = calendar.get(UCAL_DAY_OF_WEEK, icuStatus) - 1;
	if (!U_SUCCESS(icuStatus))
		return B_ERROR;
	tmOut->tm_yday = calendar.get(UCAL_DAY_OF_YEAR, icuStatus) - 1;
	if (!U_SUCCESS(icuStatus))
		return B_ERROR;
	tmOut->tm_isdst = calendar.inDaylightTime(icuStatus);
	if (!U_SUCCESS(icuStatus))
		return B_ERROR;
	tmOut->tm_gmtoff = (calendar.get(UCAL_ZONE_OFFSET, icuStatus)
		+ calendar.get(UCAL_DST_OFFSET, icuStatus)) / 1000;
	if (!U_SUCCESS(icuStatus))
		return B_ERROR;
	tmOut->tm_zone = fDataBridge->addrOfTZName[tmOut->tm_isdst ? 1 : 0];

	return B_OK;
}


}	// namespace BPrivate
