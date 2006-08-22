/*
 * Copyright 2003-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jonas Sundström, jonas.sundstrom@kirilla.com
 */

// TODO: proper locking      <<---------

#include "ZipOMaticMisc.h"
#include "ZipOMaticSettings.h"

#include <Debug.h>
#include <VolumeRoster.h>
#include <FindDirectory.h>
#include <Directory.h>
#include <Path.h>
#include <File.h>


ZippoSettings::ZippoSettings()
	:
	fBaseDir(B_USER_SETTINGS_DIRECTORY)
{
	PRINT(("ZippoSettings()\n"));
}


ZippoSettings::ZippoSettings(BMessage& message)
	: BMessage(message),
	fBaseDir(B_USER_SETTINGS_DIRECTORY)
{
	PRINT(("ZippoSettings(a_message)\n"));
}


ZippoSettings::~ZippoSettings()	
{
}


status_t
ZippoSettings::SetTo(const char* filename, BVolume* volume,
	directory_which baseDir, const char* relativePath)
{
	status_t status = B_OK;

	// copy to members
	fBaseDir = baseDir;
	fRelativePath = relativePath;
	fFilename = filename;

	// sanity check
	if (volume == NULL) {
		BVolumeRoster volumeRoster;
		volumeRoster.GetBootVolume(&fVolume);
	} else
		fVolume = *volume;

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
	PRINT(("ZippoSettings::ReadSettings()\n"));

	BFile file;
	status_t status = _GetSettingsFile(&file, B_READ_ONLY);
	if (status != B_OK)
		return status;

	return Unflatten(&file);
}


status_t
ZippoSettings::WriteSettings()
{
	PRINT(("ZippoSettings::WriteSettings()\n"));

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
	status_t status = find_and_create_directory(fBaseDir, &fVolume,
		fRelativePath.String(), &path);
	if (status != B_OK)
		return status;

	status = path.Append(fFilename.String());
	if (status != B_OK)
		return status;

	return file->SetTo(path.Path(), openMode);
}

