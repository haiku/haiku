/*
 * Copyright 2010-2014, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */

#include <unicode/uversion.h>
#include <TimeFormat.h>

#include <AutoDeleter.h>
#include <Autolock.h>
#include <DateTime.h>
#include <FormattingConventionsPrivate.h>
#include <LanguagePrivate.h>
#include <TimeZone.h>

#include <ICUWrapper.h>

#include <unicode/datefmt.h>
#include <unicode/smpdtfmt.h>

#include <vector>


U_NAMESPACE_USE


BTimeFormat::BTimeFormat()
	: BFormat()
{
}


BTimeFormat::BTimeFormat(const BLanguage& language,
	const BFormattingConventions& conventions)
	: BFormat(language, conventions)
{
}


BTimeFormat::BTimeFormat(const BTimeFormat &other)
	: BFormat(other)
{
}


BTimeFormat::~BTimeFormat()
{
}


void
BTimeFormat::SetTimeFormat(BTimeFormatStyle style,
	const BString& format)
{
	fConventions.SetExplicitTimeFormat(style, format);
}


// #pragma mark - Formatting


ssize_t
BTimeFormat::Format(char* string, size_t maxSize, time_t time,
	BTimeFormatStyle style) const
{
	ObjectDeleter<DateFormat> timeFormatter(_CreateTimeFormatter(style));
	if (timeFormatter.Get() == NULL)
		return B_NO_MEMORY;

	UnicodeString icuString;
	timeFormatter->format((UDate)time * 1000, icuString);

	CheckedArrayByteSink stringConverter(string, maxSize);
	icuString.toUTF8(stringConverter);

	if (stringConverter.Overflowed())
		return B_BAD_VALUE;

	return stringConverter.NumberOfBytesWritten();
}


status_t
BTimeFormat::Format(BString& string, const time_t time,
	const BTimeFormatStyle style, const BTimeZone* timeZone) const
{
	ObjectDeleter<DateFormat> timeFormatter(_CreateTimeFormatter(style));
	if (timeFormatter.Get() == NULL)
		return B_NO_MEMORY;

	if (timeZone != NULL) {
		ObjectDeleter<TimeZone> icuTimeZone(
			TimeZone::createTimeZone(timeZone->ID().String()));
		if (icuTimeZone.Get() == NULL)
			return B_NO_MEMORY;
		timeFormatter->setTimeZone(*icuTimeZone.Get());
	}

	UnicodeString icuString;
	timeFormatter->format((UDate)time * 1000, icuString);

	string.Truncate(0);
	BStringByteSink stringConverter(&string);
	icuString.toUTF8(stringConverter);

	return B_OK;
}


status_t
BTimeFormat::Format(BString& string, int*& fieldPositions, int& fieldCount,
	time_t time, BTimeFormatStyle style) const
{
	ObjectDeleter<DateFormat> timeFormatter(_CreateTimeFormatter(style));
	if (timeFormatter.Get() == NULL)
		return B_NO_MEMORY;

	fieldPositions = NULL;
	UErrorCode error = U_ZERO_ERROR;
	icu::FieldPositionIterator positionIterator;
	UnicodeString icuString;
	timeFormatter->format((UDate)time * 1000, icuString, &positionIterator,
		error);

	if (error != U_ZERO_ERROR)
		return B_BAD_VALUE;

	icu::FieldPosition field;
	std::vector<int> fieldPosStorage;
	fieldCount  = 0;
	while (positionIterator.next(field)) {
		fieldPosStorage.push_back(field.getBeginIndex());
		fieldPosStorage.push_back(field.getEndIndex());
		fieldCount += 2;
	}

	fieldPositions = (int*) malloc(fieldCount * sizeof(int));

	for (int i = 0 ; i < fieldCount ; i++ )
		fieldPositions[i] = fieldPosStorage[i];

	string.Truncate(0);
	BStringByteSink stringConverter(&string);
	icuString.toUTF8(stringConverter);

	return B_OK;
}


status_t
BTimeFormat::GetTimeFields(BDateElement*& fields, int& fieldCount,
	BTimeFormatStyle style) const
{
	ObjectDeleter<DateFormat> timeFormatter(_CreateTimeFormatter(style));
	if (timeFormatter.Get() == NULL)
		return B_NO_MEMORY;

	fields = NULL;
	UErrorCode error = U_ZERO_ERROR;
	icu::FieldPositionIterator positionIterator;
	UnicodeString icuString;
	time_t now;
	timeFormatter->format((UDate)time(&now) * 1000, icuString,
		&positionIterator, error);

	if (error != U_ZERO_ERROR)
		return B_BAD_VALUE;

	icu::FieldPosition field;
	std::vector<int> fieldPosStorage;
	fieldCount  = 0;
	while (positionIterator.next(field)) {
		fieldPosStorage.push_back(field.getField());
		fieldCount ++;
	}

	fields = (BDateElement*) malloc(fieldCount * sizeof(BDateElement));

	for (int i = 0 ; i < fieldCount ; i++ ) {
		switch (fieldPosStorage[i]) {
			case UDAT_HOUR_OF_DAY1_FIELD:
			case UDAT_HOUR_OF_DAY0_FIELD:
			case UDAT_HOUR1_FIELD:
			case UDAT_HOUR0_FIELD:
				fields[i] = B_DATE_ELEMENT_HOUR;
				break;
			case UDAT_MINUTE_FIELD:
				fields[i] = B_DATE_ELEMENT_MINUTE;
				break;
			case UDAT_SECOND_FIELD:
				fields[i] = B_DATE_ELEMENT_SECOND;
				break;
			case UDAT_AM_PM_FIELD:
				fields[i] = B_DATE_ELEMENT_AM_PM;
				break;
			default:
				fields[i] = B_DATE_ELEMENT_INVALID;
				break;
		}
	}

	return B_OK;
}


status_t
BTimeFormat::Parse(BString source, BTimeFormatStyle style, BTime& output)
{
	ObjectDeleter<DateFormat> timeFormatter(_CreateTimeFormatter(style));
	if (timeFormatter.Get() == NULL)
		return B_NO_MEMORY;

	// If no timezone is specified in the time string, assume GMT
	timeFormatter->setTimeZone(*icu::TimeZone::getGMT());

	ParsePosition p(0);
	UDate date = timeFormatter->parse(UnicodeString::fromUTF8(source.String()),
		p);

	output.SetTime(0, 0, 0);
	output.AddMilliseconds(date);

	return B_OK;
}


DateFormat*
BTimeFormat::_CreateTimeFormatter(const BTimeFormatStyle style) const
{
	Locale* icuLocale
		= fConventions.UseStringsFromPreferredLanguage()
			? BLanguage::Private(&fLanguage).ICULocale()
			: BFormattingConventions::Private(&fConventions).ICULocale();

	icu::DateFormat* timeFormatter
		= icu::DateFormat::createTimeInstance(DateFormat::kShort, *icuLocale);
	if (timeFormatter == NULL)
		return NULL;

	SimpleDateFormat* timeFormatterImpl
		= static_cast<SimpleDateFormat*>(timeFormatter);

	BString format;
	fConventions.GetTimeFormat(style, format);

	UnicodeString pattern(format.String());
	timeFormatterImpl->applyPattern(pattern);

	return timeFormatter;
}
