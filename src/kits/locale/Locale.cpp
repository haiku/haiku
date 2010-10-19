/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de.
** Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
** All rights reserved. Distributed under the terms of the OpenBeOS License.
*/


#include <AutoDeleter.h>
#include <Autolock.h>
#include <CalendarView.h>
#include <Catalog.h>
#include <CountryPrivate.h>
#include <LanguagePrivate.h>
#include <Locale.h>
#include <LocaleRoster.h>
#include <MutableLocaleRoster.h>
#include <TimeZone.h>

#include <unicode/datefmt.h>
#include <unicode/dcfmtsym.h>
#include <unicode/decimfmt.h>
#include <unicode/dtfmtsym.h>
#include <unicode/numfmt.h>
#include <unicode/smpdtfmt.h>
#include <unicode/ustring.h>
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
static void FetchDateFormat(bool longFormat, const Locale& locale,
				BString& format);
static void FetchTimeFormat(bool longFormat, const Locale& locale,
				BString& format);


BLocale::BLocale(const BLanguage* language, const BCountry* country)
{
	if (country != NULL)
		fCountry = *country;
	else
		be_locale->GetCountry(&fCountry);

	if (language != NULL)
		fLanguage = *language;
	else
		be_locale->GetLanguage(&fLanguage);

	_UpdateFormats();
}


BLocale::BLocale(const BLocale& other)
	:
	fCountry(other.fCountry),
	fLanguage(other.fLanguage),
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
BLocale::GetCountry(BCountry* country) const
{
	if (!country)
		return B_BAD_VALUE;

	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	*country = fCountry;

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
BLocale::SetCountry(const BCountry& newCountry)
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return;

	fCountry = newCountry;

	_UpdateFormats();
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


bool
BLocale::GetName(BString& name) const
{
	Locale icuLocale = Locale::createCanonical(fCountry.Code());
	if (icuLocale.isBogus())
		return false;

	UnicodeString uString;
	const Locale* countryLocale = BCountry::Private(&fCountry).ICULocale();
	countryLocale->getDisplayName(icuLocale, uString);
	BStringByteSink stringConverter(&name);
	uString.toUTF8(stringConverter);
	return true;
}


// #pragma mark - Date


status_t
BLocale::FormatDate(char* string, size_t maxSize, time_t time,
	bool longFormat) const
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	ObjectDeleter<DateFormat> dateFormatter = CreateDateFormat(longFormat,
		*BLanguage::Private(&fLanguage).ICULocale(),
		longFormat ? fLongDateFormat : fShortDateFormat);
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
BLocale::FormatDate(BString *string, time_t time, bool longFormat,
	const BTimeZone* timeZone) const
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	ObjectDeleter<DateFormat> dateFormatter = CreateDateFormat(longFormat,
		*BLanguage::Private(&fLanguage).ICULocale(),
		longFormat ? fLongDateFormat : fShortDateFormat);
	if (dateFormatter.Get() == NULL)
		return B_NO_MEMORY;

	if (timeZone != NULL) {
		ObjectDeleter<TimeZone> icuTimeZone
			= TimeZone::createTimeZone(timeZone->ID().String());
		if (icuTimeZone.Get() == NULL)
			return B_NO_MEMORY;
		dateFormatter->setTimeZone(*icuTimeZone.Get());
	}

	UnicodeString ICUString;
	ICUString = dateFormatter->format((UDate)time * 1000, ICUString);

	string->Truncate(0);
	BStringByteSink stringConverter(string);
	ICUString.toUTF8(stringConverter);

	return B_OK;
}


status_t
BLocale::FormatDate(BString* string, int*& fieldPositions, int& fieldCount,
	time_t time, bool longFormat) const
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	ObjectDeleter<DateFormat> dateFormatter = CreateDateFormat(longFormat,
		*BLanguage::Private(&fLanguage).ICULocale(),
		longFormat ? fLongDateFormat : fShortDateFormat);
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

	string->Truncate(0);
	BStringByteSink stringConverter(string);

	ICUString.toUTF8(stringConverter);

	return B_OK;
}


