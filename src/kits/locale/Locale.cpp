/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <AutoDeleter.h>
#include <CalendarView.h>
#include <Catalog.h>
#include <Locale.h>
#include <LocaleRoster.h>

#include <unicode/datefmt.h>
#include <unicode/dcfmtsym.h>
#include <unicode/decimfmt.h>
#include <unicode/dtfmtsym.h>
#include <unicode/smpdtfmt.h>
#include <ICUWrapper.h>

#include <vector>


#define ICU_VERSION icu_44


using BPrivate::ObjectDeleter;
using BPrivate::B_WEEK_START_MONDAY;
using BPrivate::B_WEEK_START_SUNDAY;


static DateFormat* CreateDateFormat(bool longFormat, const Locale& locale,
						const BString& format);
static DateFormat* CreateTimeFormat(bool longFormat, const Locale& locale,
						const BString& format);


BLocale::BLocale(const char* languageAndCountryCode)
	:
	fCountry(languageAndCountryCode),
	fLanguage(languageAndCountryCode),
	fICULocale(new ICU_VERSION::Locale(languageAndCountryCode))
{
}


BLocale::BLocale(const BLocale& other)
	:
	fCountry(other.fCountry),
	fLanguage(other.fLanguage),
	fICULocale(new ICU_VERSION::Locale(*other.fICULocale)),
	fLongDateFormat(other.fLongDateFormat),
	fShortDateFormat(other.fShortDateFormat),
	fLongTimeFormat(other.fLongTimeFormat),
	fShortTimeFormat(other.fShortTimeFormat)
{
}


BLocale&
BLocale::operator=(const BLocale& other)
{
	if (this == &other)
		return *this;

	*fICULocale = *other.fICULocale;

	fLongDateFormat = other.fLongDateFormat;
	fShortDateFormat = other.fShortDateFormat;
	fLongTimeFormat = other.fLongTimeFormat;
	fShortTimeFormat = other.fShortTimeFormat;

	fCountry = other.fCountry;
	fLanguage = other.fLanguage;

	return *this;
}


BLocale::~BLocale()
{
	delete fICULocale;
}


const char *
BLocale::GetString(uint32 id)
{
	// Note: this code assumes a certain order of the string bases

	if (id >= B_OTHER_STRINGS_BASE) {
		if (id == B_CODESET)
			return "UTF-8";

		return "";
	}
	return fLanguage.GetString(id);
}


void
BLocale::SetCountry(const BCountry& newCountry)
{
	fCountry = newCountry;
}


void
BLocale::SetCollator(const BCollator& newCollator)
{
	fCollator = newCollator;
}


void
BLocale::SetLanguage(const char* languageCode)
{
	fLanguage.SetTo(languageCode);
}


const char*
BLocale::Code() const
{
	return fICULocale->getName();
}


bool
BLocale::GetName(BString& name) const
{
	UnicodeString uString;
	fICULocale->getDisplayName(uString);
	BStringByteSink stringConverter(&name);
	uString.toUTF8(stringConverter);
	return true;
}

// #pragma mark - Date


status_t
BLocale::FormatDate(char* string, size_t maxSize, time_t time, bool longFormat)
{
	ObjectDeleter<DateFormat> dateFormatter = CreateDateFormat(longFormat,
		*fICULocale, longFormat ? fLongDateFormat : fShortDateFormat);
	if (dateFormatter.Get() == NULL)
		return B_NO_MEMORY;

	UnicodeString ICUString;
	ICUString = dateFormatter->format((UDate)time * 1000, ICUString);

	CheckedArrayByteSink stringConverter(string, maxSize);

	ICUString.toUTF8(stringConverter);

	if (stringConverter.Overflowed())
		return B_BAD_VALUE;

	return B_OK;
}


status_t
BLocale::FormatDate(BString *string, time_t time, bool longFormat)
{
	string->Truncate(0);
		// We make the string empty, this way even in cases where ICU fail we at
		// least return something sane
	ObjectDeleter<DateFormat> dateFormatter = CreateDateFormat(longFormat,
		*fICULocale, longFormat ? fLongDateFormat : fShortDateFormat);
	if (dateFormatter.Get() == NULL)
		return B_NO_MEMORY;

	UnicodeString ICUString;
	ICUString = dateFormatter->format((UDate)time * 1000, ICUString);

	BStringByteSink stringConverter(string);

	ICUString.toUTF8(stringConverter);

	return B_OK;
}


