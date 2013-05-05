/*
 * Copyright 2003-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2009-2010, Adrien Destugues, pulkomandy@gmail.com.
 * Copyright 2010-2011, Oliver Tappe <zooey@hirschkaefer.de>.
 * Distributed under the terms of the MIT License.
 */


#include <FormattingConventions.h>

#include <AutoDeleter.h>
#include <IconUtils.h>
#include <List.h>
#include <Language.h>
#include <Locale.h>
#include <LocaleRoster.h>
#include <Resources.h>
#include <String.h>
#include <UnicodeChar.h>

#include <unicode/datefmt.h>
#include <unicode/locid.h>
#include <unicode/smpdtfmt.h>
#include <unicode/ulocdata.h>
#include <ICUWrapper.h>

#include <iostream>
#include <map>
#include <monetary.h>
#include <new>
#include <stdarg.h>
#include <stdlib.h>


// #pragma mark - helpers


static bool
FormatUsesAmPm(const BString& format)
{
	if (format.Length() == 0)
		return false;

	bool inQuote = false;
	for (const char* s = format.String(); *s != '\0'; ++s) {
		switch (*s) {
			case '\'':
				inQuote = !inQuote;
				break;
			case 'a':
				if (!inQuote)
					return true;
				break;
		}
	}

	return false;
}


static void
CoerceFormatTo12HourClock(BString& format)
{
	char* s = format.LockBuffer(format.Length());
	if (s == NULL)
		return;

	// change format to use h instead of H, k instead of K, and append an
	// am/pm marker
	bool inQuote = false;
	for (; *s != '\0'; ++s) {
		switch (*s) {
			case '\'':
				inQuote = !inQuote;
				break;
			case 'H':
				if (!inQuote)
					*s = 'h';
				break;
			case 'K':
				if (!inQuote)
					*s = 'k';
				break;
		}
	}
	format.UnlockBuffer(format.Length());

	format.Append(" a");
}


static void
CoerceFormatTo24HourClock(BString& format)
{
	char* buffer = format.LockBuffer(format.Length());
	char* currentPos = buffer;
	if (currentPos == NULL)
		return;

	// change the format to use H instead of h, K instead of k, and determine
	// and remove the am/pm marker (including leading whitespace)
	bool inQuote = false;
	bool lastWasWhitespace = false;
	uint32 ch;
	const char* amPmStartPos = NULL;
	const char* amPmEndPos = NULL;
	const char* lastWhitespaceStart = NULL;
	for (char* previousPos = currentPos; (ch = BUnicodeChar::FromUTF8(
			(const char**)&currentPos)) != 0; previousPos = currentPos) {
		switch (ch) {
			case '\'':
				inQuote = !inQuote;
				break;
			case 'h':
				if (!inQuote)
					*previousPos = 'H';
				break;
			case 'k':
				if (!inQuote)
					*previousPos = 'K';
				break;
			case 'a':
				if (!inQuote) {
					if (lastWasWhitespace)
						amPmStartPos = lastWhitespaceStart;
					else
						amPmStartPos = previousPos;
					amPmEndPos = currentPos;
				}
				break;
			default:
				if (!inQuote && BUnicodeChar::IsWhitespace(ch)) {
					if (!lastWasWhitespace) {
						lastWhitespaceStart = previousPos;
						lastWasWhitespace = true;
					}
					continue;
				}
		}
		lastWasWhitespace = false;
	}

	format.UnlockBuffer(format.Length());
	if (amPmStartPos != NULL && amPmEndPos > amPmStartPos)
		format.Remove(amPmStartPos - buffer, amPmEndPos - amPmStartPos);
}


static void
CoerceFormatToAbbreviatedTimezone(BString& format)
{
	char* s = format.LockBuffer(format.Length());
	if (s == NULL)
		return;

	// replace a single 'z' with 'V'
	bool inQuote = false;
	bool lastWasZ = false;
	for (; *s != '\0'; ++s) {
		switch (*s) {
			case '\'':
				inQuote = !inQuote;
				break;
			case 'z':
				if (!inQuote && !lastWasZ && *(s+1) != 'z')
					*s = 'V';
				lastWasZ = true;
				continue;
		}
		lastWasZ = false;
	}
	format.UnlockBuffer(format.Length());
}


// #pragma mark - BFormattingConventions


enum ClockHoursState {
	CLOCK_HOURS_UNSET = 0,
	CLOCK_HOURS_24,
	CLOCK_HOURS_12
};


