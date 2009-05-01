/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <Country.h>
#include <String.h>

#include <monetary.h>
#include <stdarg.h>


const char *gStrings[] = {
	// date/time format
	"",
	"",
	"",
	"",
	// short date/time format
	"",
	"",
	"",
	"",
	// am/pm string
	"AM",
	"PM",
	// separators
	".",
	":",
	" ",
	".",
	",",
	// positive/negative sign
	"+",
	"-",
	// currency/monetary
	"US$"
	" "
	"."
	","
};


BCountry::BCountry()
	:
	fStrings(gStrings)
{
}


BCountry::BCountry(const char **strings)
	:
	fStrings(strings)
{
}


BCountry::~BCountry()
{
}


const char *
BCountry::Name() const
{
	return "United States Of America";
}


const char *
BCountry::GetString(uint32 id) const
{
	if (id < B_COUNTRY_STRINGS_BASE || id >= B_NUM_COUNTRY_STRINGS)
		return NULL;

	return gStrings[id - B_COUNTRY_STRINGS_BASE];
}


void 
BCountry::FormatDate(char *string, size_t maxSize, time_t time, bool longFormat)
{
	// ToDo: implement us
}


void 
BCountry::FormatDate(BString *string, time_t time, bool longFormat)
{
}


void 
BCountry::FormatTime(char *string, size_t maxSize, time_t time, bool longFormat)
{
}


void 
BCountry::FormatTime(BString *string, time_t time, bool longFormat)
{
}


const char *
BCountry::DateFormat(bool longFormat) const
{
	return fStrings[longFormat ? B_DATE_FORMAT : B_SHORT_DATE_FORMAT];
}


const char *
BCountry::TimeFormat(bool longFormat) const
{
	return fStrings[longFormat ? B_TIME_FORMAT : B_SHORT_TIME_FORMAT];
}


const char *
BCountry::DateSeparator() const
{
	return fStrings[B_DATE_SEPARATOR];
}


const char *
BCountry::TimeSeparator() const
{
	return fStrings[B_TIME_SEPARATOR];
}


void 
BCountry::FormatNumber(char *string, size_t maxSize, double value)
{
}


void 
BCountry::FormatNumber(BString *string, double value)
{
}


void 
BCountry::FormatNumber(char *string, size_t maxSize, int32 value)
{
}


void 
BCountry::FormatNumber(BString *string, int32 value)
{
}


const char *
BCountry::DecimalPoint() const
{
	return fStrings[B_DECIMAL_POINT];
}


const char *
BCountry::ThousandsSeparator() const
{
	return fStrings[B_THOUSANDS_SEPARATOR];
}


const char *
BCountry::Grouping() const
{
	return fStrings[B_GROUPING];
}


const char *
BCountry::PositiveSign() const
{
	return fStrings[B_POSITIVE_SIGN];
}


const char *
BCountry::NegativeSign() const
{
	return fStrings[B_NEGATIVE_SIGN];
}


int8 
BCountry::Measurement() const
{
	return B_US;
}


ssize_t
BCountry::FormatMonetary(char *string, size_t maxSize, char *format, ...)
{
	va_list args;
	va_start(args,format);

	ssize_t status = vstrfmon(string, maxSize, format, args);

	va_end(args);

	return status;
}


ssize_t 
BCountry::FormatMonetary(BString *string, char *format, ...)
{
	return B_OK;
}


const char *
BCountry::CurrencySymbol() const
{
	return fStrings[B_CURRENCY_SYMBOL];
}


const char *
BCountry::InternationalCurrencySymbol() const
{
	return fStrings[B_INT_CURRENCY_SYMBOL];
}


const char *
BCountry::MonDecimalPoint() const
{
	return fStrings[B_MON_DECIMAL_POINT];
}


const char *
BCountry::MonThousandsSeparator() const
{
	return fStrings[B_MON_THOUSANDS_SEPARATOR];
}


const char *
BCountry::MonGrouping() const
{
	return fStrings[B_MON_GROUPING];
}


int32 
BCountry::MonFracDigits() const
{
	return 2;
}

