/*
 * Copyright 2010-2011, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ICU_TIME_DATA_H
#define _ICU_TIME_DATA_H


#include "ICUCategoryData.h"
#include "LocaleBackend.h"

#include <unicode/datefmt.h>

#include <timelocal.h>


namespace BPrivate {
namespace Libroot {


class ICUTimeData : public ICUCategoryData {
	typedef	ICUCategoryData		inherited;
public:
								ICUTimeData(pthread_key_t tlsKey,
									struct lc_time_t& lcTimeInfo);
								~ICUTimeData();

			void				Initialize(LocaleTimeDataBridge* dataBridge);

	virtual	status_t			SetTo(const Locale& locale,
									const char* posixLocaleName);
	virtual	status_t			SetToPosix();

			const char*			GetLanginfo(int index);

			const Locale&		ICULocale() const;

private:
			status_t			_SetLCTimeEntries(const UnicodeString* strings,
									char* destination, int entrySize,
									int count, int maxCount);
			status_t			_SetLCTimePattern(DateFormat* format,
									char* destination, int destinationSize);

			char				fMon[12][24];
			char				fMonth[12][64];
			char				fWday[7][24];
			char				fWeekday[7][64];
			char				fTimeFormat[24];
			char				fDateFormat[24];
			char				fDateTimeFormat[32];
			char				fAm[24];
			char				fPm[24];
			char				fDateTimeZoneFormat[32];
			char				fAltMonth[12][64];
			char				fMonthDayOrder[4];
			char				fAmPmFormat[32];

			struct lc_time_t&	fLCTimeInfo;

			LocaleTimeDataBridge*	fDataBridge;
};


}	// namespace Libroot
}	// namespace BPrivate


#endif	// _ICU_TIME_DATA_H
