/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2014, Axel Dörfler <axeld@pinc-software.de>.
 * Copyright 2016-2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "Model.h"

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

#include "Logger.h"
#include "StorageUtils.h"
#include "RepositoryUrlUtils.h"


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
	return packageName.EndsWith("_devel")
		|| packageName.EndsWith("_debuginfo");
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
	fShowDevelopPackages(false)
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
	fSupportedLanguages.Add("es");
	fSupportedLanguages.Add("de");
	fSupportedLanguages.Add("fr");
	fSupportedLanguages.Add("it");
	fSupportedLanguages.Add("ja");
	fSupportedLanguages.Add("pt");
	fSupportedLanguages.Add("ru");
	fSupportedLanguages.Add("sk");
	fSupportedLanguages.Add("zh");

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
			if (MatchesFilter(package))
				resultList.Add(package);
		}
	}

	return resultList;
}


bool
Model::MatchesFilter(const PackageInfoRef& package) const
{
	return fCategoryFilter->AcceptsPackage(package)
			&& fSearchTermsFilter->AcceptsPackage(package)
			&& fIsFeaturedFilter->AcceptsPackage(package)
			&& (fShowAvailablePackages || package->State() != NONE)
			&& (fShowInstalledPackages || package->State() != ACTIVATED)
			&& (fShowSourcePackages || !is_source_package(package))
			&& (fShowDevelopPackages || !is_develop_package(package));
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


/*! Initially only superficial data is loaded from the server into the data
    model of the packages.  When the package is viewed, additional data needs
    to be populated including ratings.  This method takes care of that.
*/

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

	if ((flags & POPULATE_CHANGELOG) != 0) {
		_PopulatePackageChangelog(package);
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

				int32 index = 0;
				while (true) {
					BString name;
					name << index++;

					BMessage item;
					if (items.FindMessage(name, &item) != B_OK)
						break;

					BString code;
					if (item.FindString("code", &code) != B_OK) {
						printf("corrupt user rating at index %" B_PRIi32 "\n",
							index);
						continue;
					}

					BString user;
					BMessage userInfo;
					if (item.FindMessage("user", &userInfo) != B_OK
						|| userInfo.FindString("nickname", &user) != B_OK) {
						printf("ignored user rating [%s] without a user "
							"nickname\n", code.String());
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
						printf("rating [%s] has no comment or rating so will be"
							"ignored\n", code.String());
						continue;
					}

					// For which version of the package was the rating?
					BString major = "?";
					BString minor = "?";
					BString micro = "";
					double revision = -1;
					BMessage version;
					if (item.FindMessage("pkgVersion", &version) == B_OK) {
						version.FindString("major", &major);
						version.FindString("minor", &minor);
						version.FindString("micro", &micro);
						version.FindDouble("revision", &revision);
					}
					BString versionString = major;
					versionString << ".";
					versionString << minor;
					if (micro.Length() > 0) {
						versionString << ".";
						versionString << micro;
					}
					if (revision > 0) {
						versionString << "-";
						versionString << (int) revision;
					}

					BDateTime createTimestamp;
					double createTimestampMillisF;
					if (item.FindDouble("createTimestamp",
						&createTimestampMillisF) == B_OK) {
						double createTimestampSecsF =
							createTimestampMillisF / 1000.0;
						time_t createTimestampSecs =
							(time_t) createTimestampSecsF;
						createTimestamp.SetTime_t(createTimestampSecs);
					}

					// Add the rating to the PackageInfo
					UserRating userRating = UserRating(UserInfo(user), rating,
						comment, languageCode, versionString, 0, 0,
						createTimestamp);
					package->AddUserRating(userRating);

					if (Logger::IsDebugEnabled()) {
						printf("rating [%s] retrieved from server\n",
							code.String());
					}
				}

				if (Logger::IsDebugEnabled()) {
					printf("did retrieve %" B_PRIi32 " user ratings for [%s]\n",
						index - 1, packageName.String());
				}
			} else {
				_MaybeLogJsonRpcError(info, "retrieve user ratings");
			}
		} else {
			printf("unable to retrieve user ratings\n");
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
Model::_PopulatePackageChangelog(const PackageInfoRef& package)
{
	BMessage info;
	BString packageName;

	{
		BAutolock locker(&fLock);
		packageName = package->Name();
	}

	status_t status = fWebAppInterface.GetChangelog(packageName, info);

	if (status == B_OK) {
		// Parse message
		BMessage result;
		BString content;
		if (info.FindMessage("result", &result) == B_OK) {
			if (result.FindString("content", &content) == B_OK
				&& 0 != content.Length()) {
				BAutolock locker(&fLock);
				package->SetChangelog(content);
				if (Logger::IsDebugEnabled()) {
					fprintf(stdout, "changelog populated for [%s]\n",
						packageName.String());
				}
			} else {
				if (Logger::IsDebugEnabled()) {
					fprintf(stdout, "no changelog present for [%s]\n",
						packageName.String());
				}
			}
		} else {
			_MaybeLogJsonRpcError(info, "populate package changelog");
		}
	} else {
		fprintf(stdout, "unable to obtain the changelog for the package"
			" [%s]\n", packageName.String());
	}
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


/*! When bulk repository data comes down from the server, it will
    arrive as a json.gz payload.  This is stored locally as a cache
    and this method will provide the on-disk storage location for
    this file.
*/

status_t
Model::DumpExportRepositoryDataPath(BPath& path) const
{
	BPath repoDataPath;

	if (find_directory(B_USER_CACHE_DIRECTORY, &repoDataPath) == B_OK
		&& repoDataPath.Append("HaikuDepot") == B_OK
		&& create_directory(repoDataPath.Path(), 0777) == B_OK
		&& repoDataPath.Append("repository-all_en.json.gz") == B_OK) {
		path.SetTo(repoDataPath.Path());
		return B_OK;
	}

	path.Unset();
	fprintf(stdout, "unable to find the user cache file for repositories'"
		" data");
	return B_ERROR;
}


status_t
Model::IconStoragePath(BPath& path) const
{
	BPath iconStoragePath;

	if (find_directory(B_USER_CACHE_DIRECTORY, &iconStoragePath) == B_OK
		&& iconStoragePath.Append("HaikuDepot") == B_OK
		&& iconStoragePath.Append("__allicons") == B_OK
		&& create_directory(iconStoragePath.Path(), 0777) == B_OK) {
		path.SetTo(iconStoragePath.Path());
		return B_OK;
	}

	path.Unset();
	fprintf(stdout, "unable to find the user cache directory for icons");
	return B_ERROR;
}


status_t
Model::DumpExportPkgDataPath(BPath& path,
	const BString& repositorySourceCode) const
{
	BPath repoDataPath;
	BString leafName;

	leafName.SetToFormat("pkg-all-%s-%s.json.gz", repositorySourceCode.String(),
		fPreferredLanguage.String());

	if (find_directory(B_USER_CACHE_DIRECTORY, &repoDataPath) == B_OK
		&& repoDataPath.Append("HaikuDepot") == B_OK
		&& create_directory(repoDataPath.Path(), 0777) == B_OK
		&& repoDataPath.Append(leafName.String()) == B_OK) {
		path.SetTo(repoDataPath.Path());
		return B_OK;
	}

	path.Unset();
	fprintf(stdout, "unable to find the user cache file for pkgs' data");
	return B_ERROR;
}


void
Model::_UpdateIsFeaturedFilter()
{
	if (fShowFeaturedPackages && SearchTerms().IsEmpty())
		fIsFeaturedFilter = PackageFilterRef(new IsFeaturedFilter(), true);
	else
		fIsFeaturedFilter = PackageFilterRef(new AnyFilter(), true);
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


/*! This method will find the stored 'DepotInfo' that correlates to the
    supplied 'url' and will invoke the mapper function in order to get a
    replacement for the 'DepotInfo'.  The 'url' is a unique identifier
    for the repository that holds across mirrors.
*/

void
Model::ReplaceDepotByUrl(const BString& URL, DepotMapper* depotMapper,
	void* context)
{
	for (int32 i = 0; i < fDepots.CountItems(); i++) {
		DepotInfo depotInfo = fDepots.ItemAtFast(i);

		if (RepositoryUrlUtils::EqualsNormalized(URL, depotInfo.URL())) {
			BAutolock locker(&fLock);
			fDepots.Replace(i, depotMapper->MapDepot(depotInfo, context));
		}
	}
}


void
Model::LogDepotsWithNoWebAppRepositoryCode() const
{
	int32 i;

	for (i = 0; i < fDepots.CountItems(); i++) {
		const DepotInfo& depot = fDepots.ItemAt(i);

		if (depot.WebAppRepositoryCode().Length() == 0) {
			printf("depot [%s]", depot.Name().String());

			if (depot.URL().Length() > 0)
				printf(" (%s)", depot.URL().String());

			printf(" correlates with no repository in the haiku"
				"depot server system\n");
		}
	}
}


void
Model::_MaybeLogJsonRpcError(const BMessage &responsePayload,
	const char *sourceDescription) const
{
	BMessage error;
	BString errorMessage;
	double errorCode;

	if (responsePayload.FindMessage("error", &error) == B_OK
		&& error.FindString("message", &errorMessage) == B_OK
		&& error.FindDouble("code", &errorCode) == B_OK) {
		printf("[%s] --> error : [%s] (%f)\n", sourceDescription,
			errorMessage.String(), errorCode);

	} else {
		printf("[%s] --> an undefined error has occurred\n", sourceDescription);
	}
}
