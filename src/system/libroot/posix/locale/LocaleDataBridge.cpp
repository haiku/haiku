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
#include <ThreadLocale.h>


extern const unsigned short* __ctype_b;
extern const int* __ctype_tolower;
extern const int* __ctype_toupper;


namespace BPrivate {
namespace Libroot {


extern "C" GlibcLocaleStruct* _nl_current_locale();
extern "C" GlibcLocaleStruct _nl_global_locale;
#define _NL_CURRENT_DATA(category) \
	((locale_data*&)(_nl_current_locale()->__locales[category]))
#define _NL_GLOBAL_DATA(category) \
	((locale_data*&)(_nl_global_locale.__locales[category]))


LocaleCtypeDataBridge::LocaleCtypeDataBridge(bool isGlobal)
	:
	localClassInfoTable(__ctype_b),
	localToLowerTable(__ctype_tolower),
	localToUpperTable(__ctype_toupper),
	posixClassInfo(gPosixClassInfo),
	posixToLowerMap(gPosixToLowerMap),
	posixToUpperMap(gPosixToUpperMap),
	isGlobal(isGlobal)
{
	if (isGlobal) {
		addrOfClassInfoTable = &__ctype_b;
		addrOfToLowerTable = &__ctype_tolower;
		addrOfToUpperTable = &__ctype_toupper;
	} else {
		addrOfClassInfoTable = &localClassInfoTable;
		addrOfToLowerTable = &localToLowerTable;
		addrOfToUpperTable = &localToUpperTable;
	}
}


void LocaleCtypeDataBridge::setMbCurMax(unsigned short mbCurMax)
{
	__ctype_mb_cur_max = mbCurMax;
}


void
LocaleCtypeDataBridge::ApplyToCurrentThread()
{
	*__ctype_b_loc() = *addrOfClassInfoTable;
	*__ctype_tolower_loc() = *addrOfToLowerTable;
	*__ctype_toupper_loc() = *addrOfToUpperTable;
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


LocaleNumericDataBridge::LocaleNumericDataBridge(bool isGlobal)
	:
	posixLocaleConv(&gPosixLocaleConv),
	glibcNumericLocale(&glibcNumericLocaleData)
{

	memcpy(glibcNumericLocale, _NL_GLOBAL_DATA(GLIBC_LC_NUMERIC),
		sizeof(GlibcNumericLocale));

	if (isGlobal) {
		originalGlibcLocale = _NL_GLOBAL_DATA(GLIBC_LC_NUMERIC);
		_NL_GLOBAL_DATA(GLIBC_LC_NUMERIC) = (locale_data*)glibcNumericLocale;
	}
}


LocaleNumericDataBridge::~LocaleNumericDataBridge()
{
	if (isGlobal) {
		_NL_GLOBAL_DATA(GLIBC_LC_NUMERIC) = originalGlibcLocale;
	} else if (_NL_CURRENT_DATA(GLIBC_LC_NUMERIC) == (locale_data*)glibcNumericLocale) {
		_NL_CURRENT_DATA(GLIBC_LC_NUMERIC) = _NL_GLOBAL_DATA(GLIBC_LC_NUMERIC);
	}
}


void
LocaleNumericDataBridge::ApplyToCurrentThread()
{
	_NL_CURRENT_DATA(GLIBC_LC_NUMERIC) = (locale_data*)glibcNumericLocale;
}


LocaleTimeDataBridge::LocaleTimeDataBridge()
	:
	posixLCTimeInfo(&gPosixLCTimeInfo)
{
}


TimeConversionDataBridge::TimeConversionDataBridge(bool isGlobal)
	:
	localDaylight(daylight),
	localTimezone(timezone),
	isGlobal(isGlobal)
{
	if (isGlobal) {
		addrOfDaylight = &daylight;
		addrOfTimezone = &timezone;
		addrOfTZName = tzname;
	} else {
		addrOfDaylight = &localDaylight;
		addrOfTimezone = &localTimezone;
		addrOfTZName = localTZName;
		addrOfTZName[0] = localTZName0;
		addrOfTZName[1] = localTZName1;
		strlcpy(localTZName0, tzname[0], sizeof(localTZName0));
		strlcpy(localTZName1, tzname[1], sizeof(localTZName1));
	}
}


LocaleDataBridge::LocaleDataBridge(bool isGlobal)
	:
	ctypeDataBridge(isGlobal),
	numericDataBridge(isGlobal),
	timeConversionDataBridge(isGlobal),
	posixLanginfo(gPosixLanginfo),
	isGlobal(isGlobal)
{
}


void
LocaleDataBridge::ApplyToCurrentThread()
{
	ctypeDataBridge.ApplyToCurrentThread();
	numericDataBridge.ApplyToCurrentThread();
	// While timeConverstionDataBridge stores read-write variables,
	// these variables are global (by POSIX definition). Furthermore,
	// none of the backends seem to access these variables
	// directly. The values are set in the bridge mostly for
	// synchronization purposes. Therefore, don't call
	// ApplyToCurrentThread for this object.
}


}	// namespace Libroot
}	// namespace BPrivate