BFormattingConventions::BFormattingConventions(const char* id)
	:
	fCachedUse24HourClock(CLOCK_HOURS_UNSET),
	fExplicitUse24HourClock(CLOCK_HOURS_UNSET),
	fUseStringsFromPreferredLanguage(false),
	fICULocale(new icu::Locale(id))
{
}


BFormattingConventions::BFormattingConventions(
	const BFormattingConventions& other)
	:
	fCachedNumericFormat(other.fCachedNumericFormat),
	fCachedMonetaryFormat(other.fCachedMonetaryFormat),
	fCachedUse24HourClock(other.fCachedUse24HourClock),
	fExplicitNumericFormat(other.fExplicitNumericFormat),
	fExplicitMonetaryFormat(other.fExplicitMonetaryFormat),
	fExplicitUse24HourClock(other.fExplicitUse24HourClock),
	fUseStringsFromPreferredLanguage(other.fUseStringsFromPreferredLanguage),
	fICULocale(new icu::Locale(*other.fICULocale))
{
	for (int s = 0; s < B_DATE_FORMAT_STYLE_COUNT; ++s)
		fCachedDateFormats[s] = other.fCachedDateFormats[s];
	for (int s = 0; s < B_TIME_FORMAT_STYLE_COUNT; ++s)
		fCachedTimeFormats[s] = other.fCachedTimeFormats[s];
	for (int s = 0; s < B_DATE_FORMAT_STYLE_COUNT; ++s)
		fExplicitDateFormats[s] = other.fExplicitDateFormats[s];
	for (int s = 0; s < B_TIME_FORMAT_STYLE_COUNT; ++s)
		fExplicitTimeFormats[s] = other.fExplicitTimeFormats[s];
}


BFormattingConventions::BFormattingConventions(const BMessage* archive)
	:
	fCachedUse24HourClock(CLOCK_HOURS_UNSET),
	fExplicitUse24HourClock(CLOCK_HOURS_UNSET),
	fUseStringsFromPreferredLanguage(false)
{
	BString conventionsID;
	status_t status = archive->FindString("conventions", &conventionsID);
	fICULocale = new icu::Locale(conventionsID);

	for (int s = 0; s < B_DATE_FORMAT_STYLE_COUNT && status == B_OK; ++s) {
		BString format;
		status = archive->FindString("dateFormat", s, &format);
		if (status == B_OK)
			fExplicitDateFormats[s] = format;

		status = archive->FindString("timeFormat", s, &format);
		if (status == B_OK)
			fExplicitTimeFormats[s] = format;
	}

	if (status == B_OK) {
		int8 use24HourClock;
		status = archive->FindInt8("use24HourClock", &use24HourClock);
		if (status == B_OK)
			fExplicitUse24HourClock = use24HourClock;
	}
	if (status == B_OK) {
		bool useStringsFromPreferredLanguage;
		status = archive->FindBool("useStringsFromPreferredLanguage",
			&useStringsFromPreferredLanguage);
		if (status == B_OK)
			fUseStringsFromPreferredLanguage = useStringsFromPreferredLanguage;
	}
}


BFormattingConventions&
BFormattingConventions::operator=(const BFormattingConventions& other)
{
	if (this == &other)
		return *this;

	for (int s = 0; s < B_DATE_FORMAT_STYLE_COUNT; ++s)
		fCachedDateFormats[s] = other.fCachedDateFormats[s];
	for (int s = 0; s < B_TIME_FORMAT_STYLE_COUNT; ++s)
		fCachedTimeFormats[s] = other.fCachedTimeFormats[s];
	fCachedNumericFormat = other.fCachedNumericFormat;
	fCachedMonetaryFormat = other.fCachedMonetaryFormat;
	fCachedUse24HourClock = other.fCachedUse24HourClock;

	for (int s = 0; s < B_DATE_FORMAT_STYLE_COUNT; ++s)
		fExplicitDateFormats[s] = other.fExplicitDateFormats[s];
	for (int s = 0; s < B_TIME_FORMAT_STYLE_COUNT; ++s)
		fExplicitTimeFormats[s] = other.fExplicitTimeFormats[s];
	fExplicitNumericFormat = other.fExplicitNumericFormat;
	fExplicitMonetaryFormat = other.fExplicitMonetaryFormat;
	fExplicitUse24HourClock = other.fExplicitUse24HourClock;

	fUseStringsFromPreferredLanguage = other.fUseStringsFromPreferredLanguage;

	*fICULocale = *other.fICULocale;

	return *this;
}


BFormattingConventions::~BFormattingConventions()
{
	delete fICULocale;
}


