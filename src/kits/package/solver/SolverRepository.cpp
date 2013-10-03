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
#include <package/solver/SolverPackage.h>


static const int32 kInitialPackageListSize = 40;


namespace BPackageKit {


BSolverRepository::BSolverRepository()
	:
	fName(),
	fPriority(0),
	fIsInstalled(false),
	fPackages(kInitialPackageListSize, true),
	fChangeCount(0)
{
}


BSolverRepository::BSolverRepository(const BString& name)
	:
	fName(),
	fPriority(0),
	fIsInstalled(false),
	fPackages(kInitialPackageListSize, true),
	fChangeCount(0)
{
	SetTo(name);
}


BSolverRepository::BSolverRepository(BPackageInstallationLocation location)
	:
	fName(),
	fPriority(0),
	fIsInstalled(false),
	fPackages(kInitialPackageListSize, true),
	fChangeCount(0)
{
	SetTo(location);
}


BSolverRepository::BSolverRepository(BAllInstallationLocations)
	:
	fName(),
	fPriority(0),
	fIsInstalled(false),
	fPackages(kInitialPackageListSize, true),
	fChangeCount(0)
{
	SetTo(B_ALL_INSTALLATION_LOCATIONS);
}


BSolverRepository::BSolverRepository(const BRepositoryConfig& config)
	:
	fName(),
	fPriority(0),
	fIsInstalled(false),
	fPackages(kInitialPackageListSize, true),
	fChangeCount(0)
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


status_t
BSolverRepository::SetTo(const BRepositoryCache& cache)
{
	Unset();

	const BRepositoryInfo& info = cache.Info();
	if (info.InitCheck() != B_OK)
		return B_BAD_VALUE;

	fName = info.Name();
	fPriority = info.Priority();

	BRepositoryCache::Iterator it = cache.GetIterator();
	while (const BPackageInfo* packageInfo = it.Next()) {
		status_t error = AddPackage(*packageInfo);
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
	fChangeCount++;
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


void
BSolverRepository::SetInstalled(bool isInstalled)
{
	fIsInstalled = isInstalled;
	fChangeCount++;
}


BString
BSolverRepository::Name() const
{
	return fName;
}


int32
BSolverRepository::Priority() const
{
	return fPriority;
}


void
BSolverRepository::SetPriority(int32 priority)
{
	fPriority = priority;
	fChangeCount++;
}


bool
BSolverRepository::IsEmpty() const
{
	return fPackages.IsEmpty();
}


int32
BSolverRepository::CountPackages() const
{
	return fPackages.CountItems();
}


BSolverPackage*
BSolverRepository::PackageAt(int32 index) const
{
	return fPackages.ItemAt(index);
}


status_t
BSolverRepository::AddPackage(const BPackageInfo& info,
	BSolverPackage** _package)
{
	BSolverPackage* package = new(std::nothrow) BSolverPackage(this, info);
	if (package == NULL || !fPackages.AddItem(package)) {
		delete package;
		return B_NO_MEMORY;
	}

	fChangeCount++;

	if (_package != NULL)
		*_package = package;

	return B_OK;
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
		error = AddPackage(*packageInfo);
		if (error != B_OK)
			return error;
	}

	return B_OK;
}


bool
BSolverRepository::RemovePackage(BSolverPackage* package)
{
	if (!fPackages.RemoveItem(package, false))
		return false;

	fChangeCount++;
	return true;
}


bool
BSolverRepository::DeletePackage(BSolverPackage* package)
{
	if (!RemovePackage(package))
		return false;

	delete package;
	return true;
}


uint64
BSolverRepository::ChangeCount() const
{
	return fChangeCount;
}


}	// namespace BPackageKit
