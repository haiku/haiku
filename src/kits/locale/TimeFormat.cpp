/* 
 * Copyright 2010, Adrien Destugues, pulkomandy@gmail.com
 * Distributed under the terms of the MIT License.
 */

#include <TimeFormat.h>

#include <unicode/format.h>
#include <unicode/tmutfmt.h>
#include <unicode/utypes.h>
#include <ICUWrapper.h>

#define ICU_VERSION icu_44

status_t BTimeFormat::Format(int64 number, BString* buffer) const
{
	// create time unit amount instance - a combination of Number and time unit
	UErrorCode status = U_ZERO_ERROR;

	int64 days, hours, minutes, seconds, remainder;

	days = number / (24 * 3600);
	remainder = number % (24 * 3600);

	hours = remainder / 3600;
	remainder %= 3600;

	minutes = remainder / 60;
	remainder %= 60;

	seconds = remainder;
	
	TimeUnitFormat* format = new TimeUnitFormat(status);
	UnicodeString formatted;
	Formattable formattable;
	BStringByteSink bbs(buffer);

	if (!U_SUCCESS(status)) {
		delete format;
		return B_ERROR;
	}

	if (days) {
		TimeUnitAmount* daysAmount = new TimeUnitAmount(days,
			TimeUnit::UTIMEUNIT_DAY, status);

		formattable.adoptObject(daysAmount);
		formatted = ((ICU_VERSION::Format*)format)->format(formattable, formatted,
			status);
	}

	if (hours) {
		TimeUnitAmount* hoursAmount = new TimeUnitAmount(hours,
			TimeUnit::UTIMEUNIT_HOUR, status);

		formattable.adoptObject(hoursAmount);
		if (days)
			formatted.append(", ");
		formatted = ((ICU_VERSION::Format*)format)->format(formattable, formatted,
			status);
	}

	if (minutes) {
		TimeUnitAmount* minutesAmount = new TimeUnitAmount(minutes,
			TimeUnit::UTIMEUNIT_MINUTE, status);

		formattable.adoptObject(minutesAmount);
		if (days || hours)
			formatted.append(", ");
		formatted = ((ICU_VERSION::Format*)format)->format(formattable, formatted,
			status);
	}


	if (seconds || (minutes == 0 && hours == 0 && days == 0)) {
		TimeUnitAmount* secondsAmount = new TimeUnitAmount(seconds,
			TimeUnit::UTIMEUNIT_SECOND, status);

		formattable.adoptObject(secondsAmount);
		if (days || hours || minutes)
			formatted.append(", ");
		formatted = ((ICU_VERSION::Format*)format)->format(formattable, formatted,
			status);
	}
	formatted.toUTF8(bbs);


	delete format;
	return B_OK;
}
