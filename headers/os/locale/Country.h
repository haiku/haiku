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
			const char*			Code() const;
			status_t			GetIcon(BBitmap* result) const;

			const char*			GetLocalizedString(uint32 id) const;

								// measurements

			int8				Measurement() const;

								// timezones

			int					GetTimeZones(BList& timezones) const;

private:
			icu_44::Locale*		fICULocale;
};


#endif	/* _COUNTRY_H_ */
