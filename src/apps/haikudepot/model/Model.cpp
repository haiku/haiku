/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2014, Axel Dörfler <axeld@pinc-software.de>.
 * Copyright 2016-2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "Model.h"
#include "StorageUtils.h"

#include <ctime>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include <Autolock.h>
#include <Catalog.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <KeyStore.h>
#include <LocaleRoster.h>
#include <Message.h>
#include <Path.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Model"


static const char* kHaikuDepotKeyring = "HaikuDepot";


PackageFilter::~PackageFilter()
{
}


ModelListener::~ModelListener()
{
}


// #pragma mark - PackageFilters


class AnyFilter : public PackageFilter {
public:
	virtual bool AcceptsPackage(const PackageInfoRef& package) const
	{
		return true;
	}
};


class DepotFilter : public PackageFilter {
public:
	DepotFilter(const DepotInfo& depot)
		:
		fDepot(depot)
	{
	}

	virtual bool AcceptsPackage(const PackageInfoRef& package) const
	{
		// TODO: Maybe a PackageInfo ought to know the Depot it came from?
		// But right now the same package could theoretically be provided
		// from different depots and the filter would work correctly.
		// Also the PackageList could actually contain references to packages
		// instead of the packages as objects. The equal operator is quite
		// expensive as is.
		return fDepot.Packages().Contains(package);
	}

	const BString& Depot() const
	{
		return fDepot.Name();
	}

private:
	DepotInfo	fDepot;
};


class CategoryFilter : public PackageFilter {
public:
	CategoryFilter(const BString& category)
		:
		fCategory(category)
	{
	}

	virtual bool AcceptsPackage(const PackageInfoRef& package) const
	{
		if (package.Get() == NULL)
			return false;

		const CategoryList& categories = package->Categories();
		for (int i = categories.CountItems() - 1; i >= 0; i--) {
			const CategoryRef& category = categories.ItemAtFast(i);
			if (category.Get() == NULL)
				continue;
			if (category->Name() == fCategory)
				return true;
		}
		return false;
	}

	const BString& Category() const
	{
		return fCategory;
	}

private:
	BString		fCategory;
};


class ContainedInFilter : public PackageFilter {
public:
	ContainedInFilter(const PackageList& packageList)
		:
		fPackageList(packageList)
	{
	}

	virtual bool AcceptsPackage(const PackageInfoRef& package) const
	{
		return fPackageList.Contains(package);
	}

private:
	const PackageList&	fPackageList;
};


class ContainedInEitherFilter : public PackageFilter {
public:
	ContainedInEitherFilter(const PackageList& packageListA,
		const PackageList& packageListB)
		:
		fPackageListA(packageListA),
		fPackageListB(packageListB)
	{
	}

	virtual bool AcceptsPackage(const PackageInfoRef& package) const
	{
		return fPackageListA.Contains(package)
			|| fPackageListB.Contains(package);
	}

private:
	const PackageList&	fPackageListA;
	const PackageList&	fPackageListB;
};


class NotContainedInFilter : public PackageFilter {
public:
	NotContainedInFilter(const PackageList* packageList, ...)
	{
		va_list args;
		va_start(args, packageList);
		while (true) {
			const PackageList* packageList = va_arg(args, const PackageList*);
			if (packageList == NULL)
				break;
			fPackageLists.Add(packageList);
		}
		va_end(args);
	}

	virtual bool AcceptsPackage(const PackageInfoRef& package) const
	{
		if (package.Get() == NULL)
			return false;

		printf("TEST %s\n", package->Name().String());

		for (int32 i = 0; i < fPackageLists.CountItems(); i++) {
			if (fPackageLists.ItemAtFast(i)->Contains(package)) {
				printf("  contained in %" B_PRId32 "\n", i);
				return false;
			}
		}
		return true;
	}

private:
	List<const PackageList*, true>	fPackageLists;
};


class StateFilter : public PackageFilter {
public:
	StateFilter(PackageState state)
		:
		fState(state)
	{
	}

	virtual bool AcceptsPackage(const PackageInfoRef& package) const
	{
		return package->State() == NONE;
	}

private:
	PackageState	fState;
};


class SearchTermsFilter : public PackageFilter {
public:
	SearchTermsFilter(const BString& searchTerms)
	{
		// Separate the string into terms at spaces
		int32 index = 0;
		while (index < searchTerms.Length()) {
			int32 nextSpace = searchTerms.FindFirst(" ", index);
			if (nextSpace < 0)
				nextSpace = searchTerms.Length();
			if (nextSpace > index) {
				BString term;
				searchTerms.CopyInto(term, index, nextSpace - index);
				term.ToLower();
				fSearchTerms.Add(term);
			}
			index = nextSpace + 1;
		}
	}

