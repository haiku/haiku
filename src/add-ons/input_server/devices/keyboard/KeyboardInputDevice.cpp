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
	#define LOG_ERR(text...) LOG(text)
#else
	#define LOG(text)
	#define LOG_ERR(text...) fprintf(stderr, text)
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

const uint32 at_keycode_map[] = {
	0x1,	// Esc
	0x12, 	// 1
	0x13,
	0x14,
	0x15,
	0x16,
	0x17,
	0x18,
	0x19,
	0x1a,
	0x1b,
	0x1c,	// -
	0x1d,	// =
	0x1e,	// BKSP
	0x26,	// TAB
	0x27,	// Q
	0x28,	// W
	0x29,	// E
	0x2a,	// R
	0x2b,	// T
	0x2c,	// Y
	0x2d,	// U
	0x2e,	// I
	0x2f,	// O
	0x30,	// P 
	0x31,   // [
	0x32,	// ]
	0x47,	// ENTER
	0x5c,	// Left Control
	0x3c,	// A
	0x3d,	// S
	0x3e,	// D
	0x3f,	// F
	0x40,	// G
	0x41,	// H
	0x42,	// J
	0x43,	// K
	0x44,	// L
	0x45,	// ;
	0x46,	// '
	0x11,	// `
	0x4b,	// Left Shift
	0x33, 	// \ 
	0x4c,	// Z
	0x4d,	// X
	0x4e,	// C
	0x4f,	// V
	0x50,	// B
	0x51,	// N
	0x52,	// M
	0x53,	// ,
	0x54,	// .
	0x55,	// /
	0x56,	// Right Shift
	0x24,	// *
	0x5d,	// Left Alt
	0x5e,	// Space
	0x3b,	// Caps
	0x02,	// F1
	0x03,	// F2
	0x04,	// F3
	0x05,	// F4
	0x06,	// F5
	0x07,	// F6
	0x08,	// F7
	0x09,	// F8
	0x0a,	// F9
	0x0b,	// F10
	0x22,	// Num
	0x0f,	// Scroll
	0x37,	// KP 7
	0x38,	// KP 8
	0x39,	// KP 9
	0x25,	// KP -
	0x48,	// KP 4
	0x49,	// KP 5
	0x4a,	// KP 6
	0x3a,	// KP +
	0x58,	// KP 1
	0x59,	// KP 2
	0x5a,	// KP 3
	0x64,	// KP 0
	0x65,	// KP .
	0x00,	// UNMAPPED
	0x00,	// UNMAPPED
	0x00,	// UNMAPPED
	0x0c,	// F11
	0x0d,	// F12
	0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
	0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
	0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
	0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
	0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
	0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
	0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
	0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
	0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
	0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
	0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x5b,   // KP Enter
        0x60,   // Right Control
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x23,   // KP /
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x5f,   // Right Alt
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x20,   // Home
	0x57,	// Up Arrow
        0x21,   // Page Up
        0x00,   // UNMAPPED
        0x61,   // Left Arrow
        0x00,   // UNMAPPED
        0x63,   // Right Arrow
        0x00,   // UNMAPPED
        0x35,   // End
        0x62,   // Down Arrow
        0x36,   // Page Down
        0x1f,   // Insert
        0x34,   // Delete
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED
        0x00,   // UNMAPPED

};


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
	LOG("%s\n", __PRETTY_FUNCTION__);
}


KeyboardInputDevice::~KeyboardInputDevice()
{
	LOG("%s\n", __PRETTY_FUNCTION__);
	StopMonitoringDevice(kKeyboardDevicesDirectoryUSB);
	
#if DEBUG	
	fclose(sLogFile);
#endif
}


status_t
KeyboardInputDevice::InitFromSettings(void *cookie, uint32 opcode)
{
	LOG("%s\n", __PRETTY_FUNCTION__);
	
	keyboard_device *device = (keyboard_device *)cookie;

	if (get_key_repeat_rate(&device->settings.key_repeat_rate) != B_OK)
		LOG_ERR("error when get_key_repeat_rate\n");
	else
		if (ioctl(device->fd, kSetKeyRepeatRate, &device->settings.key_repeat_rate)!=B_OK)
			LOG_ERR("error when kSetKeyRepeatRate, fd:%li\n", device->fd);
	
	if (get_key_repeat_delay(&device->settings.key_repeat_delay) != B_OK)
                LOG_ERR("error when get_key_repeat_delay\n");
        else
                if (ioctl(device->fd, kSetKeyRepeatDelay, &device->settings.key_repeat_delay)!=B_OK)
			LOG_ERR("error when kSetKeyRepeatDelay, fd:%li\n", device->fd);

	return B_OK;
}


status_t
KeyboardInputDevice::InitCheck()
{
	LOG("%s\n", __PRETTY_FUNCTION__);
	RecursiveScan(kKeyboardDevicesDirectory);
	
	return B_OK;
}


status_t
KeyboardInputDevice::Start(const char *name, void *cookie)
{
	LOG("%s\n", __PRETTY_FUNCTION__);
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
	LOG("%s\n", __PRETTY_FUNCTION__);
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
	LOG("%s\n", __PRETTY_FUNCTION__);
	keyboard_device *device = new keyboard_device();
	if (!device)
		return B_NO_MEMORY;
		
	if ((device->fd = open(path, O_RDWR)) < B_OK) {
		delete device;
		return B_ERROR;
	}
		
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
	LOG("%s\n", __PRETTY_FUNCTION__);
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
		LOG("%s\n", __PRETTY_FUNCTION__);
		
		if (ioctl(dev->fd, kGetNextKey, &buffer) == B_OK) {
			at_kbd_io *at_kbd = (at_kbd_io *)buffer;
			uint32 keycode = 0;
			if (at_kbd->scancode>0)
				keycode = at_keycode_map[at_kbd->scancode-1];	

			LOG("kGetNextKey : %Ld, %02x, %02x, %02lx\n", at_kbd->timestamp, at_kbd->scancode, at_kbd->is_keydown, keycode);

			BMessage *msg = new BMessage;
			msg->what = (at_kbd->is_keydown) ? B_KEY_DOWN : B_KEY_UP;
			msg->AddInt64("when", at_kbd->timestamp);
			
			if (dev->owner->EnqueueMessage(msg)!=B_OK)
				delete msg;
		} else {
			LOG("kGetNextKey error 2\n");
			snooze(100000);
		}
		
		// be:key_repeat
		// be:old_modifiers
		// when
		// raw_char
		// key
		// modifiers
		// states
		// byte
		// bytes
		    
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
