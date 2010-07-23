/*
 * Copyright 2003-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2009-2010, Adrien Destugues, pulkomandy@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <Country.h>

#include <assert.h>
#include <iostream>
#include <stdlib.h>
#include <vector>

#include <CalendarView.h>
#include <IconUtils.h>
#include <Resources.h>
#include <String.h>

#include <unicode/datefmt.h>
#include <unicode/dcfmtsym.h>
#include <unicode/decimfmt.h>
#include <unicode/dtfmtsym.h>
#include <unicode/smpdtfmt.h>
#include <ICUWrapper.h>

#include <monetary.h>
#include <stdarg.h>


#define ICU_VERSION icu_44


using BPrivate::B_WEEK_START_MONDAY;
using BPrivate::B_WEEK_START_SUNDAY;


BCountry::BCountry(const char* languageCode, const char* countryCode)
{
	fICULocale = new ICU_VERSION::Locale(languageCode, countryCode);
	fICULongDateFormatter = NULL;
	fICUShortDateFormatter = NULL;
	fICULongTimeFormatter = NULL;
	fICUShortTimeFormatter = NULL;
}


BCountry::BCountry(const char* languageAndCountryCode)
{
	fICULocale = new ICU_VERSION::Locale(languageAndCountryCode);

	// These will be initialized the first time they are actually needed
	// this way the class is more lightweight when you just want the country
	// name or some other information
	fICULongDateFormatter = NULL;
	fICUShortDateFormatter = NULL;
	fICULongTimeFormatter = NULL;
	fICUShortTimeFormatter = NULL;
}


BCountry::BCountry(const BCountry& other)
{
	fICULocale = new ICU_VERSION::Locale(*other.fICULocale);

	// We could copy these, but we can recreate them when needed from the locale
	// anyway
	fICULongDateFormatter = NULL;
	fICUShortDateFormatter = NULL;
	fICULongTimeFormatter = NULL;
	fICUShortTimeFormatter = NULL;

	/*
	if (other.fICULongDateFormatter) {
		fICULongDateFormatter = new ICU_VERSION::SimpleDateFormat(
			*static_cast<SimpleDateFormat*>(other.fICULongDateFormatter));
	}

	if (other.fICUShortDateFormatter) {
		fICUShortDateFormatter = new ICU_VERSION::SimpleDateFormat(
			*static_cast<SimpleDateFormat*>(other.fICUShortDateFormatter));
	}

	if (other.fICULongTimeFormatter) {
		fICULongTimeFormatter = new ICU_VERSION::SimpleDateFormat(
			*static_cast<SimpleDateFormat*>(other.fICULongTimeFormatter));
	}

	if (other.fICUShortTimeFormatter) {
		fICUShortTimeFormatter = new ICU_VERSION::SimpleDateFormat(
			*static_cast<SimpleDateFormat*>(other.fICUShortTimeFormatter));
	}
	*/
}


BCountry&
BCountry::operator=(const BCountry& other)
{
	if (this == &other)
		return *this;

	delete fICULongTimeFormatter;
	delete fICUShortTimeFormatter;
	delete fICULongDateFormatter;
	delete fICUShortDateFormatter;
	delete fICULocale;

	fICULocale = new ICU_VERSION::Locale(*other.fICULocale);

	// We could copy these, but we can recreate them when needed from the locale
	// anyway
	fICULongDateFormatter = NULL;
	fICUShortDateFormatter = NULL;
	fICULongTimeFormatter = NULL;
	fICUShortTimeFormatter = NULL;

	return *this;
}


BCountry::~BCountry()
{
	delete fICULongTimeFormatter;
	delete fICUShortTimeFormatter;
	delete fICULongDateFormatter;
	delete fICUShortDateFormatter;
	delete fICULocale;
}


bool
BCountry::Name(BString& name) const
{
	UnicodeString uString;
	fICULocale->getDisplayCountry(uString);
	BStringByteSink stringConverter(&name);
	uString.toUTF8(stringConverter);
	return true;
}


