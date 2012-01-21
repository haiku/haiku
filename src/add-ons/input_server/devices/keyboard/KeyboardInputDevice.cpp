/*
 * Copyright 2004-2006, Jérôme Duval. All rights reserved.
 * Copyright 2005-2010, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2008-2009, Stephan Aßmus, superstippi@gmx.de.
 *
 * Distributed under the terms of the MIT License.
 */


#include "KeyboardInputDevice.h"

#include <errno.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Application.h>
#include <Autolock.h>
#include <Directory.h>
#include <Entry.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <String.h>

#include <keyboard_mouse_driver.h>


#undef TRACE

//#define TRACE_KEYBOARD_DEVICE
#ifdef TRACE_KEYBOARD_DEVICE

static	int32		sFunctionDepth = -1;

class FunctionTracer {
public:
	FunctionTracer(const void* pointer, const char* className,
			const char* functionName)
		:
		fFunctionName(),
		fPrepend(),
		fPointer(pointer)
	{
		sFunctionDepth++;
		fPrepend.Append(' ', sFunctionDepth * 2);
		fFunctionName << className << "::" << functionName << "()";

		debug_printf("%p -> %s%s {\n", fPointer, fPrepend.String(),
			fFunctionName.String());
	}

	~FunctionTracer()
	{
		debug_printf("%p -> %s}\n", fPointer, fPrepend.String());
		sFunctionDepth--;
	}

	static int32 Depth() { return sFunctionDepth; }

private:
			BString		fFunctionName;
			BString		fPrepend;
			const void* fPointer;
};

#	define KD_CALLED(x...) \
		FunctionTracer _ft(this, "KeyboardDevice", __FUNCTION__)
#	define KID_CALLED(x...)	\
		FunctionTracer _ft(this, "KeyboardInputDevice", __FUNCTION__)
#	define TRACE(x...) \
		do { BString _to; \
			_to.Append(' ', (FunctionTracer::Depth() + 1) * 2); \
			debug_printf("%p -> %s", this, _to.String()); \
			debug_printf(x); } while (0)
#	define LOG_EVENT(text...) debug_printf(text)
#	define LOG_ERR(text...) TRACE(text)
#else
#	define TRACE(x...) do {} while (0)
#	define KD_CALLED(x...) TRACE(x)
#	define KID_CALLED(x...) TRACE(x)
#	define LOG_ERR(text...) debug_printf(text)
#	define LOG_EVENT(text...) TRACE(x)
#endif


const static uint32 kKeyboardThreadPriority = B_FIRST_REAL_TIME_PRIORITY + 4;
const static char* kKeyboardDevicesDirectory = "/dev/input/keyboard";


extern "C" BInputServerDevice*
instantiate_input_device()
{
	return new(std::nothrow) KeyboardInputDevice();
}


