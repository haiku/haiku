/*****************************************************************************/
// Keyboard input server device addon 
// Written by Jérôme Duval
//
// KeyboardInputDevice.h
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
#ifndef __KEYBOARDINPUTDEVICE_H
#define __KEYBOARDINPUTDEVICE_H

#include "Keymap.h"
#include "TMWindow.h"
#include <InputServerDevice.h>
#include <List.h>
#include <stdio.h>
#include "kb_mouse_settings.h"

class KeyboardInputDevice;

struct keyboard_device {
	KeyboardInputDevice *owner;
	input_device_ref device_ref;
	char path[B_PATH_NAME_LENGTH];
	int fd;
	thread_id device_watcher;
	kb_settings settings;
	bool active;
	bool isAT;
	uint32 modifiers;
};


class KeyboardInputDevice : public BInputServerDevice {
public:
	KeyboardInputDevice();
	~KeyboardInputDevice();
	
	virtual status_t InitCheck();
	
	virtual status_t Start(const char *name, void *cookie);
	virtual status_t Stop(const char *name, void *cookie);
	
	virtual status_t Control(const char *name, void *cookie,
							 uint32 command, BMessage *message);
							 
	virtual status_t SystemShuttingDown();
#ifdef DEBUG							 
	static FILE *sLogFile;
#endif
private:
	status_t HandleMonitor(BMessage *message);
	status_t InitFromSettings(void *cookie, uint32 opcode = 0);
	void RecursiveScan(const char *directory);
	
	status_t AddDevice(const char *path);
	status_t RemoveDevice(const char *path);
		
	static int32 DeviceWatcher(void *arg);
	static char *GetShortName(const char *longName);
	
	void SetLeds(keyboard_device *device);
	
	BList fDevices;
	Keymap	fKeymap;
	TMWindow *fTMWindow;
};

extern "C" BInputServerDevice *instantiate_input_device();

#if DEBUG
	inline void LOG(const char *fmt, ...) { char buf[1024]; va_list ap; va_start(ap, fmt); vsprintf(buf, fmt, ap); va_end(ap); \
		fputs(buf, KeyboardInputDevice::sLogFile); fflush(KeyboardInputDevice::sLogFile); }
	#define LOG_ERR(text...) LOG(text)
#else
	#define LOG(text...)
	#define LOG_ERR(text...) fprintf(stderr, text)
#endif

#define CALLED() LOG("%s\n", __PRETTY_FUNCTION__)

#endif

