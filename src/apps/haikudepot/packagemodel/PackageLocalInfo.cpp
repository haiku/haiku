/*
 * Copyright 2024-2025, Andrew Lindesay <apl@lindesay.co.nz>.
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
PackageLocalInfo::Viewed() const
{
	return fViewed;
}


const BString&
PackageLocalInfo::LocalFilePath() const
{
	return fLocalFilePath;
}


const BString&
PackageLocalInfo::FileName() const
{
	return fFileName;
}


off_t
PackageLocalInfo::Size() const
{
	return fSize;
}


int32
PackageLocalInfo::Flags() const
{
	return fFlags;
}


bool
PackageLocalInfo::IsSystemDependency() const
{
	return fSystemDependency;
}


PackageState
PackageLocalInfo::State() const
{
	return fState;
}


const PackageInstallationLocationSet&
PackageLocalInfo::InstallationLocations() const
{
	return fInstallationLocations;
}


float
PackageLocalInfo::DownloadProgress() const
{
	return fDownloadProgress;
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


bool
PackageLocalInfo::HasInstallationLocation(int32 location) const
{
	return fInstallationLocations.find(location) != fInstallationLocations.end();
}


int32
PackageLocalInfo::CountInstallationLocations() const
{
	return static_cast<int32>(fInstallationLocations.size());
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


// #pragma mark - PackageLocalInfoBuilder


PackageLocalInfoBuilder::PackageLocalInfoBuilder()
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


PackageLocalInfoBuilder::PackageLocalInfoBuilder(const PackageLocalInfoRef& value)
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
	fSource = value;
}


PackageLocalInfoBuilder::~PackageLocalInfoBuilder()
{
}


void
PackageLocalInfoBuilder::_InitFromSource()
{
	if (fSource.IsSet()) {
		_Init(fSource);
		fSource.Unset();
	}
}


void
PackageLocalInfoBuilder::_Init(const PackageLocalInfo* value)
{
	fViewed = value->Viewed();
	fLocalFilePath = value->LocalFilePath();
	fFileName = value->FileName();
	fSize = value->Size();
	fFlags = value->Flags();
	fSystemDependency = value->IsSystemDependency();
	fState = value->State();
	fDownloadProgress = value->DownloadProgress();

	PackageInstallationLocationSet valueLocations = value->InstallationLocations();
	PackageInstallationLocationSet::const_iterator it;

	for (it = valueLocations.begin(); it != valueLocations.end(); it++)
		fInstallationLocations.insert(*it);
}


PackageLocalInfoRef
PackageLocalInfoBuilder::BuildRef()
{
	if (fSource.IsSet())
		return fSource;

	PackageLocalInfo* localInfo = new PackageLocalInfo();

	if (fViewed)
		localInfo->SetViewed();

	localInfo->SetLocalFilePath(fLocalFilePath);
	localInfo->SetFileName(fFileName);
	localInfo->SetSize(fSize);
	localInfo->SetFlags(fFlags);
	localInfo->SetSystemDependency(fSystemDependency);
	localInfo->SetState(fState);
	if (fState == DOWNLOADING)
		localInfo->SetDownloadProgress(fDownloadProgress);

	PackageInstallationLocationSet::const_iterator it;

	for (it = fInstallationLocations.begin(); it != fInstallationLocations.end(); it++)
		localInfo->AddInstallationLocation(*it);

	return PackageLocalInfoRef(localInfo, true);
}


PackageLocalInfoBuilder&
PackageLocalInfoBuilder::WithViewed()
{
	if (!fSource.IsSet() || fSource->Viewed()) {
		_InitFromSource();
		fViewed = true;
	}
	return *this;
}


PackageLocalInfoBuilder&
PackageLocalInfoBuilder::WithLocalFilePath(const char* path)
{
	if (!fSource.IsSet() || fSource->LocalFilePath() != path) {
		_InitFromSource();
		fLocalFilePath.SetTo(path);
	}
	return *this;
}


PackageLocalInfoBuilder&
PackageLocalInfoBuilder::WithFileName(const BString& value)
{
	if (!fSource.IsSet() || fSource->FileName() != value) {
		_InitFromSource();
		fFileName.SetTo(value);
	}
	return *this;
}


PackageLocalInfoBuilder&
PackageLocalInfoBuilder::WithSize(off_t size)
{
	if (!fSource.IsSet() || fSource->Size() != size) {
		_InitFromSource();
		fSize = size;
	}
	return *this;
}


PackageLocalInfoBuilder&
PackageLocalInfoBuilder::WithFlags(int32 value)
{
	if (!fSource.IsSet() || fSource->Flags() != value) {
		_InitFromSource();
		fFlags = value;
	}
	return *this;
}


PackageLocalInfoBuilder&
PackageLocalInfoBuilder::WithSystemDependency(bool value)
{
	if (!fSource.IsSet() || fSource->IsSystemDependency() != value) {
		_InitFromSource();
		fSystemDependency = value;
	}
	return *this;
}


PackageLocalInfoBuilder&
PackageLocalInfoBuilder::WithState(PackageState value)
{
	if (!fSource.IsSet() || fSource->State() != value) {
		_InitFromSource();
		fState = value;
	}
	return *this;
}


PackageLocalInfoBuilder&
PackageLocalInfoBuilder::WithDownloadProgress(float value)
{
	if (!fSource.IsSet() || fSource->DownloadProgress() != value) {
		_InitFromSource();
		fDownloadProgress = value;
	}
	return *this;
}


PackageLocalInfoBuilder&
PackageLocalInfoBuilder::AddInstallationLocation(int32 value)
{
	if (!fSource.IsSet() || !fSource->HasInstallationLocation(value)) {
		_InitFromSource();
		fInstallationLocations.insert(value);
	}
	return *this;
}


PackageLocalInfoBuilder&
PackageLocalInfoBuilder::ClearInstallationLocations()
{
	if (!fSource.IsSet() || fSource->CountInstallationLocations() != 0) {
		_InitFromSource();
		fInstallationLocations.clear();
	}
	return *this;
}