	virtual bool AcceptsPackage(const PackageInfoRef& package) const
	{
		if (package.Get() == NULL)
			return false;
		// Every search term must be found in one of the package texts
		for (int32 i = fSearchTerms.CountItems() - 1; i >= 0; i--) {
			const BString& term = fSearchTerms.ItemAtFast(i);
			if (!_TextContains(package->Name(), term)
				&& !_TextContains(package->Title(), term)
				&& !_TextContains(package->Publisher().Name(), term)
				&& !_TextContains(package->ShortDescription(), term)
				&& !_TextContains(package->FullDescription(), term)) {
				return false;
			}
		}
		return true;
	}

	BString SearchTerms() const
	{
		BString searchTerms;
		for (int32 i = 0; i < fSearchTerms.CountItems(); i++) {
			const BString& term = fSearchTerms.ItemAtFast(i);
			if (term.IsEmpty())
				continue;
			if (!searchTerms.IsEmpty())
				searchTerms.Append(" ");
			searchTerms.Append(term);
		}
		return searchTerms;
	}

private:
	bool _TextContains(BString text, const BString& string) const
	{
		text.ToLower();
		int32 index = text.FindFirst(string);
		return index >= 0;
	}

private:
	StringList fSearchTerms;
};


class IsFeaturedFilter : public PackageFilter {
public:
	IsFeaturedFilter()
	{
	}

	virtual bool AcceptsPackage(const PackageInfoRef& package) const
	{
		return package.Get() != NULL && package->IsProminent();
	}
};


static inline bool
is_source_package(const PackageInfoRef& package)
{
	const BString& packageName = package->Name();
	return packageName.EndsWith("_source");
}


static inline bool
is_develop_package(const PackageInfoRef& package)
{
	const BString& packageName = package->Name();
	return packageName.EndsWith("_devel");
}


// #pragma mark - Model


Model::Model()
	:
	fDepots(),

	fCategoryAudio(new PackageCategory(
		BitmapRef(),
		B_TRANSLATE("Audio"), "audio"), true),
	fCategoryBusiness(new PackageCategory(
		BitmapRef(),
		B_TRANSLATE("Business"), "business"), true),
	fCategoryDevelopment(new PackageCategory(
		BitmapRef(),
		B_TRANSLATE("Development"), "development"), true),
	fCategoryEducation(new PackageCategory(
		BitmapRef(),
		B_TRANSLATE("Education"), "education"), true),
	fCategoryGames(new PackageCategory(
		BitmapRef(),
		B_TRANSLATE("Games"), "games"), true),
	fCategoryGraphics(new PackageCategory(
		BitmapRef(),
		B_TRANSLATE("Graphics"), "graphics"), true),
	fCategoryInternetAndNetwork(new PackageCategory(
		BitmapRef(),
		B_TRANSLATE("Internet & Network"), "internetandnetwork"), true),
	fCategoryProductivity(new PackageCategory(
		BitmapRef(),
		B_TRANSLATE("Productivity"), "productivity"), true),
	fCategoryScienceAndMathematics(new PackageCategory(
		BitmapRef(),
		B_TRANSLATE("Science & Mathematics"), "scienceandmathematics"), true),
	fCategorySystemAndUtilities(new PackageCategory(
		BitmapRef(),
		B_TRANSLATE("System & Utilities"), "systemandutilities"), true),
	fCategoryVideo(new PackageCategory(
		BitmapRef(),
		B_TRANSLATE("Video"), "video"), true),

	fCategoryFilter(PackageFilterRef(new AnyFilter(), true)),
	fDepotFilter(""),
	fSearchTermsFilter(PackageFilterRef(new AnyFilter(), true)),
	fIsFeaturedFilter(),

	fShowFeaturedPackages(true),
	fShowAvailablePackages(true),
	fShowInstalledPackages(true),
	fShowSourcePackages(false),
	fShowDevelopPackages(false),

	fPopulateAllPackagesThread(-1),
	fStopPopulatingAllPackages(false)
{
	_UpdateIsFeaturedFilter();

	// Don't forget to add new categories to this list:
	fCategories.Add(fCategoryGames);
	fCategories.Add(fCategoryBusiness);
	fCategories.Add(fCategoryAudio);
	fCategories.Add(fCategoryVideo);
	fCategories.Add(fCategoryGraphics);
	fCategories.Add(fCategoryEducation);
	fCategories.Add(fCategoryProductivity);
	fCategories.Add(fCategorySystemAndUtilities);
	fCategories.Add(fCategoryInternetAndNetwork);
	fCategories.Add(fCategoryDevelopment);
	fCategories.Add(fCategoryScienceAndMathematics);
	// TODO: The server will eventually support an API to
	// get the defined categories and their translated names.
	// This should then be used instead of hard-coded
	// categories and translations in the app.

	fPreferredLanguage = "en";
	BLocaleRoster* localeRoster = BLocaleRoster::Default();
	if (localeRoster != NULL) {
		BMessage preferredLanguages;
		if (localeRoster->GetPreferredLanguages(&preferredLanguages) == B_OK) {
			BString language;
			if (preferredLanguages.FindString("language", 0, &language) == B_OK)
				language.CopyInto(fPreferredLanguage, 0, 2);
		}
	}

	// TODO: Fetch this from the web-app.
	fSupportedLanguages.Add("en");
	fSupportedLanguages.Add("de");
	fSupportedLanguages.Add("fr");
	fSupportedLanguages.Add("ja");
	fSupportedLanguages.Add("es");
	fSupportedLanguages.Add("zh");
	fSupportedLanguages.Add("pt");
	fSupportedLanguages.Add("ru");

	if (!fSupportedLanguages.Contains(fPreferredLanguage)) {
		// Force the preferred language to one of the currently supported
		// ones, until the web application supports all ISO language codes.
		printf("User preferred language '%s' not currently supported, "
			"defaulting to 'en'.", fPreferredLanguage.String());
		fPreferredLanguage = "en";
	}
	fWebAppInterface.SetPreferredLanguage(fPreferredLanguage);
}


