/*
 * Copyright 2003-2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <Language.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <iostream>

#include <Catalog.h>
#include <Locale.h>
#include <LocaleRoster.h>
#include <Path.h>
#include <String.h>
#include <FindDirectory.h>

#include <ICUWrapper.h>

#include <unicode/locid.h>


BLanguage::BLanguage()
	:
	fDirection(B_LEFT_TO_RIGHT),
	fICULocale(NULL)
{
	SetTo(NULL);
}


BLanguage::BLanguage(const char* language)
	:
	fDirection(B_LEFT_TO_RIGHT),
	fICULocale(NULL)
{
	SetTo(language);
}


BLanguage::BLanguage(const BLanguage& other)
	:
	fICULocale(NULL)
{
	*this = other;
}


BLanguage::~BLanguage()
{
	delete fICULocale;
}


status_t
BLanguage::SetTo(const char* language)
{
	delete fICULocale;
	fICULocale = new icu::Locale(language);
	if (fICULocale == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


status_t
BLanguage::GetNativeName(BString& name) const
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
BLanguage::GetName(BString& name, const BLanguage* displayLanguage) const
{
	status_t status = B_OK;

	BString appLanguage;
	if (displayLanguage == NULL) {
		BMessage preferredLanguage;
		status = BLocaleRoster::Default()->GetPreferredLanguages(
			&preferredLanguage);
		if (status == B_OK)
			status = preferredLanguage.FindString("language", 0, &appLanguage);
	} else {
		appLanguage = displayLanguage->Code();
	}

	if (status == B_OK) {
		UnicodeString string;
		fICULocale->getDisplayName(Locale(appLanguage), string);

		name.Truncate(0);
		BStringByteSink converter(&name);
		string.toUTF8(converter);
	}

	return status;
}


status_t
BLanguage::GetIcon(BBitmap* result) const
{
	return BLocaleRoster::Default()->GetFlagIconForCountry(result, Code());
}


const char*
BLanguage::GetString(uint32 id) const
{
	if (id < B_LANGUAGE_STRINGS_BASE
		|| id >= B_LANGUAGE_STRINGS_BASE + B_NUM_LANGUAGE_STRINGS)
		return NULL;

	return NULL;

	// TODO: fetch string from ICU

//	return fStrings[id - B_LANGUAGE_STRINGS_BASE];
}


const char*
BLanguage::Code() const
{
	return fICULocale->getLanguage();
}


const char*
BLanguage::CountryCode() const
{
	const char* country = fICULocale->getCountry();
	if (country == NULL || country[0] == '\0')
		return NULL;

	return country;
}


const char*
BLanguage::ScriptCode() const
{
	const char* script = fICULocale->getScript();
	if (script == NULL || script[0] == '\0')
		return NULL;

	return script;
}


const char*
BLanguage::Variant() const
{
	const char* variant = fICULocale->getVariant();
	if (variant == NULL || variant[0] == '\0')
		return NULL;

	return variant;
}


const char*
BLanguage::ID() const
{
	return fICULocale->getName();
}


bool
BLanguage::IsCountrySpecific() const
{
	return CountryCode() != NULL;
}


bool
BLanguage::IsVariant() const
{
	return Variant() != NULL;
}


uint8
BLanguage::Direction() const
{
	return fDirection;
}


BLanguage&
BLanguage::operator=(const BLanguage& source)
{
	if (&source != this) {
		delete fICULocale;

		fICULocale = source.fICULocale != NULL
			? source.fICULocale->clone()
			: NULL;
		fDirection = source.fDirection;
	}

	return *this;
}
