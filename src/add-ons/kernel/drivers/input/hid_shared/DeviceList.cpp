/*
	Generic device list for use in drivers.
	Copyright (C) 2008 Michael Lotz <mmlr@mlotz.ch>
	Distributed under the terms of the MIT license.
*/
#include "DeviceList.h"
#include <util/kernel_cpp.h>
#include <stdlib.h>
#include <string.h>
#include <new>

struct device_list_entry {
	char *				name;
	void *				device;
	device_list_entry *	next;
};


DeviceList::DeviceList()
	:	fDeviceList(NULL),
		fDeviceCount(0),
		fPublishList(NULL)
{
}


DeviceList::~DeviceList()
{
	_FreePublishList();

	device_list_entry *current = fDeviceList;
	while (current) {
		device_list_entry *next = current->next;
		free(current->name);
		delete current;
		current = next;
	}
}


status_t
DeviceList::AddDevice(const char *name, void *device)
{
	device_list_entry *entry = new(std::nothrow) device_list_entry;
	if (entry == NULL)
		return B_NO_MEMORY;

	entry->name = strdup(name);
	if (entry->name == NULL) {
		delete entry;
		return B_NO_MEMORY;
	}

	entry->device = device;
	entry->next = NULL;

	if (fDeviceList == NULL)
		fDeviceList = entry;
	else {
		device_list_entry *current = fDeviceList;
		while (current) {
			if (current->next == NULL) {
				current->next = entry;
				break;
			}

			current = current->next;
		}
	}

	fDeviceCount++;
	return B_OK;
}


status_t
DeviceList::RemoveDevice(const char *name, void *device)
{
	if (name == NULL && device == NULL)
		return B_BAD_VALUE;

	device_list_entry *previous = NULL;
	device_list_entry *current = fDeviceList;
	while (current) {
		if ((name != NULL && strcmp(current->name, name) == 0)
			|| (device != NULL && current->device == device)) {
			if (previous == NULL)
				fDeviceList = current->next;
			else
				previous->next = current->next;

			free(current->name);
			delete current;
			fDeviceCount--;
			return B_OK;
		}

		previous = current;
		current = current->next;
	}

	return B_ENTRY_NOT_FOUND;
}


void *
DeviceList::FindDevice(const char *name, void *device)
{
	if (name == NULL && device == NULL)
		return NULL;

	device_list_entry *current = fDeviceList;
	while (current) {
		if ((name != NULL && strcmp(current->name, name) == 0)
			|| (device != NULL && current->device == device))
			return current->device;

		current = current->next;
	}

	return NULL;
}


int32
DeviceList::CountDevices(const char *baseName)
{
	if (baseName == NULL)
		return fDeviceCount;

	int32 count = 0;
	int32 baseNameLength = strlen(baseName);
	device_list_entry *current = fDeviceList;
	while (current) {
		if (strncmp(current->name, baseName, baseNameLength) == 0)
			count++;

		current = current->next;
	}

	return count;
}


void *
DeviceList::DeviceAt(int32 index)
{
	device_list_entry *current = fDeviceList;
	while (current) {
		if (index-- == 0)
			return current->device;

		current = current->next;
	}

	return NULL;
}


const char **
DeviceList::PublishDevices()
{
	_FreePublishList();

	fPublishList = (char **)malloc((fDeviceCount + 1) * sizeof(char *));
	if (fPublishList == NULL)
		return NULL;

	int32 index = 0;
	device_list_entry *current = fDeviceList;
	while (current) {
		fPublishList[index++] = strdup(current->name);
		current = current->next;
	}

	fPublishList[index] = NULL;
	return (const char **)fPublishList;
}


void
DeviceList::_FreePublishList()
{
	if (fPublishList == NULL)
		return;

	int32 index = 0;
	while (fPublishList[index] != NULL) {
		free(fPublishList[index]);
		index++;
	}

	free(fPublishList);
	fPublishList = NULL;
}
