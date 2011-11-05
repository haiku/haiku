/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de.
** Copyright 2010-2011, Oliver Tappe, zooey@hirschkaefer.de.
** All rights reserved. Distributed under the terms of the OpenBeOS License.
*/


#include <AutoDeleter.h>
#include <Autolock.h>
#include <CalendarView.h>
#include <Catalog.h>
#include <FormattingConventionsPrivate.h>
#include <LanguagePrivate.h>
#include <Locale.h>
#include <LocaleRoster.h>
#include <MutableLocaleRoster.h>
#include <TimeZone.h>

#include <ICUWrapper.h>

#include <unicode/datefmt.h>
#include <unicode/dcfmtsym.h>
#include <unicode/decimfmt.h>
#include <unicode/dtfmtsym.h>
#include <unicode/numfmt.h>
#include <unicode/smpdtfmt.h>
#include <unicode/ustring.h>

#include <vector>


using BPrivate::ObjectDeleter;


BLocale::BLocale(const BLanguage* language,
	const BFormattingConventions* conventions)
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


BLocale::BLocale(const BLocale& other)
	:
	fConventions(other.fConventions),
	fLanguage(other.fLanguage)
{
}


/*static*/ const BLocale*
BLocale::Default()
{
	return &BPrivate::RosterData::Default()->fDefaultLocale;
}


BLocale&
BLocale::operator=(const BLocale& other)
{
	if (this == &other)
		return *this;

	BAutolock lock(fLock);
	BAutolock otherLock(other.fLock);
	if (!lock.IsLocked() || !otherLock.IsLocked())
		return *this;

	fConventions = other.fConventions;
	fLanguage = other.fLanguage;

	return *this;
}


BLocale::~BLocale()
{
}


status_t
BLocale::GetCollator(BCollator* collator) const
{
	if (!collator)
		return B_BAD_VALUE;

	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	*collator = fCollator;

	return B_OK;
}


status_t
BLocale::GetLanguage(BLanguage* language) const
{
	if (!language)
		return B_BAD_VALUE;

	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	*language = fLanguage;

	return B_OK;
}


status_t
BLocale::GetFormattingConventions(BFormattingConventions* conventions) const
{
	if (!conventions)
		return B_BAD_VALUE;

	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	*conventions = fConventions;

	return B_OK;
}


const char *
BLocale::GetString(uint32 id) const
{
	// Note: this code assumes a certain order of the string bases

	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return "";

	if (id >= B_OTHER_STRINGS_BASE) {
		if (id == B_CODESET)
			return "UTF-8";

		return "";
	}
	return fLanguage.GetString(id);
}


void
BLocale::SetFormattingConventions(const BFormattingConventions& conventions)
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return;

	fConventions = conventions;
}


void
BLocale::SetCollator(const BCollator& newCollator)
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return;

	fCollator = newCollator;
}


void
BLocale::SetLanguage(const BLanguage& newLanguage)
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return;

	fLanguage = newLanguage;
}


// #pragma mark - Date


ssize_t
BLocale::FormatDate(char* string, size_t maxSize, time_t time,
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

	UnicodeString icuString;
	dateFormatter->format((UDate)time * 1000, icuString);

	CheckedArrayByteSink stringConverter(string, maxSize);
	icuString.toUTF8(stringConverter);

	if (stringConverter.Overflowed())
		return B_BAD_VALUE;

	return stringConverter.NumberOfBytesWritten();
}


status_t
BLocale::FormatDate(BString *string, time_t time, BDateFormatStyle style,
	const BTimeZone* timeZone) const
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

	string->Truncate(0);
	BStringByteSink stringConverter(string);
	icuString.toUTF8(stringConverter);

	return B_OK;
}


status_t
BLocale::FormatDate(BString* string, int*& fieldPositions, int& fieldCount,
	time_t time, BDateFormatStyle style) const
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

	string->Truncate(0);
	BStringByteSink stringConverter(string);

	icuString.toUTF8(stringConverter);

	return B_OK;
}


