/*
 * Copyright 2002-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include "InputServer.h"
#include "InputServerTypes.h"
#include "BottomlineWindow.h"
#include "MethodReplicant.h"

#include <safemode_defs.h>
#include <syscalls.h>

#include <AppServerLink.h>
#include <MessagePrivate.h>
#include <ObjectListPrivate.h>

#include <Autolock.h>
#include <Deskbar.h>
#include <Directory.h>
#include <driver_settings.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Locker.h>
#include <Message.h>
#include <OS.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>

#include <stdio.h>

#include "SystemKeymap.h"
	// this is an automatically generated file

#include <ServerProtocol.h>

using std::nothrow;


// Global InputServer member variables.

InputServer* gInputServer;

BList InputServer::gInputFilterList;
BLocker InputServer::gInputFilterListLocker("is_filter_queue_sem");

BList InputServer::gInputMethodList;
BLocker InputServer::gInputMethodListLocker("is_method_queue_sem");

KeymapMethod InputServer::gKeymapMethod;


extern "C" _EXPORT BView* instantiate_deskbar_item();


// #pragma mark - InputDeviceListItem


InputDeviceListItem::InputDeviceListItem(BInputServerDevice& serverDevice,
		const input_device_ref& device)
	:
	fServerDevice(&serverDevice),
	fDevice(),
	fRunning(false)
{
	fDevice.name = strdup(device.name);
	fDevice.type = device.type;
	fDevice.cookie = device.cookie;
}


InputDeviceListItem::~InputDeviceListItem()
{
	free(fDevice.name);
}


void
InputDeviceListItem::Start()
{
	PRINT(("  Starting: %s\n", fDevice.name));
	status_t err = fServerDevice->Start(fDevice.name, fDevice.cookie);
	if (err != B_OK) {
		PRINTERR(("      error: %s (%lx)\n", strerror(err), err));
	}
	fRunning = err == B_OK;
}


void
InputDeviceListItem::Stop()
{
	PRINT(("  Stopping: %s\n", fDevice.name));
	fServerDevice->Stop(fDevice.name, fDevice.cookie);
	fRunning = false;
}


void
InputDeviceListItem::Control(uint32 code, BMessage* message)
{
	fServerDevice->Control(fDevice.name, fDevice.cookie, code, message);
}


bool
InputDeviceListItem::HasName(const char* name) const
{
	if (name == NULL)
		return false;

	return !strcmp(name, fDevice.name);
}


bool
InputDeviceListItem::HasType(input_device_type type) const
{
	return type == fDevice.type;
}


bool
InputDeviceListItem::Matches(const char* name, input_device_type type) const
{
	if (name != NULL)
		return HasName(name);

	return HasType(type);
}


//	#pragma mark -


InputServer::InputServer()
	: BApplication(INPUTSERVER_SIGNATURE),
	fSafeMode(false),
	fKeyboardID(0),
	fInputDeviceListLocker("input server device list"),
	fKeyboardSettings(),
	fMouseSettings(),
	fChars(NULL),
	fScreen(B_MAIN_SCREEN_ID),
	fEventQueueLock("input server event queue"),
	fReplicantMessenger(NULL),
	fInputMethodWindow(NULL),
	fInputMethodAware(false),
	fCursorSem(-1),
	fAppServerPort(-1),
	fCursorArea(-1)
{
	CALLED();
	gInputServer = this;

	set_thread_priority(find_thread(NULL), B_URGENT_DISPLAY_PRIORITY);
		// elevate priority for client interaction

	_StartEventLoop();

	char parameter[32];
	size_t parameterLength = sizeof(parameter);

	if (_kern_get_safemode_option(B_SAFEMODE_SAFE_MODE, parameter,
			&parameterLength) == B_OK) {
		if (!strcasecmp(parameter, "enabled") || !strcasecmp(parameter, "on")
			|| !strcasecmp(parameter, "true") || !strcasecmp(parameter, "yes")
			|| !strcasecmp(parameter, "enable") || !strcmp(parameter, "1"))
			fSafeMode = true;
	}

	if (_kern_get_safemode_option(B_SAFEMODE_DISABLE_USER_ADD_ONS, parameter,
			&parameterLength) == B_OK) {
		if (!strcasecmp(parameter, "enabled") || !strcasecmp(parameter, "on")
			|| !strcasecmp(parameter, "true") || !strcasecmp(parameter, "yes")
			|| !strcasecmp(parameter, "enable") || !strcmp(parameter, "1"))
			fSafeMode = true;
	}

	_InitKeyboardMouseStates();

	fAddOnManager = new(std::nothrow) ::AddOnManager(SafeMode());
	if (fAddOnManager != NULL) {
		// We need to Run() the AddOnManager looper after having loaded
		// the initial add-ons, otherwise we may deadlock when the looper
		// thread for some reason already receives node monitor notifications
		// while we are still locked ourselves and are executing LoadState()
		// at the same time (which may lock the add-on looper thread).
		// NOTE: At first sight this may look like we may loose node monitor
		// notifications while the thread is not yet running, but in fact those
		// message should just pile up and be processed later.
		fAddOnManager->LoadState();
		fAddOnManager->Run();
	}

	BMessenger messenger(this);
	BRoster().StartWatching(messenger, B_REQUEST_LAUNCHED);
}


InputServer::~InputServer()
{
	CALLED();
	if (fAddOnManager->Lock())
		fAddOnManager->Quit();

	_ReleaseInput(NULL);
}


void
InputServer::ArgvReceived(int32 argc, char** argv)
{
	CALLED();
	if (2 == argc && (0 == strcmp("-q", argv[1]))) {
		PRINT(("InputServer::ArgvReceived - Restarting ...\n"));
		status_t quit_status = B_OK;
		BMessenger msgr = BMessenger(INPUTSERVER_SIGNATURE, -1, &quit_status);
		if (B_OK == quit_status) {
			msgr.SendMessage(B_QUIT_REQUESTED);
		} else {
			PRINTERR(("Unable to send Quit message to running InputServer.\n"));
		}
	}
}


void
InputServer::_InitKeyboardMouseStates()
{
	CALLED();
	// This is where we determine the screen resolution from the app_server and find the center of the screen
	// fMousePos is then set to the center of the screen.

	fFrame = fScreen.Frame();
	if (fFrame == BRect(0, 0, 0, 0))
		fFrame = BRect(0, 0, 799, 599);
	fMousePos = BPoint((int32)((fFrame.right + 1) / 2),
		(int32)((fFrame.bottom + 1) / 2));

	memset(&fKeyInfo, 0, sizeof(fKeyInfo));

	if (_LoadKeymap() != B_OK)
		_LoadSystemKeymap();

	BMessage msg(B_MOUSE_MOVED);
	HandleSetMousePosition(&msg, &msg);

	fActiveMethod = &gKeymapMethod;
}


status_t
InputServer::_LoadKeymap()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return B_BAD_VALUE;

	path.Append("Key_map");

	status_t err;

	BFile file(path.Path(), B_READ_ONLY);
	if ((err = file.InitCheck()) != B_OK)
		return err;

	if (file.Read(&fKeys, sizeof(fKeys)) < (ssize_t)sizeof(fKeys))
		return B_BAD_VALUE;

	for (uint32 i = 0; i < sizeof(fKeys)/4; i++)
		((uint32*)&fKeys)[i] = B_BENDIAN_TO_HOST_INT32(((uint32*)&fKeys)[i]);

	if (file.Read(&fCharsSize, sizeof(uint32)) < (ssize_t)sizeof(uint32))
		return B_BAD_VALUE;

	fCharsSize = B_BENDIAN_TO_HOST_INT32(fCharsSize);
	if (fCharsSize <= 0)
		return B_BAD_VALUE;

	delete[] fChars;
	fChars = new (nothrow) char[fCharsSize];
	if (fChars == NULL)
		return B_NO_MEMORY;

	if (file.Read(fChars, fCharsSize) != (signed)fCharsSize)
		return B_BAD_VALUE;

	return B_OK;
}


status_t
InputServer::_LoadSystemKeymap()
{
	delete[] fChars;
	fKeys = kSystemKeymap;
	fCharsSize = kSystemKeyCharsSize;
	fChars = new (nothrow) char[fCharsSize];
	if (fChars == NULL)
		return B_NO_MEMORY;

	memcpy(fChars, kSystemKeyChars, fCharsSize);

	// TODO: why are we doing this?
	return _SaveKeymap(true);
}


status_t
InputServer::_SaveKeymap(bool isDefault)
{
	// we save this keymap to file
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return B_BAD_VALUE;

	path.Append("Key_map");

	BFile file;
	status_t err = file.SetTo(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (err != B_OK) {
		PRINTERR(("error %s\n", strerror(err)));
		return err;
	}

	for (uint32 i = 0; i < sizeof(fKeys) / sizeof(uint32); i++) {
		((uint32*)&fKeys)[i] = B_HOST_TO_BENDIAN_INT32(((uint32*)&fKeys)[i]);
	}

	if ((err = file.Write(&fKeys, sizeof(fKeys))) < (ssize_t)sizeof(fKeys))
		return err;

	for (uint32 i = 0; i < sizeof(fKeys) / sizeof(uint32); i++) {
		((uint32*)&fKeys)[i] = B_BENDIAN_TO_HOST_INT32(((uint32*)&fKeys)[i]);
	}

	uint32 size = B_HOST_TO_BENDIAN_INT32(fCharsSize);

	if ((err = file.Write(&size, sizeof(uint32))) < (ssize_t)sizeof(uint32))
		return B_BAD_VALUE;

	if ((err = file.Write(fChars, fCharsSize)) < (ssize_t)fCharsSize)
		return err;

	// don't bother reporting an error if this fails, since this isn't fatal
	// the keymap will still be functional, and will just be identified as (Current) in prefs instead of its
	// actual name
	if (isDefault)
		file.WriteAttr("keymap:name", B_STRING_TYPE, 0, kSystemKeymapName, strlen(kSystemKeymapName));

	return B_OK;
}


bool
InputServer::QuitRequested()
{
	CALLED();
	if (!BApplication::QuitRequested())
		return false;

	PostMessage(SYSTEM_SHUTTING_DOWN);

	bool shutdown = false;
	CurrentMessage()->FindBool("_shutdown_", &shutdown);

	// Don't actually quit when the system is being shutdown
	if (shutdown) {
		return false;
	} else {
		fAddOnManager->SaveState();

		delete_port(fEventLooperPort);
			// the event looper thread will exit after this
		fEventLooperPort = -1;
		return true;
	}
}


void
InputServer::ReadyToRun()
{
	CALLED();

	// say hello to the app_server

	BPrivate::AppServerLink link;
	link.StartMessage(AS_REGISTER_INPUT_SERVER);
	link.Flush();
}


status_t
InputServer::_AcquireInput(BMessage& message, BMessage& reply)
{
	// TODO: it currently just gets everything we have
	area_id area;
	if (message.FindInt32("cursor area", &area) == B_OK) {
		// try to clone the area
		fCursorBuffer = NULL;

		fCursorSem = create_sem(0, "cursor semaphore");
		if (fCursorSem >= B_OK) {
			fCursorArea = clone_area("input server cursor", (void**)&fCursorBuffer,
				B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, area);
		}
	}

	fAppServerPort = create_port(200, "input server target");
	if (fAppServerPort < B_OK) {
		_ReleaseInput(&message);
		return fAppServerPort;
	}

	reply.AddBool("has keyboard", true);
	reply.AddBool("has mouse", true);
	reply.AddInt32("event port", fAppServerPort);

	if (fCursorBuffer != NULL) {
		// cursor shared buffer is supported
		reply.AddInt32("cursor semaphore", fCursorSem);
	}

	return B_OK;
}


void
InputServer::_ReleaseInput(BMessage* /*message*/)
{
	if (fCursorBuffer != NULL) {
		fCursorBuffer = NULL;
		delete_sem(fCursorSem);
		delete_area(fCursorArea);

		fCursorSem = -1;
		fCursorArea = -1;
	}

	delete_port(fAppServerPort);
}


