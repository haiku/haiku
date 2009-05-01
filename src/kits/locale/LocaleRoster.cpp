/* 
** Distributed under the terms of the OpenBeOS License.
** Copyright 2003-2004. All rights reserved.
**
** Authors:	Axel Dörfler, axeld@pinc-software.de
**			Oliver Tappe, zooey@hirschkaefer.de
*/

#include <assert.h>
#include <stdio.h>		// for debug only
#include <syslog.h>

#include <Autolock.h>
#include <Catalog.h>
#include <Collator.h>
#include <Country.h>
#include <DefaultCatalog.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Language.h>
#include <Locale.h>
#include <LocaleRoster.h>
#include <Node.h>
#include <Path.h>
#include <String.h>

static const char *kPriorityAttr = "ADDON:priority";

typedef BCatalogAddOn *(*InstantiateCatalogFunc)(const char *name, 
	const char *language, int32 fingerprint);

typedef BCatalogAddOn *(*CreateCatalogFunc)(const char *name, 
	const char *language);

typedef BCatalogAddOn *(*InstantiateEmbeddedCatalogFunc)(entry_ref *appOrAddOnRef);

static BLocaleRoster gLocaleRoster;
BLocaleRoster *be_locale_roster = &gLocaleRoster;


/*
 * info about a single catalog-add-on (representing a catalog type):
 */
struct BCatalogAddOnInfo {
	BString fName;
	BString fPath;
	image_id fAddOnImage;
	InstantiateCatalogFunc fInstantiateFunc;
	InstantiateEmbeddedCatalogFunc fInstantiateEmbeddedFunc;
	CreateCatalogFunc fCreateFunc;
	uint8 fPriority;
	BList fLoadedCatalogs;
	bool fIsEmbedded;
		// an embedded add-on actually isn't an add-on, it is included
		// as part of the library. The DefaultCatalog is such a beast!

	BCatalogAddOnInfo(const BString& name, const BString& path, uint8 priority);
	~BCatalogAddOnInfo();
	bool MakeSureItsLoaded();
	void UnloadIfPossible();
};


BCatalogAddOnInfo::BCatalogAddOnInfo(const BString& name, const BString& path, 
	uint8 priority)
	:	
	fName(name),
	fPath(path),
	fAddOnImage(B_NO_INIT),
	fInstantiateFunc(NULL),
	fInstantiateEmbeddedFunc(NULL),
	fCreateFunc(NULL),
	fPriority(priority),
	fIsEmbedded(path.Length()==0)
{
}


BCatalogAddOnInfo::~BCatalogAddOnInfo()
{
	int32 count = fLoadedCatalogs.CountItems();
	for (int32 i=0; i<count; ++i) {
		BCatalogAddOn* cat 
			= static_cast<BCatalogAddOn*>(fLoadedCatalogs.ItemAt(i));
		delete cat;
	}
	fLoadedCatalogs.MakeEmpty();
	UnloadIfPossible();
}
	

bool
BCatalogAddOnInfo::MakeSureItsLoaded() 
{
	if (!fIsEmbedded && fAddOnImage < B_OK) {
		// add-on has not been loaded yet, so we try to load it:
		BString fullAddOnPath(fPath);
		fullAddOnPath << "/" << fName;
		fAddOnImage = load_add_on(fullAddOnPath.String());
		if (fAddOnImage >= B_OK) {
			get_image_symbol(fAddOnImage, "instantiate_catalog",
				B_SYMBOL_TYPE_TEXT, (void **)&fInstantiateFunc);
			get_image_symbol(fAddOnImage, "instantiate_embedded_catalog",
				B_SYMBOL_TYPE_TEXT, (void **)&fInstantiateEmbeddedFunc);
			get_image_symbol(fAddOnImage, "create_catalog",
				B_SYMBOL_TYPE_TEXT, (void **)&fCreateFunc);
			log_team(LOG_DEBUG, "catalog-add-on %s has been loaded",
				fName.String());
		} else {
			log_team(LOG_DEBUG, "could not load catalog-add-on %s (%s)",
				fName.String(), strerror(fAddOnImage));
			return false;
		}
	}
	return true;
}


