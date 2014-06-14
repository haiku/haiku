/*
 * Copyright 2013-2014, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 * 		Stephan Aßmus <superstippi@gmx.de>
 * 		Rene Gollent <rene@gollent.com>
 */


#include "PackageManager.h"

#include <stdio.h>

#include <Alert.h>
#include <Catalog.h>

#include <package/DownloadFileRequest.h>
#include <package/manager/Exceptions.h>
#include <package/RefreshRepositoryRequest.h>
#include <package/solver/SolverPackage.h>
#include <package/solver/SolverProblem.h>
#include <package/solver/SolverProblemSolution.h>

#include "AutoDeleter.h"
#include "AutoLocker.h"
#include "Model.h"
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


typedef std::set<PackageInfoRef> PackageInfoSet;


// #pragma mark - PackageProgressListener


PackageProgressListener::~PackageProgressListener()
{
}


void
PackageProgressListener::DownloadProgressChanged(const char* packageName,
	float progress)
{
}


void
PackageProgressListener::DownloadProgressComplete(const char* packageName)
{
}


void
PackageProgressListener::StartApplyingChanges(
	BPackageManager::InstalledRepository& repository)
{
}


void
PackageProgressListener::ApplyingChangesDone(
	BPackageManager::InstalledRepository& repository)
{
}



// #pragma mark - InstallPackageAction


class InstallPackageAction : public PackageAction,
	private PackageProgressListener {
public:
	InstallPackageAction(PackageInfoRef package, Model* model)
		:
		PackageAction(PACKAGE_ACTION_INSTALL, package, model),
		fLastDownloadUpdate(0)
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
		ref->SetState(PENDING);
		fPackageManager->SetCurrentActionPackage(ref, true);
		fPackageManager->AddProgressListener(this);
		const char* packageName = ref->Title().String();
		try {
			fPackageManager->Install(&packageName, 1);
		} catch (BFatalErrorException ex) {
			_SetDownloadedPackagesState(NONE);
			BString errorString;
			errorString.SetToFormat(
				"Fatal error occurred while installing package %s: "
				"%s (%s)\n", packageName, ex.Message().String(),
				ex.Details().String());
			BAlert* alert(new(std::nothrow) BAlert(B_TRANSLATE("Fatal error"),
				errorString, B_TRANSLATE("Close")));
			if (alert != NULL)
				alert->Go(NULL);
			return ex.Error();
		} catch (BAbortedByUserException ex) {
			_SetDownloadedPackagesState(NONE);
			return B_OK;
		} catch (BNothingToDoException ex) {
			return B_OK;
		} catch (BException ex) {
			fprintf(stderr, "Exception occurred while installing package "
				"%s: %s\n", packageName, ex.Message().String());
			_SetDownloadedPackagesState(NONE);
			return B_ERROR;;
		}

		fPackageManager->RemoveProgressListener(this);

		_SetDownloadedPackagesState(ACTIVATED);

		return B_OK;
	}

	// DownloadProgressListener
	virtual void DownloadProgressChanged(const char* packageName,
		float progress)
	{
		bigtime_t now = system_time();
		if (now - fLastDownloadUpdate > 250000 || progress == 1.0) {
			BString tempName(packageName);
			tempName.Truncate(tempName.FindFirst('-'));
				// strip version suffix off package filename
			PackageInfoRef ref(FindPackageByName(tempName));
			if (ref.Get() != NULL) {
				ref->SetDownloadProgress(progress);
				fLastDownloadUpdate = now;
			}
		}
	}

	virtual void DownloadProgressComplete(const char* packageName)
	{
		BString tempName(packageName);
		tempName.Truncate(tempName.FindFirst('-'));
			// strip version suffix off package filename
		PackageInfoRef ref(FindPackageByName(tempName));
		if (ref.Get() != NULL) {
			ref->SetDownloadProgress(1.0);
			fDownloadedPackages.insert(ref);
		}
	}

private:
	void _SetDownloadedPackagesState(PackageState state)
	{
		for (PackageInfoSet::iterator it = fDownloadedPackages.begin();
			it != fDownloadedPackages.end(); ++it) {
			(*it)->SetState(state);
		}
	}

private:
	bigtime_t fLastDownloadUpdate;
	PackageInfoSet fDownloadedPackages;
};


// #pragma mark - UninstallPackageAction