void
InputServer::MessageReceived(BMessage* message)
{
	CALLED();

	BMessage reply;
	status_t status = B_OK;

	PRINT(("%s what:%c%c%c%c\n", __PRETTY_FUNCTION__, (char)(message->what >> 24),
		(char)(message->what >> 16), (char)(message->what >> 8), (char)message->what));

	switch (message->what) {
		case IS_SET_METHOD:
			HandleSetMethod(message);
			break;
		case IS_GET_MOUSE_TYPE:
			status = HandleGetSetMouseType(message, &reply);
			break;
		case IS_SET_MOUSE_TYPE:
			status = HandleGetSetMouseType(message, &reply);
			break;
		case IS_GET_MOUSE_ACCELERATION:
			status = HandleGetSetMouseAcceleration(message, &reply);
			break;
		case IS_SET_MOUSE_ACCELERATION:
			status = HandleGetSetMouseAcceleration(message, &reply);
			break;
		case IS_GET_KEY_REPEAT_DELAY:
			status = HandleGetSetKeyRepeatDelay(message, &reply);
			break;
		case IS_SET_KEY_REPEAT_DELAY:
			status = HandleGetSetKeyRepeatDelay(message, &reply);
			break;
		case IS_GET_KEY_INFO:
			status = HandleGetKeyInfo(message, &reply);
			break;
		case IS_GET_MODIFIERS:
			status = HandleGetModifiers(message, &reply);
			break;
		case IS_GET_MODIFIER_KEY:
			status = HandleGetModifierKey(message, &reply);
			break;
		case IS_SET_MODIFIER_KEY:
			status = HandleSetModifierKey(message, &reply);
			break;
		case IS_SET_KEYBOARD_LOCKS:
			status = HandleSetKeyboardLocks(message, &reply);
			break;
		case IS_GET_MOUSE_SPEED:
			status = HandleGetSetMouseSpeed(message, &reply);
			break;
		case IS_SET_MOUSE_SPEED:
			status = HandleGetSetMouseSpeed(message, &reply);
			break;
		case IS_SET_MOUSE_POSITION:
			status = HandleSetMousePosition(message, &reply);
			break;
		case IS_GET_MOUSE_MAP:
			status = HandleGetSetMouseMap(message, &reply);
			break;
		case IS_SET_MOUSE_MAP:
			status = HandleGetSetMouseMap(message, &reply);
			break;
		case IS_GET_KEYBOARD_ID:
			status = HandleGetKeyboardID(message, &reply);
			break;
		case IS_GET_CLICK_SPEED:
			status = HandleGetSetClickSpeed(message, &reply);
			break;
		case IS_SET_CLICK_SPEED:
			status = HandleGetSetClickSpeed(message, &reply);
			break;
		case IS_GET_KEY_REPEAT_RATE:
			status = HandleGetSetKeyRepeatRate(message, &reply);
			break;
		case IS_SET_KEY_REPEAT_RATE:
			status = HandleGetSetKeyRepeatRate(message, &reply);
			break;
		case IS_GET_KEY_MAP:
			status = HandleGetSetKeyMap(message, &reply);
			break;
		case IS_RESTORE_KEY_MAP:
			status = HandleGetSetKeyMap(message, &reply);
			break;
		case IS_FOCUS_IM_AWARE_VIEW:
			status = HandleFocusUnfocusIMAwareView(message, &reply);
			break;
		case IS_UNFOCUS_IM_AWARE_VIEW:
			status = HandleFocusUnfocusIMAwareView(message, &reply);
			break;

		// app_server communication
		case IS_ACQUIRE_INPUT:
			status = _AcquireInput(*message, reply);
			break;
		case IS_RELEASE_INPUT:
			_ReleaseInput(message);
			return;
   		case IS_SCREEN_BOUNDS_UPDATED:
   		{
			// This is what the R5 app_server sends us when the screen
			// configuration changes
			BRect frame;
			if (message->FindRect("screen_bounds", &frame) != B_OK)
				frame = fScreen.Frame();

			if (frame == fFrame)
				break;

			BPoint pos(fMousePos.x * frame.Width() / fFrame.Width(),
				fMousePos.y * frame.Height() / fFrame.Height());
			fFrame = frame;

			BMessage set;
			set.AddPoint("where", pos);
			HandleSetMousePosition(&set, NULL);
			break;
		}

		// device looper related
		case IS_FIND_DEVICES:
		case IS_WATCH_DEVICES:
		case IS_IS_DEVICE_RUNNING:
		case IS_START_DEVICE:
		case IS_STOP_DEVICE:
		case IS_CONTROL_DEVICES:
		case SYSTEM_SHUTTING_DOWN:
		case IS_METHOD_REGISTER:
			fAddOnManager->PostMessage(message);
			return;

		case IS_SAVE_SETTINGS:
			fKeyboardSettings.Save();
			fMouseSettings.SaveSettings();
			return;

		case IS_SAVE_KEYMAP:
			_SaveKeymap();
			return;

		case B_SOME_APP_LAUNCHED:
		{
			const char *signature;
			// TODO: what's this for?
			if (message->FindString("be:signature", &signature)==B_OK) {
				PRINT(("input_server : %s\n", signature));
				if (strcmp(signature, "application/x-vnd.Be-TSKB")==0) {

				}
			}
			return;
		}

		default:
			return;
	}

	reply.AddInt32("status", status);
	message->SendReply(&reply);
}


