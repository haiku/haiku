/*
 * Copyright 2004-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini
 */

#ifndef __MOUSEINPUTDEVICE_H
#define __MOUSEINPUTDEVICE_H

#include <InputServerDevice.h>
#include <InterfaceDefs.h>
#include <List.h>
#include <stdio.h>

struct mouse_device;
class MouseInputDevice : public BInputServerDevice {
public:
	MouseInputDevice();
	~MouseInputDevice();
	
	virtual status_t InitCheck();
	
	virtual status_t Start(const char *name, void *cookie);
	virtual status_t Stop(const char *name, void *cookie);
	
	virtual status_t Control(const char *name, void *cookie,
							 uint32 command, BMessage *message);
private:
	status_t HandleMonitor(BMessage *message);
	status_t InitFromSettings(void *cookie, uint32 opcode = 0);
	void RecursiveScan(const char *directory);
	
	status_t AddDevice(const char *path);
	status_t RemoveDevice(const char *path);
	
	int32 DeviceWatcher(mouse_device *device);
	static int32 ThreadFunction(void *arg);
	
	uint32 Remap(mouse_device* device, uint32 buttons);
			
	BList fDevices;
#ifdef DEBUG
public:
	static FILE *sLogFile;
#endif
};

extern "C" BInputServerDevice *instantiate_input_device();

#endif

