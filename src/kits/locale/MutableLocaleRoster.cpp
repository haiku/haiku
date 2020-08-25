/*
 * Copyright 2003-2012, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Oliver Tappe, zooey@hirschkaefer.de
 */


#include <MutableLocaleRoster.h>

#include <pthread.h>

#include <Application.h>
#include <Autolock.h>
#include <Catalog.h>
#include <CatalogData.h>
#include <DefaultCatalog.h>
#include <Debug.h>
#include <Entry.h>
#include <FormattingConventions.h>
#include <Language.h>
#include <LocaleRosterData.h>
#include <String.h>


namespace BPrivate {


namespace {


static MutableLocaleRoster* sLocaleRoster;

static pthread_once_t sLocaleRosterInitOnce = PTHREAD_ONCE_INIT;


}	// anonymous namespace


static void
InitializeLocaleRoster()
{
	sLocaleRoster = new (std::nothrow) MutableLocaleRoster();
}


MutableLocaleRoster::MutableLocaleRoster()
{
}


MutableLocaleRoster::~MutableLocaleRoster()
{
}


/*static*/ MutableLocaleRoster*
MutableLocaleRoster::Default()
{
	if (sLocaleRoster == NULL)
		pthread_once(&sLocaleRosterInitOnce, &InitializeLocaleRoster);

	return sLocaleRoster;
}


status_t
MutableLocaleRoster::SetDefaultFormattingConventions(
	const BFormattingConventions& newFormattingConventions)
{
	return fData->SetDefaultFormattingConventions(newFormattingConventions);
}


status_t
MutableLocaleRoster::SetDefaultTimeZone(const BTimeZone& newZone)
{
	return fData->SetDefaultTimeZone(newZone);
}


status_t
MutableLocaleRoster::SetPreferredLanguages(const BMessage* languages)
{
	return fData->SetPreferredLanguages(languages);
}


status_t
MutableLocaleRoster::SetFilesystemTranslationPreferred(bool preferred)
{
	return fData->SetFilesystemTranslationPreferred(preferred);
}


status_t
MutableLocaleRoster::LoadSystemCatalog(BCatalog* catalog) const
{
	if (!catalog)
		return B_BAD_VALUE;

	// figure out libbe-image (shared object) by name
	image_info info;
	int32 cookie = 0;
	bool found = false;

	while (get_next_image_info(0, &cookie, &info) == B_OK) {
		if (info.data < (void*)&be_app
			&& (char*)info.data + info.data_size > (void*)&be_app) {
			found = true;
			break;
		}
	}

	if (!found)
		return B_ERROR;

	// load the catalog for libbe into the given catalog
	entry_ref ref;
	status_t status = BEntry(info.name).GetRef(&ref);
	if (status != B_OK)
		return status;

	return catalog->SetTo(ref);
}


/*
 * creates a new (empty) catalog of the given type (the request is dispatched
 * to the appropriate add-on).
 * If the add-on doesn't support catalog-creation or if the creation fails,
 * NULL is returned, otherwise a pointer to the freshly created catalog.
 * Any created catalog will be initialized with the given signature and
 * language-name.
 */
BCatalogData*
MutableLocaleRoster::CreateCatalog(const char* type, const char* signature,
	const char* language)
{
	if (!type || !signature || !language)
		return NULL;

	BAutolock lock(fData->fLock);
	if (!lock.IsLocked())
		return NULL;

	int32 count = fData->fCatalogAddOnInfos.CountItems();
	for (int32 i = 0; i < count; ++i) {
		CatalogAddOnInfo* info = (CatalogAddOnInfo*)
			fData->fCatalogAddOnInfos.ItemAt(i);
		if (info->fName.ICompare(type) != 0 || !info->MakeSureItsLoaded()
			|| !info->fCreateFunc)
			continue;

		BCatalogData* catalog = info->fCreateFunc(signature, language);
		if (catalog != NULL) {
			info->fLoadedCatalogs.AddItem(catalog);
			info->UnloadIfPossible();
			return catalog;
		}
	}

	return NULL;
}


/*
 * Loads a catalog for the given signature, language and fingerprint.
 * The request to load this catalog is dispatched to all add-ons in turn,
 * until an add-on reports success.
 * If a catalog depends on another language (as 'english-british' depends
 * on 'english') the dependant catalogs are automatically loaded, too.
 * So it is perfectly possible that this method returns a catalog-chain
 * instead of a single catalog.
 * NULL is returned if no matching catalog could be found.
 */
BCatalogData*
MutableLocaleRoster::LoadCatalog(const entry_ref& catalogOwner,
	const char* language, int32 fingerprint) const
{
	BAutolock lock(fData->fLock);
	if (!lock.IsLocked())
		return NULL;

	int32 count = fData->fCatalogAddOnInfos.CountItems();
	for (int32 i = 0; i < count; ++i) {
		CatalogAddOnInfo* info = (CatalogAddOnInfo*)
			fData->fCatalogAddOnInfos.ItemAt(i);

		if (!info->MakeSureItsLoaded() || !info->fInstantiateFunc)
			continue;
		BMessage languages;
		if (language != NULL) {
			// try to load catalogs for the given language:
			languages.AddString("language", language);
		} else {
			// try to load catalogs for one of the preferred languages:
			GetPreferredLanguages(&languages);
		}

		BCatalogData* catalog = NULL;
		const char* lang;
		for (int32 l = 0; languages.FindString("language", l, &lang) == B_OK;
			++l) {
			catalog = info->fInstantiateFunc(catalogOwner, lang, fingerprint);
			if (catalog != NULL)
				info->fLoadedCatalogs.AddItem(catalog);
			// Chain-load catalogs for languages that depend on
			// other languages.
			// The current implementation uses the filename in order to
			// detect dependencies (parenthood) between languages (it
			// traverses from "english_british_oxford" to "english_british"
			// to "english"):
			int32 pos;
			BString langName(lang);
			BCatalogData* currentCatalog = catalog;
			BCatalogData* nextCatalog = NULL;
			while ((pos = langName.FindLast('_')) >= 0) {
				// language is based on parent, so we load that, too:
				// (even if the parent catalog was not found)
				langName.Truncate(pos);
				nextCatalog = info->fInstantiateFunc(catalogOwner,
					langName.String(), fingerprint);
				if (nextCatalog != NULL) {
					info->fLoadedCatalogs.AddItem(nextCatalog);
					if (currentCatalog != NULL)
						currentCatalog->SetNext(nextCatalog);
					else
						catalog = nextCatalog;
					currentCatalog = nextCatalog;
				}
			}
			if (catalog != NULL)
				return catalog;
		}
		info->UnloadIfPossible();
	}

	return NULL;
}


/*
 * Loads a catalog for the given signature and language.
 *
 * Only the default catalog type is searched, and only the standard system
 * directories.
 *
 * If a catalog depends on another language (as 'english-british' depends
 * on 'english') the dependant catalogs are automatically loaded, too.
 * So it is perfectly possible that this method returns a catalog-chain
 * instead of a single catalog.
 * NULL is returned if no matching catalog could be found.
 */
BCatalogData*
MutableLocaleRoster::LoadCatalog(const char* signature,
	const char* language) const
{
	BAutolock lock(fData->fLock);
	if (!lock.IsLocked())
		return NULL;

	BMessage languages;
	if (language != NULL) {
		// try to load catalogs for the given language:
		languages.AddString("language", language);
	} else {
		// try to load catalogs for one of the preferred languages:
		GetPreferredLanguages(&languages);
	}


	int32 count = fData->fCatalogAddOnInfos.CountItems();
	CatalogAddOnInfo* defaultCatalogInfo = NULL;
	for (int32 i = 0; i < count; ++i) {
		CatalogAddOnInfo* info = (CatalogAddOnInfo*)
			fData->fCatalogAddOnInfos.ItemAt(i);
		if (info->MakeSureItsLoaded()
			&& info->fInstantiateFunc
				== BPrivate::DefaultCatalog::Instantiate) {
			defaultCatalogInfo = info;
			break;
		}
	}

	if (defaultCatalogInfo == NULL)
		return NULL;

	BPrivate::DefaultCatalog* catalog = NULL;
	const char* lang;
	for (int32 l = 0; languages.FindString("language", l, &lang) == B_OK; ++l) {
		catalog = new (std::nothrow) BPrivate::DefaultCatalog(NULL, signature,
			lang);
		if (catalog == NULL)
			continue;

		if (catalog->InitCheck() != B_OK
			|| catalog->ReadFromStandardLocations() != B_OK) {
			delete catalog;
			continue;
		}

		defaultCatalogInfo->fLoadedCatalogs.AddItem(catalog);

		// Chain-load catalogs for languages that depend on
		// other languages.
		// The current implementation uses the filename in order to
		// detect dependencies (parenthood) between languages (it
		// traverses from "english_british_oxford" to "english_british"
		// to "english"):
		int32 pos;
		BString langName(lang);
		BCatalogData* currentCatalog = catalog;
		BPrivate::DefaultCatalog* nextCatalog = NULL;
		while ((pos = langName.FindLast('_')) >= 0) {
			// language is based on parent, so we load that, too:
			// (even if the parent catalog was not found)
			langName.Truncate(pos);
			nextCatalog = new (std::nothrow) BPrivate::DefaultCatalog(NULL,
				signature, langName.String());

			if (nextCatalog == NULL)
				continue;

			if (nextCatalog->InitCheck() != B_OK
				|| nextCatalog->ReadFromStandardLocations() != B_OK) {
				delete nextCatalog;
				continue;
			}

			defaultCatalogInfo->fLoadedCatalogs.AddItem(nextCatalog);

			if (currentCatalog != NULL)
				currentCatalog->SetNext(nextCatalog);
			else
				catalog = nextCatalog;
			currentCatalog = nextCatalog;
		}
		if (catalog != NULL)
			return catalog;
	}

	return NULL;
}


/*
 * unloads the given catalog (or rather: catalog-chain).
 * Every single catalog of the chain will be deleted automatically.
 * Add-ons that have no more current catalogs are unloaded, too.
 */
status_t
MutableLocaleRoster::UnloadCatalog(BCatalogData* catalog)
{
	if (!catalog)
		return B_BAD_VALUE;

	BAutolock lock(fData->fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	status_t res = B_ERROR;
	BCatalogData* nextCatalog;

	while (catalog != NULL) {
		nextCatalog = catalog->Next();
		int32 count = fData->fCatalogAddOnInfos.CountItems();
		for (int32 i = 0; i < count; ++i) {
			CatalogAddOnInfo* info = static_cast<CatalogAddOnInfo*>(
				fData->fCatalogAddOnInfos.ItemAt(i));
			if (info->fLoadedCatalogs.HasItem(catalog)) {
				info->fLoadedCatalogs.RemoveItem(catalog);
				delete catalog;
				info->UnloadIfPossible();
				res = B_OK;
				break;
			}
		}
		catalog = nextCatalog;
	}
	return res;
}


}	// namespace BPrivate
