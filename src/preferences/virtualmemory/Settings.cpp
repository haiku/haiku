/*
 * Copyright 2010-2011, Hamish Morrison, hamish@lavabit.com
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "Settings.h"

#include <File.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <driver_settings.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static const char* kWindowSettingsFile = "VM_data";
static const char* kVirtualMemorySettings = "virtual_memory";
static const int64 kMegaByte = 1024 * 1024;


Settings::Settings()
	:
	fPositionUpdated(false)
{
	_ReadWindowSettings();
	_ReadSwapSettings();
}


Settings::~Settings()
{
	_WriteWindowSettings();
	_WriteSwapSettings();
}


void
Settings::SetWindowPosition(BPoint position)
{
	if (position == fWindowPosition)
		return;

	fWindowPosition = position;
	fPositionUpdated = true;
}


void
Settings::SetSwapEnabled(bool enabled)
{
	fSwapEnabled = enabled;
}


void
Settings::SetSwapSize(off_t size)
{
	fSwapSize = size;
}


void 
Settings::SetSwapVolume(BVolume &volume)
{
	if (volume.Device() == SwapVolume().Device()
		|| volume.InitCheck() != B_OK)
		return;

	fSwapVolume.SetTo(volume.Device());
}


void
Settings::RevertSwapChanges()
{
	fSwapEnabled = fInitialSwapEnabled;
	fSwapSize = fInitialSwapSize;
	fSwapVolume.SetTo(fInitialSwapVolume);
}


bool
Settings::IsRevertible()
{
	return fSwapEnabled != fInitialSwapEnabled
		|| fSwapSize != fInitialSwapSize
		|| fSwapVolume.Device() != fInitialSwapVolume;
}


void
Settings::_ReadWindowSettings()
{
	bool success = false;

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append(kWindowSettingsFile);
		BFile file;
		if (file.SetTo(path.Path(), B_READ_ONLY) == B_OK)
			if (file.Read(&fWindowPosition, sizeof(BPoint)) == sizeof(BPoint))
				success = true;
	}

	if (!success)
		fWindowPosition.Set(-1, -1);
}


void
Settings::_WriteWindowSettings()
{
	if (!fPositionUpdated)
		return;

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) < B_OK)
		return;

	path.Append(kWindowSettingsFile);

	BFile file;
	if (file.SetTo(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE) == B_OK)
		file.Write(&fWindowPosition, sizeof(BPoint));
}


void
Settings::_ReadSwapSettings()
{
	void* settings = load_driver_settings(kVirtualMemorySettings);
	if (settings != NULL) {
		SetSwapEnabled(get_driver_boolean_parameter(settings, "vm", false, false));
		const char* swapSize = get_driver_parameter(settings, "swap_size", NULL, NULL);
		SetSwapSize(swapSize ? atoll(swapSize) : 0);
		
#ifdef SWAP_VOLUME_IMPLEMENTED
		// we need to hang onto this one
		fBadVolName = strdup(get_driver_parameter(settings, "swap_volume", NULL, NULL));
		
		BVolumeRoster volumeRoster;
		BVolume temporaryVolume;
		
		if (fBadVolName != NULL) {
			status_t result = volumeRoster.GetNextVolume(&temporaryVolume);
			char volumeName[B_FILE_NAME_LENGTH];
			while (result != B_BAD_VALUE) {
				temporaryVolume.GetName(volumeName);
				if (strcmp(volumeName, fBadVolName) == 0 
					&& temporaryVolume.IsPersistent() && volumeName[0]) {
					SetSwapVolume(temporaryVolume);
					break;
				}
				result = volumeRoster.GetNextVolume(&temporaryVolume);
			}
		} else
			volumeRoster.GetBootVolume(&fSwapVolume);
#endif
		unload_driver_settings(settings);
	} else
		_SetSwapNull();

#ifndef SWAP_VOLUME_IMPLEMENTED
	BVolumeRoster volumeRoster;
	volumeRoster.GetBootVolume(&fSwapVolume);
#endif

	fInitialSwapEnabled = fSwapEnabled;
	fInitialSwapSize = fSwapSize;
	fInitialSwapVolume = fSwapVolume.Device();
}


void
Settings::_WriteSwapSettings()
{	
	if (!IsRevertible())
		return;

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return;

	path.Append("kernel/drivers");
	path.Append(kVirtualMemorySettings);

	BFile file;
	if (file.SetTo(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE)
		!= B_OK)
		return;
	
	char buffer[256];
#ifdef SWAP_VOLUME_IMPLEMENTED
	char volumeName[B_FILE_NAME_LENGTH] = {0};
	if (SwapVolume().InitCheck() != B_NO_INIT)
		SwapVolume().GetName(volumeName);
	else if (fBadVolName)
		strcpy(volumeName, fBadVolName);
	snprintf(buffer, sizeof(buffer), "vm %s\nswap_size %" B_PRIdOFF "\n"
		"swap_volume %s\n", SwapEnabled() ? "on" : "off", SwapSize(), 
		volumeName[0] ? volumeName : NULL);
#else
	snprintf(buffer, sizeof(buffer), "vm %s\nswap_size %" B_PRIdOFF "\n",
		fSwapEnabled ? "on" : "off", fSwapSize);
#endif

	file.Write(buffer, strlen(buffer));
}


void
Settings::_SetSwapNull()
{
	SetSwapEnabled(false);
	BVolumeRoster volumeRoster;
	BVolume temporaryVolume;
	volumeRoster.GetBootVolume(&temporaryVolume);
	SetSwapVolume(temporaryVolume);
	SetSwapSize(0);
}