void
BCatalogAddOnInfo::UnloadIfPossible() 
{
	if (!fIsEmbedded && fLoadedCatalogs.IsEmpty()) {
		unload_add_on(fAddOnImage);
		fAddOnImage = B_NO_INIT;
		fInstantiateFunc = NULL;
		fInstantiateEmbeddedFunc = NULL;
		fCreateFunc = NULL;
		log_team(LOG_DEBUG, "catalog-add-on %s has been unloaded",
			fName.String());
	}
}


/*
 * the global data that is shared between all roster-objects:
 */
struct RosterData {
	BLocker fLock;
	BList fCatalogAddOnInfos;
	BMessage fPreferredLanguages;
	//
	RosterData();
	~RosterData();
	void InitializeCatalogAddOns();
	void CleanupCatalogAddOns();
	static int CompareInfos(const void *left, const void *right);
};
static RosterData gRosterData;


RosterData::RosterData()
	:
	fLock("LocaleRosterData")
{
	BAutolock lock(fLock);
	assert(lock.IsLocked());

	// ToDo: make a decision about log-facility and -options
	openlog_team("liblocale.so", LOG_PID, LOG_USER);
#ifndef DEBUG
	// ToDo: find out why the following bugger isn't working!
	setlogmask_team(LOG_UPTO(LOG_WARNING));
#endif

	InitializeCatalogAddOns();

	// ToDo: change this to fetch preferred languages from prefs
	fPreferredLanguages.AddString("language", "german-swiss");
	fPreferredLanguages.AddString("language", "german");
	fPreferredLanguages.AddString("language", "english");
}


RosterData::~RosterData()
{
	BAutolock lock(fLock);
	assert(lock.IsLocked());
	CleanupCatalogAddOns();
	closelog();
}


int
RosterData::CompareInfos(const void *left, const void *right)
{
	return ((BCatalogAddOnInfo*)right)->fPriority 
		- ((BCatalogAddOnInfo*)left)->fPriority;
}


/*
 * iterate over add-on-folders and collect information about each 
 * catalog-add-ons (types of catalogs) into fCatalogAddOnInfos.
 */
