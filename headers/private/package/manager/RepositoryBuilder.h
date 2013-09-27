/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef _PACKAGE__MANAGER__PRIVATE__REPOSITORY_BUILDER_H_
#define _PACKAGE__MANAGER__PRIVATE__REPOSITORY_BUILDER_H_


#include <map>

#include <package/PackageInfo.h>
#include <package/solver/Solver.h>
#include <package/solver/SolverPackage.h>
#include <package/solver/SolverRepository.h>


namespace BPackageKit {

namespace BManager {

namespace BPrivate {


typedef std::map<BSolverPackage*, BString> BPackagePathMap;


class BRepositoryBuilder {
public:
								BRepositoryBuilder(
									BSolverRepository& repository);
								BRepositoryBuilder(
									BSolverRepository& repository,
									const BString& name,
									const BString& errorName = BString());
								BRepositoryBuilder(
									BSolverRepository& repository,
									const BRepositoryConfig& config);
								BRepositoryBuilder(
									BSolverRepository& repository,
									const BRepositoryCache& cache,
									const BString& errorName = BString());

			BRepositoryBuilder&	SetPackagePathMap(
									BPackagePathMap* packagePaths);

			BRepositoryBuilder&	AddPackage(const BPackageInfo& info,
									const char* packageErrorName = NULL,
									BSolverPackage** _package = NULL);
			BRepositoryBuilder&	AddPackage(const char* path,
									BSolverPackage** _package = NULL);
			BRepositoryBuilder&	AddPackages(
									BPackageInstallationLocation location,
									const char* locationErrorName);
			BRepositoryBuilder&	AddPackagesDirectory(const char* path);

			BRepositoryBuilder&	AddToSolver(BSolver* solver,
									bool isInstalled = false);

private:
			BSolverRepository&	fRepository;
			BString				fErrorName;
			BPackagePathMap*	fPackagePaths;
};


}	// namespace BPrivate

}	// namespace BManager

}	// namespace BPackageKit


#endif	// _PACKAGE__MANAGER__PRIVATE__REPOSITORY_BUILDER_H_
