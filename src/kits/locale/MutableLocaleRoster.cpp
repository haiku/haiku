/*
 * Copyright 2003-2010, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Oliver Tappe, zooey@hirschkaefer.de
 */


#include <MutableLocaleRoster.h>

#include <set>

#include <pthread.h>
#include <syslog.h>

#include <AppFileInfo.h>
#include <Application.h>
#include <Autolock.h>
#include <Catalog.h>
#include <Collator.h>
#include <Debug.h>
#include <DefaultCatalog.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <FormattingConventions.h>
#include <Language.h>
#include <Locale.h>
#include <Node.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>
#include <TimeZone.h>

#include <ICUWrapper.h>

// ICU includes
#include <unicode/locid.h>
#include <unicode/timezone.h>


namespace BPrivate {


// #pragma mark - CatalogAddOnInfo


CatalogAddOnInfo::CatalogAddOnInfo(const BString& name, const BString& path,
	uint8 priority)
	:
	fInstantiateFunc(NULL),
	fCreateFunc(NULL),
	fLanguagesFunc(NULL),
	fName(name),
	fPath(path),
	fAddOnImage(B_NO_INIT),
	fPriority(priority),
	fIsEmbedded(path.Length()==0)
{
}


CatalogAddOnInfo::~CatalogAddOnInfo()
{
	int32 count = fLoadedCatalogs.CountItems();
	for (int32 i = 0; i < count; ++i) {
		BCatalogAddOn* cat
			= static_cast<BCatalogAddOn*>(fLoadedCatalogs.ItemAt(i));
		delete cat;
	}
	fLoadedCatalogs.MakeEmpty();
	UnloadIfPossible();
}


bool
CatalogAddOnInfo::MakeSureItsLoaded()
{
	if (!fIsEmbedded && fAddOnImage < B_OK) {
		// add-on has not been loaded yet, so we try to load it:
		BString fullAddOnPath(fPath);
		fullAddOnPath << "/" << fName;
		fAddOnImage = load_add_on(fullAddOnPath.String());
		if (fAddOnImage >= B_OK) {
			get_image_symbol(fAddOnImage, "instantiate_catalog",
				B_SYMBOL_TYPE_TEXT, (void**)&fInstantiateFunc);
			get_image_symbol(fAddOnImage, "create_catalog",
				B_SYMBOL_TYPE_TEXT, (void**)&fCreateFunc);
			get_image_symbol(fAddOnImage, "get_available_languages",
				B_SYMBOL_TYPE_TEXT, (void**)&fLanguagesFunc);
			log_team(LOG_DEBUG, "catalog-add-on %s has been loaded",
				fName.String());
		} else {
			log_team(LOG_DEBUG, "could not load catalog-add-on %s (%s)",
				fName.String(), strerror(fAddOnImage));
			return false;
		}
	} else if (fIsEmbedded) {
		// The built-in catalog still has to provide this function
		fLanguagesFunc = default_catalog_get_available_languages;
	}
	return true;
}


void
CatalogAddOnInfo::UnloadIfPossible()
{
	if (!fIsEmbedded && fLoadedCatalogs.IsEmpty()) {
		unload_add_on(fAddOnImage);
		fAddOnImage = B_NO_INIT;
		fInstantiateFunc = NULL;
		fCreateFunc = NULL;
		fLanguagesFunc = NULL;
//		log_team(LOG_DEBUG, "catalog-add-on %s has been unloaded",
//			fName.String());
	}
}


// #pragma mark - RosterData


namespace {


static const char* kPriorityAttr = "ADDON:priority";

static const char* kLanguageField = "language";
static const char* kTimezoneField = "timezone";
static const char* kTranslateFilesystemField = "filesys";


static RosterData* sRosterData = NULL;
static pthread_once_t sRosterDataInitOnce = PTHREAD_ONCE_INIT;


static struct RosterDataReaper {
	~RosterDataReaper()
	{
		delete sRosterData;
		sRosterData = NULL;
	}
} sRosterDataReaper;


}	// anonymous namespace



static void
InitializeRosterData()
{
	sRosterData = new (std::nothrow) RosterData(BLanguage("en_US"),
		BFormattingConventions("en_US"));
}


RosterData::RosterData(const BLanguage& language,
	const BFormattingConventions& conventions)
	:
	fLock("LocaleRosterData"),
	fDefaultLocale(&language, &conventions),
	fIsFilesystemTranslationPreferred(false),
	fAreResourcesLoaded(false)
{
	fInitStatus = _Initialize();
}


RosterData::~RosterData()
{
	BAutolock lock(fLock);

	_CleanupCatalogAddOns();
	closelog();
}


/*static*/ RosterData*
RosterData::Default()
{
	if (sRosterData == NULL)
		pthread_once(&sRosterDataInitOnce, &BPrivate::InitializeRosterData);

	return sRosterData;
}


status_t
RosterData::InitCheck() const
{
	return fAreResourcesLoaded ? B_OK : B_NO_INIT;
}


status_t
RosterData::Refresh()
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	_LoadLocaleSettings();
	_LoadTimeSettings();