void
InputServer::HandleSetMethod(BMessage* message)
{
	CALLED();
	uint32 cookie;
	if (message->FindInt32("cookie", (int32*)&cookie) == B_OK) {
		BInputServerMethod *method = (BInputServerMethod*)cookie;
		PRINT(("%s cookie %p\n", __PRETTY_FUNCTION__, method));
		SetActiveMethod(method);
	}
}


status_t
InputServer::HandleGetSetMouseType(BMessage* message, BMessage* reply)
{
	int32 type;
	if (message->FindInt32("mouse_type", &type) == B_OK) {
		fMouseSettings.SetMouseType(type);
		be_app_messenger.SendMessage(IS_SAVE_SETTINGS);

		BMessage msg(IS_CONTROL_DEVICES);
		msg.AddInt32("type", B_POINTING_DEVICE);
		msg.AddInt32("code", B_MOUSE_TYPE_CHANGED);
		return fAddOnManager->PostMessage(&msg);
	}

	return reply->AddInt32("mouse_type", fMouseSettings.MouseType());
}


status_t
InputServer::HandleGetSetMouseAcceleration(BMessage* message,
	BMessage* reply)
{
	int32 factor;
	if (message->FindInt32("speed", &factor) == B_OK) {
		fMouseSettings.SetAccelerationFactor(factor);
		be_app_messenger.SendMessage(IS_SAVE_SETTINGS);

		BMessage msg(IS_CONTROL_DEVICES);
		msg.AddInt32("type", B_POINTING_DEVICE);
		msg.AddInt32("code", B_MOUSE_ACCELERATION_CHANGED);
		return fAddOnManager->PostMessage(&msg);
	}

	return reply->AddInt32("speed", fMouseSettings.AccelerationFactor());
}


status_t
InputServer::HandleGetSetKeyRepeatDelay(BMessage* message, BMessage* reply)
{
	bigtime_t delay;
	if (message->FindInt64("delay", &delay) == B_OK) {
		fKeyboardSettings.SetKeyboardRepeatDelay(delay);
		be_app_messenger.SendMessage(IS_SAVE_SETTINGS);

		BMessage msg(IS_CONTROL_DEVICES);
		msg.AddInt32("type", B_KEYBOARD_DEVICE);
		msg.AddInt32("code", B_KEY_REPEAT_DELAY_CHANGED);
		return fAddOnManager->PostMessage(&msg);
	}

	return reply->AddInt64("delay", fKeyboardSettings.KeyboardRepeatDelay());
}