status_t
BLocale::GetDateFields(BDateElement*& fields, int& fieldCount,
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
BLocale::GetStartOfWeek(BWeekday* startOfWeek) const
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


ssize_t
BLocale::FormatDateTime(char* target, size_t maxSize, time_t time,
	BDateFormatStyle dateStyle, BTimeFormatStyle timeStyle) const
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	BString format;
	fConventions.GetDateFormat(dateStyle, format);
	ObjectDeleter<DateFormat> dateFormatter(_CreateDateFormatter(format));
	if (dateFormatter.Get() == NULL)
		return B_NO_MEMORY;

	fConventions.GetTimeFormat(timeStyle, format);
	ObjectDeleter<DateFormat> timeFormatter(_CreateTimeFormatter(format));
	if (timeFormatter.Get() == NULL)
		return B_NO_MEMORY;

	UnicodeString icuString;
	dateFormatter->format((UDate)time * 1000, icuString);

	icuString.append(UnicodeString::fromUTF8(", "));

	timeFormatter->format((UDate)time * 1000, icuString);

	CheckedArrayByteSink stringConverter(target, maxSize);
	icuString.toUTF8(stringConverter);

	if (stringConverter.Overflowed())
		return B_BAD_VALUE;

	return stringConverter.NumberOfBytesWritten();
}


status_t
BLocale::FormatDateTime(BString* target, time_t time,
	BDateFormatStyle dateStyle, BTimeFormatStyle timeStyle,
	const BTimeZone* timeZone) const
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	BString format;
	fConventions.GetDateFormat(dateStyle, format);
	ObjectDeleter<DateFormat> dateFormatter(_CreateDateFormatter(format));
	if (dateFormatter.Get() == NULL)
		return B_NO_MEMORY;

	fConventions.GetTimeFormat(timeStyle, format);
	ObjectDeleter<DateFormat> timeFormatter(_CreateTimeFormatter(format));
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
	dateFormatter->format((UDate)time * 1000, icuString);
	icuString.append(UnicodeString::fromUTF8(", "));
	timeFormatter->format((UDate)time * 1000, icuString);

	target->Truncate(0);
	BStringByteSink stringConverter(target);
	icuString.toUTF8(stringConverter);

	return B_OK;
}


// #pragma mark - Time


ssize_t
BLocale::FormatTime(char* string, size_t maxSize, time_t time,
	BTimeFormatStyle style) const
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	BString format;
	fConventions.GetTimeFormat(style, format);
	ObjectDeleter<DateFormat> timeFormatter(_CreateTimeFormatter(format));
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
BLocale::FormatTime(BString* string, time_t time, BTimeFormatStyle style,
	const BTimeZone* timeZone) const
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	BString format;
	fConventions.GetTimeFormat(style, format);
	ObjectDeleter<DateFormat> timeFormatter(_CreateTimeFormatter(format));
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

	string->Truncate(0);
	BStringByteSink stringConverter(string);
	icuString.toUTF8(stringConverter);

	return B_OK;
}


status_t
BLocale::FormatTime(BString* string, int*& fieldPositions, int& fieldCount,
	time_t time, BTimeFormatStyle style) const
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	BString format;
	fConventions.GetTimeFormat(style, format);
	ObjectDeleter<DateFormat> timeFormatter(_CreateTimeFormatter(format));
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

	string->Truncate(0);
	BStringByteSink stringConverter(string);
	icuString.toUTF8(stringConverter);

	return B_OK;
}


status_t
BLocale::GetTimeFields(BDateElement*& fields, int& fieldCount,
	BTimeFormatStyle style) const
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	BString format;
	fConventions.GetTimeFormat(style, format);
	ObjectDeleter<DateFormat> timeFormatter(_CreateTimeFormatter(format));
	if (timeFormatter.Get() == NULL)
		return B_NO_MEMORY;

	fields = NULL;
	UErrorCode error = U_ZERO_ERROR;
	icu::FieldPositionIterator positionIterator;
	UnicodeString icuString;
	time_t now;
	timeFormatter->format((UDate)time(&now) * 1000,	icuString,
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


// #pragma mark - Numbers


ssize_t
BLocale::FormatNumber(char* string, size_t maxSize, double value) const
{
	BString fullString;
	status_t status = FormatNumber(&fullString, value);
	if (status != B_OK)
		return status;

	return strlcpy(string, fullString.String(), maxSize);
}


status_t
BLocale::FormatNumber(BString* string, double value) const
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	UErrorCode err = U_ZERO_ERROR;
	ObjectDeleter<NumberFormat> numberFormatter(NumberFormat::createInstance(
		*BFormattingConventions::Private(&fConventions).ICULocale(),
		UNUM_DECIMAL, err));

	if (numberFormatter.Get() == NULL)
		return B_NO_MEMORY;
	if (U_FAILURE(err))
		return B_BAD_VALUE;

	UnicodeString icuString;
	numberFormatter->format(value, icuString);

	string->Truncate(0);
	BStringByteSink stringConverter(string);
	icuString.toUTF8(stringConverter);

	return B_OK;
}