status_t
BLocale::GetDateFormat(BString& format, bool longFormat) const
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	if (longFormat && fLongDateFormat.Length() > 0)
		format = fLongDateFormat;
	else if (!longFormat && fShortDateFormat.Length() > 0)
		format = fShortDateFormat;
	else {
		format.Truncate(0);

		ObjectDeleter<DateFormat> dateFormatter = CreateDateFormat(longFormat,
			*BCountry::Private(&fCountry).ICULocale(), format);
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
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

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
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	ObjectDeleter<DateFormat> dateFormatter = CreateDateFormat(longFormat,
		*BCountry::Private(&fCountry).ICULocale(),
		longFormat ? fLongDateFormat : fShortDateFormat);
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
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	UErrorCode err = U_ZERO_ERROR;
	Calendar* c
		= Calendar::createInstance(*BCountry::Private(&fCountry).ICULocale(),
			err);

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
	bool longFormat) const
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	ObjectDeleter<DateFormat> dateFormatter = CreateDateFormat(longFormat,
		*BLanguage::Private(&fLanguage).ICULocale(),
		longFormat ? fLongDateFormat : fShortDateFormat);
	if (dateFormatter.Get() == NULL)
		return B_NO_MEMORY;

	ObjectDeleter<DateFormat> timeFormatter = CreateTimeFormat(longFormat,
		*BLanguage::Private(&fLanguage).ICULocale(),
		longFormat ? fLongTimeFormat : fShortTimeFormat);
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


status_t
BLocale::FormatDateTime(BString* target, time_t time, bool longFormat,
	const BTimeZone* timeZone) const
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	ObjectDeleter<DateFormat> dateFormatter = CreateDateFormat(longFormat,
		*BLanguage::Private(&fLanguage).ICULocale(),
		longFormat ? fLongDateFormat : fShortDateFormat);
	if (dateFormatter.Get() == NULL)
		return B_NO_MEMORY;

	ObjectDeleter<DateFormat> timeFormatter = CreateTimeFormat(longFormat,
		*BLanguage::Private(&fLanguage).ICULocale(),
		longFormat ? fLongTimeFormat : fShortTimeFormat);
	if (timeFormatter.Get() == NULL)
		return B_NO_MEMORY;

	if (timeZone != NULL) {
		ObjectDeleter<TimeZone> icuTimeZone
			= TimeZone::createTimeZone(timeZone->ID().String());
		if (icuTimeZone.Get() == NULL)
			return B_NO_MEMORY;
		timeFormatter->setTimeZone(*icuTimeZone.Get());
	}

	UnicodeString ICUString;
	ICUString = dateFormatter->format((UDate)time * 1000, ICUString);

	ICUString.append(UnicodeString::fromUTF8(", "));

	ICUString = timeFormatter->format((UDate)time * 1000, ICUString);

	target->Truncate(0);
	BStringByteSink stringConverter(target);
	ICUString.toUTF8(stringConverter);

	return B_OK;
}


// #pragma mark - Time


status_t
BLocale::FormatTime(char* string, size_t maxSize, time_t time,
	bool longFormat) const
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	ObjectDeleter<DateFormat> timeFormatter = CreateTimeFormat(longFormat,
		*BLanguage::Private(&fLanguage).ICULocale(),
		longFormat ? fLongTimeFormat : fShortTimeFormat);
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
BLocale::FormatTime(BString* string, time_t time, bool longFormat,
	const BTimeZone* timeZone) const
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	ObjectDeleter<DateFormat> timeFormatter = CreateTimeFormat(longFormat,
		*BLanguage::Private(&fLanguage).ICULocale(),
		longFormat ? fLongTimeFormat : fShortTimeFormat);
	if (timeFormatter.Get() == NULL)
		return B_NO_MEMORY;

	if (timeZone != NULL) {
		ObjectDeleter<TimeZone> icuTimeZone
			= TimeZone::createTimeZone(timeZone->ID().String());
		if (icuTimeZone.Get() == NULL)
			return B_NO_MEMORY;
		timeFormatter->setTimeZone(*icuTimeZone.Get());
	}

	UnicodeString ICUString;
	ICUString = timeFormatter->format((UDate)time * 1000, ICUString);

	string->Truncate(0);
	BStringByteSink stringConverter(string);

	ICUString.toUTF8(stringConverter);

	return B_OK;
}


status_t
BLocale::FormatTime(BString* string, int*& fieldPositions, int& fieldCount,
	time_t time, bool longFormat) const
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	ObjectDeleter<DateFormat> timeFormatter = CreateTimeFormat(longFormat,
		*BLanguage::Private(&fLanguage).ICULocale(),
		longFormat ? fLongTimeFormat : fShortTimeFormat);
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

	string->Truncate(0);
	BStringByteSink stringConverter(string);

	ICUString.toUTF8(stringConverter);

	return B_OK;
}


