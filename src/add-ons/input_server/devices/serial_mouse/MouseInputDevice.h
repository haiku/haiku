/*****************************************************************************/
// Mouse input server device addon
// Written by Stefano Ceccherini and Jerome Duval
// Adapted for serial mice by Oscar Lesta
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
#include <List.h>

#include <stdio.h>

class MouseInputDevice : public BInputServerDevice {
public:
	MouseInputDevice();
	~MouseInputDevice();

	virtual status_t InitCheck();

	virtual status_t Start(const char* name, void* cookie);
	virtual status_t Stop(const char* name, void* cookie);

	virtual status_t Control(const char *name, void* cookie,
							 uint32 command, BMessage* message);
private:
	status_t InitFromSettings(void* cookie, uint32 opcode = 0);

	static int32 DeviceWatcher(void* arg);

	BList fDevices;

#ifdef DEBUG
public:
	static FILE *sLogFile;
#endif
};

extern "C" BInputServerDevice *instantiate_input_device();

#endif
