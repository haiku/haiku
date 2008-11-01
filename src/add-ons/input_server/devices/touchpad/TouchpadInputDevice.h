/*
 * Copyright 2004-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */
#ifndef MOUSE_INPUT_DEVICE_H
#define MOUSE_INPUT_DEVICE_H


#include <InputServerDevice.h>
#include <InterfaceDefs.h>
#include <List.h>

#include <stdio.h>
#define DEBUG 1

class TouchpadDevice;

class TouchpadInputDevice : public BInputServerDevice {
	public:
		TouchpadInputDevice();
		~TouchpadInputDevice();

		virtual status_t InitCheck();

		virtual status_t Start(const char* name, void* cookie);
		virtual status_t Stop(const char* name, void* cookie);

		virtual status_t Control(const char* name, void* cookie,
							uint32 command, BMessage* message);

	private:
		status_t _HandleMonitor(BMessage* message);
		void _RecursiveScan(const char* directory);

		TouchpadDevice* _FindDevice(const char* path);
		status_t _AddDevice(const char* path);
		status_t _RemoveDevice(const char* path);

		BList fDevices;
#ifdef DEBUG
public:
	static FILE *sLogFile;
#endif
};

extern "C" BInputServerDevice* instantiate_input_device();

#endif	// MOUSE_INPUT_DEVICE_H
