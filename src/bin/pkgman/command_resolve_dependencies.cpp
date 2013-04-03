/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <map>

#include <Entry.h>
#include <File.h>
#include <Path.h>

#include <package/PackageInfo.h>
#include <package/solver/Solver.h>
#include <package/solver/SolverPackage.h>
#include <package/solver/SolverPackageSpecifier.h>
#include <package/solver/SolverPackageSpecifierList.h>
#include <package/solver/SolverProblem.h>
#include <package/solver/SolverRepository.h>
#include <package/solver/SolverResult.h>

#include <AutoDeleter.h>

#include "pkgman.h"


// TODO: internationalization!


using namespace BPackageKit;


typedef std::map<BSolverPackage*, BString> PackagePathMap;


static const char* kCommandUsage =
	"Usage: %s resolve-dependencies <package> <repository> [ <priority> ] ...\n"
	"Resolves and lists all packages a given package depends on. Fails, if\n"
	"not all dependencies could be resolved.\n"
	"\n"
	"<package>\n"
	"  The HPKG or package info file of the package for which the\n"
	"  dependencies shall be resolved.\n"
	"<repository>\n"
	"  Path to a directory containing packages from which the package's\n"
	"  dependencies shall be resolved. Multiple directories can be specified.\n"
	"<priority>\n"
	"  Can follow a <repository> to specify the priority of that repository.\n"
	"  The default priority is 0.\n"
	"\n"
;


static void
print_command_usage_and_exit(bool error)
{
    fprintf(error ? stderr : stdout, kCommandUsage, kProgramName);
    exit(error ? 1 : 0);
}


struct PackageInfoErrorListener : public BPackageInfo::ParseErrorListener {
public:
	PackageInfoErrorListener(const BString& errorContext)
		:
		fErrorContext(errorContext)
	{
	}

	virtual void OnError(const BString& message, int line, int column)
	{
		fprintf(stderr, "%s: Parse error in line %d:%d: %s\n",
			fErrorContext.String(), line, column, message.String());
	}

private:
	BString	fErrorContext;
};


struct RepositoryBuilder {
	RepositoryBuilder(BSolverRepository& repository, const BString& name,
		const BString& errorName = BString())
		:
		fRepository(repository),
		fErrorName(errorName.IsEmpty() ? name : errorName),
		fPackagePaths(NULL)
	{
		status_t error = fRepository.SetTo(name);
		if (error != B_OK)
			DIE(error, "failed to init %s repository", fErrorName.String());
	}

	RepositoryBuilder& SetPackagePathMap(PackagePathMap* packagePaths)
	{
		fPackagePaths = packagePaths;
		return *this;
	}

	RepositoryBuilder& AddPackage(const BPackageInfo& info,
		const char* packageErrorName = NULL, BSolverPackage** _package = NULL)
	{
		status_t error = fRepository.AddPackage(info, _package);
		if (error != B_OK) {
			DIE(error, "failed to add %s to %s repository",
				packageErrorName != NULL
					? packageErrorName
					: (BString("package ") << info.Name()).String(),
				fErrorName.String());
		}
		return *this;
	}

	RepositoryBuilder& AddPackage(const char* path,
		BSolverPackage** _package = NULL)
	{
		// read a package info from the (HPKG or package info) file
		BPackageInfo packageInfo;

		size_t pathLength = strlen(path);
		status_t error;
		if (pathLength > 5 && strcmp(path + pathLength - 5, ".hpkg") == 0) {
			// a package file
			error = packageInfo.ReadFromPackageFile(path);
		} else {
			// a package info file (supposedly)
			PackageInfoErrorListener errorListener(
				"Error: failed to read package info");
			error = packageInfo.ReadFromConfigFile(BEntry(path),
				&errorListener);
		}

		if (error != B_OK)
			DIE(errno, "failed to read package info from \"%s\"", path);

		// add the package
		BSolverPackage* package;
		AddPackage(packageInfo, path, &package);

		// enter the package path in the path map, if given
		if (fPackagePaths != NULL)
			(*fPackagePaths)[package] = path;

		if (_package != NULL)
			*_package = package;

		return *this;
	}

	RepositoryBuilder& AddPackages(BPackageInstallationLocation location,
		const char* locationErrorName)
	{
		status_t error = fRepository.AddPackages(location);
		if (error != B_OK) {
			DIE(error, "failed to add %s packages to %s repository",
				locationErrorName, fErrorName.String());
		}
		return *this;
	}

	RepositoryBuilder& AddPackagesDirectory(const char* path)
	{
		// open directory
		DIR* dir = opendir(path);
		if (dir == NULL)
			DIE(errno, "failed to open package directory \"%s\"", path);
		CObjectDeleter<DIR, int> dirCloser(dir, &closedir);

		// iterate through directory entries
		while (dirent* entry = readdir(dir)) {
			// skip "." and ".."
			const char* name = entry->d_name;
			if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
				continue;

			// stat() the entry and skip any non-file
			BPath entryPath;
			status_t error = entryPath.SetTo(path, name);
			if (error != B_OK)
				DIE(errno, "failed to construct path");

			struct stat st;
			if (lstat(entryPath.Path(), &st) != 0)
				DIE(errno, "failed to stat() %s", entryPath.Path());

			if (!S_ISREG(st.st_mode))
				continue;

			AddPackage(entryPath.Path());
		}

		return *this;
	}