status_t
InputServer::HandleGetKeyInfo(BMessage* message, BMessage* reply)
{
	return reply->AddData("key_info", B_ANY_TYPE, &fKeyInfo, sizeof(fKeyInfo));
}


status_t
InputServer::HandleGetModifiers(BMessage* message, BMessage* reply)
{
	return reply->AddInt32("modifiers", fKeyInfo.modifiers);
}


status_t
InputServer::HandleGetModifierKey(BMessage* message, BMessage* reply)
{
	int32 modifier;

	if (message->FindInt32("modifier", &modifier) == B_OK) {
		switch (modifier) {
			case B_CAPS_LOCK:
				return reply->AddInt32("key", fKeys.caps_key);
			case B_NUM_LOCK:
				return reply->AddInt32("key", fKeys.num_key);
			case B_SCROLL_LOCK:
				return reply->AddInt32("key", fKeys.scroll_key);
			case B_LEFT_SHIFT_KEY:
				return reply->AddInt32("key", fKeys.left_shift_key);
			case B_RIGHT_SHIFT_KEY:
				return reply->AddInt32("key", fKeys.right_shift_key);
			case B_LEFT_COMMAND_KEY:
				return reply->AddInt32("key", fKeys.left_command_key);
			case B_RIGHT_COMMAND_KEY:
				return reply->AddInt32("key", fKeys.right_command_key);
			case B_LEFT_CONTROL_KEY:
				return reply->AddInt32("key", fKeys.left_control_key);
			case B_RIGHT_CONTROL_KEY:
				return reply->AddInt32("key", fKeys.right_control_key);
			case B_LEFT_OPTION_KEY:
				return reply->AddInt32("key", fKeys.left_option_key);
			case B_RIGHT_OPTION_KEY:
				return reply->AddInt32("key", fKeys.right_option_key);
			case B_MENU_KEY:
				return reply->AddInt32("key", fKeys.menu_key);
		}
	}
	return B_ERROR;
}


status_t
InputServer::HandleSetModifierKey(BMessage* message, BMessage* reply)
{
	int32 modifier, key;
	if (message->FindInt32("modifier", &modifier) == B_OK
		&& message->FindInt32("key", &key) == B_OK) {
		switch (modifier) {
			case B_CAPS_LOCK:
				fKeys.caps_key = key;
				break;
			case B_NUM_LOCK:
				fKeys.num_key = key;
				break;
			case B_SCROLL_LOCK:
				fKeys.scroll_key = key;
				break;
			case B_LEFT_SHIFT_KEY:
				fKeys.left_shift_key = key;
				break;
			case B_RIGHT_SHIFT_KEY:
				fKeys.right_shift_key = key;
				break;
			case B_LEFT_COMMAND_KEY:
				fKeys.left_command_key = key;
				break;
			case B_RIGHT_COMMAND_KEY:
				fKeys.right_command_key = key;
				break;
			case B_LEFT_CONTROL_KEY:
				fKeys.left_control_key = key;
				break;
			case B_RIGHT_CONTROL_KEY:
				fKeys.right_control_key = key;
				break;
			case B_LEFT_OPTION_KEY:
				fKeys.left_option_key = key;
				break;
			case B_RIGHT_OPTION_KEY:
				fKeys.right_option_key = key;
				break;
			case B_MENU_KEY:
				fKeys.menu_key = key;
				break;
			default:
				return B_ERROR;
		}

		// TODO: unmap the key ?

		be_app_messenger.SendMessage(IS_SAVE_KEYMAP);

		BMessage msg(IS_CONTROL_DEVICES);
		msg.AddInt32("type", B_KEYBOARD_DEVICE);
		msg.AddInt32("code", B_KEY_MAP_CHANGED);
		return fAddOnManager->PostMessage(&msg);
	}

	return B_ERROR;
}


status_t
InputServer::HandleSetKeyboardLocks(BMessage* message, BMessage* reply)
{
	if (message->FindInt32("locks", (int32*)&fKeys.lock_settings) == B_OK) {
		be_app_messenger.SendMessage(IS_SAVE_KEYMAP);

		BMessage msg(IS_CONTROL_DEVICES);
		msg.AddInt32("type", B_KEYBOARD_DEVICE);
		msg.AddInt32("code", B_KEY_LOCKS_CHANGED);
		return fAddOnManager->PostMessage(&msg);
	}

	return B_ERROR;
}


status_t
InputServer::HandleGetSetMouseSpeed(BMessage* message, BMessage* reply)
{
	int32 speed;
	if (message->FindInt32("speed", &speed) == B_OK) {
		fMouseSettings.SetMouseSpeed(speed);
		be_app_messenger.SendMessage(IS_SAVE_SETTINGS);

		BMessage msg(IS_CONTROL_DEVICES);
		msg.AddInt32("type", B_POINTING_DEVICE);
		msg.AddInt32("code", B_MOUSE_SPEED_CHANGED);
		return fAddOnManager->PostMessage(&msg);
	}

	return reply->AddInt32("speed", fMouseSettings.MouseSpeed());
}


status_t
InputServer::HandleSetMousePosition(BMessage* message, BMessage* reply)
{
	CALLED();

	BPoint where;
	if (message->FindPoint("where", &where) != B_OK)
		return B_BAD_VALUE;

	// create a new event for this and enqueue it to the event list just like any other

	BMessage* event = new BMessage(B_MOUSE_MOVED);
	if (event == NULL)
		return B_NO_MEMORY;

	event->AddPoint("where", where);
	event->AddBool("be:set_mouse", true);
	if (EnqueueDeviceMessage(event) != B_OK) {
		delete event;
		return B_NO_MEMORY;
	}

	return B_OK;
}


status_t
InputServer::HandleGetSetMouseMap(BMessage* message, BMessage* reply)
{
	mouse_map *map;
	ssize_t size;
	if (message->FindData("mousemap", B_RAW_TYPE, (const void**)&map, &size) == B_OK) {
		fMouseSettings.SetMapping(*map);
		be_app_messenger.SendMessage(IS_SAVE_SETTINGS);

		BMessage msg(IS_CONTROL_DEVICES);
		msg.AddInt32("type", B_POINTING_DEVICE);
		msg.AddInt32("code", B_MOUSE_MAP_CHANGED);
		return fAddOnManager->PostMessage(&msg);
	} else {
		mouse_map map;
		fMouseSettings.Mapping(map);
		return reply->AddData("mousemap", B_RAW_TYPE, &map, sizeof(mouse_map));
	}
}


