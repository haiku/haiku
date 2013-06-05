/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/UserSettingsFileInfo.h>

#include <package/hpkg/PackageInfoAttributeValue.h>


namespace BPackageKit {


BUserSettingsFileInfo::BUserSettingsFileInfo()
	:
	fPath(),
	fTemplatePath()
{
}


BUserSettingsFileInfo::BUserSettingsFileInfo(
	const BHPKG::BUserSettingsFileInfoData& infoData)
	:
	fPath(infoData.path),
	fTemplatePath(infoData.templatePath),
	fIsDirectory(infoData.isDirectory)
{
}


BUserSettingsFileInfo::BUserSettingsFileInfo(const BString& path,
	const BString& templatePath)
	:
	fPath(path),
	fTemplatePath(templatePath),
	fIsDirectory(false)
{
}


BUserSettingsFileInfo::BUserSettingsFileInfo(const BString& path,
	bool isDirectory)
	:
	fPath(path),
	fTemplatePath(),
	fIsDirectory(isDirectory)
{
}


BUserSettingsFileInfo::~BUserSettingsFileInfo()
{
}


status_t
BUserSettingsFileInfo::InitCheck() const
{
	return fPath.IsEmpty() ? B_NO_INIT : B_OK;
}


const BString&
BUserSettingsFileInfo::Path() const
{
	return fPath;
}


const BString&
BUserSettingsFileInfo::TemplatePath() const
{
	return fTemplatePath;
}


bool
BUserSettingsFileInfo::IsDirectory() const
{
	return fIsDirectory;
}


void
BUserSettingsFileInfo::SetTo(const BString& path, const BString& templatePath)
{
	fPath = path;
	fTemplatePath = templatePath;
	fIsDirectory = false;
}


void
BUserSettingsFileInfo::SetTo(const BString& path, bool isDirectory)
{
	fPath = path;
	fTemplatePath.Truncate(0);
	fIsDirectory = isDirectory;
}


}	// namespace BPackageKit
