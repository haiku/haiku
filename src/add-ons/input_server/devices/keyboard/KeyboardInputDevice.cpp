/*
 * Copyright 2004-2006, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "KeyboardInputDevice.h"
#include "kb_mouse_driver.h"

#include <Application.h>
#include <Directory.h>
#include <Entry.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <String.h>

#include <errno.h>
#include <new>
#include <stdlib.h>
#include <unistd.h>


#if DEBUG
FILE *KeyboardInputDevice::sLogFile = NULL;
#endif

const static uint32 kKeyboardThreadPriority = B_FIRST_REAL_TIME_PRIORITY + 4;
const static char *kKeyboardDevicesDirectory = "/dev/input/keyboard";

// "/dev/" is automatically prepended by StartMonitoringDevice()
const static char *kKeyboardDevicesDirectoryPS2 = "input/keyboard/at";
const static char *kKeyboardDevicesDirectoryUSB = "input/keyboard/usb";

const static uint32 kATKeycodeMap[] = {
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
	0x1e,	// BACKSPACE
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
	0x33, 	// \ (backslash -- note: don't remove non-white-space after BS char)
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
	0x00,   // UNMAPPED		90
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED		100
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED		110
	0x00,   // UNMAPPED
	0x6e,   // Katakana/Hiragana, second key right to spacebar, japanese
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x6b,   // Ro (\\ key, japanese)
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED		120
	0x6d,   // Henkan, first key right to spacebar, japanese
	0x00,   // UNMAPPED
	0x6c,   // Muhenkan, key left to spacebar, japanese
	0x00,   // UNMAPPED
	0x6a,   // Yen (macron key, japanese)
	0x70,   // Keypad . on Brazilian ABNT2
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED		130
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED		140
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED		150
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x5b,   // KP Enter
	0x60,   // Right Control
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED		160
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED		170
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED		180
	0x23,   // KP /
	0x00,   // UNMAPPED
	0x0e,   // Print Screen
	0x5f,   // Right Alt
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED		190
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x7f,   // Break
	0x20,   // Home
	0x57,	// Up Arrow		200
	0x21,   // Page Up
	0x00,   // UNMAPPED
	0x61,   // Left Arrow
	0x00,   // UNMAPPED
	0x63,   // Right Arrow
	0x00,   // UNMAPPED
	0x35,   // End
	0x62,   // Down Arrow
	0x36,   // Page Down
	0x1f,   // Insert		200
	0x34,   // Delete
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x66,   // Left Gui
	0x67,   // Right Gui	210
	0x68,   // Menu
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED		220
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
	0x00,   // UNMAPPED
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


extern "C"
BInputServerDevice *
instantiate_input_device()
{
	return new (std::nothrow) KeyboardInputDevice();
}


static char *
get_short_name(const char *longName)
{
	CALLED();
	BString string(longName);
	BString name;

	int32 slash = string.FindLast("/");
	string.CopyInto(name, slash + 1, string.Length() - slash);
	int32 index = atoi(name.String()) + 1;

	int32 previousSlash = string.FindLast("/", slash);
	string.CopyInto(name, previousSlash + 1, slash - previousSlash - 1);

	// some special handling so that we get "USB" and "AT" instead of "usb"/"at"
	if (name.Length() < 4)
		name.ToUpper();
	else
		name.Capitalize();

	name << " Keyboard " << index;

	return strdup(name.String());
}


//	#pragma mark -


KeyboardInputDevice::KeyboardInputDevice()
	:
	fTMWindow(NULL)
{
#if DEBUG
	if (sLogFile == NULL)
		sLogFile = fopen("/var/log/keyboard_device_log.log", "a");
#endif
	CALLED();

	StartMonitoringDevice(kKeyboardDevicesDirectoryPS2);
	StartMonitoringDevice(kKeyboardDevicesDirectoryUSB);
}


KeyboardInputDevice::~KeyboardInputDevice()
{
	CALLED();
	StopMonitoringDevice(kKeyboardDevicesDirectoryUSB);
	StopMonitoringDevice(kKeyboardDevicesDirectoryPS2);

	int count = fDevices.CountItems();
	while (count-- > 0) {
		delete (keyboard_device *)fDevices.RemoveItem((int32)0);
	}

#if DEBUG	
	fclose(sLogFile);
#endif
}


status_t 
KeyboardInputDevice::SystemShuttingDown()
{
	CALLED();
	if (fTMWindow)
		fTMWindow->PostMessage(SYSTEM_SHUTTING_DOWN);

	return B_OK;
}


status_t
KeyboardInputDevice::_InitFromSettings(void *cookie, uint32 opcode)
{
	CALLED();

	keyboard_device *device = (keyboard_device *)cookie;

	if (opcode == 0 || opcode == B_KEY_REPEAT_RATE_CHANGED) {
		if (get_key_repeat_rate(&device->settings.key_repeat_rate) != B_OK)
			LOG_ERR("error when get_key_repeat_rate\n");
		else if (ioctl(device->fd, KB_SET_KEY_REPEAT_RATE,
				&device->settings.key_repeat_rate) != B_OK)
			LOG_ERR("error when KB_SET_KEY_REPEAT_RATE, fd:%d\n", device->fd);
	}

	if (opcode == 0 || opcode == B_KEY_REPEAT_DELAY_CHANGED) {
		if (get_key_repeat_delay(&device->settings.key_repeat_delay) != B_OK)
			LOG_ERR("error when get_key_repeat_delay\n");
		else if (ioctl(device->fd, KB_SET_KEY_REPEAT_DELAY,
				&device->settings.key_repeat_delay) != B_OK)
			LOG_ERR("error when KB_SET_KEY_REPEAT_DELAY, fd:%d\n", device->fd);
	}

	if (opcode == 0 
		|| opcode == B_KEY_MAP_CHANGED 
		|| opcode == B_KEY_LOCKS_CHANGED) {	
		fKeymap.LoadCurrent();
		device->modifiers = fKeymap.Locks();
		_SetLeds(device);
	}

	return B_OK;
}


status_t
KeyboardInputDevice::InitCheck()
{
	CALLED();
	// TODO: this doesn't belong here!
	_RecursiveScan(kKeyboardDevicesDirectory);

	return B_OK;
}


status_t
KeyboardInputDevice::Start(const char *name, void *cookie)
{
	CALLED();
	keyboard_device *device = (keyboard_device *)cookie;

	if ((device->fd = open(device->path, O_RDWR)) < B_OK) {
		fprintf(stderr, "error when opening %s: %s\n", device->path, strerror(errno));
		return B_ERROR;
	}

	_InitFromSettings(device);

	char threadName[B_OS_NAME_LENGTH];
	snprintf(threadName, B_OS_NAME_LENGTH, "%s watcher", name);

	device->active = true;
	device->device_watcher = spawn_thread(_DeviceWatcher, threadName,
		kKeyboardThreadPriority, device);
	if (device->device_watcher < B_OK)
		return device->device_watcher;

	resume_thread(device->device_watcher);
	return B_OK;
}


status_t
KeyboardInputDevice::Stop(const char *name, void *cookie)
{
	CALLED();
	keyboard_device *device = (keyboard_device *)cookie;

	LOG("Stop(%s)\n", name);

	close(device->fd);
	device->fd = -1;

	device->active = false;
	if (device->device_watcher >= 0) {
		suspend_thread(device->device_watcher);
		resume_thread(device->device_watcher);
		status_t dummy;
		wait_for_thread(device->device_watcher, &dummy);
	}

	if (fTMWindow) {
		fTMWindow->PostMessage(B_QUIT_REQUESTED);
		fTMWindow = NULL;
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
		_HandleMonitor(message);
	else if (command >= B_KEY_MAP_CHANGED 
		&& command <= B_KEY_REPEAT_RATE_CHANGED) {
		_InitFromSettings(cookie, command);
	}
	return B_OK;
}


status_t
KeyboardInputDevice::_HandleMonitor(BMessage *message)
{
	CALLED();
	int32 opcode = 0;
	status_t status;
	if ((status = message->FindInt32("opcode", &opcode)) < B_OK)
		return status;

	if (opcode != B_ENTRY_CREATED
		&& opcode != B_ENTRY_REMOVED)
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
		_AddDevice(path.Path());
	else
		_RemoveDevice(path.Path());

	return status;
}


status_t
KeyboardInputDevice::_AddDevice(const char *path)
{
	CALLED();
	keyboard_device *device = new (std::nothrow) keyboard_device(path);
	if (device == NULL)
		return B_NO_MEMORY;

	device->owner = this;

	input_device_ref *devices[2];
	devices[0] = &device->device_ref;
	devices[1] = NULL;

	fDevices.AddItem(device);

	return RegisterDevices(devices);
}


status_t
KeyboardInputDevice::_RemoveDevice(const char *path)
{
	CALLED();
	keyboard_device *device;
	for (int i = 0; (device = (keyboard_device *)fDevices.ItemAt(i)) != NULL; i++) {
		if (!strcmp(device->path, path)) {
			fDevices.RemoveItem(device);

			input_device_ref *devices[2];
			devices[0] = &device->device_ref;
			devices[1] = NULL;
			UnregisterDevices(devices);

			delete device;
			return B_OK;
		}	
	}

	return B_ENTRY_NOT_FOUND;
}


/*static*/ int32
KeyboardInputDevice::_DeviceWatcher(void *arg)
{
	CALLED();
	keyboard_device* device = (keyboard_device *)arg;
	KeyboardInputDevice* owner = device->owner;
	uint8 buffer[16];
	uint8 activeDeadKey = 0;
	Keymap* keymap = &owner->fKeymap;
	uint32 lastKeyCode = 0;
	uint32 repeatCount = 1;
	uint8 states[16];

	memset(states, 0, sizeof(states));

	LOG("%s\n", __PRETTY_FUNCTION__);

	while (device->active) {
		if (ioctl(device->fd, KB_READ, &buffer) != B_OK)
			return 0;

		uint32 keycode = 0;
		bool isKeyDown = false;
		bigtime_t timestamp = 0;

		LOG("KB_READ :");

		if (device->isAT) {
			at_kbd_io *at_kbd = (at_kbd_io *)buffer;
			if (at_kbd->scancode > 0)
				keycode = kATKeycodeMap[at_kbd->scancode-1];
			isKeyDown = at_kbd->is_keydown;
			timestamp = at_kbd->timestamp;
			LOG(" %02x", at_kbd->scancode);
		} else {
			raw_key_info *raw_kbd = (raw_key_info *)buffer;
			isKeyDown = raw_kbd->is_keydown;
			timestamp = raw_kbd->timestamp;
			keycode = raw_kbd->be_keycode;
		}

		if (keycode == 0)
			continue;

		LOG(" %Ld, %02x, %02lx\n", timestamp, isKeyDown, keycode);

		if (isKeyDown && keycode == 0x68) {
			// MENU KEY for OpenTracker 5.2.0+
			bool noOtherKeyPressed = true;
			for (int32 i = 0; i < 16; i++) {
				if (states[i] != 0) {
					noOtherKeyPressed = false;
					break;
				}
			}

			if (noOtherKeyPressed) {		
				BMessenger deskbar("application/x-vnd.Be-TSKB");
				if (deskbar.IsValid())
					deskbar.SendMessage('BeMn');
			}
		}

		if (keycode < 256) {
			if (isKeyDown)
				states[(keycode) >> 3] |= (1 << (7 - (keycode & 0x7)));
			else
				states[(keycode) >> 3] &= (!(1 << (7 - (keycode & 0x7))));
		}

		if (isKeyDown
			&& keycode == 0x34 // DELETE KEY
			&& (states[0x5c >> 3] & (1 << (7 - (0x5c & 0x7))))
			&& (states[0x5d >> 3] & (1 << (7 - (0x5d & 0x7))))) {
			LOG("TeamMonitor called\n");

			// show the team monitor
			if (owner->fTMWindow == NULL)
				owner->fTMWindow = new (std::nothrow) TMWindow();

			if (owner->fTMWindow != NULL) {
				owner->fTMWindow->Enable();

				// cancel timer only for R5
				if (ioctl(device->fd, KB_CANCEL_CONTROL_ALT_DEL, NULL) == B_OK)
					LOG("KB_CANCEL_CONTROL_ALT_DEL : OK\n");
			}
		}

		uint32 modifiers = keymap->Modifier(keycode);
		if (modifiers 
			&& (!(modifiers & (B_CAPS_LOCK | B_NUM_LOCK | B_SCROLL_LOCK)) 
				|| isKeyDown)) {
			BMessage *msg = new BMessage;
			if (msg == NULL)
				continue;

			msg->AddInt64("when", timestamp);
			msg->what = B_MODIFIERS_CHANGED;
			msg->AddInt32("be:old_modifiers", device->modifiers);

			if ((isKeyDown && !(modifiers & (B_CAPS_LOCK | B_NUM_LOCK | B_SCROLL_LOCK)))
				|| (isKeyDown && !(device->modifiers & modifiers)))
				device->modifiers |= modifiers;
			else
				device->modifiers &= ~modifiers;

			msg->AddInt32("modifiers", device->modifiers);
			msg->AddData("states", B_UINT8_TYPE, states, 16);

			if (owner->EnqueueMessage(msg)!=B_OK)
				delete msg;

			if (modifiers & (B_CAPS_LOCK | B_NUM_LOCK | B_SCROLL_LOCK))
				owner->_SetLeds(device);
		}

		uint8 newDeadKey = 0;
		if (activeDeadKey == 0)
			newDeadKey = keymap->IsDeadKey(keycode, device->modifiers);

		if (newDeadKey == 0) {
			char *string = NULL, *rawString = NULL;
			int32 numBytes = 0, rawNumBytes = 0;
			keymap->GetChars(keycode, device->modifiers, activeDeadKey, &string, &numBytes);
			keymap->GetChars(keycode, 0, 0, &rawString, &rawNumBytes);

			BMessage *msg = new BMessage;
			if (msg == NULL)
				continue;

			if (numBytes > 0)
				msg->what = isKeyDown ? B_KEY_DOWN : B_KEY_UP;
			else
				msg->what = isKeyDown ? B_UNMAPPED_KEY_DOWN : B_UNMAPPED_KEY_UP;

			msg->AddInt64("when", timestamp);
			msg->AddInt32("key", keycode);
			msg->AddInt32("modifiers", device->modifiers);
			msg->AddData("states", B_UINT8_TYPE, states, 16);
			if (numBytes > 0) {
				for (int i = 0; i < numBytes; i++) {
					msg->AddInt8("byte", (int8)string[i]);
				}
				msg->AddString("bytes", string);

				if (rawNumBytes <= 0) {
					rawNumBytes = 1;
					delete[] rawString;
					rawString = string;
				} else
					delete[] string;

				if (isKeyDown && lastKeyCode == keycode) {
					repeatCount++;
					msg->AddInt32("be:key_repeat", repeatCount);
				} else
					repeatCount = 1;
			} else
				delete[] string;

			if (rawNumBytes > 0)
				msg->AddInt32("raw_char", (uint32)((uint8)rawString[0] & 0x7f));

			delete[] rawString;

			if (isKeyDown && !modifiers && activeDeadKey != 0
				&& device->input_method_started) {
				// a dead key was completed
				device->EnqueueInlineInputMethod(B_INPUT_METHOD_CHANGED,
					string, true, msg);
			} else if (owner->EnqueueMessage(msg) != B_OK)
				delete msg;
		} else if (isKeyDown) {
			// start of a dead key
			if (device->EnqueueInlineInputMethod(B_INPUT_METHOD_STARTED) == B_OK) {
				char *string = NULL;
				int32 numBytes = 0;
				keymap->GetChars(keycode, device->modifiers, 0, &string, &numBytes);

				if (device->EnqueueInlineInputMethod(B_INPUT_METHOD_CHANGED, string) == B_OK)
					device->input_method_started = true;
			}
		}

		if (!isKeyDown && !modifiers) {
			if (activeDeadKey != 0) {
				device->EnqueueInlineInputMethod(B_INPUT_METHOD_STOPPED);
				device->input_method_started = false;
			}

			activeDeadKey = newDeadKey;
		}

		lastKeyCode = keycode;
	}

	return 0;
}


