/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef REPOSITORY_BUILDER_H
#define REPOSITORY_BUILDER_H


#include <map>

#include <package/PackageInfo.h>
#include <package/solver/Solver.h>
#include <package/solver/SolverPackage.h>
#include <package/solver/SolverRepository.h>


using namespace BPackageKit;


typedef std::map<BSolverPackage*, BString> PackagePathMap;


class RepositoryBuilder {
private:
			typedef RepositoryBuilder Builder;

public:
								RepositoryBuilder(BSolverRepository& repository,
									const BString& name,
									const BString& errorName = BString());
								RepositoryBuilder(BSolverRepository& repository,
									const BRepositoryConfig& config);
								RepositoryBuilder(BSolverRepository& repository,
									const BRepositoryCache& cache,
									const BString& errorName = BString());

			RepositoryBuilder&	SetPackagePathMap(PackagePathMap* packagePaths);

			RepositoryBuilder&	AddPackage(const BPackageInfo& info,
									const char* packageErrorName = NULL,
									BSolverPackage** _package = NULL);
			RepositoryBuilder&	AddPackage(const char* path,
									BSolverPackage** _package = NULL);
			RepositoryBuilder&	AddPackages(
									BPackageInstallationLocation location,
									const char* locationErrorName);
			RepositoryBuilder&	AddPackagesDirectory(const char* path);

			RepositoryBuilder&	AddToSolver(BSolver* solver,
									bool isInstalled = false);

private:
			BSolverRepository&	fRepository;
			BString				fErrorName;
			PackagePathMap*		fPackagePaths;
};


#endif	// REPOSITORY_BUILDER_H
