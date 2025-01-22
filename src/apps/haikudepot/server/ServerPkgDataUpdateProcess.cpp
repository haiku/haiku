/*
 * Copyright 2017-2025, Andrew Lindesay <apl@lindesay.co.nz>.
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


static std::size_t kPackageBatchSize = 100;

static uint32 kPackageChangeMask = PKG_CHANGED_RATINGS | PKG_CHANGED_SCREENSHOTS
	| PKG_CHANGED_CLASSIFICATION | PKG_CHANGED_LOCALIZED_TEXT | PKG_CHANGED_LOCAL_INFO
	| PKG_CHANGED_CORE_INFO;


/*!	This package listener (not at the JSON level) is feeding in the
	packages as they are parsed and processing them.
*/

class PackageFillingPkgListener : public DumpExportPkgListener {
public:
								PackageFillingPkgListener(Model *model, Stoppable* stoppable);
	virtual						~PackageFillingPkgListener();

	virtual	bool				Handle(DumpExportPkg* item);
	virtual	void				Complete();

			uint32				Count();

private:
	static	ScreenshotInfoRef	_CreateScreenshot(DumpExportPkgScreenshot* screenshot);

			void				_FlushUpdatedPackages();

			const PackageInfoRef
								_PackageForName(const BString& name) const;
			const PackageInfoRef
								_CreateUpdatePackage(const PackageInfoRef& package,
                                	const DumpExportPkg* pkg);

			void				_InitCategories();

private:
			Model*				fModel;
			std::map<BString, CategoryRef>
								fCategories;
			std::vector<PackageInfoRef>
								fUpdatedPackages;
			Stoppable*			fStoppable;
			uint32				fCount;
			bool				fDebugEnabled;
};


PackageFillingPkgListener::PackageFillingPkgListener(Model* model, Stoppable* stoppable)
	:
	fModel(model),
	fStoppable(stoppable),
	fCount(0),
	fDebugEnabled(Logger::IsDebugEnabled())
{
	_InitCategories();
}


PackageFillingPkgListener::~PackageFillingPkgListener()
{
}


void
PackageFillingPkgListener::_InitCategories()
{
	std::vector<CategoryRef> categories = fModel->Categories();
	std::vector<CategoryRef>::const_iterator it;

	for (it = categories.begin(); it != categories.end(); it++) {
		const CategoryRef& category = *it;
		fCategories[category->Code()] = category;
	}

	if (fCategories.empty())
		HDERROR("there are no categories present");
}


const PackageInfoRef
PackageFillingPkgListener::_PackageForName(const BString& name) const
{
	return fModel->PackageForName(name);
}


void
PackageFillingPkgListener::_FlushUpdatedPackages()
{
	fModel->AddPackagesWithChange(fUpdatedPackages, kPackageChangeMask);
	fUpdatedPackages.clear();
}


/*!	This method will produce a new package with the data provided by the
	server.
*/
const PackageInfoRef
PackageFillingPkgListener::_CreateUpdatePackage(const PackageInfoRef& package,
	const DumpExportPkg* pkg)
{
	int32 i;

	PackageClassificationInfoBuilder classificationInfoBuilder(
		package->PackageClassificationInfo());
	PackageLocalizedTextBuilder localizedTextBuilder(package->LocalizedText());
	PackageLocalInfoBuilder localInfoBuilder(package->LocalInfo());
	PackageCoreInfoBuilder coreInfoBuilder(package->CoreInfo());
	PackageScreenshotInfoBuilder screenshotInfoBuilder;
		// don't want to start with the existing data; just take what comes from the server.
	PackageUserRatingInfoBuilder userRatingBuilder;

	localizedTextBuilder.WithHasChangelog(pkg->HasChangelog());

	if (0 != pkg->CountPkgVersions()) {

		// this makes the assumption that the only version will be the
		// latest one.

		DumpExportPkgVersion* pkgVersion = pkg->PkgVersionsItemAt(0);

		if (!pkgVersion->TitleIsNull())
			localizedTextBuilder.WithTitle(*(pkgVersion->Title()));

		if (!pkgVersion->SummaryIsNull())
			localizedTextBuilder.WithSummary(*(pkgVersion->Summary()));

		if (!pkgVersion->DescriptionIsNull())
			localizedTextBuilder.WithDescription(*(pkgVersion->Description()));

		if (!pkgVersion->PayloadLengthIsNull())
			localInfoBuilder.WithSize(static_cast<off_t>(pkgVersion->PayloadLength()));

		if (!pkgVersion->CreateTimestampIsNull()) {
			PackageVersionRef versionRef = PackageUtils::Version(package);

			if (versionRef.IsSet()) {
				PackageVersion* version = versionRef.Get();
				versionRef = PackageVersionRef(
					new PackageVersion(*version, pkgVersion->CreateTimestamp()), true);
			} else {
				versionRef
					= PackageVersionRef(new PackageVersion(pkgVersion->CreateTimestamp()), true);
			}

			coreInfoBuilder.WithVersion(versionRef);
		}
	}

	if (!pkg->DerivedRatingIsNull()) {
		// TODO; unify the naming here!
		userRatingBuilder.WithSummary(UserRatingSummaryBuilder()
				.WithAverageRating(pkg->DerivedRating())
				.WithRatingCount(pkg->DerivedRatingSampleSize())
				.BuildRef());
	}

	int32 countPkgCategories = pkg->CountPkgCategories();

	for (i = 0; i < countPkgCategories; i++) {
		BString* categoryCode = pkg->PkgCategoriesItemAt(i)->Code();
		CategoryRef category = fCategories[*categoryCode];

		if (!category.IsSet())
			HDERROR("unable to find the category for [%s]", categoryCode->String());
		else
			classificationInfoBuilder.AddCategory(category);
	}

	if (!pkg->ProminenceOrderingIsNull())
		classificationInfoBuilder.WithProminence(static_cast<uint32>(pkg->ProminenceOrdering()));

	if (!pkg->IsNativeDesktopIsNull())
		classificationInfoBuilder.WithIsNativeDesktop(pkg->IsNativeDesktop());

	int32 countPkgScreenshots = pkg->CountPkgScreenshots();

	for (i = 0; i < countPkgScreenshots; i++) {
		DumpExportPkgScreenshot* screenshot = pkg->PkgScreenshotsItemAt(i);
		screenshotInfoBuilder.AddScreenshot(_CreateScreenshot(screenshot));
	}

	return PackageInfoBuilder(package)
		.WithScreenshotInfo(screenshotInfoBuilder.BuildRef())
		.WithUserRatingInfo(userRatingBuilder.BuildRef())
		.WithLocalizedText(localizedTextBuilder.BuildRef())
		.WithLocalInfo(localInfoBuilder.BuildRef())
		.WithCoreInfo(coreInfoBuilder.BuildRef())
		.WithPackageClassificationInfo(classificationInfoBuilder.BuildRef())
		.BuildRef();
}


