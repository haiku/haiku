/*
 * Copyright 2013-2025, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 * 		Stephan Aßmus <superstippi@gmx.de>
 * 		Rene Gollent <rene@gollent.com>
 *		Julian Harnath <julian.harnath@rwth-aachen.de>
 *		Andrew Lindesay <apl@lindesay.co.nz>
 *
 * Note that this file has been re-factored from `PackageManager.cpp` and
 * authors have been carried across in 2021.
 */


#include "InstallPackageProcess.h"

#include <algorithm>

#include <AutoDeleter.h>
#include <AutoLocker.h>
#include <Catalog.h>
#include <Locker.h>

#include <package/hpkg/NoErrorOutput.h>
#include <package/hpkg/PackageContentHandler.h>
#include <package/hpkg/PackageEntry.h>
#include <package/hpkg/PackageEntryAttribute.h>
#include <package/hpkg/PackageReader.h>
#include <package/manager/Exceptions.h>
#include <package/solver/SolverPackage.h>

#include "AppUtils.h"
#include "HaikuDepotConstants.h"
#include "Logger.h"
#include "PackageManager.h"
#include "PackageUtils.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InstallPackageProcess"

using namespace BPackageKit;
using namespace BPackageKit::BPrivate;
using namespace BPackageKit::BManager::BPrivate;

using BPackageKit::BSolver;
using BPackageKit::BSolverPackage;
using BPackageKit::BSolverRepository;
using BPackageKit::BHPKG::BNoErrorOutput;
using BPackageKit::BHPKG::BPackageContentHandler;
using BPackageKit::BHPKG::BPackageEntry;
using BPackageKit::BHPKG::BPackageEntryAttribute;
using BPackageKit::BHPKG::BPackageInfoAttributeValue;
using BPackageKit::BHPKG::BPackageReader;


class DownloadProgress {
public:
	DownloadProgress()
		:
		fPackageName(),
		fProgress(0)
	{
	}

	DownloadProgress(BString packageName, float progress)
		:
		fPackageName(packageName),
		fProgress(progress)
	{
	}

	virtual ~DownloadProgress()
	{
	}

	BString PackageName() const
	{
		return fPackageName;
	}

	float Progress() const
	{
		return fProgress;
	}

private:
	BString	fPackageName;
	float	fProgress;
};


InstallPackageProcess::InstallPackageProcess(const BString& packageName, Model* model)
	:
	AbstractPackageProcess(packageName, model),
	fLastDownloadUpdate(0)
{
	fDescription = B_TRANSLATE("Installing \"%PackageName%\"");
	fDescription.ReplaceAll("%PackageName%", packageName);
}


InstallPackageProcess::~InstallPackageProcess()
{
}


const char*
InstallPackageProcess::Name() const
{
	return "InstallPackageProcess";
}


const char*
InstallPackageProcess::Description() const
{
	return fDescription;
}


float
InstallPackageProcess::Progress()
{
	if (ProcessState() == PROCESS_RUNNING && !fDownloadProgresses.empty()) {
		AutoLocker<BLocker> locker(&fLock);
		float sum = 0.0;
		int32 count = 0;

		std::map<BString, DownloadProgress>::const_iterator it;

		for (it = fDownloadProgresses.begin(); it != fDownloadProgresses.end(); it++) {
			DownloadProgress downloadProgress = it->second;

			if (!downloadProgress.PackageName().IsEmpty()) {
				sum += downloadProgress.Progress();
				count++;
			}
		}
		if (sum > 0.0)
			return sum / static_cast<float>(count);
	}
	return kProgressIndeterminate;
}


