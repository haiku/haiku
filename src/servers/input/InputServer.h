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
#include "InputServerFilter.h"
#include "InputServerMethod.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <Locker.h>

#include <Debug.h>
#include <FindDirectory.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <OS.h>
#include <SupportDefs.h>

class BPortLink;

class InputDeviceListItem
{
	public:
	BInputServerDevice* mIsd;    
	input_device_ref*   mDev;

	InputDeviceListItem(BInputServerDevice* isd, input_device_ref* dev):
		mIsd(isd), mDev(dev) {};	
};

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
	virtual ~InputServer(void);

	virtual void ArgvReceived(long, char**);

	void InitKeyboardMouseStates(void);
	
	void InitDevices(void);
	void InitFilters(void);
	void InitMethods(void);

	virtual bool QuitRequested(void);
	virtual void ReadyToRun(void);
	//thread_id Run(void);
	virtual void MessageReceived(BMessage*); 

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
	void HandleGetClickSpeed(BMessage*, BMessage*);
	void HandleSetClickSpeed(BMessage*, BMessage*);
	void HandleGetSetKeyRepeatRate(BMessage*, BMessage*);
	void HandleGetSetKeyMap(BMessage*, BMessage*);
	void HandleFocusUnfocusIMAwareView(BMessage*, BMessage*);

	void HandleFindDevices(BMessage*, BMessage*);
	void HandleWatchDevices(BMessage*, BMessage*);
	void HandleIsDeviceRunning(BMessage*, BMessage*);
	void HandleStartStopDevices(BMessage*, BMessage*);
	void HandleControlDevices(BMessage*, BMessage*);
	void HandleSystemShuttingDown(BMessage*, BMessage*);
	void HandleNodeMonitor(BMessage*);
	
	status_t EnqueueDeviceMessage(BMessage*);
	status_t EnqueueMethodMessage(BMessage*);
	status_t UnlockMethodQueue(void);
	status_t LockMethodQueue(void);
	status_t SetNextMethod(bool);
	//SetActiveMethod(_BMethodAddOn_*);
	const BMessenger* MethodReplicant(void);

	status_t EventLoop(void*);
	static bool     EventLoopRunning(void);

	bool DispatchEvents(BList*);
	int DispatchEvent(BMessage*);
	bool CacheEvents(BList*);
	const BList* GetNextEvents(BList*);
	bool FilterEvents(BList*);
	bool SanitizeEvents(BList*);
	bool MethodizeEvents(BList*, bool);

	static status_t StartStopDevices(const char *, input_device_type, bool);
	status_t ControlDevices(const char *, input_device_type, unsigned long, BMessage*);

	bool DoMouseAcceleration(long*, long*);
	bool SetMousePos(long*, long*, long, long);
	bool SetMousePos(long*, long*, BPoint);
	bool SetMousePos(long*, long*, float, float);

	bool SafeMode(void);

	static BList   gInputDeviceList;
	static BLocker gInputDeviceListLocker;
	
private:
	void InitTestDevice();
	
	status_t AddInputServerDevice(const char* path);
	status_t AddInputServerFilter(const char* path);
	status_t AddInputServerMethod(const char* path);
	
	bool 			sEventLoopRunning;
	bool 			sSafeMode;
	port_id 		sEventPort;
	BPoint			sMousePos;
	int32			sMouseType;
	int32			sMouseSpeed;
	int32			sMouseAcceleration;
	bigtime_t		sMouseClickSpeed;
	int32			sKeyRepeatRate;
	bigtime_t       sKeyRepeatDelay;
	mouse_map		sMouseMap;
	
	port_id			ISPort;
	port_id      	EventLooperPort;
	thread_id    	ISPortThread;
	static int32 	ISPortWatcher(void *arg);
	void WatchPort();
	
	static bool doStartStopDevice(void*, void*);
	
	static BList mInputServerDeviceList;
	static BList mInputServerFilterList;
	static BList mInputServerMethodList;
	
	// added this to communicate via portlink
	
	BPortLink *serverlink;
	
	//fMouseState;
};


#endif