bool
BCountry::LocaleName(BString& name) const
{
	UnicodeString uString;
	fICULocale->getDisplayName(uString);
	BStringByteSink stringConverter(&name);
	uString.toUTF8(stringConverter);
	return true;
}


const char*
BCountry::Code() const
{
	return fICULocale->getName();
}


status_t
BCountry::GetIcon(BBitmap* result)
{
	if (result == NULL)
		return B_BAD_DATA;
	// TODO: a proper way to locate the library being used ?
	BResources storage("/boot/system/lib/liblocale.so");
	if (storage.InitCheck() != B_OK)
		return B_ERROR;
	size_t size;
	const char* code = fICULocale->getCountry();
	if (code != NULL) {
		const void* buffer = storage.LoadResource(B_VECTOR_ICON_TYPE, code,
			&size);
		if (buffer != NULL && size != 0) {
			return BIconUtils::GetVectorIcon(static_cast<const uint8*>(buffer),
				size, result);
		}
	}
	return B_BAD_DATA;
}


// #pragma mark - Date


DateFormat*
BCountry::DateFormatter(bool longFormat)
{
	// TODO: ICU allows for 4 different levels of expansion :
	// short, medium, long, and full. Our bool parameter is not enough...
	if (longFormat) {
		if (fICULongDateFormatter == NULL) {
			fICULongDateFormatter = DateFormat::createDateInstance(
				DateFormat::FULL, *fICULocale);
		}
		return fICULongDateFormatter;
	} else {
		if (fICUShortDateFormatter == NULL) {
			fICUShortDateFormatter = DateFormat::createDateInstance(
				DateFormat::SHORT, *fICULocale);
		}
		return fICUShortDateFormatter;
	}
}


DateFormat*
BCountry::TimeFormatter(bool longFormat)
{
	// TODO: ICU allows for 4 different levels of expansion :
	// short, medium, long, and full. Our bool parameter is not enough...
	if (longFormat) {
		if (fICULongTimeFormatter == NULL) {
			fICULongTimeFormatter = DateFormat::createTimeInstance(
				DateFormat::MEDIUM, *fICULocale);
		}
		return fICULongTimeFormatter;
	} else {
		if (fICUShortTimeFormatter == NULL) {
			fICUShortTimeFormatter = DateFormat::createTimeInstance(
				DateFormat::SHORT, *fICULocale);
		}
		return fICUShortTimeFormatter;
	}
}


status_t
BCountry::FormatDate(char* string, size_t maxSize, time_t time, bool longFormat)
{
	ICU_VERSION::DateFormat* dateFormatter = DateFormatter(longFormat);
	if (dateFormatter == NULL)
		return B_NO_MEMORY;

	UnicodeString ICUString;
	ICUString = dateFormatter->format((UDate)time * 1000, ICUString);

	CheckedArrayByteSink stringConverter(string, maxSize);

	ICUString.toUTF8(stringConverter);

	if (stringConverter.Overflowed())
		return B_BAD_VALUE;
	else
		return B_OK;
}


status_t
BCountry::FormatDate(BString *string, time_t time, bool longFormat)
{
	string->Truncate(0);
		// We make the string empty, this way even in cases where ICU fail we at
		// least return something sane
	ICU_VERSION::DateFormat* dateFormatter = DateFormatter(longFormat);
	if (dateFormatter == NULL)
		return B_NO_MEMORY;

	UnicodeString ICUString;
	ICUString = dateFormatter->format((UDate)time * 1000, ICUString);

	BStringByteSink stringConverter(string);

	ICUString.toUTF8(stringConverter);

	return B_OK;
}


