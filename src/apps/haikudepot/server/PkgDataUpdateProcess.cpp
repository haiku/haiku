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
#include <support/StopWatch.h>
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

class PackageFillingPkgListener :
	public DumpExportPkgListener, public PackageConsumer {
public:
								PackageFillingPkgListener(Model *model,
									BString& depotName, Stoppable* stoppable);
	virtual						~PackageFillingPkgListener();

	virtual bool				ConsumePackage(const PackageInfoRef& package,
									void *context);
	virtual	bool				Handle(DumpExportPkg* item);
	virtual	void				Complete();

			uint32				Count();

private:
			int32				IndexOfPackageByName(const BString& name) const;
			int32				IndexOfCategoryByName(
									const BString& name) const;
			int32				IndexOfCategoryByCode(
									const BString& code) const;

			BString				fDepotName;
			Model*				fModel;
			CategoryList		fCategories;
			Stoppable*			fStoppable;
			uint32				fCount;
			bool				fDebugEnabled;
};


PackageFillingPkgListener::PackageFillingPkgListener(Model* model,
	BString& depotName, Stoppable* stoppable)
	:
	fDepotName(depotName),
	fModel(model),
	fStoppable(stoppable),
	fCount(0),
	fDebugEnabled(Logger::IsDebugEnabled())
{
	fCategories = model->Categories();
}


PackageFillingPkgListener::~PackageFillingPkgListener()
{
}


	// TODO; performance could be improved by not needing the linear search

inline int32
PackageFillingPkgListener::IndexOfCategoryByName(
	const BString& name) const
{
	int32 i;
	int32 categoryCount = fCategories.CountItems();

	for (i = 0; i < categoryCount; i++) {
		const CategoryRef categoryRef = fCategories.ItemAtFast(i);

		if (categoryRef->Name() == name)
			return i;
	}

	return -1;
}


bool
PackageFillingPkgListener::ConsumePackage(const PackageInfoRef& package,
	void *context)
{
	DumpExportPkg* pkg = static_cast<DumpExportPkg*>(context);
	int32 i;

		// Collects all of the changes here into one set of notifications to
		// the package's listeners.  This way the quantity of BMessages
		// communicated back to listeners is considerably reduced.  See stop
		// invocation later in this method.

	package->StartCollatingChanges();

	if (0 != pkg->CountPkgVersions()) {

			// this makes the assumption that the only version will be the
			// latest one.

		DumpExportPkgVersion* pkgVersion = pkg->PkgVersionsItemAt(0);

		if (!pkgVersion->TitleIsNull())
			package->SetTitle(*(pkgVersion->Title()));

		if (!pkgVersion->SummaryIsNull())
			package->SetShortDescription(*(pkgVersion->Summary()));

		if (!pkgVersion->DescriptionIsNull())
			package->SetFullDescription(*(pkgVersion->Description()));

		if (!pkgVersion->PayloadLengthIsNull())
			package->SetSize(pkgVersion->PayloadLength());
	}

	int32 countPkgCategories = pkg->CountPkgCategories();

	for (i = 0; i < countPkgCategories; i++) {
		BString* categoryCode = pkg->PkgCategoriesItemAt(i)->Code();
		int categoryIndex = IndexOfCategoryByName(*(categoryCode));

		if (categoryIndex == -1) {
			printf("unable to find the category for [%s]\n",
				categoryCode->String());
		} else {
			package->AddCategory(
				fCategories.ItemAtFast(categoryIndex));
		}
	}

	if (!pkg->DerivedRatingIsNull()) {
		RatingSummary summary;
		summary.averageRating = pkg->DerivedRating();
		package->SetRatingSummary(summary);
	}

	if (!pkg->ProminenceOrderingIsNull())
		package->SetProminence(pkg->ProminenceOrdering());

	if (!pkg->PkgChangelogContentIsNull())
		package->SetChangelog(*(pkg->PkgChangelogContent()));

	int32 countPkgScreenshots = pkg->CountPkgScreenshots();

	for (i = 0; i < countPkgScreenshots; i++) {
		DumpExportPkgScreenshot* screenshot = pkg->PkgScreenshotsItemAt(i);
		package->AddScreenshotInfo(ScreenshotInfo(
			*(screenshot->Code()),
			static_cast<int32>(screenshot->Width()),
			static_cast<int32>(screenshot->Height()),
			static_cast<int32>(screenshot->Length())
		));
	}

	if (fDebugEnabled) {
		printf("did populate data for [%s] (%s)\n", pkg->Name()->String(),
			fDepotName.String());
	}

	fCount++;

	package->EndCollatingChanges();

	return !fStoppable->WasStopped();
}


uint32
PackageFillingPkgListener::Count()
{
	return fCount;
}


bool
PackageFillingPkgListener::Handle(DumpExportPkg* pkg)
{
	fModel->ForPackageByNameInDepot(fDepotName, *(pkg->Name()), this, pkg);
	return !fStoppable->WasStopped();
}


void
PackageFillingPkgListener::Complete()
{
}


PkgDataUpdateProcess::PkgDataUpdateProcess(
	AbstractServerProcessListener* listener,
	const BPath& localFilePath,
	BString naturalLanguageCode,
	BString repositorySourceCode,
	BString depotName,
	Model *model,
	uint32 options)
	:
	AbstractSingleFileServerProcess(listener, options),
	fLocalFilePath(localFilePath),
	fNaturalLanguageCode(naturalLanguageCode),
	fRepositorySourceCode(repositorySourceCode),
	fModel(model),
	fDepotName(depotName)
{
	fName.SetToFormat("PkgDataUpdateProcess<%s>", depotName.String());
}


PkgDataUpdateProcess::~PkgDataUpdateProcess()
{
}


const char*
PkgDataUpdateProcess::Name()
{
	return fName.String();
}


BString
PkgDataUpdateProcess::UrlPathComponent()
{
	BString urlPath;
	urlPath.SetToFormat("/__pkg/all-%s-%s.json.gz",
		fRepositorySourceCode.String(),
		fNaturalLanguageCode.String());
	return urlPath;
}


BPath&
PkgDataUpdateProcess::LocalPath()
{
	return fLocalFilePath;
}


status_t
PkgDataUpdateProcess::ProcessLocalData()
{
	BStopWatch watch("PkgDataUpdateProcess::ProcessLocalData", true);

	PackageFillingPkgListener* itemListener =
		new PackageFillingPkgListener(fModel, fDepotName, this);

	BulkContainerDumpExportPkgJsonListener* listener =
		new BulkContainerDumpExportPkgJsonListener(itemListener);

	status_t result = ParseJsonFromFileWithListener(listener, fLocalFilePath);

	if (Logger::IsInfoEnabled()) {
		double secs = watch.ElapsedTime() / 1000000.0;
		fprintf(stdout, "[%s] did process %" B_PRIi32 " packages' data "
			"in  (%6.3g secs)\n", Name(), itemListener->Count(), secs);
	}

	if (B_OK != result)
		return result;

	return listener->ErrorStatus();
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
