/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/GlobalWritableFileInfo.h>

#include <package/hpkg/PackageInfoAttributeValue.h>


namespace BPackageKit {


BGlobalWritableFileInfo::BGlobalWritableFileInfo()
	:
	fPath(),
	fUpdateType(B_WRITABLE_FILE_UPDATE_TYPE_ENUM_COUNT)
{
}


BGlobalWritableFileInfo::BGlobalWritableFileInfo(
	const BHPKG::BGlobalWritableFileInfoData& infoData)
	:
	fPath(infoData.path),
	fUpdateType(infoData.updateType),
	fIsDirectory(infoData.isDirectory)
{
}


BGlobalWritableFileInfo::BGlobalWritableFileInfo(const BString& path,
	BWritableFileUpdateType updateType, bool isDirectory)
	:
	fPath(path),
	fUpdateType(updateType),
	fIsDirectory(isDirectory)
{
}


BGlobalWritableFileInfo::~BGlobalWritableFileInfo()
{
}


status_t
BGlobalWritableFileInfo::InitCheck() const
{
	if (fPath.IsEmpty())
		return B_NO_INIT;
	return B_OK;
}


const BString&
BGlobalWritableFileInfo::Path() const
{
	return fPath;
}


bool
BGlobalWritableFileInfo::IsIncluded() const
{
	return fUpdateType != B_WRITABLE_FILE_UPDATE_TYPE_ENUM_COUNT;
}


BWritableFileUpdateType
BGlobalWritableFileInfo::UpdateType() const
{
	return fUpdateType;
}


bool
BGlobalWritableFileInfo::IsDirectory() const
{
	return fIsDirectory;
}


void
BGlobalWritableFileInfo::SetTo(const BString& path,
	BWritableFileUpdateType updateType, bool isDirectory)
{
	fPath = path;
	fUpdateType = updateType;
	fIsDirectory = isDirectory;
}


}	// namespace BPackageKit