void 
RosterData::InitializeCatalogAddOns() 
{
	BAutolock lock(fLock);
	assert(lock.IsLocked());

	// add info about embedded default catalog:
	BCatalogAddOnInfo *defaultCatalogAddOnInfo
		= new BCatalogAddOnInfo("Default", "", 
			 DefaultCatalog::kDefaultCatalogAddOnPriority);
	defaultCatalogAddOnInfo->fInstantiateFunc = DefaultCatalog::Instantiate;
	defaultCatalogAddOnInfo->fInstantiateEmbeddedFunc 
		= DefaultCatalog::InstantiateEmbedded;
	defaultCatalogAddOnInfo->fCreateFunc = DefaultCatalog::Create;
	fCatalogAddOnInfos.AddItem((void*)defaultCatalogAddOnInfo);

	directory_which folders[] = {
		B_COMMON_ADDONS_DIRECTORY,
		B_BEOS_ADDONS_DIRECTORY,
		static_cast<directory_which>(-1)
	};
	BPath addOnPath;
	BDirectory addOnFolder;
	char buf[4096];
	status_t err;
	for (int f=0; folders[f]>=0; ++f) {
		find_directory(folders[f], &addOnPath);
		BString addOnFolderName(addOnPath.Path());
		addOnFolderName << "/locale/catalogs";
		err = addOnFolder.SetTo(addOnFolderName.String());
		if (err != B_OK)
			continue;

		// scan through all the folder's entries for catalog add-ons:
		int32 count;
		int8 priority;
		entry_ref eref;
		BNode node;
		BEntry entry;
		dirent *dent;
		while ((count = addOnFolder.GetNextDirents((dirent *)buf, 4096)) > 0) {
			dent = (dirent *)buf;
			while (count-- > 0) {
				if (strcmp(dent->d_name, ".") && strcmp(dent->d_name, "..")) {
					// we have found (what should be) a catalog-add-on:
					eref.device = dent->d_pdev;
					eref.directory = dent->d_pino;
					eref.set_name(dent->d_name);
					entry.SetTo(&eref, true);
						// traverse through any links to get to the real thang!
					node.SetTo(&entry);
					priority = -1;
					if (node.ReadAttr(kPriorityAttr, B_INT8_TYPE, 0, 
						&priority, sizeof(int8)) <= 0) {
						// add-on has no priority-attribute yet, so we load it to
						// fetch the priority from the corresponding symbol...
						BString fullAddOnPath(addOnFolderName);
						fullAddOnPath << "/" << dent->d_name;
						image_id image = load_add_on(fullAddOnPath.String());
						if (image >= B_OK) {
							uint8 *prioPtr;
							if (get_image_symbol(image, "gCatalogAddOnPriority",
								B_SYMBOL_TYPE_TEXT, 
								(void **)&prioPtr) == B_OK) {
								priority = *prioPtr;
								node.WriteAttr(kPriorityAttr, B_INT8_TYPE, 0, 
									&priority, sizeof(int8));
							}
							unload_add_on(image);
						} else {
							log_team(LOG_ERR, 
								"couldn't load add-on %s, error: %s\n", 
								fullAddOnPath.String(), strerror(image));
						}
					}
					if (priority >= 0) {
						// add-ons with priority<0 will be ignored
						fCatalogAddOnInfos.AddItem(
							(void*)new BCatalogAddOnInfo(dent->d_name, 
								addOnFolderName, priority)
						);
					}
				}
				// Bump the dirent-pointer by length of the dirent just handled:
				dent = (dirent *)((char *)dent + dent->d_reclen);
			}
		}
	}
	fCatalogAddOnInfos.SortItems(CompareInfos);

	for (int32 i=0; i<fCatalogAddOnInfos.CountItems(); ++i) {
		BCatalogAddOnInfo *info 
			= static_cast<BCatalogAddOnInfo*>(fCatalogAddOnInfos.ItemAt(i));
		log_team(LOG_INFO, 
			"roster uses catalog-add-on %s/%s with priority %d",
			info->fIsEmbedded ? "(embedded)" : info->fPath.String(), 
			info->fName.String(), info->fPriority);
	}
}


/*
 * unloads all catalog-add-ons (which will throw away all loaded catalogs, too)
 */
void
RosterData::CleanupCatalogAddOns() 
{
	BAutolock lock(fLock);
	assert(lock.IsLocked());
	int32 count = fCatalogAddOnInfos.CountItems();
	for (int32 i=0; i<count; ++i) {
		BCatalogAddOnInfo *info 
			= static_cast<BCatalogAddOnInfo*>(fCatalogAddOnInfos.ItemAt(i));
		delete info;
	}
	fCatalogAddOnInfos.MakeEmpty();
}


/*
 * several attributes/resource-IDs used within the Locale Kit:
 */
const char *BLocaleRoster::kCatLangAttr = "BEOS:LOCALE_LANGUAGE";
	// name of catalog language, lives in every catalog file
const char *BLocaleRoster::kCatSigAttr = "BEOS:LOCALE_SIGNATURE";
	// catalog signature, lives in every catalog file
const char *BLocaleRoster::kCatFingerprintAttr = "BEOS:LOCALE_FINGERPRINT";
	// catalog fingerprint, may live in catalog file

const char *BLocaleRoster::kCatManagerMimeType 
	= "application/x-vnd.Be.locale.catalog-manager";
	// signature of catalog managing app
const char *BLocaleRoster::kCatEditorMimeType 
	= "application/x-vnd.Be.locale.catalog-editor";
	// signature of catalog editor app

