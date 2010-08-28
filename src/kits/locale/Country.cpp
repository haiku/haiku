/*
 * Copyright 2003-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2009-2010, Adrien Destugues, pulkomandy@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <Country.h>

#include <AutoDeleter.h>
#include <IconUtils.h>
#include <List.h>
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


bool
BCountry::GetName(BString& name) const
{
	UnicodeString uString;
	fICULocale->getDisplayCountry(uString);
	BStringByteSink stringConverter(&name);
	uString.toUTF8(stringConverter);
	return true;
}


const char*
BCountry::Code() const
{
	return fICULocale->getName();
}


status_t
BCountry::GetIcon(BBitmap* result) const
{
	if (result == NULL)
		return B_BAD_DATA;
	// TODO: a proper way to locate the library being used ?
	BResources storage("/boot/system/lib/liblocale.so");
	if (storage.InitCheck() != B_OK)
		return B_ERROR;
	size_t size;
	const char* code = fICULocale->getCountry();
	if (code != NULL) {
		const void* buffer = storage.LoadResource(B_VECTOR_ICON_TYPE, code,
			&size);
		if (buffer != NULL && size != 0) {
			return BIconUtils::GetVectorIcon(static_cast<const uint8*>(buffer),
				size, result);
		}
	}
	return B_BAD_DATA;
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