bool
BFormattingConventions::operator==(const BFormattingConventions& other) const
{
	if (this == &other)
		return true;

	for (int s = 0; s < B_DATE_FORMAT_STYLE_COUNT; ++s) {
		if (fExplicitDateFormats[s] != other.fExplicitDateFormats[s])
			return false;
	}
	for (int s = 0; s < B_TIME_FORMAT_STYLE_COUNT; ++s) {
		if (fExplicitTimeFormats[s] != other.fExplicitTimeFormats[s])
			return false;
	}

	return fExplicitNumericFormat == other.fExplicitNumericFormat
		&& fExplicitMonetaryFormat == other.fExplicitMonetaryFormat
		&& fExplicitUse24HourClock == other.fExplicitUse24HourClock
		&& fUseStringsFromPreferredLanguage
			== other.fUseStringsFromPreferredLanguage
		&& *fICULocale == *other.fICULocale;
}


bool
BFormattingConventions::operator!=(const BFormattingConventions& other) const
{
	return !(*this == other);
}


const char*
BFormattingConventions::ID() const
{
	return fICULocale->getName();
}


const char*
BFormattingConventions::LanguageCode() const
{
	return fICULocale->getLanguage();
}


const char*
BFormattingConventions::CountryCode() const
{
	const char* country = fICULocale->getCountry();
	if (country == NULL || country[0] == '\0')
		return NULL;

	return country;
}


bool
BFormattingConventions::AreCountrySpecific() const
{
	return CountryCode() != NULL;
}


status_t
BFormattingConventions::GetNativeName(BString& name) const
{
	UnicodeString string;
	fICULocale->getDisplayName(*fICULocale, string);
	string.toTitle(NULL, *fICULocale);

	name.Truncate(0);
	BStringByteSink converter(&name);
	string.toUTF8(converter);

	return B_OK;
}


status_t
BFormattingConventions::GetName(BString& name,
	const BLanguage* displayLanguage) const
{
	BString displayLanguageID;
	if (displayLanguage == NULL) {
		BLanguage defaultLanguage;
		BLocale::Default()->GetLanguage(&defaultLanguage);
		displayLanguageID = defaultLanguage.Code();
	} else {
		displayLanguageID = displayLanguage->Code();
	}

	UnicodeString uString;
	fICULocale->getDisplayName(Locale(displayLanguageID.String()), uString);
	name.Truncate(0);
	BStringByteSink stringConverter(&name);
	uString.toUTF8(stringConverter);

	return B_OK;
}


BMeasurementKind
BFormattingConventions::MeasurementKind() const
{
	UErrorCode error = U_ZERO_ERROR;
	switch (ulocdata_getMeasurementSystem(ID(), &error)) {
		case UMS_US:
			return B_US;
		case UMS_SI:
		default:
			return B_METRIC;
	}
}


status_t
BFormattingConventions::GetDateFormat(BDateFormatStyle style,
	BString& outFormat) const
{
	if (style < 0 || style >= B_DATE_FORMAT_STYLE_COUNT)
		return B_BAD_VALUE;

	outFormat = fExplicitDateFormats[style].Length()
		? fExplicitDateFormats[style]
		: fCachedDateFormats[style];

	if (outFormat.Length() > 0)
		return B_OK;

	ObjectDeleter<DateFormat> dateFormatter(
		DateFormat::createDateInstance((DateFormat::EStyle)style, *fICULocale));
	if (dateFormatter.Get() == NULL)
		return B_NO_MEMORY;

	SimpleDateFormat* dateFormatterImpl
		= static_cast<SimpleDateFormat*>(dateFormatter.Get());

	UnicodeString icuString;
	dateFormatterImpl->toPattern(icuString);
	BStringByteSink stringConverter(&outFormat);
	icuString.toUTF8(stringConverter);

	fCachedDateFormats[style] = outFormat;

	return B_OK;
}


