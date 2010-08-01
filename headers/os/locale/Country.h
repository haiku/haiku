/*
 * Copyright 2003-2010, Haiku, Inc.
 * Distributed under the terms of the MIT Licence.
 */
#ifndef _COUNTRY_H_
#define _COUNTRY_H_


#include <List.h>
#include <LocaleStrings.h>
#include <String.h>
#include <SupportDefs.h>


class BBitmap;
class BMessage;

namespace icu_44 {
	class DateFormat;
	class Locale;
}


enum {
	B_METRIC = 0,
	B_US
};

typedef enum {
	B_DATE_ELEMENT_INVALID = B_BAD_DATA,
	B_DATE_ELEMENT_YEAR = 0,
	B_DATE_ELEMENT_MONTH,
	B_DATE_ELEMENT_DAY,
	B_DATE_ELEMENT_AM_PM,
	B_DATE_ELEMENT_HOUR,
	B_DATE_ELEMENT_MINUTE,
	B_DATE_ELEMENT_SECOND
} BDateElement;


class BCountry {
public:
								BCountry(const char* languageCode,
									const char* countryCode);
								BCountry(const char* languageAndCountryCode
									= "en_US");
								BCountry(const BCountry& other);
								BCountry& operator=(const BCountry& other);
								~BCountry();

			bool				GetName(BString& name) const;
			bool				GetLocaleName(BString& name) const;
			const char*			Code() const;
			status_t			GetIcon(BBitmap* result) const;

			const char*			GetLocalizedString(uint32 id) const;

								// Date

			status_t			FormatDate(char* string, size_t maxSize,
									time_t time, bool longFormat);
			status_t			FormatDate(BString* string, time_t time,
									bool longFormat);
			status_t			FormatDate(BString* string,
									int*& fieldPositions, int& fieldCount,
									time_t time, bool longFormat);
			status_t			GetDateFields(BDateElement*& fields,
									int& fieldCount, bool longFormat) const;
			status_t			GetDateFormat(BString&, bool longFormat) const;
			status_t			SetDateFormat(const char* formatString,
									bool longFormat = true);

			int					StartOfWeek() const;

								// Time

			status_t			FormatTime(char* string, size_t maxSize,
									time_t time, bool longFormat);
			status_t			FormatTime(BString* string, time_t time,
									bool longFormat);
			status_t			FormatTime(BString* string,
									int*& fieldPositions, int& fieldCount,
									time_t time, bool longFormat);
			status_t			GetTimeFields(BDateElement*& fields,
									int& fieldCount, bool longFormat) const;

			status_t			SetTimeFormat(const char* formatString,
									bool longFormat = true);
			status_t			GetTimeFormat(BString& out,
									bool longFormat) const;

								// numbers

			status_t			FormatNumber(char* string, size_t maxSize,
									double value);
			status_t			FormatNumber(BString* string, double value);
			status_t			FormatNumber(char* string, size_t maxSize,
									int32 value);
			status_t			FormatNumber(BString* string, int32 value);

								// measurements

			int8				Measurement() const;

								// monetary

			ssize_t				FormatMonetary(char* string, size_t maxSize,
									double value);
			ssize_t				FormatMonetary(BString* string, double value);

								// timezones

			int					GetTimeZones(BList& timezones) const;

private:
			icu_44::Locale*		fICULocale;
			BString				fLongDateFormat;
			BString				fShortDateFormat;
			BString				fLongTimeFormat;
			BString				fShortTimeFormat;
};


#endif	/* _COUNTRY_H_ */
