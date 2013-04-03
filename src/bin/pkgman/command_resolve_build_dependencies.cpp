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

//#include "DecisionProvider.h"
//#include "JobStateListener.h"
#include "pkgman.h"


using namespace BPackageKit;


// TODO: internationalization!


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
		fErrorName(errorName.IsEmpty() ? name : errorName)
	{
		status_t error = fRepository.SetTo(name);
		if (error != B_OK)
			DIE(error, "failed to init %s repository", fErrorName.String());
	}

	RepositoryBuilder& AddPackage(const BPackageInfo& info,
		const char* packageErrorName = NULL)
	{
		status_t error = fRepository.AddPackage(info);
		if (error != B_OK) {
			DIE(error, "failed to add %s to %s repository",
				packageErrorName != NULL
					? packageErrorName
					: (BString("package ") << info.Name()).String(),
				fErrorName.String());
		}
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

			// read a package info from the (HPKG or package info) file
			BPackageInfo packageInfo;

			size_t nameLength = strlen(name);
			if (nameLength > 5 && strcmp(name + nameLength - 5, ".hpkg") == 0) {
				// a package file
				error = packageInfo.ReadFromPackageFile(entryPath.Path());
			} else {
				// a package info file (supposedly)
				PackageInfoErrorListener errorListener("reading package info");
				error = packageInfo.ReadFromConfigFile(BEntry(entryPath.Path()),
					&errorListener);
			}

			if (error != B_OK) {
				DIE(errno, "failed to read package info from \"%s\"",
					entryPath.Path());
			}

			AddPackage(packageInfo, entryPath.Path());
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
};


static const char* kCommandUsage =
	"Usage: %s resolve-build-dependencies <package> <prerequisites>\n"
	"  <build-repository> [ <prerequisite-repository> ... ]\n"
	"Resolves and lists all packages needed for building a package. Fails, if\n"
	"not all dependencies could be resolved.\n"
	"\n"
	"<package>\n"
	"  The package info for the package to be built, requiring all build\n"
	"  requisites.\n"
	"<prerequisites>\n"
	"  A text file listing the package's build prerequisites, i.e. the\n"
	"  requisites that must be installed from the host environment.\n"
	"<build-repository>\n"
	"  Path to a directory containing package infos for all the packages that\n"
	"  can be built. This repository is used to resolve the build requisites\n"
	"  the package to build.\n"
	"<prerequisite-repository>\n"
	"  Path to a directory containing packages from which the package's\n"
	"  prerequisites shall be resolved. Multiple directories can be\n"
	"  specified. If none is given, the system's installed packages are used.\n"
	"\n"
;

static const char* kPrerequisitesPackageInfoTemplate =
	"name			_build_prerequisites_\n"
	"version		1.0.0-1\n"
	"architecture	%s\n"
	"summary		none\n"
	"description	none\n"
	"packager		none\n"
	"vendor			\"%s\"\n"
	"licenses {\n"
	"	MIT\n"
	"}\n"
	"copyrights {\n"
	"	none\n"
	"}\n"
	"provides {\n"
	"	_build_prerequisites_ = 1.0.0\n"
	"}\n"
	"requires {\n %s }\n";


static void
print_command_usage_and_exit(bool error)
{
    fprintf(error ? stderr : stdout, kCommandUsage, kProgramName);
    exit(error ? 1 : 0);
}


static void
resolve_packages(BSolver* solver, const BPackageInfo& packageInfo,
	bool isPrerequisitePhase, BSolverResult& _result)
{
	const char* requisitesString = isPrerequisitePhase
		? "prerequisites" :"requisites";

	// add a repository with the build package resolvable
	BSolverRepository dummyRepository;
	RepositoryBuilder(dummyRepository, "dummy", "build package")
		.AddPackage(packageInfo, "package to install")
		.AddToSolver(solver);

	// resolve
	BSolverPackageSpecifierList packagesToInstall;
	if (!packagesToInstall.AppendSpecifier(
			BSolverPackageSpecifier(&dummyRepository,
				BPackageResolvableExpression(packageInfo.Name())))) {
		DIE(B_NO_MEMORY, "failed to add package to install");
	}

	status_t error = solver->Install(packagesToInstall);
	if (error != B_OK)
		DIE(error, "failed to resolve %s", requisitesString);

	if (solver->HasProblems()) {
		fprintf(stderr, "Encountered problems resolving %s:\n",
			requisitesString);

		int32 problemCount = solver->CountProblems();
		for (int32 i = 0; i < problemCount; i++) {
			printf("  %" B_PRId32 ": %s\n", i + 1,
				solver->ProblemAt(i)->ToString().String());
		}
		exit(1);
	}

	BSolverResult& result = _result;
	error = solver->GetResult(result);
	if (error != B_OK)
		DIE(error, "failed to resolve %s", requisitesString);

	// print packages
	printf("[%s]\n", requisitesString);
	for (int32 i = 0; const BSolverResultElement* element = result.ElementAt(i);
			i++) {
// TODO: Print the path to the package/package info!
		printf("%s-%s\n",
			element->Package()->Info().Name().String(),
			element->Package()->Info().Version().ToString().String());
// TODO: Filter out the given package!
	}
}


