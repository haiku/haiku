/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Copyright 2010-2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hamish Morrison, hamish@lavabit.com
 *      Alexander von Gluck, kallisti5@unixzen.com
 */


#include "Settings.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <VolumeRoster.h>

#include <driver_settings.h>


static const char* const kWindowSettingsFile = "VM_data";
static const char* const kVirtualMemorySettings = "virtual_memory";
static const off_t kMegaByte = 1024 * 1024;


Settings::Settings()
{
	fDefaultSettings.enabled = true;
	fDefaultSettings.automatic = true;

	system_info sysInfo;
	get_system_info(&sysInfo);
	fDefaultSettings.size = (off_t)sysInfo.max_pages * B_PAGE_SIZE * 2;
	fDefaultSettings.volume = dev_for_path("/boot");
}


void
Settings::SetSwapEnabled(bool enabled, bool revertable)
{
	fCurrentSettings.enabled = enabled;
	if (!revertable)
		fInitialSettings.enabled = enabled;
}


void
Settings::SetSwapAutomatic(bool automatic, bool revertable)
{
	fCurrentSettings.automatic = automatic;
	if (!revertable)
		fInitialSettings.automatic = automatic;
}


void
Settings::SetSwapSize(off_t size, bool revertable)
{
	fCurrentSettings.size = size;
	if (!revertable)
		fInitialSettings.size = size;
}


void
Settings::SetSwapVolume(dev_t volume, bool revertable)
{
	fCurrentSettings.volume = volume;
	if (!revertable)
		fInitialSettings.volume = volume;

}


void
Settings::SetWindowPosition(BPoint position)
{
	fWindowPosition = position;
}


status_t
Settings::ReadWindowSettings()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return B_ERROR;

	path.Append(kWindowSettingsFile);
	BFile file;
	if (file.SetTo(path.Path(), B_READ_ONLY) != B_OK)
		return B_ERROR;

	if (file.Read(&fWindowPosition, sizeof(BPoint)) == sizeof(BPoint))
		return B_OK;
	else
		return B_ERROR;
}


status_t
Settings::WriteWindowSettings()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) < B_OK)
		return B_ERROR;

	path.Append(kWindowSettingsFile);

	BFile file;
	if (file.SetTo(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE)
		!= B_OK)
		return B_ERROR;

	file.Write(&fWindowPosition, sizeof(BPoint));
	return B_OK;
}


status_t
Settings::ReadSwapSettings()
{
	void* settings = load_driver_settings(kVirtualMemorySettings);
	if (settings == NULL)
		return kErrorSettingsNotFound;

	const char* enabled = get_driver_parameter(settings, "vm", NULL, NULL);
	const char* automatic = get_driver_parameter(settings, "swap_auto",
		NULL, NULL);
	const char* size = get_driver_parameter(settings, "swap_size", NULL, NULL);
	const char* volume = get_driver_parameter(settings, "swap_volume_name",
		NULL, NULL);
	const char* device = get_driver_parameter(settings,
		"swap_volume_device", NULL, NULL);
	const char* filesystem = get_driver_parameter(settings,
		"swap_volume_filesystem", NULL, NULL);
	const char* capacity = get_driver_parameter(settings,
		"swap_volume_capacity", NULL, NULL);

	if (enabled == NULL	|| automatic == NULL || size == NULL || device == NULL
		|| volume == NULL || capacity == NULL || filesystem == NULL)
		return kErrorSettingsInvalid;

	off_t volCapacity = atoll(capacity);

	SetSwapEnabled(get_driver_boolean_parameter(settings,
		"vm", true, false));
	SetSwapAutomatic(get_driver_boolean_parameter(settings,
		"swap_auto", true, false));
	SetSwapSize(atoll(size));
	unload_driver_settings(settings);

	int32 bestScore = -1;
	dev_t bestVol = -1;

	BVolume vol;
	fs_info volStat;
	BVolumeRoster roster;
	while (roster.GetNextVolume(&vol) == B_OK) {
		if (!vol.IsPersistent() || vol.IsReadOnly() || vol.IsRemovable()
			|| vol.IsShared())
			continue;
		if (fs_stat_dev(vol.Device(), &volStat) == 0) {
			int32 score = 0;
			if (strcmp(volume, volStat.volume_name) == 0)
				score += 4;
			if (strcmp(device, volStat.device_name) == 0)
				score += 3;
			if (volCapacity == volStat.total_blocks * volStat.block_size)
				score += 2;
			if (strcmp(filesystem, volStat.fsh_name) == 0)
				score += 1;
			if (score >= 4 && score > bestScore) {
				bestVol = vol.Device();
				bestScore = score;
			}
		}
	}

	SetSwapVolume(bestVol);
	fInitialSettings = fCurrentSettings;

	if (bestVol < 0)
		return kErrorVolumeNotFound;
	else
		return B_OK;
}


status_t
Settings::WriteSwapSettings()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return B_ERROR;

	path.Append("kernel/drivers");
	path.Append(kVirtualMemorySettings);

	BFile file;
	if (file.SetTo(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE)
		!= B_OK)
		return B_ERROR;

	fs_info info;
	fs_stat_dev(SwapVolume(), &info);

	char buffer[1024];
	snprintf(buffer, sizeof(buffer), "vm %s\nswap_auto %s\nswap_size %lld\n"
		"swap_volume_name %s\nswap_volume_device %s\n"
		"swap_volume_filesystem %s\nswap_volume_capacity %lld\n",
		SwapEnabled() ? "on" : "off", SwapAutomatic() ? "yes" : "no",
		SwapSize(), info.volume_name, info.device_name, info.fsh_name,
		info.total_blocks * info.block_size);

	file.Write(buffer, strlen(buffer));
	return B_OK;
}


bool
Settings::IsRevertable()
{
	return SwapEnabled() != fInitialSettings.enabled
		|| SwapAutomatic() != fInitialSettings.automatic
		|| SwapSize() != fInitialSettings.size
		|| SwapVolume() != fInitialSettings.volume;
}


void
Settings::RevertSwapSettings()
{
	SetSwapEnabled(fInitialSettings.enabled);
	SetSwapAutomatic(fInitialSettings.automatic);
	SetSwapSize(fInitialSettings.size);
	SetSwapVolume(fInitialSettings.volume);
}


bool
Settings::IsDefaultable()
{
	return SwapEnabled() != fDefaultSettings.enabled
		|| SwapAutomatic() != fDefaultSettings.automatic
		|| SwapSize() != fDefaultSettings.size
		|| SwapVolume() != fDefaultSettings.volume;
}


void
Settings::DefaultSwapSettings(bool revertable)
{
	SetSwapEnabled(fDefaultSettings.enabled);
	SetSwapAutomatic(fDefaultSettings.automatic);
	SetSwapSize(fDefaultSettings.size);
	SetSwapVolume(fDefaultSettings.volume);
	if (!revertable)
		fInitialSettings = fDefaultSettings;
}
