/*
 * Copyright 2017-2024, Andrew Lindesay <apl@lindesay.co.nz>.
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
#include <StopWatch.h>
#include <Url.h>

#include "DumpExportPkg.h"
#include "DumpExportPkgCategory.h"
#include "DumpExportPkgJsonListener.h"
#include "DumpExportPkgScreenshot.h"
#include "DumpExportPkgVersion.h"
#include "HaikuDepotConstants.h"
#include "Logger.h"
#include "PackageUtils.h"
#include "ServerSettings.h"
#include "StorageUtils.h"


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

	PackageClassificationInfoRef packageClassificationInfo(new PackageClassificationInfo(), true);

	PackageLocalizedTextRef localizedText = PackageUtils::NewLocalizedText(package);
	PackageLocalInfoRef localInfo = PackageUtils::NewLocalInfo(package);

	localizedText->SetHasChangelog(pkg->HasChangelog());

	if (0 != pkg->CountPkgVersions()) {

			// this makes the assumption that the only version will be the
			// latest one.

		DumpExportPkgVersion* pkgVersion = pkg->PkgVersionsItemAt(0);

		if (!pkgVersion->TitleIsNull())
			localizedText->SetTitle(*(pkgVersion->Title()));

		if (!pkgVersion->SummaryIsNull())
			localizedText->SetSummary(*(pkgVersion->Summary()));

		if (!pkgVersion->DescriptionIsNull())
			localizedText->SetDescription(*(pkgVersion->Description()));

		if (!pkgVersion->PayloadLengthIsNull())
			localInfo->SetSize(static_cast<off_t>(pkgVersion->PayloadLength()));

		if (!pkgVersion->CreateTimestampIsNull())
			package->SetVersionCreateTimestamp(pkgVersion->CreateTimestamp());
	}

	int32 countPkgCategories = pkg->CountPkgCategories();

	for (i = 0; i < countPkgCategories; i++) {
		BString* categoryCode = pkg->PkgCategoriesItemAt(i)->Code();
		CategoryRef category = fModel->CategoryByCode(*categoryCode);

		if (!category.IsSet()) {
			HDERROR("unable to find the category for [%s]",
				categoryCode->String());
		} else
			packageClassificationInfo->AddCategory(category);
	}

	if (!pkg->DerivedRatingIsNull()) {
		UserRatingInfoRef userRatingInfo(new UserRatingInfo(), true);
		UserRatingSummaryRef userRatingSummary(new UserRatingSummary(), true);
		// TODO; unify the naming here!
		userRatingSummary->SetAverageRating(pkg->DerivedRating());
		userRatingSummary->SetRatingCount(pkg->DerivedRatingSampleSize());
		userRatingInfo->SetSummary(userRatingSummary);
		package->SetUserRatingInfo(userRatingInfo);
	}

	if (!pkg->ProminenceOrderingIsNull())
		packageClassificationInfo->SetProminence(static_cast<uint32>(pkg->ProminenceOrdering()));

	if (!pkg->IsNativeDesktopIsNull())
		packageClassificationInfo->SetIsNativeDesktop(pkg->IsNativeDesktop());

	int32 countPkgScreenshots = pkg->CountPkgScreenshots();

	for (i = 0; i < countPkgScreenshots; i++) {
		DumpExportPkgScreenshot* screenshot = pkg->PkgScreenshotsItemAt(i);
		package->AddScreenshotInfo(ScreenshotInfoRef(new ScreenshotInfo(
			*(screenshot->Code()),
			static_cast<int32>(screenshot->Width()),
			static_cast<int32>(screenshot->Height()),
			static_cast<int32>(screenshot->Length())
		), true));
	}

	package->SetLocalizedText(localizedText);
	package->SetLocalInfo(localInfo);

	HDTRACE("did populate data for [%s] (%s)", pkg->Name()->String(),
			fDepotName.String());

	fCount++;

	package->SetPackageClassificationInfo(packageClassificationInfo);

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
	AutoLocker<BLocker> locker(fModel->Lock());
	DepotInfoRef depot = fModel->DepotForName(fDepotName);

	if (depot.Get() != NULL) {
		const BString packageName = *(pkg->Name());
		PackageInfoRef package = depot->PackageByName(packageName);
		if (package.Get() != NULL)
			ConsumePackage(package, pkg);
		else {
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
	BString depotName,
	Model *model,
	uint32 serverProcessOptions)
	:
	AbstractSingleFileServerProcess(serverProcessOptions),
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
	urlPath.SetToFormat("/__pkg/all-%s-%s.json.gz", _DeriveWebAppRepositorySourceCode().String(),
		fModel->PreferredLanguage()->ID());
	return urlPath;
}


status_t
ServerPkgDataUpdateProcess::GetLocalPath(BPath& path) const
{
	BString webAppRepositorySourceCode = _DeriveWebAppRepositorySourceCode();

	if (!webAppRepositorySourceCode.IsEmpty()) {
		AutoLocker<BLocker> locker(fModel->Lock());
		return StorageUtils::DumpExportPkgDataPath(path, webAppRepositorySourceCode,
			fModel->PreferredLanguage());
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