Model::~Model()
{
	StopPopulatingAllPackages();
}


bool
Model::AddListener(const ModelListenerRef& listener)
{
	return fListeners.Add(listener);
}


PackageList
Model::CreatePackageList() const
{
	// Iterate all packages from all depots.
	// If configured, restrict depot, filter by search terms, status, name ...
	PackageList resultList;

	for (int32 i = 0; i < fDepots.CountItems(); i++) {
		const DepotInfo& depot = fDepots.ItemAtFast(i);

		if (fDepotFilter.Length() > 0 && fDepotFilter != depot.Name())
			continue;

		const PackageList& packages = depot.Packages();

		for (int32 j = 0; j < packages.CountItems(); j++) {
			const PackageInfoRef& package = packages.ItemAtFast(j);
			if (fCategoryFilter->AcceptsPackage(package)
				&& fSearchTermsFilter->AcceptsPackage(package)
				&& fIsFeaturedFilter->AcceptsPackage(package)
				&& (fShowAvailablePackages || package->State() != NONE)
				&& (fShowInstalledPackages || package->State() != ACTIVATED)
				&& (fShowSourcePackages || !is_source_package(package))
				&& (fShowDevelopPackages || !is_develop_package(package))) {
				resultList.Add(package);
			}
		}
	}

	return resultList;
}


bool
Model::AddDepot(const DepotInfo& depot)
{
	return fDepots.Add(depot);
}


bool
Model::HasDepot(const BString& name) const
{
	return NULL != DepotForName(name);
}


const DepotInfo*
Model::DepotForName(const BString& name) const
{
	for (int32 i = fDepots.CountItems() - 1; i >= 0; i--) {
		if (fDepots.ItemAtFast(i).Name() == name)
			return &fDepots.ItemAtFast(i);
	}
	return NULL;
}


bool
Model::SyncDepot(const DepotInfo& depot)
{
	for (int32 i = fDepots.CountItems() - 1; i >= 0; i--) {
		const DepotInfo& existingDepot = fDepots.ItemAtFast(i);
		if (existingDepot.Name() == depot.Name()) {
			DepotInfo mergedDepot(existingDepot);
			mergedDepot.SyncPackages(depot.Packages());
			fDepots.Replace(i, mergedDepot);
			return true;
		}
	}
	return false;
}


void
Model::Clear()
{
	fDepots.Clear();
}


void
Model::SetPackageState(const PackageInfoRef& package, PackageState state)
{
	switch (state) {
		default:
		case NONE:
			fInstalledPackages.Remove(package);
			fActivatedPackages.Remove(package);
			fUninstalledPackages.Remove(package);
			break;
		case INSTALLED:
			if (!fInstalledPackages.Contains(package))
				fInstalledPackages.Add(package);
			fActivatedPackages.Remove(package);
			fUninstalledPackages.Remove(package);
			break;
		case ACTIVATED:
			if (!fInstalledPackages.Contains(package))
				fInstalledPackages.Add(package);
			if (!fActivatedPackages.Contains(package))
				fActivatedPackages.Add(package);
			fUninstalledPackages.Remove(package);
			break;
		case UNINSTALLED:
			fInstalledPackages.Remove(package);
			fActivatedPackages.Remove(package);
			if (!fUninstalledPackages.Contains(package))
				fUninstalledPackages.Add(package);
			break;
	}

	package->SetState(state);
}


// #pragma mark - filters


void
Model::SetCategory(const BString& category)
{
	PackageFilter* filter;

	if (category.Length() == 0)
		filter = new AnyFilter();
	else
		filter = new CategoryFilter(category);

	fCategoryFilter.SetTo(filter, true);
}


BString
Model::Category() const
{
	CategoryFilter* filter
		= dynamic_cast<CategoryFilter*>(fCategoryFilter.Get());
	if (filter == NULL)
		return "";
	return filter->Category();
}


void
Model::SetDepot(const BString& depot)
{
	fDepotFilter = depot;
}


BString
Model::Depot() const
{
	return fDepotFilter;
}


void
Model::SetSearchTerms(const BString& searchTerms)
{
	PackageFilter* filter;

	if (searchTerms.Length() == 0)
		filter = new AnyFilter();
	else
		filter = new SearchTermsFilter(searchTerms);

	fSearchTermsFilter.SetTo(filter, true);
	_UpdateIsFeaturedFilter();
}


