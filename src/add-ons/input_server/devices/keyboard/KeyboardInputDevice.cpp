/*****************************************************************************/
// Mouse input server device addon
// Written by Stefano Ceccherini
//
// KeyboardInputDevice.cpp
//
// Copyright (c) 2004 Haiku Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/
#include "KeyboardInputDevice.h"

#include <Directory.h>
#include <Entry.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <String.h>
#include <stdlib.h>
#include <unistd.h>

#define DEBUG 1
#if DEBUG
	inline void LOG(const char *fmt, ...) { char buf[1024]; va_list ap; va_start(ap, fmt); vsprintf(buf, fmt, ap); va_end(ap); \
		fputs(buf, KeyboardInputDevice::sLogFile); fflush(KeyboardInputDevice::sLogFile); }
#else
	#define LOG(text)
#endif

const static uint32 kSetLeds = 0x2711;
const static uint32 kSetRepeatingKey = 0x2712;
const static uint32 kSetNonRepeatingKey = 0x2713;
const static uint32 kSetKeyRepeatRate = 0x2714;
const static uint32 kSetKeyRepeatDelay = 0x2716;
const static uint32 kGetNextKey = 0x270f;

const static uint32 kKeyboardThreadPriority = B_FIRST_REAL_TIME_PRIORITY + 4;
const static char *kKeyboardDevicesDirectory = "/dev/input/keyboard";

// "/dev/" is automatically prepended by StartMonitoringDevice()
const static char *kKeyboardDevicesDirectoryUSB = "input/keyboard/usb";

FILE *KeyboardInputDevice::sLogFile = NULL;

struct keyboard_device {
	KeyboardInputDevice *owner;
	input_device_ref device_ref;
	char path[B_PATH_NAME_LENGTH];
	int fd;
	thread_id device_watcher;
	kb_settings settings;
	bool active;
};

extern "C"
BInputServerDevice *
instantiate_input_device()
{
	return new KeyboardInputDevice();
}


KeyboardInputDevice::KeyboardInputDevice()
{

#if DEBUG
	if (sLogFile == NULL)
		sLogFile = fopen("/var/log/keyboard_device_log.log", "a");
#endif

	StartMonitoringDevice(kKeyboardDevicesDirectoryUSB);
}


KeyboardInputDevice::~KeyboardInputDevice()
{
	StopMonitoringDevice(kKeyboardDevicesDirectoryUSB);
	
#if DEBUG	
	fclose(sLogFile);
#endif
}


status_t
KeyboardInputDevice::InitFromSettings(void *cookie, uint32 opcode)
{
	// retrieve current values

	return B_OK;
}


status_t
KeyboardInputDevice::InitCheck()
{
	RecursiveScan(kKeyboardDevicesDirectory);
	
	return B_OK;
}


status_t
KeyboardInputDevice::Start(const char *name, void *cookie)
{
	keyboard_device *device = (keyboard_device *)cookie;
	
	char threadName[B_OS_NAME_LENGTH];
	snprintf(threadName, B_OS_NAME_LENGTH, "%s watcher", name);
	
	device->active = true;
	device->device_watcher = spawn_thread(DeviceWatcher, threadName,
									kKeyboardThreadPriority, device);
	
	resume_thread(device->device_watcher);
	
	return B_OK;
}


status_t
KeyboardInputDevice::Stop(const char *name, void *cookie)
{
	keyboard_device *device = (keyboard_device *)cookie;
	
	LOG("Stop(%s)\n", name);
	
	device->active = false;
	if (device->device_watcher >= 0) {
		status_t dummy;
		wait_for_thread(device->device_watcher, &dummy);
	}
	
	return B_OK;
}


status_t
KeyboardInputDevice::Control(const char *name, void *cookie,
						  uint32 command, BMessage *message)
{
	LOG("Control(%s, code: %lu)\n", name, command);

	if (command == B_NODE_MONITOR)
		HandleMonitor(message);
	else if (command >= B_KEY_MAP_CHANGED 
		&& command <= B_KEY_REPEAT_RATE_CHANGED) {
		InitFromSettings(cookie, command);
	}
	return B_OK;
}