status_t
InputServer::HandleGetKeyboardID(BMessage* message, BMessage* reply)
{
	return reply->AddInt16("id", fKeyboardID);
}


status_t
InputServer::HandleGetSetClickSpeed(BMessage* message, BMessage* reply)
{
	bigtime_t clickSpeed;
	if (message->FindInt64("speed", &clickSpeed) == B_OK) {
		fMouseSettings.SetClickSpeed(clickSpeed);
		be_app_messenger.SendMessage(IS_SAVE_SETTINGS);

		BMessage msg(IS_CONTROL_DEVICES);
		msg.AddInt32("type", B_POINTING_DEVICE);
		msg.AddInt32("code", B_CLICK_SPEED_CHANGED);
		return fAddOnManager->PostMessage(&msg);
	}

	return reply->AddInt64("speed", fMouseSettings.ClickSpeed());
}


status_t
InputServer::HandleGetSetKeyRepeatRate(BMessage* message, BMessage* reply)
{
	int32 keyRepeatRate;
	if (message->FindInt32("rate", &keyRepeatRate) == B_OK) {
		fKeyboardSettings.SetKeyboardRepeatRate(keyRepeatRate);
		be_app_messenger.SendMessage(IS_SAVE_SETTINGS);

		BMessage msg(IS_CONTROL_DEVICES);
		msg.AddInt32("type", B_KEYBOARD_DEVICE);
		msg.AddInt32("code", B_KEY_REPEAT_RATE_CHANGED);
		return fAddOnManager->PostMessage(&msg);
	}

	return reply->AddInt32("rate", fKeyboardSettings.KeyboardRepeatRate());
}


status_t
InputServer::HandleGetSetKeyMap(BMessage* message, BMessage* reply)
{
	CALLED();

	if (message->what == IS_GET_KEY_MAP) {
		status_t status = reply->AddData("keymap", B_ANY_TYPE, &fKeys, sizeof(fKeys));
		if (status == B_OK)
			status = reply->AddData("key_buffer", B_ANY_TYPE, fChars, fCharsSize);

		return status;
	}

	if (_LoadKeymap() != B_OK)
		_LoadSystemKeymap();

	BMessage msg(IS_CONTROL_DEVICES);
	msg.AddInt32("type", B_KEYBOARD_DEVICE);
	msg.AddInt32("code", B_KEY_MAP_CHANGED);
	return fAddOnManager->PostMessage(&msg);
}


status_t
InputServer::HandleFocusUnfocusIMAwareView(BMessage* message,
	BMessage* reply)
{
	CALLED();

	BMessenger messenger;
	status_t status = message->FindMessenger("view", &messenger);
	if (status != B_OK)
		return status;

	// check if current view is ours

	if (message->what == IS_FOCUS_IM_AWARE_VIEW) {
		PRINT(("HandleFocusUnfocusIMAwareView : entering\n"));
		fInputMethodAware = true;
	} else {
		PRINT(("HandleFocusUnfocusIMAwareView : leaving\n"));
		fInputMethodAware = false;
	}

	return B_OK;
}


/*!	Enqueues the message into the event queue.
	The message must only be deleted in case this method returns an error.
*/
status_t
InputServer::EnqueueDeviceMessage(BMessage* message)
{
	CALLED();

	BAutolock _(fEventQueueLock);
	if (!fEventQueue.AddItem(message))
		return B_NO_MEMORY;

	if (fEventQueue.CountItems() == 1) {
		// notify event loop only if we haven't already done so
		write_port_etc(fEventLooperPort, 1, NULL, 0, B_RELATIVE_TIMEOUT, 0);
	}
	return B_OK;
}


/*!	Enqueues the message into the method queue.
	The message must only be deleted in case this method returns an error.
*/
status_t
InputServer::EnqueueMethodMessage(BMessage* message)
{
	CALLED();
	PRINT(("%s what:%c%c%c%c\n", __PRETTY_FUNCTION__, (char)(message->what >> 24),
		(char)(message->what >> 16), (char)(message->what >> 8), (char)message->what));

#ifdef DEBUG
	if (message->what == 'IMEV') {
		int32 code;
		message->FindInt32("be:opcode", &code);
		PRINT(("%s be:opcode %li\n", __PRETTY_FUNCTION__, code));
	}
#endif

	BAutolock _(fEventQueueLock);
	if (!fMethodQueue.AddItem(message))
		return B_NO_MEMORY;

	if (fMethodQueue.CountItems() == 1) {
		// notify event loop only if we haven't already done so
		write_port_etc(fEventLooperPort, 0, NULL, 0, B_RELATIVE_TIMEOUT, 0);
	}
	return B_OK;
}


status_t
InputServer::SetNextMethod(bool direction)
{
	gInputMethodListLocker.Lock();

	int32 index = gInputMethodList.IndexOf(fActiveMethod);
	index += (direction ? 1 : -1);

	if (index < -1)
		index = gInputMethodList.CountItems() - 1;
	if (index >= gInputMethodList.CountItems())
		index = -1;

	BInputServerMethod *method = &gKeymapMethod;

	if (index != -1)
		method = (BInputServerMethod *)gInputMethodList.ItemAt(index);

	SetActiveMethod(method);

	gInputMethodListLocker.Unlock();
	return B_OK;
}


void
InputServer::SetActiveMethod(BInputServerMethod* method)
{
	CALLED();
	if (fActiveMethod)
		fActiveMethod->fOwner->MethodActivated(false);

	fActiveMethod = method;

	if (fActiveMethod)
		fActiveMethod->fOwner->MethodActivated(true);
}


const BMessenger*
InputServer::MethodReplicant()
{
	return fReplicantMessenger;
}


void
InputServer::SetMethodReplicant(const BMessenger* messenger)
{
	fReplicantMessenger = messenger;
}


bool
InputServer::EventLoopRunning()
{
	return fEventLooperPort >= B_OK;
}


status_t
InputServer::GetDeviceInfo(const char* name, input_device_type *_type,
	bool *_isRunning)
{
    BAutolock lock(fInputDeviceListLocker);

	for (int32 i = fInputDeviceList.CountItems() - 1; i >= 0; i--) {
		InputDeviceListItem* item = (InputDeviceListItem*)fInputDeviceList.ItemAt(i);

		if (item->HasName(name)) {
			if (_type)
				*_type = item->Type();
			if (_isRunning)
				*_isRunning = item->Running();

			return B_OK;
		}
	}

	return B_NAME_NOT_FOUND;
}