status_t
InstallPackageProcess::RunInternal()
{
	// TODO: allow configuring the installation location

	PackageManager* packageManager = new(std::nothrow)
		PackageManager(static_cast<BPackageInstallationLocation>(InstallLocation()));
	ObjectDeleter<PackageManager> solverDeleter(packageManager);

	SetPackageState(fPackageName, PENDING);

	packageManager->Init(BPackageManager::B_ADD_INSTALLED_REPOSITORIES
		| BPackageManager::B_ADD_REMOTE_REPOSITORIES | BPackageManager::B_REFRESH_REPOSITORIES);


	PackageInfoRef package = FindPackageByName(fPackageName);
	PackageState state = PackageUtils::State(package);

	packageManager->SetCurrentActionPackage(package, true);
	packageManager->AddProgressListener(this);

	BString packageName = fPackageName;
	PackageLocalInfoRef localInfo = package->LocalInfo();

	if (localInfo.IsSet() && localInfo->IsLocalFile())
		packageName = localInfo->LocalFilePath();

	const char* packageNameString = packageName.String();

	try {
		packageManager->Install(&packageNameString, 1);
	} catch (BFatalErrorException& ex) {
		BString errorString;
		errorString.SetToFormat("Fatal error occurred while installing package %s: %s (%s)\n",
			packageNameString, ex.Message().String(), ex.Details().String());
		AppUtils::NotifySimpleError(B_TRANSLATE("Fatal error"), errorString, B_STOP_ALERT);
		_SetDownloadedPackagesState(NONE);
		SetPackageState(fPackageName, state);
		return ex.Error();
	} catch (BAbortedByUserException& ex) {
		HDINFO("Installation of package %s is aborted by user: %s", packageNameString,
			ex.Message().String());
		_SetDownloadedPackagesState(NONE);
		SetPackageState(fPackageName, state);
		return B_OK;
	} catch (BNothingToDoException& ex) {
		HDINFO("Nothing to do while installing package %s: %s", packageNameString,
			ex.Message().String());
		return B_OK;
	} catch (BException& ex) {
		HDERROR("Exception occurred while installing package %s: %s", packageNameString,
			ex.Message().String());
		_SetDownloadedPackagesState(NONE);
		SetPackageState(fPackageName, state);
		return B_ERROR;
	}

	packageManager->RemoveProgressListener(this);

	_SetDownloadedPackagesState(ACTIVATED);

	return B_OK;
}


// #pragma mark - DownloadProgressListener


void
InstallPackageProcess::DownloadProgressChanged(const char* packageName, float progress)
{
	if (!_ShouldProcessProgress() && progress != 1.0)
		return;

	BString simplePackageName;

	if (_DeriveSimplePackageName(packageName, simplePackageName) != B_OK) {
		HDERROR("malformed canonical package name [%s]", packageName);
		return;
	}

	SetPackageDownloadProgress(simplePackageName, progress);
	_SetDownloadProgress(simplePackageName, progress);
}


void
InstallPackageProcess::DownloadProgressComplete(const char* packageName)
{
	BString simplePackageName;
	if (_DeriveSimplePackageName(packageName, simplePackageName) != B_OK) {
		HDERROR("malformed canonical package name [%s]", packageName);
		return;
	}
	_SetDownloadProgress(simplePackageName, 1.0);
	SetPackageDownloadProgress(simplePackageName, 1.0);
	fDownloadedPackageNames.insert(simplePackageName);
}


void
InstallPackageProcess::ConfirmedChanges(BPackageManager::InstalledRepository& repository)
{
	BPackageManager::InstalledRepository::PackageList& activationList
		= repository.PackagesToActivate();

	BSolverPackage* package = NULL;
	for (int32 i = 0; (package = activationList.ItemAt(i)); i++) {
		BString packageName = package->Info().Name();
		SetPackageState(packageName, PENDING);
	}
}


void
InstallPackageProcess::_SetDownloadedPackagesState(PackageState state)
{
	std::set<BString>::const_iterator it;
	for (it = fDownloadedPackageNames.begin(); it != fDownloadedPackageNames.end(); ++it) {
		BString packageName = *it;
		SetPackageState(packageName, state);
	}
}


/*!	This method will extract the plain package name from the canonical
 */

/*static*/ status_t
InstallPackageProcess::_DeriveSimplePackageName(const BString& canonicalForm,
	BString& simplePackageName)
{
	int32 hypenIndex = canonicalForm.FindFirst('-');
	if (hypenIndex <= 0)
		return B_BAD_DATA;
	simplePackageName.SetTo(canonicalForm);
	simplePackageName.Truncate(hypenIndex);
	return B_OK;
}


void
InstallPackageProcess::_SetDownloadProgress(const BString& simplePackageName, float progress)
{
	DownloadProgress downloadProgress(simplePackageName, progress);
	fDownloadProgresses[simplePackageName] = downloadProgress;
	_NotifyChanged();
}
