/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
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


static const char* kWindowSettingsFile = "VM_data";
static const char* kVirtualMemorySettings = "virtual_memory";


Settings::Settings()
	:
	fPositionUpdated(false),
	fSwapUpdated(false)
{
	ReadWindowSettings();
	ReadSwapSettings();
}


Settings::~Settings()
{
	WriteWindowSettings();
	WriteSwapSettings();
}


void
Settings::ReadWindowSettings()
{
	bool success = false;

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append(kWindowSettingsFile);
		BFile file;
		if (file.SetTo(path.Path(), B_READ_ONLY) == B_OK)
			// Now read in the data
			if (file.Read(&fWindowPosition, sizeof(BPoint)) == sizeof(BPoint))
				success = true;
	}

	if (!success)
		fWindowPosition.Set(-1, -1);
}


void
Settings::WriteWindowSettings()
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
Settings::SetWindowPosition(BPoint position)
{
	if (position == fWindowPosition)
		return;

	fWindowPosition = position;
	fPositionUpdated = true;
}


void
Settings::ReadSwapSettings()
{
	// read current swap settings from disk
	void* settings = load_driver_settings("virtual_memory");
	if (settings != NULL) {
		fSwapEnabled = get_driver_boolean_parameter(settings, "vm", false, false);

		const char* string = get_driver_parameter(settings, "swap_size", NULL, NULL);
		fSwapSize = string ? atoll(string) : 0;

		if (fSwapSize <= 0) {
			fSwapEnabled = false;
			fSwapSize = 0;
		}
		unload_driver_settings(settings);
	} else {
		// settings are not available, try to find out what the kernel is up to
		// ToDo: introduce a kernel call for this!
		fSwapSize = 0;

		BPath path;
		if (find_directory(B_COMMON_VAR_DIRECTORY, &path) == B_OK) {
			path.Append("swap");
			BEntry swap(path.Path());
			if (swap.GetSize(&fSwapSize) != B_OK)
				fSwapSize = 0;
		}

		fSwapEnabled = fSwapSize != 0;
	}

	// ToDo: read those as well
	BVolumeRoster volumeRoster;
	volumeRoster.GetBootVolume(&fSwapVolume);

	fInitialSwapEnabled = fSwapEnabled;
	fInitialSwapSize = fSwapSize;
	fInitialSwapVolume = fSwapVolume.Device();
}


void
Settings::WriteSwapSettings()
{
	if (!SwapChanged())
		return;

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return;

	path.Append("kernel/drivers");
	path.Append(kVirtualMemorySettings);

	BFile file;
	if (file.SetTo(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE) != B_OK)
		return;

	char buffer[256];
	snprintf(buffer, sizeof(buffer), "vm %s\nswap_size %Ld\n",
		fSwapEnabled ? "on" : "off", fSwapSize);

	file.Write(buffer, strlen(buffer));
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
	if (volume.Device() == fSwapVolume.Device()
		|| volume.InitCheck() != B_OK)
		return;

	fSwapVolume.SetTo(volume.Device());
}


void
Settings::SetSwapDefaults()
{
	fSwapEnabled = true;

	BVolumeRoster volumeRoster;
	volumeRoster.GetBootVolume(&fSwapVolume);

	system_info info;
	get_system_info(&info);
	fSwapSize = (off_t)info.max_pages * B_PAGE_SIZE;
}


void
Settings::RevertSwapChanges()
{
	fSwapEnabled = fInitialSwapEnabled;
	fSwapSize = fInitialSwapSize;
	fSwapVolume.SetTo(fInitialSwapVolume);
}


bool
Settings::SwapChanged()
{
	return fSwapEnabled != fInitialSwapEnabled
		|| fSwapSize != fInitialSwapSize
		|| fSwapVolume.Device() != fInitialSwapVolume;
}