const char *BLocaleRoster::kEmbeddedCatAttr = "BEOS:LOCALE_EMBEDDED_CATALOG";
	// attribute which contains flattened data of embedded catalog
	// this may live in an app- or add-on-file
int32 BLocaleRoster::kEmbeddedCatResId = 0xCADA;
	// a unique value used to identify the resource (=> embedded CAtalog DAta)
	// which contains flattened data of embedded catalog.
	// this may live in an app- or add-on-file

/*
 * BLocaleRoster, the exported interface to the locale roster data:
 */
BLocaleRoster::BLocaleRoster()
{
}


BLocaleRoster::~BLocaleRoster()
{
}


status_t 
BLocaleRoster::GetDefaultCollator(BCollator **collator) const
{
	// It should just use the archived collator from the locale settings;
	// if that is not available, just return the standard collator
	if (!collator)
		return B_BAD_VALUE;
	*collator = new BCollator();
	return B_OK;
}


status_t 
BLocaleRoster::GetDefaultLanguage(BLanguage **language) const
{
	if (!language)
		return B_BAD_VALUE;
	*language = new BLanguage(NULL);
	return B_OK;
}


status_t 
BLocaleRoster::GetDefaultCountry(BCountry **country) const
{
	if (!country)
		return B_BAD_VALUE;
	*country = new BCountry();
	return B_OK;
}


status_t 
BLocaleRoster::GetPreferredLanguages(BMessage *languages) const
{
	if (!languages)
		return B_BAD_VALUE;

	BAutolock lock(gRosterData.fLock);
	assert(lock.IsLocked());

	*languages = gRosterData.fPreferredLanguages;
	return B_OK;
}


status_t 
BLocaleRoster::SetPreferredLanguages(BMessage *languages)
{
	BAutolock lock(gRosterData.fLock);
	assert(lock.IsLocked());

	if (languages)
		gRosterData.fPreferredLanguages = *languages;
	else
		gRosterData.fPreferredLanguages.MakeEmpty();
	return B_OK;
}


/*
 * creates a new (empty) catalog of the given type (the request is dispatched
 * to the appropriate add-on).
 * If the add-on doesn't support catalog-creation or if the creation fails,
 * NULL is returned, otherwise a pointer to the freshly created catalog.
 * Any created catalog will be initialized with the given signature and
 * language-name.
 */
