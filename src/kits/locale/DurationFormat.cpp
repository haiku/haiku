/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include <DurationFormat.h>

#include <new>

#include <unicode/calendar.h>
#include <unicode/tmunit.h>
#include <unicode/utypes.h>

#include <AutoDeleter.h>
#include <TimeUnitFormat.h>


using BPrivate::ObjectDeleter;


// maps our unit element to the corresponding ICU unit
static const UCalendarDateFields skUnitMap[] = {
	UCAL_YEAR,
	UCAL_MONTH,
	UCAL_WEEK_OF_MONTH,
	UCAL_DAY_OF_WEEK,
	UCAL_HOUR_OF_DAY,
	UCAL_MINUTE,
	UCAL_SECOND,
};


status_t BDurationFormat::Format(bigtime_t startValue, bigtime_t stopValue,
	BString* buffer, time_unit_style style, const BString& separator) const
{
	if (buffer == NULL)
		return B_BAD_VALUE;

	UErrorCode icuStatus = U_ZERO_ERROR;
	ObjectDeleter<Calendar> calendar = Calendar::createInstance(icuStatus);
	if (calendar.Get() == NULL)
		return B_NO_MEMORY;
	if (!U_SUCCESS(icuStatus))
		return B_ERROR;

	calendar->setTime((UDate)startValue / 1000, icuStatus);
	if (!U_SUCCESS(icuStatus))
		return B_ERROR;

	BTimeUnitFormat timeUnitFormat;
	UDate stop = (UDate)stopValue / 1000;
	bool needSeparator = false;
	for (int unit = 0; unit <= B_TIME_UNIT_LAST; ++unit) {
		int delta = calendar->fieldDifference(stop, skUnitMap[unit], icuStatus);
		if (!U_SUCCESS(icuStatus))
			return B_ERROR;

		if (delta != 0) {
			if (needSeparator)
				buffer->Append(separator);
			else
				needSeparator = true;
			status_t status = timeUnitFormat.Format(delta,
				(time_unit_element)unit, buffer, style);
			if (status != B_OK)
				return status;
		}
	}

	return B_OK;
}
