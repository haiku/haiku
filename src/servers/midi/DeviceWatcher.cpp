/*
 * Copyright 2004-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Matthijs Hollemans
 *		Jerome Leveque
 *		Philippe Houdoin
 */

#include "DeviceWatcher.h"
#include "PortDrivers.h"
#include "debug.h"

#include <Application.h>
#include <Bitmap.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Path.h>
#include <Resources.h>
#include <Roster.h>

#include <PathMonitor.h>

using namespace BPrivate;

const char *kDevicesRoot = "/dev/midi";

DeviceWatcher::DeviceWatcher()
	: BLooper("MIDI devices watcher")
{
	// TODO: add support for vector icons
	
	fLargeIcon = new BBitmap(BRect(0, 0, 31, 31), B_CMAP8);
	fMiniIcon  = new BBitmap(BRect(0, 0, 15, 15), B_CMAP8);

	app_info info;
	be_app->GetAppInfo(&info);
	BFile file(&info.ref, B_READ_ONLY);

	BResources res;
	if (res.SetTo(&file) == B_OK) {
		size_t size;
		const void* bits;

		bits = res.LoadResource(B_LARGE_ICON_TYPE, 10, &size);
		fLargeIcon->SetBits(bits, size, 0, B_CMAP8);

		bits = res.LoadResource(B_MINI_ICON_TYPE, 11, &size);
		fMiniIcon->SetBits(bits, size, 0, B_CMAP8);
	}
	
	Start();
}


DeviceWatcher::~DeviceWatcher()
{
	Stop();
	
	delete fLargeIcon;
	delete fMiniIcon;
}


status_t 
DeviceWatcher::Start()
{
	// Do an initial scan
    _ScanDevices(kDevicesRoot);

	// Okay, now just watch for any change
	return BPathMonitor::StartWatching(kDevicesRoot, B_ENTRY_CREATED
		| B_ENTRY_REMOVED | B_ENTRY_MOVED | B_WATCH_FILES_ONLY
		| B_WATCH_RECURSIVELY, this);
}


status_t 
DeviceWatcher::Stop()
{
	return BPathMonitor::StopWatching(kDevicesRoot, this);
}


void
DeviceWatcher::MessageReceived(BMessage* message)
{
	if (message->what != B_PATH_MONITOR)
		return;
		
	int32 opcode;
	if (message->FindInt32("opcode", &opcode) != B_OK)
		return;
		
	// message->PrintToStream();

	const char* path;
	if (message->FindString("path", &path) != B_OK)
		return;

	switch (opcode) {
		case B_ENTRY_CREATED: {
			_AddDevice(path);
			break;
		}
		case B_ENTRY_REMOVED: {
			_RemoveDevice(path);
			break;
		}
	}
}


// #pragma mark -


void 
DeviceWatcher::_ScanDevices(const char* path)
{
	BDirectory dir(path);
	if (dir.InitCheck() != B_OK)
		return;
	
	BEntry entry;
	while (dir.GetNextEntry(&entry) == B_OK)  {
		BPath name;
		entry.GetPath(&name);
		if (entry.IsDirectory())
			_ScanDevices(name.Path());
		else
           	_AddDevice(name.Path());
	}
}


void
DeviceWatcher::_AddDevice(const char* path)
{
	int fd = open(path, O_RDWR | O_EXCL);
	if (fd < 0)
		return;
		
	// printf("DeviceWatcher::_AddDevice(\"%s\");\n", path);
		
	BMidiEndpoint* endpoint;

	endpoint = new MidiPortConsumer(fd, path);
	_SetIcons(endpoint);
	printf("Register %s MidiPortConsumer\n", endpoint->Name());
	endpoint->Register();

	endpoint = new MidiPortProducer(fd, path);
	_SetIcons(endpoint);
	printf("Register %s MidiPortProducer\n", endpoint->Name());
	endpoint->Register();
}


void
DeviceWatcher::_RemoveDevice(const char* path)
{
	// printf("DeviceWatcher::_RemoveDevice(\"%s\");\n", path);
		
	// TODO: handle device removing
}


void 
DeviceWatcher::_SetIcons(BMidiEndpoint* endpoint)
{
	BMessage msg;

	// TODO: handle Haiku vector icon type

	msg.AddData("be:large_icon", B_LARGE_ICON_TYPE, fLargeIcon->Bits(), 
		fLargeIcon->BitsLength());
	
	msg.AddData("be:mini_icon", B_MINI_ICON_TYPE, fMiniIcon->Bits(), 
		fMiniIcon->BitsLength());

	endpoint->SetProperties(&msg);
}
