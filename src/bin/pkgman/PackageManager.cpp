/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include "PackageManager.h"

#include <Directory.h>
#include <package/DownloadFileRequest.h>
#include <package/PackageRoster.h>
#include <package/RefreshRepositoryRequest.h>
#include <package/solver/SolverPackage.h>
#include <package/solver/SolverPackageSpecifier.h>
#include <package/solver/SolverPackageSpecifierList.h>
#include <package/solver/SolverProblem.h>
#include <package/solver/SolverProblemSolution.h>
#include <package/solver/SolverResult.h>

#include <CopyEngine.h>
#include <package/ActivationTransaction.h>
#include <package/DaemonClient.h>

#include "pkgman.h"
#include "RepositoryBuilder.h"


using namespace BPackageKit::BPrivate;


// #pragma mark - RemoteRepository


PackageManager::RemoteRepository::RemoteRepository()
	:
	BSolverRepository()
{
}


status_t
PackageManager::RemoteRepository::Init(BPackageRoster& roster,
	BContext& context, const char* name, bool refresh)
{
	// get the repository config
	status_t error = roster.GetRepositoryConfig(name, &fConfig);
	if (error != B_OK)
		return error;

	// refresh
	if (!refresh)
		return B_OK;

	BRefreshRepositoryRequest refreshRequest(context, fConfig);
	error = refreshRequest.Process();
	if (error != B_OK) {
		WARN(error, "refreshing repository \"%s\" failed", name);
		return B_OK;
	}

	// re-get the config
	return roster.GetRepositoryConfig(name, &fConfig);
}


const BRepositoryConfig&
PackageManager::RemoteRepository::Config() const
{
	return fConfig;
}


// #pragma mark - InstalledRepository


PackageManager::InstalledRepository::InstalledRepository(const char* name,
	BPackageInstallationLocation location, int32 priority)
	:
	BSolverRepository(),
	fDisabledPackages(10, true),
	fInitialName(name),
	fLocation(location),
	fInitialPriority(priority)
{
}


void
PackageManager::InstalledRepository::DisablePackage(BSolverPackage* package)
{
	if (fDisabledPackages.HasItem(package)) {
		fprintf(stderr, "*** package %s already disabled\n",
			package->VersionedName().String());
		exit(1);
	}

	if (package->Repository() != this) {
		fprintf(stderr, "*** package %s not in repository %s\n",
			package->VersionedName().String(), Name().String());
		exit(1);
	}

	// move to disabled list
	if (!fDisabledPackages.AddItem(package))
		DIE(B_NO_MEMORY, "*** failed to add package to list");

	RemovePackage(package);
}


// #pragma mark - Solver


PackageManager::PackageManager(BPackageInstallationLocation location,
	uint32 flags)
	:
	fLocation(location),
	fSolver(NULL),
	fSystemRepository(new (std::nothrow) InstalledRepository("system",
		B_PACKAGE_INSTALLATION_LOCATION_SYSTEM, -1)),
	fCommonRepository(new (std::nothrow) InstalledRepository("common",
		B_PACKAGE_INSTALLATION_LOCATION_COMMON, -2)),
	fHomeRepository(new (std::nothrow) InstalledRepository("home",
		B_PACKAGE_INSTALLATION_LOCATION_HOME, -3)),
	fInstalledRepositories(10),
	fOtherRepositories(10, true),
	fDecisionProvider(),
	fJobStateListener(JobStateListener::EXIT_ON_ABORT),
	fContext(fDecisionProvider, fJobStateListener)
{
	// create the solver
	status_t error = BSolver::Create(fSolver);
	if (error != B_OK)
		DIE(error, "failed to create solver");

	if (fSystemRepository == NULL || fCommonRepository == NULL
		|| fHomeRepository == NULL) {
		DIE(B_NO_MEMORY, "failed to create repositories");
	}

	// add installation location repositories
	if ((flags & ADD_INSTALLED_REPOSITORIES) != 0) {
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
			_AddInstalledRepository(fCommonRepository);

			if (!fCommonRepository->IsInstalled())
				_AddInstalledRepository(fHomeRepository);
		}
	}

	// add other repositories
	if ((flags & ADD_REMOTE_REPOSITORIES) != 0) {
		BPackageRoster roster;
		BStringList repositoryNames;
		error = roster.GetRepositoryNames(repositoryNames);
		if (error != B_OK)
			WARN(error, "failed to get repository names");
	
		int32 repositoryNameCount = repositoryNames.CountStrings();
		for (int32 i = 0; i < repositoryNameCount; i++) {
			RemoteRepository* repository = new(std::nothrow) RemoteRepository;
			if (repository == NULL || !fOtherRepositories.AddItem(repository))
				DIE(B_NO_MEMORY, "failed to create/add repository object");
	
			const BString& name = repositoryNames.StringAt(i);
			error = repository->Init(roster, fContext, name,
				(flags & REFRESH_REPOSITORIES) != 0);
			if (error != B_OK) {
				WARN(error,
					"failed to get config for repository \"%s\". Skipping.",
					name.String());
				fOtherRepositories.RemoveItem(repository, true);
				continue;
			}
	
			RepositoryBuilder(*repository, repository->Config())
				.AddToSolver(fSolver, false);
		}
	}
}