status_t
BLocale::GetTimeFields(BDateElement*& fields, int& fieldCount,
	bool longFormat) const
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	ObjectDeleter<DateFormat> timeFormatter = CreateTimeFormat(longFormat,
		*BCountry::Private(&fCountry).ICULocale(),
		longFormat ? fLongTimeFormat : fShortTimeFormat);
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
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	if (longFormat)
		fLongTimeFormat = formatString;
	else
		fShortTimeFormat = formatString;

	return B_OK;
}


status_t
BLocale::GetTimeFormat(BString& format, bool longFormat) const
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	if (longFormat && fLongTimeFormat.Length() > 0)
		format = fLongTimeFormat;
	else if (!longFormat && fShortTimeFormat.Length() > 0)
		format = fShortTimeFormat;
	else {
		format.Truncate(0);

		ObjectDeleter<DateFormat> timeFormatter = CreateTimeFormat(longFormat,
			*BCountry::Private(&fCountry).ICULocale(),
			longFormat ? fLongTimeFormat : fShortTimeFormat);
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


void
BLocale::_UpdateFormats()
{
	FetchDateFormat(true, *BCountry::Private(&fCountry).ICULocale(),
		this->fLongDateFormat);
	FetchDateFormat(false, *BCountry::Private(&fCountry).ICULocale(),
		this->fShortDateFormat);
	FetchTimeFormat(true, *BCountry::Private(&fCountry).ICULocale(),
		this->fLongTimeFormat);
	FetchTimeFormat(false, *BCountry::Private(&fCountry).ICULocale(),
		this->fShortTimeFormat);
}


// #pragma mark - Numbers


status_t
BLocale::FormatNumber(char* string, size_t maxSize, double value) const
{
	BString fullString;
	status_t status = FormatNumber(&fullString, value);
	if (status == B_OK)
		strlcpy(string, fullString.String(), maxSize);

	return status;
}


status_t
BLocale::FormatNumber(BString* string, double value) const
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	UErrorCode err = U_ZERO_ERROR;
	ObjectDeleter<NumberFormat> numberFormatter	= NumberFormat::createInstance(
		*BCountry::Private(&fCountry).ICULocale(), NumberFormat::kNumberStyle,
		err);

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
BLocale::FormatNumber(char* string, size_t maxSize, int32 value) const
{
	BString fullString;
	status_t status = FormatNumber(&fullString, value);
	if (status == B_OK)
		strlcpy(string, fullString.String(), maxSize);

	return status;
}


status_t
BLocale::FormatNumber(BString* string, int32 value) const
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	UErrorCode err = U_ZERO_ERROR;
	ObjectDeleter<NumberFormat> numberFormatter	= NumberFormat::createInstance(
		*BCountry::Private(&fCountry).ICULocale(), NumberFormat::kNumberStyle,
		err);

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
BLocale::FormatMonetary(char* string, size_t maxSize, double value) const
{
	BString fullString;
	ssize_t written = FormatMonetary(&fullString, value);
	if (written < 0)
		return written;

	return strlcpy(string, fullString.String(), maxSize);
}


ssize_t
BLocale::FormatMonetary(BString* string, double value) const
{
	if (string == NULL)
		return B_BAD_VALUE;

	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	UErrorCode err;
	ObjectDeleter<NumberFormat> numberFormatter
		= NumberFormat::createCurrencyInstance(
			*BCountry::Private(&fCountry).ICULocale(), err);

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


status_t
BLocale::FormatMonetary(BString* string, int*& fieldPositions,
	BNumberElement*& fieldTypes, int& fieldCount, double value) const
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	UErrorCode err = U_ZERO_ERROR;
	ObjectDeleter<NumberFormat> numberFormatter
		= NumberFormat::createCurrencyInstance(
			*BCountry::Private(&fCountry).ICULocale(), err);
	if (U_FAILURE(err))
		return B_NO_MEMORY;

	fieldPositions = NULL;
	fieldTypes = NULL;
	ICU_VERSION::FieldPositionIterator positionIterator;
	UnicodeString ICUString;
	ICUString = numberFormatter->format(value, ICUString, &positionIterator,
		err);

	if (err != U_ZERO_ERROR)
		return B_ERROR;

	ICU_VERSION::FieldPosition field;
	std::vector<int> fieldPosStorage;
	std::vector<int> fieldTypeStorage;
	fieldCount  = 0;
	while (positionIterator.next(field)) {
		fieldTypeStorage.push_back(field.getField());
		fieldPosStorage.push_back(field.getBeginIndex() | (field.getEndIndex() << 16));
		fieldCount ++;

	}

	fieldPositions = (int*) malloc(fieldCount * sizeof(int));
	fieldTypes = (BNumberElement*) malloc(fieldCount * sizeof(BNumberElement));

	for (int i = 0 ; i < fieldCount ; i++ ) {
		fieldPositions[i] = fieldPosStorage[i];
		switch (fieldTypeStorage[i]) {
			case NumberFormat::kCurrencyField:
				fieldTypes[i] = B_NUMBER_ELEMENT_CURRENCY;
				break;
			case NumberFormat::kIntegerField:
				fieldTypes[i] = B_NUMBER_ELEMENT_INTEGER;
				break;
			case NumberFormat::kFractionField:
				fieldTypes[i] = B_NUMBER_ELEMENT_FRACTIONAL;
				break;
			default:
				fieldTypes[i] = B_NUMBER_ELEMENT_INVALID;
				break;
		}
	}

	string->Truncate(0);
	BStringByteSink stringConverter(string);

	ICUString.toUTF8(stringConverter);

	return B_OK;
}


status_t
BLocale::GetCurrencySymbol(BString& result) const
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	UErrorCode error = U_ZERO_ERROR;
	NumberFormat* format = NumberFormat::createCurrencyInstance(
		*BCountry::Private(&fCountry).ICULocale(), error);

	if (U_FAILURE(error))
		return B_ERROR;

	char* symbol = (char*)malloc(20);
	u_strToUTF8(symbol, 20, NULL, format->getCurrency(), -1, &error);
	if (U_FAILURE(error))
		return B_BAD_DATA;
	result.Append(symbol);
	delete format;
	delete symbol;
	return B_OK;
}


