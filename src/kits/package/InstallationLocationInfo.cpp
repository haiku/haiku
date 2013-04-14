/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <package/InstallationLocationInfo.h>


namespace BPackageKit {


BInstallationLocationInfo::BInstallationLocationInfo()
	:
	fLocation(B_PACKAGE_INSTALLATION_LOCATION_ENUM_COUNT),
	fBaseDirectoryRef(),
	fPackageDirectoryRef(),
	fActivePackageInfos(),
	fInactivePackageInfos(),
	fChangeCount(0)
{
}


BInstallationLocationInfo::~BInstallationLocationInfo()
{
}


void
BInstallationLocationInfo::Unset()
{
	fLocation = B_PACKAGE_INSTALLATION_LOCATION_ENUM_COUNT;
	fBaseDirectoryRef = node_ref();
	fPackageDirectoryRef = node_ref();
	fActivePackageInfos.MakeEmpty();
	fInactivePackageInfos.MakeEmpty();
	fChangeCount = 0;
}


BPackageInstallationLocation
BInstallationLocationInfo::Location() const
{
	return fLocation;
}


void
BInstallationLocationInfo::SetLocation(BPackageInstallationLocation location)
{
	fLocation = location;
}


const node_ref&
BInstallationLocationInfo::BaseDirectoryRef() const
{
	return fBaseDirectoryRef;
}


status_t
BInstallationLocationInfo::SetBaseDirectoryRef(const node_ref& ref)
{
	fBaseDirectoryRef = ref;
	return fBaseDirectoryRef == ref ? B_OK : B_NO_MEMORY;
}


const node_ref&
BInstallationLocationInfo::PackagesDirectoryRef() const
{
	return fPackageDirectoryRef;
}


status_t
BInstallationLocationInfo::SetPackagesDirectoryRef(const node_ref& ref)
{
	fPackageDirectoryRef = ref;
	return fPackageDirectoryRef == ref ? B_OK : B_NO_MEMORY;
}


const BPackageInfoSet&
BInstallationLocationInfo::ActivePackageInfos() const
{
	return fActivePackageInfos;
}


void
BInstallationLocationInfo::SetActivePackageInfos(const BPackageInfoSet& infos)
{
	fActivePackageInfos = infos;
}


const BPackageInfoSet&
BInstallationLocationInfo::InactivePackageInfos() const
{
	return fInactivePackageInfos;
}


void
BInstallationLocationInfo::SetInactivePackageInfos(const BPackageInfoSet& infos)
{
	fInactivePackageInfos = infos;
}


int64
BInstallationLocationInfo::ChangeCount() const
{
	return fChangeCount;
}


void
BInstallationLocationInfo::SetChangeCount(int64 changeCount)
{
	fChangeCount = changeCount;
}


}	// namespace BPackageKit
