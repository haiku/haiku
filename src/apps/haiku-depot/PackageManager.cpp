/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 * 		Stephan Aßmus <superstippi@gmx.de>
 * 		Rene Gollent, reneGollent.com.
 */


#include "PackageManager.h"

#include <stdio.h>

#include <Catalog.h>

#include <package/DownloadFileRequest.h>
#include <package/manager/Exceptions.h>
#include <package/RefreshRepositoryRequest.h>
#include <package/solver/SolverPackage.h>
#include <package/solver/SolverProblem.h>
#include <package/solver/SolverProblemSolution.h>

#include "AutoLocker.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackageManager"


using BPackageKit::BRefreshRepositoryRequest;
using BPackageKit::DownloadFileRequest;
using namespace BPackageKit::BPrivate;
using BPackageKit::BManager::BPrivate::BException;
using BPackageKit::BManager::BPrivate::BFatalErrorException;


// #pragma mark - PackageAction


PackageAction::PackageAction(const PackageInfo& package,
	PackageManager* manager)
	:
	fPackageManager(manager),
	fPackage(package)
{
}


PackageAction::~PackageAction()
{
}


// #pragma mark - InstallPackageAction


class InstallPackageAction : public PackageAction {
public:
	InstallPackageAction(const PackageInfo& package, PackageManager* manager)
		:
		PackageAction(package, manager)
	{
	}

	virtual const char* Label() const
	{
		return B_TRANSLATE("Install");
	}

	virtual status_t Perform()
	{
		const char* packageName = Package().Title().String();
		try {
			fPackageManager->Install(&packageName, 1);
		} catch (BFatalErrorException ex) {
			fprintf(stderr, "Fatal error occurred while installing package "
				"%s: %s (%s)\n", packageName, ex.Message().String(),
				ex.Details().String());
			return ex.Error();
		} catch (BException ex) {
			fprintf(stderr, "Exception occurred while installing package "
				"%s: %s\n", packageName, ex.Message().String());
		}

		return B_OK;
	}
};


// #pragma mark - PackageManager


PackageManager::PackageManager(BPackageInstallationLocation location)
	:
	BPackageManager(location),
	BPackageManager::UserInteractionHandler(),
	fDecisionProvider(),
	fJobStateListener(),
	fContext(fDecisionProvider, fJobStateListener),
	fClientInstallationInterface()
{
	fInstallationInterface = &fClientInstallationInterface;
	fRequestHandler = this;
	fUserInteractionHandler = this;

	fPendingActionsSem = create_sem(0, "PendingPackageActions");
	if (fPendingActionsSem >= 0) {
		fPendingActionsWorker = spawn_thread(&_PackageActionWorker,
			"Planet Express", B_NORMAL_PRIORITY, this);
		if (fPendingActionsWorker >= 0)
			resume_thread(fPendingActionsWorker);
	}
}


PackageManager::~PackageManager()
{
	delete_sem(fPendingActionsSem);
	status_t result;
	wait_for_thread(fPendingActionsWorker, &result);
}



PackageState
PackageManager::GetPackageState(const PackageInfo& package)
{
	// TODO: Fetch information from the PackageKit
	return NONE;
}


PackageActionList
PackageManager::GetPackageActions(const PackageInfo& package)
{
	PackageActionList actionList;

	// TODO: Actually fetch applicable actions for this package.
	// If the package is installed and active, it can be
	//		* uninstalled
	//		* deactivated
	// If the package is installed and inactive, it can be
	//		* uninstalled
	//		* activated
	// If the package is not installed, it can be
	//		* installed (and it will be automatically activated)
	actionList.Add(PackageActionRef(new InstallPackageAction(package, this),
			true));

	return actionList;
}


status_t
PackageManager::SchedulePackageActions(PackageActionList& list)
{
	AutoLocker<BLocker> lock(&fPendingActionsLock);
	for (int32 i = 0; i < list.CountItems(); i++) {
		if (!fPendingActions.Add(list.ItemAtFast(i))) {
			return B_NO_MEMORY;
		}
	}

	return release_sem_etc(fPendingActionsSem, list.CountItems(), 0);
}


status_t
PackageManager::RefreshRepository(const BRepositoryConfig& repoConfig)
{
	status_t result;
	try {
		result = BRefreshRepositoryRequest(fContext, repoConfig).Process();
	} catch (BFatalErrorException ex) {
		fprintf(stderr, "Fatal error occurred while refreshing repository: "
			"%s (%s)\n", ex.Message().String(), ex.Details().String());
		result = ex.Error();
	} catch (BException ex) {
		fprintf(stderr, "Exception occurred while refreshing "
			"repository: %s\n", ex.Message().String());
		result = B_ERROR;
	}

	return result;
}


status_t
PackageManager::DownloadPackage(const BString& fileURL,
	const BEntry& targetEntry, const BString& checksum)
{
	status_t result;
	try {
		result = DownloadFileRequest(fContext, fileURL, targetEntry, checksum)
			.Process();
	} catch (BFatalErrorException ex) {
		fprintf(stderr, "Fatal error occurred while downloading package: "
			"%s: %s (%s)\n", fileURL.String(), ex.Message().String(),
			ex.Details().String());
		result = ex.Error();
	} catch (BException ex) {
		fprintf(stderr, "Exception occurred while downloading package "
			"%s: %s\n", fileURL.String(), ex.Message().String());
		result = B_ERROR;
	}

	return result;
}


void
PackageManager::HandleProblems()
{
	// TODO: implement
}


void
PackageManager::ConfirmChanges(bool fromMostSpecific)
{
	// TODO: implement
}


void
PackageManager::Warn(status_t error, const char* format, ...)
{
	// TODO: implement
}


void
PackageManager::ProgressStartApplyingChanges(InstalledRepository& repository)
{
	// TODO: implement
}


void
PackageManager::ProgressTransactionCommitted(InstalledRepository& repository,
	const char* transactionDirectoryName)
{
	// TODO: implement
}


void
PackageManager::ProgressApplyingChangesDone(InstalledRepository& repository)
{
	// TODO: implement
}


status_t
PackageManager::_PackageActionWorker(void* arg)
{
	PackageManager* manager = reinterpret_cast<PackageManager*>(arg);

	while (acquire_sem(manager->fPendingActionsSem) == B_OK) {
		PackageActionRef ref;
		{
			AutoLocker<BLocker> lock(&manager->fPendingActionsLock);
			ref = manager->fPendingActions.ItemAt(0);
			if (ref.Get() == NULL)
				break;
			manager->fPendingActions.Remove(0);
		}

		ref->Perform();
	}

	return 0;
}