static char*
get_short_name(const char* longName)
{
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


KeyboardDevice::KeyboardDevice(KeyboardInputDevice* owner, const char* path)
	:
	BHandler("keyboard device"),
	fOwner(owner),
	fFD(-1),
	fThread(-1),
	fActive(false),
	fInputMethodStarted(false),
	fKeyboardID(0),
	fUpdateSettings(false),
	fSettingsCommand(0),
	fKeymapLock("keymap lock")
{
	KD_CALLED();

	strlcpy(fPath, path, B_PATH_NAME_LENGTH);
	fDeviceRef.name = get_short_name(path);
	fDeviceRef.type = B_KEYBOARD_DEVICE;
	fDeviceRef.cookie = this;

	if (be_app->Lock()) {
		be_app->AddHandler(this);
		be_app->Unlock();
	}
}


KeyboardDevice::~KeyboardDevice()
{
	KD_CALLED();
	TRACE("delete\n");

	if (fActive)
		Stop();

	free(fDeviceRef.name);

	if (be_app->Lock()) {
		be_app->RemoveHandler(this);
		be_app->Unlock();
	}
}


void
KeyboardDevice::MessageReceived(BMessage* message)
{
	KD_CALLED();

	if (message->what != B_INPUT_METHOD_EVENT) {
		BHandler::MessageReceived(message);
		return;
	}

	int32 opcode;
	if (message->FindInt32("be:opcode", &opcode) != B_OK)
		return;

	if (opcode == B_INPUT_METHOD_STOPPED)
		fInputMethodStarted = false;
}


status_t
KeyboardDevice::Start()
{
	KD_CALLED();
	TRACE("name: %s\n", fDeviceRef.name);

	fFD = open(fPath, O_RDWR);
		// let the control thread handle any error on opening the device

	char threadName[B_OS_NAME_LENGTH];
	snprintf(threadName, B_OS_NAME_LENGTH, "%s watcher", fDeviceRef.name);

	fThread = spawn_thread(_ControlThreadEntry, threadName,
		kKeyboardThreadPriority, this);
	if (fThread < B_OK)
		return fThread;

	fActive = true;
	resume_thread(fThread);

	return fFD >= 0 ? B_OK : B_ERROR;
}


void
KeyboardDevice::Stop()
{
	KD_CALLED();
	TRACE("name: %s\n", fDeviceRef.name);

	fActive = false;

	close(fFD);
	fFD = -1;

	if (fThread >= 0) {
		suspend_thread(fThread);
		resume_thread(fThread);
		status_t dummy;
		wait_for_thread(fThread, &dummy);
	}
}


status_t
KeyboardDevice::UpdateSettings(uint32 opcode)
{
	KD_CALLED();

	if (fThread < 0)
		return B_ERROR;

	// schedule updating the settings from within the control thread
	fSettingsCommand = opcode;
	fUpdateSettings = true;

	return B_OK;
}


// #pragma mark - control thread


/*static*/ int32
KeyboardDevice::_ControlThreadEntry(void* arg)
{
	KeyboardDevice* device = (KeyboardDevice*)arg;
	return device->_ControlThread();
}


int32
KeyboardDevice::_ControlThread()
{
	KD_CALLED();
	TRACE("fPath: %s\n", fPath);

	if (fFD < B_OK) {
		LOG_ERR("KeyboardDevice: error when opening %s: %s\n",
			fPath, strerror(errno));
		_ControlThreadCleanup();
		// TOAST!
		return B_ERROR;
	}

	_UpdateSettings(0);

	raw_key_info keyInfo;
	uint8 activeDeadKey = 0;
	uint32 lastKeyCode = 0;
	uint32 repeatCount = 1;
	uint8 states[16];
	bool ctrlAltDelPressed = false;

	memset(states, 0, sizeof(states));

	if (fKeyboardID == 0) {
		if (ioctl(fFD, KB_GET_KEYBOARD_ID, &fKeyboardID) == 0) {
			BMessage message(IS_SET_KEYBOARD_ID);
			message.AddInt16("id", fKeyboardID);
			be_app->PostMessage(&message);
		}
	}

	while (fActive) {
		status_t status = ioctl(fFD, KB_READ, &keyInfo, sizeof(keyInfo));
		if (status == B_BUSY) {
			// probably the debugger is listening to key events, wait and try
			// again
			snooze(100000);
			continue;
		} else if (status != B_OK) {
			_ControlThreadCleanup();
			// TOAST!
			return 0;
		}

		// Update the settings from this thread if necessary
		if (fUpdateSettings) {
			_UpdateSettings(fSettingsCommand);
			fUpdateSettings = false;
		}

		uint32 keycode = keyInfo.keycode;
		bool isKeyDown = keyInfo.is_keydown;

		LOG_EVENT("KB_READ: %Ld, %02x, %02lx\n", keyInfo.timestamp, isKeyDown,
			keycode);

		if (keycode == 0)
			continue;

		if (isKeyDown && keycode == 0x68) {
			// MENU KEY for Tracker
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

		if (isKeyDown && keycode == 0x34 // DELETE KEY
			&& (states[fCommandKey >> 3] & (1 << (7 - (fCommandKey & 0x7))))
			&& (states[fControlKey >> 3] & (1 << (7 - (fControlKey & 0x7))))) {
			LOG_EVENT("TeamMonitor called\n");

			// show the team monitor
			if (fOwner->fTeamMonitorWindow == NULL)
				fOwner->fTeamMonitorWindow = new(std::nothrow) TeamMonitorWindow();

			if (fOwner->fTeamMonitorWindow != NULL)
				fOwner->fTeamMonitorWindow->Enable();

			ctrlAltDelPressed = true;
		}

		if (ctrlAltDelPressed) {
			if (fOwner->fTeamMonitorWindow != NULL) {
				BMessage message(kMsgCtrlAltDelPressed);
				message.AddBool("key down", isKeyDown);
				fOwner->fTeamMonitorWindow->PostMessage(&message);
			}

			if (!isKeyDown)
				ctrlAltDelPressed = false;
		}

		BAutolock lock(fKeymapLock);

		uint32 modifiers = fKeymap.Modifier(keycode);
		bool isLock
			= (modifiers & (B_CAPS_LOCK | B_NUM_LOCK | B_SCROLL_LOCK)) != 0;
		if (modifiers != 0 && (!isLock || isKeyDown)) {
			uint32 oldModifiers = fModifiers;

			if ((isKeyDown && !isLock)
				|| (isKeyDown && !(fModifiers & modifiers)))
				fModifiers |= modifiers;
			else {
				fModifiers &= ~modifiers;

				// ensure that we don't clear a combined B_*_KEY when still
				// one of the individual B_{LEFT|RIGHT}_*_KEY is pressed
				if (fModifiers & (B_LEFT_SHIFT_KEY | B_RIGHT_SHIFT_KEY))
					fModifiers |= B_SHIFT_KEY;
				if (fModifiers & (B_LEFT_COMMAND_KEY | B_RIGHT_COMMAND_KEY))
					fModifiers |= B_COMMAND_KEY;
				if (fModifiers & (B_LEFT_CONTROL_KEY | B_RIGHT_CONTROL_KEY))
					fModifiers |= B_CONTROL_KEY;
				if (fModifiers & (B_LEFT_OPTION_KEY | B_RIGHT_OPTION_KEY))
					fModifiers |= B_OPTION_KEY;
			}

			if (fModifiers != oldModifiers) {
				BMessage* message = new BMessage(B_MODIFIERS_CHANGED);
				if (message == NULL)
					continue;

				message->AddInt64("when", keyInfo.timestamp);
				message->AddInt32("be:old_modifiers", oldModifiers);
				message->AddInt32("modifiers", fModifiers);
				message->AddData("states", B_UINT8_TYPE, states, 16);

				if (fOwner->EnqueueMessage(message) != B_OK)
					delete message;

				if (isLock)
					_UpdateLEDs();
			}
		}

		uint8 newDeadKey = 0;
		if (activeDeadKey == 0 || !isKeyDown)
			newDeadKey = fKeymap.ActiveDeadKey(keycode, fModifiers);

		char* string = NULL;
		char* rawString = NULL;
		int32 numBytes = 0, rawNumBytes = 0;
		if (newDeadKey == 0) {
			fKeymap.GetChars(keycode, fModifiers, activeDeadKey, &string,
				&numBytes);
		}
		fKeymap.GetChars(keycode, 0, 0, &rawString, &rawNumBytes);

		BMessage* msg = new BMessage;
		if (msg == NULL) {
			delete[] string;
			delete[] rawString;
			continue;
		}

		if (numBytes > 0)
			msg->what = isKeyDown ? B_KEY_DOWN : B_KEY_UP;
		else
			msg->what = isKeyDown ? B_UNMAPPED_KEY_DOWN : B_UNMAPPED_KEY_UP;

		msg->AddInt64("when", keyInfo.timestamp);
		msg->AddInt32("key", keycode);
		msg->AddInt32("modifiers", fModifiers);
		msg->AddData("states", B_UINT8_TYPE, states, 16);
		if (numBytes > 0) {
			for (int i = 0; i < numBytes; i++)
				msg->AddInt8("byte", (int8)string[i]);
			msg->AddData("bytes", B_STRING_TYPE, string, numBytes + 1);

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

		if (newDeadKey == 0) {
			if (isKeyDown && !modifiers && activeDeadKey != 0) {
				// a dead key was completed
				activeDeadKey = 0;
				if (fInputMethodStarted) {
					_EnqueueInlineInputMethod(B_INPUT_METHOD_CHANGED,
						string, true, msg);
					_EnqueueInlineInputMethod(B_INPUT_METHOD_STOPPED);
					fInputMethodStarted = false;
					msg = NULL;
				}
			}
		} else if (isKeyDown
			&& _EnqueueInlineInputMethod(B_INPUT_METHOD_STARTED) == B_OK) {
			// start of a dead key
			char* string = NULL;
			int32 numBytes = 0;
			fKeymap.GetChars(keycode, fModifiers, 0, &string, &numBytes);

			if (_EnqueueInlineInputMethod(B_INPUT_METHOD_CHANGED, string)
					== B_OK)
				fInputMethodStarted = true;

			activeDeadKey = newDeadKey;
			delete[] string;
		}

		if (msg != NULL && fOwner->EnqueueMessage(msg) != B_OK)
			delete msg;

		lastKeyCode = isKeyDown ? keycode : 0;
	}

	return 0;
}


void
KeyboardDevice::_ControlThreadCleanup()
{
	// NOTE: Only executed when the control thread detected an error
	// and from within the control thread!

	if (fActive) {
		fThread = -1;
		fOwner->_RemoveDevice(fPath);
	} else {
		// In case active is already false, another thread
		// waits for this thread to quit, and may already hold
		// locks that _RemoveDevice() wants to acquire. In another
		// words, the device is already being removed, so we simply
		// quit here.
	}
}


void
KeyboardDevice::_UpdateSettings(uint32 opcode)
{
	KD_CALLED();

	if (opcode == 0 || opcode == B_KEY_REPEAT_RATE_CHANGED) {
		if (get_key_repeat_rate(&fSettings.key_repeat_rate) != B_OK) {
			LOG_ERR("error when get_key_repeat_rate\n");
		} else if (ioctl(fFD, KB_SET_KEY_REPEAT_RATE,
			&fSettings.key_repeat_rate) != B_OK) {
			LOG_ERR("error when KB_SET_KEY_REPEAT_RATE, fd:%d\n", fFD);
		}
	}

	if (opcode == 0 || opcode == B_KEY_REPEAT_DELAY_CHANGED) {
		if (get_key_repeat_delay(&fSettings.key_repeat_delay) != B_OK) {
			LOG_ERR("error when get_key_repeat_delay\n");
		} else if (ioctl(fFD, KB_SET_KEY_REPEAT_DELAY,
			&fSettings.key_repeat_delay) != B_OK) {
			LOG_ERR("error when KB_SET_KEY_REPEAT_DELAY, fd:%d\n", fFD);
		}
	}

	if (opcode == 0 || opcode == B_KEY_MAP_CHANGED
		|| opcode == B_KEY_LOCKS_CHANGED) {
		BAutolock lock(fKeymapLock);
		fKeymap.RetrieveCurrent();
		fModifiers = fKeymap.Map().lock_settings;
		_UpdateLEDs();
		fControlKey = fKeymap.KeyForModifier(B_LEFT_CONTROL_KEY);
		fCommandKey = fKeymap.KeyForModifier(B_LEFT_COMMAND_KEY);
	}
}


void
KeyboardDevice::_UpdateLEDs()
{
	if (fFD < 0)
		return;

	char lockIO[3] = {0, 0, 0};

	if ((fModifiers & B_NUM_LOCK) != 0)
		lockIO[0] = 1;
	if ((fModifiers & B_CAPS_LOCK) != 0)
		lockIO[1] = 1;
	if ((fModifiers & B_SCROLL_LOCK) != 0)
		lockIO[2] = 1;

	ioctl(fFD, KB_SET_LEDS, &lockIO);
}


status_t
KeyboardDevice::_EnqueueInlineInputMethod(int32 opcode,
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

	status_t status = fOwner->EnqueueMessage(message);
	if (status != B_OK)
		delete message;

	return status;
}


//	#pragma mark -


KeyboardInputDevice::KeyboardInputDevice()
	:
	fDevices(2, true),
	fDeviceListLock("KeyboardInputDevice list"),
	fTeamMonitorWindow(NULL)
{
	KID_CALLED();

	StartMonitoringDevice(kKeyboardDevicesDirectory);
	_RecursiveScan(kKeyboardDevicesDirectory);
}


KeyboardInputDevice::~KeyboardInputDevice()
{
	KID_CALLED();

	if (fTeamMonitorWindow) {
		fTeamMonitorWindow->PostMessage(B_QUIT_REQUESTED);
		fTeamMonitorWindow = NULL;
	}

	StopMonitoringDevice(kKeyboardDevicesDirectory);
	fDevices.MakeEmpty();
}


status_t
KeyboardInputDevice::SystemShuttingDown()
{
	KID_CALLED();
	if (fTeamMonitorWindow)
		fTeamMonitorWindow->PostMessage(SYSTEM_SHUTTING_DOWN);

	return B_OK;
}


status_t
KeyboardInputDevice::InitCheck()
{
	KID_CALLED();
	return BInputServerDevice::InitCheck();
}


status_t
KeyboardInputDevice::Start(const char* name, void* cookie)
{
	KID_CALLED();
	TRACE("name %s\n", name);

	KeyboardDevice* device = (KeyboardDevice*)cookie;

	return device->Start();
}


status_t
KeyboardInputDevice::Stop(const char* name, void* cookie)
{
	KID_CALLED();
	TRACE("name %s\n", name);

	KeyboardDevice* device = (KeyboardDevice*)cookie;

	device->Stop();
	return B_OK;
}


status_t
KeyboardInputDevice::Control(const char* name, void* cookie,
	uint32 command, BMessage* message)
{
	KID_CALLED();
	TRACE("KeyboardInputDevice::Control(%s, code: %lu)\n", name, command);

	if (command == B_NODE_MONITOR)
		_HandleMonitor(message);
	else if (command >= B_KEY_MAP_CHANGED
		&& command <= B_KEY_REPEAT_RATE_CHANGED) {
		KeyboardDevice* device = (KeyboardDevice*)cookie;
		device->UpdateSettings(command);
	}
	return B_OK;
}


status_t
KeyboardInputDevice::_HandleMonitor(BMessage* message)
{
	KID_CALLED();

	const char* path;
	int32 opcode;
	if (message->FindInt32("opcode", &opcode) != B_OK
		|| (opcode != B_ENTRY_CREATED && opcode != B_ENTRY_REMOVED)
		|| message->FindString("path", &path) != B_OK)
		return B_BAD_VALUE;

	if (opcode == B_ENTRY_CREATED)
		return _AddDevice(path);

#if 0
	return _RemoveDevice(path);
#else
	// Don't handle B_ENTRY_REMOVED, let the control thread take care of it.
	return B_OK;
#endif
}


KeyboardDevice*
KeyboardInputDevice::_FindDevice(const char* path) const
{
	for (int i = fDevices.CountItems() - 1; i >= 0; i--) {
		KeyboardDevice* device = fDevices.ItemAt(i);
		if (strcmp(device->Path(), path) == 0)
			return device;
	}

	return NULL;
}


status_t
KeyboardInputDevice::_AddDevice(const char* path)
{
	KID_CALLED();
	TRACE("path: %s\n", path);

	BAutolock _(fDeviceListLock);

	_RemoveDevice(path);

	KeyboardDevice* device = new(std::nothrow) KeyboardDevice(this, path);
	if (device == NULL)
		return B_NO_MEMORY;

	input_device_ref* devices[2];
	devices[0] = device->DeviceRef();
	devices[1] = NULL;

	fDevices.AddItem(device);

	return RegisterDevices(devices);
}


status_t
KeyboardInputDevice::_RemoveDevice(const char* path)
{
	BAutolock _(fDeviceListLock);

	KeyboardDevice* device = _FindDevice(path);
	if (device == NULL)
		return B_ENTRY_NOT_FOUND;

	KID_CALLED();
	TRACE("path: %s\n", path);

	input_device_ref* devices[2];
	devices[0] = device->DeviceRef();
	devices[1] = NULL;

	UnregisterDevices(devices);

	fDevices.RemoveItem(device);

	return B_OK;
}


void
KeyboardInputDevice::_RecursiveScan(const char* directory)
{
	KID_CALLED();
	TRACE("directory: %s\n", directory);

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