status_t
BCountry::FormatDate(BString* string, int*& fieldPositions, int& fieldCount,
	time_t time, bool longFormat)
{
	string->Truncate(0);

	ICU_VERSION::DateFormat* dateFormatter = DateFormatter(longFormat);
	if (dateFormatter == NULL)
		return B_NO_MEMORY;

	fieldPositions = NULL;
	UErrorCode error = U_ZERO_ERROR;
	ICU_VERSION::FieldPositionIterator positionIterator;
	UnicodeString ICUString;
	ICUString = DateFormatter(longFormat)->format((UDate)time * 1000, ICUString,
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
BCountry::DateFormat(BString& format, bool longFormat)
{
	format.Truncate(0);

	ICU_VERSION::DateFormat* dateFormatter = DateFormatter(longFormat);
	if (dateFormatter == NULL)
		return B_NO_MEMORY;

	SimpleDateFormat* dateFormatterImpl
		= static_cast<SimpleDateFormat*>(dateFormatter);

	UnicodeString ICUString;
	ICUString = dateFormatterImpl->toPattern(ICUString);

	BStringByteSink stringConverter(&format);

	ICUString.toUTF8(stringConverter);

	return B_OK;
}


status_t
BCountry::SetDateFormat(const char* formatString, bool longFormat)
{
	ICU_VERSION::DateFormat* dateFormatter = DateFormatter(longFormat);
	if (dateFormatter == NULL)
		return B_NO_MEMORY;

	SimpleDateFormat* dateFormatterImpl
		= static_cast<SimpleDateFormat*>(dateFormatter);

	UnicodeString pattern(formatString);
	dateFormatterImpl->applyPattern(pattern);

	return B_OK;
}


status_t
BCountry::DateFields(BDateElement*& fields, int& fieldCount, bool longFormat)
{
	ICU_VERSION::DateFormat* dateFormatter = DateFormatter(longFormat);
	if (dateFormatter == NULL)
		return B_NO_MEMORY;

	fields = NULL;
	UErrorCode error = U_ZERO_ERROR;
	ICU_VERSION::FieldPositionIterator positionIterator;
	UnicodeString ICUString;
	time_t now;
	ICUString = dateFormatter->format((UDate)time(&now) * 1000, ICUString,
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
BCountry::StartOfWeek()
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


// #pragma mark - Time


status_t
BCountry::FormatTime(char* string, size_t maxSize, time_t time, bool longFormat)
{
	ICU_VERSION::DateFormat* timeFormatter = TimeFormatter(longFormat);
	if (timeFormatter == NULL)
		return B_NO_MEMORY;

	UnicodeString ICUString;
	ICUString = timeFormatter->format((UDate)time * 1000, ICUString);

	CheckedArrayByteSink stringConverter(string, maxSize);

	ICUString.toUTF8(stringConverter);

	if (stringConverter.Overflowed())
		return B_BAD_VALUE;
	else
		return B_OK;
}


status_t
BCountry::FormatTime(BString* string, time_t time, bool longFormat)
{
	string->Truncate(0);

	ICU_VERSION::DateFormat* timeFormatter = TimeFormatter(longFormat);
	if (timeFormatter == NULL)
		return B_NO_MEMORY;

	UnicodeString ICUString;
	ICUString = timeFormatter->format((UDate)time * 1000, ICUString);

	BStringByteSink stringConverter(string);

	ICUString.toUTF8(stringConverter);

	return B_OK;
}


status_t
BCountry::FormatTime(BString* string, int*& fieldPositions, int& fieldCount,
	time_t time, bool longFormat)
{
	string->Truncate(0);

	ICU_VERSION::DateFormat* timeFormatter = TimeFormatter(longFormat);
	if (timeFormatter == NULL)
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
BCountry::TimeFields(BDateElement*& fields, int& fieldCount, bool longFormat)
{
	ICU_VERSION::DateFormat* timeFormatter = TimeFormatter(longFormat);
	if (timeFormatter == NULL)
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
BCountry::SetTimeFormat(const char* formatString, bool longFormat)
{
	ICU_VERSION::DateFormat* timeFormatter = TimeFormatter(longFormat);
	if (timeFormatter == NULL)
		return B_NO_MEMORY;

	SimpleDateFormat* dateFormatterImpl
		= static_cast<SimpleDateFormat*>(timeFormatter);

	UnicodeString pattern(formatString);
	dateFormatterImpl->applyPattern(pattern);

	return B_OK;
}


status_t
BCountry::TimeFormat(BString& format, bool longFormat)
{
	ICU_VERSION::DateFormat* timeFormatter = TimeFormatter(longFormat);
	if (timeFormatter == NULL)
		return B_NO_MEMORY;

	SimpleDateFormat* dateFormatterImpl
		= static_cast<SimpleDateFormat*>(timeFormatter);

	UnicodeString ICUString;
	ICUString = dateFormatterImpl->toPattern(ICUString);

	BStringByteSink stringConverter(&format);

	ICUString.toUTF8(stringConverter);

	return B_OK;
}


// #pragma mark - Numbers


void
BCountry::FormatNumber(char* string, size_t maxSize, double value)
{
	BString fullString;
	FormatNumber(&fullString, value);
	strncpy(string, fullString.String(), maxSize);
}


status_t
BCountry::FormatNumber(BString* string, double value)
{
	UErrorCode err = U_ZERO_ERROR;
	NumberFormat* numberFormatter
		= NumberFormat::createInstance(*fICULocale, NumberFormat::kNumberStyle,
			err);

	if (U_FAILURE(err))
		return B_ERROR;

	UnicodeString ICUString;
	ICUString = numberFormatter->format(value, ICUString);

	string->Truncate(0);
	BStringByteSink stringConverter(string);

	ICUString.toUTF8(stringConverter);

	return B_OK;
}


void
BCountry::FormatNumber(char* string, size_t maxSize, int32 value)
{
	BString fullString;
	FormatNumber(&fullString, value);
	strncpy(string, fullString.String(), maxSize);
}


void
BCountry::FormatNumber(BString* string, int32 value)
{
	UErrorCode err;
	NumberFormat* numberFormatter
		= NumberFormat::createInstance(*fICULocale, err);

	assert(err == U_ZERO_ERROR);

	UnicodeString ICUString;
	ICUString = numberFormatter->format((int32_t)value, ICUString);

	string->Truncate(0);
	BStringByteSink stringConverter(string);

	ICUString.toUTF8(stringConverter);
}


// This will only work for locales using the decimal system...
bool
BCountry::DecimalPoint(BString& format) const
{
	UErrorCode err;
	NumberFormat* numberFormatter
		= NumberFormat::createInstance(*fICULocale, err);

	assert(err == U_ZERO_ERROR);

	DecimalFormat* decimalFormatter
		= dynamic_cast<DecimalFormat*>(numberFormatter);

	assert(decimalFormatter != NULL);

	const DecimalFormatSymbols* syms
		= decimalFormatter->getDecimalFormatSymbols();

	UnicodeString ICUString;
	ICUString = syms->getSymbol(DecimalFormatSymbols::kDecimalSeparatorSymbol);

	BStringByteSink stringConverter(&format);

	ICUString.toUTF8(stringConverter);

	return true;
}


bool
BCountry::ThousandsSeparator(BString& separator) const
{
	UErrorCode err;
	NumberFormat* numberFormatter
		= NumberFormat::createInstance(*fICULocale, err);
	assert(err == U_ZERO_ERROR);
	DecimalFormat* decimalFormatter
		= dynamic_cast<DecimalFormat*>(numberFormatter);

	assert(decimalFormatter != NULL);

	const DecimalFormatSymbols* syms
		= decimalFormatter->getDecimalFormatSymbols();

	UnicodeString ICUString;
	ICUString = syms->getSymbol(DecimalFormatSymbols::kPatternSeparatorSymbol);

	BStringByteSink stringConverter(&separator);

	ICUString.toUTF8(stringConverter);

	return true;
}


bool
BCountry::Grouping(BString& grouping) const
{
	UErrorCode err;
	NumberFormat* numberFormatter
		= NumberFormat::createInstance(*fICULocale, err);
	assert(err == U_ZERO_ERROR);
	DecimalFormat* decimalFormatter
		= dynamic_cast<DecimalFormat*>(numberFormatter);

	assert(decimalFormatter != NULL);

	const DecimalFormatSymbols* syms
		= decimalFormatter->getDecimalFormatSymbols();

	UnicodeString ICUString;
	ICUString = syms->getSymbol(DecimalFormatSymbols::kGroupingSeparatorSymbol);

	BStringByteSink stringConverter(&grouping);

	ICUString.toUTF8(stringConverter);

	return true;
}


bool
BCountry::PositiveSign(BString& sign) const
{
	UErrorCode err;
	NumberFormat* numberFormatter
		= NumberFormat::createInstance(*fICULocale, err);
	assert(err == U_ZERO_ERROR);
	DecimalFormat* decimalFormatter
		= dynamic_cast<DecimalFormat*>(numberFormatter);

	assert(decimalFormatter != NULL);

	const DecimalFormatSymbols* syms
		= decimalFormatter->getDecimalFormatSymbols();

	UnicodeString ICUString;
	ICUString = syms->getSymbol(DecimalFormatSymbols::kPlusSignSymbol);

	BStringByteSink stringConverter(&sign);

	ICUString.toUTF8(stringConverter);

	return true;
}


bool
BCountry::NegativeSign(BString& sign) const
{
	UErrorCode err;
	NumberFormat* numberFormatter
		= NumberFormat::createInstance(*fICULocale, err);
	assert(err == U_ZERO_ERROR);
	DecimalFormat* decimalFormatter
		= dynamic_cast<DecimalFormat*>(numberFormatter);

	assert(decimalFormatter != NULL);

	const DecimalFormatSymbols* syms
		= decimalFormatter->getDecimalFormatSymbols();

	UnicodeString ICUString;
	ICUString = syms->getSymbol(DecimalFormatSymbols::kMinusSignSymbol);

	BStringByteSink stringConverter(&sign);

	ICUString.toUTF8(stringConverter);

	return true;
}


// TODO does ICU even support this ? Is it in the keywords ?
int8
BCountry::Measurement() const
{
	return B_US;
}


ssize_t
BCountry::FormatMonetary(char* string, size_t maxSize, double value)
{
	BString fullString;
	ssize_t written = FormatMonetary(&fullString, value);
	if (written < 0)
		return written;

	return strlcpy(string, fullString.String(), maxSize);
}


ssize_t
BCountry::FormatMonetary(BString* string, double value)
{
	if (string == NULL)
		return B_BAD_VALUE;

	UErrorCode err;
	NumberFormat* numberFormatter
		= NumberFormat::createCurrencyInstance(*fICULocale, err);

	assert(err == U_ZERO_ERROR);

	UnicodeString ICUString;
	ICUString = numberFormatter->format(value, ICUString);

	string->Truncate(0);
	BStringByteSink stringConverter(string);

	ICUString.toUTF8(stringConverter);

	return string->Length();
}


bool
BCountry::CurrencySymbol(BString& symbol) const
{
	UErrorCode err;
	NumberFormat* numberFormatter
		= NumberFormat::createCurrencyInstance(*fICULocale, err);
	assert(err == U_ZERO_ERROR);
	DecimalFormat* decimalFormatter
		= dynamic_cast<DecimalFormat*>(numberFormatter);

	assert(decimalFormatter != NULL);

	const DecimalFormatSymbols* syms
		= decimalFormatter->getDecimalFormatSymbols();

	UnicodeString ICUString;
	ICUString = syms->getSymbol(DecimalFormatSymbols::kCurrencySymbol);

	BStringByteSink stringConverter(&symbol);

	ICUString.toUTF8(stringConverter);

	return true;
}


bool
BCountry::InternationalCurrencySymbol(BString& symbol) const
{
	UErrorCode err;
	NumberFormat* numberFormatter
		= NumberFormat::createCurrencyInstance(*fICULocale, err);
	assert(err == U_ZERO_ERROR);
	DecimalFormat* decimalFormatter
		= dynamic_cast<DecimalFormat*>(numberFormatter);

	assert(decimalFormatter != NULL);

	const DecimalFormatSymbols* syms
		= decimalFormatter->getDecimalFormatSymbols();

	UnicodeString ICUString;
	ICUString = syms->getSymbol(DecimalFormatSymbols::kIntlCurrencySymbol);

	BStringByteSink stringConverter(&symbol);

	ICUString.toUTF8(stringConverter);

	return true;
}


bool
BCountry::MonDecimalPoint(BString& decimal) const
{
	UErrorCode err;
	NumberFormat* numberFormatter
		= NumberFormat::createCurrencyInstance(*fICULocale, err);
	assert(err == U_ZERO_ERROR);
	DecimalFormat* decimalFormatter
		= dynamic_cast<DecimalFormat*>(numberFormatter);

	assert(decimalFormatter != NULL);

	const DecimalFormatSymbols* syms
		= decimalFormatter->getDecimalFormatSymbols();

	UnicodeString ICUString;
	ICUString = syms->getSymbol(DecimalFormatSymbols::kMonetarySeparatorSymbol);

	BStringByteSink stringConverter(&decimal);

	ICUString.toUTF8(stringConverter);

	return true;
}


bool
BCountry::MonThousandsSeparator(BString& separator) const
{
	UErrorCode err;
	NumberFormat* numberFormatter
		= NumberFormat::createCurrencyInstance(*fICULocale, err);
	assert(err == U_ZERO_ERROR);
	DecimalFormat* decimalFormatter
		= dynamic_cast<DecimalFormat*>(numberFormatter);

	assert(decimalFormatter != NULL);

	const DecimalFormatSymbols* syms
		= decimalFormatter->getDecimalFormatSymbols();

	UnicodeString ICUString;
	ICUString = syms->getSymbol(DecimalFormatSymbols::kPatternSeparatorSymbol);

	BStringByteSink stringConverter(&separator);

	ICUString.toUTF8(stringConverter);

	return true;
}


bool
BCountry::MonGrouping(BString& grouping) const
{
	UErrorCode err;
	NumberFormat* numberFormatter
		= NumberFormat::createCurrencyInstance(*fICULocale, err);
	assert(err == U_ZERO_ERROR);
	DecimalFormat* decimalFormatter
		= dynamic_cast<DecimalFormat*>(numberFormatter);

	assert(decimalFormatter != NULL);

	const DecimalFormatSymbols* syms
		= decimalFormatter->getDecimalFormatSymbols();

	UnicodeString ICUString;
	ICUString = syms->getSymbol(
		DecimalFormatSymbols::kMonetaryGroupingSeparatorSymbol);

	BStringByteSink stringConverter(&grouping);

	ICUString.toUTF8(stringConverter);

	return true;
}


// TODO: is this possible to get from ICU ?
int32
BCountry::MonFracDigits() const
{
	return 2;
}


// #pragma mark - Timezones


status_t
BCountry::GetTimeZones(BMessage* timezones)
{
	if (timezones == NULL)
		return B_BAD_DATA;


	StringEnumeration* icuTimeZoneList = TimeZone::createEnumeration(
		fICULocale->getCountry());
	UErrorCode error = U_ZERO_ERROR;

	const char* tzName;
	while (tzName = icuTimeZoneList->next(NULL, error)) {
		if (error == U_ZERO_ERROR)
			timezones->AddString("zones", tzName);
		else
			error = U_ZERO_ERROR;
	}

	delete icuTimeZoneList;

	return B_OK;
}