BString
Model::SearchTerms() const
{
	SearchTermsFilter* filter
		= dynamic_cast<SearchTermsFilter*>(fSearchTermsFilter.Get());
	if (filter == NULL)
		return "";
	return filter->SearchTerms();
}


void
Model::SetShowFeaturedPackages(bool show)
{
	fShowFeaturedPackages = show;
	_UpdateIsFeaturedFilter();
}


void
Model::SetShowAvailablePackages(bool show)
{
	fShowAvailablePackages = show;
}


void
Model::SetShowInstalledPackages(bool show)
{
	fShowInstalledPackages = show;
}


void
Model::SetShowSourcePackages(bool show)
{
	fShowSourcePackages = show;
}


void
Model::SetShowDevelopPackages(bool show)
{
	fShowDevelopPackages = show;
}


// #pragma mark - information retrieval


void
Model::PopulatePackage(const PackageInfoRef& package, uint32 flags)
{
	// TODO: There should probably also be a way to "unpopulate" the
	// package information. Maybe a cache of populated packages, so that
	// packages loose their extra information after a certain amount of
	// time when they have not been accessed/displayed in the UI. Otherwise
	// HaikuDepot will consume more and more resources in the packages.
	// Especially screen-shots will be a problem eventually.
	{
		BAutolock locker(&fLock);
		bool alreadyPopulated = fPopulatedPackages.Contains(package);
		if ((flags & POPULATE_FORCE) == 0 && alreadyPopulated)
			return;
		if (!alreadyPopulated)
			fPopulatedPackages.Add(package);
	}

	if ((flags & POPULATE_USER_RATINGS) != 0) {
		// Retrieve info from web-app
		BMessage info;

		BString packageName;
		BString architecture;
		{
			BAutolock locker(&fLock);
			packageName = package->Name();
			architecture = package->Architecture();
		}

		status_t status = fWebAppInterface.RetrieveUserRatings(packageName,
			architecture, 0, 50, info);
		if (status == B_OK) {
			// Parse message
			BMessage result;
			BMessage items;
			if (info.FindMessage("result", &result) == B_OK
				&& result.FindMessage("items", &items) == B_OK) {

				BAutolock locker(&fLock);
				package->ClearUserRatings();

				int index = 0;
				while (true) {
					BString name;
					name << index++;

					BMessage item;
					if (items.FindMessage(name, &item) != B_OK)
						break;
//					item.PrintToStream();

					BString user;
					BMessage userInfo;
					if (item.FindMessage("user", &userInfo) != B_OK
						|| userInfo.FindString("nickname", &user) != B_OK) {
						// Ignore, we need the user name
						continue;
					}

					// Extract basic info, all items are optional
					BString languageCode;
					BString comment;
					double rating;
					item.FindString("naturalLanguageCode", &languageCode);
					item.FindString("comment", &comment);
					if (item.FindDouble("rating", &rating) != B_OK)
						rating = -1;
					if (comment.Length() == 0 && rating == -1) {
						// No useful information given.
						continue;
					}

					// For which version of the package was the rating?
					BString major = "?";
					BString minor = "?";
					BString micro = "";
					BMessage version;
					if (item.FindMessage("pkgVersion", &version) == B_OK) {
						version.FindString("major", &major);
						version.FindString("minor", &minor);
						version.FindString("micro", &micro);
					}
					BString versionString = major;
					versionString << ".";
					versionString << minor;
					if (micro.Length() > 0) {
						versionString << ".";
						versionString << micro;
					}
					// Add the rating to the PackageInfo
					package->AddUserRating(
						UserRating(UserInfo(user), rating,
							comment, languageCode, versionString, 0, 0)
					);
				}
			} else if (info.FindMessage("error", &result) == B_OK) {
				result.PrintToStream();
			}
		}
	}

	if ((flags & POPULATE_SCREEN_SHOTS) != 0) {
		ScreenshotInfoList screenshotInfos;
		{
			BAutolock locker(&fLock);
			screenshotInfos = package->ScreenshotInfos();
			package->ClearScreenshots();
		}
		for (int i = 0; i < screenshotInfos.CountItems(); i++) {
			const ScreenshotInfo& info = screenshotInfos.ItemAtFast(i);
			_PopulatePackageScreenshot(package, info, 320, false);
		}
	}
}


void
Model::PopulateAllPackages()
{
	StopPopulatingAllPackages();

	fStopPopulatingAllPackages = false;

	fPopulateAllPackagesThread = spawn_thread(&_PopulateAllPackagesEntry,
		"Package populator", B_NORMAL_PRIORITY, this);
	if (fPopulateAllPackagesThread >= 0)
		resume_thread(fPopulateAllPackagesThread);
}


void
Model::StopPopulatingAllPackages()
{
	if (fPopulateAllPackagesThread < 0)
		return;

	fStopPopulatingAllPackages = true;
	wait_for_thread(fPopulateAllPackagesThread, NULL);
	fPopulateAllPackagesThread = -1;
}


