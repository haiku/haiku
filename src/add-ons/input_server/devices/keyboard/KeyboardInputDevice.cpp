/*****************************************************************************/
// Keyboard input server device addon
// Written by Jérôme Duval
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

#if DEBUG
	inline void LOG(const char *fmt, ...) { char buf[1024]; va_list ap; va_start(ap, fmt); vsprintf(buf, fmt, ap); va_end(ap); \
		fputs(buf, KeyboardInputDevice::sLogFile); fflush(KeyboardInputDevice::sLogFile); }
	#define LOG_ERR(text...) LOG(text)
#else
	#define LOG(text...)
	#define LOG_ERR(text...) fprintf(stderr, text)
#endif

#define CALLED() LOG("%s\n", __PRETTY_FUNCTION__)

const static uint32 kSetLeds = 0x2711;
const static uint32 kSetRepeatingKey = 0x2712;
const static uint32 kSetNonRepeatingKey = 0x2713;
const static uint32 kSetKeyRepeatRate = 0x2714;
const static uint32 kSetKeyRepeatDelay = 0x2716;
const static uint32 kCancelTimer = 0x2719;
const static uint32 kGetNextKey = 0x270f;

const static uint32 kKeyboardThreadPriority = B_FIRST_REAL_TIME_PRIORITY + 4;
const static char *kKeyboardDevicesDirectory = "/dev/input/keyboard";

// "/dev/" is automatically prepended by StartMonitoringDevice()
const static char *kKeyboardDevicesDirectoryUSB = "input/keyboard/usb";

const uint32 at_keycode_map[] = {
	0x1,	// Esc
	0x12, 	// 1
	0x13,	// 2
	0x14,	// 3
	0x15,	// 4
	0x16,	// 5
	0x17,	// 6
	0x18,	// 7
	0x19,	// 8
	0x1a,	// 9
	0x1b,	// 0
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
	0x69,	// <
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
	0x6e,   // Katakana/Hiragana, second key right to spacebar, japanese
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x6b,   // Ro (\\ key, japanese)
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x6d,   // Henkan, first key right to spacebar, japanese
	0x00,   // UNMAPPED
	0x6c,   // Muhenkan, key left to spacebar, japanese
	0x00,   // UNMAPPED
	0x6a,   // Yen (macron key, japanese)
	0x70,   // Keypad . on Brazilian ABNT2
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
	0x0e,   // Print Screen
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
	0x7f,   // Break
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
	0x66,   // Left Gui
	0x67,   // Right Gui
	0x68,   // Menu
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
	CALLED();
	
	StartMonitoringDevice(kKeyboardDevicesDirectoryUSB);
}


KeyboardInputDevice::~KeyboardInputDevice()
{
	CALLED();
	StopMonitoringDevice(kKeyboardDevicesDirectoryUSB);
	
#if DEBUG	
	fclose(sLogFile);
#endif
}


status_t
KeyboardInputDevice::InitFromSettings(void *cookie, uint32 opcode)
{
	CALLED();
	
	keyboard_device *device = (keyboard_device *)cookie;

	if (opcode == 0 
		|| opcode == B_KEY_REPEAT_RATE_CHANGED) { 
		if (get_key_repeat_rate(&device->settings.key_repeat_rate) != B_OK)
			LOG_ERR("error when get_key_repeat_rate\n");
		else
			if (ioctl(device->fd, kSetKeyRepeatRate, &device->settings.key_repeat_rate)!=B_OK)
				LOG_ERR("error when kSetKeyRepeatRate, fd:%d\n", device->fd);
	}
	
	if (opcode == 0 
		|| opcode == B_KEY_REPEAT_DELAY_CHANGED) { 
	
		if (get_key_repeat_delay(&device->settings.key_repeat_delay) != B_OK)
			LOG_ERR("error when get_key_repeat_delay\n");
		else
			if (ioctl(device->fd, kSetKeyRepeatDelay, &device->settings.key_repeat_delay)!=B_OK)
				LOG_ERR("error when kSetKeyRepeatDelay, fd:%d\n", device->fd);
	}
	
	if (opcode == 0 
		|| opcode == B_KEY_MAP_CHANGED 
		|| opcode == B_KEY_LOCKS_CHANGED) {	
		fKeymap.LoadCurrent();
		device->modifiers = fKeymap.Locks();
		SetLeds(device);
	}
	
	return B_OK;
}


