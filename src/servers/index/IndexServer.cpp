/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */

#include "IndexServer.h"

#include <Directory.h>
#include <driver_settings.h>
#include <FindDirectory.h>
#include <Path.h>
#include <String.h>

#include <syscalls.h>


VolumeObserverHandler::VolumeObserverHandler(IndexServer* indexServer)
	:
	fIndexServer(indexServer)
{
	
}


void
VolumeObserverHandler::MessageReceived(BMessage* message)
{
	if (message->what != B_NODE_MONITOR)
		return;

	dev_t device;
	int32 opcode;
	message->FindInt32("opcode", &opcode) ;
	switch (opcode) {
		case B_DEVICE_MOUNTED :
			message->FindInt32("new device", &device);
			fIndexServer->AddVolume(BVolume(device));
			break ;
		
		case B_DEVICE_UNMOUNTED :
			message->FindInt32("device", &device);
			fIndexServer->RemoveVolume(BVolume(device));
			break ;
	}
}


AnalyserMonitorHandler::AnalyserMonitorHandler(IndexServer* indexServer)
	:
	fIndexServer(indexServer)
{
	
}


void
AnalyserMonitorHandler::AddOnEnabled(const add_on_entry_info* entryInfo)
{
	entry_ref ref;
	make_entry_ref(entryInfo->dir_nref.device, entryInfo->dir_nref.node,
		entryInfo->name, &ref);
	fIndexServer->RegisterAddOn(ref);
};


void
AnalyserMonitorHandler::AddOnDisabled(const add_on_entry_info* entryInfo)
{
	entry_ref ref;
	make_entry_ref(entryInfo->dir_nref.device, entryInfo->dir_nref.node,
		entryInfo->name, &ref);
	fIndexServer->UnregisterAddOn(ref);
};


IndexServer::IndexServer()
	:
	BApplication("application/x-vnd.Haiku-index_server"),

	fVolumeObserverHandler(this),
	fAddOnMonitorHandler(this),
	fPulseRunner(NULL)
{
	AddHandler(&fVolumeObserverHandler);
	AddHandler(&fAddOnMonitorHandler);
}


IndexServer::~IndexServer()
{
	for (int i = 0; i < fAddOnList.CountItems(); i++) {
		IndexServerAddOn* addon = fAddOnList.ItemAt(i);
		for (int i = 0; i < fVolumeWatcherList.CountItems(); i++)
			fVolumeWatcherList.ItemAt(i)->RemoveAnalyser(addon->Name());
		image_id image = addon->ImageId();
		delete addon;
		unload_add_on(image);
	}

	_StopWatchingVolumes();

	delete fPulseRunner;

	RemoveHandler(&fVolumeObserverHandler);
	RemoveHandler(&fAddOnMonitorHandler);
}


void
IndexServer::ReadyToRun()
{
	_StartWatchingAddOns();
	_StartWatchingVolumes();
}


void
IndexServer::MessageReceived(BMessage *message)
{
	BApplication::MessageReceived(message);
}


bool
IndexServer::QuitRequested()
{
	_StopWatchingVolumes();
	return BApplication::QuitRequested();
}


void
IndexServer::AddVolume(const BVolume& volume)
{
	// ignore volumes like / or /dev
	if (volume.Capacity() == 0)
		return;
	
	// check if volume is already in our list
	for (int i = 0; i < fVolumeWatcherList.CountItems(); i++) {
		VolumeWatcher* current = fVolumeWatcherList.ItemAt(i);
		if (current->Volume() == volume)
			return;
	}

	char name[256];
	volume.GetName(name);
	STRACE("IndexServer::AddVolume %s\n", name);

	VolumeWatcher* watcher = new VolumeWatcher(volume);
/*	if (!watcher->Enabled()) {
		delete watcher;
		return;
	}*/
	fVolumeWatcherList.AddItem(watcher);
	_SetupVolumeWatcher(watcher);
	watcher->StartWatching();
}


void
IndexServer::RemoveVolume(const BVolume& volume)
{
	VolumeWatcher* watcher = NULL;
	for (int i = 0; i < fVolumeWatcherList.CountItems(); i++) {
		VolumeWatcher* current = fVolumeWatcherList.ItemAt(i);
		if (current->Volume() == volume) {
			watcher = current;
			break;
		}
	}

	if (!watcher)
		return;

	watcher->Stop();
	fVolumeWatcherList.RemoveItem(watcher);
	watcher->PostMessage(B_QUIT_REQUESTED);
}


void
IndexServer::RegisterAddOn(entry_ref ref)
{
	STRACE("RegisterAddOn %s\n", ref.name);

	BPath path(&ref);
	image_id image = load_add_on(path.Path());
	if (image < 0)
		return;

	create_index_server_addon* createFunc;

	// Get the instantiation function
	status_t status = get_image_symbol(image, "instantiate_index_server_addon",
		B_SYMBOL_TYPE_TEXT, (void**)&createFunc);
	if (status != B_OK) {
		unload_add_on(image);
		return;
	}

	IndexServerAddOn* addon = createFunc(image, ref.name);
	if (!addon) {
		unload_add_on(image);
		return;
	}
	if (!fAddOnList.AddItem(addon)) {
		unload_add_on(image);
		return;
	}

	for (int i = 0; i < fVolumeWatcherList.CountItems(); i++) {
		VolumeWatcher* watcher = fVolumeWatcherList.ItemAt(i);
		FileAnalyser* analyser = _SetupFileAnalyser(addon, watcher->Volume());
		if (!analyser)
			continue;
		if (!watcher->AddAnalyser(analyser))
			delete analyser;
	}

}


