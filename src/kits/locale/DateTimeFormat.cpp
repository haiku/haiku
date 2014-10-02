/*
 * Copyright 2010-2014, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */

#include <DateTimeFormat.h>

#include <AutoDeleter.h>
#include <Autolock.h>
#include <FormattingConventionsPrivate.h>
#include <LanguagePrivate.h>
#include <TimeZone.h>

#include <ICUWrapper.h>

#include <unicode/datefmt.h>
#include <unicode/smpdtfmt.h>


BDateTimeFormat::BDateTimeFormat()
	: BFormat()
{
}


BDateTimeFormat::BDateTimeFormat(const BDateTimeFormat &other)
	: BFormat(other)
{
}


BDateTimeFormat::~BDateTimeFormat()
{
}


// #pragma mark - Formatting


ssize_t
BDateTimeFormat::Format(char* target, size_t maxSize, time_t time,
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
BDateTimeFormat::Format(BString& target, const time_t time,
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

	target.Truncate(0);
	BStringByteSink stringConverter(&target);
	icuString.toUTF8(stringConverter);

	return B_OK;
}


DateFormat*
BDateTimeFormat::_CreateDateFormatter(const BString& format) const
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


DateFormat*
BDateTimeFormat::_CreateTimeFormatter(const BString& format) const
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

	UnicodeString pattern(format.String());
	timeFormatterImpl->applyPattern(pattern);

	return timeFormatter;
}
