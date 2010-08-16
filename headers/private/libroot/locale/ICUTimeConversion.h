/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ICU_TIME_CONVERSION_H
#define _ICU_TIME_CONVERSION_H


#include <time.h>

#include <StorageDefs.h>

#include "ICUTimeData.h"
#include "LocaleBackend.h"


namespace BPrivate {


class ICUTimeConversion {
public:
								ICUTimeConversion(const ICUTimeData& timeData);
	virtual						~ICUTimeConversion();

	virtual void				Initialize(
									TimeConversionDataBridge* dataBridge);

			status_t			TZSet(const char* timeZoneID);

			status_t			Localtime(const time_t* inTime,
									struct tm* tmOut);
			status_t			Gmtime(const time_t* inTime, struct tm* tmOut);

			status_t			Mktime(struct tm* inOutTm, time_t& timeOut);

private:
			status_t			_FillTmValues(const TimeZone* icuTimeZone,
									const time_t* inTime, struct tm* tmOut);

			const ICUTimeData&	fTimeData;

			TimeConversionDataBridge*	fDataBridge;

			char				fTimeZoneID[B_FILE_NAME_LENGTH];
};


}	// namespace BPrivate


#endif	// _ICU_TIME_BACKEND_H