status_t
InputServer::GetDeviceInfos(BMessage *msg)
{
	CALLED();
	BAutolock lock(fInputDeviceListLocker);

	for (int32 i = fInputDeviceList.CountItems() - 1; i >= 0; i--) {
		InputDeviceListItem* item = (InputDeviceListItem*)fInputDeviceList.ItemAt(i);
		msg->AddString("device", item->Name());
		msg->AddInt32("type", item->Type());
	}
	return B_OK;
}


status_t
InputServer::UnregisterDevices(BInputServerDevice& serverDevice,
	input_device_ref **devices)
{
    CALLED();
    BAutolock lock(fInputDeviceListLocker);

	if (devices != NULL) {
		// remove the devices as specified only
		input_device_ref *device = NULL;
		for (int32 i = 0; (device = devices[i]) != NULL; i++) {
			for (int32 j = fInputDeviceList.CountItems() - 1; j >= 0; j--) {
				InputDeviceListItem* item = (InputDeviceListItem*)fInputDeviceList.ItemAt(j);

				if (item->ServerDevice() == &serverDevice && item->HasName(device->name)) {
					item->Stop();
					if (fInputDeviceList.RemoveItem(j))
						delete item;
					break;
				}
			}
		}
	} else {
		// remove all devices from this BInputServerObject
		for (int32 i = fInputDeviceList.CountItems() - 1; i >= 0; i--) {
			InputDeviceListItem* item = (InputDeviceListItem*)fInputDeviceList.ItemAt(i);

			if (item->ServerDevice() == &serverDevice) {
				item->Stop();
				if (fInputDeviceList.RemoveItem(i))
					delete item;
			}
		}
	}

    return B_OK;
}


status_t
InputServer::RegisterDevices(BInputServerDevice& serverDevice,
	input_device_ref** devices)
{
	if (devices == NULL)
		return B_BAD_VALUE;

	BAutolock lock(fInputDeviceListLocker);

	input_device_ref *device = NULL;
	for (int32 i = 0; (device = devices[i]) != NULL; i++) {
		if (device->type != B_POINTING_DEVICE
			&& device->type != B_KEYBOARD_DEVICE
			&& device->type != B_UNDEFINED_DEVICE)
			continue;

		// find existing input server device

		bool found = false;
		for (int32 j = fInputDeviceList.CountItems() - 1; j >= 0; j--) {
			InputDeviceListItem* item = (InputDeviceListItem*)fInputDeviceList.ItemAt(j);

			if (item->HasName(device->name)) {
debug_printf("InputServer::RegisterDevices() device_ref already exists: %s\n", device->name);
				PRINT(("RegisterDevices found %s\n", device->name));
				found = true;
				break;
			}
		}

		if (!found) {
			PRINT(("RegisterDevices not found %s\n", device->name));
			InputDeviceListItem* item = new (nothrow) InputDeviceListItem(serverDevice,
				*device);
			if (item != NULL && fInputDeviceList.AddItem(item)) {
				item->Start();
			} else {
				delete item;
				return B_NO_MEMORY;
			}
		}
	}

	return B_OK;
}


status_t
InputServer::StartStopDevices(const char* name, input_device_type type,
	bool doStart)
{
	CALLED();
	BAutolock lock(fInputDeviceListLocker);

	for (int32 i = fInputDeviceList.CountItems() - 1; i >= 0; i--) {
		InputDeviceListItem* item
			= (InputDeviceListItem*)fInputDeviceList.ItemAt(i);
		if (!item)
			continue;

		if (item->Matches(name, type)) {
			if (doStart == item->Running()) {
				if (name)
					return B_OK;
				else
					continue;
			}

			if (doStart)
				item->Start();
			else
				item->Stop();

			if (name)
				return B_OK;
		}
	}

	if (name) {
		// item not found
		return B_ERROR;
	}

	return B_OK;
}



status_t
InputServer::StartStopDevices(BInputServerDevice& serverDevice, bool doStart)
{
	CALLED();
	BAutolock lock(fInputDeviceListLocker);

	for (int32 i = fInputDeviceList.CountItems() - 1; i >= 0; i--) {
		InputDeviceListItem* item = (InputDeviceListItem*)fInputDeviceList.ItemAt(i);

		if (item->ServerDevice() != &serverDevice)
			continue;

		if (doStart == item->Running())
			continue;

		if (doStart)
			item->Start();
		else
			item->Stop();
	}

	return B_OK;
}


status_t
InputServer::ControlDevices(const char* name, input_device_type type,
	uint32 code, BMessage* message)
{
	CALLED();
	BAutolock lock(fInputDeviceListLocker);

	for (int32 i = fInputDeviceList.CountItems() - 1; i >= 0; i--) {
		InputDeviceListItem* item = (InputDeviceListItem*)fInputDeviceList.ItemAt(i);
		if (!item)
			continue;

		if (item->Matches(name, type)) {
			item->Control(code, message);

			if (name)
				return B_OK;
		}
	}

	if (name)
		return B_ERROR;

	return B_OK;
}


bool
InputServer::DoMouseAcceleration(int32* _x, int32* _y)
{
	CALLED();
	int32 speed = fMouseSettings.MouseSpeed() >> 15;

	// ToDo: implement mouse acceleration
	(void)speed;
	//*_y = *_x * speed;
	PRINT(("DoMouse : %ld %ld %ld %ld\n", *_x, *_y, speed, fMouseSettings.MouseSpeed()));
	return true;
}


bool
InputServer::SetMousePos(long *, long *, long, long)
{
	CALLED();
	return true;
}


bool
InputServer::SetMousePos(long *, long *, BPoint)
{
	CALLED();
	return true;
}


bool
InputServer::SetMousePos(long *, long *, float, float)
{
	CALLED();
	return true;
}


bool
InputServer::SafeMode()
{
	return fSafeMode;
}


status_t
InputServer::_StartEventLoop()
{
	CALLED();
	fEventLooperPort = create_port(100, "input server events");
	if (fEventLooperPort < 0) {
		PRINTERR(("InputServer: create_port error: (0x%lx) %s\n",
			fEventLooperPort, strerror(fEventLooperPort)));
		return fEventLooperPort;
	}

	thread_id thread = spawn_thread(_EventLooper, "_input_server_event_loop_",
		B_REAL_TIME_DISPLAY_PRIORITY + 3, this);
	if (thread < B_OK || resume_thread(thread) < B_OK) {
		if (thread >= B_OK)
			kill_thread(thread);
		delete_port(fEventLooperPort);
		fEventLooperPort = -1;
		return thread < B_OK ? thread : B_ERROR;
	}

	return B_OK;
}


