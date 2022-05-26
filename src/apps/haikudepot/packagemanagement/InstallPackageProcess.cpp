/*
 * Copyright 2013-2021, Haiku, Inc. All Rights Reserved.
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

#include <AutoLocker.h>
#include <Catalog.h>
#include <Locker.h>

#include <package/manager/Exceptions.h>
#include <package/hpkg/NoErrorOutput.h>
#include <package/hpkg/PackageContentHandler.h>
#include <package/hpkg/PackageEntry.h>
#include <package/hpkg/PackageEntryAttribute.h>
#include <package/hpkg/PackageReader.h>
#include <package/solver/SolverPackage.h>

#include "AppUtils.h"
#include "Logger.h"
#include "PackageManager.h"


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


InstallPackageProcess::InstallPackageProcess(
	PackageInfoRef package, Model* model)
	:
	AbstractPackageProcess(package, model),
	fLastDownloadUpdate(0)
{
	fDescription = B_TRANSLATE("Installing \"%PackageName%\"");
	fDescription.ReplaceAll("%PackageName%", package->Name());
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
		std::vector<DownloadProgress>::const_iterator it;
		for (it = fDownloadProgresses.begin();
				it != fDownloadProgresses.end(); it++) {
			DownloadProgress downloadProgress = *it;
			sum += downloadProgress.Progress();
		}
		if (sum > 0.0)
			return sum / (float) fDownloadProgresses.size();
	}
	return kProgressIndeterminate;
}


status_t
InstallPackageProcess::RunInternal()
{
	fPackageManager->Init(BPackageManager::B_ADD_INSTALLED_REPOSITORIES
		| BPackageManager::B_ADD_REMOTE_REPOSITORIES
		| BPackageManager::B_REFRESH_REPOSITORIES);
	PackageInfoRef ref(fPackage);
	PackageState state = ref->State();
	ref->SetState(PENDING);

	fPackageManager->SetCurrentActionPackage(ref, true);
	fPackageManager->AddProgressListener(this);

	BString packageName;
	if (ref->IsLocalFile())
		packageName = ref->LocalFilePath();
	else
		packageName = ref->Name();

	const char* packageNameString = packageName.String();
	try {
		fPackageManager->Install(&packageNameString, 1);
	} catch (BFatalErrorException& ex) {
		BString errorString;
		errorString.SetToFormat(
			"Fatal error occurred while installing package %s: "
			"%s (%s)\n", packageNameString, ex.Message().String(),
			ex.Details().String());
		AppUtils::NotifySimpleError(B_TRANSLATE("Fatal error"), errorString,
			B_STOP_ALERT);
		_SetDownloadedPackagesState(NONE);
		ref->SetState(state);
		return ex.Error();
	} catch (BAbortedByUserException& ex) {
		HDINFO("Installation of package %s is aborted by user: %s",
			packageNameString, ex.Message().String());
		_SetDownloadedPackagesState(NONE);
		ref->SetState(state);
		return B_OK;
	} catch (BNothingToDoException& ex) {
		HDINFO("Nothing to do while installing package %s: %s",
			packageNameString, ex.Message().String());
		return B_OK;
	} catch (BException& ex) {
		HDERROR("Exception occurred while installing package %s: %s",
			packageNameString, ex.Message().String());
		_SetDownloadedPackagesState(NONE);
		ref->SetState(state);
		return B_ERROR;
	}

	fPackageManager->RemoveProgressListener(this);

	_SetDownloadedPackagesState(ACTIVATED);

	return B_OK;
}


// #pragma mark - DownloadProgressListener


void
InstallPackageProcess::DownloadProgressChanged(
	const char* packageName, float progress)
{
	bigtime_t now = system_time();
	if (now - fLastDownloadUpdate < 250000 && progress != 1.0)
		return;
	fLastDownloadUpdate = now;
	BString simplePackageName;
	if (_DeriveSimplePackageName(packageName, simplePackageName) != B_OK) {
		HDERROR("malformed canonical package name [%s]", packageName);
		return;
	}
	PackageInfoRef ref(FindPackageByName(simplePackageName));
	if (ref.IsSet()) {
		ref->SetDownloadProgress(progress);
		_SetDownloadProgress(simplePackageName, progress);
	} else {
		HDERROR("unable to find the package info for simple package name [%s]",
			simplePackageName.String());
	}
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
	PackageInfoRef ref(FindPackageByName(simplePackageName));
	if (!ref.IsSet()) {
		HDERROR("unable to find the package info for simple package name [%s]",
			simplePackageName.String());
		return;
	}
	ref->SetDownloadProgress(1.0);
	fDownloadedPackages.insert(ref);
}


void
InstallPackageProcess::ConfirmedChanges(
	BPackageManager::InstalledRepository& repository)
{
	BPackageManager::InstalledRepository::PackageList& activationList =
		repository.PackagesToActivate();

	BSolverPackage* package = NULL;
	for (int32 i = 0; (package = activationList.ItemAt(i)); i++) {
		PackageInfoRef ref(FindPackageByName(package->Info().Name()));
		if (ref.IsSet())
			ref->SetState(PENDING);
	}
}


void
InstallPackageProcess::_SetDownloadedPackagesState(PackageState state)
{
	for (PackageInfoSet::iterator it = fDownloadedPackages.begin();
			it != fDownloadedPackages.end(); ++it) {
		(*it)->SetState(state);
	}
}


static bool
_IsDownloadProgressBefore(const DownloadProgress& dp1,
	const DownloadProgress& dp2)
{
	return dp1.PackageName().Compare(dp2.PackageName()) < 0;
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
InstallPackageProcess::_SetDownloadProgress(const BString& simplePackageName,
	float progress)
{
	AutoLocker<BLocker> locker(&fLock);
	DownloadProgress downloadProgress(simplePackageName, progress);
	std::vector<DownloadProgress>::iterator itInsertionPt
		= std::lower_bound(
			fDownloadProgresses.begin(), fDownloadProgresses.end(),
			downloadProgress, &_IsDownloadProgressBefore);

	if (itInsertionPt != fDownloadProgresses.end()) {
		if ((*itInsertionPt).PackageName() == simplePackageName) {
			itInsertionPt = fDownloadProgresses.erase(itInsertionPt);
		}
	}

	fDownloadProgresses.insert(itInsertionPt, downloadProgress);
	_NotifyChanged();
}
