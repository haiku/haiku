/*
 * Copyright 2003-2011, Axel Dörfler, axeld@pinc-software.de.
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


BCountry::BCountry(const char* countryCode)
	:
	fICULocale(new icu::Locale("", countryCode))
{
}


BCountry::BCountry(const BCountry& other)
	:
	fICULocale(new icu::Locale(*other.fICULocale))
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
	status_t status = B_OK;
	BString appLanguage;
	if (displayLanguage == NULL) {
		BMessage preferredLanguages;
		status = BLocaleRoster::Default()->GetPreferredLanguages(
			&preferredLanguages);
		if (status == B_OK)
			status = preferredLanguages.FindString("language", 0, &appLanguage);
	} else {
		appLanguage = displayLanguage->Code();
	}

	if (status == B_OK) {
		UnicodeString uString;
		fICULocale->getDisplayName(Locale(appLanguage), uString);
		name.Truncate(0);
		BStringByteSink stringConverter(&name);
		uString.toUTF8(stringConverter);
	}

	return status;
}


const char*
BCountry::Code() const
{
	return fICULocale->getCountry();
}


status_t
BCountry::GetIcon(BBitmap* result) const
{
	return BLocaleRoster::Default()->GetFlagIconForCountry(result, Code());
}


status_t
BCountry::GetAvailableTimeZones(BMessage* timeZones) const
{
	return BLocaleRoster::Default()->GetAvailableTimeZonesForCountry(timeZones,
		Code());
}