PackageManager::~PackageManager()
{
	delete fSystemRepository;
	delete fCommonRepository;
	delete fHomeRepository;
}


void
PackageManager::Install(const char* const* packages, int packageCount)
{
	// solve
	BSolverPackageSpecifierList packagesToInstall;
	for (int i = 0; i < packageCount; i++) {
		if (!packagesToInstall.AppendSpecifier(packages[i]))
			DIE(B_NO_MEMORY, "failed to add specified package");
	}

	const BSolverPackageSpecifier* unmatchedSpecifier;
	status_t error = fSolver->Install(packagesToInstall, &unmatchedSpecifier);
	if (error != B_OK) {
		if (unmatchedSpecifier != NULL) {
			DIE(error, "failed to find a match for \"%s\"",
				unmatchedSpecifier->SelectString().String());
		} else
			DIE(error, "failed to compute packages to install");
	}

	_HandleProblems();

	// install/uninstall packages
	_AnalyzeResult();
	_PrintResult();
	_ApplyPackageChanges();
}


void
PackageManager::Uninstall(const char* const* packages, int packageCount)
{
	// find the packages that match the specification
	BSolverPackageSpecifierList packagesToUninstall;
	for (int i = 0; i < packageCount; i++) {
		if (!packagesToUninstall.AppendSpecifier(packages[i]))
			DIE(B_NO_MEMORY, "failed to add specified package");
	}

	const BSolverPackageSpecifier* unmatchedSpecifier;
	PackageList foundPackages;
	status_t error = fSolver->FindPackages(packagesToUninstall,
		BSolver::B_FIND_INSTALLED_ONLY, foundPackages, &unmatchedSpecifier);
	if (error != B_OK) {
		if (unmatchedSpecifier != NULL) {
			DIE(error, "failed to find a match for \"%s\"",
				unmatchedSpecifier->SelectString().String());
		} else
			DIE(error, "failed to compute packages to uninstall");
	}

	// determine the inverse base package closure for the found packages
	InstalledRepository* installationRepository
		= dynamic_cast<InstalledRepository*>(
			foundPackages.ItemAt(0)->Repository());
	bool foundAnotherPackage;
	do {
		foundAnotherPackage = false;
		int32 count = installationRepository->CountPackages();
		for (int32 i = 0; i < count; i++) {
			BSolverPackage* package = installationRepository->PackageAt(i);
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
		installationRepository->DisablePackage(package);

	error = fSolver->VerifyInstallation(BSolver::B_VERIFY_ALLOW_UNINSTALL);
	if (error != B_OK)
		DIE(error, "failed to compute packages to uninstall");

	_HandleProblems();

	// install/uninstall packages
	_AnalyzeResult();

	for (int32 i = foundPackages.CountItems() - 1; i >= 0; i--) {
		if (!fPackagesToDeactivate.AddItem(foundPackages.ItemAt(i)))
			DIE(B_NO_MEMORY, "failed to add package to uninstall");
	}

	_PrintResult();
	_ApplyPackageChanges();
}


void
PackageManager::Update(const char* const* packages, int packageCount)
{
	// solve
	BSolverPackageSpecifierList packagesToUpdate;
	for (int i = 0; i < packageCount; i++) {
		if (!packagesToUpdate.AppendSpecifier(packages[i]))
			DIE(B_NO_MEMORY, "failed to add specified package");
	}

	const BSolverPackageSpecifier* unmatchedSpecifier;
	status_t error = fSolver->Update(packagesToUpdate, true,
		&unmatchedSpecifier);
	if (error != B_OK) {
		if (unmatchedSpecifier != NULL) {
			DIE(error, "failed to find a match for \"%s\"",
				unmatchedSpecifier->SelectString().String());
		} else
			DIE(error, "failed to compute packages to update");
	}

	_HandleProblems();

	// install/uninstall packages
	_AnalyzeResult();
	_PrintResult();
	_ApplyPackageChanges();
}


void
PackageManager::_HandleProblems()
{
	while (fSolver->HasProblems()) {
		printf("Encountered problems:\n");

		int32 problemCount = fSolver->CountProblems();
		for (int32 i = 0; i < problemCount; i++) {
			// print problem and possible solutions
			BSolverProblem* problem = fSolver->ProblemAt(i);
			printf("problem %" B_PRId32 ": %s\n", i + 1,
				problem->ToString().String());

			int32 solutionCount = problem->CountSolutions();
			for (int32 k = 0; k < solutionCount; k++) {
				const BSolverProblemSolution* solution = problem->SolutionAt(k);
				printf("  solution %" B_PRId32 ":\n", k + 1);
				int32 elementCount = solution->CountElements();
				for (int32 l = 0; l < elementCount; l++) {
					const BSolverProblemSolutionElement* element
						= solution->ElementAt(l);
					printf("    - %s\n", element->ToString().String());
				}
			}

			// let the user choose a solution
			printf("Please select a solution, skip the problem for now or "
				"quit.\n");
			for (;;) {
				if (solutionCount > 1)
					printf("select [1...%" B_PRId32 "/s/q]: ", solutionCount);
				else
					printf("select [1/s/q]: ");
	
				char buffer[32];
				if (fgets(buffer, sizeof(buffer), stdin) == NULL
					|| strcmp(buffer, "q\n") == 0) {
					exit(1);
				}

				if (strcmp(buffer, "s\n") == 0)
					break;

				char* end;
				long selected = strtol(buffer, &end, 0);
				if (end == buffer || *end != '\n' || selected < 1
					|| selected > solutionCount) {
					printf("*** invalid input\n");
					continue;
				}

				status_t error = fSolver->SelectProblemSolution(problem,
					problem->SolutionAt(selected - 1));
				if (error != B_OK)
					DIE(error, "failed to set solution");
				break;
			}
		}

		status_t error = fSolver->SolveAgain();
		if (error != B_OK)
			DIE(error, "failed to recompute packages to un/-install");
	}
}


void
PackageManager::_AnalyzeResult()
{
	BSolverResult result;
	status_t error = fSolver->GetResult(result);
	if (error != B_OK)
		DIE(error, "failed to compute packages to un/-install");

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
						: fPackagesToActivate;
				if (!packageList.AddItem(package))
					DIE(B_NO_MEMORY, "failed to add package to activate");
				break;
			}

			case BSolverResultElement::B_TYPE_UNINSTALL:
				if (!fPackagesToDeactivate.AddItem(package))
					DIE(B_NO_MEMORY, "failed to add package to deactivate");
				break;
		}
	}

	// Make sure base packages are installed in the same location.
	for (int32 i = 0; i < fPackagesToActivate.CountItems(); i++) {
		BSolverPackage* package = fPackagesToActivate.ItemAt(i);
		int32 index = _FindBasePackage(potentialBasePackages, package->Info());
		if (index < 0)
			continue;

		BSolverPackage* basePackage = potentialBasePackages.RemoveItemAt(index);
		if (!fPackagesToActivate.AddItem(basePackage))
			DIE(B_NO_MEMORY, "failed to add package to activate");
	}
}