void
IndexServer::UnregisterAddOn(entry_ref ref)
{
	IndexServerAddOn* addon = _FindAddon(ref.name);
	if (!addon)
		return;

	for (int i = 0; i < fVolumeWatcherList.CountItems(); i++)
		fVolumeWatcherList.ItemAt(i)->RemoveAnalyser(addon->Name());

	fAddOnList.RemoveItem(addon);
	unload_add_on(addon->ImageId());
	delete addon;
}


FileAnalyser*
IndexServer::CreateFileAnalyser(const BString& name, const BVolume& volume)
{
	Lock();
	IndexServerAddOn* addon = _FindAddon(name);
	if (!addon) {
		Unlock();
		return NULL;
	}
	FileAnalyser* analyser = addon->CreateFileAnalyser(volume);
	Unlock();
	return analyser;
}


void
IndexServer::_StartWatchingVolumes()
{
	BVolume volume;
	while (fVolumeRoster.GetNextVolume(&volume) != B_BAD_VALUE)
		AddVolume(volume);
	fVolumeRoster.StartWatching(this);
}


void
IndexServer::_StopWatchingVolumes()
{
	STRACE("_StopWatchingVolumes\n");

	for (int i = 0; i < fVolumeWatcherList.CountItems(); i++) {
		VolumeWatcher* watcher = fVolumeWatcherList.ItemAt(i);
		watcher->Stop();
		watcher->PostMessage(B_QUIT_REQUESTED);
	}
	fVolumeWatcherList.MakeEmpty();
}


void
IndexServer::_SetupVolumeWatcher(VolumeWatcher* watcher)
{
	for (int i = 0; i < fAddOnList.CountItems(); i++) {
		IndexServerAddOn* addon = fAddOnList.ItemAt(i);
		FileAnalyser* analyser = _SetupFileAnalyser(addon, watcher->Volume());
		if (!analyser)
			continue;
		if (!watcher->AddAnalyser(analyser))
			delete analyser;
	}
}


FileAnalyser*
IndexServer::_SetupFileAnalyser(IndexServerAddOn* addon, const BVolume& volume)
{
	FileAnalyser* analyser = addon->CreateFileAnalyser(volume);
	if (!analyser)
		return NULL;
	AnalyserSettings* settings = new AnalyserSettings(analyser->Name(),
		analyser->Volume());
	BReference<AnalyserSettings> settingsRef(settings, true);
	if (!settings) {
		delete analyser;
		return NULL;
	}
	analyser->SetSettings(settings);
	return analyser;
}


void
IndexServer::_StartWatchingAddOns()
{
	AddHandler(&fAddOnMonitorHandler);
	BMessage pulse(B_PULSE);
	fPulseRunner = new BMessageRunner(&fAddOnMonitorHandler, &pulse, 1000000LL);
		// the monitor handler needs a pulse to check if add-ons are ready

	char parameter[32];
	size_t parameterLength = sizeof(parameter);
	bool safeMode = false;
	if (_kern_get_safemode_option(B_SAFEMODE_SAFE_MODE, parameter,
			&parameterLength) == B_OK) {
		if (!strcasecmp(parameter, "enabled") || !strcasecmp(parameter, "on")
			|| !strcasecmp(parameter, "true") || !strcasecmp(parameter, "yes")
			|| !strcasecmp(parameter, "enable") || !strcmp(parameter, "1"))
			safeMode = true;
	}

	// load dormant media nodes
	const directory_which directories[] = {
		B_USER_NONPACKAGED_ADDONS_DIRECTORY,
		B_USER_ADDONS_DIRECTORY,
		B_COMMON_NONPACKAGED_ADDONS_DIRECTORY,
		B_COMMON_ADDONS_DIRECTORY,
		B_SYSTEM_ADDONS_DIRECTORY
	};

	// when safemode, only B_SYSTEM_ADDONS_DIRECTORY is used
	for (uint32 i = safeMode ? 4 : 0;
			i < sizeof(directories) / sizeof(directory_which); i++) {
		BDirectory directory;
		node_ref nodeRef;
		BPath path;
		if (find_directory(directories[i], &path) == B_OK
			&& path.Append("index_server") == B_OK
			&& directory.SetTo(path.Path()) == B_OK
			&& directory.GetNodeRef(&nodeRef) == B_OK)
			fAddOnMonitorHandler.AddDirectory(&nodeRef, true);
	}
}


IndexServerAddOn*
IndexServer::_FindAddon(const BString& name)
{
	for (int i = 0; i < fAddOnList.CountItems(); i++) {
		IndexServerAddOn* current = fAddOnList.ItemAt(i);
		if (current->Name() == name)
			return current;
	}
	return NULL;
}