ssize_t
BLocale::FormatNumber(char* string, size_t maxSize, int32 value) const
{
	BString fullString;
	status_t status = FormatNumber(&fullString, value);
	if (status != B_OK)
		return status;

	return strlcpy(string, fullString.String(), maxSize);
}


status_t
BLocale::FormatNumber(BString* string, int32 value) const
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	UErrorCode err = U_ZERO_ERROR;
	ObjectDeleter<NumberFormat> numberFormatter(NumberFormat::createInstance(
		*BFormattingConventions::Private(&fConventions).ICULocale(),
		UNUM_DECIMAL, err));

	if (numberFormatter.Get() == NULL)
		return B_NO_MEMORY;
	if (U_FAILURE(err))
		return B_BAD_VALUE;

	UnicodeString icuString;
	numberFormatter->format((int32_t)value, icuString);

	string->Truncate(0);
	BStringByteSink stringConverter(string);
	icuString.toUTF8(stringConverter);

	return B_OK;
}


ssize_t
BLocale::FormatMonetary(char* string, size_t maxSize, double value) const
{
	BString fullString;
	status_t status = FormatMonetary(&fullString, value);
	if (status != B_OK)
		return status;

	return strlcpy(string, fullString.String(), maxSize);
}


status_t
BLocale::FormatMonetary(BString* string, double value) const
{
	if (string == NULL)
		return B_BAD_VALUE;

	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	UErrorCode err = U_ZERO_ERROR;
	ObjectDeleter<NumberFormat> numberFormatter(
		NumberFormat::createCurrencyInstance(
			*BFormattingConventions::Private(&fConventions).ICULocale(),
			err));

	if (numberFormatter.Get() == NULL)
		return B_NO_MEMORY;
	if (U_FAILURE(err))
		return B_BAD_VALUE;

	UnicodeString icuString;
	numberFormatter->format(value, icuString);

	string->Truncate(0);
	BStringByteSink stringConverter(string);
	icuString.toUTF8(stringConverter);

	return B_OK;
}


DateFormat*
BLocale::_CreateDateFormatter(const BString& format) const
{
	Locale* icuLocale
		= fConventions.UseStringsFromPreferredLanguage()
			? BLanguage::Private(&fLanguage).ICULocale()
			: BFormattingConventions::Private(&fConventions).ICULocale();

	DateFormat* dateFormatter
		= DateFormat::createDateInstance(DateFormat::kShort, *icuLocale);
	if (dateFormatter == NULL)
		return NULL;

	SimpleDateFormat* dateFormatterImpl
		= static_cast<SimpleDateFormat*>(dateFormatter);

	UnicodeString pattern(format.String());
	dateFormatterImpl->applyPattern(pattern);

	return dateFormatter;
}


DateFormat*
BLocale::_CreateTimeFormatter(const BString& format) const
{
	Locale* icuLocale
		= fConventions.UseStringsFromPreferredLanguage()
			? BLanguage::Private(&fLanguage).ICULocale()
			: BFormattingConventions::Private(&fConventions).ICULocale();

	DateFormat* timeFormatter
		= DateFormat::createTimeInstance(DateFormat::kShort, *icuLocale);
	if (timeFormatter == NULL)
		return NULL;

	SimpleDateFormat* timeFormatterImpl
		= static_cast<SimpleDateFormat*>(timeFormatter);

	UnicodeString pattern(format.String());
	timeFormatterImpl->applyPattern(pattern);

	return timeFormatter;
}
