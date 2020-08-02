/*
 * Copyright 2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Akshay Agarwal <agarwal.akshay.akshay8@gmail.com>
 */


#include <unicode/uversion.h>
#include <RelativeDateTimeFormat.h>

#include <stdlib.h>
#include <time.h>

#include <unicode/gregocal.h>
#include <unicode/reldatefmt.h>
#include <unicode/utypes.h>

#include <ICUWrapper.h>

#include <Language.h>
#include <Locale.h>
#include <LocaleRoster.h>
#include <TimeUnitFormat.h>


U_NAMESPACE_USE


static const URelativeDateTimeUnit kTimeUnitToRelativeDateTime[] = {
	UDAT_REL_UNIT_YEAR,
	UDAT_REL_UNIT_MONTH,
	UDAT_REL_UNIT_WEEK,
	UDAT_REL_UNIT_DAY,
	UDAT_REL_UNIT_HOUR,
	UDAT_REL_UNIT_MINUTE,
	UDAT_REL_UNIT_SECOND,
};


static const UCalendarDateFields kTimeUnitToICUDateField[] = {
	UCAL_YEAR,
	UCAL_MONTH,
	UCAL_WEEK_OF_MONTH,
	UCAL_DAY_OF_WEEK,
	UCAL_HOUR_OF_DAY,
	UCAL_MINUTE,
	UCAL_SECOND,
};


BRelativeDateTimeFormat::BRelativeDateTimeFormat()
	: Inherited()
{
	Locale icuLocale(fLanguage.Code());
	UErrorCode icuStatus = U_ZERO_ERROR;

	fFormatter = new RelativeDateTimeFormatter(icuLocale, icuStatus);
	if (fFormatter == NULL) {
		fInitStatus = B_NO_MEMORY;
		return;
	}

	fCalendar = new GregorianCalendar(icuStatus);
	if (fCalendar == NULL) {
		fInitStatus = B_NO_MEMORY;
		return;
	}

	if (!U_SUCCESS(icuStatus))
		fInitStatus = B_ERROR;
}


BRelativeDateTimeFormat::BRelativeDateTimeFormat(const BLanguage& language,
	const BFormattingConventions& conventions)
	: Inherited(language, conventions)
{
	Locale icuLocale(fLanguage.Code());
	UErrorCode icuStatus = U_ZERO_ERROR;

	fFormatter = new RelativeDateTimeFormatter(icuLocale, icuStatus);
	if (fFormatter == NULL) {
		fInitStatus = B_NO_MEMORY;
		return;
	}

	fCalendar = new GregorianCalendar(icuStatus);
	if (fCalendar == NULL) {
		fInitStatus = B_NO_MEMORY;
		return;
	}

	if (!U_SUCCESS(icuStatus))
		fInitStatus = B_ERROR;
}


BRelativeDateTimeFormat::BRelativeDateTimeFormat(const BRelativeDateTimeFormat& other)
	: Inherited(other),
	fFormatter(other.fFormatter != NULL
		? new RelativeDateTimeFormatter(*other.fFormatter) : NULL),
	fCalendar(other.fCalendar != NULL
		? new GregorianCalendar(*other.fCalendar) : NULL)
{
	if ((fFormatter == NULL && other.fFormatter != NULL)
		|| (fCalendar == NULL && other.fCalendar != NULL))
		fInitStatus = B_NO_MEMORY;
}


BRelativeDateTimeFormat::~BRelativeDateTimeFormat()
{
	delete fFormatter;
	delete fCalendar;
}


status_t
BRelativeDateTimeFormat::Format(BString& string,
	const time_t timeValue) const
{
	if (fFormatter == NULL)
		return B_NO_INIT;

	time_t currentTime = time(NULL);

	UErrorCode icuStatus = U_ZERO_ERROR;
	fCalendar->setTime((UDate)currentTime * 1000, icuStatus);
	if (!U_SUCCESS(icuStatus))
		return B_ERROR;

	UDate UTimeValue = (UDate)timeValue * 1000;

	int delta = 0;
	int offset = 1;
	URelativeDateTimeUnit unit = UDAT_REL_UNIT_SECOND;

	for (int timeUnit = 0; timeUnit <= B_TIME_UNIT_LAST; ++timeUnit) {
		delta = fCalendar->fieldDifference(UTimeValue,
			kTimeUnitToICUDateField[timeUnit], icuStatus);

		if (!U_SUCCESS(icuStatus))
			return B_ERROR;

		if (abs(delta) >= offset) {
			unit = kTimeUnitToRelativeDateTime[timeUnit];
			break;
		}
	}

	UnicodeString unicodeResult;

	// Note: icu::RelativeDateTimeFormatter::formatNumeric() is a part of ICU Draft API
	// and may be changed in the future versions and was introduced in ICU 57.
	fFormatter->formatNumeric(delta, unit, unicodeResult, icuStatus);

	if (!U_SUCCESS(icuStatus))
		return B_ERROR;

	BStringByteSink byteSink(&string);
	unicodeResult.toUTF8(byteSink);

	return B_OK;
}