	return B_OK;
}


int
RosterData::CompareInfos(const void* left, const void* right)
{
	return ((CatalogAddOnInfo*)right)->fPriority
		- ((CatalogAddOnInfo*)left)->fPriority;
}


status_t
RosterData::SetDefaultFormattingConventions(
	const BFormattingConventions& newFormattingConventions)
{
	status_t status = B_OK;

	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	status = _SetDefaultFormattingConventions(newFormattingConventions);

	if (status == B_OK)
		status = _SaveLocaleSettings();

	if (status == B_OK) {
		BMessage updateMessage(B_LOCALE_CHANGED);
		status = _AddDefaultFormattingConventionsToMessage(&updateMessage);
		if (status == B_OK)
			status = be_roster->Broadcast(&updateMessage);
	}

	return status;
}


status_t
RosterData::SetDefaultTimeZone(const BTimeZone& newZone)
{
	status_t status = B_OK;

	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	status = _SetDefaultTimeZone(newZone);

	if (status == B_OK)
		status = _SaveTimeSettings();

	if (status == B_OK) {
		BMessage updateMessage(B_LOCALE_CHANGED);
		status = _AddDefaultTimeZoneToMessage(&updateMessage);
		if (status == B_OK)
			status = be_roster->Broadcast(&updateMessage);
	}

	return status;
}


status_t
RosterData::SetPreferredLanguages(const BMessage* languages)
{
	status_t status = B_OK;

	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	status = _SetPreferredLanguages(languages);

	if (status == B_OK)
		status = _SaveLocaleSettings();

	if (status == B_OK) {
		BMessage updateMessage(B_LOCALE_CHANGED);
		status = _AddPreferredLanguagesToMessage(&updateMessage);
		if (status == B_OK)
			status = be_roster->Broadcast(&updateMessage);
	}

	return status;
}


status_t
RosterData::SetFilesystemTranslationPreferred(bool preferred)
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	_SetFilesystemTranslationPreferred(preferred);

	status_t status = _SaveLocaleSettings();

	if (status == B_OK) {
		BMessage updateMessage(B_LOCALE_CHANGED);
		status = _AddFilesystemTranslationPreferenceToMessage(&updateMessage);
		if (status == B_OK)
			status = be_roster->Broadcast(&updateMessage);
	}

	return status;
}


status_t
RosterData::_Initialize()
{
	openlog_team("liblocale.so", LOG_PID, LOG_USER);
#ifndef DEBUG
	setlogmask_team(LOG_UPTO(LOG_WARNING));
#endif

	status_t result = _InitializeCatalogAddOns();
	if (result != B_OK)
		return result;

	if ((result = Refresh()) != B_OK)
		return result;

	fInitStatus = B_OK;
	return B_OK;
}