status_t
BLocale::FormatDate(BString* string, int*& fieldPositions, int& fieldCount,
	time_t time, bool longFormat)
{
	string->Truncate(0);

	ObjectDeleter<DateFormat> dateFormatter = CreateDateFormat(longFormat,
		*fICULocale, longFormat ? fLongDateFormat : fShortDateFormat);
	if (dateFormatter.Get() == NULL)
		return B_NO_MEMORY;

	fieldPositions = NULL;
	UErrorCode error = U_ZERO_ERROR;
	ICU_VERSION::FieldPositionIterator positionIterator;
	UnicodeString ICUString;
	ICUString = dateFormatter->format((UDate)time * 1000, ICUString,
		&positionIterator, error);

	if (error != U_ZERO_ERROR)
		return B_ERROR;

	ICU_VERSION::FieldPosition field;
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

	BStringByteSink stringConverter(string);

	ICUString.toUTF8(stringConverter);

	return B_OK;
}


status_t
BLocale::GetDateFormat(BString& format, bool longFormat) const
{
	if (longFormat && fLongDateFormat.Length() > 0)
		format = fLongDateFormat;
	else if (!longFormat && fShortDateFormat.Length() > 0)
		format = fShortDateFormat;
	else {
		format.Truncate(0);

		ObjectDeleter<DateFormat> dateFormatter = CreateDateFormat(longFormat,
			*fICULocale, longFormat ? fLongDateFormat : fShortDateFormat);
		if (dateFormatter.Get() == NULL)
			return B_NO_MEMORY;

		SimpleDateFormat* dateFormatterImpl
			= static_cast<SimpleDateFormat*>(dateFormatter.Get());

		UnicodeString ICUString;
		ICUString = dateFormatterImpl->toPattern(ICUString);

		BStringByteSink stringConverter(&format);

		ICUString.toUTF8(stringConverter);
	}

	return B_OK;
}


status_t
BLocale::SetDateFormat(const char* formatString, bool longFormat)
{
printf("FV::SetDateFormat: df='%s'\n", formatString);
	if (longFormat)
		fLongDateFormat = formatString;
	else
		fShortDateFormat = formatString;

	return B_OK;
}


