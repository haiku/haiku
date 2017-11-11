/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "PkgDataUpdateProcess.h"

#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

#include <Autolock.h>
#include <FileIO.h>
#include <Url.h>

#include "Logger.h"
#include "ServerSettings.h"
#include "StorageUtils.h"
#include "DumpExportPkg.h"
#include "DumpExportPkgCategory.h"
#include "DumpExportPkgJsonListener.h"
#include "DumpExportPkgScreenshot.h"
#include "DumpExportPkgVersion.h"


/*! This package listener (not at the JSON level) is feeding in the
    packages as they are parsed and processing them.
*/

class PackageFillingPkgListener : public DumpExportPkgListener {
public:
								PackageFillingPkgListener(
									const PackageList& packages,
									const CategoryList& categories,
									BLocker& lock);
	virtual						~PackageFillingPkgListener();

	virtual	void				Handle(DumpExportPkg* item);
	virtual	void				Complete();

private:
			int32				IndexOfPackageByName(const BString& name) const;
			int32				IndexOfCategoryByName(
									const BString& name) const;
			int32				IndexOfCategoryByCode(
									const BString& code) const;
	const	PackageList&		fPackages;
	const	CategoryList&		fCategories;
			BLocker&			fLock;


};


PackageFillingPkgListener::PackageFillingPkgListener(
	const PackageList& packages, const CategoryList& categories,
	BLocker& lock)
	:
	fPackages(packages),
	fCategories(categories),
	fLock(lock)
{
}


PackageFillingPkgListener::~PackageFillingPkgListener()
{
}


	// TODO; performance could be improved by not needing the linear search

int32
PackageFillingPkgListener::IndexOfPackageByName(
	const BString& name) const
{
	int32 i;

	for (i = 0; i < fPackages.CountItems(); i++) {
		const PackageInfoRef& packageInfo = fPackages.ItemAt(i);

		if (packageInfo->Name() == name)
			return i;
	}

	return -1;
}


	// TODO; performance could be improved by not needing the linear search

int32
PackageFillingPkgListener::IndexOfCategoryByName(
	const BString& name) const
{
	int32 i;

	for (i = 0; i < fCategories.CountItems(); i++) {
		const CategoryRef categoryRef = fCategories.ItemAtFast(i);

		if (categoryRef->Name() == name)
			return i;
	}

	return -1;
}


void
PackageFillingPkgListener::Handle(DumpExportPkg* pkg)
{
	BAutolock locker(fLock); // lock from the model.
	int32 packageIndex = IndexOfPackageByName(*(pkg->Name()));

	if (packageIndex == -1) {
		printf("unable to find package data for pkg name [%s]\n",
			pkg->Name()->String());
	} else {
		int32 i;
		const PackageInfoRef& packageInfo = fPackages.ItemAt(packageIndex);

		if (0 != pkg->CountPkgVersions()) {

				// this makes the assumption that the only version will be the
				// latest one.

			DumpExportPkgVersion* pkgVersion = pkg->PkgVersionsItemAt(0);

			if (!pkgVersion->TitleIsNull())
				packageInfo->SetTitle(*(pkgVersion->Title()));

			if (!pkgVersion->SummaryIsNull())
				packageInfo->SetShortDescription(*(pkgVersion->Summary()));

			if (!pkgVersion->DescriptionIsNull()) {
				packageInfo->SetFullDescription(*(pkgVersion->Description()));
			}

			if (!pkgVersion->PayloadLengthIsNull())
				packageInfo->SetSize(pkgVersion->PayloadLength());
		}

		for (i = 0; i < pkg->CountPkgCategories(); i++) {
			BString* categoryCode = pkg->PkgCategoriesItemAt(i)->Code();
			int categoryIndex = IndexOfCategoryByName(*(categoryCode));

			if (categoryIndex == -1) {
				printf("unable to find the category for [%s]\n",
					categoryCode->String());
			} else {
				packageInfo->AddCategory(fCategories.ItemAtFast(i));
			}
		}

		if (!pkg->DerivedRatingIsNull()) {
			RatingSummary summary;
			summary.averageRating = pkg->DerivedRating();
			packageInfo->SetRatingSummary(summary);
		}

		if (!pkg->ProminenceOrderingIsNull()) {
			packageInfo->SetProminence(pkg->ProminenceOrdering());
		}

		if (!pkg->PkgChangelogContentIsNull()) {
			packageInfo->SetChangelog(*(pkg->PkgChangelogContent()));
		}

		for (i = 0; i < pkg->CountPkgScreenshots(); i++) {
			DumpExportPkgScreenshot* screenshot = pkg->PkgScreenshotsItemAt(i);
			packageInfo->AddScreenshotInfo(ScreenshotInfo(
				*(screenshot->Code()),
				static_cast<int32>(screenshot->Width()),
				static_cast<int32>(screenshot->Height()),
				static_cast<int32>(screenshot->Length())
			));
		}

		if (Logger::IsDebugEnabled()) {
			printf("did populate data for [%s]\n", pkg->Name()->String());
		}
	}
}


void
PackageFillingPkgListener::Complete()
{
}


PkgDataUpdateProcess::PkgDataUpdateProcess(
	const BPath& localFilePath,
	BLocker& lock,
	BString repositorySourceCode,
	BString naturalLanguageCode,
	const PackageList& packages,
	const CategoryList& categories)
	:
	fLocalFilePath(localFilePath),
	fNaturalLanguageCode(naturalLanguageCode),
	fRepositorySourceCode(repositorySourceCode),
	fPackages(packages),
	fCategories(categories),
	fLock(lock)
{
}


PkgDataUpdateProcess::~PkgDataUpdateProcess()
{
}


status_t
PkgDataUpdateProcess::Run()
{
	printf("will fetch packages' data\n");

	BString urlPath;
	urlPath.SetToFormat("/__pkg/all-%s-%s.json.gz",
		fRepositorySourceCode.String(),
		fNaturalLanguageCode.String());

	status_t result = DownloadToLocalFile(fLocalFilePath,
		ServerSettings::CreateFullUrl(urlPath),
		0, 0);

	if (result == B_OK || result == APP_ERR_NOT_MODIFIED) {
		printf("did fetch packages' data\n");

			// now load the data in and process it.

		printf("will process packages' data\n");
		result = PopulateDataToDepots();
		printf("did process packages' data\n");
	}

	return result;
}


status_t
PkgDataUpdateProcess::PopulateDataToDepots()
{
	PackageFillingPkgListener* itemListener =
		new PackageFillingPkgListener(fPackages, fCategories, fLock);

	BJsonEventListener* listener =
		new BulkContainerDumpExportPkgJsonListener(itemListener);

	return ParseJsonFromFileWithListener(listener, fLocalFilePath);
}


void
PkgDataUpdateProcess::GetStandardMetaDataPath(BPath& path) const
{
	path.SetTo(fLocalFilePath.Path());
}


void
PkgDataUpdateProcess::GetStandardMetaDataJsonPath(
	BString& jsonPath) const
{
	jsonPath.SetTo("$.info");
}


const char*
PkgDataUpdateProcess::LoggingName() const
{
	return "pkg-data-update";
}