void
Model::SetUsername(BString username)
{
	BString password;
	if (username.Length() > 0) {
		BPasswordKey key;
		BKeyStore keyStore;
		if (keyStore.GetKey(kHaikuDepotKeyring, B_KEY_TYPE_PASSWORD, username,
				key) == B_OK) {
			password = key.Password();
		} else {
			username = "";
		}
	}
	SetAuthorization(username, password, false);
}


const BString&
Model::Username() const
{
	return fWebAppInterface.Username();
}


void
Model::SetAuthorization(const BString& username, const BString& password,
	bool storePassword)
{
	if (storePassword && username.Length() > 0 && password.Length() > 0) {
		BPasswordKey key(password, B_KEY_PURPOSE_WEB, username);
		BKeyStore keyStore;
		keyStore.AddKeyring(kHaikuDepotKeyring);
		keyStore.AddKey(kHaikuDepotKeyring, key);
	}

	BAutolock locker(&fLock);
	fWebAppInterface.SetAuthorization(username, password);

	_NotifyAuthorizationChanged();
}


// #pragma mark - private


void
Model::PopulateWebAppRepositoryCode(DepotInfo& depotInfo)
{
	if (depotInfo.BaseURL().Length() > 0) {

		BMessage repositoriesEnvelope;
		BMessage result;
		double total;
		StringList repositorySourceBaseURLs;

		repositorySourceBaseURLs.Add(depotInfo.BaseURL());

		// TODO; better API call handling around errors.

		if (fWebAppInterface.RetrieveRepositoriesForSourceBaseURLs(
			repositorySourceBaseURLs, repositoriesEnvelope) == B_OK
			&& repositoriesEnvelope.FindMessage("result", &result) == B_OK
			&& result.FindDouble("total", &total) == B_OK) {

			if ((int64)total > 0) {
				BMessage repositories;
				BMessage repository;
				BString repositoryCode;

				if (result.FindMessage("items", &repositories) == B_OK
					&& repositories.FindMessage("0", &repository) == B_OK
					&& repository.FindString("code", &repositoryCode) == B_OK) {

					depotInfo.SetWebAppRepositoryCode(repositoryCode);

					printf("did assign web app repository code '%s' to local "
						"depot '%s'\n",
						depotInfo.WebAppRepositoryCode().String(),
						depotInfo.Name().String());
				} else {
					printf("unable to find the 'code' in the api response for "
						"local depot '%s'\n",
						depotInfo.Name().String());
				}
			} else {
				printf("unable to find a repository code for '%s'\n",
					depotInfo.BaseURL().String());
			}
		} else {
			printf("unexpected result obtaining repository code for '%s'\n",
				depotInfo.BaseURL().String());
		}
	} else {
		printf("missing base url for depot info %s --> will not obtain web app "
			"repository code\n",
			depotInfo.Name().String());
	}
}


void
Model::_UpdateIsFeaturedFilter()
{
	if (fShowFeaturedPackages && SearchTerms().IsEmpty())
		fIsFeaturedFilter = PackageFilterRef(new IsFeaturedFilter(), true);
	else
		fIsFeaturedFilter = PackageFilterRef(new AnyFilter(), true);
}


int32
Model::_PopulateAllPackagesEntry(void* cookie)
{
	Model* model = static_cast<Model*>(cookie);
	model->_PopulateAllPackagesThread(true);
	model->_PopulateAllPackagesThread(false);
	model->_PopulateAllPackagesIcons();
	return 0;
}


void
Model::_PopulateAllPackagesIcons()
{
	fLocalIconStore.UpdateFromServerIfNecessary();

	int32 depotIndex = 0;
	int32 packageIndex = 0;
	int32 countIconsSet = 0;

	fprintf(stdout, "will populate all packages' icons\n");

	while (true) {
		PackageInfoRef package;
		BAutolock locker(&fLock);

		if (depotIndex > fDepots.CountItems()) {
			fprintf(stdout, "did populate %" B_PRId32 " packages' icons\n",
				countIconsSet);
			return;
		}

		const DepotInfo& depot = fDepots.ItemAt(depotIndex);
		const PackageList& packages = depot.Packages();

		if (packageIndex >= packages.CountItems()) {
			// Need the next depot
			packageIndex = 0;
			depotIndex++;
		} else {
			package = packages.ItemAt(packageIndex);
#ifdef DEBUG
			fprintf(stdout, "will populate package icon for [%s]\n",
				package->Name().String());
#endif
			if (_PopulatePackageIcon(package) == B_OK)
				countIconsSet++;

			packageIndex++;
		}
	}
}


