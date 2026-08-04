/*
 * Copyright 2004-2026, Haiku, Inc. All rights reserved.
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
#include <drivers/Drivers.h>

using std::nothrow;

using namespace BPrivate;
using BPrivate::HashMap;
using BPrivate::HashString;


const char *kDevicesRoot = "/dev/midi";


class DeviceEndpoints {
public:
	DeviceEndpoints(int fd, MidiPortConsumer* consumer, MidiPortProducer* producer)
		:
		fFD(fd),
		fConsumer(consumer),
		fProducer(producer)
	{
	}

	int 				fFD;
	MidiPortConsumer*	fConsumer;
	MidiPortProducer*	fProducer;
};


DeviceWatcher::DeviceWatcher()
	:
	BLooper("MIDI devices watcher"),
	fDeviceEndpointsMap(),
	fDefaultVectorIconData(NULL),
	fDefaultVectorIconDataSize(0),
	fDefaultLargeIcon(NULL),
	fDefaultMiniIcon(NULL)
{
	// Load default midi device endpoint vector icon data from resources
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
			fDefaultVectorIconData = new(std::nothrow) uint8[dataSize];

		if (fDefaultVectorIconData) {
			// data is own by resources local object: copy its content for
			// later use
			memcpy(fDefaultVectorIconData, data, dataSize);
			fDefaultVectorIconDataSize = dataSize;
		}
	}

	// Render 32x32 and 16x16 B_CMAP8 icons for R5 compatibility
	if (fDefaultVectorIconData != NULL) {
		fDefaultLargeIcon = new(std::nothrow) BBitmap(BRect(0, 0, 31, 31), B_CMAP8);
		fDefaultMiniIcon = new(std::nothrow) BBitmap(BRect(0, 0, 15, 15), B_CMAP8);

		if (BIconUtils::GetVectorIcon(fDefaultVectorIconData, fDefaultVectorIconDataSize,
				fDefaultLargeIcon)
			!= B_OK) {
			delete fDefaultLargeIcon;
			fDefaultLargeIcon = NULL;
		}
		if (BIconUtils::GetVectorIcon(fDefaultVectorIconData, fDefaultVectorIconDataSize,
				fDefaultMiniIcon)
			!= B_OK) {
			delete fDefaultMiniIcon;
			fDefaultMiniIcon = NULL;
		}
	}

	Start();
}


DeviceWatcher::~DeviceWatcher()
{
	Stop();

	delete fDefaultLargeIcon;
	delete fDefaultMiniIcon;
	delete[] fDefaultVectorIconData;
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
	return BPathMonitor::StartWatching(kDevicesRoot,
		B_WATCH_FILES_ONLY | B_WATCH_RECURSIVELY, this);
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
		_SetProperties(fd, path, consumer);
		TRACE(("Register %s MidiPortConsumer\n", consumer->Name()));
		consumer->Register();
	}

	if ((flags & O_ACCMODE) != O_WRONLY) {
		producer = new MidiPortProducer(fd, path);
		_SetProperties(fd, path, producer);
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
DeviceWatcher::_SetProperties(int fd, const char* path, BMidiEndpoint* endpoint)
{
	BMessage msg;

	// add device path to endpoint properties
	msg.AddString("device", path);

	char name[B_FILE_NAME_LENGTH];
	if (ioctl(fd, B_GET_DEVICE_NAME, name, sizeof(name)) == B_OK) {
		// device has a custom, probably more user-friendly name
		// use it instead of the device path
		endpoint->SetName(name);
	}

	// icon(s)

	uint8* vectorIconData;
	size_t vectorIconDataSize;

	BBitmap* largeIcon = NULL;
	BBitmap* miniIcon = NULL;

	if (_GetVectorIcon(fd, &vectorIconData, &vectorIconDataSize) != B_OK) {
		// no custom icon (or failed to retrieve it)
		// use default vector icon, if any
		vectorIconData = fDefaultVectorIconData;
		vectorIconDataSize = fDefaultVectorIconDataSize;
		largeIcon = fDefaultLargeIcon;
		miniIcon = fDefaultMiniIcon;
		TRACE((" _GetVectorIcon(\"%s\") failed to retrieve a custom vector icon, "
				"using default icon\n",
			path));
	} else {
		TRACE((" _GetVectorIcon(\"%s\") retrieve a custom vector icon!\n", path));
	}

	if (vectorIconData && vectorIconDataSize > 0) {
		msg.AddData("icon", B_VECTOR_ICON_TYPE, vectorIconData, vectorIconDataSize);

		if (vectorIconData != fDefaultVectorIconData) {
			// custom vector icon: can't use default large and mini icons
			// so render the custom vector icon into a large and mini icon bitmaps
			largeIcon = new(std::nothrow) BBitmap(BRect(0, 0, 31, 31), B_CMAP8);
			miniIcon = new(std::nothrow) BBitmap(BRect(0, 0, 15, 15), B_CMAP8);

			if (BIconUtils::GetVectorIcon(vectorIconData, vectorIconDataSize, largeIcon) != B_OK) {
				delete largeIcon;
				largeIcon = NULL;
			}
			if (BIconUtils::GetVectorIcon(vectorIconData, vectorIconDataSize, miniIcon) != B_OK) {
				delete miniIcon;
				miniIcon = NULL;
			}
		}
	}

	if (largeIcon)
		msg.AddData("be:large_icon", B_LARGE_ICON_TYPE, largeIcon->Bits(), largeIcon->BitsLength());

	if (miniIcon)
		msg.AddData("be:mini_icon", B_MINI_ICON_TYPE, miniIcon->Bits(), miniIcon->BitsLength());

	// cleanup the custom icon data and custom bitmaps of large and mini icon, if any
	if (vectorIconData != fDefaultVectorIconData)
		delete[] vectorIconData;

	if (largeIcon != fDefaultLargeIcon)
		delete largeIcon;

	if (miniIcon != fDefaultMiniIcon)
		delete miniIcon;

	endpoint->SetProperties(&msg);
}


status_t
DeviceWatcher::_GetVectorIcon(int fd, uint8** _data, size_t* _size)
{
	// NOTE: The actual icon data size is unknown as of yet. After the first call
	// to B_GET_VECTOR_ICON, the actual data size is known and the final buffer
	// is allocated with the correct size. If the buffer needed to be
	// larger, then the temporary buffer above will not yet contain the
	// valid icon data. In that case, a second call to B_GET_VECTOR_ICON
	// retrieves it into the final buffer.

	uint8 data[8192];
	device_icon iconData = {sizeof(data), data};
	status_t status = ioctl(fd, B_GET_VECTOR_ICON, &iconData, sizeof(device_icon));
	if (status != 0)
		status = errno;

	if (status == B_OK) {
		*_data = new(std::nothrow) uint8[iconData.icon_size];
		if (*_data == NULL)
			return B_NO_MEMORY;
	}

	if (status == B_OK) {
		if (iconData.icon_size > (int32)sizeof(data)) {
			// the stack buffer does not contain the data, see NOTE above
			iconData.icon_data = *_data;
			status = ioctl(fd, B_GET_VECTOR_ICON, &iconData, sizeof(device_icon));
			if (status != 0)
				status = errno;
		} else {
			memcpy(*_data, data, iconData.icon_size);
		}

		*_size = iconData.icon_size;
	}

	return status;
}
