/*
 * Copyright 2010-2014, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 *		Adrien Desutugues <pulkomandy@pulkomandy.tk>
 */

#include <DateFormat.h>

#include <AutoDeleter.h>
#include <Autolock.h>
#include <FormattingConventionsPrivate.h>
#include <LanguagePrivate.h>
#include <Locale.h>
#include <LocaleRoster.h>
#include <TimeZone.h>

#include <ICUWrapper.h>

#include <unicode/datefmt.h>
#include <unicode/smpdtfmt.h>

#include <vector>


BDateFormat::BDateFormat(const BLanguage* const language,
	const BFormattingConventions* const conventions)
{
	if (conventions != NULL)
		fConventions = *conventions;
	else
		BLocale::Default()->GetFormattingConventions(&fConventions);

	if (language != NULL)
		fLanguage = *language;
	else
		BLocale::Default()->GetLanguage(&fLanguage);
}


BDateFormat::BDateFormat(const BDateFormat &other)
	: fConventions(other.fConventions),
	fLanguage(other.fLanguage)
{
}


/*static*/ const BDateFormat*
BDateFormat::Default()
{
	return BLocaleRoster::Default()->GetDefaultDateFormat();
}


BDateFormat::~BDateFormat()
{
}


void
BDateFormat::SetFormattingConventions(const BFormattingConventions& conventions)
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return;

	fConventions = conventions;
}


void
BDateFormat::SetLanguage(const BLanguage& newLanguage)
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return;

	fLanguage = newLanguage;
}


status_t
BDateFormat::GetDateFormat(BDateFormatStyle style,
	BString& outFormat) const
{
	return fConventions.GetDateFormat(style, outFormat);
}


void
BDateFormat::SetDateFormat(BDateFormatStyle style,
	const BString& format)
{
	fConventions.SetExplicitDateFormat(style, format);
}


ssize_t
BDateFormat::Format(char* string, const size_t maxSize, const time_t time,
	const BDateFormatStyle style) const
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	BString format;
	fConventions.GetDateFormat(style, format);
	ObjectDeleter<DateFormat> dateFormatter(_CreateDateFormatter(format));
	if (dateFormatter.Get() == NULL)
		return B_NO_MEMORY;

	UnicodeString icuString;
	dateFormatter->format((UDate)time * 1000, icuString);

	CheckedArrayByteSink stringConverter(string, maxSize);
	icuString.toUTF8(stringConverter);

	if (stringConverter.Overflowed())
		return B_BAD_VALUE;

	return stringConverter.NumberOfBytesWritten();
}


status_t
BDateFormat::Format(BString& string, const time_t time,
	const BDateFormatStyle style, const BTimeZone* timeZone) const
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	BString format;
	fConventions.GetDateFormat(style, format);
	ObjectDeleter<DateFormat> dateFormatter(_CreateDateFormatter(format));
	if (dateFormatter.Get() == NULL)
		return B_NO_MEMORY;

	if (timeZone != NULL) {
		ObjectDeleter<TimeZone> icuTimeZone(
			TimeZone::createTimeZone(timeZone->ID().String()));
		if (icuTimeZone.Get() == NULL)
			return B_NO_MEMORY;
		dateFormatter->setTimeZone(*icuTimeZone.Get());
	}

	UnicodeString icuString;
	dateFormatter->format((UDate)time * 1000, icuString);

	string.Truncate(0);
	BStringByteSink stringConverter(&string);
	icuString.toUTF8(stringConverter);

	return B_OK;
}


status_t
BDateFormat::Format(BString& string, const BDate& time,
	const BDateFormatStyle style, const BTimeZone* timeZone) const
{
	if (!time.IsValid())
		return B_BAD_DATA;

	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	BString format;
	fConventions.GetDateFormat(style, format);
	ObjectDeleter<DateFormat> dateFormatter(_CreateDateFormatter(format));
	if (dateFormatter.Get() == NULL)
		return B_NO_MEMORY;

	UErrorCode err = U_ZERO_ERROR;
	ObjectDeleter<Calendar> calendar(Calendar::createInstance(err));
	if (!U_SUCCESS(err))
		return B_NO_MEMORY;

	if (timeZone != NULL) {
		ObjectDeleter<TimeZone> icuTimeZone(
			TimeZone::createTimeZone(timeZone->ID().String()));
		if (icuTimeZone.Get() == NULL)
			return B_NO_MEMORY;
		dateFormatter->setTimeZone(*icuTimeZone.Get());
		calendar->setTimeZone(*icuTimeZone.Get());
	}

	// Note ICU calendar uses months in range 0..11, while we use the more
	// natural 1..12 in BDate.
	calendar->set(time.Year(), time.Month() - 1, time.Day());

	UnicodeString icuString;
	FieldPosition p;
	dateFormatter->format(*calendar.Get(), icuString, p);

	string.Truncate(0);
	BStringByteSink stringConverter(&string);
	icuString.toUTF8(stringConverter);

	return B_OK;
}


