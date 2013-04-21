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

#include <package/ActivationTransaction.h>
#include <package/DaemonClient.h>

#include "pkgman.h"
#include "RepositoryBuilder.h"


using namespace BPackageKit::BPrivate;


// #pragma mark - Repository


PackageManager::Repository::Repository()
	:
	BSolverRepository()
{
}


status_t
PackageManager::Repository::Init(BPackageRoster& roster, BContext& context,
	const char* name)
{
	// get the repository config
	status_t error = roster.GetRepositoryConfig(name, &fConfig);
	if (error != B_OK)
		return error;

	// refresh
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
PackageManager::Repository::Config() const
{
	return fConfig;
}


// #pragma mark - Solver


PackageManager::PackageManager(BPackageInstallationLocation location,
	bool addInstalledRepositories, bool addOtherRepositories)
	:
	fLocation(location),
	fSolver(NULL),
	fSystemRepository(),
	fCommonRepository(),
	fHomeRepository(),
	fInstalledRepositories(10),
	fOtherRepositories(10, true),
	fDecisionProvider(),
	fJobStateListener(),
	fContext(fDecisionProvider, fJobStateListener)
{
	// create the solver
	status_t error = BSolver::Create(fSolver);
	if (error != B_OK)
		DIE(error, "failed to create solver");

	// add installation location repositories
	if (addInstalledRepositories) {
		// We add only the repository of our actual installation location as the
		// "installed" repository. The repositories for the more general
		// installation locations are added as regular repositories, but with
		// better priorities than the actual (remote) repositories. This
		// prevents the solver from showing conflicts when a package in a more
		// specific installation location overrides a package in a more general
		// one. Instead any requirement that is already installed in a more
		// general installation location will turn up as to be installed as
		// well. But we can easily filter those out.
		RepositoryBuilder(fSystemRepository, "system")
			.AddPackages(B_PACKAGE_INSTALLATION_LOCATION_SYSTEM, "system")
			.AddToSolver(fSolver, false);
		fSystemRepository.SetPriority(-1);

		bool installInHome = location == B_PACKAGE_INSTALLATION_LOCATION_HOME;
		RepositoryBuilder(fCommonRepository, "common")
			.AddPackages(B_PACKAGE_INSTALLATION_LOCATION_COMMON, "common")
			.AddToSolver(fSolver, !installInHome);
	
		if (!fInstalledRepositories.AddItem(&fSystemRepository)
			|| !fInstalledRepositories.AddItem(&fCommonRepository)) {
			DIE(B_NO_MEMORY, "failed to add installed repositories to list");
		}
	
		if (installInHome) {
			fCommonRepository.SetPriority(-2);
			RepositoryBuilder(fHomeRepository, "home")
				.AddPackages(B_PACKAGE_INSTALLATION_LOCATION_HOME, "home")
				.AddToSolver(fSolver, true);
	
			if (!fInstalledRepositories.AddItem(&fHomeRepository))
				DIE(B_NO_MEMORY, "failed to add home repository to list");
		}
	}

	// add other repositories
	if (addOtherRepositories) {
		BPackageRoster roster;
		BStringList repositoryNames;
		error = roster.GetRepositoryNames(repositoryNames);
		if (error != B_OK)
			WARN(error, "failed to get repository names");
	
		int32 repositoryNameCount = repositoryNames.CountStrings();
		for (int32 i = 0; i < repositoryNameCount; i++) {
			Repository* repository = new(std::nothrow) Repository;
			if (repository == NULL || !fOtherRepositories.AddItem(repository))
				DIE(B_NO_MEMORY, "failed to create/add repository object");
	
			const BString& name = repositoryNames.StringAt(i);
			error = repository->Init(roster, fContext, name);
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
	// solve
	BSolverPackageSpecifierList packagesToUninstall;
	for (int i = 0; i < packageCount; i++) {
		if (!packagesToUninstall.AppendSpecifier(packages[i]))
			DIE(B_NO_MEMORY, "failed to add specified package");
	}

	const BSolverPackageSpecifier* unmatchedSpecifier;
	status_t error = fSolver->Uninstall(packagesToUninstall,
		&unmatchedSpecifier);
	if (error != B_OK) {
		if (unmatchedSpecifier != NULL) {
			DIE(error, "failed to find a match for \"%s\"",
				unmatchedSpecifier->SelectString().String());
		} else
			DIE(error, "failed to compute packages to uninstall");
	}

	_HandleProblems();

	// install/uninstall packages
	_AnalyzeResult();
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

	for (int32 i = 0; const BSolverResultElement* element = result.ElementAt(i);
			i++) {
		BSolverPackage* package = element->Package();

		switch (element->Type()) {
			case BSolverResultElement::B_TYPE_INSTALL:
				if (!fInstalledRepositories.HasItem(package->Repository())) {
					if (!fPackagesToActivate.AddItem(package))
						DIE(B_NO_MEMORY, "failed to add package to activate");
				}
				break;

			case BSolverResultElement::B_TYPE_UNINSTALL:
				if (!fPackagesToDeactivate.AddItem(package))
					DIE(B_NO_MEMORY, "failed to add package to deactivate");
				break;
		}
	}

	if (fPackagesToActivate.IsEmpty() && fPackagesToDeactivate.IsEmpty()) {
		printf("Nothing to do.\n");
		exit(0);
	}
}


void
PackageManager::_PrintResult()
{
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
		Repository* repository
			= static_cast<Repository*>(package->Repository());
		BString url = repository->Config().BaseURL();
		BString fileName(package->Info().CanonicalFileName());
		if (fileName.IsEmpty())
			DIE(B_NO_MEMORY, "failed to allocate file name");
		url << '/' << fileName;

		BEntry entry;
		error = entry.SetTo(&transactionDirectory, fileName);
		if (error != B_OK)
			DIE(error, "failed to create package entry");

		// download the package
		DownloadFileRequest downloadRequest(fContext, url, entry,
			package->Info().Checksum());
		error = downloadRequest.Process();
		if (error != B_OK)
			DIE(error, "failed to download package");

		// add package to transaction
		if (!transaction.AddPackageToActivate(
				package->Info().CanonicalFileName())) {
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