int
command_resolve_build_dependencies(int argc, const char* const* argv)
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

	// The remaining arguments are the build package info, the prerequisites file,
	// the build requisites directory, and zero or more prerequisites
	// directories.
	if (argc < optind + 3)
		print_command_usage_and_exit(true);

	const char* buildPackagePath = argv[optind++];
	const char* prerequisitesPath = argv[optind++];
	const char* requisitesDirectoryPath = argv[optind++];
	int prerequisitesDirectoryPathCount = argc - optind;
	const char* const* prerequisitesDirectoryPaths
		= prerequisitesDirectoryPathCount > 0 ? argv + optind : NULL;

	if (prerequisitesDirectoryPaths != NULL) {
		DIE(B_NOT_SUPPORTED,
			"sorry, prerequisite directory repositories not supported yet");
// TODO: Support prerequisite directory repositories!
	}

	// read build package info
	BEntry buildPackageEntry(buildPackagePath);
	BPackageInfo buildPackageInfo;
	PackageInfoErrorListener buildPackageInfoErrorListener(
		"Error: Failed to parse package info");
	status_t error = buildPackageInfo.ReadFromConfigFile(buildPackageEntry,
		&buildPackageInfoErrorListener);
	if (error != B_OK)
		DIE(error, "failed to read build package info file");

	// read the prerequisites file into a string
	BString prerequisitesString;
	BFile prerequisitesFile;
	error = prerequisitesFile.SetTo(prerequisitesPath, B_READ_ONLY);
	if (error != B_OK)
		DIE(error, "failed to open prerequisites file");

	off_t fileSize;
	error = prerequisitesFile.GetSize(&fileSize);
	if (error != B_OK)
		DIE(error, "failed to get prerequisites file size");

	if (char* buffer = prerequisitesString.LockBuffer(fileSize)) {
		ssize_t bytesRead = prerequisitesFile.Read(buffer, fileSize);
		if (bytesRead != fileSize) {
			DIE(bytesRead < 0 ? (status_t)bytesRead :B_ERROR,
				"failed to read prerequisites");
		}
	} else
		DIE(B_NO_MEMORY, "failed to read prerequisites");

	prerequisitesString.UnlockBuffer();
	prerequisitesFile.Unset();

	// create a package info that contains the build prerequisites
	BPackageInfo prerequisitesPackageInfo;
	PackageInfoErrorListener buildPrerequisitesPackageInfoErrorListener(
		"Error: Failed to parse prerequisites package info");
	error = prerequisitesPackageInfo.ReadFromConfigString(
		BString().SetToFormat(kPrerequisitesPackageInfoTemplate,
			BPackageInfo::kArchitectureNames[buildPackageInfo.Architecture()],
			buildPackageInfo.Vendor().String(),
			prerequisitesString.String()),
		&buildPrerequisitesPackageInfoErrorListener);
	if (error != B_OK)
		DIE(error, "failed to create prerequisites package info");

	// resolve the prerequisites

	// create the solver
	BSolver* solver;
	error = BSolver::Create(solver);
	if (error != B_OK)
		DIE(error, "failed to create solver");

	// add prerequisite repository
	BSolverRepository prerequisiteRepository;
	RepositoryBuilder(prerequisiteRepository, "prerequisites")
		.AddPackages(B_PACKAGE_INSTALLATION_LOCATION_SYSTEM, "system")
		.AddPackages(B_PACKAGE_INSTALLATION_LOCATION_COMMON, "common")
		.AddToSolver(solver);

	BSolverResult result;
	resolve_packages(solver, prerequisitesPackageInfo, true, result);

	// resolve the requisites

	// reset the solver
	error = solver->Init();
	if (error != B_OK)
		DIE(error, "failed to re-init solver");

	// add an installation repository with the resolved prerequisites
	BSolverRepository installationRepository;
	RepositoryBuilder installationRepositoryBuilder(installationRepository,
		"installation");
	for (int32 i = 0; const BSolverResultElement* element = result.ElementAt(i);
			i++) {
		installationRepositoryBuilder.AddPackage(element->Package()->Info());
	}
	installationRepositoryBuilder.AddToSolver(solver, true);

	// add requisite repository
	BSolverRepository requisiteRepository;
	RepositoryBuilder(requisiteRepository, "requisites")
		.AddPackagesDirectory(requisitesDirectoryPath)
		.AddToSolver(solver);

	resolve_packages(solver, buildPackageInfo, false, result);
// TODO: We need to make sure that the package isn't part of a cyclic
// dependency.

	return 0;
}
