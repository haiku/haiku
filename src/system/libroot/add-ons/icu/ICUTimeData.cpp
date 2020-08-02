/*
 * Copyright 2010-2011, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */


#include "ICUTimeData.h"

#include <langinfo.h>
#include <string.h>
#include <strings.h>

#include <unicode/dtfmtsym.h>
#include <unicode/gregocal.h>
#include <unicode/smpdtfmt.h>

#include <AutoDeleter.h>

#include "ICUMessagesData.h"


U_NAMESPACE_USE


namespace BPrivate {
namespace Libroot {


ICUTimeData::ICUTimeData(pthread_key_t tlsKey, struct lc_time_t& lcTimeInfo,
	const ICUMessagesData& messagesData)
	:
	inherited(tlsKey),
	fLCTimeInfo(lcTimeInfo),
	fDataBridge(NULL),
	fMessagesData(messagesData)
{
	for (int i = 0; i < 12; ++i) {
		fLCTimeInfo.mon[i] = fMon[i];
		fLCTimeInfo.month[i] = fMonth[i];
		fLCTimeInfo.alt_month[i] = fAltMonth[i];
	}
	for (int i = 0; i < 7; ++i) {
		fLCTimeInfo.wday[i] = fWday[i];
		fLCTimeInfo.weekday[i] = fWeekday[i];
	}
	fLCTimeInfo.X_fmt = fTimeFormat;
	fLCTimeInfo.x_fmt = fDateFormat;
	fLCTimeInfo.c_fmt = fDateTimeFormat;
	fLCTimeInfo.am = fAm;
	fLCTimeInfo.pm = fPm;
	fLCTimeInfo.date_fmt = fDateTimeZoneFormat;
	fLCTimeInfo.md_order = fMonthDayOrder;
	fLCTimeInfo.ampm_fmt = fAmPmFormat;
}


ICUTimeData::~ICUTimeData()
{
}


void
ICUTimeData::Initialize(LocaleTimeDataBridge* dataBridge)
{
	fDataBridge = dataBridge;
}


status_t
ICUTimeData::SetTo(const Locale& locale, const char* posixLocaleName)
{
	status_t result = inherited::SetTo(locale, posixLocaleName);
	if (result != B_OK)
		return result;

	UErrorCode icuStatus = U_ZERO_ERROR;
	DateFormatSymbols formatSymbols(ICULocaleForStrings(), icuStatus);
	if (!U_SUCCESS(icuStatus))
		return B_UNSUPPORTED;

	int count = 0;
	const UnicodeString* strings = formatSymbols.getShortMonths(count);
	result = _SetLCTimeEntries(strings, fMon[0], sizeof(fMon[0]), count, 12);

	if (result == B_OK) {
		strings = formatSymbols.getMonths(count);
		result = _SetLCTimeEntries(strings, fMonth[0], sizeof(fMonth[0]), count,
			12);
	}

	if (result == B_OK) {
		strings = formatSymbols.getShortWeekdays(count);
		if (count == 8 && strings[0].length() == 0) {
			// ICUs weekday arrays are 1-based
			strings++;
			count = 7;
		}
		result
			= _SetLCTimeEntries(strings, fWday[0], sizeof(fWday[0]), count, 7);
	}

	if (result == B_OK) {
		strings = formatSymbols.getWeekdays(count);
		if (count == 8 && strings[0].length() == 0) {
			// ICUs weekday arrays are 1-based
			strings++;
			count = 7;
		}
		result = _SetLCTimeEntries(strings, fWeekday[0], sizeof(fWeekday[0]),
			count, 7);
	}

	if (result == B_OK) {
		try {
			DateFormat* format = DateFormat::createTimeInstance(
				DateFormat::kDefault, fLocale);
			result = _SetLCTimePattern(format, fTimeFormat, sizeof(fTimeFormat));
			delete format;
		} catch(...) {
			result = B_NO_MEMORY;
		}
	}

	if (result == B_OK) {
		try {
			DateFormat* format = DateFormat::createDateInstance(
				DateFormat::kDefault, fLocale);
			result = _SetLCTimePattern(format, fDateFormat, sizeof(fDateFormat));
			delete format;
		} catch(...) {
			result = B_NO_MEMORY;
		}
	}

	if (result == B_OK) {
		try {
			DateFormat* format = DateFormat::createDateTimeInstance(
				DateFormat::kFull, DateFormat::kFull, fLocale);
			result = _SetLCTimePattern(format, fDateTimeFormat,
				sizeof(fDateTimeFormat));
			delete format;
		} catch(...) {
			result = B_NO_MEMORY;
		}
	}

	if (result == B_OK) {
		strings = formatSymbols.getAmPmStrings(count);
		result = _SetLCTimeEntries(strings, fAm, sizeof(fAm), 1, 1);
		if (result == B_OK)
			result = _SetLCTimeEntries(&strings[1], fPm, sizeof(fPm), 1, 1);
	}

	if (result == B_OK) {
		strings = formatSymbols.getMonths(count, DateFormatSymbols::STANDALONE,
			DateFormatSymbols::WIDE);
		result = _SetLCTimeEntries(strings, fAltMonth[0], sizeof(fAltMonth[0]),
			count, 12);
	}

	strcpy(fAmPmFormat, fDataBridge->posixLCTimeInfo->ampm_fmt);
		// ICU does not provide anything for this (and that makes sense, too)

	return result;
}


status_t
ICUTimeData::SetToPosix()
{
	status_t result = inherited::SetToPosix();

	if (result == B_OK) {
		for (int i = 0; i < 12; ++i) {
			strcpy(fMon[i], fDataBridge->posixLCTimeInfo->mon[i]);
			strcpy(fMonth[i], fDataBridge->posixLCTimeInfo->month[i]);
			strcpy(fAltMonth[i], fDataBridge->posixLCTimeInfo->alt_month[i]);
		}
		for (int i = 0; i < 7; ++i) {
			strcpy(fWday[i], fDataBridge->posixLCTimeInfo->wday[i]);
			strcpy(fWeekday[i], fDataBridge->posixLCTimeInfo->weekday[i]);
		}
		strcpy(fTimeFormat, fDataBridge->posixLCTimeInfo->X_fmt);
		strcpy(fDateFormat, fDataBridge->posixLCTimeInfo->x_fmt);
		strcpy(fDateTimeFormat, fDataBridge->posixLCTimeInfo->c_fmt);
		strcpy(fAm, fDataBridge->posixLCTimeInfo->am);
		strcpy(fPm, fDataBridge->posixLCTimeInfo->pm);
		strcpy(fDateTimeZoneFormat, fDataBridge->posixLCTimeInfo->date_fmt);
		strcpy(fMonthDayOrder, fDataBridge->posixLCTimeInfo->md_order);
		strcpy(fAmPmFormat, fDataBridge->posixLCTimeInfo->ampm_fmt);
	}

	return result;
}


const char*
ICUTimeData::GetLanginfo(int index)
{
	switch(index) {
		case D_T_FMT:
			return fDateTimeFormat;
		case D_FMT:
			return fDateFormat;
		case T_FMT:
			return fTimeFormat;
		case T_FMT_AMPM:
			return fAmPmFormat;
		case AM_STR:
			return fAm;
		case PM_STR:
			return fPm;

		case DAY_1:
		case DAY_2:
		case DAY_3:
		case DAY_4:
		case DAY_5:
		case DAY_6:
		case DAY_7:
			return fWeekday[index - DAY_1];

		case ABDAY_1:
		case ABDAY_2:
		case ABDAY_3:
		case ABDAY_4:
		case ABDAY_5:
		case ABDAY_6:
		case ABDAY_7:
			return fWday[index - ABDAY_1];

		case MON_1:
		case MON_2:
		case MON_3:
		case MON_4:
		case MON_5:
		case MON_6:
		case MON_7:
		case MON_8:
		case MON_9:
		case MON_10:
		case MON_11:
		case MON_12:
			return fMonth[index - MON_1];

		case ABMON_1:
		case ABMON_2:
		case ABMON_3:
		case ABMON_4:
		case ABMON_5:
		case ABMON_6:
		case ABMON_7:
		case ABMON_8:
		case ABMON_9:
		case ABMON_10:
		case ABMON_11:
		case ABMON_12:
			return fMon[index - ABMON_1];

		default:
			return "";
	}
}


const Locale&
ICUTimeData::ICULocaleForStrings() const
{
	// check if the date strings should be taken from the messages-locale
	// or from the time-locale (default)
	UErrorCode icuStatus = U_ZERO_ERROR;
	char stringsValue[16];
	fLocale.getKeywordValue("strings", stringsValue, sizeof(stringsValue),
		icuStatus);
	if (U_SUCCESS(icuStatus) && strcasecmp(stringsValue, "messages") == 0)
		return fMessagesData.ICULocale();
	else
		return fLocale;
}


status_t
ICUTimeData::_SetLCTimeEntries(const UnicodeString* strings, char* destination,
	int entrySize, int count, int maxCount)
{
	if (strings == NULL)
		return B_ERROR;

	status_t result = B_OK;
	if (count > maxCount)
		count = maxCount;
	for (int32 i = 0; result == B_OK && i < count; ++i) {
		result = _ConvertUnicodeStringToLocaleconvEntry(strings[i], destination,
			entrySize);
		destination += entrySize;
	}

	return result;
}


status_t
ICUTimeData::_SetLCTimePattern(DateFormat* format, char* destination,
	int destinationSize)
{
	SimpleDateFormat* simpleFormat = dynamic_cast<SimpleDateFormat*>(format);
	if (!simpleFormat)
		return B_BAD_TYPE;

	// convert ICU-type pattern to posix (i.e. strftime()) format string
	UnicodeString icuPattern;
	simpleFormat->toPattern(icuPattern);
	UnicodeString posixPattern;
	if (icuPattern.length() > 0) {
		UChar lastCharSeen = 0;
		int lastCharCount = 1;
		bool inSingleQuotes = false;
		bool inDoubleQuotes = false;
		// we loop one character past the end on purpose, which will result in a
		// final -1 char to be processed, which in turn will let us handle the
		// last character (via lastCharSeen)
		for (int i = 0; i <= icuPattern.length(); ++i) {
			UChar currChar = icuPattern.charAt(i);
			if (lastCharSeen != 0 && currChar == lastCharSeen) {
				lastCharCount++;
				continue;
			}

			if (!inSingleQuotes && !inDoubleQuotes) {
				switch (lastCharSeen) {
					case L'a':
						posixPattern.append(UnicodeString("%p", ""));
						break;
					case L'd':
						if (lastCharCount == 2)
							posixPattern.append(UnicodeString("%d", ""));
						else
							posixPattern.append(UnicodeString("%e", ""));
						break;
					case L'D':
						posixPattern.append(UnicodeString("%j", ""));
						break;
					case L'c':
						// fall through, to handle 'c' the same as 'e'
					case L'e':
						if (lastCharCount == 4)
							posixPattern.append(UnicodeString("%A", ""));
						else if (lastCharCount <= 2)
							posixPattern.append(UnicodeString("%u", ""));
						else
							posixPattern.append(UnicodeString("%a", ""));
						break;
					case L'E':
						if (lastCharCount == 4)
							posixPattern.append(UnicodeString("%A", ""));
						else
							posixPattern.append(UnicodeString("%a", ""));
						break;
					case L'k':
						// fall through, to handle 'k' the same as 'h'
					case L'h':
						if (lastCharCount == 2)
							posixPattern.append(UnicodeString("%I", ""));
						else
							posixPattern.append(UnicodeString("%l", ""));
						break;
					case L'H':
						if (lastCharCount == 2)
							posixPattern.append(UnicodeString("%H", ""));
						else
							posixPattern.append(UnicodeString("%k", ""));
						break;
					case L'm':
						posixPattern.append(UnicodeString("%M", ""));
						break;
					case L'L':
						// fall through, to handle 'L' the same as 'M'
					case L'M':
						if (lastCharCount == 4)
							posixPattern.append(UnicodeString("%B", ""));
						else if (lastCharCount == 3)
							posixPattern.append(UnicodeString("%b", ""));
						else
							posixPattern.append(UnicodeString("%m", ""));
						break;
					case L's':
						posixPattern.append(UnicodeString("%S", ""));
						break;
					case L'w':
						posixPattern.append(UnicodeString("%V", ""));
						break;
					case L'y':
						if (lastCharCount == 2)
							posixPattern.append(UnicodeString("%y", ""));
						else
							posixPattern.append(UnicodeString("%Y", ""));
						break;
					case L'Y':
						posixPattern.append(UnicodeString("%G", ""));
						break;
					case L'z':
						posixPattern.append(UnicodeString("%Z", ""));
						break;
					case L'Z':
						posixPattern.append(UnicodeString("%z", ""));
						break;
					default:
						if (lastCharSeen != 0)
							posixPattern.append(lastCharSeen);
				}
			} else {
				if (lastCharSeen != 0)
					posixPattern.append(lastCharSeen);
			}

			if (currChar == L'"') {
				inDoubleQuotes = !inDoubleQuotes;
				lastCharSeen = 0;
			} else if (currChar == L'\'') {
				inSingleQuotes = !inSingleQuotes;
				lastCharSeen = 0;
			} else
				lastCharSeen = currChar;

			lastCharCount = 1;
		}
	}

	return _ConvertUnicodeStringToLocaleconvEntry(posixPattern, destination,
		destinationSize);
}


}	// namespace Libroot
}	// namespace BPrivate