/*
iterate over add-on-folders and collect information about each
catalog-add-ons (types of catalogs) into fCatalogAddOnInfos.
*/
status_t
RosterData::_InitializeCatalogAddOns()
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	// add info about embedded default catalog:
	CatalogAddOnInfo* defaultCatalogAddOnInfo
		= new(std::nothrow) CatalogAddOnInfo("Default", "",
			DefaultCatalog::kDefaultCatalogAddOnPriority);
	if (!defaultCatalogAddOnInfo)
		return B_NO_MEMORY;

	defaultCatalogAddOnInfo->fInstantiateFunc = DefaultCatalog::Instantiate;
	defaultCatalogAddOnInfo->fCreateFunc = DefaultCatalog::Create;
	fCatalogAddOnInfos.AddItem((void*)defaultCatalogAddOnInfo);

	directory_which folders[] = {
		B_USER_ADDONS_DIRECTORY,
		B_COMMON_ADDONS_DIRECTORY,
		B_SYSTEM_ADDONS_DIRECTORY,
	};
	BPath addOnPath;
	BDirectory addOnFolder;
	char buf[4096];
	status_t err;
	for (uint32 f = 0; f < sizeof(folders) / sizeof(directory_which); ++f) {
		find_directory(folders[f], &addOnPath);
		BString addOnFolderName(addOnPath.Path());
		addOnFolderName << "/locale/catalogs";

		system_info info;
		if (get_system_info(&info) == B_OK
				&& (info.abi & B_HAIKU_ABI_MAJOR)
				!= (B_HAIKU_ABI & B_HAIKU_ABI_MAJOR)) {
			switch (B_HAIKU_ABI & B_HAIKU_ABI_MAJOR) {
				case B_HAIKU_ABI_GCC_2:
					addOnFolderName << "/gcc2";
					break;
				case B_HAIKU_ABI_GCC_4:
					addOnFolderName << "/gcc4";
					break;
			}
		}


		err = addOnFolder.SetTo(addOnFolderName.String());
		if (err != B_OK)
			continue;

		// scan through all the folder's entries for catalog add-ons:
		int32 count;
		int8 priority;
		entry_ref eref;
		BNode node;
		BEntry entry;
		dirent* dent;
		while ((count = addOnFolder.GetNextDirents((dirent*)buf, 4096)) > 0) {
			dent = (dirent*)buf;
			while (count-- > 0) {
				if (strcmp(dent->d_name, ".") && strcmp(dent->d_name, "..")
						&& strcmp(dent->d_name, "gcc2")
						&& strcmp(dent->d_name, "gcc4")) {
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
						// add-on has no priority-attribute yet, so we load it
						// to fetch the priority from the corresponding
						// symbol...
						BString fullAddOnPath(addOnFolderName);
						fullAddOnPath << "/" << dent->d_name;
						image_id image = load_add_on(fullAddOnPath.String());
						if (image >= B_OK) {
							uint8* prioPtr;
							if (get_image_symbol(image, "gCatalogAddOnPriority",
								B_SYMBOL_TYPE_DATA,
								(void**)&prioPtr) == B_OK) {
								priority = *prioPtr;
								node.WriteAttr(kPriorityAttr, B_INT8_TYPE, 0,
									&priority, sizeof(int8));
							} else {
								log_team(LOG_ERR,
									"couldn't get priority for add-on %s\n",
									fullAddOnPath.String());
							}
							unload_add_on(image);
						} else {
							log_team(LOG_ERR,
								"couldn't load add-on %s, error: %s\n",
								fullAddOnPath.String(), strerror(image));
						}
					}

					if (priority >= 0) {
						// add-ons with priority < 0 will be ignored
						CatalogAddOnInfo* addOnInfo
							= new(std::nothrow) CatalogAddOnInfo(dent->d_name,
								addOnFolderName, priority);
						if (addOnInfo)
							fCatalogAddOnInfos.AddItem((void*)addOnInfo);
					}
				}
				// Bump the dirent-pointer by length of the dirent just handled:
				dent = (dirent*)((char*)dent + dent->d_reclen);
			}
		}
	}
	fCatalogAddOnInfos.SortItems(CompareInfos);

	return B_OK;
}


