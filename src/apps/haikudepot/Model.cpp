/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2014, Axel Dörfler <axeld@pinc-software.de>.
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
#include <FindDirectory.h>
#include <File.h>
#include <LocaleRoster.h>
#include <Message.h>
#include <Path.h>

#include "WebAppInterface.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Model"


// #pragma mark - PackageFilters


PackageFilter::~PackageFilter()
{
}


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
		if (package.Get()==NULL)
			return false;

		printf("TEST %s\n", package->Title().String());

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
			if (!_TextContains(package->Title(), term)
				&& !_TextContains(package->Publisher().Name(), term)
				&& !_TextContains(package->ShortDescription(), term)
				&& !_TextContains(package->FullDescription(), term)) {
				return false;
			}
		}
		return true;
	}

private:
	bool _TextContains(BString text, const BString& string) const
	{
		text.ToLower();
		int32 index = text.FindFirst(string);
		return index >= 0;
	}

private:
	List<BString, false>	fSearchTerms;
};


static inline bool
is_source_package(const PackageInfoRef& package)
{
	const BString& packageName = package->Title();
	return packageName.EndsWith("_source");
}


static inline bool
is_develop_package(const PackageInfoRef& package)
{
	const BString& packageName = package->Title();
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

	fShowAvailablePackages(true),
	fShowInstalledPackages(false),
	fShowSourcePackages(false),
	fShowDevelopPackages(false),

	fPopulateAllPackagesThread(-1),
	fStopPopulatingAllPackages(false)
{
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
}


Model::~Model()
{
	StopPopulatingAllPackages();
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


void
Model::Clear()
{
	StopPopulatingAllPackages();
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


void
Model::SetDepot(const BString& depot)
{
	fDepotFilter = depot;
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


// #pragma mark - information retrival


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
		if (fPopulatedPackages.Contains(package))
			return;
		fPopulatedPackages.Add(package);
	}

	if ((flags & POPULATE_USER_RATINGS) != 0) {
		// Retrieve info from web-app
		WebAppInterface interface;
		interface.SetPreferredLanguage(fPreferredLanguage);
		BMessage info;

		BString packageName;
		BString architecture;	
		{
			BAutolock locker(&fLock);
			packageName = package->Title();
			architecture = package->Architecture();
		}
	
		status_t status = interface.RetrieveUserRatings(packageName,
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


int32
Model::_PopulateAllPackagesEntry(void* cookie)
{
	Model* model = static_cast<Model*>(cookie);

	model->fPreferredLanguage = "en";
	BLocaleRoster* localeRoster = BLocaleRoster::Default();
	if (localeRoster != NULL) {
		BMessage preferredLanguages;
		if (localeRoster->GetPreferredLanguages(&preferredLanguages) == B_OK) {
			BString language;
			if (preferredLanguages.FindString("language", 0, &language) == B_OK)
				language.CopyInto(model->fPreferredLanguage, 0, 2);
		}
	}

	model->_PopulateAllPackagesThread(true);
	model->_PopulateAllPackagesThread(false);
	return 0;
}


void
Model::_PopulateAllPackagesThread(bool fromCacheOnly)
{
	int32 depotIndex = 0;
	int32 packageIndex = 0;
	PackageList bulkPackageList;
	PackageList packagesWithIconsList;

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
			_PopulatePackageInfos(bulkPackageList, fromCacheOnly,
				packagesWithIconsList);
			bulkPackageList.Clear();
		}
		if (fromCacheOnly)
			_PopulatePackageIcon(package, fromCacheOnly);
		// TODO: Average user rating. It needs to be shown in the
		// list view, so without the user clicking the package.
	}

	if (bulkPackageList.CountItems() > 0) {
		_PopulatePackageInfos(bulkPackageList, fromCacheOnly,
			packagesWithIconsList);
	}

	if (!fromCacheOnly) {
		for (int i = packagesWithIconsList.CountItems() - 1; i >= 0; i--) {
			if (fStopPopulatingAllPackages)
				break;
			const PackageInfoRef& package = packagesWithIconsList.ItemAtFast(i);
			printf("Getting/Updating native icon for %s\n",
				package->Title().String());
			_PopulatePackageIcon(package, fromCacheOnly);
		}
	}
}


void
Model::_PopulatePackageInfos(PackageList& packages, bool fromCacheOnly,
	PackageList& packagesWithIcons)
{
	if (fStopPopulatingAllPackages)
		return;
	
	if (fromCacheOnly)
		return;
	
	// Retrieve info from web-app
	WebAppInterface interface;
	interface.SetPreferredLanguage(fPreferredLanguage);
	BMessage info;

	StringList packageNames;
	StringList packageArchitectures;
	for (int i = 0; i < packages.CountItems(); i++) {
		const PackageInfoRef& package = packages.ItemAtFast(i);
		packageNames.Add(package->Title());
		packageArchitectures.Add(package->Architecture());
	}

	status_t status = interface.RetrieveBulkPackageInfo(packageNames,
		packageArchitectures, info);
	if (status == B_OK) {
		// Parse message
//		info.PrintToStream();
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
					if (pkgName == package->Title()) {
						_PopulatePackageInfo(package, pkgInfo);
						if (_HasNativeIcon(pkgInfo))
							packagesWithIcons.Add(package);
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
			_PopulatePackageInfos(firstHalf, fromCacheOnly, packagesWithIcons);
			_PopulatePackageInfos(secondHalf, fromCacheOnly, packagesWithIcons);
		} else {
			while (packages.CountItems() > 0) {
				const PackageInfoRef& package = packages.ItemAtFast(0);
				_PopulatePackageInfo(package, fromCacheOnly);
				packages.Remove(0);
			}
		}
	}

	if (packages.CountItems() > 0) {
		for (int i = 0; i < packages.CountItems(); i++) {
			const PackageInfoRef& package = packages.ItemAtFast(i);
			printf("No package info for %s\n", package->Title().String());
		}
	}
}


void
Model::_PopulatePackageInfo(const PackageInfoRef& package, bool fromCacheOnly)
{
	if (fromCacheOnly)
		return;
	
	// Retrieve info from web-app
	WebAppInterface interface;
	interface.SetPreferredLanguage(fPreferredLanguage);
	BMessage info;

	status_t status = interface.RetrievePackageInfo(package->Title(),
		package->Architecture(), info);
	if (status == B_OK) {
		// Parse message
//		info.PrintToStream();
		BMessage result;
		if (info.FindMessage("result", &result) == B_OK)
			_PopulatePackageInfo(package, result);
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
			BString languageCode;
			if (version.FindString("naturalLanguageCode",
					&languageCode) != B_OK 
				|| languageCode != fPreferredLanguage) {
				continue;
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
			package->Title().String(), foundInfo.String());
	}

	// If the user already clicked this package, remove it from the
	// list of populated packages, so that clicking it again will
	// populate any additional information.
	// TODO: Trigger re-populating if the package is currently showing.
	fPopulatedPackages.Remove(package);
}


void
Model::_PopulatePackageIcon(const PackageInfoRef& package, bool fromCacheOnly)
{
	// See if there is a cached icon file
	BFile iconFile;
	BPath iconCachePath;
	bool fileExists = false;
	BString iconName(package->Title());
	iconName << ".hvif";
	time_t modifiedTime;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &iconCachePath) == B_OK
		&& iconCachePath.Append("HaikuDepot/IconCache") == B_OK
		&& create_directory(iconCachePath.Path(), 0777) == B_OK
		&& iconCachePath.Append(iconName) == B_OK) {
		// Try opening the file in read-only mode, which will fail if its
		// not a file or does not exist.
		fileExists = iconFile.SetTo(iconCachePath.Path(), B_READ_ONLY) == B_OK;
		if (fileExists)
			iconFile.GetModificationTime(&modifiedTime);
	}

	if (fileExists) {
		time_t now;
		time(&now);
		if (fromCacheOnly || now - modifiedTime < 60 * 60) {
			// Cache file is recent enough, just use it and return.
			BitmapRef bitmapRef(new(std::nothrow)SharedBitmap(iconFile), true);
			BAutolock locker(&fLock);
			package->SetIcon(bitmapRef);
			return;
		}
	}

	if (fromCacheOnly)
		return;

	// Retrieve icon from web-app
	WebAppInterface interface;
	BMallocIO buffer;

	status_t status = interface.RetrievePackageIcon(package->Title(), &buffer);
	if (status == B_OK) {
		BitmapRef bitmapRef(new(std::nothrow)SharedBitmap(buffer), true);
		BAutolock locker(&fLock);
		package->SetIcon(bitmapRef);
		locker.Unlock();
		if (iconFile.SetTo(iconCachePath.Path(),
				B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE) == B_OK) {
			iconFile.Write(buffer.Buffer(), buffer.BufferLength());
		}
	}
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
	WebAppInterface interface;
	BMallocIO buffer;

	int32 scaledHeight = scaledWidth * info.Height() / info.Width();

	status_t status = interface.RetrieveScreenshot(info.Code(),
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


bool
Model::_HasNativeIcon(const BMessage& message) const
{
	BMessage pkgIcons;
	if (message.FindMessage("pkgIcons", &pkgIcons) != B_OK)
		return false;

	int32 index = 0;
	while (true) {
		BString name;
		name << index++;
		
		BMessage typeCodeInfo;
		if (pkgIcons.FindMessage(name, &typeCodeInfo) != B_OK)
			break;
	
		BString mediaTypeCode;
		if (typeCodeInfo.FindString("mediaTypeCode", &mediaTypeCode) == B_OK
			&& mediaTypeCode == "application/x-vnd.haiku-icon") {
			return true;
		}
	}
	return false;
}
