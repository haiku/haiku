/*
 * Copyright 2003-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jonas Sundström, jonas@kirilla.com
 */

// TODO: proper locking      <<---------


#include "ZipOMaticSettings.h"

#include <Debug.h>
#include <Directory.h>
#include <File.h>
#include <Path.h>
#include <VolumeRoster.h>

#include "ZipOMaticMisc.h"


ZippoSettings::ZippoSettings()
	:
	fBaseDir(B_USER_SETTINGS_DIRECTORY)
{

}


ZippoSettings::ZippoSettings(BMessage& message)
	:
	BMessage(message),
	fBaseDir(B_USER_SETTINGS_DIRECTORY)
{

}


ZippoSettings::~ZippoSettings()	
{

}


status_t
ZippoSettings::SetTo(const char* filename, BVolume* volume,
	directory_which baseDir, const char* relativePath)
{
	status_t status = B_OK;

	fBaseDir = baseDir;
	fRelativePath = relativePath;
	fFilename = filename;

	if (volume == NULL) {
		BVolumeRoster volumeRoster;
		volumeRoster.GetBootVolume(&fVolume);
	} else {
		fVolume = *volume;
	}

	status = fVolume.InitCheck();
	if (status != B_OK)
		return status;

	return InitCheck();
}


status_t
ZippoSettings::InitCheck()
{
	BFile file;
	return _GetSettingsFile(&file, B_READ_ONLY | B_CREATE_FILE);
}


status_t
ZippoSettings::ReadSettings()
{
	BFile file;
	status_t status = _GetSettingsFile(&file, B_READ_ONLY);
	if (status != B_OK)
		return status;

	return Unflatten(&file);
}


status_t
ZippoSettings::WriteSettings()
{
	BFile file;
	status_t status = _GetSettingsFile(&file, B_WRITE_ONLY | B_ERASE_FILE);
	if (status != B_OK)
		return status;

	return Flatten(&file);
}


status_t
ZippoSettings::_GetSettingsFile(BFile* file, uint32 openMode)
{
	BPath path;
	status_t status = FindAndCreateDirectory(fBaseDir, &fVolume,
		fRelativePath.String(), &path);
	if (status != B_OK)
		return status;

	status = path.Append(fFilename.String());
	if (status != B_OK)
		return status;

	return file->SetTo(path.Path(), openMode);
}

