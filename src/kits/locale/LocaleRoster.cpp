/*
 * Copyright 2003-2010, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Oliver Tappe, zooey@hirschkaefer.de
 */


#include <LocaleRoster.h>

#include <ctype.h>
#include <set>

#include <assert.h>
#include <syslog.h>

#include <Autolock.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <Collator.h>
#include <DefaultCatalog.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FormattingConventions.h>
#include <fs_attr.h>
#include <IconUtils.h>
#include <Language.h>
#include <Locale.h>
#include <MutableLocaleRoster.h>
#include <Node.h>
#include <Path.h>
#include <String.h>
#include <TimeZone.h>

#include <ICUWrapper.h>

// ICU includes
#include <unicode/locdspnm.h>
#include <unicode/locid.h>
#include <unicode/timezone.h>


using BPrivate::CatalogAddOnInfo;
using BPrivate::MutableLocaleRoster;
using BPrivate::RosterData;


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


static status_t
load_resources_if_needed(RosterData* rosterData)
{
	if (rosterData->fAreResourcesLoaded)
		return B_OK;

	status_t result = rosterData->fResources.SetToImage(
		(const void*)&BLocaleRoster::Default);
	if (result != B_OK)
		return result;

	result = rosterData->fResources.PreloadResourceType();
	if (result != B_OK)
		return result;

	rosterData->fAreResourcesLoaded = true;
	return B_OK;
}


static const char*
country_code_for_language(const BLanguage& language)
{
	if (language.IsCountrySpecific())
		return language.CountryCode();

	// TODO: implement for real! For now, we just map some well known
	// languages to countries to make ReadOnlyBootPrompt happy.
	switch ((tolower(language.Code()[0]) << 8) | tolower(language.Code()[1])) {
		case 'be':	// Belarus
			return "BY";
		case 'cs':	// Czech Republic
			return "CZ";
		case 'da':	// Denmark
			return "DK";
		case 'el':	// Greece
			return "GR";
		case 'en':	// United Kingdom
			return "GB";
		case 'ja':	// Japan
			return "JP";
		case 'ko':	// South Korea
			return "KR";
		case 'nb':	// Norway
			return "NO";
		case 'pa':	// Pakistan
			return "PK";
		case 'sv':	// Sweden
			return "SE";
		case 'uk':	// Ukraine
			return "UA";
		case 'zh':	// China
			return "CN";

		// Languages with a matching country name
		case 'de':	// Germany
		case 'es':	// Spain
		case 'fi':	// Finland
		case 'fr':	// France
		case 'hr':	// Croatia
		case 'hu':	// Hungary
		case 'in':	// India
		case 'it':	// Italy
		case 'lt':	// Lithuania
		case 'nl':	// Netherlands
		case 'pl':	// Poland
		case 'pt':	// Portugal
		case 'ro':	// Romania
		case 'ru':	// Russia
		case 'sk':	// Slovakia
			return language.Code();
	}

	return NULL;
}


// #pragma mark -


BLocaleRoster::BLocaleRoster()
{
}


BLocaleRoster::~BLocaleRoster()
{
}


/*static*/ BLocaleRoster*
BLocaleRoster::Default()
{
	return MutableLocaleRoster::Default();
}


status_t
BLocaleRoster::Refresh()
{
	return RosterData::Default()->Refresh();
}


