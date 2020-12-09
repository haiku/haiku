/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackageSettingsItem.h"

#include <driver_settings.h>

#include <AutoDeleterDrivers.h>
#include <boot/vfs.h>
#include <system/directories.h>


namespace PackageFS {


// #pragma mark - PackageSettingsItem


PackageSettingsItem::PackageSettingsItem()
	:
	fEntries(),
	fHashNext(NULL)
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


/*static*/ PackageSettingsItem*
PackageSettingsItem::Load(::Directory* systemDirectory, const char* name)
{
	// open the driver settings file
	const char* settingsFilePath
		= &(kSystemSettingsDirectory "/packages")[strlen(kSystemDirectory) + 1];

	int fd = open_from(systemDirectory, settingsFilePath, B_READ_ONLY, 0);
	if (fd < 0)
		return NULL;
	FileDescriptorCloser fdCloser(fd);

	// load the driver settings
	void* settingsHandle = load_driver_settings_file(fd);
	if (settingsHandle == NULL)
		return NULL;
	DriverSettingsUnloader settingsDeleter(settingsHandle);

	const driver_settings* settings = get_driver_settings(settingsHandle);
	for (int i = 0; i < settings->parameter_count; i++) {
		const driver_parameter& parameter = settings->parameters[i];
		if (strcmp(parameter.name, "Package") != 0
			|| parameter.value_count < 1
			|| strcmp(parameter.values[0], name) != 0) {
			continue;
		}

		PackageSettingsItem* settingsItem
			= new(std::nothrow) PackageSettingsItem;
		if (settingsItem == NULL || settingsItem->Init(parameter) != B_OK) {
			delete settingsItem;
			return NULL;
		}

		return settingsItem;
	}

	return NULL;
}


status_t
PackageSettingsItem::Init(const driver_parameter& parameter)
{
	if (fEntries.Init() != B_OK)
		return B_NO_MEMORY;

	for (int i = 0; i < parameter.parameter_count; i++) {
		const driver_parameter& subParameter = parameter.parameters[i];
		if (strcmp(subParameter.name, "EntryBlacklist") != 0)
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

		const char* name = path;
		size_t nameLength = componentEnd - path;

		Entry* entry = FindEntry(parent, name, nameLength);
		if (entry == NULL) {
			entry = new(std::nothrow) Entry(parent);
			if (entry == NULL || !entry->SetName(name, nameLength)) {
				delete entry;
				return B_NO_MEMORY;
			}
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
PackageSettingsItem::FindEntry(Entry* parent, const char* name) const
{
	return fEntries.Lookup(EntryKey(parent, name));
}


PackageSettingsItem::Entry*
PackageSettingsItem::FindEntry(Entry* parent, const char* name,
	size_t nameLength) const
{
	return fEntries.Lookup(EntryKey(parent, name, nameLength));
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


}	// namespace PackageFS
