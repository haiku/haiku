/*
 * Copyright 2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "PackageLocalInfo.h"

#include "Package.h"


PackageLocalInfo::PackageLocalInfo()
	:
	fViewed(false),
	fLocalFilePath(),
	fFileName(),
	fSize(0),
	fFlags(0),
	fSystemDependency(false),
	fState(NONE),
	fInstallationLocations(),
	fDownloadProgress(0.0)
{
}


PackageLocalInfo::PackageLocalInfo(const PackageLocalInfo& other)
	:
	fViewed(other.fViewed),
	fLocalFilePath(other.fLocalFilePath),
	fFileName(other.fFileName),
	fSize(other.fSize),
	fFlags(other.fFlags),
	fSystemDependency(other.fSystemDependency),
	fState(other.fState),
	fInstallationLocations(),
	fDownloadProgress(other.fDownloadProgress)
{
	PackageInstallationLocationSet otherLocations = other.InstallationLocations();
	PackageInstallationLocationSet::const_iterator it;

	for (it = otherLocations.begin(); it != otherLocations.end(); it++)
		fInstallationLocations.insert(*it);
}


PackageLocalInfo::~PackageLocalInfo()
{
}


PackageLocalInfo&
PackageLocalInfo::operator=(const PackageLocalInfo& other)
{
	fState = other.fState;
	fInstallationLocations = other.fInstallationLocations;
	fDownloadProgress = other.fDownloadProgress;
	fFlags = other.fFlags;
	fSystemDependency = other.fSystemDependency;
	fLocalFilePath = other.fLocalFilePath;
	fFileName = other.fFileName;
	fSize = other.fSize;
	fViewed = other.fViewed;

	return *this;
}


bool
PackageLocalInfo::operator==(const PackageLocalInfo& other) const
{
	return fState == other.fState
		&& fFlags == other.fFlags
		&& fInstallationLocations == other.fInstallationLocations
		&& fDownloadProgress == other.fDownloadProgress
		&& fSystemDependency == other.fSystemDependency
		&& fLocalFilePath == other.fLocalFilePath
		&& fFileName == other.fFileName
		&& fSize == other.fSize
		&& fViewed == other.fViewed;
}


bool
PackageLocalInfo::operator!=(const PackageLocalInfo& other) const
{
	return !(*this == other);
}


bool
PackageLocalInfo::IsSystemPackage() const
{
	return (fFlags & BPackageKit::B_PACKAGE_FLAG_SYSTEM_PACKAGE) != 0;
}


void
PackageLocalInfo::SetSystemDependency(bool isDependency)
{
	fSystemDependency = isDependency;
}


void
PackageLocalInfo::SetState(PackageState state)
{
	if (fState != state) {
		fState = state;
		if (fState != DOWNLOADING)
			fDownloadProgress = 0.0;
	}
}


void
PackageLocalInfo::AddInstallationLocation(int32 location)
{
	fInstallationLocations.insert(location);
	SetState(ACTIVATED);
		// TODO: determine how to differentiate between installed and active.
}


void
PackageLocalInfo::ClearInstallationLocations()
{
	fInstallationLocations.clear();
}


void
PackageLocalInfo::SetDownloadProgress(float progress)
{
	fState = DOWNLOADING;
	fDownloadProgress = progress;
}


void
PackageLocalInfo::SetLocalFilePath(const char* path)
{
	fLocalFilePath = path;
}


bool
PackageLocalInfo::IsLocalFile() const
{
	return !fLocalFilePath.IsEmpty() && fInstallationLocations.empty();
}


void
PackageLocalInfo::SetSize(int64 size)
{
	fSize = size;
}


void
PackageLocalInfo::SetViewed()
{
	fViewed = true;
}


void
PackageLocalInfo::SetFlags(int32 value)
{
	fFlags = value;
}


void
PackageLocalInfo::SetFileName(const BString& value)
{
	fFileName = value;
}
