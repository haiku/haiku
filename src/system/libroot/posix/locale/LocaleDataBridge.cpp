/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */


#include "LocaleBackend.h"

#include <ctype.h>
#include <langinfo.h>

#include <PosixCtype.h>
#include <PosixLanginfo.h>
#include <PosixLCTimeInfo.h>
#include <PosixLocaleConv.h>


// struct used by glibc to access locale info
struct locale_data
{
	const char* name;
	const char* filedata;
	off_t filesize;
	int mmaped;
	unsigned int usage_count;
	int use_translit;
	const char *options;
	unsigned int nstrings;
	union locale_data_value
	{
		const uint32_t* wstr;
		const char* string;
		unsigned int word;
	}
	values[];
};
extern struct locale_data _nl_C_LC_NUMERIC;


namespace BPrivate {


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
	addrOfGlibcDecimalPoint(&_nl_C_LC_NUMERIC.values[0].string),
	addrOfGlibcThousandsSep(&_nl_C_LC_NUMERIC.values[1].string),
	addrOfGlibcGrouping(&_nl_C_LC_NUMERIC.values[2].string),
	addrOfGlibcWCDecimalPoint(&_nl_C_LC_NUMERIC.values[3].word),
	addrOfGlibcWCThousandsSep(&_nl_C_LC_NUMERIC.values[4].word),
	posixLocaleConv(&gPosixLocaleConv)
{
}


LocaleTimeDataBridge::LocaleTimeDataBridge()
	:
	posixLCTimeInfo(&gPosixLCTimeInfo)
{
}


LocaleDataBridge::LocaleDataBridge()
	:
	posixLanginfo(gPosixLanginfo)
{
}


}	// namespace BPrivate