status_t
BLocale::GetDateFields(BDateElement*& fields, int& fieldCount,
	bool longFormat) const
{
	ObjectDeleter<DateFormat> dateFormatter = CreateDateFormat(longFormat,
		*fICULocale, longFormat ? fLongDateFormat : fShortDateFormat);
	if (dateFormatter.Get() == NULL)
		return B_NO_MEMORY;

	fields = NULL;
	UErrorCode error = U_ZERO_ERROR;
	ICU_VERSION::FieldPositionIterator positionIterator;
	UnicodeString ICUString;
	time_t now;
	ICUString = dateFormatter->format((UDate)time(&now) * 1000, ICUString,
		&positionIterator, error);

	if (U_FAILURE(error))
		return B_ERROR;

	ICU_VERSION::FieldPosition field;
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


int
BLocale::StartOfWeek() const
{
	UErrorCode err = U_ZERO_ERROR;
	Calendar* c = Calendar::createInstance(*fICULocale, err);

	if (err == U_ZERO_ERROR && c->getFirstDayOfWeek(err) == UCAL_SUNDAY) {
		delete c;
		return B_WEEK_START_SUNDAY;
	} else {
		delete c;
		// Might be another day, but BeAPI will not handle it
		return B_WEEK_START_MONDAY;
	}
}


status_t
BLocale::FormatDateTime(char* target, size_t maxSize, time_t time,
	bool longFormat)
{
	ObjectDeleter<DateFormat> dateFormatter = CreateDateFormat(longFormat,
		*fICULocale, longFormat ? fLongDateFormat : fShortDateFormat);
	if (dateFormatter.Get() == NULL)
		return B_NO_MEMORY;

	ObjectDeleter<DateFormat> timeFormatter = CreateTimeFormat(longFormat,
		*fICULocale, longFormat ? fLongTimeFormat : fShortTimeFormat);
	if (timeFormatter.Get() == NULL)
		return B_NO_MEMORY;

	UnicodeString ICUString;
	ICUString = dateFormatter->format((UDate)time * 1000, ICUString);

	ICUString.append(UnicodeString::fromUTF8(", "));

	ICUString = timeFormatter->format((UDate)time * 1000, ICUString);

	CheckedArrayByteSink stringConverter(target, maxSize);
	ICUString.toUTF8(stringConverter);

	if (stringConverter.Overflowed())
		return B_BAD_VALUE;

	return B_OK;
}


// #pragma mark - Time


status_t
BLocale::FormatTime(char* string, size_t maxSize, time_t time, bool longFormat)
{
	ObjectDeleter<DateFormat> timeFormatter = CreateTimeFormat(longFormat,
		*fICULocale, longFormat ? fLongTimeFormat : fShortTimeFormat);
	if (timeFormatter.Get() == NULL)
		return B_NO_MEMORY;

	UnicodeString ICUString;
	ICUString = timeFormatter->format((UDate)time * 1000, ICUString);

	CheckedArrayByteSink stringConverter(string, maxSize);

	ICUString.toUTF8(stringConverter);

	if (stringConverter.Overflowed())
		return B_BAD_VALUE;

	return B_OK;
}


status_t
BLocale::FormatTime(BString* string, time_t time, bool longFormat)
{
	string->Truncate(0);

	ObjectDeleter<DateFormat> timeFormatter = CreateTimeFormat(longFormat,
		*fICULocale, longFormat ? fLongTimeFormat : fShortTimeFormat);
	if (timeFormatter.Get() == NULL)
		return B_NO_MEMORY;

	UnicodeString ICUString;
	ICUString = timeFormatter->format((UDate)time * 1000, ICUString);

	BStringByteSink stringConverter(string);

	ICUString.toUTF8(stringConverter);

	return B_OK;
}


status_t
BLocale::FormatTime(BString* string, int*& fieldPositions, int& fieldCount,
	time_t time, bool longFormat)
{
	string->Truncate(0);

	ObjectDeleter<DateFormat> timeFormatter = CreateTimeFormat(longFormat,
		*fICULocale, longFormat ? fLongTimeFormat : fShortTimeFormat);
	if (timeFormatter.Get() == NULL)
		return B_NO_MEMORY;

	fieldPositions = NULL;
	UErrorCode error = U_ZERO_ERROR;
	ICU_VERSION::FieldPositionIterator positionIterator;
	UnicodeString ICUString;
	ICUString = timeFormatter->format((UDate)time * 1000, ICUString,
		&positionIterator, error);

	if (error != U_ZERO_ERROR)
		return B_ERROR;

	ICU_VERSION::FieldPosition field;
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

	BStringByteSink stringConverter(string);

	ICUString.toUTF8(stringConverter);

	return B_OK;
}


status_t
BLocale::GetTimeFields(BDateElement*& fields, int& fieldCount,
	bool longFormat) const
{
	ObjectDeleter<DateFormat> timeFormatter = CreateTimeFormat(longFormat,
		*fICULocale, longFormat ? fLongTimeFormat : fShortTimeFormat);
	if (timeFormatter.Get() == NULL)
		return B_NO_MEMORY;

	fields = NULL;
	UErrorCode error = U_ZERO_ERROR;
	ICU_VERSION::FieldPositionIterator positionIterator;
	UnicodeString ICUString;
	time_t now;
	ICUString = timeFormatter->format((UDate)time(&now) * 1000,	ICUString,
		&positionIterator, error);

	if (error != U_ZERO_ERROR)
		return B_ERROR;

	ICU_VERSION::FieldPosition field;
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
BLocale::SetTimeFormat(const char* formatString, bool longFormat)
{
	if (longFormat)
		fLongTimeFormat = formatString;
	else
		fShortTimeFormat = formatString;

	return B_OK;
}


status_t
BLocale::GetTimeFormat(BString& format, bool longFormat) const
{
	if (longFormat && fLongTimeFormat.Length() > 0)
		format = fLongTimeFormat;
	else if (!longFormat && fShortTimeFormat.Length() > 0)
		format = fShortTimeFormat;
	else {
		format.Truncate(0);

		ObjectDeleter<DateFormat> timeFormatter = CreateTimeFormat(longFormat,
			*fICULocale, longFormat ? fLongTimeFormat : fShortTimeFormat);
		if (timeFormatter.Get() == NULL)
			return B_NO_MEMORY;

		SimpleDateFormat* timeFormatterImpl
			= static_cast<SimpleDateFormat*>(timeFormatter.Get());

		UnicodeString ICUString;
		ICUString = timeFormatterImpl->toPattern(ICUString);

		BStringByteSink stringConverter(&format);
		ICUString.toUTF8(stringConverter);
	}

	return B_OK;
}


// #pragma mark - Numbers


status_t
BLocale::FormatNumber(char* string, size_t maxSize, double value)
{
	BString fullString;
	status_t status = FormatNumber(&fullString, value);
	if (status == B_OK)
		strlcpy(string, fullString.String(), maxSize);

	return status;
}


status_t
BLocale::FormatNumber(BString* string, double value)
{
	UErrorCode err = U_ZERO_ERROR;
	ObjectDeleter<NumberFormat> numberFormatter	= NumberFormat::createInstance(
		*fICULocale, NumberFormat::kNumberStyle, err);

	if (numberFormatter.Get() == NULL)
		return B_NO_MEMORY;
	if (U_FAILURE(err))
		return B_ERROR;

	UnicodeString ICUString;
	ICUString = numberFormatter->format(value, ICUString);

	string->Truncate(0);
	BStringByteSink stringConverter(string);
	ICUString.toUTF8(stringConverter);

	return B_OK;
}


status_t
BLocale::FormatNumber(char* string, size_t maxSize, int32 value)
{
	BString fullString;
	status_t status = FormatNumber(&fullString, value);
	if (status == B_OK)
		strlcpy(string, fullString.String(), maxSize);

	return status;
}


status_t
BLocale::FormatNumber(BString* string, int32 value)
{
	UErrorCode err = U_ZERO_ERROR;
	ObjectDeleter<NumberFormat> numberFormatter	= NumberFormat::createInstance(
		*fICULocale, NumberFormat::kNumberStyle, err);

	if (numberFormatter.Get() == NULL)
		return B_NO_MEMORY;
	if (U_FAILURE(err))
		return B_ERROR;

	UnicodeString ICUString;
	ICUString = numberFormatter->format((int32_t)value, ICUString);

	string->Truncate(0);
	BStringByteSink stringConverter(string);
	ICUString.toUTF8(stringConverter);

	return B_OK;
}


ssize_t
BLocale::FormatMonetary(char* string, size_t maxSize, double value)
{
	BString fullString;
	ssize_t written = FormatMonetary(&fullString, value);
	if (written < 0)
		return written;

	return strlcpy(string, fullString.String(), maxSize);
}


ssize_t
BLocale::FormatMonetary(BString* string, double value)
{
	if (string == NULL)
		return B_BAD_VALUE;

	UErrorCode err;
	ObjectDeleter<NumberFormat> numberFormatter
		= NumberFormat::createCurrencyInstance(*fICULocale, err);

	if (numberFormatter.Get() == NULL)
		return B_NO_MEMORY;
	if (U_FAILURE(err))
		return B_ERROR;

	UnicodeString ICUString;
	ICUString = numberFormatter->format(value, ICUString);

	string->Truncate(0);
	BStringByteSink stringConverter(string);
	ICUString.toUTF8(stringConverter);

	return string->Length();
}


// #pragma mark - Helpers


static DateFormat*
CreateDateFormat(bool longFormat, const Locale& locale,
	const BString& format)
{
	DateFormat* dateFormatter = DateFormat::createDateInstance(
		longFormat ? DateFormat::FULL : DateFormat::SHORT, locale);

	if (format.Length() > 0) {
		SimpleDateFormat* dateFormatterImpl
			= static_cast<SimpleDateFormat*>(dateFormatter);

		UnicodeString pattern(format.String());
		dateFormatterImpl->applyPattern(pattern);
	}

	return dateFormatter;
}


static DateFormat*
CreateTimeFormat(bool longFormat, const Locale& locale,
	const BString& format)
{
	DateFormat* timeFormatter = DateFormat::createTimeInstance(
		longFormat ? DateFormat::MEDIUM : DateFormat::SHORT, locale);

	if (format.Length() > 0) {
		SimpleDateFormat* timeFormatterImpl
			= static_cast<SimpleDateFormat*>(timeFormatter);

		UnicodeString pattern(format.String());
		timeFormatterImpl->applyPattern(pattern);
	}

	return timeFormatter;
}
