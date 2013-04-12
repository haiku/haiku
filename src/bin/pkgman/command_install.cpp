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
//#include <sys/stat.h>

#include <package/PackageRoster.h>
#include <package/RepositoryConfig.h>
#include <package/solver/SolverPackageSpecifier.h>
#include <package/solver/SolverPackageSpecifierList.h>
#include <package/solver/SolverProblem.h>
#include <package/solver/SolverProblemSolution.h>
#include <package/solver/SolverResult.h>

#include <AutoDeleter.h>

#include "Command.h"
#include "pkgman.h"
#include "RepositoryBuilder.h"


// TODO: internationalization!


using namespace BPackageKit;


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

	BSolverRepository homeRepository;
	if (installInHome) {
		commonRepository.SetPriority(-2);
		RepositoryBuilder(homeRepository, "home")
			.AddPackages(B_PACKAGE_INSTALLATION_LOCATION_HOME, "home")
			.AddToSolver(solver, true);
	}

	// other repositories
	BObjectList<BSolverRepository> otherRepositories(10, true);
	BPackageRoster roster;
	BStringList repositoryNames;
	error = roster.GetRepositoryNames(repositoryNames);
	if (error != B_OK)
		WARN(error, "failed to get repository names");

	int32 repositoryNameCount = repositoryNames.CountStrings();
	for (int32 i = 0; i < repositoryNameCount; i++) {
		const BString& name = repositoryNames.StringAt(i);
		BRepositoryConfig config;
		error = roster.GetRepositoryConfig(name, &config);
		if (error != B_OK) {
			WARN(error, "failed to get config for repository \"%s\". Skipping.",
				name.String());
			continue;
		}

		BSolverRepository* repository = new(std::nothrow) BSolverRepository;
		if (repository == NULL || !otherRepositories.AddItem(repository))
			DIE(B_NO_MEMORY, "out of memory");

		RepositoryBuilder(*repository, config)
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

	printf("transaction:\n");
	for (int32 i = 0; const BSolverResultElement* element = result.ElementAt(i);
			i++) {
		BSolverPackage* package = element->Package();
		switch (element->Type()) {
			case BSolverResultElement::B_TYPE_INSTALL:
				printf("  install package %s from repository %s\n",
					package->VersionedName().String(),
					package->Repository()->Name().String());
				break;
			case BSolverResultElement::B_TYPE_UNINSTALL:
				printf("  uninstall package %s\n",
					package->VersionedName().String());
				break;
		}
	}

	return 0;
}