status_t
InputServer::_EventLooper(void* arg)
{
	InputServer* self = (InputServer*)arg;
	self->_EventLoop();

	return B_OK;
}


void
InputServer::_EventLoop()
{
	while (true) {
		// Block until we find the size of the next message
		ssize_t length = port_buffer_size(fEventLooperPort);
		if (length < B_OK) {
			PRINT(("[Event Looper] port gone, exiting.\n"));
			return;
		}

		PRINT(("[Event Looper] BMessage Size = %lu\n", length));

		char buffer[length];
		int32 code;
		status_t err = read_port(fEventLooperPort, &code, buffer, length);
		if (err != length) {
			if (err >= 0) {
				PRINTERR(("InputServer: failed to read full packet (read %lu of %lu)\n", err, length));
			} else {
				PRINTERR(("InputServer: read_port error: (0x%lx) %s\n", err, strerror(err)));
			}
			continue;
		}

		EventList events;
		if (fEventQueueLock.Lock()) {
			// move the items to our own list to block the event queue as short as possible
			events.AddList(&fEventQueue);
			fEventQueue.MakeEmpty();
			fEventQueueLock.Unlock();
		}

		if (length > 0) {
			BMessage* event = new BMessage;

			if ((err = event->Unflatten(buffer)) < 0) {
				PRINTERR(("[InputServer] Unflatten() error: (0x%lx) %s\n", err, strerror(err)));
				delete event;
				continue;
			}

			events.AddItem(event);
		}

		// This is where the message should be processed.

		if (_SanitizeEvents(events)
			&& _MethodizeEvents(events)
			&& _FilterEvents(events)) {
			_UpdateMouseAndKeys(events);
			_DispatchEvents(events);
		}
	}
}


/*!	Updates the internal mouse position and keyboard info. */
void
InputServer::_UpdateMouseAndKeys(EventList& events)
{
	for (int32 index = 0;BMessage* event = (BMessage*)events.ItemAt(index);
			index++) {
		switch (event->what) {
			case B_MOUSE_DOWN:
			case B_MOUSE_UP:
			case B_MOUSE_MOVED:
				event->FindPoint("where", &fMousePos);
				break;

			case B_KEY_DOWN:
			case B_UNMAPPED_KEY_DOWN:
				// update modifiers
				uint32 modifiers;
				if (event->FindInt32("modifiers", (int32*)&modifiers) == B_OK)
					fKeyInfo.modifiers = modifiers;

				// update key states
				const uint8 *data;
				ssize_t size;
				if (event->FindData("states", B_UINT8_TYPE,
						(const void**)&data, &size) == B_OK) {
					PRINT(("updated keyinfo\n"));
					if (size == sizeof(fKeyInfo.key_states))
						memcpy(fKeyInfo.key_states, data, size);
				}

				if (fActiveMethod == NULL)
					break;

				// we scan for Alt+Space key down events which means we change
				// to next input method
				// (pressing "shift" will let us switch to the previous method)

				PRINT(("SanitizeEvents: %lx, %x\n", fKeyInfo.modifiers,
					fKeyInfo.key_states[KEY_Spacebar >> 3]));

				uint8 byte;
				if (event->FindInt8("byte", (int8*)&byte) < B_OK)
					byte = 0;

				if (((fKeyInfo.modifiers & B_COMMAND_KEY) != 0 && byte == ' ')
					|| byte == B_HANKAKU_ZENKAKU) {
					SetNextMethod(!(fKeyInfo.modifiers & B_SHIFT_KEY));

					// this event isn't sent to the user
					events.RemoveItemAt(index);
					delete event;
					continue;
				}
				break;
		}
	}
}


/*!	Frees events from unwanted fields, adds missing fields, and removes
	unwanted events altogether.
*/
bool
InputServer::_SanitizeEvents(EventList& events)
{
	CALLED();

	for (int32 index = 0; BMessage* event = (BMessage*)events.ItemAt(index);
			index++) {
		switch (event->what) {
	   		case B_MOUSE_MOVED:
	   		case B_MOUSE_DOWN:
	   		{
	   			int32 buttons;
	   			if (event->FindInt32("buttons", &buttons) != B_OK)
	   				event->AddInt32("buttons", 0);

	   			// supposed to fall through
	   		}
	   		case B_MOUSE_UP:
	   		{
	   			BPoint where;
				int32 x, y;
				float absX, absY;

				if (event->FindInt32("x", &x) == B_OK
					&& event->FindInt32("y", &y) == B_OK) {
					where.x = fMousePos.x + x;
					where.y = fMousePos.y - y;

					event->RemoveName("x");
					event->RemoveName("y");
					event->AddInt32("be:delta_x", x);
					event->AddInt32("be:delta_y", y);

					PRINT(("new position: %f, %f, %ld, %ld\n",
						where.x, where.y, x, y));
				} else if (event->FindFloat("x", &absX) == B_OK
					&& event->FindFloat("y", &absY) == B_OK) {
					// The device gives us absolute screen coords in range 0..1;
					// convert them to absolute screen position
					// (the message is supposed to contain the original
					// absolute coordinates as "be:tablet_x/y").
					where.x = absX * fFrame.Width();
					where.y = absY * fFrame.Height();

					event->RemoveName("x");
					event->RemoveName("y");
					PRINT(("new position : %f, %f\n", where.x, where.y));
				} else if (event->FindPoint("where", &where) == B_OK) {
					PRINT(("new position : %f, %f\n", where.x, where.y));
				}

				// Constrain and filter the mouse coords and add the final
				// point to the message.
				where.x = roundf(where.x);
				where.y = roundf(where.y);
				where.ConstrainTo(fFrame);
				if (event->ReplacePoint("where", where) != B_OK)
					event->AddPoint("where", where);

				if (!event->HasInt64("when"))
					event->AddInt64("when", system_time());

				event->AddInt32("modifiers", fKeyInfo.modifiers);
				break;
	   		}
			case B_KEY_DOWN:
			case B_UNMAPPED_KEY_DOWN:
				// add modifiers
				if (!event->HasInt32("modifiers"))
					event->AddInt32("modifiers", fKeyInfo.modifiers);

				// add key states
				if (!event->HasData("states", B_UINT8_TYPE)) {
					event->AddData("states", B_UINT8_TYPE, fKeyInfo.key_states,
						sizeof(fKeyInfo.key_states));
				}
				break;
		}
	}

	return true;
}


