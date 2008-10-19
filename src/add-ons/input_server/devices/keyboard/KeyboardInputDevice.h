/*
 * Copyright 2004-2008, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef KEYBOARD_INPUT_DEVICE_H
#define KEYBOARD_INPUT_DEVICE_H


#include "Keymap.h"
#include "TMWindow.h"
#include "kb_mouse_settings.h"

#include <Handler.h>
#include <InputServerDevice.h>
#include <Locker.h>
#include <List.h>

#include <ObjectList.h>


class KeyboardInputDevice;

struct keyboard_device : public BHandler {
							keyboard_device(const char* path);
	virtual					~keyboard_device();

	virtual	void			MessageReceived(BMessage* message);
			status_t		EnqueueInlineInputMethod(int32 opcode,
								const char* string = NULL,
								bool confirmed = false,
								BMessage* keyDown = NULL);

	KeyboardInputDevice*	owner;
	input_device_ref		device_ref;
	char					path[B_PATH_NAME_LENGTH];
	int						fd;
	thread_id				device_watcher;
	kb_settings				settings;
	volatile bool			active;
	bool					isAT;
	volatile bool			input_method_started;
	uint32					modifiers;
};


class KeyboardInputDevice : public BInputServerDevice {
public:
							KeyboardInputDevice();
							~KeyboardInputDevice();

	virtual	status_t		InitCheck();

	virtual	status_t		Start(const char* name, void* cookie);
	virtual status_t		Stop(const char* name, void* cookie);

	virtual status_t		Control(const char* name, void* cookie,
								uint32 command, BMessage* message);

	virtual status_t		SystemShuttingDown();

private:
			status_t		_HandleMonitor(BMessage* message);
			status_t		_InitFromSettings(void* cookie, uint32 opcode = 0);
			void			_RecursiveScan(const char* directory);

			status_t		_AddDevice(const char* path);
			status_t		_RemoveDevice(const char* path);

			void			_SetLeds(keyboard_device* device);

	static	int32			_DeviceWatcher(void* arg);

			BObjectList<keyboard_device> fDevices;
			Keymap			fKeymap;
			TMWindow*		fTeamMonitorWindow;
			BLocker			fKeymapLock;
};

extern "C" BInputServerDevice* instantiate_input_device();

#endif	// KEYBOARD_INPUT_DEVICE_H
