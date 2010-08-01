/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <Catalog.h>
#include <Locale.h>
#include <LocaleRoster.h>


static BLocale gLocale;
BLocale *be_locale = &gLocale;


BLocale::BLocale()
{
	BLocaleRoster roster;
	roster.GetDefaultCollator(&fCollator);
	roster.GetDefaultCountry(&fCountry);
	roster.GetDefaultLanguage(&fLanguage);
}


BLocale::~BLocale()
{
}


const char *
BLocale::GetString(uint32 id)
{
	// Note: this code assumes a certain order of the string bases

	if (id >= B_OTHER_STRINGS_BASE) {
		if (id == B_CODESET)
			return "UTF-8";

		return "";
	}
	return fLanguage.GetString(id);
}
