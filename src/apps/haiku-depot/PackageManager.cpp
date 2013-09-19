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

#include "AutoDeleter.h"
#include "AutoLocker.h"
#include "ProblemWindow.h"
#include "ResultWindow.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackageManager"


using namespace BPackageKit::BPrivate;
using namespace BPackageKit::BManager::BPrivate;

using BPackageKit::BRefreshRepositoryRequest;
using BPackageKit::DownloadFileRequest;
using BPackageKit::BSolver;
using BPackageKit::BSolverPackage;
using BPackageKit::BSolverRepository;

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


// #pragma mark - UninstallPackageAction


class UninstallPackageAction : public PackageAction {
public:
	UninstallPackageAction(const PackageInfo& package, PackageManager* manager)
		:
		PackageAction(package, manager)
	{
	}

	virtual const char* Label() const
	{
		return B_TRANSLATE("Uninstall");
	}

	virtual status_t Perform()
	{
		const char* packageName = Package().Title().String();
		try {
			fPackageManager->Uninstall(&packageName, 1);
		} catch (BFatalErrorException ex) {
			fprintf(stderr, "Fatal error occurred while uninstalling package "
				"%s: %s (%s)\n", packageName, ex.Message().String(),
				ex.Details().String());
			return ex.Error();
		} catch (BException ex) {
			fprintf(stderr, "Exception occurred while uninstalling package "
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
	fClientInstallationInterface(),
	fProblemWindow(NULL)
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

	if (fProblemWindow != NULL)
		fProblemWindow->PostMessage(B_QUIT_REQUESTED);
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

	BObjectList<BSolverPackage> packages;
	status_t result = Solver()->FindPackages(package.Title(),
		BSolver::B_FIND_IN_NAME, packages);
	if (result == B_OK) {
		bool installed = false;
		bool systemPackage = false;
		for (int32 i = 0; i < packages.CountItems(); i++) {
			const BSolverPackage* solverPackage = packages.ItemAt(i);
			if (solverPackage->Name() != package.Title())
				continue;

			const BSolverRepository* repository = solverPackage->Repository();
			if (repository == static_cast<const BSolverRepository*>(
					SystemRepository())) {
				installed = true;
				systemPackage = true;
			} else if (repository == static_cast<const BSolverRepository*>(
					CommonRepository())) {
				installed = true;
			} else if (repository == static_cast<const BSolverRepository*>(
					HomeRepository())) {
				installed = true;
			}
		}

		if (installed) {
			if (!systemPackage) {
				actionList.Add(PackageActionRef(new UninstallPackageAction(
					package, this),	true));
			}
		} else {
			actionList.Add(PackageActionRef(new InstallPackageAction(package,
					this), true));
		}
		// TODO: activation status
	}

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
	if (fProblemWindow == NULL)
		fProblemWindow = new ProblemWindow;

	ProblemWindow::SolverPackageSet dummy;
	if (!fProblemWindow->Go(fSolver,dummy, dummy))
		throw BAbortedByUserException();
}


void
PackageManager::ConfirmChanges(bool fromMostSpecific)
{
	ResultWindow* window = new ResultWindow;
	ObjectDeleter<ResultWindow> windowDeleter(window);

	bool hasOtherChanges = false;
	int32 count = fInstalledRepositories.CountItems();
	if (fromMostSpecific) {
		for (int32 i = count - 1; i >= 0; i--)
			hasOtherChanges
				|= _AddResults(*fInstalledRepositories.ItemAt(i), window);
	} else {
		for (int32 i = 0; i < count; i++)
			hasOtherChanges
				|= _AddResults(*fInstalledRepositories.ItemAt(i), window);
	}

	if (!hasOtherChanges)
		return;

	// show the window
	if (windowDeleter.Detach()->Go() == 0)
		throw BAbortedByUserException();
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


bool
PackageManager::_AddResults(InstalledRepository& repository,
	ResultWindow* window)
{
	if (!repository.HasChanges())
		return false;

	ProblemWindow::SolverPackageSet dummy;
	return window->AddLocationChanges(repository.Name(),
		repository.PackagesToActivate(), dummy,
		repository.PackagesToDeactivate(), dummy);
}