status_t 
KeyboardInputDevice::HandleMonitor(BMessage *message)
{
	int32 opcode = 0;
	status_t status;
	if ((status = message->FindInt32("opcode", &opcode)) < B_OK)
		return status;
	
	if ((opcode != B_ENTRY_CREATED) 
		&& (opcode != B_ENTRY_REMOVED))
		return B_OK;
		
	
	BEntry entry;
	BPath path;
	dev_t device;
	ino_t directory;
	const char *name = NULL;
		
	message->FindInt32("device", &device);
	message->FindInt64("directory", &directory);
	message->FindString("name", &name);
			
	entry_ref ref(device, directory, name);
			
	if ((status = entry.SetTo(&ref)) != B_OK)
		return status;
	if ((status = entry.GetPath(&path)) != B_OK)
		return status;
	if ((status = path.InitCheck()) != B_OK)
		return status;
	
	if (opcode == B_ENTRY_CREATED)
		AddDevice(path.Path());
	else
		RemoveDevice(path.Path());
	
	return status;
}


status_t
KeyboardInputDevice::AddDevice(const char *path)
{
	keyboard_device *device = new keyboard_device();
	if (!device)
		return B_NO_MEMORY;
	
	if ((device->fd = open(path, O_RDWR)) < B_OK) {
		delete device;
		return B_ERROR;
	}
		
	device->fd = -1;
	device->device_watcher = -1;
	device->active = false;
	strcpy(device->path, path);
	device->device_ref.name = GetShortName(path);
	device->device_ref.type = B_KEYBOARD_DEVICE;
	device->device_ref.cookie = device;
	device->owner = this;
		
	input_device_ref *devices[2];
	devices[0] = &device->device_ref;
	devices[1] = NULL;
	
	fDevices.AddItem(device);
	
	InitFromSettings(device);
	
	return RegisterDevices(devices);
}


status_t
KeyboardInputDevice::RemoveDevice(const char *path)
{
	int32 i = 0;
	keyboard_device *device = NULL;
	while ((device = (keyboard_device *)fDevices.ItemAt(i)) != NULL) {
		if (!strcmp(device->path, path)) {
			free(device->device_ref.name);
			if (device->fd >= 0)
				close(device->fd);
			fDevices.RemoveItem(device);
			delete device;
			return B_OK;
		}	
	}
	
	return B_ENTRY_NOT_FOUND;
}


int32
KeyboardInputDevice::DeviceWatcher(void *arg)
{
	keyboard_device *dev = (keyboard_device *)arg;
	uint8 buffer[12];
	
	while (dev->active) {
		if (ioctl(dev->fd, kGetNextKey, &buffer) < B_OK)
			continue;
		LOG("kGetNextKey : %lx%lx%lx\n", (uint32*)buffer, (uint32*)buffer+1, (uint32*)buffer+2);
	}
	
	return 0;
}


char *
KeyboardInputDevice::GetShortName(const char *longName)
{
	BString string(longName);
	BString name;
	
	int32 slash = string.FindLast("/");
	int32 previousSlash = string.FindLast("/", slash) + 1;
	string.CopyInto(name, slash, string.Length() - slash);
	
	int32 deviceIndex = atoi(name.String()) + 1;
	string.CopyInto(name, previousSlash, slash - previousSlash); 
	name << " Keyboard " << deviceIndex;
		
	return strdup(name.String());
}


void
KeyboardInputDevice::RecursiveScan(const char *directory)
{
	BEntry entry;
	BDirectory dir(directory);
	while (dir.GetNextEntry(&entry) == B_OK) {
		BPath path;
		entry.GetPath(&path);
		if (entry.IsDirectory())
			RecursiveScan(path.Path());
		else
			AddDevice(path.Path());
	}
}
