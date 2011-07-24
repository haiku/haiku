/*
 * Copyright (c) 2002, Jerome Duval (jerome.duval@free.fr)
 * Distributed under the terms of the MIT License.
 */


#include "MultiAudioAddOn.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <File.h>
#include <Path.h>

#include "debug.h"
#include "MultiAudioNode.h"
#include "MultiAudioDevice.h"


#define MULTI_SAVE

const char* kSettingsName = "Media/multi_audio_settings";


//! instantiation function
extern "C" BMediaAddOn*
make_media_addon(image_id image)
{
	CALLED();
	return new MultiAudioAddOn(image);
}


//	#pragma mark -


MultiAudioAddOn::MultiAudioAddOn(image_id image)
	: BMediaAddOn(image),
	fDevices()
{
	CALLED();
	fInitStatus = _RecursiveScan("/dev/audio/hmulti/");
	if (fInitStatus != B_OK)
		return;

	_LoadSettings();
	fInitStatus = B_OK;
}


MultiAudioAddOn::~MultiAudioAddOn()
{
	CALLED();

	void *device = NULL;
	for (int32 i = 0; (device = fDevices.ItemAt(i)); i++)
		delete (MultiAudioDevice*)device;

	_SaveSettings();
}


status_t
MultiAudioAddOn::InitCheck(const char** _failureText)
{
	CALLED();
	return fInitStatus;
}


int32
MultiAudioAddOn::CountFlavors()
{
	CALLED();
	return fDevices.CountItems();
}


status_t
MultiAudioAddOn::GetFlavorAt(int32 index, const flavor_info** _info)
{
	CALLED();

	MultiAudioDevice* device = (MultiAudioDevice*)fDevices.ItemAt(index);
	if (device == NULL)
		return B_BAD_INDEX;

	flavor_info* info = new (std::nothrow) flavor_info;
	if (info == NULL)
		return B_NO_MEMORY;

	MultiAudioNode::GetFlavor(info, index);
	info->name = (char*)device->Description().friendly_name;

	*_info = info;
	return B_OK;
}


BMediaNode*
MultiAudioAddOn::InstantiateNodeFor(const flavor_info* info, BMessage* config,
	status_t* _error)
{
	CALLED();

	MultiAudioDevice* device = (MultiAudioDevice*)fDevices.ItemAt(
		info->internal_id);
	if (device == NULL) {
		*_error = B_ERROR;
		return NULL;
	}

#ifdef MULTI_SAVE
	if (fSettings.FindMessage(device->Description().friendly_name, config)
			== B_OK) {
		fSettings.RemoveData(device->Description().friendly_name);
	}
#endif

	MultiAudioNode* node = new (std::nothrow) MultiAudioNode(this,
		device->Description().friendly_name, device, info->internal_id, config);
	if (node == NULL)
		*_error = B_NO_MEMORY;
	else
		*_error = node->InitCheck();

	return node;
}


status_t
MultiAudioAddOn::GetConfigurationFor(BMediaNode* _node, BMessage* message)
{
	CALLED();
	MultiAudioNode* node = dynamic_cast<MultiAudioNode*>(_node);
	if (node == NULL)
		return B_BAD_TYPE;

#ifdef MULTI_SAVE
	if (message == NULL) {
		BMessage settings;
		if (node->GetConfigurationFor(&settings) == B_OK) {
			fSettings.AddMessage(node->Name(), &settings);
		}
		return B_OK;
	}
#endif

	// currently never called by the media kit. Seems it is not implemented.

	return node->GetConfigurationFor(message);
}


bool
MultiAudioAddOn::WantsAutoStart()
{
	CALLED();
	return false;
}


status_t
MultiAudioAddOn::AutoStart(int count, BMediaNode** _node, int32* _internalID,
	bool* _hasMore)
{
	CALLED();
	return B_OK;
}


status_t
MultiAudioAddOn::_RecursiveScan(const char* rootPath, BEntry* rootEntry, uint32 depth)
{
	CALLED();
	if (depth > 16)
		return B_ERROR;

	BDirectory root;
	if (rootEntry != NULL)
		root.SetTo(rootEntry);
	else if (rootPath != NULL)
		root.SetTo(rootPath);
	else {
		PRINT(("Error in MultiAudioAddOn::RecursiveScan() null params\n"));
		return B_ERROR;
	}

	BEntry entry;
	while (root.GetNextEntry(&entry) == B_OK) {
		if (entry.IsDirectory()) {
			_RecursiveScan(rootPath, &entry, depth + 1);
		} else {
			BPath path;
			entry.GetPath(&path);
			MultiAudioDevice *device = 
				new(std::nothrow) MultiAudioDevice(path.Path()
					+ strlen(rootPath), path.Path());
			if (device) {
				if (device->InitCheck() == B_OK)
					fDevices.AddItem(device);
				else
					delete device;
			}
		}
	}

	return B_OK;
}


void
MultiAudioAddOn::_SaveSettings()
{
	CALLED();
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return;

	path.Append(kSettingsName);

	BFile file(path.Path(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	if (file.InitCheck() == B_OK)
		fSettings.Flatten(&file);
}


void
MultiAudioAddOn::_LoadSettings()
{
	CALLED();
	fSettings.MakeEmpty();

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return;

	path.Append(kSettingsName);

	BFile file(path.Path(), B_READ_ONLY);
	if (file.InitCheck() == B_OK && fSettings.Unflatten(&file) == B_OK) {
		PRINT_OBJECT(fSettings);
	} else {
		PRINT(("Error unflattening settings file %s\n", path.Path()));
	}
}
