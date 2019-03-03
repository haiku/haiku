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
#include <sys/stat.h>

#include <package/manager/RepositoryBuilder.h>
#include <package/solver/SolverPackageSpecifier.h>
#include <package/solver/SolverPackageSpecifierList.h>
#include <package/solver/SolverProblem.h>
#include <package/solver/SolverResult.h>

#include <AutoDeleter.h>

#include "Command.h"
#include "pkgman.h"


// TODO: internationalization!


using namespace BPackageKit;
using BManager::BPrivate::BPackagePathMap;
using BManager::BPrivate::BRepositoryBuilder;


static const char* const kShortUsage =
	"  %command% <package> ... <repository> [ <priority> ] ...\n"
	"    Resolves all packages the given packages depend on.\n";

static const char* const kLongUsage =
	"Usage: %program% %command% <package> ... <repository> [ <priority> ] ...\n"
	"Resolves and lists all packages the given packages depend on. Fails, if\n"
	"not all dependencies could be resolved.\n"
	"\n"
	"Options:\n"
	"  --debug <level>\n"
	"    Print debug output. <level> should be between 0 (no debug output,\n"
	"    the default) and 10 (most debug output).\n"
	"\n"
	"Arguments:\n"
	"  <package>\n"
	"    The HPKG or package info file of the package for which the\n"
	"    dependencies shall be resolved. Multiple files can be specified.\n"
	"  <repository>\n"
	"    Path to a directory containing packages from which the package's\n"
	"    dependencies shall be resolved. Multiple directories can be\n"
	"    specified.\n"
	"  <priority>\n"
	"    Can follow a <repository> to specify the priority of that\n"
	"    repository. The default priority is 0.\n"
	"\n";


DEFINE_COMMAND(ResolveDependenciesCommand, "resolve-dependencies", kShortUsage,
	kLongUsage, COMMAND_CATEGORY_OTHER)


static void
check_problems(BSolver* solver, const char* errorContext)
{
	if (solver->HasProblems()) {
		fprintf(stderr,
			"Encountered problems %s:\n", errorContext);

		int32 problemCount = solver->CountProblems();
		for (int32 i = 0; i < problemCount; i++) {
			printf("  %" B_PRId32 ": %s\n", i + 1,
				solver->ProblemAt(i)->ToString().String());
		}
		exit(1);
	}
}


static void
verify_result(const BSolverResult& result,
	const BPackagePathMap& specifiedPackagePaths)
{
	// create the solver
	BSolver* solver;
	status_t error = BSolver::Create(solver);
	if (error != B_OK)
		DIE(error, "failed to create solver");

	// Add an installation repository and add all of the result packages save
	// the specified packages.
	BSolverRepository installation;
	BRepositoryBuilder installationBuilder(installation, "installation");

	for (int32 i = 0; const BSolverResultElement* element = result.ElementAt(i);
			i++) {
		BSolverPackage* package = element->Package();
		if (specifiedPackagePaths.find(package) == specifiedPackagePaths.end())
			installationBuilder.AddPackage(package->Info());
	}
	installationBuilder.AddToSolver(solver, true);

	// resolve
	error = solver->VerifyInstallation();
	if (error != B_OK)
		DIE(error, "failed to verify computed package dependencies");

	check_problems(solver, "verifying computed package dependencies");
}


int
ResolveDependenciesCommand::Execute(int argc, const char* const* argv)
{
	while (true) {
		static struct option sLongOptions[] = {
			{ "debug", required_argument, 0, OPTION_DEBUG },
			{ "help", no_argument, 0, 'h' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "h", sLongOptions, NULL);
		if (c == -1)
			break;

		if (fCommonOptions.HandleOption(c))
			continue;

		switch (c) {
			case 'h':
				PrintUsageAndExit(false);
				break;

			default:
				PrintUsageAndExit(true);
				break;
		}
	}

	// The remaining arguments are the package (info) files and the repository
	// directories (at least one), optionally with priorities.
	if (argc < optind + 2)
		PrintUsageAndExit(true);

	// Determine where the package list ends and the repository list starts.
	const char* const* specifiedPackages = argv + optind;
	for (; optind < argc; optind++) {
		const char* path = argv[optind];
		struct stat st;
		if (stat(path, &st) != 0)
			DIE(errno, "failed to stat() \"%s\"", path);

		if (S_ISDIR(st.st_mode))
			break;
	}

	const char* const* repositoryDirectories = argv + optind;
	int repositoryDirectoryCount = argc - optind;
	int specifiedPackageCount = repositoryDirectories - specifiedPackages;

	// create the solver
	BSolver* solver;
	status_t error = BSolver::Create(solver);
	if (error != B_OK)
		DIE(error, "failed to create solver");

	solver->SetDebugLevel(fCommonOptions.DebugLevel());

	// add repositories
	BPackagePathMap packagePaths;
	BObjectList<BSolverRepository> repositories(10, true);
	int32 repositoryIndex = 0;
	for (int i = 0; i < repositoryDirectoryCount; i++, repositoryIndex++) {
		const char* directoryPath = repositoryDirectories[i];

		BSolverRepository* repository = new(std::nothrow) BSolverRepository;
		if (repository == NULL || !repositories.AddItem(repository))
			DIE(B_NO_MEMORY, "failed to create repository");


		if (i + 1 < repositoryDirectoryCount) {
			char* end;
			long priority = strtol(repositoryDirectories[i + 1], &end, 0);
			if (*end == '\0') {
				repository->SetPriority((uint8)priority);
				i++;
			}
		}

		BRepositoryBuilder(*repository,
				BString("repository") << repositoryIndex)
			.SetPackagePathMap(&packagePaths)
			.AddPackagesDirectory(directoryPath)
			.AddToSolver(solver);
	}

	// add a repository with only the specified packages
	BPackagePathMap specifiedPackagePaths;
	BSolverRepository dummyRepository;
	{
		BRepositoryBuilder builder(dummyRepository, "dummy",
				"specified packages");
		builder.SetPackagePathMap(&specifiedPackagePaths);

		for (int i = 0; i < specifiedPackageCount; i++)
			builder.AddPackage(specifiedPackages[i]);

		builder.AddToSolver(solver);
	}

	// resolve
	BSolverPackageSpecifierList packagesToInstall;
	for (BPackagePathMap::const_iterator it = specifiedPackagePaths.begin();
		it != specifiedPackagePaths.end(); ++it) {
		if (!packagesToInstall.AppendSpecifier(it->first))
			DIE(B_NO_MEMORY, "failed to add specified package");
	}

	error = solver->Install(packagesToInstall);
	if (error != B_OK)
		DIE(error, "failed to resolve package dependencies");

	check_problems(solver, "resolving package dependencies");

	BSolverResult result;
	error = solver->GetResult(result);
	if (error != B_OK)
		DIE(error, "failed to resolve package dependencies");

	// Verify that the resolved packages don't depend on the specified package.
	verify_result(result, specifiedPackagePaths);

	// print packages
	for (int32 i = 0; const BSolverResultElement* element = result.ElementAt(i);
			i++) {
		// skip the specified package
		BSolverPackage* package = element->Package();
		if (specifiedPackagePaths.find(package) != specifiedPackagePaths.end())
			continue;

		// resolve and print the path
		BPackagePathMap::const_iterator it = packagePaths.find(package);
		if (it == packagePaths.end()) {
			DIE(B_ERROR, "ugh, no package %p (%s-%s) not in package path map",
				package,  package->Info().Name().String(),
				package->Info().Version().ToString().String());
		}

		printf("%s\n", it->second.String());
	}

	return 0;
}
