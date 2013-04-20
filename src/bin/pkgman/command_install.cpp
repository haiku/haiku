/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include <Directory.h>
#include <package/DownloadFileRequest.h>
#include <package/PackageRoster.h>
#include <package/RepositoryConfig.h>
#include <package/solver/SolverPackageSpecifier.h>
#include <package/solver/SolverPackageSpecifierList.h>
#include <package/solver/SolverProblem.h>
#include <package/solver/SolverProblemSolution.h>
#include <package/solver/SolverResult.h>

#include <AutoDeleter.h>
#include <package/ActivationTransaction.h>
#include <package/DaemonClient.h>

#include "Command.h"
#include "DecisionProvider.h"
#include "JobStateListener.h"
#include "pkgman.h"
#include "RepositoryBuilder.h"


// TODO: internationalization!


using namespace BPackageKit;
using namespace BPackageKit::BPrivate;


static const char* const kShortUsage =
	"  %command% <package> ...\n"
	"    Installs one or more packages.\n";

static const char* const kLongUsage =
	"Usage: %program% %command% <package> ...\n"
	"Installs the specified packages.\n"
	"\n"
	"Options:\n"
	"  -H, --home\n"
	"    Install the packages in the user's home directory. Default is to.\n"
	"    install in the common directory.\n"
	"\n";


DEFINE_COMMAND(InstallCommand, "install", kShortUsage, kLongUsage)


struct Repository : public BSolverRepository {
	Repository()
		:
		BSolverRepository()
	{
	}

	status_t Init(BPackageRoster& roster, const char* name)
	{
		return roster.GetRepositoryConfig(name, &fConfig);
	}

	const BRepositoryConfig& Config() const
	{
		return fConfig;
	}

private:
	BRepositoryConfig	fConfig;
};


int
InstallCommand::Execute(int argc, const char* const* argv)
{
	bool installInHome = false;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ "home", no_argument, 0, 'H' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "hu", sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'h':
				PrintUsageAndExit(false);
				break;

			case 'H':
				installInHome = true;
				break;

			default:
				PrintUsageAndExit(true);
				break;
		}
	}

	// The remaining arguments are the packages to be installed.
	if (argc < optind + 1)
		PrintUsageAndExit(true);

	int packageCount = argc - optind;
	const char* const* packages = argv + optind;