status_t
BFormattingConventions::GetTimeFormat(BTimeFormatStyle style,
	BString& outFormat) const
{
	if (style < 0 || style >= B_TIME_FORMAT_STYLE_COUNT)
		return B_BAD_VALUE;

	outFormat = fExplicitTimeFormats[style].Length()
		? fExplicitTimeFormats[style]
		: fCachedTimeFormats[style];

	if (outFormat.Length() > 0)
		return B_OK;

	ObjectDeleter<DateFormat> timeFormatter(
		DateFormat::createTimeInstance((DateFormat::EStyle)style, *fICULocale));
	if (timeFormatter.Get() == NULL)
		return B_NO_MEMORY;

	SimpleDateFormat* timeFormatterImpl
		= static_cast<SimpleDateFormat*>(timeFormatter.Get());

	UnicodeString icuString;
	timeFormatterImpl->toPattern(icuString);
	BStringByteSink stringConverter(&outFormat);
	icuString.toUTF8(stringConverter);

	int8 use24HourClock = fExplicitUse24HourClock != CLOCK_HOURS_UNSET
		? fExplicitUse24HourClock : fCachedUse24HourClock;
	if (use24HourClock != CLOCK_HOURS_UNSET) {
		// adjust to 12/24-hour clock as requested
		bool localeUses24HourClock = !FormatUsesAmPm(outFormat);
		if (localeUses24HourClock) {
			if (use24HourClock == CLOCK_HOURS_12)
				CoerceFormatTo12HourClock(outFormat);
		} else {
			if (use24HourClock == CLOCK_HOURS_24)
				CoerceFormatTo24HourClock(outFormat);
		}
	}

	if (style != B_FULL_TIME_FORMAT) {
		// use abbreviated timezone in short timezone format
		CoerceFormatToAbbreviatedTimezone(outFormat);
	}

	fCachedTimeFormats[style] = outFormat;

	return B_OK;
}


status_t
BFormattingConventions::GetNumericFormat(BString& outFormat) const
{
	// TODO!
	return B_UNSUPPORTED;
}


status_t
BFormattingConventions::GetMonetaryFormat(BString& outFormat) const
{
	// TODO!
	return B_UNSUPPORTED;
}


void
BFormattingConventions::SetExplicitDateFormat(BDateFormatStyle style,
	const BString& format)
{
	fExplicitDateFormats[style] = format;
}


void
BFormattingConventions::SetExplicitTimeFormat(BTimeFormatStyle style,
	const BString& format)
{
	fExplicitTimeFormats[style] = format;
}


void
BFormattingConventions::SetExplicitNumericFormat(const BString& format)
{
	fExplicitNumericFormat = format;
}


void
BFormattingConventions::SetExplicitMonetaryFormat(const BString& format)
{
	fExplicitMonetaryFormat = format;
}


bool
BFormattingConventions::UseStringsFromPreferredLanguage() const
{
	return fUseStringsFromPreferredLanguage;
}


void
BFormattingConventions::SetUseStringsFromPreferredLanguage(bool value)
{
	fUseStringsFromPreferredLanguage = value;
}


bool
BFormattingConventions::Use24HourClock() const
{
	int8 use24HourClock = fExplicitUse24HourClock != CLOCK_HOURS_UNSET
		?  fExplicitUse24HourClock : fCachedUse24HourClock;

	if (use24HourClock == CLOCK_HOURS_UNSET) {
		BString format;
		GetTimeFormat(B_MEDIUM_TIME_FORMAT, format);
		fCachedUse24HourClock
			= FormatUsesAmPm(format) ? CLOCK_HOURS_12 : CLOCK_HOURS_24;
		return fCachedUse24HourClock == CLOCK_HOURS_24;
	}

	return fExplicitUse24HourClock == CLOCK_HOURS_24;
}


void
BFormattingConventions::SetExplicitUse24HourClock(bool value)
{
	int8 newUse24HourClock = value ? CLOCK_HOURS_24 : CLOCK_HOURS_12;
	if (fExplicitUse24HourClock == newUse24HourClock)
		return;

	fExplicitUse24HourClock = newUse24HourClock;

	for (int s = 0; s < B_TIME_FORMAT_STYLE_COUNT; ++s)
		fCachedTimeFormats[s].Truncate(0);
}


void
BFormattingConventions::UnsetExplicitUse24HourClock()
{
	fExplicitUse24HourClock = CLOCK_HOURS_UNSET;

	for (int s = 0; s < B_TIME_FORMAT_STYLE_COUNT; ++s)
		fCachedTimeFormats[s].Truncate(0);
}


status_t
BFormattingConventions::Archive(BMessage* archive, bool deep) const
{
	status_t status = archive->AddString("conventions", fICULocale->getName());
	for (int s = 0; s < B_DATE_FORMAT_STYLE_COUNT && status == B_OK; ++s) {
		status = archive->AddString("dateFormat", fExplicitDateFormats[s]);
		if (status == B_OK)
			status = archive->AddString("timeFormat", fExplicitTimeFormats[s]);
	}
	if (status == B_OK)
		status = archive->AddInt8("use24HourClock", fExplicitUse24HourClock);
	if (status == B_OK) {
		status = archive->AddBool("useStringsFromPreferredLanguage",
			fUseStringsFromPreferredLanguage);
	}

	return status;
}
