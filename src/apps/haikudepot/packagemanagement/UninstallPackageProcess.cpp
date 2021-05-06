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


#include "UninstallPackageProcess.h"

#include <package/manager/Exceptions.h>
#include <package/solver/SolverPackage.h>

#include <Catalog.h>

#include "Alert.h"
#include "AppUtils.h"
#include "Logger.h"
#include "PackageManager.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "UninstallPackageAction"


using namespace BPackageKit;
using namespace BPackageKit::BManager::BPrivate;

using BPackageKit::BSolverPackage;


UninstallPackageProcess::UninstallPackageProcess(PackageInfoRef package,
		Model* model)
	:
	AbstractPackageProcess(package, model)
{
	fDescription = B_TRANSLATE("Uninstalling \"%PackageName%\"");
	fDescription.ReplaceAll("%PackageName%", fPackage->Name());
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
	fPackageManager->Init(BPackageManager::B_ADD_INSTALLED_REPOSITORIES);
	PackageInfoRef ref(fPackage);
	PackageState state = ref->State();
	fPackageManager->SetCurrentActionPackage(ref, false);
	fPackageManager->AddProgressListener(this);
	const char* packageName = ref->Name().String();
	try {
		fPackageManager->Uninstall(&packageName, 1);
	} catch (BFatalErrorException& ex) {
		BString errorString;
		errorString.SetToFormat(
			"Fatal error occurred while uninstalling package %s: "
			"%s (%s)\n", packageName, ex.Message().String(),
			ex.Details().String());
		AppUtils::NotifySimpleError(B_TRANSLATE("Fatal error"), errorString,
			B_STOP_ALERT);
		ref->SetState(state);
		return ex.Error();
	} catch (BAbortedByUserException& ex) {
		return B_OK;
	} catch (BNothingToDoException& ex) {
		return B_OK;
	} catch (BException& ex) {
		HDERROR("Exception occurred while uninstalling package %s: %s",
			packageName, ex.Message().String());
		ref->SetState(state);
		return B_ERROR;
	}

	fPackageManager->RemoveProgressListener(this);

	ref->ClearInstallationLocations();
	ref->SetState(NONE);

	return B_OK;
}


void
UninstallPackageProcess::StartApplyingChanges(
		BPackageManager::InstalledRepository& repository)
{
	BPackageManager::InstalledRepository::PackageList& packages
		= repository.PackagesToDeactivate();
	for (int32 i = 0; i < packages.CountItems(); i++) {
		PackageInfoRef ref(FindPackageByName(packages.ItemAt(i)
				->Name()));
		if (ref.IsSet())
			fRemovedPackages.push_back(ref);
	}
}


void
UninstallPackageProcess::ApplyingChangesDone(
		BPackageManager::InstalledRepository& repository)
{
	std::vector<PackageInfoRef>::iterator it;
	for (it = fRemovedPackages.begin(); it != fRemovedPackages.end(); it++) {
		PackageInfoRef packageInfoRef = *it;
		packageInfoRef->SetState(NONE);
	}
}
