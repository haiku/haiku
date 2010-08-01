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
#include <unicode/tmutfmt.h>
#include <unicode/utypes.h>
#include <ICUWrapper.h>


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


status_t BTimeUnitFormat::Format(int32 value, time_unit_element unit,
	BString* buffer, time_unit_style style) const
{
	if (buffer == NULL || unit < 0 || unit > B_TIME_UNIT_LAST
		|| (style != B_TIME_UNIT_ABBREVIATED && style != B_TIME_UNIT_FULL))
		return B_BAD_VALUE;

	UErrorCode icuStatus = U_ZERO_ERROR;
	TimeUnitFormat timeUnitFormatter(icuStatus);

	if (!U_SUCCESS(icuStatus))
		return B_ERROR;

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
	timeUnitFormatter.format(formattable, unicodeResult, pos, icuStatus);
	if (!U_SUCCESS(icuStatus))
		return B_ERROR;

	BStringByteSink byteSink(buffer);
	unicodeResult.toUTF8(byteSink);

	return B_OK;
}