BCatalogAddOn*
BLocaleRoster::CreateCatalog(const char *type, const char *signature, 
	const char *language)
{
	if (!type || !signature || !language)
		return NULL;

	BAutolock lock(gRosterData.fLock);
	assert(lock.IsLocked());

	int32 count = gRosterData.fCatalogAddOnInfos.CountItems();
	for (int32 i=0; i<count; ++i) {
		BCatalogAddOnInfo *info 
			= (BCatalogAddOnInfo*)gRosterData.fCatalogAddOnInfos.ItemAt(i);
		if (info->fName.ICompare(type)!=0 || !info->MakeSureItsLoaded() 
			|| !info->fCreateFunc)
			continue;

		BCatalogAddOn *catalog = info->fCreateFunc(signature, language);
		if (catalog) {
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
BCatalogAddOn*
BLocaleRoster::LoadCatalog(const char *signature, const char *language,
	int32 fingerprint)
{
	if (!signature)
		return NULL;

	BAutolock lock(gRosterData.fLock);
	assert(lock.IsLocked());

	int32 count = gRosterData.fCatalogAddOnInfos.CountItems();
	for (int32 i=0; i<count; ++i) {
		BCatalogAddOnInfo *info 
			= (BCatalogAddOnInfo*)gRosterData.fCatalogAddOnInfos.ItemAt(i);

		if (!info->MakeSureItsLoaded() || !info->fInstantiateFunc)
			continue;
		BMessage languages;
		if (language)
			// try to load catalogs for the given language:
			languages.AddString("language", language);
		else
			// try to load catalogs for one of the preferred languages:
			GetPreferredLanguages(&languages);

		BCatalogAddOn *catalog = NULL;
		const char *lang;
		for (int32 l=0; languages.FindString("language", l, &lang)==B_OK; ++l) {
			catalog = info->fInstantiateFunc(signature, lang, fingerprint);
			if (catalog) {
				info->fLoadedCatalogs.AddItem(catalog);
#if 0			
				// Chain-loading of catalogs has been disabled, as with the
				// current way of handling languages (there are no general
				// languages like 'English', but only specialized ones, like
				// 'English-american') it does not make sense... 
				//
				// Chain-load catalogs for languages that depend on 
				// other languages.
				// The current implementation uses the filename in order to 
				// detect dependencies (parenthood) between languages (it
				// traverses from "english-british-oxford" to "english-british"
				// to "english"):
				int32 pos;
				BString langName(lang);
				BCatalogAddOn *currCatalog=catalog, *nextCatalog;
				while ((pos = langName.FindLast('-')) >= 0) {
					// language is based on parent, so we load that, too:
					langName.Truncate(pos);
					nextCatalog = info->fInstantiateFunc(signature, 
						langName.String(), fingerprint);
					if (nextCatalog) {
						info->fLoadedCatalogs.AddItem(nextCatalog);
						currCatalog->fNext = nextCatalog;
						currCatalog = nextCatalog;
					}
				}
#endif
				return catalog;
			}
		}
		info->UnloadIfPossible();
	}

	return NULL;
}


/*
 * Loads an embedded catalog from the given entry-ref (which is usually an
 * app- or add-on-file. The request to load the catalog is dispatched to all 
 * add-ons in turn, until an add-on reports success.
 * NULL is returned if no embedded catalog could be found.
 */
BCatalogAddOn*
BLocaleRoster::LoadEmbeddedCatalog(entry_ref *appOrAddOnRef)
{
	if (!appOrAddOnRef)
		return NULL;

	BAutolock lock(gRosterData.fLock);
	assert(lock.IsLocked());

	int32 count = gRosterData.fCatalogAddOnInfos.CountItems();
	for (int32 i=0; i<count; ++i) {
		BCatalogAddOnInfo *info 
			= (BCatalogAddOnInfo*)gRosterData.fCatalogAddOnInfos.ItemAt(i);

		if (!info->MakeSureItsLoaded() || !info->fInstantiateEmbeddedFunc)
			continue;

		BCatalogAddOn *catalog = NULL;
		catalog = info->fInstantiateEmbeddedFunc(appOrAddOnRef);
		if (catalog) {
			info->fLoadedCatalogs.AddItem(catalog);
			return catalog;
		}
		info->UnloadIfPossible();
	}

	return NULL;
}


/*
 * unloads the given catalog (or rather: catalog-chain).
 * Every single catalog of the chain will be deleted automatically.
 * Add-ons that have no more current catalogs are unloaded, too.
 */
status_t
BLocaleRoster::UnloadCatalog(BCatalogAddOn *catalog)
{
	if (!catalog)
		return B_BAD_VALUE;

	BAutolock lock(gRosterData.fLock);
	assert(lock.IsLocked());

	status_t res = B_ERROR;
	BCatalogAddOn *nextCatalog;
	// note: as we currently aren't chainloading catalogs, there is only
	//       one catalog to unload...
	while (catalog) {
		nextCatalog = catalog->fNext;
		int32 count = gRosterData.fCatalogAddOnInfos.CountItems();
		for (int32 i=0; i<count; ++i) {
			BCatalogAddOnInfo *info 
				= static_cast<BCatalogAddOnInfo*>(
					gRosterData.fCatalogAddOnInfos.ItemAt(i)
				);
			if (info->fLoadedCatalogs.HasItem(catalog)) {
				info->fLoadedCatalogs.RemoveItem(catalog);
				delete catalog;
				info->UnloadIfPossible();
				res = B_OK;
			}
		}
		catalog = nextCatalog;
	}
	return res;
}