// #pragma mark - Helpers


static DateFormat*
CreateDateFormat(bool longFormat, const Locale& locale,	const BString& format)
{
	DateFormat* dateFormatter = DateFormat::createDateInstance(
		longFormat ? DateFormat::kFull : DateFormat::kShort, locale);

	if (format.Length() > 0) {
		SimpleDateFormat* dateFormatterImpl
			= static_cast<SimpleDateFormat*>(dateFormatter);

		UnicodeString pattern(format.String());
		dateFormatterImpl->applyPattern(pattern);
	}

	return dateFormatter;
}


static DateFormat*
CreateTimeFormat(bool longFormat, const Locale& locale, const BString& format)
{
	DateFormat* timeFormatter = DateFormat::createTimeInstance(
		longFormat ? DateFormat::kMedium: DateFormat::kShort, locale);

	if (format.Length() > 0) {
		SimpleDateFormat* timeFormatterImpl
			= static_cast<SimpleDateFormat*>(timeFormatter);

		UnicodeString pattern(format.String());
		timeFormatterImpl->applyPattern(pattern);
	}

	return timeFormatter;
}


static void
FetchDateFormat(bool longFormat, const Locale& locale, BString& format)
{
	DateFormat* dateFormatter = DateFormat::createDateInstance(
		longFormat ? DateFormat::kFull : DateFormat::kShort, locale);

	SimpleDateFormat* dateFormatterImpl
		= static_cast<SimpleDateFormat*>(dateFormatter);

	UnicodeString pattern;
	dateFormatterImpl->toPattern(pattern);
	format.Truncate(0);
	BStringByteSink stringConverter(&format);
	pattern.toUTF8(stringConverter);
}


static void
FetchTimeFormat(bool longFormat, const Locale& locale, BString& format)
{
	DateFormat* timeFormatter = DateFormat::createTimeInstance(
		longFormat ? DateFormat::kMedium : DateFormat::kShort, locale);

	SimpleDateFormat* timeFormatterImpl
		= static_cast<SimpleDateFormat*>(timeFormatter);

	UnicodeString pattern;
	timeFormatterImpl->toPattern(pattern);
	format.Truncate(0);
	BStringByteSink stringConverter(&format);
	pattern.toUTF8(stringConverter);
}
