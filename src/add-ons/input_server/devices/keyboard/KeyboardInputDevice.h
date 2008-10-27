/*
 * Copyright 2004-2008, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef KEYBOARD_INPUT_DEVICE_H
#define KEYBOARD_INPUT_DEVICE_H


#include <Handler.h>
#include <InputServerDevice.h>
#include <Locker.h>
#include <List.h>

#include <InputServerTypes.h>
#include <ObjectList.h>

#include "Keymap.h"
#include "TeamMonitorWindow.h"
#include "kb_mouse_settings.h"


class KeyboardInputDevice;

class KeyboardDevice : public BHandler {
public:
								KeyboardDevice(KeyboardInputDevice* owner,
									const char* path);
	virtual						~KeyboardDevice();

	virtual	void				MessageReceived(BMessage* message);
			status_t			EnqueueInlineInputMethod(int32 opcode,
									const char* string = NULL,
									bool confirmed = false,
									BMessage* keyDown = NULL);

			status_t			Start();
			void				Stop();

			status_t			InitFromSettings(uint32 opcode = 0);
			void				UpdateLEDs();

			const char*			Path() const { return fPath; }
			input_device_ref*	DeviceRef() { return &fDeviceRef; }

private:
	static	int32				_ThreadEntry(void* arg);
			int32				_Thread();

private:
	KeyboardInputDevice*		fOwner;
	input_device_ref			fDeviceRef;
	char						fPath[B_PATH_NAME_LENGTH];
	int							fFD;
	thread_id					fThread;
	kb_settings					fSettings;
	volatile bool				fActive;
	bool						fIsAT;
	volatile bool				fInputMethodStarted;
	uint32						fModifiers;

			Keymap				fKeymap;
			BLocker				fKeymapLock;
};


class KeyboardInputDevice : public BInputServerDevice {
public:
								KeyboardInputDevice();
	virtual						~KeyboardInputDevice();

	virtual	status_t			InitCheck();

	virtual	status_t			Start(const char* name, void* cookie);
	virtual status_t			Stop(const char* name, void* cookie);

	virtual status_t			Control(const char* name, void* cookie,
									uint32 command, BMessage* message);

	virtual status_t			SystemShuttingDown();

private:
	friend struct KeyboardDevice;
	// TODO: needed by the control thread to remove a dead device
	// find a better way...

			status_t			_HandleMonitor(BMessage* message);
			void				_RecursiveScan(const char* directory);

			KeyboardDevice*		_FindDevice(const char* path) const;
			status_t			_AddDevice(const char* path);
			status_t			_RemoveDevice(const char* path);

			BObjectList<KeyboardDevice> fDevices;
			TeamMonitorWindow*	fTeamMonitorWindow;
};

extern "C" BInputServerDevice* instantiate_input_device();

#endif	// KEYBOARD_INPUT_DEVICE_H