/*
 * unloads all catalog-add-ons (which will throw away all loaded catalogs, too)
 */
void
RosterData::_CleanupCatalogAddOns()
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return;

	int32 count = fCatalogAddOnInfos.CountItems();
	for (int32 i = 0; i<count; ++i) {
		CatalogAddOnInfo* info
			= static_cast<CatalogAddOnInfo*>(fCatalogAddOnInfos.ItemAt(i));
		delete info;
	}
	fCatalogAddOnInfos.MakeEmpty();
}


status_t
RosterData::_LoadLocaleSettings()
{
	BPath path;
	BFile file;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status == B_OK) {
		path.Append("Locale settings");
		status = file.SetTo(path.Path(), B_READ_ONLY);
	}
	BMessage settings;
	if (status == B_OK)
		status = settings.Unflatten(&file);

	if (status == B_OK) {
		BFormattingConventions conventions(&settings);
		fDefaultLocale.SetFormattingConventions(conventions);

		_SetPreferredLanguages(&settings);

		bool preferred;
		if (settings.FindBool(kTranslateFilesystemField, &preferred) == B_OK)
			_SetFilesystemTranslationPreferred(preferred);

		return B_OK;
	}


	// Something went wrong (no settings file or invalid BMessage), so we
	// set everything to default values

	fPreferredLanguages.MakeEmpty();
	fPreferredLanguages.AddString(kLanguageField, "en");
	BLanguage defaultLanguage("en_US");
	fDefaultLocale.SetLanguage(defaultLanguage);
	BFormattingConventions conventions("en_US");
	fDefaultLocale.SetFormattingConventions(conventions);

	return status;
}


status_t
RosterData::_LoadTimeSettings()
{
	BPath path;
	BFile file;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status == B_OK) {
		path.Append("Time settings");
		status = file.SetTo(path.Path(), B_READ_ONLY);
	}
	BMessage settings;
	if (status == B_OK)
		status = settings.Unflatten(&file);
	if (status == B_OK) {
		BString timeZoneID;
		if (settings.FindString(kTimezoneField, &timeZoneID) == B_OK)
			_SetDefaultTimeZone(BTimeZone(timeZoneID.String()));
		else
			_SetDefaultTimeZone(BTimeZone(BTimeZone::kNameOfGmtZone));

		return B_OK;
	}

	// Something went wrong (no settings file or invalid BMessage), so we
	// set everything to default values
	_SetDefaultTimeZone(BTimeZone(BTimeZone::kNameOfGmtZone));

	return status;
}


status_t
RosterData::_SaveLocaleSettings()
{
	BMessage settings;
	status_t status = _AddDefaultFormattingConventionsToMessage(&settings);
	if (status == B_OK)
		_AddPreferredLanguagesToMessage(&settings);
	if (status == B_OK)
		_AddFilesystemTranslationPreferenceToMessage(&settings);

	BPath path;
	if (status == B_OK)
		status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);

	BFile file;
	if (status == B_OK) {
		path.Append("Locale settings");
		status = file.SetTo(path.Path(),
			B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
	}
	if (status == B_OK)
		status = settings.Flatten(&file);
	if (status == B_OK)
		status = file.Sync();

	return status;
}


status_t
RosterData::_SaveTimeSettings()
{
	BMessage settings;
	status_t status = _AddDefaultTimeZoneToMessage(&settings);

	BPath path;
	if (status == B_OK)
		status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);

	BFile file;
	if (status == B_OK) {
		path.Append("Time settings");
		status = file.SetTo(path.Path(),
			B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
	}
	if (status == B_OK)
		status = settings.Flatten(&file);
	if (status == B_OK)
		status = file.Sync();

	return status;
}


