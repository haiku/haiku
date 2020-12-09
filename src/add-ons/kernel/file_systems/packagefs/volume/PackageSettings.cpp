/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackageSettings.h"

#include <driver_settings.h>

#include <AutoDeleterDrivers.h>
#include <directories.h>
#include <fs/KPath.h>
#include <vfs.h>

#include "DebugSupport.h"


static const char* const kEntryBlacklistParameterName = "EntryBlacklist";


// #pragma mark - PackageSettingsItem


PackageSettingsItem::PackageSettingsItem()
	:
	fName(),
	fEntries()
{
}


PackageSettingsItem::~PackageSettingsItem()
{
	Entry* entry = fEntries.Clear(true);
	while (entry != NULL) {
		Entry* next = entry->HashNext();
		delete entry;
		entry = next;
	}
}


status_t
PackageSettingsItem::Init(const char* name)
{
	if (!fName.SetTo(name) || fEntries.Init() != B_OK)
		RETURN_ERROR(B_NO_MEMORY);
	return B_OK;
}


status_t
PackageSettingsItem::ApplySettings(const driver_parameter* parameters,
	int parameterCount)
{
	for (int i = 0; i < parameterCount; i++) {
		const driver_parameter& subParameter = parameters[i];
		if (strcmp(subParameter.name, kEntryBlacklistParameterName) != 0)
			continue;

		status_t error = _AddBlackListedEntries(subParameter);
		// abort only in case of serious issues (memory shortage)
		if (error == B_NO_MEMORY)
			return error;
	}

	return B_OK;
}


void
PackageSettingsItem::AddEntry(Entry* entry)
{
	fEntries.Insert(entry);
}


status_t
PackageSettingsItem::AddEntry(const char* path, Entry*& _entry)
{
	Entry* parent = NULL;

	while (*path != '\0') {
		while (*path == '/') {
			path++;
			continue;
		}

		const char* componentEnd = strchr(path, '/');
		if (componentEnd == NULL)
			componentEnd = path + strlen(path);

		String name;
		if (!name.SetTo(path, componentEnd - path))
			RETURN_ERROR(B_NO_MEMORY);

		Entry* entry = FindEntry(parent, name);
		if (entry == NULL) {
			entry = new(std::nothrow) Entry(parent, name);
			if (entry == NULL)
				RETURN_ERROR(B_NO_MEMORY);
			AddEntry(entry);
		}

		path = componentEnd;
		parent = entry;
	}

	if (parent == NULL)
		return B_BAD_VALUE;

	_entry = parent;
	return B_OK;
}


PackageSettingsItem::Entry*
PackageSettingsItem::FindEntry(Entry* parent, const String& name) const
{
	return fEntries.Lookup(EntryKey(parent, name));
}


PackageSettingsItem::Entry*
PackageSettingsItem::FindEntry(Entry* parent, const char* name) const
{
	return fEntries.Lookup(EntryKey(parent, name));
}


status_t
PackageSettingsItem::_AddBlackListedEntries(const driver_parameter& parameter)
{
	for (int i = 0; i < parameter.parameter_count; i++) {
		Entry* entry;
		status_t error = AddEntry(parameter.parameters[i].name, entry);
		// abort only in case of serious issues (memory shortage)
		if (error == B_NO_MEMORY)
			return error;

		entry->SetBlackListed(true);
	}

	return B_OK;
}


// #pragma mark - PackageSettings


PackageSettings::PackageSettings()
	:
	fPackageItems()
{
}


PackageSettings::~PackageSettings()
{
	PackageSettingsItem* item = fPackageItems.Clear(true);
	while (item != NULL) {
		PackageSettingsItem* next = item->HashNext();
		delete item;
		item = next;
	}
}


status_t
PackageSettings::Load(dev_t mountPointDeviceID, ino_t mountPointNodeID,
	PackageFSMountType mountType)
{
	status_t error = fPackageItems.Init();
	if (error != B_OK)
		RETURN_ERROR(error);

	// First get the safe mode options. Those apply to the system package.
	if (mountType == PACKAGE_FS_MOUNT_TYPE_SYSTEM) {
		void* settingsHandle = load_driver_settings(B_SAFEMODE_DRIVER_SETTINGS);
		if (settingsHandle != NULL) {
			if (const driver_settings* settings
					= get_driver_settings(settingsHandle)) {
				error = _AddPackageSettingsItem("haiku", settings->parameters,
					settings->parameter_count);
				// abort only in case of serious issues (memory shortage)
				if (error == B_NO_MEMORY)
					return error;
			}
			unload_driver_settings(settingsHandle);
		}
	}

	// get the mount point relative settings file path
	const char* settingsFilePath = mountType == PACKAGE_FS_MOUNT_TYPE_HOME
		? &(kUserSettingsGlobalDirectory "/packages")
			[strlen(kUserConfigDirectory) + 1]
		: &(kSystemSettingsDirectory "/packages")[strlen(kSystemDirectory) + 1];

	// get an absolute path
	KPath path;
	if (path.InitCheck() != B_OK)
		RETURN_ERROR(path.InitCheck());

	error = vfs_entry_ref_to_path(mountPointDeviceID, mountPointNodeID,
		NULL, true, path.LockBuffer(), path.BufferSize());
	if (error != B_OK)
		return error;
	path.UnlockBuffer();

	error = path.Append(settingsFilePath);
	if (error != B_OK)
		return error;

	// load the driver settings
	void* settingsHandle = load_driver_settings(path.Path());
	if (settingsHandle == NULL)
		return B_ENTRY_NOT_FOUND;
	DriverSettingsUnloader settingsDeleter(settingsHandle);

	const driver_settings* settings = get_driver_settings(settingsHandle);
	for (int i = 0; i < settings->parameter_count; i++) {
		const driver_parameter& parameter = settings->parameters[i];
		if (strcmp(parameter.name, "Package") != 0
			|| parameter.value_count < 1) {
			continue;
		}

		error = _AddPackageSettingsItem(parameter.values[0],
			parameter.parameters, parameter.parameter_count);
		// abort only in case of serious issues (memory shortage)
		if (error == B_NO_MEMORY)
			return error;
	}

	return B_OK;
}


const PackageSettingsItem*
PackageSettings::PackageItemFor(const String& name) const
{
	return fPackageItems.Lookup(name);
}


status_t
PackageSettings::_AddPackageSettingsItem(const char* name,
	const driver_parameter* parameters, int parameterCount)
{
	// get/create the package item
	PackageSettingsItem* packageItem = fPackageItems.Lookup(StringKey(name));
	if (packageItem == NULL) {
		packageItem = new(std::nothrow) PackageSettingsItem;
		if (packageItem == NULL || packageItem->Init(name) != B_OK) {
			delete packageItem;
			RETURN_ERROR(B_NO_MEMORY);
		}

		fPackageItems.Insert(packageItem);
	}

	// apply the settings
	return packageItem->ApplySettings(parameters, parameterCount);
}
