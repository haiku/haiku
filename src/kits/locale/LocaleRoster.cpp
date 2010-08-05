/*
 * Copyright 2003-2010, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Oliver Tappe, zooey@hirschkaefer.de
 */


#include <LocaleRoster.h>

#include <set>

#include <assert.h>
#include <syslog.h>

#include <Autolock.h>
#include <AppFileInfo.h>
#include <Catalog.h>
#include <Collator.h>
#include <Country.h>
#include <DefaultCatalog.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Language.h>
#include <Locale.h>
#include <MutableLocaleRoster.h>
#include <Node.h>
#include <Path.h>
#include <String.h>
#include <TimeZone.h>

#include <ICUWrapper.h>

// ICU includes
#include <unicode/locid.h>
#include <unicode/timezone.h>


using BPrivate::CatalogAddOnInfo;


/*
 * several attributes/resource-IDs used within the Locale Kit:
 */
const char* BLocaleRoster::kCatLangAttr = "BEOS:LOCALE_LANGUAGE";
	// name of catalog language, lives in every catalog file
const char* BLocaleRoster::kCatSigAttr = "BEOS:LOCALE_SIGNATURE";
	// catalog signature, lives in every catalog file
const char* BLocaleRoster::kCatFingerprintAttr = "BEOS:LOCALE_FINGERPRINT";
	// catalog fingerprint, may live in catalog file

const char* BLocaleRoster::kEmbeddedCatAttr = "BEOS:LOCALE_EMBEDDED_CATALOG";
	// attribute which contains flattened data of embedded catalog
	// this may live in an app- or add-on-file
int32 BLocaleRoster::kEmbeddedCatResId = 0xCADA;
	// a unique value used to identify the resource (=> embedded CAtalog DAta)
	// which contains flattened data of embedded catalog.
	// this may live in an app- or add-on-file


BLocaleRoster::BLocaleRoster()
{
}


BLocaleRoster::~BLocaleRoster()
{
}


status_t
BLocaleRoster::Refresh()
{
	return gRosterData.Refresh();
}


status_t
BLocaleRoster::GetDefaultCollator(BCollator* collator) const
{
	if (!collator)
		return B_BAD_VALUE;

	BAutolock lock(gRosterData.fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	*collator = *gRosterData.fDefaultLocale.Collator();

	return B_OK;
}


status_t
BLocaleRoster::GetDefaultLanguage(BLanguage* language) const
{
	if (!language)
		return B_BAD_VALUE;

	BAutolock lock(gRosterData.fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	*language = gRosterData.fDefaultLanguage;

	return B_OK;
}


status_t
BLocaleRoster::GetDefaultCountry(BCountry* country) const
{
	if (!country)
		return B_BAD_VALUE;

	BAutolock lock(gRosterData.fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	*country = *gRosterData.fDefaultLocale.Country();

	return B_OK;
}


status_t
BLocaleRoster::GetDefaultLocale(BLocale* locale) const
{
	if (!locale)
		return B_BAD_VALUE;

	BAutolock lock(gRosterData.fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	*locale = gRosterData.fDefaultLocale;

	return B_OK;
}


status_t
BLocaleRoster::GetDefaultTimeZone(BTimeZone* timezone) const
{
	if (!timezone)
		return B_BAD_VALUE;

	BAutolock lock(gRosterData.fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	*timezone = gRosterData.fDefaultTimeZone;

	return B_OK;
}


status_t
BLocaleRoster::GetLanguage(const char* languageCode,
	BLanguage** _language) const
{
	if (_language == NULL || languageCode == NULL || languageCode[0] == '\0')
		return B_BAD_VALUE;

	BLanguage* language = new(std::nothrow) BLanguage(languageCode);
	if (language == NULL)
		return B_NO_MEMORY;

	*_language = language;
	return B_OK;
}


status_t
BLocaleRoster::GetPreferredLanguages(BMessage* languages) const
{
	if (!languages)
		return B_BAD_VALUE;

	BAutolock lock(gRosterData.fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	*languages = gRosterData.fPreferredLanguages;

	return B_OK;
}


status_t
BLocaleRoster::GetInstalledLanguages(BMessage* languages) const
{
	if (!languages)
		return B_BAD_VALUE;

	int32 i;
	UnicodeString icuLanguageName;
	BString languageName;

	int32_t localeCount;
	const Locale* icuLocaleList
		= Locale::getAvailableLocales(localeCount);

	// TODO: Loop over the strings and add them to a std::set to remove
	//       duplicates?
	for (i = 0; i < localeCount; i++)
		languages->AddString("langs", icuLocaleList[i].getName());

	return B_OK;
}


status_t
BLocaleRoster::GetAvailableCountries(BMessage* countries) const
{
	if (!countries)
		return B_BAD_VALUE;

	int32 i;
	const char* const* countryList = uloc_getISOCountries();

	for (i = 0; countryList[i] != NULL; i++)
		countries->AddString("countries", countryList[i]);

	return B_OK;
}


status_t
BLocaleRoster::GetInstalledCatalogs(BMessage*  languageList,
		const char* sigPattern,	const char* langPattern, int32 fingerprint) const
{
	if (languageList == NULL)
		return B_BAD_VALUE;

	BAutolock lock(gRosterData.fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	int32 count = gRosterData.fCatalogAddOnInfos.CountItems();
	for (int32 i = 0; i < count; ++i) {
		CatalogAddOnInfo* info
			= (CatalogAddOnInfo*)gRosterData.fCatalogAddOnInfos.ItemAt(i);

		if (!info->MakeSureItsLoaded() || !info->fLanguagesFunc)
			continue;

		info->fLanguagesFunc(languageList, sigPattern, langPattern,
			fingerprint);
	}

	return B_OK;
}


BCatalog*
BLocaleRoster::_GetCatalog(BCatalog* catalog, vint32* catalogInitStatus)
{
	// This function is used in the translation macros, so it can't return a
	// status_t. Maybe it could throw exceptions ?

	if (*catalogInitStatus == true) {
		// Catalog already loaded - nothing else to do
		return catalog;
	}

	// figure out image (shared object) from catalog address
	image_info info;
	int32 cookie = 0;
	bool found = false;

	while (get_next_image_info(0, &cookie, &info) == B_OK) {
		if ((char*)info.data < (char*)catalog && (char*)info.data
				+ info.data_size > (char*)catalog) {
			found = true;
			break;
		}
	}

	if (!found) {
		log_team(LOG_DEBUG, "Catalog %x doesn't belong to any image !",
			catalog);
		return catalog;
	}
	// figure out mimetype from image
	BFile objectFile(info.name, B_READ_ONLY);
	BAppFileInfo objectInfo(&objectFile);
	char objectSignature[B_MIME_TYPE_LENGTH];
	if (objectInfo.GetSignature(objectSignature) != B_OK) {
		log_team(LOG_ERR, "File %s has no mimesignature, so it can't use"
			"localization.", info.name);
		return catalog;
	}

	// drop supertype from mimetype (should be "application/"):
	char* stripSignature = objectSignature;
	while (*stripSignature != '/')
		stripSignature ++;
	stripSignature ++;

	log_team(LOG_DEBUG,
		"Image %s (address %x) requested catalog with mimetype %s",
		info.name, catalog, stripSignature);

	// load the catalog for this mimetype and return it to the app
	catalog->SetCatalog(stripSignature, 0);
	*catalogInitStatus = true;

	return catalog;
}
