/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <package/solver/SolverRepository.h>

#include <package/PackageDefs.h>
#include <package/PackageRoster.h>
#include <package/RepositoryCache.h>
#include <package/RepositoryConfig.h>


namespace BPackageKit {


BSolverRepository::BSolverRepository()
	:
	fName(),
	fPriority(0),
	fIsInstalled(false),
	fPackages()
{
}


BSolverRepository::BSolverRepository(const BString& name)
	:
	fName(),
	fPriority(0),
	fIsInstalled(false),
	fPackages()
{
	SetTo(name);
}


BSolverRepository::BSolverRepository(BPackageInstallationLocation location)
	:
	fName(),
	fPriority(0),
	fIsInstalled(false),
	fPackages()
{
	SetTo(location);
}


BSolverRepository::BSolverRepository(BAllInstallationLocations)
	:
	fName(),
	fPriority(0),
	fIsInstalled(false),
	fPackages()
{
	SetTo(B_ALL_INSTALLATION_LOCATIONS);
}


BSolverRepository::BSolverRepository(const BRepositoryConfig& config)
	:
	fName(),
	fPriority(0),
	fIsInstalled(false),
	fPackages()
{
	SetTo(config);
}


BSolverRepository::~BSolverRepository()
{
}


status_t
BSolverRepository::SetTo(const BString& name)
{
	Unset();

	fName = name;
	return fName.IsEmpty() ? B_BAD_VALUE : B_OK;
}


status_t
BSolverRepository::SetTo(BPackageInstallationLocation location)
{
	Unset();

	fName = "Installed";

	status_t error = AddPackages(location);
	if (error != B_OK) {
		Unset();
		return error;
	}

	fIsInstalled = true;
	return B_OK;
}


status_t
BSolverRepository::SetTo(BAllInstallationLocations)
{
	status_t error = SetTo(B_PACKAGE_INSTALLATION_LOCATION_SYSTEM);
	if (error != B_OK)
		return error;

	error = AddPackages(B_PACKAGE_INSTALLATION_LOCATION_COMMON);
	if (error != B_OK) {
		Unset();
		return error;
	}

	error = AddPackages(B_PACKAGE_INSTALLATION_LOCATION_HOME);
	if (error != B_OK) {
		Unset();
		return error;
	}

	return B_OK;
}


status_t
BSolverRepository::SetTo(const BRepositoryConfig& config)
{
	Unset();

	if (config.InitCheck() != B_OK)
		return B_BAD_VALUE;

	fName = config.Name();
	fPriority = config.Priority();

	BPackageRoster roster;
	BRepositoryCache cache;
	status_t error = roster.GetRepositoryCache(config.Name(), &cache);
	if (error != B_OK) {
		Unset();
		return error;
	}

	BRepositoryCache::Iterator it = cache.GetIterator();
	while (const BPackageInfo* packageInfo = it.Next()) {
		error = AddPackage(*packageInfo);
		if (error != B_OK) {
			Unset();
			return error;
		}
	}

	return B_OK;
}


void
BSolverRepository::Unset()
{
	fName = BString();
	fPriority = 0;
	fIsInstalled = false;
	fPackages.MakeEmpty();
}


status_t
BSolverRepository::InitCheck()
{
	return fName.IsEmpty() ? B_NO_INIT : B_OK;
}


bool
BSolverRepository::IsInstalled() const
{
	return fIsInstalled;
}


BString
BSolverRepository::Name() const
{
	return fName;
}


uint8
BSolverRepository::Priority() const
{
	return fPriority;
}


status_t
BSolverRepository::AddPackage(const BPackageInfo& info)
{
	return fPackages.AddInfo(info);
}


status_t
BSolverRepository::AddPackages(BPackageInstallationLocation location)
{
	BPackageRoster roster;
	BPackageInfoSet packageInfos;
	status_t error = roster.GetActivePackages(location, packageInfos);
	if (error != B_OK)
		return error;

	BRepositoryCache::Iterator it = packageInfos.GetIterator();
	while (const BPackageInfo* packageInfo = it.Next()) {
		error = fPackages.AddInfo(*packageInfo);
		if (error != B_OK)
			return error;
	}

	return B_OK;
}


BSolverRepository::Iterator
BSolverRepository::GetIterator() const
{
	return fPackages.GetIterator();
}


}	// namespace BPackageKit
