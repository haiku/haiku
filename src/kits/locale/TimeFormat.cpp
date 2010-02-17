/* 
 * Copyright 2010, Adrien Destugues, pulkomandy@gmail.com
 * Distributed under the terms of the MIT License.
 */

#include <TimeFormat.h>

#include <unicode/format.h>
#include <unicode/tmutfmt.h>
#include <unicode/utypes.h>
#include <ICUWrapper.h>

status_t BTimeFormat::Format(int64 number, BString* buffer) const
{
	// create time unit amount instance - a combination of Number and time unit
	UErrorCode status = U_ZERO_ERROR;
	TimeUnitAmount* source = new TimeUnitAmount(number/1000000, TimeUnit::UTIMEUNIT_SECOND, status);
	// create time unit format instance
	TimeUnitFormat* format = new TimeUnitFormat(status);
	// format a time unit amount
	UnicodeString formatted;
	Formattable formattable(source);
	if (!U_SUCCESS(status)) {
		delete source;
		delete format;
		return B_ERROR;
	}

	formatted = ((icu_4_2::Format*)format)->format(formattable, formatted, status);

	BStringByteSink bbs(buffer);
	formatted.toUTF8(bbs);

	delete source;
	delete format;
	return B_OK;
}
