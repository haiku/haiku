/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */


#include "LocaleBackend.h"

#include <ctype.h>
#include <langinfo.h>
#include <string.h>
#include <time.h>

#include <PosixCtype.h>
#include <PosixLanginfo.h>
#include <PosixLCTimeInfo.h>
#include <PosixLocaleConv.h>


extern locale_data* _nl_current_LC_NUMERIC;


namespace BPrivate {
namespace Libroot {


LocaleCtypeDataBridge::LocaleCtypeDataBridge()
	:
	addrOfClassInfoTable(&__ctype_b),
	addrOfToLowerTable(&__ctype_tolower),
	addrOfToUpperTable(&__ctype_toupper),
	posixClassInfo(gPosixClassInfo),
	posixToLowerMap(gPosixToLowerMap),
	posixToUpperMap(gPosixToUpperMap)
{
}


void LocaleCtypeDataBridge::setMbCurMax(unsigned short mbCurMax)
{
	__ctype_mb_cur_max = mbCurMax;
}


LocaleMessagesDataBridge::LocaleMessagesDataBridge()
	:
	posixLanginfo(gPosixLanginfo)
{
}


LocaleMonetaryDataBridge::LocaleMonetaryDataBridge()
	:
	posixLocaleConv(&gPosixLocaleConv)
{
}


LocaleNumericDataBridge::LocaleNumericDataBridge()
	:
	originalGlibcLocale(_nl_current_LC_NUMERIC),
	posixLocaleConv(&gPosixLocaleConv)
{
	memcpy(&glibcNumericLocale, _nl_current_LC_NUMERIC,
		sizeof(glibcNumericLocale));
	_nl_current_LC_NUMERIC = (locale_data*)&glibcNumericLocale;
}


LocaleNumericDataBridge::~LocaleNumericDataBridge()
{
	_nl_current_LC_NUMERIC = originalGlibcLocale;
}


LocaleTimeDataBridge::LocaleTimeDataBridge()
	:
	posixLCTimeInfo(&gPosixLCTimeInfo)
{
}


TimeConversionDataBridge::TimeConversionDataBridge()
	:
	addrOfDaylight(&daylight),
	addrOfTimezone(&timezone),
	addrOfTZName(tzname)
{
}


LocaleDataBridge::LocaleDataBridge()
	:
	posixLanginfo(gPosixLanginfo)
{
}


}	// namespace Libroot
}	// namespace BPrivate
