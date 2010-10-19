/*
 * Copyright 2003-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2009-2010, Adrien Destugues, pulkomandy@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <Country.h>

#include <AutoDeleter.h>
#include <IconUtils.h>
#include <List.h>
#include <Language.h>
#include <LocaleRoster.h>
#include <Resources.h>
#include <String.h>

#include <unicode/locid.h>
#include <unicode/ulocdata.h>
#include <ICUWrapper.h>

#include <iostream>
#include <map>
#include <monetary.h>
#include <new>
#include <stdarg.h>
#include <stdlib.h>


#define ICU_VERSION icu_44


BCountry::BCountry(const char* languageCode, const char* countryCode)
	:
	fICULocale(new ICU_VERSION::Locale(languageCode, countryCode))
{
}


BCountry::BCountry(const char* languageAndCountryCode)
	:
	fICULocale(new ICU_VERSION::Locale(languageAndCountryCode))
{
}


BCountry::BCountry(const BCountry& other)
	:
	fICULocale(new ICU_VERSION::Locale(*other.fICULocale))
{
}


BCountry&
BCountry::operator=(const BCountry& other)
{
	if (this == &other)
		return *this;

	*fICULocale = *other.fICULocale;

	return *this;
}


BCountry::~BCountry()
{
	delete fICULocale;
}


status_t
BCountry::GetNativeName(BString& name) const
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
BCountry::GetName(BString& name, const BLanguage* displayLanguage) const
{
	BString appLanguage;
	if (displayLanguage == NULL) {
		BMessage preferredLanguage;
		be_locale_roster->GetPreferredLanguages(&preferredLanguage);
		preferredLanguage.FindString("language", 0, &appLanguage);
	} else {
		appLanguage = displayLanguage->Code();
	}

	UnicodeString uString;
	fICULocale->getDisplayName(Locale(appLanguage), uString);
	BStringByteSink stringConverter(&name);
	uString.toUTF8(stringConverter);

	return B_OK;
}


const char*
BCountry::Code() const
{
	return fICULocale->getName();
}


status_t
BCountry::GetIcon(BBitmap* result) const
{
	const char* code = fICULocale->getCountry();
	if (code == NULL)
		return  B_ERROR;
	return be_locale_roster->GetFlagIconForCountry(result, code);
}


status_t
BCountry::GetAvailableTimeZones(BMessage* timeZones) const
{
	return be_locale_roster->GetAvailableTimeZonesForCountry(timeZones, Code());
}


// TODO does ICU even support this ? Is it in the keywords ?
int8
BCountry::Measurement() const
{
	UErrorCode error = U_ZERO_ERROR;
	switch (ulocdata_getMeasurementSystem(Code(), &error)) {
		case UMS_US:
			return B_US;
		case UMS_SI:
		default:
			return B_METRIC;
	}
}
