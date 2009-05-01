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
	if (id >= B_LANGUAGE_STRINGS_BASE)
		return fLanguage->GetString(id);

	return fCountry->GetString(id);
}

status_t 
BLocale::GetAppCatalog(BCatalog *catalog) {
	if (!catalog)
		return B_BAD_VALUE;
	if (be_catalog)
		debugger( "GetAppCatalog() has been called while be_catalog != NULL");
	return BCatalog::GetAppCatalog(catalog);
}
