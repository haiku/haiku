/*****************************************************************************/
// Mouse input server device addon 
// Written by Stefano Ceccherini 
//
// MouseInputDevice.h
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
#ifndef __MOUSEINPUTDEVICE_H
#define __MOUSEINPUTDEVICE_H

#include <InputServerDevice.h>
#include <InterfaceDefs.h>
#include <stdio.h>

// TODO : these structs are needed in : input_server, mouse driver, mouse addon, mouse prefs
// => factorisation has to be done in a global header, possibly private

typedef struct {
        bool    enabled;        // Acceleration on / off
        int32   accel_factor;   // accel factor: 256 = step by 1, 128 = step by 1/2
        int32   speed;          // speed accelerator (1=1X, 2 = 2x)...
} mouse_accel;

typedef struct {
        int32		    type;
        mouse_map       map;
        mouse_accel     accel;
        bigtime_t       click_speed;
} mouse_settings;

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
	status_t InitFromSettings(uint32 opcode = 0);
	
	int32 fButtons;
	
	thread_id fThread;
	int fFd;
	bool fQuit;
	FILE *fLogFile;
	
	static int32 DeviceWatcher(void *arg);
	
	mouse_settings	fSettings;
};

extern "C" BInputServerDevice *instantiate_input_device();

#endif

