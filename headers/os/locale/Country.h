/*
 * Copyright 2003-2011, Haiku, Inc.
 * Distributed under the terms of the MIT Licence.
 */
#ifndef _COUNTRY_H_
#define _COUNTRY_H_


#include <List.h>
#include <LocaleStrings.h>
#include <String.h>
#include <SupportDefs.h>


class BBitmap;
class BLanguage;
class BMessage;

namespace icu {
	class DateFormat;
	class Locale;
}


class BCountry {
public:
								BCountry(const char* countryCode = NULL);
								BCountry(const BCountry& other);
								BCountry& operator=(const BCountry& other);
								~BCountry();

			status_t			GetNativeName(BString& name) const;
			status_t			GetName(BString& name,
									const BLanguage* displayLanguage = NULL
									) const;

			const char*			Code() const;
									// ISO-3166
			status_t			GetIcon(BBitmap* result) const;

			status_t			GetAvailableTimeZones(
									BMessage* timeZones) const;

			class Private;
private:
	friend	class Private;

			icu::Locale*		fICULocale;
};


#endif	/* _COUNTRY_H_ */