status_t
BDateFormat::Format(BString& string, int*& fieldPositions, int& fieldCount,
	const time_t time, const BDateFormatStyle style) const
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	BString format;
	fConventions.GetDateFormat(style, format);
	ObjectDeleter<DateFormat> dateFormatter(_CreateDateFormatter(format));
	if (dateFormatter.Get() == NULL)
		return B_NO_MEMORY;

	fieldPositions = NULL;
	UErrorCode error = U_ZERO_ERROR;
	icu::FieldPositionIterator positionIterator;
	UnicodeString icuString;
	dateFormatter->format((UDate)time * 1000, icuString, &positionIterator,
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
BDateFormat::GetFields(BDateElement*& fields, int& fieldCount,
	BDateFormatStyle style) const
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	BString format;
	fConventions.GetDateFormat(style, format);
	ObjectDeleter<DateFormat> dateFormatter(_CreateDateFormatter(format));
	if (dateFormatter.Get() == NULL)
		return B_NO_MEMORY;

	fields = NULL;
	UErrorCode error = U_ZERO_ERROR;
	icu::FieldPositionIterator positionIterator;
	UnicodeString icuString;
	time_t now;
	dateFormatter->format((UDate)time(&now) * 1000, icuString,
		&positionIterator, error);

	if (U_FAILURE(error))
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
			case UDAT_YEAR_FIELD:
				fields[i] = B_DATE_ELEMENT_YEAR;
				break;
			case UDAT_MONTH_FIELD:
				fields[i] = B_DATE_ELEMENT_MONTH;
				break;
			case UDAT_DATE_FIELD:
				fields[i] = B_DATE_ELEMENT_DAY;
				break;
			default:
				fields[i] = B_DATE_ELEMENT_INVALID;
				break;
		}
	}

	return B_OK;
}


status_t
BDateFormat::GetStartOfWeek(BWeekday* startOfWeek) const
{
	if (startOfWeek == NULL)
		return B_BAD_VALUE;

	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	UErrorCode err = U_ZERO_ERROR;
	ObjectDeleter<Calendar> calendar = Calendar::createInstance(
		*BFormattingConventions::Private(&fConventions).ICULocale(), err);

	if (U_FAILURE(err))
		return B_ERROR;

	UCalendarDaysOfWeek icuWeekStart = calendar->getFirstDayOfWeek(err);
	if (U_FAILURE(err))
		return B_ERROR;

	switch (icuWeekStart) {
		case UCAL_SUNDAY:
			*startOfWeek = B_WEEKDAY_SUNDAY;
			break;
		case UCAL_MONDAY:
			*startOfWeek = B_WEEKDAY_MONDAY;
			break;
		case UCAL_TUESDAY:
			*startOfWeek = B_WEEKDAY_TUESDAY;
			break;
		case UCAL_WEDNESDAY:
			*startOfWeek = B_WEEKDAY_WEDNESDAY;
			break;
		case UCAL_THURSDAY:
			*startOfWeek = B_WEEKDAY_THURSDAY;
			break;
		case UCAL_FRIDAY:
			*startOfWeek = B_WEEKDAY_FRIDAY;
			break;
		case UCAL_SATURDAY:
			*startOfWeek = B_WEEKDAY_SATURDAY;
			break;
		default:
			return B_ERROR;
	}

	return B_OK;
}


DateFormat*
BDateFormat::_CreateDateFormatter(const BString& format) const
{
	Locale* icuLocale
		= fConventions.UseStringsFromPreferredLanguage()
			? BLanguage::Private(&fLanguage).ICULocale()
			: BFormattingConventions::Private(&fConventions).ICULocale();

	icu::DateFormat* dateFormatter
		= icu::DateFormat::createDateInstance(DateFormat::kShort, *icuLocale);
	if (dateFormatter == NULL)
		return NULL;

	SimpleDateFormat* dateFormatterImpl
		= static_cast<SimpleDateFormat*>(dateFormatter);

	UnicodeString pattern(format.String());
	dateFormatterImpl->applyPattern(pattern);

	return dateFormatter;
}


