/*
 * Copyright 2010-2011, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */


#include "ICUTimeConversion.h"

#include <math.h>
#include <strings.h>

#include <unicode/gregocal.h>


namespace BPrivate {
namespace Libroot {


ICUTimeConversion::ICUTimeConversion(const ICUTimeData& timeData)
	:
	fTimeData(timeData),
	fDataBridge(NULL),
	fTimeZone(NULL)
{
	fTimeZoneID[0] = '\0';
}


ICUTimeConversion::~ICUTimeConversion()
{
	delete fTimeZone;
}


void
ICUTimeConversion::Initialize(TimeConversionDataBridge* dataBridge)
{
	fDataBridge = dataBridge;
}


status_t
ICUTimeConversion::TZSet(const char* timeZoneID, const char* tz)
{
	bool offsetHasBeenSet = false;

	// The given TZ environment variable's content overrides the default
	// system timezone.
	if (tz != NULL) {
		// If the value given in the TZ env-var starts with a colon, that
		// value is implementation specific, we expect a full timezone ID.
		if (*tz == ':') {
			// nothing to do if the given name matches the current timezone
			if (strcasecmp(fTimeZoneID, tz + 1) == 0)
				return B_OK;

			strlcpy(fTimeZoneID, tz + 1, sizeof(fTimeZoneID));
		} else {
			// note timezone name
			strlcpy(fTimeZoneID, tz, sizeof(fTimeZoneID));

			// nothing to do if the given name matches the current timezone
			if (strcasecmp(fTimeZoneID, fDataBridge->addrOfTZName[0]) == 0)
				return B_OK;

			// parse TZ variable (only <std> and <offset> supported)
			const char* tzNameEnd = tz;
			while(isalpha(*tzNameEnd))
				++tzNameEnd;
			if (*tzNameEnd == '-' || *tzNameEnd == '+') {
				int hours = 0;
				int minutes = 0;
				int seconds = 0;
				sscanf(tzNameEnd + 1, "%2d:%2d:%2d", &hours, &minutes,
					&seconds);
				hours = min_c(24, max_c(0, hours));
				minutes = min_c(59, max_c(0, minutes));
				seconds = min_c(59, max_c(0, seconds));

				*fDataBridge->addrOfTimezone = (*tzNameEnd == '-' ? -1 : 1)
					* (hours * 3600 + minutes * 60 + seconds);
				offsetHasBeenSet = true;
			}
		}
	} else {
		// nothing to do if the given name matches the current timezone
		if (strcasecmp(fTimeZoneID, timeZoneID) == 0)
			return B_OK;

		strlcpy(fTimeZoneID, timeZoneID, sizeof(fTimeZoneID));
	}

	delete fTimeZone;
	fTimeZone = TimeZone::createTimeZone(fTimeZoneID);
	if (fTimeZone == NULL)
		return B_NO_MEMORY;

	if (offsetHasBeenSet) {
		fTimeZone->setRawOffset(*fDataBridge->addrOfTimezone * -1 * 1000);
	} else {
		int32_t rawOffset;
		int32_t dstOffset;
		UDate nowMillis = 1000 * (UDate)time(NULL);
		UErrorCode icuStatus = U_ZERO_ERROR;
		fTimeZone->getOffset(nowMillis, FALSE, rawOffset, dstOffset, icuStatus);
		if (!U_SUCCESS(icuStatus)) {
			*fDataBridge->addrOfTimezone = 0;
			*fDataBridge->addrOfDaylight = false;
			strcpy(fDataBridge->addrOfTZName[0], "GMT");
			strcpy(fDataBridge->addrOfTZName[1], "GMT");

			return B_ERROR;
		}
		*fDataBridge->addrOfTimezone = -1 * (rawOffset + dstOffset) / 1000;
			// we want seconds, not the ms that ICU gives us
	}

	*fDataBridge->addrOfDaylight = fTimeZone->useDaylightTime();

	for (int i = 0; i < 2; ++i) {
		if (tz != NULL && *tz != ':' && i == 0) {
			strcpy(fDataBridge->addrOfTZName[0], fTimeZoneID);
		} else {
			UnicodeString icuString;
			fTimeZone->getDisplayName(i == 1, TimeZone::SHORT,
				fTimeData.ICULocale(), icuString);
			CheckedArrayByteSink byteSink(fDataBridge->addrOfTZName[i],
				sizeof(fTimeZoneID));
			icuString.toUTF8(byteSink);

			// make sure to canonicalize "GMT+00:00" to just "GMT"
			if (strcmp(fDataBridge->addrOfTZName[i], "GMT+00:00") == 0)
				fDataBridge->addrOfTZName[i][3] = '\0';
		}
	}

	return B_OK;
}


status_t
ICUTimeConversion::Localtime(const time_t* inTime, struct tm* tmOut)
{
	if (fTimeZone == NULL)
		return B_NO_INIT;

	tmOut->tm_zone = fTimeZoneID;
	return _FillTmValues(fTimeZone, inTime, tmOut);
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
	if (fTimeZone == NULL)
		return B_NO_INIT;

	UErrorCode icuStatus = U_ZERO_ERROR;
	GregorianCalendar calendar(*fTimeZone, fTimeData.ICULocale(),
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

	return _FillTmValues(fTimeZone, &timeOut, inOutTm);
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


}	// namespace Libroot
}	// namespace BPrivate
