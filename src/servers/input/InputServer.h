/*****************************************************************************/
// OpenBeOS input_server Backend Application
//
// This is the primary application class for the OpenBeOS input_server.
//
//
// Copyright (c) 2001 OpenBeOS Project
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


#ifndef INPUTSERVERAPP_H
#define INPUTSERVERAPP_H

// BeAPI Headers
#include <Application.h>
#include "InputServerDevice.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <Debug.h>
#include <FindDirectory.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <OS.h>
#include <SupportDefs.h>

#include "InputServerTypes.h"

/*****************************************************************************/
// InputServer
//
// Application class for input_server.
/*****************************************************************************/

class InputServer : public BApplication
{
	typedef BApplication Inherited;
public:
	InputServer(void);
	~InputServer(void);

	void ArgvReceived(long, char**);

	void InitKeyboardMouseStates(void);
	
	void InitDevices(void);
	void InitFilters(void);
	void InitMethods(void);

	bool QuitRequested(void);
	void ReadyToRun(void);
	//thread_id Run(void);
	void MessageReceived(BMessage*); 

	void HandleSetMethod(BMessage*);
	void HandleGetSetMouseType(BMessage*, BMessage*);
	void HandleGetSetMouseAcceleration(BMessage*, BMessage*);
	void HandleGetSetKeyRepeatDelay(BMessage*, BMessage*);
	void HandleGetKeyInfo(BMessage*, BMessage*);
	void HandleGetModifiers(BMessage*, BMessage*);
	void HandleSetModifierKey(BMessage*, BMessage*);
	void HandleSetKeyboardLocks(BMessage*, BMessage*);
	void HandleGetSetMouseSpeed(BMessage*, BMessage*);
	void HandleSetMousePosition(BMessage*, BMessage*);
	void HandleGetSetMouseMap(BMessage*, BMessage*);
	void HandleGetKeyboardID(BMessage*, BMessage*);
	void HandleGetSetClickSpeed(BMessage*, BMessage*);
	void HandleGetSetKeyRepeatRate(BMessage*, BMessage*);
	void HandleGetSetKeyMap(BMessage*, BMessage*);
	void HandleFocusUnfocusIMAwareView(BMessage*, BMessage*);

	status_t EnqueueDeviceMessage(BMessage*);
	status_t EnqueueMethodMessage(BMessage*);
	status_t UnlockMethodQueue(void);
	status_t LockMethodQueue(void);
	status_t SetNextMethod(bool);
	//SetActiveMethod(_BMethodAddOn_*);
	const BMessenger* MethodReplicant(void);

	status_t EventLoop(void*);
	bool     EventLoopRunning(void);

	bool DispatchEvents(BList*);
	bool CacheEvents(BList*);
	const BList* GetNextEvents(BList*);
	bool FilterEvents(BList*);
	bool SanitizeEvents(BList*);
	bool MethodizeEvents(BList*, bool);

	status_t StartStopDevices(const char *, input_device_type, bool);
	status_t ControlDevices(const char *, input_device_type, unsigned long, BMessage*);

	bool DoMouseAcceleration(long*, long*);
	bool SetMousePos(long*, long*, long, long);
	bool SetMousePos(long*, long*, BPoint);
	bool SetMousePos(long*, long*, float, float);

	bool SafeMode(void);

private:
	status_t AddInputServerDevice(const char* path);
	
	bool sEventLoopRunning;
	bool sSafeMode;
	
	BList mInputServerDeviceList;
	BList mInputMethodList;
	BList mInputFilterList;
	port_id ISPort;
	port_id EventLooperPort;
	thread_id ISPortThread;
	static int32 ISPortWatcher(void *arg);
	void WatchPort();

	//fMouseState;
};

#endif