void
PackageManager::_PrintResult()
{
	if (fPackagesToActivate.IsEmpty() && fPackagesToDeactivate.IsEmpty()) {
		printf("Nothing to do.\n");
		exit(0);
	}

	printf("The following changes will be made:\n");
	for (int32 i = 0; BSolverPackage* package = fPackagesToActivate.ItemAt(i);
		i++) {
		printf("  install package %s from repository %s\n",
			package->Info().CanonicalFileName().String(),
			package->Repository()->Name().String());
	}

	for (int32 i = 0; BSolverPackage* package = fPackagesToDeactivate.ItemAt(i);
		i++) {
		printf("  uninstall package %s\n", package->VersionedName().String());
	}
// TODO: Print file/download sizes. Unfortunately our package infos don't
// contain the file size. Which is probably correct. The file size (and possibly
// other information) should, however, be provided by the repository cache in
// some way. Extend BPackageInfo? Create a BPackageFileInfo?

	if (!fDecisionProvider.YesNoDecisionNeeded(BString(), "Continue?", "y", "n",
			"y")) {
		exit(1);
	}
}


void
PackageManager::_ApplyPackageChanges()
{
	// create an activation transaction
	BDaemonClient daemonClient;
	BActivationTransaction transaction;
	BDirectory transactionDirectory;
	status_t error = daemonClient.CreateTransaction(fLocation, transaction,
		transactionDirectory);
	if (error != B_OK)
		DIE(error, "failed to create transaction");

	// download the new packages and prepare the transaction
	for (int32 i = 0; BSolverPackage* package = fPackagesToActivate.ItemAt(i);
		i++) {
		// get package URL and target entry

		BString fileName(package->Info().CanonicalFileName());
		if (fileName.IsEmpty())
			DIE(B_NO_MEMORY, "failed to allocate file name");

		BEntry entry;
		error = entry.SetTo(&transactionDirectory, fileName);
		if (error != B_OK)
			DIE(error, "failed to create package entry");

		RemoteRepository* remoteRepository
			= dynamic_cast<RemoteRepository*>(package->Repository());
		if (remoteRepository == NULL) {
			// clone the existing package
			_ClonePackageFile(
				dynamic_cast<InstalledRepository*>(package->Repository()),
				fileName, entry);
		} else {
			// download the package
			BString url = remoteRepository->Config().PackagesURL();
			url << '/' << fileName;

			DownloadFileRequest downloadRequest(fContext, url, entry,
				package->Info().Checksum());
			error = downloadRequest.Process();
			if (error != B_OK)
				DIE(error, "failed to download package");
		}

		// add package to transaction
		if (!transaction.AddPackageToActivate(fileName)) {
			DIE(B_NO_MEMORY,
				"failed to add package to activate to transaction");
		}
	}

	for (int32 i = 0; BSolverPackage* package = fPackagesToDeactivate.ItemAt(i);
		i++) {
		// add package to transaction
		if (!transaction.AddPackageToDeactivate(
				package->Info().CanonicalFileName())) {
			DIE(B_NO_MEMORY,
				"failed to add package to deactivate to transaction");
		}
	}

	// commit the transaction
	BDaemonClient::BCommitTransactionResult transactionResult;
	error = daemonClient.CommitTransaction(transaction, transactionResult);
	if (error != B_OK) {
		fprintf(stderr, "*** failed to commit transaction: %s\n",
			transactionResult.FullErrorMessage().String());
		exit(1);
	}

	printf("Installation done. Old activation state backed up in \"%s\"\n",
		transactionResult.OldStateDirectory().String());

	printf("Cleaning up ...\n");
	BEntry transactionDirectoryEntry;
	if ((error = transactionDirectory.GetEntry(&transactionDirectoryEntry))
			!= B_OK
		|| (error = transactionDirectoryEntry.Remove()) != B_OK) {
		WARN(error, "failed to remove transaction directory");
	}
}


