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
	fCategoryVideo(new PackageCategory(
		BitmapRef(),
		B_TRANSLATE("Video"), "video"), true),
	fCategoryGraphics(new PackageCategory(
		BitmapRef(),
		B_TRANSLATE("Graphics"), "graphics"), true),
	fCategoryProductivity(new PackageCategory(
		BitmapRef(),
		B_TRANSLATE("Productivity"), "productivity"), true),
	fCategoryDevelopment(new PackageCategory(
		BitmapRef(),
		B_TRANSLATE("Development"), "development"), true),
	fCategoryCommandLine(new PackageCategory(
		BitmapRef(),
		B_TRANSLATE("Command line"), "command-line"), true),
	fCategoryGames(new PackageCategory(
		BitmapRef(),
		B_TRANSLATE("Games"), "games"), true),

	fCategoryFilter(PackageFilterRef(new AnyFilter(), true)),
	fDepotFilter(""),
	fSearchTermsFilter(PackageFilterRef(new AnyFilter(), true)),

	fShowSourcePackages(false),
	fShowDevelopPackages(false),

	fPopulateAllPackagesThread(-1),
	fStopPopulatingAllPackages(false)
{
	// Don't forget to add new categories to this list:
	fCategories.Add(fCategoryAudio);
	fCategories.Add(fCategoryVideo);
	fCategories.Add(fCategoryGraphics);
	fCategories.Add(fCategoryProductivity);
	fCategories.Add(fCategoryDevelopment);
	fCategories.Add(fCategoryCommandLine);
	fCategories.Add(fCategoryGames);

	// A category for packages that the user installed.
	fUserCategories.Add(CategoryRef(new PackageCategory(
		BitmapRef(),
		B_TRANSLATE("Installed packages"), "installed"), true));

	// A category for packages that not yet installed.
	fUserCategories.Add(CategoryRef(new PackageCategory(
		BitmapRef(),
		B_TRANSLATE("Available packages"), "available"), true));

	// A category for packages that the user specifically uninstalled.
	// For example, a user may have removed packages from a default
	// Haiku installation
	fUserCategories.Add(CategoryRef(new PackageCategory(
		BitmapRef(),
		B_TRANSLATE("Uninstalled packages"), "uninstalled"), true));

	// A category for all packages that the user has installed or uninstalled.
	// Those packages resemble what makes their system different from a
	// fresh Haiku installation.
	fUserCategories.Add(CategoryRef(new PackageCategory(
		BitmapRef(),
		B_TRANSLATE("User modified packages"), "modified"), true));

	// Two categories to see just the packages which are downloading or
	// have updates available
	fProgressCategories.Add(CategoryRef(new PackageCategory(
		BitmapRef(),
		B_TRANSLATE("Downloading"), "downloading"), true));
	fProgressCategories.Add(CategoryRef(new PackageCategory(
		BitmapRef(),
		B_TRANSLATE("Update available"), "updates"), true));
}


Model::~Model()
{
	StopPopulatingAllPackages();
}


PackageList
Model::CreatePackageList() const
{
	// TODO: Allow to restrict depot, filter by search terms, ...

	// Return all packages from all depots.
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
	else if (category == "installed")
		filter = new ContainedInFilter(fInstalledPackages);
	else if (category == "uninstalled")
		filter = new ContainedInFilter(fUninstalledPackages);
	else if (category == "available") {
		filter = new StateFilter(NONE);
//		filter = new NotContainedInFilter(&fInstalledPackages,
//			&fUninstalledPackages, &fDownloadingPackages, &fUpdateablePackages,
//			NULL);
	} else if (category == "modified") {
		filter = new ContainedInEitherFilter(fInstalledPackages,
			fUninstalledPackages);
	} else if (category == "downloading")
		filter = new ContainedInFilter(fDownloadingPackages);
	else if (category == "updates")
		filter = new ContainedInFilter(fUpdateablePackages);
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
	if (fPopulatedPackages.Contains(package))
		return;

	BAutolock _(&fLock);

	// TODO: Replace with actual backend that retrieves package extra
	// information and user-contributed package information.

	// TODO: There should probably also be a way to "unpopulate" the
	// package information. Maybe a cache of populated packages, so that
	// packages loose their extra information after a certain amount of
	// time when they have not been accessed/displayed in the UI. Otherwise
	// HaikuDepot will consume more and more resources in the packages.
	// Especially screen-shots will be a problem eventually.

	// TODO: Simulate a delay in retrieving this info, and do that on
	// a separate thread.

	fPopulatedPackages.Add(package);

	if (package->Title() == "wonderbrush") {

		if ((flags & POPULATE_CATEGORIES) != 0) {
			package->AddCategory(CategoryGraphics());
			package->AddCategory(CategoryProductivity());
		}

		if ((flags & POPULATE_USER_RATINGS) != 0) {
			package->AddUserRating(
				UserRating(UserInfo("binky"), 4.5f,
				"Awesome!", "en", "2.1.2", 0, 0)
			);
			package->AddUserRating(
				UserRating(UserInfo("twinky"), 5.0f,
				"The best!", "en", "2.1.2", 3, 1)
			);
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
	model->_PopulateAllPackagesThread(true);
	model->_PopulateAllPackagesThread(false);
	return 0;
}


void
Model::_PopulateAllPackagesThread(bool fromCacheOnly)
{
	int32 depotIndex = 0;
	int32 packageIndex = 0;

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
	
//		_PopulatePackageInfo(package, fromCacheOnly);
		_PopulatePackageIcon(package, fromCacheOnly);
		// TODO: Average user rating. It needs to be shown in the
		// list view, so without the user clicking the package.
	}
}


void
Model::_PopulatePackageInfo(const PackageInfoRef& package, bool fromCacheOnly)
{
	if (fromCacheOnly)
		return;
	
	// Retrieve info from web-app
	WebAppInterface interface;
	BMessage info;

	status_t status = interface.RetrievePackageInfo(package->Title(), info);
	if (status == B_OK) {
		// TODO: Parse message...
		info.PrintToStream();
	}
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
		package->SetIcon(bitmapRef);
		if (iconFile.SetTo(iconCachePath.Path(),
				B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE) == B_OK) {
			iconFile.Write(buffer.Buffer(), buffer.BufferLength());
		}
	}
}
