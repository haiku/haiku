/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "Model.h"

#include <stdio.h>

#include <Catalog.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Model"


// #pragma mark - PackageFilters


PackageFilter::~PackageFilter()
{
}


class AnyFilter : public PackageFilter {
public:
	virtual bool AcceptsPackage(const PackageInfo& package) const
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
	
	virtual bool AcceptsPackage(const PackageInfo& package) const
	{
		// TODO: Maybe a PackageInfo ought to know the Depot it came from?
		// But right now the same package could theoretically be provided
		// from different depots and the filter would work correctly.
		// Also the PackageList could actually contain references to packages
		// instead of the packages as objects. The equal operator is quite
		// expensive as is.
		const PackageInfoList& packageList = fDepot.PackageList();
		for (int i = packageList.CountItems() - 1; i >= 0; i--) {
			if (packageList.ItemAtFast(i) == package)
				return true;
		}
		return false;
	}
	
private:
	DepotInfo	fDepot;
};


// #pragma mark - Model


Model::Model()
	:
	fSearchTerms(),
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
	fDepotFilter(PackageFilterRef(new AnyFilter(), true)),
	fSearchTermsFilter(PackageFilterRef(new AnyFilter(), true))
{
	// Don't forget to add new categories to this list:
	fCategories.Add(fCategoryAudio);
	fCategories.Add(fCategoryVideo);
	fCategories.Add(fCategoryGraphics);
	fCategories.Add(fCategoryProductivity);
	fCategories.Add(fCategoryDevelopment);
	fCategories.Add(fCategoryCommandLine);
	fCategories.Add(fCategoryGames);
}


PackageInfoList
Model::CreatePackageList() const
{
	// TODO: Allow to restrict depot, filter by search terms, ...

	// Return all packages from all depots.
	PackageInfoList resultList;

	for (int32 i = 0; i < fDepots.CountItems(); i++) {
		const PackageInfoList& packageList
			= fDepots.ItemAtFast(i).PackageList();

		for (int32 j = 0; j < packageList.CountItems(); j++) {
			const PackageInfo& package = packageList.ItemAtFast(j);
			if (fCategoryFilter->AcceptsPackage(package)
				&& fDepotFilter->AcceptsPackage(package)
				&& fSearchTermsFilter->AcceptsPackage(package)) {
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

