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

class PortLink;

// Constants
#define SET_METHOD					'stmd'
#define GET_MOUSE_TYPE				'gtmt'
#define SET_MOUSE_TYPE 				'stmt'
#define GET_MOUSE_ACCELERATION 		'gtma'
#define SET_MOUSE_ACCELERATION 		'stma'
#define GET_KEY_REPEAT_DELAY 		'gkrd'
#define SET_KEY_REPEAT_DELAY 		'skrd'
#define GET_KEY_INFO 				'ktki'
#define GET_MODIFIERS 				'gtmf'
#define SET_MODIFIER_KEY 			'smky'
#define SET_KEYBOARD_LOCKS 			'skbl'
#define GET_MOUSE_SPEED 			'gtms'
#define SET_MOUSE_SPEED 			'stms'
#define SET_MOUSE_POSITION 			'stmp'
#define GET_MOUSE_MAP 				'gtmm'
#define SET_MOUSE_MAP 				'stmm'
#define GET_KEYBOARD_ID 			'gkbi'
#define GET_CLICK_SPEED 			'gtcs'
#define SET_CLICK_SPEED 			'stcs'
#define GET_KEY_REPEAT_RATE 		'gkrr'
#define SET_KEY_REPEAT_RATE 		'skrr'
#define GET_KEY_MAP 				'gtkm'
#define SET_KEY_MAP 				'stkm'
#define FOCUS_IM_AWARE_VIEW 		'fiav'
#define UNFOCUS_IM_AWARE_VIEW 		'uiav'

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
	void HandleGetMouseType(BMessage*, BMessage*);
	void HandleSetMouseType(BMessage*, BMessage*);
	void HandleGetMouseAcceleration(BMessage*, BMessage*);
	void HandleSetMouseAcceleration(BMessage*, BMessage*);
	void HandleGetKeyRepeatDelay(BMessage*, BMessage*);
	void HandleSetKeyRepeatDelay(BMessage*, BMessage*);
	void HandleGetKeyInfo(BMessage*, BMessage*);
	void HandleGetModifiers(BMessage*, BMessage*);
	void HandleSetModifierKey(BMessage*, BMessage*);
	void HandleSetKeyboardLocks(BMessage*, BMessage*);
	void HandleGetMouseSpeed(BMessage*, BMessage*);
	void HandleSetMouseSpeed(BMessage*, BMessage*);
	void HandleSetMousePosition(BMessage*, BMessage*);
	void HandleGetMouseMap(BMessage*, BMessage*);
	void HandleSetMouseMap(BMessage*, BMessage*);
	void HandleGetKeyboardID(BMessage*, BMessage*);
	void HandleGetClickSpeed(BMessage*, BMessage*);
	void HandleSetClickSpeed(BMessage*, BMessage*);
	void HandleGetKeyRepeatRate(BMessage*, BMessage*);
	void HandleSetKeyRepeatRate(BMessage*, BMessage*);
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
	
	PortLink *serverlink;
	
	//fMouseState;
};


#endif