status_t
KeyboardInputDevice::InitCheck()
{
	CALLED();
	RecursiveScan(kKeyboardDevicesDirectory);
	
	return B_OK;
}


status_t
KeyboardInputDevice::Start(const char *name, void *cookie)
{
	CALLED();
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
	CALLED();
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
	CALLED();
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
	CALLED();
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
	CALLED();
	keyboard_device *device = new keyboard_device();
	if (!device)
		return B_NO_MEMORY;
		
	if ((device->fd = open(path, O_RDWR)) < B_OK) {
		fprintf(stderr, "error when opening %s\n", path);
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
	device->isAT = false;
	if (strstr(device->path, "keyboard/at") != NULL)
		device->isAT = true;
		
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
	CALLED();
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
	CALLED();
	keyboard_device *dev = (keyboard_device *)arg;
	uint8 buffer[16];
	uint8 activeDeadKey = 0;
	Keymap *keymap = &dev->owner->fKeymap;
	uint32 lastKeyCode = 0;
	uint32 repeatCount = 1;
	uint8 states[16];
	
	memset(states, 0, sizeof(states));
	
	while (dev->active) {
		LOG("%s\n", __PRETTY_FUNCTION__);
		
		if (ioctl(dev->fd, kGetNextKey, &buffer) == B_OK) {
			uint32 keycode = 0;
			bool is_keydown = false;
			bigtime_t timestamp = 0;
			
			LOG("kGetNextKey :");
			if (dev->isAT) {
				at_kbd_io *at_kbd = (at_kbd_io *)buffer;
				if (at_kbd->scancode>0)
					keycode = at_keycode_map[at_kbd->scancode-1];
				is_keydown = at_kbd->is_keydown;
				timestamp = at_kbd->timestamp;
				LOG(" %02x", at_kbd->scancode);
			} else {
				raw_key_info *raw_kbd = (raw_key_info *)buffer;
				is_keydown = raw_kbd->is_keydown;
				timestamp = raw_kbd->timestamp;
				keycode = raw_kbd->be_keycode;
			}
			
			if (keycode == 0)
				continue;

			LOG(" %Ld, %02x, %02lx\n", timestamp, is_keydown, keycode);
			
			if (is_keydown
				&& (keycode == 0x68) ) { // MENU KEY for OpenTracker 5.2.0+
				bool nokey = true;
				for (int32 i=0; i<16; i++)
					if (states[i] != 0) {
						nokey = false;
						break;
					}
				
				if (nokey) {		
					BMessenger msger("application/x-vnd.Be-TSKB");
					if (msger.IsValid())
						msger.SendMessage('BeMn');
				}
			}
			
			if (is_keydown)
				states[(keycode)>>3] |= (1 << (7 - (keycode & 0x7)));
			else
				states[(keycode)>>3] &= (!(1 << (7 - (keycode & 0x7))));
				
			if (is_keydown
				&& (keycode == 0x34) // DELETE KEY
				&& (states[0x5c >> 3] & (1 << (7 - (0x5c & 0x7))))
				&& (states[0x5d >> 3] & (1 << (7 - (0x5d & 0x7))))) {
				
				// show the team monitor
				// argh we don't have one !
				
				// cancel timer only for R5
				if (ioctl(dev->fd, kCancelTimer, NULL) == B_OK)
					LOG("kCancelTimer : OK\n");
			}
			
			uint32 modifiers = keymap->Modifier(keycode);
			if (modifiers 
				&& ( !(modifiers & (B_CAPS_LOCK | B_NUM_LOCK | B_SCROLL_LOCK)) 
					|| is_keydown)) {
				BMessage *msg = new BMessage;
				msg->AddInt64("when", timestamp);
				msg->what = B_MODIFIERS_CHANGED;
				msg->AddInt32("be:old_modifiers", dev->modifiers);
				if ((is_keydown && !(modifiers & (B_CAPS_LOCK | B_NUM_LOCK | B_SCROLL_LOCK)))
					|| (is_keydown && !(dev->modifiers & modifiers)))
					dev->modifiers |= modifiers;
				else
					dev->modifiers &= ~modifiers;
					
				msg->AddInt32("modifiers", dev->modifiers);
				msg->AddData("states", B_UINT8_TYPE, states, 16);
				if (dev->owner->EnqueueMessage(msg)!=B_OK)
					delete msg;
					
				if (modifiers & (B_CAPS_LOCK | B_NUM_LOCK | B_SCROLL_LOCK))
					dev->owner->SetLeds(dev);
			}
			
			char *str = NULL, *str2 = NULL;
			int32 numBytes = 0, numBytes2 = 0;
			uint8 newDeadKey = 0;
				
			if (activeDeadKey == 0) {
				newDeadKey = keymap->IsDeadKey(keycode, dev->modifiers);				
			}
		
			
			// new dead key behaviour
			/*if (newDeadKey == 0) {
				keymap->GetChars(keycode, dev->modifiers, activeDeadKey, &str, &numBytes);
				keymap->GetChars(keycode, 0, 0, &str2, &numBytes2);
			}
			
			if (true) {	
			*/
			
			// R5-like dead key behaviour
			if (newDeadKey == 0) {
				keymap->GetChars(keycode, dev->modifiers, activeDeadKey, &str, &numBytes);
				keymap->GetChars(keycode, 0, 0, &str2, &numBytes2);
			
				BMessage *msg = new BMessage;
				if (numBytes>0)
					msg->what = is_keydown ? B_KEY_DOWN : B_KEY_UP;
				else
					msg->what = is_keydown ? B_UNMAPPED_KEY_DOWN : B_UNMAPPED_KEY_UP;
				
				msg->AddInt64("when", timestamp);
				msg->AddInt32("key", keycode);
				msg->AddInt32("modifiers", dev->modifiers);
				msg->AddData("states", B_UINT8_TYPE, states, 16);
				if (numBytes>0) {
					for (int i=0; i<numBytes; i++)
						msg->AddInt8("byte", (int8)str[i]);
					msg->AddString("bytes", str);
				
					if (numBytes2<=0) {
						numBytes2 = 1;
						str2 = str;
					}
					
					if (is_keydown && (lastKeyCode == keycode)) {
						repeatCount++;
						msg->AddInt32("be:key_repeat", repeatCount);
					} else
						repeatCount = 1;
				}
				
				if (numBytes2>0)
					msg->AddInt32("raw_char", (uint32)((uint8)str2[0] & 0x7f));
				
				if (dev->owner->EnqueueMessage(msg)!=B_OK)
				delete msg;
			}
			
			if (!is_keydown && !modifiers) {
				activeDeadKey = newDeadKey;
			}
			lastKeyCode = keycode;
		} else {
			LOG("kGetNextKey error 2\n");
			snooze(100000);
		}
	}
	
	return 0;
}


char *
KeyboardInputDevice::GetShortName(const char *longName)
{
	CALLED();
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
	CALLED();
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


void
KeyboardInputDevice::SetLeds(keyboard_device *device)
{
	if (device->fd<0)
		return;
	
	uint32 locks = device->modifiers;
	char lock_io[3];
	memset(lock_io, 0, sizeof(lock_io));
	if (locks & B_NUM_LOCK)
		lock_io[0] = 1;
	if (locks & B_CAPS_LOCK)
		lock_io[1] = 1;
	if (locks & B_SCROLL_LOCK)
		lock_io[2] = 1;
	
	ioctl(device->fd, kSetLeds, &lock_io);
}
