/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <PathFinder.h>

#include <package/PackageResolvableExpression.h>
#include <package/solver/SolverPackage.h>

#include <directories.h>
#include <package/manager/PackageManager.h>


// NOTE: This is only the package kit specific part of BPathFinder. Everything
// else is implemented in the storage kit.


using namespace BPackageKit;
using namespace BPackageKit::BPrivate;
using namespace BPackageKit::BManager::BPrivate;


static status_t
find_package(const BPackageResolvableExpression& expression,
	BString& _versionedPackageName)
{
	if (expression.InitCheck() != B_OK)
		return B_BAD_VALUE;

	// create the package manager -- we only want to use its solver
	BPackageManager::ClientInstallationInterface installationInterface;
	BPackageManager::UserInteractionHandler userInteractionHandler;
	BPackageManager packageManager(B_PACKAGE_INSTALLATION_LOCATION_HOME,
		&installationInterface, &userInteractionHandler);
	packageManager.Init(BPackageManager::B_ADD_INSTALLED_REPOSITORIES);

	// search
	BObjectList<BSolverPackage> packages;
	status_t error = packageManager.Solver()->FindPackages(expression.Name(),
		BSolver::B_FIND_IN_NAME | BSolver::B_FIND_IN_PROVIDES, packages);
	if (error != B_OK)
		return B_ENTRY_NOT_FOUND;

	// find the newest matching package
	BSolverPackage* foundPackage = NULL;
	for (int32 i = 0; BSolverPackage* package = packages.ItemAt(i); i++) {
		if (package->Info().Matches(expression)
			&& (foundPackage == NULL
				|| package->Info().Version().Compare(
					foundPackage->Info().Version()) > 0)) {
			foundPackage = package;
		}
	}

	if (foundPackage == NULL)
		return B_ENTRY_NOT_FOUND;

	BString version = foundPackage->Info().Version().ToString();
	_versionedPackageName = foundPackage->VersionedName();
	return _versionedPackageName.IsEmpty() ? B_NO_MEMORY : B_OK;
}


BPathFinder::BPathFinder(const BResolvableExpression& expression,
	const char* dependency)
{
	SetTo(expression, dependency);
}


status_t
BPathFinder::SetTo(const BResolvableExpression& expression,
	const char* dependency)
{
	BString versionedPackageName;
	fInitStatus = find_package(expression, versionedPackageName);
	if (fInitStatus != B_OK)
		return fInitStatus;

	BString packageLinksPath;
	packageLinksPath.SetToFormat(kSystemPackageLinksDirectory "/%s/.self",
		versionedPackageName.String());
	if (packageLinksPath.IsEmpty())
		return fInitStatus = B_NO_MEMORY;

	struct stat st;
	if (lstat(packageLinksPath, &st) < 0)
		return fInitStatus = B_ENTRY_NOT_FOUND;

	return _SetTo(NULL, packageLinksPath, dependency);
}
