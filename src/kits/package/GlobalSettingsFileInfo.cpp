/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/GlobalSettingsFileInfo.h>

#include <package/hpkg/PackageInfoAttributeValue.h>


namespace BPackageKit {


BGlobalSettingsFileInfo::BGlobalSettingsFileInfo()
	:
	fPath(),
	fUpdateType(B_SETTINGS_FILE_UPDATE_TYPE_ENUM_COUNT)
{
}


BGlobalSettingsFileInfo::BGlobalSettingsFileInfo(
	const BHPKG::BGlobalSettingsFileInfoData& infoData)
	:
	fPath(infoData.path),
	fUpdateType(infoData.updateType)
{
}


BGlobalSettingsFileInfo::BGlobalSettingsFileInfo(const BString& path,
	BSettingsFileUpdateType updateType)
	:
	fPath(path),
	fUpdateType(updateType)
{
}


BGlobalSettingsFileInfo::~BGlobalSettingsFileInfo()
{
}


status_t
BGlobalSettingsFileInfo::InitCheck() const
{
	if (fPath.IsEmpty())
		return B_NO_INIT;
	return B_OK;
}


const BString&
BGlobalSettingsFileInfo::Path() const
{
	return fPath;
}


bool
BGlobalSettingsFileInfo::IsIncluded() const
{
	return fUpdateType != B_SETTINGS_FILE_UPDATE_TYPE_ENUM_COUNT;
}


BSettingsFileUpdateType
BGlobalSettingsFileInfo::UpdateType() const
{
	return fUpdateType;
}


void
BGlobalSettingsFileInfo::SetTo(const BString& path,
	BSettingsFileUpdateType updateType)
{
	fPath = path;
	fUpdateType = updateType;
}


}	// namespace BPackageKit
