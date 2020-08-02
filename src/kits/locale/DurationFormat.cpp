/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include <unicode/uversion.h>
#include <DurationFormat.h>

#include <new>

#include <unicode/gregocal.h>
#include <unicode/utypes.h>

#include <Locale.h>
#include <LocaleRoster.h>
#include <TimeZone.h>

#include <TimeZonePrivate.h>


U_NAMESPACE_USE


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


BDurationFormat::BDurationFormat(const BLanguage& language,
	const BFormattingConventions& conventions,
	const BString& separator, const time_unit_style style)
	:
	Inherited(language, conventions),
	fSeparator(separator),
	fTimeUnitFormat(language, conventions, style)
{
	UErrorCode icuStatus = U_ZERO_ERROR;
	fCalendar = new GregorianCalendar(icuStatus);
	if (fCalendar == NULL) {
		fInitStatus = B_NO_MEMORY;
		return;
	}
}


BDurationFormat::BDurationFormat(const BString& separator,
	const time_unit_style style)
	:
	Inherited(),
	fSeparator(separator),
	fTimeUnitFormat(style)
{
	UErrorCode icuStatus = U_ZERO_ERROR;
	fCalendar = new GregorianCalendar(icuStatus);
	if (fCalendar == NULL) {
		fInitStatus = B_NO_MEMORY;
		return;
	}
}


BDurationFormat::BDurationFormat(const BDurationFormat& other)
	:
	Inherited(other),
	fSeparator(other.fSeparator),
	fTimeUnitFormat(other.fTimeUnitFormat),
	fCalendar(other.fCalendar != NULL
		? new GregorianCalendar(*other.fCalendar) : NULL)
{
	if (fCalendar == NULL && other.fCalendar != NULL)
		fInitStatus = B_NO_MEMORY;
}


BDurationFormat::~BDurationFormat()
{
	delete fCalendar;
}


void
BDurationFormat::SetSeparator(const BString& separator)
{
	fSeparator = separator;
}


status_t
BDurationFormat::SetTimeZone(const BTimeZone* timeZone)
{
	if (fCalendar == NULL)
		return B_NO_INIT;

	BTimeZone::Private zonePrivate;
	if (timeZone == NULL) {
		BTimeZone defaultTimeZone;
		status_t result
			= BLocaleRoster::Default()->GetDefaultTimeZone(&defaultTimeZone);
		if (result != B_OK)
			return result;
		zonePrivate.SetTo(&defaultTimeZone);
	} else
		zonePrivate.SetTo(timeZone);

	TimeZone* icuTimeZone = zonePrivate.ICUTimeZone();
	if (icuTimeZone != NULL)
		fCalendar->setTimeZone(*icuTimeZone);

	return B_OK;
}


status_t
BDurationFormat::Format(BString& buffer, const bigtime_t startValue,
	const bigtime_t stopValue) const
{
	UErrorCode icuStatus = U_ZERO_ERROR;
	fCalendar->setTime((UDate)startValue / 1000, icuStatus);
	if (!U_SUCCESS(icuStatus))
		return B_ERROR;

	UDate stop = (UDate)stopValue / 1000;
	bool needSeparator = false;
	for (int unit = 0; unit <= B_TIME_UNIT_LAST; ++unit) {
		int delta
			= fCalendar->fieldDifference(stop, skUnitMap[unit], icuStatus);
		if (!U_SUCCESS(icuStatus))
			return B_ERROR;

		if (delta != 0) {
			if (needSeparator)
				buffer.Append(fSeparator);
			else
				needSeparator = true;
			status_t status = fTimeUnitFormat.Format(buffer, delta,
				(time_unit_element)unit);
			if (status != B_OK)
				return status;
		}
	}

	return B_OK;
}
