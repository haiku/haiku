/*
 * Copyright 2017-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "ServerPkgDataUpdateProcess.h"

#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

#include <AutoDeleter.h>
#include <AutoLocker.h>
#include <Catalog.h>
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
#include "HaikuDepotConstants.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ServerPkgDataUpdateProcess"


/*! This package listener (not at the JSON level) is feeding in the
    packages as they are parsed and processing them.
*/

class PackageFillingPkgListener : public DumpExportPkgListener {
public:
								PackageFillingPkgListener(Model *model,
									BString& depotName, Stoppable* stoppable);
	virtual						~PackageFillingPkgListener();

	virtual bool				ConsumePackage(const PackageInfoRef& package,
									DumpExportPkg* pkg);
	virtual	bool				Handle(DumpExportPkg* item);
	virtual	void				Complete();

			uint32				Count();

private:
			int32				IndexOfPackageByName(const BString& name) const;

private:
			BString				fDepotName;
			Model*				fModel;
			std::vector<CategoryRef>
								fCategories;
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
}


PackageFillingPkgListener::~PackageFillingPkgListener()
{
}


bool
PackageFillingPkgListener::ConsumePackage(const PackageInfoRef& package,
	DumpExportPkg* pkg)
{
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
		CategoryRef category = fModel->CategoryByCode(*categoryCode);

		if (category.Get() == NULL) {
			HDERROR("unable to find the category for [%s]",
				categoryCode->String());
		} else
			package->AddCategory(category);
	}

	RatingSummary summary;
	summary.averageRating = RATING_MISSING;

	if (!pkg->DerivedRatingIsNull())
		summary.averageRating = pkg->DerivedRating();

	package->SetRatingSummary(summary);

	package->SetHasChangelog(pkg->HasChangelog());

	if (!pkg->ProminenceOrderingIsNull())
		package->SetProminence(pkg->ProminenceOrdering());

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

	HDDEBUG("did populate data for [%s] (%s)", pkg->Name()->String(),
			fDepotName.String());

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
	const DepotInfo* depotInfo = fModel->DepotForName(fDepotName);

	if (depotInfo != NULL) {
		const BString packageName = *(pkg->Name());
		int32 packageIndex = depotInfo->PackageIndexByName(packageName);

		if (-1 != packageIndex) {
			const PackageList& packages = depotInfo->Packages();
			const PackageInfoRef& packageInfoRef =
				packages.ItemAtFast(packageIndex);

			AutoLocker<BLocker> locker(fModel->Lock());
			ConsumePackage(packageInfoRef, pkg);
		} else {
			HDINFO("[PackageFillingPkgListener] unable to find the pkg [%s]",
				packageName.String());
		}
	} else {
		HDINFO("[PackageFillingPkgListener] unable to find the depot [%s]",
			fDepotName.String());
	}

	return !fStoppable->WasStopped();
}


void
PackageFillingPkgListener::Complete()
{
}


ServerPkgDataUpdateProcess::ServerPkgDataUpdateProcess(
	BString naturalLanguageCode,
	BString depotName,
	Model *model,
	uint32 serverProcessOptions)
	:
	AbstractSingleFileServerProcess(serverProcessOptions),
	fNaturalLanguageCode(naturalLanguageCode),
	fModel(model),
	fDepotName(depotName)
{
	fName.SetToFormat("ServerPkgDataUpdateProcess<%s>", depotName.String());
	fDescription.SetTo(
		B_TRANSLATE("Synchronizing package data for repository "
			"'%REPO_NAME%'"));
	fDescription.ReplaceAll("%REPO_NAME%", depotName.String());
}


ServerPkgDataUpdateProcess::~ServerPkgDataUpdateProcess()
{
}


const char*
ServerPkgDataUpdateProcess::Name() const
{
	return fName.String();
}


const char*
ServerPkgDataUpdateProcess::Description() const
{
	return fDescription.String();
}


BString
ServerPkgDataUpdateProcess::UrlPathComponent()
{
	BString urlPath;
	urlPath.SetToFormat("/__pkg/all-%s-%s.json.gz",
		_DeriveWebAppRepositorySourceCode().String(),
		fNaturalLanguageCode.String());
	return urlPath;
}


status_t
ServerPkgDataUpdateProcess::GetLocalPath(BPath& path) const
{
	BString webAppRepositorySourceCode = _DeriveWebAppRepositorySourceCode();

	if (!webAppRepositorySourceCode.IsEmpty()) {
		AutoLocker<BLocker> locker(fModel->Lock());
		return fModel->DumpExportPkgDataPath(path, webAppRepositorySourceCode);
	}

	return B_ERROR;
}


status_t
ServerPkgDataUpdateProcess::ProcessLocalData()
{
	BStopWatch watch("ServerPkgDataUpdateProcess::ProcessLocalData", true);

	PackageFillingPkgListener* itemListener =
		new PackageFillingPkgListener(fModel, fDepotName, this);
	ObjectDeleter<PackageFillingPkgListener>
		itemListenerDeleter(itemListener);

	BulkContainerDumpExportPkgJsonListener* listener =
		new BulkContainerDumpExportPkgJsonListener(itemListener);
	ObjectDeleter<BulkContainerDumpExportPkgJsonListener>
		listenerDeleter(listener);

	BPath localPath;
	status_t result = GetLocalPath(localPath);

	if (result != B_OK)
		return result;

	result = ParseJsonFromFileWithListener(listener, localPath);

	if (B_OK != result)
		return result;

	if (Logger::IsInfoEnabled()) {
		double secs = watch.ElapsedTime() / 1000000.0;
		HDINFO("[%s] did process %" B_PRIi32 " packages' data "
			"in  (%6.3g secs)", Name(), itemListener->Count(), secs);
	}

	return listener->ErrorStatus();
}


status_t
ServerPkgDataUpdateProcess::GetStandardMetaDataPath(BPath& path) const
{
	return GetLocalPath(path);
}


void
ServerPkgDataUpdateProcess::GetStandardMetaDataJsonPath(
	BString& jsonPath) const
{
	jsonPath.SetTo("$.info");
}


BString
ServerPkgDataUpdateProcess::_DeriveWebAppRepositorySourceCode() const
{
	const DepotInfo* depot = fModel->DepotForName(fDepotName);

	if (depot == NULL) {
		return BString();
	}

	return depot->WebAppRepositorySourceCode();
}


status_t
ServerPkgDataUpdateProcess::RunInternal()
{
	if (_DeriveWebAppRepositorySourceCode().IsEmpty()) {
		HDINFO("[%s] am not updating data for depot [%s] as there is no"
			" web app repository source code available",
			Name(), fDepotName.String());
		return B_OK;
	}

	return AbstractSingleFileServerProcess::RunInternal();
}
