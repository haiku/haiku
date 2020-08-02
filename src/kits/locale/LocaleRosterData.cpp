/*
 * Copyright 2003-2012, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Oliver Tappe, zooey@hirschkaefer.de
 */


#include <unicode/uversion.h>
#include <LocaleRosterData.h>

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
#include <PathFinder.h>
#include <Roster.h>
#include <String.h>
#include <StringList.h>
#include <TimeZone.h>

// ICU includes
#include <unicode/locid.h>
#include <unicode/timezone.h>


U_NAMESPACE_USE


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
		BCatalogData* cat
			= static_cast<BCatalogData*>(fLoadedCatalogs.ItemAt(i));
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
		} else
			return false;
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
	}
}


// #pragma mark - LocaleRosterData


namespace {


static const char* kPriorityAttr = "ADDON:priority";

static const char* kLanguageField = "language";
static const char* kTimezoneField = "timezone";
static const char* kTranslateFilesystemField = "filesys";


}	// anonymous namespace


LocaleRosterData::LocaleRosterData(const BLanguage& language,
	const BFormattingConventions& conventions)
	:
	fLock("LocaleRosterData"),
	fDefaultLocale(&language, &conventions),
	fIsFilesystemTranslationPreferred(true),
	fAreResourcesLoaded(false)
{
	fInitStatus = _Initialize();
}


LocaleRosterData::~LocaleRosterData()
{
	BAutolock lock(fLock);

	_CleanupCatalogAddOns();
}


status_t
LocaleRosterData::InitCheck() const
{
	return fAreResourcesLoaded ? B_OK : B_NO_INIT;
}


status_t
LocaleRosterData::Refresh()
{
	BAutolock lock(fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	_LoadLocaleSettings();
	_LoadTimeSettings();

	return B_OK;
}


int
LocaleRosterData::CompareInfos(const void* left, const void* right)
{
	const CatalogAddOnInfo* leftInfo
		= * static_cast<const CatalogAddOnInfo* const *>(left);
	const CatalogAddOnInfo* rightInfo
		= * static_cast<const CatalogAddOnInfo* const *>(right);

	return leftInfo->fPriority - rightInfo->fPriority;
}


status_t
LocaleRosterData::SetDefaultFormattingConventions(
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
LocaleRosterData::SetDefaultTimeZone(const BTimeZone& newZone)
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
LocaleRosterData::SetPreferredLanguages(const BMessage* languages)
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
LocaleRosterData::SetFilesystemTranslationPreferred(bool preferred)
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
LocaleRosterData::GetResources(BResources** resources)
{
	if (resources == NULL)
		return B_BAD_VALUE;

	if (!fAreResourcesLoaded) {
		status_t result
			= fResources.SetToImage((const void*)&BLocaleRoster::Default);
		if (result != B_OK)
			return result;

		result = fResources.PreloadResourceType();
		if (result != B_OK)
			return result;

		fAreResourcesLoaded = true;
	}

	*resources = &fResources;
	return B_OK;
}


status_t
LocaleRosterData::_Initialize()
{
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
LocaleRosterData::_InitializeCatalogAddOns()
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

	BStringList folders;
	BPathFinder::FindPaths(B_FIND_PATH_ADD_ONS_DIRECTORY, "locale/catalogs/",
		B_FIND_PATH_EXISTING_ONLY, folders);

	BPath addOnPath;
	BDirectory addOnFolder;
	char buf[4096];
	status_t err;
	for (int32 f = 0; f < folders.CountStrings(); f++) {
		BString addOnFolderName = folders.StringAt(f);
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
		while ((count = addOnFolder.GetNextDirents((dirent*)buf, sizeof(buf)))
				> 0) {
			dent = (dirent*)buf;
			while (count-- > 0) {
				if (strcmp(dent->d_name, ".") != 0
						&& strcmp(dent->d_name, "..") != 0
						&& strcmp(dent->d_name, "x86") != 0
						&& strcmp(dent->d_name, "x86_gcc2") != 0) {
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
							}
							unload_add_on(image);
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
LocaleRosterData::_CleanupCatalogAddOns()
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
LocaleRosterData::_LoadLocaleSettings()
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
LocaleRosterData::_LoadTimeSettings()
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
LocaleRosterData::_SaveLocaleSettings()
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
LocaleRosterData::_SaveTimeSettings()
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
LocaleRosterData::_SetDefaultFormattingConventions(
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
LocaleRosterData::_SetDefaultTimeZone(const BTimeZone& newZone)
{
	fDefaultTimeZone = newZone;

	TimeZone* timeZone = TimeZone::createTimeZone(newZone.ID().String());
	if (timeZone == NULL)
		return B_ERROR;
	TimeZone::adoptDefault(timeZone);

	return B_OK;
}


status_t
LocaleRosterData::_SetPreferredLanguages(const BMessage* languages)
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
LocaleRosterData::_SetFilesystemTranslationPreferred(bool preferred)
{
	fIsFilesystemTranslationPreferred = preferred;
}


status_t
LocaleRosterData::_AddDefaultFormattingConventionsToMessage(
	BMessage* message) const
{
	BFormattingConventions conventions;
	fDefaultLocale.GetFormattingConventions(&conventions);

	return conventions.Archive(message);
}


status_t
LocaleRosterData::_AddDefaultTimeZoneToMessage(BMessage* message) const
{
	return message->AddString(kTimezoneField, fDefaultTimeZone.ID());
}


status_t
LocaleRosterData::_AddPreferredLanguagesToMessage(BMessage* message) const
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
LocaleRosterData::_AddFilesystemTranslationPreferenceToMessage(
	BMessage* message) const
{
	return message->AddBool(kTranslateFilesystemField,
		fIsFilesystemTranslationPreferred);
}


}	// namespace BPrivate