	RepositoryBuilder& AddToSolver(BSolver* solver, bool isInstalled = false)
	{
		fRepository.SetInstalled(isInstalled);

		status_t error = solver->AddRepository(&fRepository);
		if (error != B_OK) {
			DIE(error, "failed to add %s repository to solver",
				fErrorName.String());
		}
		return *this;
	}

private:
	BSolverRepository&	fRepository;
	BString				fErrorName;
	PackagePathMap*		fPackagePaths;
};


static void
verify_result(const BSolverResult& result, BSolverPackage* specifiedPackage)
{
	// create the solver
	BSolver* solver;
	status_t error = BSolver::Create(solver);
	if (error != B_OK)
		DIE(error, "failed to create solver");

	// Add an installation repository and add all of the result packages save
	// the specified package.
	BSolverRepository installation;
	RepositoryBuilder installationBuilder(installation, "installation");

	for (int32 i = 0; const BSolverResultElement* element = result.ElementAt(i);
			i++) {
		BSolverPackage* package = element->Package();
		if (package != specifiedPackage)
			installationBuilder.AddPackage(package->Info());
	}
	installationBuilder.AddToSolver(solver, true);

	// resolve
	error = solver->VerifyInstallation();
	if (error != B_OK)
		DIE(error, "failed to verify computed package dependencies");

	if (solver->HasProblems()) {
		fprintf(stderr,
			"Encountered problems verifying computed package dependencies:\n");

		int32 problemCount = solver->CountProblems();
		for (int32 i = 0; i < problemCount; i++) {
			printf("  %" B_PRId32 ": %s\n", i + 1,
				solver->ProblemAt(i)->ToString().String());
		}
		exit(1);
	}
}


int
command_resolve_dependencies(int argc, const char* const* argv)
{
	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "hu", sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'h':
				print_command_usage_and_exit(false);
				break;

			default:
				print_command_usage_and_exit(true);
				break;
		}
	}

	// The remaining arguments are the package (info) file and the repository
	// directories (at least one), optionally with priorities.
	if (argc < optind + 2)
		print_command_usage_and_exit(true);

	const char* packagePath = argv[optind++];
	int repositoryDirectoryCount = argc - optind;
	const char* const* repositoryDirectories = argv + optind;

	// create the solver
	BSolver* solver;
	status_t error = BSolver::Create(solver);
	if (error != B_OK)
		DIE(error, "failed to create solver");

	// add repositories
	PackagePathMap packagePaths;
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

		RepositoryBuilder(*repository, BString("repository") << repositoryIndex)
			.SetPackagePathMap(&packagePaths)
			.AddPackagesDirectory(directoryPath)
			.AddToSolver(solver);
	}

	// add a repository with only the specified package
	BSolverRepository dummyRepository;
	RepositoryBuilder(dummyRepository, "dummy", "specified package")
		.AddPackage(packagePath)
		.AddToSolver(solver);
	BSolverPackage* specifiedPackage = dummyRepository.PackageAt(0);

	// resolve
	BSolverPackageSpecifierList packagesToInstall;
	if (!packagesToInstall.AppendSpecifier(
			BSolverPackageSpecifier(&dummyRepository,
				BPackageResolvableExpression(
					specifiedPackage->Info().Name())))) {
		DIE(B_NO_MEMORY, "failed to add specified package");
	}

	error = solver->Install(packagesToInstall);
	if (error != B_OK)
		DIE(error, "failed to resolve package dependencies");

	if (solver->HasProblems()) {
		fprintf(stderr,
			"Encountered problems resolving package dependencies:\n");

		int32 problemCount = solver->CountProblems();
		for (int32 i = 0; i < problemCount; i++) {
			printf("  %" B_PRId32 ": %s\n", i + 1,
				solver->ProblemAt(i)->ToString().String());
		}
		exit(1);
	}

	BSolverResult result;
	error = solver->GetResult(result);
	if (error != B_OK)
		DIE(error, "failed to resolve package dependencies");

	// Verify that the resolved packages don't depend on the specified package.
	verify_result(result, specifiedPackage);

	// print packages
	for (int32 i = 0; const BSolverResultElement* element = result.ElementAt(i);
			i++) {
		// skip the specified package
		BSolverPackage* package = element->Package();
		if (package == specifiedPackage)
			continue;

		// resolve and print the path
		PackagePathMap::const_iterator it = packagePaths.find(package);
		if (it == packagePaths.end()) {
			DIE(B_ERROR, "ugh, no package %p (%s-%s) not in package path map",
				package,  package->Info().Name().String(),
				package->Info().Version().ToString().String());
		}

		printf("%s\n", it->second.String());
	}

	return 0;
}
