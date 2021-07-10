/*
 * Copyright 2003-2011, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2009-2019, Adrien Destugues, pulkomandy@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <unicode/uversion.h>
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


U_NAMESPACE_USE


BCountry::BCountry(const char* countryCode)
	:
	fICULocale(NULL)
{
	SetTo(countryCode);
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

	if (!fICULocale)
		fICULocale = new icu::Locale(*other.fICULocale);
	else
		*fICULocale = *other.fICULocale;

	return *this;
}


BCountry::~BCountry()
{
	delete fICULocale;
}


status_t
BCountry::SetTo(const char* countryCode)
{
	delete fICULocale;
	fICULocale = new icu::Locale("", countryCode);

	return InitCheck();
}


status_t
BCountry::InitCheck() const
{
	if (fICULocale == NULL)
		return B_NO_MEMORY;

	if (fICULocale->isBogus())
		return B_BAD_DATA;

	return B_OK;
}


status_t
BCountry::GetNativeName(BString& name) const
{
	status_t valid = InitCheck();
	if (valid != B_OK)
		return valid;

	UnicodeString string;
	fICULocale->getDisplayCountry(*fICULocale, string);
	string.toTitle(NULL, *fICULocale);

	name.Truncate(0);
	BStringByteSink converter(&name);
	string.toUTF8(converter);

	return B_OK;
}


status_t
BCountry::GetName(BString& name, const BLanguage* displayLanguage) const
{
	status_t status = InitCheck();
	if (status != B_OK)
		return status;

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
		fICULocale->getDisplayCountry(Locale(appLanguage), uString);
		name.Truncate(0);
		BStringByteSink stringConverter(&name);
		uString.toUTF8(stringConverter);
	}

	return status;
}


const char*
BCountry::Code() const
{
	status_t status = InitCheck();
	if (status != B_OK)
		return NULL;

	return fICULocale->getCountry();
}


status_t
BCountry::GetIcon(BBitmap* result) const
{
	status_t status = InitCheck();
	if (status != B_OK)
		return status;

	return BLocaleRoster::Default()->GetFlagIconForCountry(result, Code());
}


status_t
BCountry::GetAvailableTimeZones(BMessage* timeZones) const
{
	status_t status = InitCheck();
	if (status != B_OK)
		return status;

	return BLocaleRoster::Default()->GetAvailableTimeZonesForCountry(timeZones,
		Code());
}

