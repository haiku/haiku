/*
 * Copyright 2013-2014, Haiku, Inc. All Rights Reserved.
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
	fLatestActivePackageInfos(),
	fLatestInactivePackageInfos(),
	fCurrentlyActivePackageInfos(),
	fOldStateName(),
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
	fLatestActivePackageInfos.MakeEmpty();
	fLatestInactivePackageInfos.MakeEmpty();
	fCurrentlyActivePackageInfos.MakeEmpty();
	fOldStateName.Truncate(0);
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
BInstallationLocationInfo::LatestActivePackageInfos() const
{
	return fLatestActivePackageInfos;
}


void
BInstallationLocationInfo::SetLatestActivePackageInfos(
	const BPackageInfoSet& infos)
{
	fLatestActivePackageInfos = infos;
}


const BPackageInfoSet&
BInstallationLocationInfo::LatestInactivePackageInfos() const
{
	return fLatestInactivePackageInfos;
}


void
BInstallationLocationInfo::SetLatestInactivePackageInfos(
	const BPackageInfoSet& infos)
{
	fLatestInactivePackageInfos = infos;
}


const BPackageInfoSet&
BInstallationLocationInfo::CurrentlyActivePackageInfos() const
{
	return fCurrentlyActivePackageInfos;
}


void
BInstallationLocationInfo::SetCurrentlyActivePackageInfos(
	const BPackageInfoSet& infos)
{
	fCurrentlyActivePackageInfos = infos;
}


const BString&
BInstallationLocationInfo::OldStateName() const
{
	return fOldStateName;
}


void
BInstallationLocationInfo::SetOldStateName(const BString& name)
{
	fOldStateName = name;
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