void
Model::_PopulateAllPackagesThread(bool fromCacheOnly)
{
	int32 depotIndex = 0;
	int32 packageIndex = 0;
	PackageList bulkPackageList;

	while (!fStopPopulatingAllPackages) {
		// Obtain PackageInfoRef while keeping the depot and package lists
		// locked.
		PackageInfoRef package;
		{
			BAutolock locker(&fLock);

			if (depotIndex >= fDepots.CountItems())
				break;
			const DepotInfo& depot = fDepots.ItemAt(depotIndex);

			const PackageList& packages = depot.Packages();
			if (packageIndex >= packages.CountItems()) {
				// Need the next depot
				packageIndex = 0;
				depotIndex++;
				continue;
			}

			package = packages.ItemAt(packageIndex);
			packageIndex++;
		}

		if (package.Get() == NULL)
			continue;

		//_PopulatePackageInfo(package, fromCacheOnly);
		bulkPackageList.Add(package);
		if (bulkPackageList.CountItems() == 50) {
			_PopulatePackageInfos(bulkPackageList, fromCacheOnly);
			bulkPackageList.Clear();
		}
		// TODO: Average user rating. It needs to be shown in the
		// list view, so without the user clicking the package.
	}

	if (bulkPackageList.CountItems() > 0) {
		_PopulatePackageInfos(bulkPackageList, fromCacheOnly);
	}
}


bool
Model::_GetCacheFile(BPath& path, BFile& file, directory_which directory,
	const char* relativeLocation, const char* fileName, uint32 openMode) const
{
	if (find_directory(directory, &path) == B_OK
		&& path.Append(relativeLocation) == B_OK
		&& create_directory(path.Path(), 0777) == B_OK
		&& path.Append(fileName) == B_OK) {
		// Try opening the file which will fail if its
		// not a file or does not exist.
		return file.SetTo(path.Path(), openMode) == B_OK;
	}
	return false;
}


bool
Model::_GetCacheFile(BPath& path, BFile& file, directory_which directory,
	const char* relativeLocation, const char* fileName,
	bool ignoreAge, time_t maxAge) const
{
	if (!_GetCacheFile(path, file, directory, relativeLocation, fileName,
			B_READ_ONLY)) {
		return false;
	}

	if (ignoreAge)
		return true;

	time_t modifiedTime;
	file.GetModificationTime(&modifiedTime);
	time_t now;
	time(&now);
	return now - modifiedTime < maxAge;
}


void
Model::_PopulatePackageInfos(PackageList& packages, bool fromCacheOnly)
{
	if (fStopPopulatingAllPackages)
		return;

	// See if there are cached info files
	for (int i = packages.CountItems() - 1; i >= 0; i--) {
		if (fStopPopulatingAllPackages)
			return;

		const PackageInfoRef& package = packages.ItemAtFast(i);

		BFile file;
		BPath path;
		BString name(package->Name());
		name << ".info";
		if (_GetCacheFile(path, file, B_USER_CACHE_DIRECTORY,
			"HaikuDepot", name, fromCacheOnly, 60 * 60)) {
			// Cache file is recent enough, just use it and return.
			BMessage pkgInfo;
			if (pkgInfo.Unflatten(&file) == B_OK) {
				_PopulatePackageInfo(package, pkgInfo);
				packages.Remove(i);
			}
		}
	}

	if (fromCacheOnly || packages.IsEmpty())
		return;

	// Retrieve info from web-app
	BMessage info;

	StringList packageNames;
	StringList packageArchitectures;
	StringList repositoryCodes;

	for (int i = 0; i < packages.CountItems(); i++) {
		const PackageInfoRef& package = packages.ItemAtFast(i);
		packageNames.Add(package->Name());

		if (!packageArchitectures.Contains(package->Architecture()))
			packageArchitectures.Add(package->Architecture());

		const DepotInfo *depot = DepotForName(package->DepotName());

		if (depot != NULL) {
			BString repositoryCode = depot->WebAppRepositoryCode();

			if (repositoryCode.Length() != 0
				&& !repositoryCodes.Contains(repositoryCode)) {
				repositoryCodes.Add(repositoryCode);
			}
		}
	}

	if (repositoryCodes.CountItems() != 0) {
		status_t status = fWebAppInterface.RetrieveBulkPackageInfo(packageNames,
			packageArchitectures, repositoryCodes, info);

		if (status == B_OK) {
			// Parse message
			BMessage result;
			BMessage pkgs;
			if (info.FindMessage("result", &result) == B_OK
				&& result.FindMessage("pkgs", &pkgs) == B_OK) {
				int32 index = 0;
				while (true) {
					if (fStopPopulatingAllPackages)
						return;
					BString name;
					name << index++;
					BMessage pkgInfo;
					if (pkgs.FindMessage(name, &pkgInfo) != B_OK)
						break;

					BString pkgName;
					if (pkgInfo.FindString("name", &pkgName) != B_OK)
						continue;

					// Find the PackageInfoRef
					bool found = false;
					for (int i = 0; i < packages.CountItems(); i++) {
						const PackageInfoRef& package = packages.ItemAtFast(i);
						if (pkgName == package->Name()) {
							_PopulatePackageInfo(package, pkgInfo);

							// Store in cache
							BFile file;
							BPath path;
							BString fileName(package->Name());
							fileName << ".info";
							if (_GetCacheFile(path, file, B_USER_CACHE_DIRECTORY,
									"HaikuDepot", fileName,
									B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE)) {
								pkgInfo.Flatten(&file);
							}

							packages.Remove(i);
							found = true;
							break;
						}
					}
					if (!found)
						printf("No matching package for %s\n", pkgName.String());
				}
			}
		} else {
			printf("Error sending request: %s\n", strerror(status));
			int count = packages.CountItems();
			if (count >= 4) {
				// Retry in smaller chunks
				PackageList firstHalf;
				PackageList secondHalf;
				for (int i = 0; i < count / 2; i++)
					firstHalf.Add(packages.ItemAtFast(i));
				for (int i = count / 2; i < count; i++)
					secondHalf.Add(packages.ItemAtFast(i));
				packages.Clear();
				_PopulatePackageInfos(firstHalf, fromCacheOnly);
				_PopulatePackageInfos(secondHalf, fromCacheOnly);
			} else {
				while (packages.CountItems() > 0) {
					const PackageInfoRef& package = packages.ItemAtFast(0);
					_PopulatePackageInfo(package, fromCacheOnly);
					packages.Remove(0);
				}
			}
		}
	}

	if (packages.CountItems() > 0) {
		for (int i = 0; i < packages.CountItems(); i++) {
			const PackageInfoRef& package = packages.ItemAtFast(i);
			printf("No package info for %s\n", package->Name().String());
		}
	}
}