/*static*/ ScreenshotInfoRef
PackageFillingPkgListener::_CreateScreenshot(DumpExportPkgScreenshot* screenshot)
{
	return ScreenshotInfoRef(
		new ScreenshotInfo(*(screenshot->Code()), static_cast<int32>(screenshot->Width()),
			static_cast<int32>(screenshot->Height()), static_cast<int32>(screenshot->Length())),
		true);
}


uint32
PackageFillingPkgListener::Count()
{
	return fCount;
}


bool
PackageFillingPkgListener::Handle(DumpExportPkg* pkg)
{
	const BString packageName = *(pkg->Name());
	PackageInfoRef package = _PackageForName(packageName);

	if (package.IsSet()) {
		fUpdatedPackages.push_back(_CreateUpdatePackage(package, pkg));
		HDTRACE("did populate data for [%s]", pkg->Name()->String());
		fCount++;
	} else {
		HDINFO("[PackageFillingPkgListener] unable to find the pkg [%s]", packageName.String());
	}

	if (fUpdatedPackages.size() > kPackageBatchSize)
		_FlushUpdatedPackages();

	return !fStoppable->WasStopped();
}


void
PackageFillingPkgListener::Complete()
{
	_FlushUpdatedPackages();
}


ServerPkgDataUpdateProcess::ServerPkgDataUpdateProcess(BString depotName, Model* model,
	uint32 serverProcessOptions)
	:
	AbstractSingleFileServerProcess(serverProcessOptions),
	fModel(model),
	fDepotName(depotName)
{
	fName.SetToFormat("ServerPkgDataUpdateProcess<%s>", depotName.String());
	fDescription.SetTo(B_TRANSLATE("Synchronizing package data for repository '%REPO_NAME%'"));
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
		return StorageUtils::DumpExportPkgDataPath(path, webAppRepositorySourceCode,
			fModel->PreferredLanguage());
	}

	return B_ERROR;
}


status_t
ServerPkgDataUpdateProcess::ProcessLocalData()
{
	BStopWatch watch("ServerPkgDataUpdateProcess::ProcessLocalData", true);

	PackageFillingPkgListener* itemListener = new PackageFillingPkgListener(fModel, this);
	ObjectDeleter<PackageFillingPkgListener> itemListenerDeleter(itemListener);

	BulkContainerDumpExportPkgJsonListener* listener
		= new BulkContainerDumpExportPkgJsonListener(itemListener);
	ObjectDeleter<BulkContainerDumpExportPkgJsonListener> listenerDeleter(listener);

	BPath localPath;
	status_t result = GetLocalPath(localPath);

	if (result != B_OK)
		return result;

	result = ParseJsonFromFileWithListener(listener, localPath);

	if (B_OK != result)
		return result;

	if (Logger::IsInfoEnabled()) {
		double secs = watch.ElapsedTime() / 1000000.0;
		HDINFO("[%s] did process %" B_PRIi32 " packages' data in  (%6.3g secs)", Name(),
			itemListener->Count(), secs);
	}

	return listener->ErrorStatus();
}


status_t
ServerPkgDataUpdateProcess::GetStandardMetaDataPath(BPath& path) const
{
	return GetLocalPath(path);
}


void
ServerPkgDataUpdateProcess::GetStandardMetaDataJsonPath(BString& jsonPath) const
{
	jsonPath.SetTo("$.info");
}


BString
ServerPkgDataUpdateProcess::_DeriveWebAppRepositorySourceCode() const
{
	const DepotInfo* depot = fModel->DepotForName(fDepotName);

	if (depot == NULL)
		return BString();

	return depot->WebAppRepositorySourceCode();
}


status_t
ServerPkgDataUpdateProcess::RunInternal()
{
	if (_DeriveWebAppRepositorySourceCode().IsEmpty()) {
		HDINFO("[%s] am not updating data for depot [%s] as there is no web app repository source "
			   "code available",
			Name(), fDepotName.String());
		return B_OK;
	}

	return AbstractSingleFileServerProcess::RunInternal();
}