class UninstallPackageAction : public PackageAction,
	private PackageProgressListener {
public:
	UninstallPackageAction(PackageInfoRef package, Model* model)
		:
		PackageAction(PACKAGE_ACTION_UNINSTALL, package, model)
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
		fPackageManager->AddProgressListener(this);
		const char* packageName = ref->Title().String();
		try {
			fPackageManager->Uninstall(&packageName, 1);
		} catch (BFatalErrorException ex) {
			BString errorString;
			errorString.SetToFormat(
				"Fatal error occurred while uninstalling package %s: "
				"%s (%s)\n", packageName, ex.Message().String(),
				ex.Details().String());
			BAlert* alert(new(std::nothrow) BAlert(B_TRANSLATE("Fatal error"),
				errorString, B_TRANSLATE("Close")));
			if (alert != NULL)
				alert->Go(NULL);
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

		fPackageManager->RemoveProgressListener(this);

		ref->SetState(NONE);

		return B_OK;
	}

	void StartApplyingChanges(
		BPackageManager::InstalledRepository& repository)

	{
		BPackageManager::InstalledRepository::PackageList& packages
			= repository.PackagesToDeactivate();
		for (int32 i = 0; i < packages.CountItems(); i++) {
			PackageInfoRef ref(FindPackageByName(packages.ItemAt(i)
					->Name()));
			if (ref.Get() != NULL)
				fRemovedPackages.Add(ref);
		}
	}

	void ApplyingChangesDone(
		BPackageManager::InstalledRepository& repository)
	{
		for (int32 i = 0; i < fRemovedPackages.CountItems(); i++)
			fRemovedPackages.ItemAt(i)->SetState(NONE);
	}

private:
	PackageList fRemovedPackages;
};


// #pragma mark - PackageManager


PackageManager::PackageManager(BPackageInstallationLocation location)
	:
	BPackageManager(location, &fClientInstallationInterface, this),
	BPackageManager::UserInteractionHandler(),
	fDecisionProvider(),
	fClientInstallationInterface(),
	fProblemWindow(NULL),
	fCurrentInstallPackage(NULL),
	fCurrentUninstallPackage(NULL)
{
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
PackageManager::GetPackageActions(PackageInfoRef package, Model* model)
{
	PackageActionList actionList;
	if (package->IsSystemPackage() || package->IsSystemDependency())
		return actionList;

	int32 state = package->State();
	if (state == ACTIVATED || state == INSTALLED) {
		actionList.Add(PackageActionRef(new UninstallPackageAction(
			package, model), true));
	} else if (state == NONE || state == UNINSTALLED) {
		actionList.Add(PackageActionRef(new InstallPackageAction(package,
				model),	true));
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
		result = BPackageManager::RefreshRepository(repoConfig);
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
		result = BPackageManager::DownloadPackage(fileURL, targetEntry,
			checksum);
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
PackageManager::AddProgressListener(PackageProgressListener* listener)
{
	fPackageProgressListeners.AddItem(listener);
}


void
PackageManager::RemoveProgressListener(PackageProgressListener* listener)
{
	fPackageProgressListeners.RemoveItem(listener);
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
		uninstallPackages.insert(fCurrentUninstallPackage);

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
PackageManager::ProgressPackageDownloadStarted(const char* packageName)
{
	// TODO: implement
}


void
PackageManager::ProgressPackageDownloadActive(const char* packageName,
	float completionPercentage)
{
	for (int32 i = 0; i < fPackageProgressListeners.CountItems(); i++) {
		fPackageProgressListeners.ItemAt(i)->DownloadProgressChanged(
			packageName, completionPercentage);
	}
}


void
PackageManager::ProgressPackageDownloadComplete(const char* packageName)
{
	for (int32 i = 0; i < fPackageProgressListeners.CountItems(); i++) {
		fPackageProgressListeners.ItemAt(i)->DownloadProgressComplete(
			packageName);
	}
}


void
PackageManager::ProgressPackageChecksumStarted(const char* title)
{
	// TODO: implement
}


void
PackageManager::ProgressPackageChecksumComplete(const char* title)
{
	// TODO: implement
}


void
PackageManager::ProgressStartApplyingChanges(InstalledRepository& repository)
{
	for (int32 i = 0; i < fPackageProgressListeners.CountItems(); i++)
		fPackageProgressListeners.ItemAt(i)->StartApplyingChanges(repository);
}


void
PackageManager::ProgressTransactionCommitted(InstalledRepository& repository,
	const BCommitTransactionResult& result)
{
	// TODO: implement
}


void
PackageManager::ProgressApplyingChangesDone(InstalledRepository& repository)
{
	for (int32 i = 0; i < fPackageProgressListeners.CountItems(); i++)
		fPackageProgressListeners.ItemAt(i)->ApplyingChangesDone(repository);
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