void
Model::_PopulatePackageInfo(const PackageInfoRef& package, bool fromCacheOnly)
{
	if (fromCacheOnly)
		return;

	BString repositoryCode;
	const DepotInfo* depot = DepotForName(package->DepotName());

	if (depot != NULL) {
		repositoryCode = depot->WebAppRepositoryCode();

		if (repositoryCode.Length() > 0) {
			// Retrieve info from web-app
			BMessage info;

			status_t status = fWebAppInterface.RetrievePackageInfo(
				package->Name(), package->Architecture(), repositoryCode,
				info);

			if (status == B_OK) {
				// Parse message
		//		info.PrintToStream();
				BMessage result;
				if (info.FindMessage("result", &result) == B_OK)
					_PopulatePackageInfo(package, result);
			}
		} else {
			printf("unable to find the web app repository code for depot; %s\n",
				package->DepotName().String());
		}
	} else {
		printf("no depot for name; %s\n",
			package->DepotName().String());
	}
}


static void
append_word_list(BString& words, const char* word)
{
	if (words.Length() > 0)
		words << ", ";
	words << word;
}


void
Model::_PopulatePackageInfo(const PackageInfoRef& package, const BMessage& data)
{
	BAutolock locker(&fLock);

	BString foundInfo;

	BMessage versions;
	if (data.FindMessage("versions", &versions) == B_OK) {
		// Search a summary and description in the preferred language
		int32 index = 0;
		while (true) {
			BString name;
			name << index++;
			BMessage version;
			if (versions.FindMessage(name, &version) != B_OK)
				break;

			BString title;
			if (version.FindString("title", &title) == B_OK) {
				package->SetTitle(title);
				append_word_list(foundInfo, "title");
			}
			BString summary;
			if (version.FindString("summary", &summary) == B_OK) {
				package->SetShortDescription(summary);
				append_word_list(foundInfo, "summary");
			}
			BString description;
			if (version.FindString("description", &description) == B_OK) {
				package->SetFullDescription(description);
				append_word_list(foundInfo, "description");
			}
			double payloadLength;
			if (version.FindDouble("payloadLength", &payloadLength) == B_OK) {
				package->SetSize((int64)payloadLength);
				append_word_list(foundInfo, "size");
			}
			break;
		}
	}

	BMessage categories;
	if (data.FindMessage("pkgCategoryCodes", &categories) == B_OK) {
		bool foundCategory = false;
		int32 index = 0;
		while (true) {
			BString name;
			name << index++;
			BString category;
			if (categories.FindString(name, &category) != B_OK)
				break;

			package->ClearCategories();
			for (int i = fCategories.CountItems() - 1; i >= 0; i--) {
				const CategoryRef& categoryRef = fCategories.ItemAtFast(i);
				if (categoryRef->Name() == category) {
					package->AddCategory(categoryRef);
					foundCategory = true;
					break;
				}
			}
		}
		if (foundCategory)
			append_word_list(foundInfo, "categories");
	}

	double derivedRating;
	if (data.FindDouble("derivedRating", &derivedRating) == B_OK) {
		RatingSummary summary;
		summary.averageRating = derivedRating;
		package->SetRatingSummary(summary);

		append_word_list(foundInfo, "rating");
	}

	double prominenceOrdering;
	if (data.FindDouble("prominenceOrdering", &prominenceOrdering) == B_OK) {
		package->SetProminence(prominenceOrdering);

		append_word_list(foundInfo, "prominence");
	}

	BString changelog;
	if (data.FindString("pkgChangelogContent", &changelog) == B_OK) {
		package->SetChangelog(changelog);

		append_word_list(foundInfo, "changelog");
	}

	BMessage screenshots;
	if (data.FindMessage("pkgScreenshots", &screenshots) == B_OK) {
		package->ClearScreenshotInfos();
		bool foundScreenshot = false;
		int32 index = 0;
		while (true) {
			BString name;
			name << index++;

			BMessage screenshot;
			if (screenshots.FindMessage(name, &screenshot) != B_OK)
				break;

			BString code;
			double width;
			double height;
			double dataSize;
			if (screenshot.FindString("code", &code) == B_OK
				&& screenshot.FindDouble("width", &width) == B_OK
				&& screenshot.FindDouble("height", &height) == B_OK
				&& screenshot.FindDouble("length", &dataSize) == B_OK) {
				package->AddScreenshotInfo(ScreenshotInfo(code, (int32)width,
					(int32)height, (int32)dataSize));
				foundScreenshot = true;
			}
		}
		if (foundScreenshot)
			append_word_list(foundInfo, "screenshots");
	}

	if (foundInfo.Length() > 0) {
		printf("Populated package info for %s: %s\n",
			package->Name().String(), foundInfo.String());
	}

	// If the user already clicked this package, remove it from the
	// list of populated packages, so that clicking it again will
	// populate any additional information.
	// TODO: Trigger re-populating if the package is currently showing.
	fPopulatedPackages.Remove(package);
}


