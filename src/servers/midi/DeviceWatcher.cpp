/*
 * Copyright 2004-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Matthijs Hollemans
 *		Jerome Leveque
 *		Philippe Houdoin
 *		Pete Goodeve
 */

#include "debug.h"
#include "DeviceWatcher.h"
#include "PortDrivers.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <new>

#include <Application.h>
#include <Bitmap.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <IconUtils.h>
#include <Path.h>
#include <PathMonitor.h>
#include <Resources.h>
#include <Roster.h>

using std::nothrow;

using namespace BPrivate;
using BPrivate::HashMap;
using BPrivate::HashString;


const char *kDevicesRoot = "/dev/midi";


class DeviceEndpoints {
public:
	DeviceEndpoints(int fd, MidiPortConsumer* consumer, MidiPortProducer* producer)
		:  fFD(fd), fConsumer(consumer), fProducer(producer)
	{
	}

	int 				fFD;
	MidiPortConsumer*	fConsumer;
	MidiPortProducer*	fProducer;
};



DeviceWatcher::DeviceWatcher()
	: BLooper("MIDI devices watcher"),
	fDeviceEndpointsMap(), fVectorIconData(NULL), fVectorIconDataSize(0),
	fLargeIcon(NULL), fMiniIcon(NULL)
{
	// Load midi endpoint vector icon data
	app_info info;
	be_app->GetAppInfo(&info);
	BFile file(&info.ref, B_READ_ONLY);

	BResources resources;
	if (resources.SetTo(&file) == B_OK) {
		size_t dataSize;
		// Load MIDI port endpoint vector icon
		const uint8* data = (const uint8*)resources.LoadResource(
			B_VECTOR_ICON_TYPE,	"endpoint_vector_icon", &dataSize);

		if (data != NULL && dataSize > 0)
			fVectorIconData = new(std::nothrow) uint8[dataSize];

		if (fVectorIconData) {
			// data is own by resources local object: copy its content for
			// later use
			memcpy(fVectorIconData, data, dataSize);
			fVectorIconDataSize = dataSize;
		}
	}

	// Render 32x32 and 16x16 B_CMAP8 icons for R5 compatibility
	if (fVectorIconData != NULL) {
		fLargeIcon = new(std::nothrow) BBitmap(BRect(0, 0, 31, 31), B_CMAP8);
		fMiniIcon  = new(std::nothrow) BBitmap(BRect(0, 0, 15, 15), B_CMAP8);

		if (BIconUtils::GetVectorIcon(fVectorIconData, fVectorIconDataSize,
			fLargeIcon) != B_OK) {
			delete fLargeIcon;
			fLargeIcon = NULL;
		}
		if (BIconUtils::GetVectorIcon(fVectorIconData, fVectorIconDataSize,
			fMiniIcon) != B_OK) {
			delete fMiniIcon;
			fMiniIcon = NULL;
		}
	}

	Start();
}


DeviceWatcher::~DeviceWatcher()
{
	Stop();

	delete fLargeIcon;
	delete fMiniIcon;
	delete[] fVectorIconData;
}


status_t
DeviceWatcher::Start()
{
	// Do an initial scan

	// We need to do this from a separate thread, otherwise we will deadlock.
	// The reason is that we instantiate a BMidiRoster object, which sends a
	// message to the midi_server to register itself, and blocks until it gets
	// a response. But since we _are_ the midi_server we will never be able to
	// send that response if our main thread is already blocking.

    resume_thread(spawn_thread(_InitialDevicesScanThread,
		"Initial devices scan", B_NORMAL_PRIORITY, this));

	// And watch for any change
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


/* static  */
int32
DeviceWatcher::_InitialDevicesScanThread(void* data)
{
	((DeviceWatcher*)data)->_ScanDevices(kDevicesRoot);
	return 0;
}


void
DeviceWatcher::_ScanDevices(const char* path)
{
	TRACE(("DeviceWatcher::_ScanDevices(\"%s\");\n", path));

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
	TRACE(("DeviceWatcher::_AddDevice(\"%s\");\n", path));

	if (fDeviceEndpointsMap.ContainsKey(path)) {
		// Already known
		TRACE(("already known...!\n"));
		return;
	}

	BEntry entry(path);
	if (entry.IsDirectory())
		// Invalid path!
		return;

	if (entry.IsSymLink()) {
		BEntry symlink(path, true);
		if (symlink.IsDirectory()) {
			// Invalid path!
			return;
		}
	}

	int fd = open(path, O_RDWR | O_EXCL);
	if (fd < 0) {
		if (errno != EACCES)
			return;

		// maybe it's a input or output only port?
		fd = open(path, O_RDONLY | O_EXCL);
		if (fd < 0 && errno == EACCES)
			fd = open(path, O_WRONLY | O_EXCL);
		if (fd < 0)
			return;
	}

	TRACE(("Doing _AddDevice(\"%s\"); fd=%d\n", path, fd));

	MidiPortConsumer* consumer = NULL;
	MidiPortProducer* producer = NULL;

	int flags = fcntl(fd, F_GETFL);

	if ((flags & O_ACCMODE) != O_RDONLY) {
		consumer = new MidiPortConsumer(fd, path);
		_SetIcons(consumer);
		TRACE(("Register %s MidiPortConsumer\n", consumer->Name()));
		consumer->Register();
	}

	if ((flags & O_ACCMODE) != O_WRONLY) {
		producer = new MidiPortProducer(fd, path);
		_SetIcons(producer);
		TRACE(("Register %s MidiPortProducer\n", producer->Name()));
		producer->Register();
	}

	fDeviceEndpointsMap.Put(path, new DeviceEndpoints(fd, consumer, producer));
	TRACE(("Done _AddDevice(\"%s\")\n", path));
}


void
DeviceWatcher::_RemoveDevice(const char* path)
{
	TRACE(("DeviceWatcher::_RemoveDevice(\"%s\");\n", path));

	DeviceEndpoints* deviceEndpoints = fDeviceEndpointsMap.Get(path);
	if (!deviceEndpoints) {
		TRACE(("_RemoveDevice(\"%s\") didn't find endpoint in map!!\n", path));
		return;
	}

	TRACE((" _RemoveDevice(\"%s\") unregistering\n", path));
	if (deviceEndpoints->fConsumer)
		deviceEndpoints->fConsumer->Unregister();
	if (deviceEndpoints->fProducer)
		deviceEndpoints->fProducer->Unregister();

	TRACE((" _RemoveDevice(\"%s\") releasing\n", path));
	if (deviceEndpoints->fConsumer)
		deviceEndpoints->fConsumer->Release();
	if (deviceEndpoints->fProducer)
		deviceEndpoints->fProducer->Release();

	TRACE((" _RemoveDevice(\"%s\") removing from map\n", path));
	fDeviceEndpointsMap.Remove(path);
	TRACE(("Done _RemoveDevice(\"%s\")\n", path));
}


void
DeviceWatcher::_SetIcons(BMidiEndpoint* endpoint)
{
	BMessage msg;

	if (fVectorIconData && fVectorIconDataSize > 0) {
		msg.AddData("icon", B_VECTOR_ICON_TYPE, fVectorIconData,
			fVectorIconDataSize);
	}

	if (fLargeIcon) {
		msg.AddData("be:large_icon", B_LARGE_ICON_TYPE, fLargeIcon->Bits(),
			fLargeIcon->BitsLength());
	}

	if (fMiniIcon) {
		msg.AddData("be:mini_icon", B_MINI_ICON_TYPE, fMiniIcon->Bits(),
			fMiniIcon->BitsLength());
	}

	endpoint->SetProperties(&msg);
}
