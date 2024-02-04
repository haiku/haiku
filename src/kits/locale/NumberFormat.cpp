/*
 * Copyright 2003, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2010-2011, Oliver Tappe, zooey@hirschkaefer.de.
 * Copyright 2012, John Scipione, jscipione@gmail.com
 * Copyright 2017, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Copyright 2021, Andrew Lindesay, apl@lindesay.co.nz
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <unicode/uversion.h>
#include <NumberFormat.h>

#include <AutoDeleter.h>
#include <Autolock.h>
#include <FormattingConventionsPrivate.h>

#include <ICUWrapper.h>

#include <unicode/dcfmtsym.h>
#include <unicode/decimfmt.h>
#include <unicode/numfmt.h>


U_NAMESPACE_USE


class BNumberFormatImpl {
public:
					BNumberFormatImpl();
					~BNumberFormatImpl();

	NumberFormat*	GetInteger(BFormattingConventions* convention);
	NumberFormat*	GetFloat(BFormattingConventions* convention);
	NumberFormat*	GetCurrency(BFormattingConventions* convention);
	NumberFormat*	GetPercent(BFormattingConventions* convention);

	ssize_t			ApplyFormatter(NumberFormat* formatter, char* string,
						size_t maxSize, const double value);
	status_t		ApplyFormatter(NumberFormat* formatter, BString& string,
						const double value);

private:
	NumberFormat*	fIntegerFormat;
	NumberFormat*	fFloatFormat;
	NumberFormat*	fCurrencyFormat;
	NumberFormat*	fPercentFormat;
};


BNumberFormatImpl::BNumberFormatImpl()
{
	// They are initialized lazily as needed
	fIntegerFormat = NULL;
	fFloatFormat = NULL;
	fCurrencyFormat = NULL;
	fPercentFormat = NULL;
}


BNumberFormatImpl::~BNumberFormatImpl()
{
	delete fIntegerFormat;
	delete fFloatFormat;
	delete fCurrencyFormat;
	delete fPercentFormat;
}


NumberFormat*
BNumberFormatImpl::GetInteger(BFormattingConventions* convention)
{
	if (fIntegerFormat == NULL) {
		UErrorCode err = U_ZERO_ERROR;
		fIntegerFormat = NumberFormat::createInstance(
			*BFormattingConventions::Private(convention).ICULocale(),
			UNUM_DECIMAL, err);

		if (fIntegerFormat == NULL)
			return NULL;
		if (U_FAILURE(err)) {
			delete fIntegerFormat;
			fIntegerFormat = NULL;
			return NULL;
		}
	}

	return fIntegerFormat;
}


NumberFormat*
BNumberFormatImpl::GetFloat(BFormattingConventions* convention)
{
	if (fFloatFormat == NULL) {
		UErrorCode err = U_ZERO_ERROR;
		fFloatFormat = NumberFormat::createInstance(
			*BFormattingConventions::Private(convention).ICULocale(),
			UNUM_DECIMAL, err);

		if (fFloatFormat == NULL)
			return NULL;
		if (U_FAILURE(err)) {
			delete fFloatFormat;
			fFloatFormat = NULL;
			return NULL;
		}
	}

	return fFloatFormat;
}


NumberFormat*
BNumberFormatImpl::GetCurrency(BFormattingConventions* convention)
{
	if (fCurrencyFormat == NULL) {
		UErrorCode err = U_ZERO_ERROR;
		fCurrencyFormat = NumberFormat::createCurrencyInstance(
			*BFormattingConventions::Private(convention).ICULocale(),
			err);

		if (fCurrencyFormat == NULL)
			return NULL;
		if (U_FAILURE(err)) {
			delete fCurrencyFormat;
			fCurrencyFormat = NULL;
			return NULL;
		}
	}

	return fCurrencyFormat;
}


NumberFormat*
BNumberFormatImpl::GetPercent(BFormattingConventions* convention)
{
	if (fPercentFormat == NULL) {
		UErrorCode err = U_ZERO_ERROR;
		fPercentFormat = NumberFormat::createInstance(
			*BFormattingConventions::Private(convention).ICULocale(),
			UNUM_PERCENT, err);

		if (fPercentFormat == NULL)
			return NULL;
		if (U_FAILURE(err)) {
			delete fPercentFormat;
			fPercentFormat = NULL;
			return NULL;
		}
	}

	return fPercentFormat;
}


ssize_t
BNumberFormatImpl::ApplyFormatter(NumberFormat* formatter, char* string,
	size_t maxSize, const double value)
{
	BString fullString;
	status_t status = ApplyFormatter(formatter, fullString, value);
	if (status != B_OK)
		return status;

	return strlcpy(string, fullString.String(), maxSize);
}


status_t
BNumberFormatImpl::ApplyFormatter(NumberFormat* formatter, BString& string,
	const double value)
{
	if (formatter == NULL)
		return B_NO_MEMORY;

	UnicodeString icuString;
	formatter->format(value, icuString);

	string.Truncate(0);
	BStringByteSink stringConverter(&string);
	icuString.toUTF8(stringConverter);

	return B_OK;
}


BNumberFormat::BNumberFormat()
	: BFormat()
{
	fPrivateData = new BNumberFormatImpl();
}


BNumberFormat::BNumberFormat(const BLocale* locale)
	: BFormat(locale)
{
	fPrivateData = new BNumberFormatImpl();
}


BNumberFormat::BNumberFormat(const BNumberFormat &other)
	: BFormat(other)
{
	fPrivateData = new BNumberFormatImpl(*other.fPrivateData);
}


BNumberFormat::~BNumberFormat()
{
	delete fPrivateData;
}


// #pragma mark - Formatting


ssize_t
BNumberFormat::Format(char* string, size_t maxSize, const double value)
{
	BString fullString;
	status_t status = Format(fullString, value);
	if (status != B_OK)
		return status;

	return strlcpy(string, fullString.String(), maxSize);
}


status_t
BNumberFormat::Format(BString& string, const double value)
{
	NumberFormat* formatter = fPrivateData->GetFloat(&fConventions);

	if (formatter == NULL)
		return B_NO_MEMORY;

	UnicodeString icuString;
	formatter->format(value, icuString);

	string.Truncate(0);
	BStringByteSink stringConverter(&string);
	icuString.toUTF8(stringConverter);

	return B_OK;
}


ssize_t
BNumberFormat::Format(char* string, size_t maxSize, const int32 value)
{
	BString fullString;
	status_t status = Format(fullString, value);
	if (status != B_OK)
		return status;

	return strlcpy(string, fullString.String(), maxSize);
}


status_t
BNumberFormat::Format(BString& string, const int32 value)
{
	NumberFormat* formatter = fPrivateData->GetInteger(&fConventions);

	if (formatter == NULL)
		return B_NO_MEMORY;

	UnicodeString icuString;
	formatter->format((int32_t)value, icuString);

	string.Truncate(0);
	BStringByteSink stringConverter(&string);
	icuString.toUTF8(stringConverter);

	return B_OK;
}


status_t
BNumberFormat::SetPrecision(int precision)
{
	NumberFormat* decimalFormatter = fPrivateData->GetFloat(&fConventions);
	NumberFormat* currencyFormatter = fPrivateData->GetCurrency(&fConventions);
	NumberFormat* percentFormatter = fPrivateData->GetPercent(&fConventions);

	if ((decimalFormatter == NULL) || (currencyFormatter == NULL) || (percentFormatter == NULL))
		return B_ERROR;

	decimalFormatter->setMinimumFractionDigits(precision);
	decimalFormatter->setMaximumFractionDigits(precision);

	currencyFormatter->setMinimumFractionDigits(precision);
	currencyFormatter->setMaximumFractionDigits(precision);

	percentFormatter->setMinimumFractionDigits(precision);
	percentFormatter->setMaximumFractionDigits(precision);

	return B_OK;
}


ssize_t
BNumberFormat::FormatMonetary(char* string, size_t maxSize, const double value)
{
	return fPrivateData->ApplyFormatter(
		fPrivateData->GetCurrency(&fConventions), string, maxSize, value);
}


status_t
BNumberFormat::FormatMonetary(BString& string, const double value)
{
	return fPrivateData->ApplyFormatter(
		fPrivateData->GetCurrency(&fConventions), string, value);
}


ssize_t
BNumberFormat::FormatPercent(char* string, size_t maxSize, const double value)
{
	return fPrivateData->ApplyFormatter(
		fPrivateData->GetPercent(&fConventions), string, maxSize, value);
}


status_t
BNumberFormat::FormatPercent(BString& string, const double value)
{
	return fPrivateData->ApplyFormatter(
		fPrivateData->GetPercent(&fConventions), string, value);
}


status_t
BNumberFormat::Parse(const BString& string, double& value)
{
	NumberFormat* parser = fPrivateData->GetFloat(&fConventions);

	if (parser == NULL)
		return B_NO_MEMORY;

	UnicodeString unicode(string.String());
	Formattable result(0);
	UErrorCode err = U_ZERO_ERROR;

	parser->parse(unicode, result, err);

	if (err != U_ZERO_ERROR)
		return B_BAD_DATA;

	value = result.getDouble();

	return B_OK;
}


BString
BNumberFormat::GetSeparator(BNumberElement element)
{
	DecimalFormatSymbols::ENumberFormatSymbol symbol;
	BString result;

	switch(element) {
		case B_DECIMAL_SEPARATOR:
			symbol = DecimalFormatSymbols::kDecimalSeparatorSymbol;
			break;
		case B_GROUPING_SEPARATOR:
			symbol = DecimalFormatSymbols::kGroupingSeparatorSymbol;
			break;

		default:
			return result;
	}

	NumberFormat* format = fPrivateData->GetFloat(&fConventions);
	DecimalFormat* decimal = dynamic_cast<DecimalFormat*>(format);

	if (decimal == NULL)
		return result;

	const DecimalFormatSymbols* symbols = decimal->getDecimalFormatSymbols();
	UnicodeString string = symbols->getSymbol(symbol);

	BStringByteSink stringConverter(&result);
	string.toUTF8(stringConverter);

	return result;
}