status_t
Model::_PopulatePackageIcon(const PackageInfoRef& package)
{
	BPath bestIconPath;

	if ( fLocalIconStore.TryFindIconPath(
		package->Name(), bestIconPath) == B_OK) {

		BFile bestIconFile(bestIconPath.Path(), O_RDONLY);
		BitmapRef bitmapRef(new(std::nothrow)SharedBitmap(bestIconFile), true);
		BAutolock locker(&fLock);
		package->SetIcon(bitmapRef);

#ifdef DEBUG
		fprintf(stdout, "have set the package icon for [%s] from [%s]\n",
			package->Name().String(), bestIconPath.Path());
#endif

		return B_OK;
	}

#ifdef DEBUG
	fprintf(stdout, "did not set the package icon for [%s]; no data\n",
		package->Name().String());
#endif

	return B_FILE_NOT_FOUND;
}


void
Model::_PopulatePackageScreenshot(const PackageInfoRef& package,
	const ScreenshotInfo& info, int32 scaledWidth, bool fromCacheOnly)
{
	// See if there is a cached screenshot
	BFile screenshotFile;
	BPath screenshotCachePath;
	bool fileExists = false;
	BString screenshotName(info.Code());
	screenshotName << "@" << scaledWidth;
	screenshotName << ".png";
	time_t modifiedTime;
	if (find_directory(B_USER_CACHE_DIRECTORY, &screenshotCachePath) == B_OK
		&& screenshotCachePath.Append("HaikuDepot/Screenshots") == B_OK
		&& create_directory(screenshotCachePath.Path(), 0777) == B_OK
		&& screenshotCachePath.Append(screenshotName) == B_OK) {
		// Try opening the file in read-only mode, which will fail if its
		// not a file or does not exist.
		fileExists = screenshotFile.SetTo(screenshotCachePath.Path(),
			B_READ_ONLY) == B_OK;
		if (fileExists)
			screenshotFile.GetModificationTime(&modifiedTime);
	}

	if (fileExists) {
		time_t now;
		time(&now);
		if (fromCacheOnly || now - modifiedTime < 60 * 60) {
			// Cache file is recent enough, just use it and return.
			BitmapRef bitmapRef(new(std::nothrow)SharedBitmap(screenshotFile),
				true);
			BAutolock locker(&fLock);
			package->AddScreenshot(bitmapRef);
			return;
		}
	}

	if (fromCacheOnly)
		return;

	// Retrieve screenshot from web-app
	BMallocIO buffer;

	int32 scaledHeight = scaledWidth * info.Height() / info.Width();

	status_t status = fWebAppInterface.RetrieveScreenshot(info.Code(),
		scaledWidth, scaledHeight, &buffer);
	if (status == B_OK) {
		BitmapRef bitmapRef(new(std::nothrow)SharedBitmap(buffer), true);
		BAutolock locker(&fLock);
		package->AddScreenshot(bitmapRef);
		locker.Unlock();
		if (screenshotFile.SetTo(screenshotCachePath.Path(),
				B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE) == B_OK) {
			screenshotFile.Write(buffer.Buffer(), buffer.BufferLength());
		}
	} else {
		fprintf(stderr, "Failed to retrieve screenshot for code '%s' "
			"at %" B_PRIi32 "x%" B_PRIi32 ".\n", info.Code().String(),
			scaledWidth, scaledHeight);
	}
}


// #pragma mark - listener notification methods


void
Model::_NotifyAuthorizationChanged()
{
	for (int32 i = fListeners.CountItems() - 1; i >= 0; i--) {
		const ModelListenerRef& listener = fListeners.ItemAtFast(i);
		if (listener.Get() != NULL)
			listener->AuthorizationChanged();
	}
}
