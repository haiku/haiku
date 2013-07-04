/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include "RepositoryBuilder.h"

#include <errno.h>
#include <dirent.h>

#include <Entry.h>
#include <package/RepositoryCache.h>
#include <Path.h>

#include <AutoDeleter.h>

#include "PackageInfoErrorListener.h"
#include "pkgman.h"


RepositoryBuilder::RepositoryBuilder(BSolverRepository& repository,
	const BString& name, const BString& errorName)
	:
	fRepository(repository),
	fErrorName(errorName.IsEmpty() ? name : errorName),
	fPackagePaths(NULL)
{
	status_t error = fRepository.SetTo(name);
	if (error != B_OK)
		DIE(error, "failed to init %s repository", fErrorName.String());
}


RepositoryBuilder::RepositoryBuilder(BSolverRepository& repository,
	const BRepositoryConfig& config)
	:
	fRepository(repository),
	fErrorName(fRepository.Name()),
	fPackagePaths(NULL)
{
	status_t error = fRepository.SetTo(config);
	if (error != B_OK)
		DIE(error, "failed to init %s repository", fErrorName.String());
}


RepositoryBuilder::RepositoryBuilder(BSolverRepository& repository,
	const BRepositoryCache& cache, const BString& errorName)
	:
	fRepository(repository),
	fErrorName(errorName.IsEmpty() ? cache.Info().Name() : errorName),
	fPackagePaths(NULL)
{
	status_t error = fRepository.SetTo(cache);
	if (error != B_OK)
		DIE(error, "failed to init %s repository", fErrorName.String());
	fErrorName = fRepository.Name();
}


RepositoryBuilder&
RepositoryBuilder::SetPackagePathMap(PackagePathMap* packagePaths)
{
	fPackagePaths = packagePaths;
	return *this;
}


RepositoryBuilder&
RepositoryBuilder::AddPackage(const BPackageInfo& info,
	const char* packageErrorName, BSolverPackage** _package)
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


RepositoryBuilder&
RepositoryBuilder::AddPackage(const char* path, BSolverPackage** _package)
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
		error = packageInfo.ReadFromConfigFile(BEntry(path, true),
			&errorListener);
	}

	if (error != B_OK)
		DIE(error, "failed to read package info from \"%s\"", path);

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


RepositoryBuilder&
RepositoryBuilder::AddPackages(BPackageInstallationLocation location,
	const char* locationErrorName)
{
	status_t error = fRepository.AddPackages(location);
	if (error != B_OK) {
		DIE(error, "failed to add %s packages to %s repository",
			locationErrorName, fErrorName.String());
	}
	return *this;
}


RepositoryBuilder&
RepositoryBuilder::AddPackagesDirectory(const char* path)
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
		if (stat(entryPath.Path(), &st) != 0)
			DIE(errno, "failed to stat() %s", entryPath.Path());

		if (!S_ISREG(st.st_mode))
			continue;

		AddPackage(entryPath.Path());
	}

	return *this;
}


RepositoryBuilder&
RepositoryBuilder::AddToSolver(BSolver* solver, bool isInstalled)
{
	fRepository.SetInstalled(isInstalled);

	status_t error = solver->AddRepository(&fRepository);
	if (error != B_OK) {
		DIE(error, "failed to add %s repository to solver",
			fErrorName.String());
	}
	return *this;
}