status_t
RosterData::_SetDefaultFormattingConventions(
	const BFormattingConventions& newFormattingConventions)
{
	fDefaultLocale.SetFormattingConventions(newFormattingConventions);

	UErrorCode icuError = U_ZERO_ERROR;
	Locale icuLocale = Locale::createCanonical(newFormattingConventions.ID());
	if (icuLocale.isBogus())
		return B_ERROR;

	Locale::setDefault(icuLocale, icuError);
	if (!U_SUCCESS(icuError))
		return B_ERROR;

	return B_OK;
}


status_t
RosterData::_SetDefaultTimeZone(const BTimeZone& newZone)
{
	fDefaultTimeZone = newZone;

	TimeZone* timeZone = TimeZone::createTimeZone(newZone.ID().String());
	if (timeZone == NULL)
		return B_ERROR;
	TimeZone::adoptDefault(timeZone);

	return B_OK;
}


status_t
RosterData::_SetPreferredLanguages(const BMessage* languages)
{
	BString langName;
	if (languages != NULL
		&& languages->FindString(kLanguageField, &langName) == B_OK) {
		fDefaultLocale.SetCollator(BCollator(langName.String()));
		fDefaultLocale.SetLanguage(BLanguage(langName.String()));

		fPreferredLanguages.RemoveName(kLanguageField);
		for (int i = 0; languages->FindString(kLanguageField, i, &langName)
				== B_OK; i++) {
			fPreferredLanguages.AddString(kLanguageField, langName);
		}
	} else {
		fPreferredLanguages.MakeEmpty();
		fPreferredLanguages.AddString(kLanguageField, "en");
		fDefaultLocale.SetCollator(BCollator("en"));
	}

	return B_OK;
}


void
RosterData::_SetFilesystemTranslationPreferred(bool preferred)
{
	fIsFilesystemTranslationPreferred = preferred;
}


status_t
RosterData::_AddDefaultFormattingConventionsToMessage(BMessage* message) const
{
	BFormattingConventions conventions;
	fDefaultLocale.GetFormattingConventions(&conventions);

	return conventions.Archive(message);
}


status_t
RosterData::_AddDefaultTimeZoneToMessage(BMessage* message) const
{
	return message->AddString(kTimezoneField, fDefaultTimeZone.ID());
}


status_t
RosterData::_AddPreferredLanguagesToMessage(BMessage* message) const
{
	status_t status = B_OK;

	BString langName;
	for (int i = 0; fPreferredLanguages.FindString("language", i,
			&langName) == B_OK; i++) {
		status = message->AddString(kLanguageField, langName);
		if (status != B_OK)
			break;
	}

	return status;
}


status_t
RosterData::_AddFilesystemTranslationPreferenceToMessage(BMessage* message)
	const
{
	return message->AddBool(kTranslateFilesystemField,
		fIsFilesystemTranslationPreferred);
}


// #pragma mark - MutableLocaleRoster


namespace {


static MutableLocaleRoster sLocaleRoster;


}	// anonymous namespace


MutableLocaleRoster::MutableLocaleRoster()
{
}


MutableLocaleRoster::~MutableLocaleRoster()
{
}


/*static*/ MutableLocaleRoster*
MutableLocaleRoster::Default()
{
	return &sLocaleRoster;
}


status_t
MutableLocaleRoster::SetDefaultFormattingConventions(const BFormattingConventions& newFormattingConventions)
{
	return RosterData::Default()->SetDefaultFormattingConventions(
		newFormattingConventions);
}


status_t
MutableLocaleRoster::SetDefaultTimeZone(const BTimeZone& newZone)
{
	return RosterData::Default()->SetDefaultTimeZone(newZone);
}


status_t
MutableLocaleRoster::SetPreferredLanguages(const BMessage* languages)
{
	return RosterData::Default()->SetPreferredLanguages(languages);
}


status_t
MutableLocaleRoster::SetFilesystemTranslationPreferred(bool preferred)
{
	return RosterData::Default()->SetFilesystemTranslationPreferred(preferred);
}


