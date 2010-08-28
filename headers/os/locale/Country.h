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

			status_t			GetAvailableTimeZones(
									BMessage* timeZones) const;

			int8				Measurement() const;

private:
			icu_44::Locale*		fICULocale;
};


#endif	/* _COUNTRY_H_ */
