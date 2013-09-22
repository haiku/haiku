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
#include "PackageInfo.h"
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


// #pragma mark - InstallPackageAction


class InstallPackageAction : public PackageAction {
public:
	InstallPackageAction(PackageInfoRef package)
		:
		PackageAction(package)
	{
	}

	virtual const char* Label() const
	{
		return B_TRANSLATE("Install");
	}

	virtual status_t Perform()
	{
		fPackageManager->Init(BPackageManager::B_ADD_INSTALLED_REPOSITORIES
			| BPackageManager::B_ADD_REMOTE_REPOSITORIES
			| BPackageManager::B_REFRESH_REPOSITORIES);
		PackageInfoRef ref(Package());
		fPackageManager->SetCurrentActionPackage(ref, true);
		const char* packageName = ref->Title().String();
		try {
			fPackageManager->Install(&packageName, 1);
		} catch (BFatalErrorException ex) {
			fprintf(stderr, "Fatal error occurred while installing package "
				"%s: %s (%s)\n", packageName, ex.Message().String(),
				ex.Details().String());
			return ex.Error();
		} catch (BAbortedByUserException ex) {
			return B_OK;
		} catch (BNothingToDoException ex) {
			return B_OK;
		} catch (BException ex) {
			fprintf(stderr, "Exception occurred while installing package "
				"%s: %s\n", packageName, ex.Message().String());
			return B_ERROR;;
		}

		ref->SetState(ACTIVATED);

		return B_OK;
	}
};


// #pragma mark - UninstallPackageAction


class UninstallPackageAction : public PackageAction {
public:
	UninstallPackageAction(PackageInfoRef package)
		:
		PackageAction(package)
	{
	}

	virtual const char* Label() const
	{
		return B_TRANSLATE("Uninstall");
	}

	virtual status_t Perform()
	{
		fPackageManager->Init(BPackageManager::B_ADD_INSTALLED_REPOSITORIES);
		PackageInfoRef ref(Package());
		fPackageManager->SetCurrentActionPackage(ref, false);
		const char* packageName = ref->Title().String();
		try {
			fPackageManager->Uninstall(&packageName, 1);
		} catch (BFatalErrorException ex) {
			fprintf(stderr, "Fatal error occurred while uninstalling package "
				"%s: %s (%s)\n", packageName, ex.Message().String(),
				ex.Details().String());
			return ex.Error();
		} catch (BAbortedByUserException ex) {
			return B_OK;
		} catch (BNothingToDoException ex) {
			return B_OK;
		} catch (BException ex) {
			fprintf(stderr, "Exception occurred while uninstalling package "
				"%s: %s\n", packageName, ex.Message().String());
			return B_ERROR;
		}

		ref->SetState(NONE);

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
	fProblemWindow(NULL),
	fCurrentInstallPackage(NULL),
	fCurrentUninstallPackage(NULL)
{
	fInstallationInterface = &fClientInstallationInterface;
	fRequestHandler = this;
	fUserInteractionHandler = this;
}


PackageManager::~PackageManager()
{
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
PackageManager::GetPackageActions(PackageInfoRef package)
{
	Init(B_ADD_INSTALLED_REPOSITORIES | B_ADD_REMOTE_REPOSITORIES);
	PackageActionList actionList;

	bool installed = false;
	bool systemPackage = false;
	BSolverPackage* solverPackage = _GetSolverPackage(package);
	if (solverPackage == NULL)
		return actionList;

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

	if (installed) {
		if (!systemPackage) {
			actionList.Add(PackageActionRef(new UninstallPackageAction(
				package), true));
		}
	} else {
		actionList.Add(PackageActionRef(new InstallPackageAction(package),
				true));
	}
	// TODO: activation status

	return actionList;
}


void
PackageManager::SetCurrentActionPackage(PackageInfoRef package, bool install)
{
	BSolverPackage* solverPackage = _GetSolverPackage(package);
	fCurrentInstallPackage = install ? solverPackage : NULL;
	fCurrentUninstallPackage = install ? NULL : solverPackage;
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

	ProblemWindow::SolverPackageSet installPackages;
	ProblemWindow::SolverPackageSet uninstallPackages;
	if (fCurrentInstallPackage != NULL)
		installPackages.insert(fCurrentInstallPackage);

	if (fCurrentUninstallPackage != NULL)
		uninstallPackages.insert(fCurrentInstallPackage);

	if (!fProblemWindow->Go(fSolver,installPackages, uninstallPackages))
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


bool
PackageManager::_AddResults(InstalledRepository& repository,
	ResultWindow* window)
{
	if (!repository.HasChanges())
		return false;

	ProblemWindow::SolverPackageSet installPackages;
	ProblemWindow::SolverPackageSet uninstallPackages;
	if (fCurrentInstallPackage != NULL)
		installPackages.insert(fCurrentInstallPackage);

	if (fCurrentUninstallPackage != NULL)
		uninstallPackages.insert(fCurrentUninstallPackage);

	return window->AddLocationChanges(repository.Name(),
		repository.PackagesToActivate(), installPackages,
		repository.PackagesToDeactivate(), uninstallPackages);
}


BSolverPackage*
PackageManager::_GetSolverPackage(PackageInfoRef package)
{
	int32 flags = BSolver::B_FIND_IN_NAME;
	if (package->State() == ACTIVATED || package->State() == INSTALLED)
		flags |= BSolver::B_FIND_INSTALLED_ONLY;

	BObjectList<BSolverPackage> packages;
	status_t result = Solver()->FindPackages(package->Title(),
		flags, packages);
	if (result == B_OK) {
		for (int32 i = 0; i < packages.CountItems(); i++) {
			BSolverPackage* solverPackage = packages.ItemAt(i);
			if (solverPackage->Name() != package->Title())
				continue;
			else if (package->State() == NONE
				&& dynamic_cast<BPackageManager::RemoteRepository*>(
					solverPackage->Repository()) == NULL) {
				continue;
			}
			return solverPackage;
		}
	}

	return NULL;
}
