/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/UserSettingsFileInfo.h>


namespace BPackageKit {


BUserSettingsFileInfo::BUserSettingsFileInfo()
	:
	fPath(),
	fTemplatePath()
{
}


BUserSettingsFileInfo::BUserSettingsFileInfo(const BString& path,
	const BString& templatePath)
	:
	fPath(path),
	fTemplatePath(templatePath)
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


void
BUserSettingsFileInfo::SetTo(const BString& path, const BString& templatePath)
{
	fPath = path;
	fTemplatePath = templatePath;
}


}	// namespace BPackageKit