// TODO: Refresh repositories.

	// create the solver
	BSolver* solver;
	status_t error = BSolver::Create(solver);
	if (error != B_OK)
		DIE(error, "failed to create solver");

	// add repositories

	// We add only the repository of our actual installation location as the
	// "installed" repository. The repositories for the more general
	// installation locations are added as regular repositories, but with better
	// priorities than the actual (remote) repositories. This prevents the solver
	// from showing conflicts when a package in a more specific installation
	// location overrides a package in a more general one. Instead any
	// requirement that is already installed in a more general installation
	// location will turn up as to be installed as well. But we can easily
	// filter those out.
	BSolverRepository systemRepository;
	RepositoryBuilder(systemRepository, "system")
		.AddPackages(B_PACKAGE_INSTALLATION_LOCATION_SYSTEM, "system")
		.AddToSolver(solver, false);
	systemRepository.SetPriority(-1);

	BSolverRepository commonRepository;
	RepositoryBuilder(commonRepository, "common")
		.AddPackages(B_PACKAGE_INSTALLATION_LOCATION_COMMON, "common")
		.AddToSolver(solver, !installInHome);

	BObjectList<BSolverRepository> installedRepositories(10);
	if (!installedRepositories.AddItem(&systemRepository)
		|| !installedRepositories.AddItem(&commonRepository)) {
		DIE(B_NO_MEMORY, "failed to add installed repositories to list");
	}

	BSolverRepository homeRepository;
	if (installInHome) {
		commonRepository.SetPriority(-2);
		RepositoryBuilder(homeRepository, "home")
			.AddPackages(B_PACKAGE_INSTALLATION_LOCATION_HOME, "home")
			.AddToSolver(solver, true);

		if (!installedRepositories.AddItem(&homeRepository))
			DIE(B_NO_MEMORY, "failed to add home repository to list");
	}

	// other repositories
	BObjectList<Repository> otherRepositories(10, true);
	BPackageRoster roster;
	BStringList repositoryNames;
	error = roster.GetRepositoryNames(repositoryNames);
	if (error != B_OK)
		WARN(error, "failed to get repository names");

	int32 repositoryNameCount = repositoryNames.CountStrings();
	for (int32 i = 0; i < repositoryNameCount; i++) {
		Repository* repository = new(std::nothrow) Repository;
		if (repository == NULL || !otherRepositories.AddItem(repository))
			DIE(B_NO_MEMORY, "out of memory");

		const BString& name = repositoryNames.StringAt(i);
		error = repository->Init(roster, name);
		if (error != B_OK) {
			WARN(error, "failed to get config for repository \"%s\". Skipping.",
				name.String());
			otherRepositories.RemoveItem(repository, true);
			continue;
		}

		RepositoryBuilder(*repository, repository->Config())
			.AddToSolver(solver, false);
	}

	// solve
	BSolverPackageSpecifierList packagesToInstall;
	for (int i = 0; i < packageCount; i++) {
		if (!packagesToInstall.AppendSpecifier(packages[i]))
			DIE(B_NO_MEMORY, "failed to add specified package");
	}

	const BSolverPackageSpecifier* unmatchedSpecifier;
	error = solver->Install(packagesToInstall, &unmatchedSpecifier);
	if (error != B_OK) {
		if (unmatchedSpecifier != NULL) {
			DIE(error, "failed to find a match for \"%s\"",
				unmatchedSpecifier->SelectString().String());
		} else
			DIE(error, "failed to compute packages to install");
	}

	// deal with problems
	while (solver->HasProblems()) {
		printf("Encountered problems:\n");

		int32 problemCount = solver->CountProblems();
		for (int32 i = 0; i < problemCount; i++) {
			BSolverProblem* problem = solver->ProblemAt(i);
			printf("  %" B_PRId32 ": %s\n", i + 1,
				problem->ToString().String());

			int32 solutionCount = problem->CountSolutions();
			for (int32 k = 0; k < solutionCount; k++) {
				const BSolverProblemSolution* solution = problem->SolutionAt(k);
				printf("    solution %" B_PRId32 ":\n", k + 1);
				int32 elementCount = solution->CountElements();
				for (int32 l = 0; l < elementCount; l++) {
					const BSolverProblemSolutionElement* element
						= solution->ElementAt(l);
					printf("      - %s\n", element->ToString().String());
				}
			}
		}
// TODO: Allow the user to select solutions!
exit(1);
	}

	// print result
	BSolverResult result;
	error = solver->GetResult(result);
	if (error != B_OK)
		DIE(error, "failed to compute packages to install");

	printf("The following changes will be made:\n");
	for (int32 i = 0; const BSolverResultElement* element = result.ElementAt(i);
			i++) {
		BSolverPackage* package = element->Package();

		switch (element->Type()) {
			case BSolverResultElement::B_TYPE_INSTALL:
				if (installedRepositories.HasItem(package->Repository()))
					continue;

				printf("  install package %s from repository %s\n",
					package->Info().CanonicalFileName().String(),
					package->Repository()->Name().String());
				break;

			case BSolverResultElement::B_TYPE_UNINSTALL:
				printf("  uninstall package %s\n",
					package->VersionedName().String());
				break;
		}
	}
// TODO: Print file/download sizes. Unfortunately our package infos don't
// contain the file size. Which is probably correct. The file size (and possibly
// other information) should, however, be provided by the repository cache in
// some way. Extend BPackageInfo? Create a BPackageFileInfo?

	DecisionProvider decisionProvider;
	if (!decisionProvider.YesNoDecisionNeeded(BString(), "Continue?", "y", "n",
			"y")) {
		return 1;
	}

	// create an activation transaction
	BDaemonClient daemonClient;
	BPackageInstallationLocation location = installInHome
		? B_PACKAGE_INSTALLATION_LOCATION_HOME
		: B_PACKAGE_INSTALLATION_LOCATION_COMMON;
	BActivationTransaction transaction;
	BDirectory transactionDirectory;
	error = daemonClient.CreateTransaction(location, transaction,
		transactionDirectory);
	if (error != B_OK)
		DIE(error, "failed to create transaction");

	// download the new packages and prepare the transaction
	JobStateListener listener;
	BContext context(decisionProvider, listener);

	for (int32 i = 0; const BSolverResultElement* element = result.ElementAt(i);
			i++) {
		BSolverPackage* package = element->Package();

		switch (element->Type()) {
			case BSolverResultElement::B_TYPE_INSTALL:
			{
				if (installedRepositories.HasItem(package->Repository()))
					continue;

				// get package URL and target entry
				Repository* repository
					= static_cast<Repository*>(package->Repository());
				BString url = repository->Config().BaseURL();
				BString fileName(package->Info().CanonicalFileName());
				if (fileName.IsEmpty())
					DIE(B_NO_MEMORY, "out of memory");
				url << '/' << fileName;

				BEntry entry;
				error = entry.SetTo(&transactionDirectory, fileName);
				if (error != B_OK)
					DIE(error, "failed to create package entry");

				// download the package
				DownloadFileRequest downloadRequest(context, url, entry,
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
				break;
			}

			case BSolverResultElement::B_TYPE_UNINSTALL:
				// add package to transaction
				if (!transaction.AddPackageToDeactivate(
						package->Info().CanonicalFileName())) {
					DIE(B_NO_MEMORY,
						"failed to add package to deactivate to transaction");
				}
				break;
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

	return 0;
}
