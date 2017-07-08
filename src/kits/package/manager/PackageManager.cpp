/*
 * Copyright 2013-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 *		Rene Gollent <rene@gollent.com>
 */


#include <package/manager/PackageManager.h>

#include <Catalog.h>
#include <Directory.h>
#include <package/CommitTransactionResult.h>
#include <package/DownloadFileRequest.h>
#include <package/PackageRoster.h>
#include <package/RefreshRepositoryRequest.h>
#include <package/RepositoryCache.h>
#include <package/solver/SolverPackage.h>
#include <package/solver/SolverPackageSpecifier.h>
#include <package/solver/SolverPackageSpecifierList.h>
#include <package/solver/SolverProblem.h>
#include <package/solver/SolverProblemSolution.h>
#include <package/solver/SolverResult.h>

#include <CopyEngine.h>
#include <package/ActivationTransaction.h>
#include <package/DaemonClient.h>
#include <package/FetchFileJob.h>
#include <package/manager/RepositoryBuilder.h>
#include <package/ValidateChecksumJob.h>

#include "PackageManagerUtils.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackageManagerKit"


using BPackageKit::BPrivate::FetchFileJob;
using BPackageKit::BPrivate::ValidateChecksumJob;


namespace BPackageKit {

namespace BManager {

namespace BPrivate {


// #pragma mark - BPackageManager


BPackageManager::BPackageManager(BPackageInstallationLocation location,
	InstallationInterface* installationInterface,
	UserInteractionHandler* userInteractionHandler)
	:
	fDebugLevel(0),
	fLocation(location),
	fSolver(NULL),
	fSystemRepository(new (std::nothrow) InstalledRepository("system",
		B_PACKAGE_INSTALLATION_LOCATION_SYSTEM, -1)),
	fHomeRepository(new (std::nothrow) InstalledRepository("home",
		B_PACKAGE_INSTALLATION_LOCATION_HOME, -3)),
	fInstalledRepositories(10),
	fOtherRepositories(10, true),
	fLocalRepository(new (std::nothrow) MiscLocalRepository),
	fTransactions(5, true),
	fInstallationInterface(installationInterface),
	fUserInteractionHandler(userInteractionHandler)
{
}


BPackageManager::~BPackageManager()
{
	delete fSolver;
	delete fSystemRepository;
	delete fHomeRepository;
	delete fLocalRepository;
}


void
BPackageManager::Init(uint32 flags)
{
	if (fSolver != NULL)
		return;

	// create the solver
	status_t error = BSolver::Create(fSolver);
	if (error != B_OK)
		DIE(error, "Failed to create solver");

	if (fSystemRepository == NULL || fHomeRepository == NULL
		|| fLocalRepository == NULL) {
		throw std::bad_alloc();
	}

	fSolver->SetDebugLevel(fDebugLevel);

	BRepositoryBuilder(*fLocalRepository).AddToSolver(fSolver, false);

	// add installation location repositories
	if ((flags & B_ADD_INSTALLED_REPOSITORIES) != 0) {
		// We add only the repository of our actual installation location as the
		// "installed" repository. The repositories for the more general
		// installation locations are added as regular repositories, but with
		// better priorities than the actual (remote) repositories. This
		// prevents the solver from showing conflicts when a package in a more
		// specific installation location overrides a package in a more general
		// one. Instead any requirement that is already installed in a more
		// general installation location will turn up as to be installed as
		// well. But we can easily filter those out.
		_AddInstalledRepository(fSystemRepository);

		if (!fSystemRepository->IsInstalled()) {
			// Only add the home repository if the directory exists
			BPath path;
			status_t error = find_directory(B_USER_PACKAGES_DIRECTORY, &path);
			if (error == B_OK && BEntry(path.Path()).Exists())
				_AddInstalledRepository(fHomeRepository);
		}
	}

	// add other repositories
	if ((flags & B_ADD_REMOTE_REPOSITORIES) != 0) {
		BPackageRoster roster;
		BStringList repositoryNames;
		error = roster.GetRepositoryNames(repositoryNames);
		if (error != B_OK) {
			fUserInteractionHandler->Warn(error,
				B_TRANSLATE("Failed to get repository names"));
		}

		int32 repositoryNameCount = repositoryNames.CountStrings();
		for (int32 i = 0; i < repositoryNameCount; i++) {
			_AddRemoteRepository(roster, repositoryNames.StringAt(i),
				(flags & B_REFRESH_REPOSITORIES) != 0);
		}
	}
}


void
BPackageManager::SetDebugLevel(int32 level)
{
	fDebugLevel = level;

	if (fSolver != NULL)
		fSolver->SetDebugLevel(fDebugLevel);
}


void
BPackageManager::Install(const char* const* packages, int packageCount)
{
	BSolverPackageSpecifierList packagesToInstall;
	_AddPackageSpecifiers(packages, packageCount, packagesToInstall);
	Install(packagesToInstall);
}


void
BPackageManager::Install(const BSolverPackageSpecifierList& packages)
{
	Init(B_ADD_INSTALLED_REPOSITORIES | B_ADD_REMOTE_REPOSITORIES
		| B_REFRESH_REPOSITORIES);

	// solve
	const BSolverPackageSpecifier* unmatchedSpecifier;
	status_t error = fSolver->Install(packages, &unmatchedSpecifier);
	if (error != B_OK) {
		if (unmatchedSpecifier != NULL) {
			DIE(error, "Failed to find a match for \"%s\"",
				unmatchedSpecifier->SelectString().String());
		} else
			DIE(error, "Failed to compute packages to install");
	}

	_HandleProblems();

	// install/uninstall packages
	_AnalyzeResult();
	_ConfirmChanges();
	_ApplyPackageChanges();
}


void
BPackageManager::Uninstall(const char* const* packages, int packageCount)
{
	BSolverPackageSpecifierList packagesToUninstall;
	if (!packagesToUninstall.AppendSpecifiers(packages, packageCount))
		throw std::bad_alloc();
	Uninstall(packagesToUninstall);
}


void
BPackageManager::Uninstall(const BSolverPackageSpecifierList& packages)
{
	Init(B_ADD_INSTALLED_REPOSITORIES);

	// find the packages that match the specification
	const BSolverPackageSpecifier* unmatchedSpecifier;
	PackageList foundPackages;
	status_t error = fSolver->FindPackages(packages,
		BSolver::B_FIND_INSTALLED_ONLY, foundPackages, &unmatchedSpecifier);
	if (error != B_OK) {
		if (unmatchedSpecifier != NULL) {
			DIE(error, "Failed to find a match for \"%s\"",
				unmatchedSpecifier->SelectString().String());
		} else
			DIE(error, "Failed to compute packages to uninstall");
	}

	// determine the inverse base package closure for the found packages
// TODO: Optimize!
	InstalledRepository& installationRepository = InstallationRepository();
	bool foundAnotherPackage;
	do {
		foundAnotherPackage = false;
		int32 count = installationRepository.CountPackages();
		for (int32 i = 0; i < count; i++) {
			BSolverPackage* package = installationRepository.PackageAt(i);
			if (foundPackages.HasItem(package))
				continue;

			if (_FindBasePackage(foundPackages, package->Info()) >= 0) {
				foundPackages.AddItem(package);
				foundAnotherPackage = true;
			}
		}
	} while (foundAnotherPackage);

	// remove the packages from the repository
	for (int32 i = 0; BSolverPackage* package = foundPackages.ItemAt(i); i++)
		installationRepository.DisablePackage(package);

	for (;;) {
		error = fSolver->VerifyInstallation(BSolver::B_VERIFY_ALLOW_UNINSTALL);
		if (error != B_OK)
			DIE(error, "Failed to compute packages to uninstall");

		_HandleProblems();

		// (virtually) apply the result to this repository
		_AnalyzeResult();

		for (int32 i = foundPackages.CountItems() - 1; i >= 0; i--) {
			if (!installationRepository.PackagesToDeactivate()
					.AddItem(foundPackages.ItemAt(i))) {
				throw std::bad_alloc();
			}
		}

		installationRepository.ApplyChanges();

		// verify the next specific respository
		if (!_NextSpecificInstallationLocation())
			break;

		foundPackages.MakeEmpty();

		// NOTE: In theory, after verifying a more specific location, it would
		// be more correct to compute the inverse base package closure for the
		// packages we need to uninstall and (if anything changed) verify again.
		// In practice, however, base packages are always required with an exact
		// version (ATM). If that base package still exist in a more general
		// location (the only reason why the package requiring the base package
		// wouldn't be marked to be uninstalled as well) there shouldn't have
		// been any reason to remove it from the more specific location in the
		// first place.
	}

	_ConfirmChanges(true);
	_ApplyPackageChanges(true);
}


void
BPackageManager::Update(const char* const* packages, int packageCount)
{
	BSolverPackageSpecifierList packagesToUpdate;
	_AddPackageSpecifiers(packages, packageCount, packagesToUpdate);
	Update(packagesToUpdate);
}


void
BPackageManager::Update(const BSolverPackageSpecifierList& packages)
{
	Init(B_ADD_INSTALLED_REPOSITORIES | B_ADD_REMOTE_REPOSITORIES
		| B_REFRESH_REPOSITORIES);

	// solve
	const BSolverPackageSpecifier* unmatchedSpecifier;
	status_t error = fSolver->Update(packages, true,
		&unmatchedSpecifier);
	if (error != B_OK) {
		if (unmatchedSpecifier != NULL) {
			DIE(error, "Failed to find a match for \"%s\"",
				unmatchedSpecifier->SelectString().String());
		} else
			DIE(error, "Failed to compute packages to update");
	}

	_HandleProblems();

	// install/uninstall packages
	_AnalyzeResult();
	_ConfirmChanges();
	_ApplyPackageChanges();
}


void
BPackageManager::FullSync()
{
	Init(B_ADD_INSTALLED_REPOSITORIES | B_ADD_REMOTE_REPOSITORIES
		| B_REFRESH_REPOSITORIES);

	// solve
	status_t error = fSolver->FullSync();
	if (error != B_OK)
		DIE(error, "Failed to compute packages to synchronize");

	_HandleProblems();

	// install/uninstall packages
	_AnalyzeResult();
	_ConfirmChanges();
	_ApplyPackageChanges();
}


void
BPackageManager::VerifyInstallation()
{
	Init(B_ADD_INSTALLED_REPOSITORIES | B_ADD_REMOTE_REPOSITORIES
		| B_REFRESH_REPOSITORIES);

	for (;;) {
		status_t error = fSolver->VerifyInstallation();
		if (error != B_OK)
			DIE(error, "Failed to compute package dependencies");

		_HandleProblems();

		// (virtually) apply the result to this repository
		_AnalyzeResult();
		InstallationRepository().ApplyChanges();

		// verify the next specific respository
		if (!_NextSpecificInstallationLocation())
			break;
	}

	_ConfirmChanges();
	_ApplyPackageChanges();
}


BPackageManager::InstalledRepository&
BPackageManager::InstallationRepository()
{
	if (fInstalledRepositories.IsEmpty())
		DIE("No installation repository");

	return *fInstalledRepositories.LastItem();
}


void
BPackageManager::JobStarted(BSupportKit::BJob* job)
{
	if (dynamic_cast<FetchFileJob*>(job) != NULL) {
		FetchFileJob* fetchJob = (FetchFileJob*)job;
		fUserInteractionHandler->ProgressPackageDownloadStarted(
			fetchJob->DownloadFileName());
	} else if (dynamic_cast<ValidateChecksumJob*>(job) != NULL) {
		fUserInteractionHandler->ProgressPackageChecksumStarted(
			job->Title().String());
	}
}


void
BPackageManager::JobProgress(BSupportKit::BJob* job)
{
	if (dynamic_cast<FetchFileJob*>(job) != NULL) {
		FetchFileJob* fetchJob = (FetchFileJob*)job;
		fUserInteractionHandler->ProgressPackageDownloadActive(
			fetchJob->DownloadFileName(), fetchJob->DownloadProgress(),
			fetchJob->DownloadBytes(), fetchJob->DownloadTotalBytes());
	}
}


void
BPackageManager::JobSucceeded(BSupportKit::BJob* job)
{
	if (dynamic_cast<FetchFileJob*>(job) != NULL) {
		FetchFileJob* fetchJob = (FetchFileJob*)job;
		fUserInteractionHandler->ProgressPackageDownloadComplete(
			fetchJob->DownloadFileName());
	} else if (dynamic_cast<ValidateChecksumJob*>(job) != NULL) {
		fUserInteractionHandler->ProgressPackageChecksumComplete(
			job->Title().String());
	}
}


void
BPackageManager::_HandleProblems()
{
	while (fSolver->HasProblems()) {
		fUserInteractionHandler->HandleProblems();

		status_t error = fSolver->SolveAgain();
		if (error != B_OK)
			DIE(error, "Failed to recompute packages to un/-install");
	}
}


void
BPackageManager::_AnalyzeResult()
{
	BSolverResult result;
	status_t error = fSolver->GetResult(result);
	if (error != B_OK)
		DIE(error, "Failed to compute packages to un/-install");

	InstalledRepository& installationRepository = InstallationRepository();
	PackageList& packagesToActivate
		= installationRepository.PackagesToActivate();
	PackageList& packagesToDeactivate
		= installationRepository.PackagesToDeactivate();

	PackageList potentialBasePackages;

	for (int32 i = 0; const BSolverResultElement* element = result.ElementAt(i);
			i++) {
		BSolverPackage* package = element->Package();

		switch (element->Type()) {
			case BSolverResultElement::B_TYPE_INSTALL:
			{
				PackageList& packageList
					= dynamic_cast<InstalledRepository*>(package->Repository())
							!= NULL
						? potentialBasePackages
						: packagesToActivate;
				if (!packageList.AddItem(package))
					throw std::bad_alloc();
				break;
			}

			case BSolverResultElement::B_TYPE_UNINSTALL:
				if (!packagesToDeactivate.AddItem(package))
					throw std::bad_alloc();
				break;
		}
	}

	// Make sure base packages are installed in the same location.
	for (int32 i = 0; i < packagesToActivate.CountItems(); i++) {
		BSolverPackage* package = packagesToActivate.ItemAt(i);
		int32 index = _FindBasePackage(potentialBasePackages, package->Info());
		if (index < 0)
			continue;

		BSolverPackage* basePackage = potentialBasePackages.RemoveItemAt(index);
		if (!packagesToActivate.AddItem(basePackage))
			throw std::bad_alloc();
	}

	fInstallationInterface->ResultComputed(installationRepository);
}


void
BPackageManager::_ConfirmChanges(bool fromMostSpecific)
{
	// check, if there are any changes at all
	int32 count = fInstalledRepositories.CountItems();
	bool hasChanges = false;
	for (int32 i = 0; i < count; i++) {
		if (fInstalledRepositories.ItemAt(i)->HasChanges()) {
			hasChanges = true;
			break;
		}
	}

	if (!hasChanges)
		throw BNothingToDoException();

	fUserInteractionHandler->ConfirmChanges(fromMostSpecific);
}


void
BPackageManager::_ApplyPackageChanges(bool fromMostSpecific)
{
	int32 count = fInstalledRepositories.CountItems();
	if (fromMostSpecific) {
		for (int32 i = count - 1; i >= 0; i--)
			_PreparePackageChanges(*fInstalledRepositories.ItemAt(i));
	} else {
		for (int32 i = 0; i < count; i++)
			_PreparePackageChanges(*fInstalledRepositories.ItemAt(i));
	}

	for (int32 i = 0; Transaction* transaction = fTransactions.ItemAt(i); i++)
		_CommitPackageChanges(*transaction);

// TODO: Clean up the transaction directories on error!
}


void
BPackageManager::_PreparePackageChanges(
	InstalledRepository& installationRepository)
{
	if (!installationRepository.HasChanges())
		return;

	PackageList& packagesToActivate
		= installationRepository.PackagesToActivate();
	PackageList& packagesToDeactivate
		= installationRepository.PackagesToDeactivate();

	// create the transaction
	Transaction* transaction = new Transaction(installationRepository);
	if (!fTransactions.AddItem(transaction)) {
		delete transaction;
		throw std::bad_alloc();
	}

	status_t error = fInstallationInterface->PrepareTransaction(*transaction);
	if (error != B_OK)
		DIE(error, "Failed to create transaction");

	// download the new packages and prepare the transaction
	for (int32 i = 0; BSolverPackage* package = packagesToActivate.ItemAt(i);
		i++) {
		// get package URL and target entry

		BString fileName(package->Info().FileName());
		if (fileName.IsEmpty())
			throw std::bad_alloc();

		BEntry entry;
		error = entry.SetTo(&transaction->TransactionDirectory(), fileName);
		if (error != B_OK)
			DIE(error, "Failed to create package entry");

		RemoteRepository* remoteRepository
			= dynamic_cast<RemoteRepository*>(package->Repository());
		if (remoteRepository != NULL) {
			// download the package
			BString url = remoteRepository->Config().PackagesURL();
			url << '/' << fileName;

			status_t error = DownloadPackage(url, entry,
				package->Info().Checksum());
			if (error != B_OK)
				DIE(error, "Failed to download package %s",
					package->Info().Name().String());
		} else if (package->Repository() != &installationRepository) {
			// clone the existing package
			LocalRepository* localRepository
				= dynamic_cast<LocalRepository*>(package->Repository());
			if (localRepository == NULL) {
				DIE("Internal error: repository %s is not a local repository",
					package->Repository()->Name().String());
			}
			_ClonePackageFile(localRepository, package, entry);
		}

		// add package to transaction
		if (!transaction->ActivationTransaction().AddPackageToActivate(
				fileName)) {
			throw std::bad_alloc();
		}
	}

	for (int32 i = 0; BSolverPackage* package = packagesToDeactivate.ItemAt(i);
		i++) {
		// add package to transaction
		if (!transaction->ActivationTransaction().AddPackageToDeactivate(
				package->Info().FileName())) {
			throw std::bad_alloc();
		}
	}
}


void
BPackageManager::_CommitPackageChanges(Transaction& transaction)
{
	InstalledRepository& installationRepository = transaction.Repository();

	fUserInteractionHandler->ProgressStartApplyingChanges(
		installationRepository);

	// commit the transaction
	BCommitTransactionResult transactionResult;
	status_t error = fInstallationInterface->CommitTransaction(transaction,
		transactionResult);
	if (error != B_OK)
		DIE(error, "Failed to commit transaction");
	if (transactionResult.Error() != B_TRANSACTION_OK)
		DIE(transactionResult);

	fUserInteractionHandler->ProgressTransactionCommitted(
		installationRepository, transactionResult);

	BEntry transactionDirectoryEntry;
	if ((error = transaction.TransactionDirectory()
			.GetEntry(&transactionDirectoryEntry)) != B_OK
		|| (error = transactionDirectoryEntry.Remove()) != B_OK) {
		fUserInteractionHandler->Warn(error,
			B_TRANSLATE("Failed to remove transaction directory"));
	}

	fUserInteractionHandler->ProgressApplyingChangesDone(
		installationRepository);
}


void
BPackageManager::_ClonePackageFile(LocalRepository* repository,
	BSolverPackage* package, const BEntry& entry)
{
	// get source and destination path
	BPath sourcePath;
	repository->GetPackagePath(package, sourcePath);

	BPath destinationPath;
	status_t error = entry.GetPath(&destinationPath);
	if (error != B_OK) {
		DIE(error, "Failed to entry path of package file to install \"%s\"",
			package->Info().FileName().String());
	}

	// Copy the package. Ideally we would just hard-link it, but BFS doesn't
	// support that.
	error = BCopyEngine().CopyEntry(sourcePath.Path(), destinationPath.Path());
	if (error != B_OK)
		DIE(error, "Failed to copy package file \"%s\"", sourcePath.Path());
}


int32
BPackageManager::_FindBasePackage(const PackageList& packages,
	const BPackageInfo& info)
{
	if (info.BasePackage().IsEmpty())
		return -1;

	// find the requirement matching the base package
	BPackageResolvableExpression* basePackage = NULL;
	int32 count = info.RequiresList().CountItems();
	for (int32 i = 0; i < count; i++) {
		BPackageResolvableExpression* requires = info.RequiresList().ItemAt(i);
		if (requires->Name() == info.BasePackage()) {
			basePackage = requires;
			break;
		}
	}

	if (basePackage == NULL) {
		fUserInteractionHandler->Warn(B_OK, B_TRANSLATE("Package %s-%s "
			"doesn't have a matching requires for its base package \"%s\""),
			info.Name().String(), info.Version().ToString().String(),
			info.BasePackage().String());
		return -1;
	}

	// find the first package matching the base package requires
	count = packages.CountItems();
	for (int32 i = 0; i < count; i++) {
		BSolverPackage* package = packages.ItemAt(i);
		if (package->Name() == basePackage->Name()
			&& package->Info().Matches(*basePackage)) {
			return i;
		}
	}

	return -1;
}


void
BPackageManager::_AddInstalledRepository(InstalledRepository* repository)
{
	fInstallationInterface->InitInstalledRepository(*repository);

	BRepositoryBuilder(*repository)
		.AddToSolver(fSolver, repository->Location() == fLocation);
	repository->SetPriority(repository->InitialPriority());

	if (!fInstalledRepositories.AddItem(repository))
		throw std::bad_alloc();
}


void
BPackageManager::_AddRemoteRepository(BPackageRoster& roster, const char* name,
	bool refresh)
{
	BRepositoryConfig config;
	status_t error = roster.GetRepositoryConfig(name, &config);
	if (error != B_OK) {
		fUserInteractionHandler->Warn(error, B_TRANSLATE(
			"Failed to get config for repository \"%s\". Skipping."), name);
		return;
	}

	BRepositoryCache cache;
	error = _GetRepositoryCache(roster, config, refresh, cache);
	if (error != B_OK) {
		fUserInteractionHandler->Warn(error, B_TRANSLATE(
			"Failed to get cache for repository \"%s\". Skipping."), name);
		return;
	}

	RemoteRepository* repository = new RemoteRepository(config);
	if (!fOtherRepositories.AddItem(repository)) {
		delete repository;
		throw std::bad_alloc();
	}

	BRepositoryBuilder(*repository, cache, config.Name())
		.AddToSolver(fSolver, false);
}


status_t
BPackageManager::_GetRepositoryCache(BPackageRoster& roster,
	const BRepositoryConfig& config, bool refresh, BRepositoryCache& _cache)
{
	if (!refresh && roster.GetRepositoryCache(config.Name(), &_cache) == B_OK)
		return B_OK;

	status_t error = RefreshRepository(config);
	if (error != B_OK) {
		fUserInteractionHandler->Warn(error, B_TRANSLATE(
			"Refreshing repository \"%s\" failed"), config.Name().String());
	}

	return roster.GetRepositoryCache(config.Name(), &_cache);
}


void
BPackageManager::_AddPackageSpecifiers(const char* const* searchStrings,
	int searchStringCount, BSolverPackageSpecifierList& specifierList)
{
	for (int i = 0; i < searchStringCount; i++) {
		const char* searchString = searchStrings[i];
		if (_IsLocalPackage(searchString)) {
			BSolverPackage* package = _AddLocalPackage(searchString);
			if (!specifierList.AppendSpecifier(package))
				throw std::bad_alloc();
		} else {
			if (!specifierList.AppendSpecifier(searchString))
				throw std::bad_alloc();
		}
	}
}


bool
BPackageManager::_IsLocalPackage(const char* fileName)
{
	// Simple heuristic: fileName contains ".hpkg" and there's actually a file
	// it refers to.
	struct stat st;
	return strstr(fileName, ".hpkg") != NULL && stat(fileName, &st) == 0
		&& S_ISREG(st.st_mode);
}


BSolverPackage*
BPackageManager::_AddLocalPackage(const char* fileName)
{
	if (fLocalRepository == NULL)
		throw std::bad_alloc();
	return fLocalRepository->AddLocalPackage(fileName);
}


bool
BPackageManager::_NextSpecificInstallationLocation()
{
	try {
		if (fLocation == B_PACKAGE_INSTALLATION_LOCATION_SYSTEM) {
			fLocation = B_PACKAGE_INSTALLATION_LOCATION_HOME;
			fSystemRepository->SetInstalled(false);
			_AddInstalledRepository(fHomeRepository);
			return true;
		}
	} catch (BFatalErrorException& e) {
		// No home repo. This is acceptable for example when we are in an haikuporter chroot.
	}

	return false;
}


status_t
BPackageManager::DownloadPackage(const BString& fileURL,
	const BEntry& targetEntry, const BString& checksum)
{
	BDecisionProvider provider;
	BContext context(provider, *this);
	return DownloadFileRequest(context, fileURL, targetEntry, checksum)
		.Process();
}


status_t
BPackageManager::RefreshRepository(const BRepositoryConfig& repoConfig)
{
	BDecisionProvider provider;
	BContext context(provider, *this);
	return BRefreshRepositoryRequest(context, repoConfig).Process();
}


// #pragma mark - RemoteRepository


BPackageManager::RemoteRepository::RemoteRepository(
	const BRepositoryConfig& config)
	:
	BSolverRepository(),
	fConfig(config)
{
}


const BRepositoryConfig&
BPackageManager::RemoteRepository::Config() const
{
	return fConfig;
}


// #pragma mark - LocalRepository


BPackageManager::LocalRepository::LocalRepository()
	:
	BSolverRepository()
{
}


BPackageManager::LocalRepository::LocalRepository(const BString& name)
	:
	BSolverRepository(name)
{
}


// #pragma mark - MiscLocalRepository


BPackageManager::MiscLocalRepository::MiscLocalRepository()
	:
	LocalRepository("local"),
	fPackagePaths()
{
	SetPriority(-127);
}


BSolverPackage*
BPackageManager::MiscLocalRepository::AddLocalPackage(const char* fileName)
{
	BSolverPackage* package;
	BRepositoryBuilder(*this).AddPackage(fileName, &package);

	fPackagePaths[package] = fileName;

	return package;
}


void
BPackageManager::MiscLocalRepository::GetPackagePath(BSolverPackage* package,
	BPath& _path)
{
	PackagePathMap::const_iterator it = fPackagePaths.find(package);
	if (it == fPackagePaths.end()) {
		DIE("Package %s not in local repository",
			package->VersionedName().String());
	}

	status_t error = _path.SetTo(it->second.c_str());
	if (error != B_OK)
		DIE(error, "Failed to init package path %s", it->second.c_str());
}


// #pragma mark - InstalledRepository


BPackageManager::InstalledRepository::InstalledRepository(const char* name,
	BPackageInstallationLocation location, int32 priority)
	:
	LocalRepository(),
	fDisabledPackages(10, true),
	fPackagesToActivate(),
	fPackagesToDeactivate(),
	fInitialName(name),
	fLocation(location),
	fInitialPriority(priority)
{
}


void
BPackageManager::InstalledRepository::GetPackagePath(BSolverPackage* package,
	BPath& _path)
{
	directory_which packagesWhich;
	switch (fLocation) {
		case B_PACKAGE_INSTALLATION_LOCATION_SYSTEM:
			packagesWhich = B_SYSTEM_PACKAGES_DIRECTORY;
			break;
		case B_PACKAGE_INSTALLATION_LOCATION_HOME:
			packagesWhich = B_USER_PACKAGES_DIRECTORY;
			break;
		default:
			DIE("Don't know packages directory path for installation location "
				"\"%s\"", Name().String());
	}

	BString fileName(package->Info().FileName());
	status_t error = find_directory(packagesWhich, &_path);
	if (error != B_OK || (error = _path.Append(fileName)) != B_OK) {
		DIE(error, "Failed to get path of package file \"%s\" in installation "
			"location \"%s\"", fileName.String(), Name().String());
	}
}


void
BPackageManager::InstalledRepository::DisablePackage(BSolverPackage* package)
{
	if (fDisabledPackages.HasItem(package))
		DIE("Package %s already disabled", package->VersionedName().String());

	if (package->Repository() != this) {
		DIE("Package %s not in repository %s",
			package->VersionedName().String(), Name().String());
	}

	// move to disabled list
	if (!fDisabledPackages.AddItem(package))
		throw std::bad_alloc();

	RemovePackage(package);
}


bool
BPackageManager::InstalledRepository::EnablePackage(BSolverPackage* package)
{
	return fDisabledPackages.RemoveItem(package);
}


bool
BPackageManager::InstalledRepository::HasChanges() const
{
	return !fPackagesToActivate.IsEmpty() || !fPackagesToDeactivate.IsEmpty();
}


void
BPackageManager::InstalledRepository::ApplyChanges()
{
	// disable packages to deactivate
	for (int32 i = 0; BSolverPackage* package = fPackagesToDeactivate.ItemAt(i);
		i++) {
		if (!fDisabledPackages.HasItem(package))
			DisablePackage(package);
	}

	// add packages to activate
	for (int32 i = 0; BSolverPackage* package = fPackagesToActivate.ItemAt(i);
		i++) {
		status_t error = AddPackage(package->Info());
		if (error != B_OK) {
			DIE(error, "Failed to add package %s to %s repository",
				package->Name().String(), Name().String());
		}
	}
}


// #pragma mark - Transaction


BPackageManager::Transaction::Transaction(InstalledRepository& repository)
	:
	fRepository(repository),
	fTransaction(),
	fTransactionDirectory()
{
}


BPackageManager::Transaction::~Transaction()
{
}


// #pragma mark - InstallationInterface


BPackageManager::InstallationInterface::~InstallationInterface()
{
}


void
BPackageManager::InstallationInterface::ResultComputed(
	InstalledRepository& repository)
{
}


// #pragma mark - ClientInstallationInterface


BPackageManager::ClientInstallationInterface::ClientInstallationInterface()
	:
	fDaemonClient()
{
}


BPackageManager::ClientInstallationInterface::~ClientInstallationInterface()
{
}


void
BPackageManager::ClientInstallationInterface::InitInstalledRepository(
	InstalledRepository& repository)
{
	const char* name = repository.InitialName();
	BRepositoryBuilder(repository, name)
		.AddPackages(repository.Location(), name);
}


status_t
BPackageManager::ClientInstallationInterface::PrepareTransaction(
	Transaction& transaction)
{
	return fDaemonClient.CreateTransaction(transaction.Repository().Location(),
		transaction.ActivationTransaction(),
		transaction.TransactionDirectory());
}


status_t
BPackageManager::ClientInstallationInterface::CommitTransaction(
	Transaction& transaction, BCommitTransactionResult& _result)
{
	return fDaemonClient.CommitTransaction(transaction.ActivationTransaction(),
		_result);
}


// #pragma mark - UserInteractionHandler


BPackageManager::UserInteractionHandler::~UserInteractionHandler()
{
}


void
BPackageManager::UserInteractionHandler::HandleProblems()
{
	throw BAbortedByUserException();
}


void
BPackageManager::UserInteractionHandler::ConfirmChanges(bool fromMostSpecific)
{
	throw BAbortedByUserException();
}


void
BPackageManager::UserInteractionHandler::Warn(status_t error,
	const char* format, ...)
{
}


void
BPackageManager::UserInteractionHandler::ProgressPackageDownloadStarted(
	const char* packageName)
{
}


void
BPackageManager::UserInteractionHandler::ProgressPackageDownloadActive(
	const char* packageName, float completionPercentage, off_t bytes,
	off_t totalBytes)
{
}


void
BPackageManager::UserInteractionHandler::ProgressPackageDownloadComplete(
	const char* packageName)
{
}


void
BPackageManager::UserInteractionHandler::ProgressPackageChecksumStarted(
	const char* title)
{
}


void
BPackageManager::UserInteractionHandler::ProgressPackageChecksumComplete(
	const char* title)
{
}


void
BPackageManager::UserInteractionHandler::ProgressStartApplyingChanges(
	InstalledRepository& repository)
{
}


void
BPackageManager::UserInteractionHandler::ProgressTransactionCommitted(
	InstalledRepository& repository, const BCommitTransactionResult& result)
{
}


void
BPackageManager::UserInteractionHandler::ProgressApplyingChangesDone(
	InstalledRepository& repository)
{
}


}	// namespace BPrivate

}	// namespace BManager

}	// namespace BPackageKit
