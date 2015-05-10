/*
 * Copyright 2003, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2010-2011, Oliver Tappe, zooey@hirschkaefer.de.
 * Copyright 2012, John Scipione, jscipione@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <unicode/uversion.h>
#include <NumberFormat.h>

#include <AutoDeleter.h>
#include <Autolock.h>
#include <FormattingConventionsPrivate.h>

#include <ICUWrapper.h>

#include <unicode/numfmt.h>


BNumberFormat::BNumberFormat()
	: BFormat()
{
}


BNumberFormat::BNumberFormat(const BNumberFormat &other)
	: BFormat(other)
{
}


BNumberFormat::~BNumberFormat()
{
}


// #pragma mark - Formatting


ssize_t
BNumberFormat::Format(char* string, size_t maxSize, const double value) const
{
	BString fullString;
	status_t status = Format(fullString, value);
	if (status != B_OK)
		return status;

	return strlcpy(string, fullString.String(), maxSize);
}


status_t
BNumberFormat::Format(BString& string, const double value) const
{
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

	string.Truncate(0);
	BStringByteSink stringConverter(&string);
	icuString.toUTF8(stringConverter);

	return B_OK;
}


ssize_t
BNumberFormat::Format(char* string, size_t maxSize, const int32 value) const
{
	BString fullString;
	status_t status = Format(fullString, value);
	if (status != B_OK)
		return status;

	return strlcpy(string, fullString.String(), maxSize);
}


status_t
BNumberFormat::Format(BString& string, const int32 value) const
{
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

	string.Truncate(0);
	BStringByteSink stringConverter(&string);
	icuString.toUTF8(stringConverter);

	return B_OK;
}


ssize_t
BNumberFormat::FormatMonetary(char* string, size_t maxSize, const double value)
	const
{
	BString fullString;
	status_t status = FormatMonetary(fullString, value);
	if (status != B_OK)
		return status;

	return strlcpy(string, fullString.String(), maxSize);
}


status_t
BNumberFormat::FormatMonetary(BString& string, const double value) const
{
	if (string == NULL)
		return B_BAD_VALUE;

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

	string.Truncate(0);
	BStringByteSink stringConverter(&string);
	icuString.toUTF8(stringConverter);

	return B_OK;
}