void
PackageManager::_ClonePackageFile(InstalledRepository* repository,
	const BString& fileName, const BEntry& entry) const
{
	// get the source and destination file paths
	directory_which packagesWhich;
	if (repository == fSystemRepository) {
		packagesWhich = B_SYSTEM_PACKAGES_DIRECTORY;
	} else if (repository == fCommonRepository) {
		packagesWhich = B_COMMON_PACKAGES_DIRECTORY;
	} else {
		fprintf(stderr, "*** don't know packages directory path for "
			"installation location \"%s\"", repository->Name().String());
		exit(1);
	}

	BPath sourcePath;
	status_t error = find_directory(packagesWhich, &sourcePath);
	if (error != B_OK || (error = sourcePath.Append(fileName)) != B_OK) {
		DIE(error, "failed to get path of package file \"%s\" in installation "
			"location \"%s\"", fileName.String(), repository->Name().String());
	}

	BPath destinationPath;
	error = entry.GetPath(&destinationPath);
	if (error != B_OK) {
		DIE(error, "failed to entry path of package file to install \"%s\"",
			fileName.String());
	}

	// Copy the package. Ideally we would just hard-link it, but BFS doesn't
	// support that.
	error = BCopyEngine().CopyEntry(sourcePath.Path(), destinationPath.Path());
	if (error != B_OK)
		DIE(error, "failed to copy package file \"%s\"", sourcePath.Path());
}


int32
PackageManager::_FindBasePackage(const PackageList& packages,
	const BPackageInfo& info) const
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
		printf("warning: package %s-%s doesn't have a matching requires for "
			"its base package \"%s\"\n", info.Name().String(),
			info.Version().ToString().String(), info.BasePackage().String());
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
PackageManager::_AddInstalledRepository(InstalledRepository* repository)
{
	const char* name = repository->InitialName();
	RepositoryBuilder(*repository, name)
		.AddPackages(repository->Location(), name)
		.AddToSolver(fSolver, repository->Location() == fLocation);
	repository->SetPriority(repository->InitialPriority());

	if (!fInstalledRepositories.AddItem(repository))
		DIE(B_NO_MEMORY, "failed to add %s repository to list", name);
}