/*!	Applies the filters of the active input method to the
	incoming events. It will also move the events in the method
	queue to the event list.
*/
bool
InputServer::_MethodizeEvents(EventList& events)
{
	CALLED();

	if (fActiveMethod == NULL)
		return true;

	int32 count = events.CountItems();
	for (int32 i = 0; i < count;) {
		_FilterEvent(fActiveMethod, events, i, count);
	}

	{
		// move the method events into the event queue - they are not
		// "methodized" either
		BAutolock _(fEventQueueLock);
		events.AddList(&fMethodQueue);
		fMethodQueue.MakeEmpty();
	}

	if (!fInputMethodAware) {
		// special handling for non-input-method-aware views

		int32 newCount = events.CountItems();
			// we may add new events in this loop that don't need to be checked again

		for (int32 i = 0; i < newCount; i++) {
			BMessage* event = events.ItemAt(i);

			if (event->what != B_INPUT_METHOD_EVENT)
				continue;

			SERIAL_PRINT(("IME received\n"));

			bool removeEvent = true;

			int32 opcode;
			if (event->FindInt32("be:opcode", &opcode) == B_OK) {
				bool inlineOnly;
				if (event->FindBool("be:inline_only", &inlineOnly) != B_OK)
					inlineOnly = false;

				if (inlineOnly) {
					BMessage translated;
					bool confirmed;
					if (opcode == B_INPUT_METHOD_CHANGED
						&& event->FindBool("be:confirmed", &confirmed) == B_OK && confirmed
						&& event->FindMessage("be:translated", &translated) == B_OK) {
						// translate event for the non-aware view
						*event = translated;
						removeEvent = false;
					}
				} else {
					if (fInputMethodWindow == NULL
						&& opcode == B_INPUT_METHOD_STARTED)
						fInputMethodWindow = new (nothrow) BottomlineWindow();

					if (fInputMethodWindow != NULL) {
						EventList newEvents;
						fInputMethodWindow->HandleInputMethodEvent(event, newEvents);

						if (!newEvents.IsEmpty()) {
							events.AddList(&newEvents);
							opcode = B_INPUT_METHOD_STOPPED;
						}

						if (opcode == B_INPUT_METHOD_STOPPED) {
							fInputMethodWindow->PostMessage(B_QUIT_REQUESTED);
							fInputMethodWindow = NULL;
						}
					}
				}
			}

			if (removeEvent) {
				// the inline/bottom window has eaten the event
				events.RemoveItemAt(i--);
				delete event;
				newCount--;
			}
		}
	}

	return events.CountItems() > 0;
}


/*!	This method applies all defined filters to each event in the
	supplied list.  The supplied list is modified to reflect the
	output of the filters.
	The method returns true if the filters were applied to all
	events without error and false otherwise.
*/
bool
InputServer::_FilterEvents(EventList& events)
{
	CALLED();
	BAutolock _(gInputFilterListLocker);

	int32 count = gInputFilterList.CountItems();
	int32 eventCount = events.CountItems();

	for (int32 i = 0; i < count; i++) {
		BInputServerFilter* filter = (BInputServerFilter*)gInputFilterList.ItemAt(i);

		// Apply the current filter to all available event messages.

		for (int32 eventIndex = 0; eventIndex < eventCount;) {
			_FilterEvent(filter, events, eventIndex, eventCount);
		}
	}

	return eventCount != 0;
}


void
InputServer::_DispatchEvents(EventList& events)
{
	CALLED();

	int32 count = events.CountItems();

	for (int32 i = 0; i < count; i++) {
		BMessage* event = events.ItemAt(i);

		// now we must send each event to the app_server
		_DispatchEvent(event);
		delete event;
	}

	events.MakeEmpty();
}


/*!	Applies the given filter to the event list.
	For your convenience, it also alters the \a index and \a count arguments
	ready for the next call to this method.
*/
void
InputServer::_FilterEvent(BInputServerFilter* filter, EventList& events,
	int32& index, int32& count)
{
	BMessage* event = events.ItemAt(index);

	BList newEvents;
	filter_result result = filter->Filter(event, &newEvents);

	if (result == B_SKIP_MESSAGE || newEvents.CountItems() > 0) {
		// we no longer need the current event
		events.RemoveItemAt(index);
		delete event;

		if (result == B_DISPATCH_MESSAGE) {
			EventList addedEvents;
			EventList::Private(&addedEvents).AsBList()->AddList(&newEvents);
			_SanitizeEvents(addedEvents);
			// add the new events - but don't methodize them again
			events.AddList(&addedEvents, index);
			index += newEvents.CountItems();
			count = events.CountItems();
		} else
			count--;
	} else
		index++;
}


status_t
InputServer::_DispatchEvent(BMessage* event)
{
	CALLED();

   	switch (event->what) {
		case B_MOUSE_MOVED:
		case B_MOUSE_DOWN:
		case B_MOUSE_UP:
			if (fCursorBuffer) {
				atomic_set((int32*)&fCursorBuffer->pos, (uint32)fMousePos.x << 16UL
					| ((uint32)fMousePos.y & 0xffff));
				if (atomic_or(&fCursorBuffer->read, 1) == 0)
					release_sem(fCursorSem);
        	}
        	break;

		case B_KEY_DOWN:
		case B_KEY_UP:
		case B_UNMAPPED_KEY_DOWN:
		case B_UNMAPPED_KEY_UP:
		case B_MODIFIERS_CHANGED:
		{
			// update or add modifiers
			uint32 modifiers;
			if (event->FindInt32("modifiers", (int32*)&modifiers) == B_OK)
				fKeyInfo.modifiers = modifiers;
			else
				event->AddInt32("modifiers", fKeyInfo.modifiers);

			// update or add key states
			const uint8 *data;
			ssize_t size;
			if (event->FindData("states", B_UINT8_TYPE,
					(const void**)&data, &size) == B_OK) {
				PRINT(("updated keyinfo\n"));
				if (size == sizeof(fKeyInfo.key_states))
					memcpy(fKeyInfo.key_states, data, size);
			} else {
				event->AddData("states", B_UINT8_TYPE, fKeyInfo.key_states,
					sizeof(fKeyInfo.key_states));
			}

			break;
		}

   		default:
			break;
	}

	BMessenger reply;
	BMessage::Private messagePrivate(event);
	return messagePrivate.SendMessage(fAppServerPort, -1, 0, 0, false, reply);
}


//	#pragma mark -


extern "C" void
RegisterDevices(input_device_ref** devices)
{
	CALLED();
}


BView *
instantiate_deskbar_item()
{
	return new MethodReplicant(INPUTSERVER_SIGNATURE);
}


//	#pragma mark -


int
main(int /*argc*/, char** /*argv*/)
{
	InputServer	*inputServer = new InputServer;

	inputServer->Run();
	delete inputServer;

	return 0;
}