void
KeyboardInputDevice::_RecursiveScan(const char *directory)
{
	CALLED();
	BEntry entry;
	BDirectory dir(directory);
	while (dir.GetNextEntry(&entry) == B_OK) {
		BPath path;
		entry.GetPath(&path);
		if (entry.IsDirectory())
			_RecursiveScan(path.Path());
		else
			_AddDevice(path.Path());
	}
}


void
KeyboardInputDevice::_SetLeds(keyboard_device *device)
{
	if (device->fd < 0)
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

	ioctl(device->fd, KB_SET_LEDS, &lock_io);
}


//	#pragma mark -


keyboard_device::keyboard_device(const char *path)
	: BHandler("keyboard device"),
	owner(NULL),
	fd(-1),
	device_watcher(-1),
	active(false),
	input_method_started(false)
{
	strcpy(this->path, path);
	device_ref.name = get_short_name(path);
	device_ref.type = B_KEYBOARD_DEVICE;
	device_ref.cookie = this;

	isAT = strstr(path, "keyboard/at") != NULL;

	if (be_app->Lock()) {
		be_app->AddHandler(this);
		be_app->Unlock();
	}
}


keyboard_device::~keyboard_device()
{
	free(device_ref.name);

	if (be_app->Lock()) {
		be_app->RemoveHandler(this);
		be_app->Unlock();
	}
}


status_t
keyboard_device::EnqueueInlineInputMethod(int32 opcode,
	const char* string, bool confirmed, BMessage* keyDown)
{
	BMessage* message = new BMessage(B_INPUT_METHOD_EVENT);
	if (message == NULL)
		return B_NO_MEMORY;

	message->AddInt32("be:opcode", opcode);
	message->AddBool("be:inline_only", true);

	if (string != NULL)
		message->AddString("be:string", string);
	if (confirmed)
		message->AddBool("be:confirmed", true);
	if (keyDown)
		message->AddMessage("be:translated", keyDown);
	if (opcode == B_INPUT_METHOD_STARTED)
		message->AddMessenger("be:reply_to", this);

	status_t status = owner->EnqueueMessage(message);
	if (status != B_OK)
		delete message;

	return status;
}


void
keyboard_device::MessageReceived(BMessage *message)
{
	if (message->what != B_INPUT_METHOD_EVENT) {
		BHandler::MessageReceived(message);
		return;
	}

	int32 opcode;
	if (message->FindInt32("be:opcode", &opcode) != B_OK)
		return;

	if (opcode == B_INPUT_METHOD_STOPPED)
		input_method_started = false;
}

