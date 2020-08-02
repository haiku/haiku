/*
 * Copyright 2010-2014, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */

#include <unicode/uversion.h>
#include <DateTimeFormat.h>

#include <AutoDeleter.h>
#include <Autolock.h>
#include <FormattingConventionsPrivate.h>
#include <LanguagePrivate.h>
#include <TimeZone.h>

#include <ICUWrapper.h>

#include <unicode/datefmt.h>
#include <unicode/dtptngen.h>
#include <unicode/smpdtfmt.h>


U_NAMESPACE_USE


BDateTimeFormat::BDateTimeFormat(const BLocale* locale)
	: BFormat(locale)
{
}


BDateTimeFormat::BDateTimeFormat(const BLanguage& language,
	const BFormattingConventions& conventions)
	: BFormat(language, conventions)
{
}


BDateTimeFormat::BDateTimeFormat(const BDateTimeFormat &other)
	: BFormat(other)
{
}


BDateTimeFormat::~BDateTimeFormat()
{
}


void
BDateTimeFormat::SetDateTimeFormat(BDateFormatStyle dateStyle,
	BTimeFormatStyle timeStyle, int32 elements) {
	UErrorCode error = U_ZERO_ERROR;
	DateTimePatternGenerator* generator
		= DateTimePatternGenerator::createInstance(
			*BLanguage::Private(&fLanguage).ICULocale(), error);

	BString skeleton;
	if (elements & B_DATE_ELEMENT_YEAR)
		skeleton << "yyyy";
	if (elements & B_DATE_ELEMENT_MONTH)
		skeleton << "MM";
	if (elements & B_DATE_ELEMENT_WEEKDAY)
		skeleton << "eee";
	if (elements & B_DATE_ELEMENT_DAY)
		skeleton << "dd";
	if (elements & B_DATE_ELEMENT_AM_PM)
		skeleton << "a";
	if (elements & B_DATE_ELEMENT_HOUR) {
		if (fConventions.Use24HourClock())
			skeleton << "k";
		else
			skeleton << "K";
	}
	if (elements & B_DATE_ELEMENT_MINUTE)
		skeleton << "mm";
	if (elements & B_DATE_ELEMENT_SECOND)
		skeleton << "ss";
	if (elements & B_DATE_ELEMENT_TIMEZONE)
		skeleton << "z";

	UnicodeString pattern = generator->getBestPattern(
		UnicodeString::fromUTF8(skeleton.String()), error);

	BString buffer;
	BStringByteSink stringConverter(&buffer);
	pattern.toUTF8(stringConverter);

	fConventions.SetExplicitDateTimeFormat(dateStyle, timeStyle, buffer);

	delete generator;
}


// #pragma mark - Formatting


ssize_t
BDateTimeFormat::Format(char* target, size_t maxSize, time_t time,
	BDateFormatStyle dateStyle, BTimeFormatStyle timeStyle) const
{
	BString format;
	fConventions.GetDateTimeFormat(dateStyle, timeStyle, format);
	ObjectDeleter<DateFormat> dateFormatter(_CreateDateTimeFormatter(format));
	if (dateFormatter.Get() == NULL)
		return B_NO_MEMORY;

	UnicodeString icuString;
	dateFormatter->format((UDate)time * 1000, icuString);

	CheckedArrayByteSink stringConverter(target, maxSize);
	icuString.toUTF8(stringConverter);

	if (stringConverter.Overflowed())
		return B_BAD_VALUE;

	return stringConverter.NumberOfBytesWritten();
}


status_t
BDateTimeFormat::Format(BString& target, const time_t time,
	BDateFormatStyle dateStyle, BTimeFormatStyle timeStyle,
	const BTimeZone* timeZone) const
{
	BString format;
	fConventions.GetDateTimeFormat(dateStyle, timeStyle, format);
	ObjectDeleter<DateFormat> dateFormatter(_CreateDateTimeFormatter(format));
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

	target.Truncate(0);
	BStringByteSink stringConverter(&target);
	icuString.toUTF8(stringConverter);

	return B_OK;
}


DateFormat*
BDateTimeFormat::_CreateDateTimeFormatter(const BString& format) const
{
	Locale* icuLocale
		= fConventions.UseStringsFromPreferredLanguage()
			? BLanguage::Private(&fLanguage).ICULocale()
			: BFormattingConventions::Private(&fConventions).ICULocale();

	icu::DateFormat* dateFormatter = icu::DateFormat::createDateTimeInstance(
		DateFormat::kDefault, DateFormat::kDefault, *icuLocale);
	if (dateFormatter == NULL)
		return NULL;

	SimpleDateFormat* dateFormatterImpl
		= static_cast<SimpleDateFormat*>(dateFormatter);

	UnicodeString pattern(format.String());
	dateFormatterImpl->applyPattern(pattern);

	return dateFormatter;
}
