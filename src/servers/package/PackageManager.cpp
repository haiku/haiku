/*
 * Copyright 2013-2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackageManager.h"

#include <Catalog.h>
#include <Notification.h>
#include <package/DownloadFileRequest.h>
#include <package/RefreshRepositoryRequest.h>
#include <package/solver/SolverPackage.h>
#include <package/solver/SolverPackageSpecifierList.h>
#include <package/solver/SolverProblem.h>
#include <package/solver/SolverProblemSolution.h>

#include <AutoDeleter.h>
#include <package/manager/Exceptions.h>
#include <package/manager/RepositoryBuilder.h>
#include <Server.h>

#include "ProblemWindow.h"
#include "ResultWindow.h"
#include "Root.h"
#include "Volume.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackageManager"

using BPackageKit::BManager::BPrivate::BAbortedByUserException;
using BPackageKit::BManager::BPrivate::BFatalErrorException;
using BPackageKit::BManager::BPrivate::BRepositoryBuilder;


PackageManager::PackageManager(Root* root, Volume* volume)
	:
	BPackageManager(volume->Location(), this, this),
	BPackageManager::UserInteractionHandler(),
	fRoot(root),
	fVolume(volume),
	fSolverPackages(),
	fPackagesAddedByUser(),
	fPackagesRemovedByUser(),
	fProblemWindow(NULL)
{
}


PackageManager::~PackageManager()
{
	if (fProblemWindow != NULL)
		fProblemWindow->PostMessage(B_QUIT_REQUESTED);
}


void
PackageManager::HandleUserChanges()
{
	const PackageSet& packagesToActivate = fVolume->PackagesToBeActivated();
	const PackageSet& packagesToDeactivate = fVolume->PackagesToBeDeactivated();

	if (packagesToActivate.empty() && packagesToDeactivate.empty())
		return;

	if (packagesToActivate.empty()) {
		// only packages removed -- use uninstall mode
		Init(B_ADD_INSTALLED_REPOSITORIES);

		BSolverPackageSpecifierList packagesToUninstall;
		for (PackageSet::const_iterator it = packagesToDeactivate.begin();
			it != packagesToDeactivate.end(); ++it) {
			BSolverPackage* solverPackage = _SolverPackageFor(*it);
			if (solverPackage == NULL)
				continue;

			if (!packagesToUninstall.AppendSpecifier(solverPackage))
				throw std::bad_alloc();
			fPackagesRemovedByUser.insert(solverPackage);
		}

		if (fPackagesRemovedByUser.empty())
			return;

		Uninstall(packagesToUninstall);
	} else {
		// packages added and (possibly) remove -- manually add/remove those
		// from the repository and use verify mode
		Init(B_ADD_INSTALLED_REPOSITORIES | B_ADD_REMOTE_REPOSITORIES
			| B_REFRESH_REPOSITORIES);

		// disable and remove uninstalled packages
		InstalledRepository& repository = InstallationRepository();
		for (PackageSet::const_iterator it = packagesToDeactivate.begin();
			it != packagesToDeactivate.end(); ++it) {
			BSolverPackage* solverPackage = _SolverPackageFor(*it);
			if (solverPackage == NULL)
				continue;

			repository.DisablePackage(solverPackage);
			if (!repository.PackagesToDeactivate().AddItem(solverPackage))
				throw std::bad_alloc();
			fPackagesRemovedByUser.insert(solverPackage);
		}

		// add new packages
		BRepositoryBuilder repositoryBuilder(repository);
		for (PackageSet::const_iterator it = packagesToActivate.begin();
			it != packagesToActivate.end(); ++it) {
			Package* package = *it;
			BSolverPackage* solverPackage;
			repositoryBuilder.AddPackage(package->Info(), NULL, &solverPackage);
			fSolverPackages[package] = solverPackage;
			if (!repository.PackagesToActivate().AddItem(solverPackage))
				throw std::bad_alloc();
			fPackagesAddedByUser.insert(solverPackage);
		}

		if (fPackagesRemovedByUser.empty() && fPackagesAddedByUser.empty())
			return;

// TODO: When packages are moved out of the packages directory, we can't create
// a complete "old-state" directory.

		VerifyInstallation();
	}
}


void
PackageManager::InitInstalledRepository(InstalledRepository& repository)
{
	const char* name = repository.InitialName();
	BRepositoryBuilder repositoryBuilder(repository, name);

	if (Volume* volume = fRoot->GetVolume(repository.Location())) {
		for (PackageFileNameHashTable::Iterator it
				= volume->PackagesByFileNameIterator(); it.HasNext();) {
			Package* package = it.Next();
			if (package->IsActive()) {
				BSolverPackage* solverPackage;
				repositoryBuilder.AddPackage(package->Info(), NULL,
					&solverPackage);
				fSolverPackages[package] = solverPackage;
			}
		}
	}
}


void
PackageManager::ResultComputed(InstalledRepository& repository)
{
	// Normalize the result, i.e. remove the packages added by the user which
	// have been removed again in the problem resolution process, and vice
	// versa.
	if (repository.Location() != fVolume->Location())
		return;

	PackageList& packagesToActivate = repository.PackagesToActivate();
	PackageList& packagesToDeactivate = repository.PackagesToDeactivate();

	for (int32 i = 0; BSolverPackage* package = packagesToDeactivate.ItemAt(i);
		i++) {
		if (fPackagesAddedByUser.erase(package) == 0)
			continue;

		for (SolverPackageMap::iterator it = fSolverPackages.begin();
			it != fSolverPackages.end(); ++it) {
			if (it->second == package) {
				fSolverPackages.erase(it);
				break;
			}
		}

		repository.EnablePackage(package);
		packagesToDeactivate.RemoveItemAt(i--);
		packagesToActivate.RemoveItem(package);
		repository.DeletePackage(package);
	}

	for (int32 i = 0; BSolverPackage* package = packagesToActivate.ItemAt(i);
		i++) {
		if (fPackagesRemovedByUser.erase(package) == 0)
			continue;

		repository.EnablePackage(package);
		packagesToActivate.RemoveItemAt(i--);
		packagesToDeactivate.RemoveItem(package);

		// Note: We keep the package activated, but nonetheless it is gone,
		// since the user has removed it from the packages directory. So unless
		// the user moves it back, we won't find it upon next reboot.
		// TODO: We probable even run into trouble when the user moves in a
		// replacement afterward.
	}
}


status_t
PackageManager::PrepareTransaction(Transaction& transaction)
{
	Volume* volume = fRoot->GetVolume(transaction.Repository().Location());
	if (volume == NULL)
		return B_BAD_VALUE;

	return volume->CreateTransaction(transaction.Repository().Location(),
		transaction.ActivationTransaction(),
		transaction.TransactionDirectory());
}


status_t
PackageManager::CommitTransaction(Transaction& transaction,
	BCommitTransactionResult& _result)
{
	Volume* volume = fRoot->GetVolume(transaction.Repository().Location());
	if (volume == NULL)
		return B_BAD_VALUE;

	// Get the packages that have already been added to/removed from the
	// packages directory and thus need to be handled specially by
	// Volume::CommitTransaction().
	PackageSet packagesAlreadyAdded;
	PackageSet packagesAlreadyRemoved;
	if (volume == fVolume) {
		const PackageSet& packagesToActivate = volume->PackagesToBeActivated();
		for (PackageSet::const_iterator it = packagesToActivate.begin();
			it != packagesToActivate.end(); ++it) {
			Package* package = *it;
			if (fPackagesAddedByUser.find(_SolverPackageFor(package))
					!= fPackagesAddedByUser.end()) {
				packagesAlreadyAdded.insert(package);
			}
		}

		const PackageSet& packagesToDeactivate
			= volume->PackagesToBeDeactivated();
		for (PackageSet::const_iterator it = packagesToDeactivate.begin();
			it != packagesToDeactivate.end(); ++it) {
			Package* package = *it;
			if (fPackagesRemovedByUser.find(_SolverPackageFor(package))
					!= fPackagesRemovedByUser.end()) {
				packagesAlreadyRemoved.insert(package);
			}
		}
	}

	volume->CommitTransaction(transaction.ActivationTransaction(),
		packagesAlreadyAdded, packagesAlreadyRemoved, _result);
	return B_OK;
}


void
PackageManager::HandleProblems()
{
	_InitGui();

	if (fProblemWindow == NULL)
		fProblemWindow = new ProblemWindow;

	if (!fProblemWindow->Go(fSolver, fPackagesAddedByUser,
			fPackagesRemovedByUser)) {
		throw BAbortedByUserException();
	}
}


void
PackageManager::ConfirmChanges(bool fromMostSpecific)
{
	// Check whether there are any changes other than those made by the user.
	_InitGui();
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
	va_list args;
	va_start(args, format);
	BString message;
	message.SetToFormatVarArgs(format, args);
	va_end(args);

	if (error != B_OK)
		message << BString().SetToFormat(": %s", strerror(error));

	BNotification notification(B_ERROR_NOTIFICATION);
	notification.SetGroup(B_TRANSLATE("Package daemon"));
	notification.SetTitle(B_TRANSLATE("Warning"));
	notification.SetContent(message);
	notification.Send();
}


void
PackageManager::ProgressPackageDownloadStarted(const char* packageName)
{
}


void
PackageManager::ProgressPackageDownloadActive(const char* packageName,
	float completionPercentage, off_t bytes, off_t totalBytes)
{
}


void
PackageManager::ProgressPackageDownloadComplete(const char* packageName)
{
}


void
PackageManager::ProgressPackageChecksumStarted(const char* title)
{
}


void
PackageManager::ProgressPackageChecksumComplete(const char* title)
{
}


void
PackageManager::ProgressStartApplyingChanges(InstalledRepository& repository)
{
}


void
PackageManager::ProgressTransactionCommitted(InstalledRepository& repository,
	const BCommitTransactionResult& result)
{
}


void
PackageManager::ProgressApplyingChangesDone(InstalledRepository& repository)
{
}


void
PackageManager::JobFailed(BSupportKit::BJob* job)
{
// TODO:...
}


void
PackageManager::JobAborted(BSupportKit::BJob* job)
{
// TODO:...
}


bool
PackageManager::_AddResults(InstalledRepository& repository,
	ResultWindow* window)
{
	if (!repository.HasChanges())
		return false;

	return window->AddLocationChanges(repository.Name(),
		repository.PackagesToActivate(), fPackagesAddedByUser,
		repository.PackagesToDeactivate(), fPackagesRemovedByUser);
}


BSolverPackage*
PackageManager::_SolverPackageFor(Package* package) const
{
	SolverPackageMap::const_iterator it = fSolverPackages.find(package);
	return it != fSolverPackages.end() ? it->second : NULL;
}


void
PackageManager::_InitGui()
{
	BServer* server = dynamic_cast<BServer*>(be_app);
	if (server == NULL || server->InitGUIContext() != B_OK)
		throw BFatalErrorException("failed to initialize the GUI");
}
