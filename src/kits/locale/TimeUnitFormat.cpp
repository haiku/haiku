/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Adrien Destugues <pulkomandy@gmail.com>
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include <TimeUnitFormat.h>

#include <new>

#include <unicode/format.h>
#include <unicode/locid.h>
#include <unicode/tmutfmt.h>
#include <unicode/utypes.h>
#include <ICUWrapper.h>

#include <Language.h>
#include <Locale.h>
#include <LocaleRoster.h>

// maps our unit element to the corresponding ICU unit
static const TimeUnit::UTimeUnitFields skUnitMap[] = {
	TimeUnit::UTIMEUNIT_YEAR,
	TimeUnit::UTIMEUNIT_MONTH,
	TimeUnit::UTIMEUNIT_WEEK,
	TimeUnit::UTIMEUNIT_DAY,
	TimeUnit::UTIMEUNIT_HOUR,
	TimeUnit::UTIMEUNIT_MINUTE,
	TimeUnit::UTIMEUNIT_SECOND,
};


BTimeUnitFormat::BTimeUnitFormat()
	:
	Inherited(),
	fFormatter(NULL)
{
	fInitStatus = SetLocale(fLocale);
}


BTimeUnitFormat::BTimeUnitFormat(const BTimeUnitFormat& other)
	:
	Inherited(other),
	fFormatter(other.fFormatter != NULL
		? new TimeUnitFormat(*other.fFormatter) : NULL)
{
	if (fFormatter == NULL && other.fFormatter != NULL)
		fInitStatus = B_NO_MEMORY;
}


BTimeUnitFormat::~BTimeUnitFormat()
{
	delete fFormatter;
}


BTimeUnitFormat&
BTimeUnitFormat::operator=(const BTimeUnitFormat& other)
{
	if (this == &other)
		return *this;

	Inherited::operator=(other);

	delete fFormatter;
	fFormatter = other.fFormatter != NULL
		? new TimeUnitFormat(*other.fFormatter) : NULL;

	if (fFormatter == NULL && other.fFormatter != NULL)
		fInitStatus = B_NO_MEMORY;

	return *this;
}


status_t
BTimeUnitFormat::SetLocale(const BLocale* locale)
{
	status_t result = Inherited::SetLocale(locale);
	if (result != B_OK)
		return result;

	BLanguage language;
	if (fLocale != NULL)
		fLocale->GetLanguage(&language);
	else
		BLocale::Default()->GetLanguage(&language);

	Locale icuLocale(language.Code());
	UErrorCode icuStatus = U_ZERO_ERROR;
	if (fFormatter == NULL) {
		fFormatter = new TimeUnitFormat(icuLocale, icuStatus);
		if (fFormatter == NULL)
			return B_NO_MEMORY;
	} else
		fFormatter->setLocale(icuLocale, icuStatus);
	if (!U_SUCCESS(icuStatus))
		return B_ERROR;

	return B_OK;
}


status_t
BTimeUnitFormat::Format(int32 value, time_unit_element unit,
	BString* buffer, time_unit_style style) const
{
	if (buffer == NULL || unit < 0 || unit > B_TIME_UNIT_LAST
		|| (style != B_TIME_UNIT_ABBREVIATED && style != B_TIME_UNIT_FULL))
		return B_BAD_VALUE;

	if (fFormatter == NULL)
		return B_NO_INIT;

	UErrorCode icuStatus = U_ZERO_ERROR;
	TimeUnitAmount* timeUnitAmount
		= new TimeUnitAmount((double)value, skUnitMap[unit], icuStatus);
	if (timeUnitAmount == NULL)
		return B_NO_MEMORY;
	if (!U_SUCCESS(icuStatus))
		return B_ERROR;

	Formattable formattable;
	formattable.adoptObject(timeUnitAmount);
	FieldPosition pos(FieldPosition::DONT_CARE);
	UnicodeString unicodeResult;
	fFormatter->format(formattable, unicodeResult, pos, icuStatus);
	if (!U_SUCCESS(icuStatus))
		return B_ERROR;

	BStringByteSink byteSink(buffer);
	unicodeResult.toUTF8(byteSink);

	return B_OK;
}
