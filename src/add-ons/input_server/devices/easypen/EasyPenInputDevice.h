/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 *
 * References:
 *   Google search "technic doc genius" , http://www.bebits.com/app/2152
 */
 
#ifndef __EASYPENINPUTDEVICE_H
#define __EASYPENINPUTDEVICE_H

#include <InputServerDevice.h>
#include <List.h>
#include <stdio.h>

struct tablet_device;

class EasyPenInputDevice : public BInputServerDevice {
public:
	EasyPenInputDevice();
	~EasyPenInputDevice();
	
	virtual status_t InitCheck();
	
	virtual status_t Start(const char *name, void *cookie);
	virtual status_t Stop(const char *name, void *cookie);
	
	virtual status_t Control(const char *name, void *cookie,
							 uint32 command, BMessage *message);
private:
	static int32 DeviceWatcher(void *arg);
			
	BList fDevices;
	bigtime_t fClickSpeed;
#ifdef DEBUG
public:
	static FILE *sLogFile;
#endif
};

extern "C" BInputServerDevice *instantiate_input_device();

#endif

