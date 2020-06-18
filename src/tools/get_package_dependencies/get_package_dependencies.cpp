/*
 * Copyright 2013-2020, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 *		Andrew Lindesay <apl@lindesay.co.nz>
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <map>

#include <package/RepositoryCache.h>
#include <package/manager/Exceptions.h>
#include <package/manager/RepositoryBuilder.h>
#include <package/solver/Solver.h>
#include <package/solver/SolverPackageSpecifier.h>
#include <package/solver/SolverPackageSpecifierList.h>
#include <package/solver/SolverProblem.h>
#include <package/solver/SolverProblemSolution.h>
#include <package/solver/SolverRepository.h>
#include <package/solver/SolverResult.h>


using namespace BPackageKit;
using namespace BPackageKit::BManager::BPrivate;


static const char* sProgramName = "get_package_dependencies";


#define DIE(result, msg...)											\
do {																\
	fprintf(stderr, "*** " msg);									\
	fprintf(stderr, " : %s\n", strerror(result));					\
	exit(5);														\
} while(0)


void
print_usage_and_exit(bool error)
{
	fprintf(error ? stderr : stdout,
		"Usage: %s <repository> ... -- <package> ...\n"
		"Resolves the dependencies of the given packages using the given\n"
		"repositories and prints the URLs of the packages that are also\n"
		"needed to satisfy all requirements. Fails, if there are conflicts\n"
		"or some requirements cannot be satisfied.\n",
		sProgramName);
	exit(error ? 1 : 0);
}


int
main(int argc, const char* const* argv)
{
	if (argc < 2)
		print_usage_and_exit(true);

	if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
		print_usage_and_exit(false);

	// get lists of repositories and packages
	int argIndex = 1;
	const char* const* repositories = argv + argIndex;
	while (argIndex < argc && strcmp(argv[argIndex], "--") != 0)
		argIndex++;
	int repositoryCount = argv + argIndex - repositories;

	if (repositoryCount == 0 || argIndex == argc)
		print_usage_and_exit(true);

	const char* const* packages = argv + argIndex + 1;
	int packageCount = argv + argc - packages;

	// create the solver
	BSolver* solver;
	status_t error = BSolver::Create(solver);
	if (error != B_OK)
		DIE(error, "failed to create solver");

	// add the "installed" repository with the given packages
	BSolverRepository installedRepository;
	try {
		BRepositoryBuilder installedRepositoryBuilder(installedRepository, "installed");
		for (int i = 0; i < packageCount; i++)
			installedRepositoryBuilder.AddPackage(packages[i]);
		installedRepositoryBuilder.AddToSolver(solver, true);
	} catch (BFatalErrorException& e) {
		DIE(e.Error(), "%s %s", e.Message().String(), e.Details().String());
	}

	// add external repositories
	std::map<BSolverRepository*, BRepositoryInfo> repositoryInfos;
	for (int i = 0; i < repositoryCount; i++) {
		BSolverRepository* repository = new BSolverRepository;
		BRepositoryCache cache;
		error = cache.SetTo(repositories[i]);
		if (error != B_OK)
			DIE(error, "failed to read repository file '%s'", repositories[i]);
		BRepositoryBuilder(*repository, cache)
			.AddToSolver(solver, false);
		if (cache.Info().BaseURL().IsEmpty()) {
			DIE(B_ERROR, "missing base url in repository file '%s'",
				repositories[i]);
		}
		repositoryInfos[repository] = cache.Info();
	}

	// solve
	error = solver->VerifyInstallation();
	if (error != B_OK)
		DIE(error, "failed to compute packages to install");

	// print problems (and fail), if any
	if (solver->HasProblems()) {
		fprintf(stderr, "Encountered problems:\n");

		int32 problemCount = solver->CountProblems();
		for (int32 i = 0; i < problemCount; i++) {
			// print problem and possible solutions
			BSolverProblem* problem = solver->ProblemAt(i);
			fprintf(stderr, "problem %" B_PRId32 ": %s\n", i + 1,
				problem->ToString().String());

			int32 solutionCount = problem->CountSolutions();
			for (int32 k = 0; k < solutionCount; k++) {
				const BSolverProblemSolution* solution = problem->SolutionAt(k);
				fprintf(stderr, "  solution %" B_PRId32 ":\n", k + 1);
				int32 elementCount = solution->CountElements();
				for (int32 l = 0; l < elementCount; l++) {
					const BSolverProblemSolutionElement* element
						= solution->ElementAt(l);
					fprintf(stderr, "    - %s\n", element->ToString().String());
				}
			}
		}

		exit(1);
	}

	// print URL of packages that additionally need to be installed
	BSolverResult result;
	error = solver->GetResult(result);
	if (error != B_OK)
		DIE(error, "failed to compute packages to install");

	bool duplicatePackage = false;
	for (int32 i = 0; const BSolverResultElement* element = result.ElementAt(i);
			i++) {
		BSolverPackage* package = element->Package();

		switch (element->Type()) {
			case BSolverResultElement::B_TYPE_INSTALL:
				if (package->Repository() != &installedRepository) {
					const BRepositoryInfo& info
						= repositoryInfos[package->Repository()];
					BString url = info.BaseURL();
					url << "/packages/" << package->Info().CanonicalFileName();
					printf("%s\n", url.String());
				}
				break;

			case BSolverResultElement::B_TYPE_UNINSTALL:
				fprintf(stderr, "Error: would need to uninstall package %s\n",
					package->VersionedName().String());
				duplicatePackage = true;
				break;
		}
	}

	return duplicatePackage ? 1 : 0;
}
