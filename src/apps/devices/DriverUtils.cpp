/*
 * Copyright 2026 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Leo Rouleau
 */

#include "DriverUtils.h"

#include <FindDirectory.h>
#include <Message.h>
#include <Path.h>

#include <DriverSettingsMessageAdapter.h>
#include <driver_settings.h>
#include <package/PackageInfo.h>

#include "Device.h"

static const char* kCriticalDriverSubsystems[] = {"bus_managers/", "busses/", "drivers/disk/",
	"drivers/power/", "drivers/timer/", "drivers/bus/", "drivers/common/", "drivers/input/", NULL};

static const char* kCriticalDriverModules[] = {"acpi", "pci", "root", NULL};

// templates used for DriverSettingsMessageAdapter

const static settings_template kEmptyTemplate[] = {{0, NULL, NULL}};

const static settings_template kBlockedEntriesTemplate[]
	= {{B_MESSAGE_TYPE, NULL, kEmptyTemplate}, {0, NULL, NULL}};

const static settings_template kPackageTemplate[] = {{B_STRING_TYPE, "name", NULL, true},
	{B_MESSAGE_TYPE, "BlockedEntries", kBlockedEntriesTemplate},
	{B_MESSAGE_TYPE, "EntryBlacklist", kBlockedEntriesTemplate}, {0, NULL, NULL}};

const static settings_template kPackagesTemplate[]
	= {{B_MESSAGE_TYPE, "Package", kPackageTemplate}, {0, NULL, NULL}};

namespace DriverUtils {


status_t
UpdatePackageBlockedEntry(const char* settingsPath, const char* packageName,
	const char* relativePath, bool block)
{
	DriverSettingsMessageAdapter adapter;
	BMessage message;

	status_t status = adapter.ConvertFromDriverSettings(settingsPath, kPackagesTemplate, message);

	if (status != B_OK && status != B_ENTRY_NOT_FOUND)
		return status;

	BMessage packageMsg;
	int32 packageIndex = -1;
	for (int32 i = 0; message.FindMessage("Package", i, &packageMsg) == B_OK; i++) {
		BString name;
		if (packageMsg.FindString("name", &name) == B_OK && name == packageName) {
			packageIndex = i;
			break;
		}
	}

	if (packageIndex < 0) {
		packageMsg.MakeEmpty();
		packageMsg.AddString("name", packageName);
	}

	// replace old EntryBlacklist with BlockedEntries if needed
	BMessage blockedMsg;
	if (packageMsg.FindMessage("BlockedEntries", &blockedMsg) != B_OK) {
		if (packageMsg.FindMessage("EntryBlacklist", &blockedMsg) == B_OK)
			packageMsg.RemoveName("EntryBlacklist");
	}

	if (block) {
		BMessage emptyMsg;
		if (!blockedMsg.HasMessage(relativePath))
			blockedMsg.AddMessage(relativePath, &emptyMsg);
	} else {
		blockedMsg.RemoveName(relativePath);
	}

	packageMsg.RemoveName("BlockedEntries");
	if (!blockedMsg.IsEmpty())
		packageMsg.AddMessage("BlockedEntries", &blockedMsg);

	if (packageIndex >= 0) {
		if (blockedMsg.IsEmpty())
			message.RemoveData("Package", packageIndex);
		else
			message.ReplaceMessage("Package", packageIndex, &packageMsg);
	} else if (!blockedMsg.IsEmpty()) {
		message.AddMessage("Package", &packageMsg);
	}

	return adapter.ConvertToDriverSettings(settingsPath, kPackagesTemplate, message);
}


status_t
GetDriverPackageSettings(const BString& driver, BPath& settingsPath, BString& relativePath)
{
	BPath systemPath;
	status_t status = find_directory(B_SYSTEM_DIRECTORY, &systemPath);
	if (status != B_OK)
		return status;

	BPath userPath;
	status = find_directory(B_USER_DIRECTORY, &userPath);
	if (status != B_OK)
		return status;

	relativePath = driver;

	if (driver.StartsWith(systemPath.Path())) {
		status = find_directory(B_SYSTEM_SETTINGS_DIRECTORY, &settingsPath);
		if (status != B_OK)
			return status;
		settingsPath.Append("packages");
		relativePath.RemoveFirst(systemPath.Path());
	} else if (driver.StartsWith(userPath.Path())) {
		status = find_directory(B_USER_SETTINGS_DIRECTORY, &settingsPath);
		if (status != B_OK)
			return status;
		settingsPath.Append("global/packages");
		relativePath.RemoveFirst(userPath.Path());
	} else {
		return B_ENTRY_NOT_FOUND;
	}

	if (relativePath.StartsWith("/"))
		relativePath.Remove(0, 1);

	return B_OK;
}


bool
IsDriverEnabled(const BString& driver)
{
	BPath settingsPath;
	BString relativePath;
	if (GetDriverPackageSettings(driver, settingsPath, relativePath) != B_OK)
		return true;

	BString packageName;
	if (!IsPackagedDriver(driver, &packageName))
		return true;

	DriverSettingsMessageAdapter adapter;
	BMessage message;
	if (adapter.ConvertFromDriverSettings(settingsPath.Path(), kPackagesTemplate, message) != B_OK)
		return true;

	BMessage packageMsg;
	for (int32 i = 0; message.FindMessage("Package", i, &packageMsg) == B_OK; i++) {
		BString name;
		if (packageMsg.FindString("name", &name) == B_OK && name == packageName) {
			BMessage blockedMsg;
			if (packageMsg.FindMessage("BlockedEntries", &blockedMsg) == B_OK
				|| packageMsg.FindMessage("EntryBlacklist", &blockedMsg) == B_OK) {
				return !blockedMsg.HasMessage(relativePath.String());
			}
			return true;
		}
	}

	return true;
}


bool
IsPackagedDriver(const BString& driver, BString* outPackageName)
{
	BPath settingsPath;
	BString relativePath;
	if (GetDriverPackageSettings(driver, settingsPath, relativePath) != B_OK)
		return false;

	char packagePath[B_PATH_NAME_LENGTH];
	status_t status = find_path_for_path(driver.String(), B_FIND_PATH_PACKAGE_PATH, NULL,
		packagePath, sizeof(packagePath));
	if (status != B_OK)
		return false;

	BPackageKit::BPackageInfo packageInfo;
	status = packageInfo.ReadFromPackageFile(packagePath);
	if (status != B_OK)
		return false;

	if (outPackageName != NULL)
		*outPackageName = packageInfo.Name();

	return true;
}


bool
IsCriticalDriver(Device* device)
{
	if (device == NULL)
		return false;

	Category category = device->GetCategory();
	if (category == CAT_ACPI || category == CAT_COMPUTER || category == CAT_BUS
		|| category == CAT_MASS || category == CAT_MEMORY || category == CAT_CPU
		|| category == CAT_INTEL || category == CAT_GENERIC) {
		return true;
	}

	BString driver = device->GetDriverUsed();
	if (driver.IsEmpty())
		return false;

	driver.ToLower();

	for (size_t i = 0; kCriticalDriverSubsystems[i] != NULL; i++) {
		if (driver.FindFirst(kCriticalDriverSubsystems[i]) != B_ERROR)
			return true;
	}

	for (size_t i = 0; kCriticalDriverModules[i] != NULL; i++) {
		const char* pattern = kCriticalDriverModules[i];
		BString patternPath("/");
		patternPath << pattern;
		if (driver == pattern || driver.EndsWith(patternPath))
			return true;
	}

	return false;
}

} // namespace DriverUtils