status_t
BLocaleRoster::GetDefaultTimeZone(BTimeZone* timezone) const
{
	if (!timezone)
		return B_BAD_VALUE;

	RosterData* rosterData = RosterData::Default();
	BAutolock lock(rosterData->fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	*timezone = rosterData->fDefaultTimeZone;

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

	RosterData* rosterData = RosterData::Default();
	BAutolock lock(rosterData->fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	*languages = rosterData->fPreferredLanguages;

	return B_OK;
}


/**
 * \brief Fills \c message with 'language'-fields containing the language-
 * ID(s) of all available languages.
 */
status_t
BLocaleRoster::GetAvailableLanguages(BMessage* languages) const
{
	if (!languages)
		return B_BAD_VALUE;

	int32_t localeCount;
	const Locale* icuLocaleList = Locale::getAvailableLocales(localeCount);

	for (int i = 0; i < localeCount; i++)
		languages->AddString("language", icuLocaleList[i].getName());

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
		countries->AddString("country", countryList[i]);

	return B_OK;
}


status_t
BLocaleRoster::GetAvailableTimeZones(BMessage* timeZones) const
{
	if (!timeZones)
		return B_BAD_VALUE;

	status_t status = B_OK;

	StringEnumeration* zoneList = TimeZone::createEnumeration();

	UErrorCode icuStatus = U_ZERO_ERROR;
	int32 count = zoneList->count(icuStatus);
	if (U_SUCCESS(icuStatus)) {
		for (int i = 0; i < count; ++i) {
			const char* zoneID = zoneList->next(NULL, icuStatus);
			if (zoneID == NULL || !U_SUCCESS(icuStatus)) {
				status = B_ERROR;
				break;
			}
 			timeZones->AddString("timeZone", zoneID);
		}
	} else
		status = B_ERROR;

	delete zoneList;

	return status;
}


status_t
BLocaleRoster::GetAvailableTimeZonesWithRegionInfo(BMessage* timeZones) const
{
	if (!timeZones)
		return B_BAD_VALUE;

	status_t status = B_OK;

	UErrorCode icuStatus = U_ZERO_ERROR;

	StringEnumeration* zoneList = TimeZone::createTimeZoneIDEnumeration(
		UCAL_ZONE_TYPE_CANONICAL, NULL, NULL, icuStatus);

	int32 count = zoneList->count(icuStatus);
	if (U_SUCCESS(icuStatus)) {
		for (int i = 0; i < count; ++i) {
			const char* zoneID = zoneList->next(NULL, icuStatus);
			if (zoneID == NULL || !U_SUCCESS(icuStatus)) {
				status = B_ERROR;
				break;
			}
			timeZones->AddString("timeZone", zoneID);

			char region[5];
			icuStatus = U_ZERO_ERROR;
			TimeZone::getRegion(zoneID, region, 5, icuStatus);
			if (!U_SUCCESS(icuStatus)) {
				status = B_ERROR;
				break;
			}
			timeZones->AddString("region", region);
		}
	} else
		status = B_ERROR;

	delete zoneList;

	return status;
}


status_t
BLocaleRoster::GetAvailableTimeZonesForCountry(BMessage* timeZones,
	const char* countryCode) const
{
	if (!timeZones)
		return B_BAD_VALUE;

	status_t status = B_OK;

	StringEnumeration* zoneList = TimeZone::createEnumeration(countryCode);
		// countryCode == NULL will yield all timezones not bound to a country

	UErrorCode icuStatus = U_ZERO_ERROR;
	int32 count = zoneList->count(icuStatus);
	if (U_SUCCESS(icuStatus)) {
		for (int i = 0; i < count; ++i) {
			const char* zoneID = zoneList->next(NULL, icuStatus);
			if (zoneID == NULL || !U_SUCCESS(icuStatus)) {
				status = B_ERROR;
				break;
			}
			timeZones->AddString("timeZone", zoneID);
		}
	} else
		status = B_ERROR;

	delete zoneList;

	return status;
}


status_t
BLocaleRoster::GetFlagIconForCountry(BBitmap* flagIcon, const char* countryCode)
{
	if (countryCode == NULL)
		return B_BAD_VALUE;

	RosterData* rosterData = RosterData::Default();
	BAutolock lock(rosterData->fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	status_t status = load_resources_if_needed(rosterData);
	if (status != B_OK)
		return status;

	// Normalize the country code: 2 letters uppercase
	// filter things out so that "pt_BR" gives the flag for brazil

	int codeLength = strlen(countryCode);
	if (codeLength < 2)
		return B_BAD_VALUE;

	char normalizedCode[3];
	normalizedCode[0] = toupper(countryCode[codeLength - 2]);
	normalizedCode[1] = toupper(countryCode[codeLength - 1]);
	normalizedCode[2] = '\0';

	size_t size;
	const void* buffer = rosterData->fResources.LoadResource(
		B_VECTOR_ICON_TYPE, normalizedCode, &size);
	if (buffer == NULL || size == 0)
		return B_NAME_NOT_FOUND;

	return BIconUtils::GetVectorIcon(static_cast<const uint8*>(buffer), size,
		flagIcon);
}


status_t
BLocaleRoster::GetFlagIconForLanguage(BBitmap* flagIcon,
	const char* languageCode)
{
	if (languageCode == NULL || languageCode[0] == '\0'
		|| languageCode[1] == '\0')
		return B_BAD_VALUE;

	RosterData* rosterData = RosterData::Default();
	BAutolock lock(rosterData->fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	status_t status = load_resources_if_needed(rosterData);
	if (status != B_OK)
		return status;

	// Normalize the language code: first two letters, lowercase

	char normalizedCode[3];
	normalizedCode[0] = tolower(languageCode[0]);
	normalizedCode[1] = tolower(languageCode[1]);
	normalizedCode[2] = '\0';

	size_t size;
	const void* buffer = rosterData->fResources.LoadResource(
		B_VECTOR_ICON_TYPE, normalizedCode, &size);
	if (buffer != NULL && size != 0) {
		return BIconUtils::GetVectorIcon(static_cast<const uint8*>(buffer),
			size, flagIcon);
	}

	// There is no language flag, try to get the default country's flag for
	// the language instead.

	BLanguage language(languageCode);
	const char* countryCode = country_code_for_language(language);
	if (countryCode == NULL)
		return B_NAME_NOT_FOUND;

	return GetFlagIconForCountry(flagIcon, countryCode);
}


status_t
BLocaleRoster::GetAvailableCatalogs(BMessage*  languageList,
	const char* sigPattern,	const char* langPattern, int32 fingerprint) const
{
	if (languageList == NULL)
		return B_BAD_VALUE;

	RosterData* rosterData = RosterData::Default();
	BAutolock lock(rosterData->fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	int32 count = rosterData->fCatalogAddOnInfos.CountItems();
	for (int32 i = 0; i < count; ++i) {
		CatalogAddOnInfo* info
			= (CatalogAddOnInfo*)rosterData->fCatalogAddOnInfos.ItemAt(i);

		if (!info->MakeSureItsLoaded() || !info->fLanguagesFunc)
			continue;

		info->fLanguagesFunc(languageList, sigPattern, langPattern,
			fingerprint);
	}

	return B_OK;
}


bool
BLocaleRoster::IsFilesystemTranslationPreferred() const
{
	RosterData* rosterData = RosterData::Default();
	BAutolock lock(rosterData->fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	return rosterData->fIsFilesystemTranslationPreferred;
}


/*!	\brief Looks up a localized filename from a catalog.
	\param localizedFileName A pre-allocated BString object for the result
		of the lookup.
	\param ref An entry_ref with an attribute holding data for catalog lookup.
	\param traverse A boolean to decide if symlinks are to be traversed.
	\return
	- \c B_OK: success
	- \c B_ENTRY_NOT_FOUND: failure. Attribute not found, entry not found
		in catalog, etc
	- other error codes: failure

	Attribute format:  "signature:context:string"
	(no colon in any of signature, context and string)

	Lookup is done for the top preferred language, only.
	Lookup fails if a comment is present in the catalog entry.
*/
status_t
BLocaleRoster::GetLocalizedFileName(BString& localizedFileName,
	const entry_ref& ref, bool traverse)
{
	BString signature;
	BString context;
	BString string;

	status_t status = _PrepareCatalogEntry(ref, signature, context, string,
		traverse);

	if (status != B_OK)
		return status;

	BCatalog catalog(ref);
	const char* temp = catalog.GetString(string, context);

	if (temp == NULL)
		return B_ENTRY_NOT_FOUND;

	localizedFileName = temp;
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
		log_team(LOG_DEBUG, "Catalog %x doesn't belong to any image!",
			catalog);
		return catalog;
	}

	// load the catalog for this mimetype and return it to the app
	entry_ref ref;
	if (BEntry(info.name).GetRef(&ref) == B_OK && catalog->SetTo(ref) == B_OK)
		*catalogInitStatus = true;

	return catalog;
}


status_t
BLocaleRoster::_PrepareCatalogEntry(const entry_ref& ref, BString& signature,
	BString& context, BString& string, bool traverse)
{
	BEntry entry(&ref, traverse);
	if (!entry.Exists())
		return B_ENTRY_NOT_FOUND;

	BNode node(&entry);
	status_t status = node.InitCheck();
	if (status != B_OK)
		return status;

	status = node.ReadAttrString("SYS:NAME", &signature);
	if (status != B_OK)
		return status;

	int32 first = signature.FindFirst(':');
	int32 last = signature.FindLast(':');
	if (first == last)
		return B_ENTRY_NOT_FOUND;

	context = signature;
	string = signature;

	signature.Truncate(first);
	context.Truncate(last);
	context.Remove(0, first + 1);
	string.Remove(0, last + 1);

	if (signature.Length() == 0 || context.Length() == 0
		|| string.Length() == 0)
		return B_ENTRY_NOT_FOUND;

	return B_OK;
}