status_t
MutableLocaleRoster::GetSystemCatalog(BCatalogAddOn** catalog) const
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

	if (!found) {
		log_team(LOG_DEBUG, "Unable to find libbe-image!");

		return B_ERROR;
	}

	// load the catalog for libbe and return it to the app
	entry_ref ref;
	BEntry(info.name).GetRef(&ref);

	*catalog = LoadCatalog(ref);

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
MutableLocaleRoster::CreateCatalog(const char* type, const char* signature,
	const char* language)
{
	if (!type || !signature || !language)
		return NULL;

	BAutolock lock(RosterData::Default()->fLock);
	if (!lock.IsLocked())
		return NULL;

	int32 count = RosterData::Default()->fCatalogAddOnInfos.CountItems();
	for (int32 i = 0; i < count; ++i) {
		CatalogAddOnInfo* info = (CatalogAddOnInfo*)
			RosterData::Default()->fCatalogAddOnInfos.ItemAt(i);
		if (info->fName.ICompare(type)!=0 || !info->MakeSureItsLoaded()
			|| !info->fCreateFunc)
			continue;

		BCatalogAddOn* catalog = info->fCreateFunc(signature, language);
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
MutableLocaleRoster::LoadCatalog(const entry_ref& catalogOwner, const char* language,
	int32 fingerprint) const
{
	BAutolock lock(RosterData::Default()->fLock);
	if (!lock.IsLocked())
		return NULL;

	int32 count = RosterData::Default()->fCatalogAddOnInfos.CountItems();
	for (int32 i = 0; i < count; ++i) {
		CatalogAddOnInfo* info = (CatalogAddOnInfo*)
			RosterData::Default()->fCatalogAddOnInfos.ItemAt(i);

		if (!info->MakeSureItsLoaded() || !info->fInstantiateFunc)
			continue;
		BMessage languages;
		if (language)
			// try to load catalogs for the given language:
			languages.AddString("language", language);
		else
			// try to load catalogs for one of the preferred languages:
			GetPreferredLanguages(&languages);

		BCatalogAddOn* catalog = NULL;
		const char* lang;
		for (int32 l=0; languages.FindString("language", l, &lang)==B_OK; ++l) {
			catalog = info->fInstantiateFunc(catalogOwner, lang, fingerprint);
			if (catalog)
				info->fLoadedCatalogs.AddItem(catalog);
			// Chain-load catalogs for languages that depend on
			// other languages.
			// The current implementation uses the filename in order to
			// detect dependencies (parenthood) between languages (it
			// traverses from "english_british_oxford" to "english_british"
			// to "english"):
			int32 pos;
			BString langName(lang);
			BCatalogAddOn* currCatalog = catalog;
			BCatalogAddOn* nextCatalog = NULL;
			while ((pos = langName.FindLast('_')) >= 0) {
				// language is based on parent, so we load that, too:
				// (even if the parent catalog was not found)
				langName.Truncate(pos);
				nextCatalog = info->fInstantiateFunc(catalogOwner,
					langName.String(), fingerprint);
				if (nextCatalog) {
					info->fLoadedCatalogs.AddItem(nextCatalog);
					if(currCatalog)
						currCatalog->SetNext(nextCatalog);
					else
						catalog = nextCatalog;
					currCatalog = nextCatalog;
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
 * unloads the given catalog (or rather: catalog-chain).
 * Every single catalog of the chain will be deleted automatically.
 * Add-ons that have no more current catalogs are unloaded, too.
 */
status_t
MutableLocaleRoster::UnloadCatalog(BCatalogAddOn* catalog)
{
	if (!catalog)
		return B_BAD_VALUE;

	BAutolock lock(RosterData::Default()->fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	status_t res = B_ERROR;
	BCatalogAddOn* nextCatalog;

	while (catalog != NULL) {
		nextCatalog = catalog->Next();
		int32 count = RosterData::Default()->fCatalogAddOnInfos.CountItems();
		for (int32 i = 0; i < count; ++i) {
			CatalogAddOnInfo* info = static_cast<CatalogAddOnInfo*>(
				RosterData::Default()->fCatalogAddOnInfos.ItemAt(i));
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
