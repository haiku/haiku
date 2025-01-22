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


#include "UninstallPackageProcess.h"

#include <package/manager/Exceptions.h>
#include <package/solver/SolverPackage.h>

#include <AutoDeleter.h>
#include <Catalog.h>

#include "Alert.h"
#include "AppUtils.h"
#include "Logger.h"
#include "PackageManager.h"
#include "PackageUtils.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "UninstallPackageAction"


using namespace BPackageKit;
using namespace BPackageKit::BManager::BPrivate;

using BPackageKit::BSolverPackage;


UninstallPackageProcess::UninstallPackageProcess(const BString& packageName, Model* model)
	:
	AbstractPackageProcess(packageName, model)
{
	fDescription = B_TRANSLATE("Uninstalling \"%PackageName%\"");
	fDescription.ReplaceAll("%PackageName%", packageName);
}


UninstallPackageProcess::~UninstallPackageProcess()
{
}


const char*
UninstallPackageProcess::Name() const
{
	return "UninstallPackageAction";
}


const char*
UninstallPackageProcess::Description() const
{
	return fDescription;
}


status_t
UninstallPackageProcess::RunInternal()
{
	// TODO: ideally if the package is installed at multiple locations,
	// the user should be able to pick which one to remove.

	PackageManager* packageManager = new(std::nothrow)
		PackageManager(static_cast<BPackageInstallationLocation>(InstallLocation()));
	ObjectDeleter<PackageManager> solverDeleter(packageManager);

	packageManager->Init(BPackageManager::B_ADD_INSTALLED_REPOSITORIES);

	PackageInfoRef package = FindPackageByName(fPackageName);
	PackageState state = PackageUtils::State(package);

	packageManager->SetCurrentActionPackage(package, false);
	packageManager->AddProgressListener(this);

	const char* packageNameString = package->Name().String();

	try {
		packageManager->Uninstall(&packageNameString, 1);
	} catch (BFatalErrorException& ex) {
		BString errorString;
		errorString.SetToFormat("Fatal error occurred while uninstalling package %s: %s (%s)\n",
			fPackageName.String(), ex.Message().String(), ex.Details().String());
		AppUtils::NotifySimpleError(B_TRANSLATE("Fatal error"), errorString, B_STOP_ALERT);
		SetPackageState(fPackageName, state);
		return ex.Error();
	} catch (BAbortedByUserException& ex) {
		return B_OK;
	} catch (BNothingToDoException& ex) {
		return B_OK;
	} catch (BException& ex) {
		HDERROR("Exception occurred while uninstalling package %s: %s", fPackageName.String(),
			ex.Message().String());
		SetPackageState(fPackageName, state);
		return B_ERROR;
	}

	packageManager->RemoveProgressListener(this);

	return B_OK;
}


void
UninstallPackageProcess::StartApplyingChanges(BPackageManager::InstalledRepository& repository)
{
	BPackageManager::InstalledRepository::PackageList& packages = repository.PackagesToDeactivate();

	for (int32 i = 0; i < packages.CountItems(); i++) {
		BString packageName = packages.ItemAt(i)->Name();
		fRemovedPackageNames.insert(packageName);
	}
}


void
UninstallPackageProcess::ApplyingChangesDone(BPackageManager::InstalledRepository& repository)
{
	std::set<BString>::const_iterator it;
	for (it = fRemovedPackageNames.begin(); it != fRemovedPackageNames.end(); it++) {
		BString packageName = *it;
		SetPackageState(packageName, NONE);
		ClearPackageInstallationLocations(packageName);
	}
}
