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
#include <InputServerDevice.h>
#include <InputServerFilter.h>
#include <InputServerMethod.h>
#include "AddOnManager.h"
#include "DeviceManager.h"

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

#if DEBUG>=1
	#define EXIT()		printf("EXIT %s\n", __PRETTY_FUNCTION__)
	#define CALLED()	printf("CALLED %s\n", __PRETTY_FUNCTION__)
#else
	#define EXIT()		((void)0)
	#define CALLED()	((void)0)
#endif

class BPortLink;

class InputDeviceListItem
{
	public:
	BInputServerDevice* mIsd;    
	input_device_ref   	mDev;
	bool 				mStarted;

	InputDeviceListItem(BInputServerDevice* isd, input_device_ref dev):
		mIsd(isd), mDev(dev), mStarted(false) {};	
};

class _BDeviceAddOn_
{
	
};

class _BMethodAddOn_
{

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
	
	//void InitDevices(void);
	//void InitFilters(void);
	//void InitMethods(void);

	virtual bool QuitRequested(void);
	virtual void ReadyToRun(void);
	//thread_id Run(void);
	virtual void MessageReceived(BMessage*); 

	void HandleSetMethod(BMessage*);
	status_t HandleGetSetMouseType(BMessage*, BMessage*);
	status_t HandleGetSetMouseAcceleration(BMessage*, BMessage*);
	status_t HandleGetSetKeyRepeatDelay(BMessage*, BMessage*);
	status_t HandleGetKeyInfo(BMessage*, BMessage*);
	status_t HandleGetModifiers(BMessage*, BMessage*);
	status_t HandleSetModifierKey(BMessage*, BMessage*);
	status_t HandleSetKeyboardLocks(BMessage*, BMessage*);
	status_t HandleGetSetMouseSpeed(BMessage*, BMessage*);
	status_t HandleSetMousePosition(BMessage*, BMessage*);
	status_t HandleGetSetMouseMap(BMessage*, BMessage*);
	status_t HandleGetKeyboardID(BMessage*, BMessage*);
	status_t HandleGetSetClickSpeed(BMessage*, BMessage*);
	status_t HandleGetSetKeyRepeatRate(BMessage*, BMessage*);
	status_t HandleGetSetKeyMap(BMessage*, BMessage*);
	status_t HandleFocusUnfocusIMAwareView(BMessage*, BMessage*);

	status_t HandleFindDevices(BMessage*, BMessage*);
	status_t HandleWatchDevices(BMessage*, BMessage*);
	status_t HandleIsDeviceRunning(BMessage*, BMessage*);
	status_t HandleStartStopDevices(BMessage*, BMessage*);
	status_t HandleControlDevices(BMessage*, BMessage*);
	status_t HandleSystemShuttingDown(BMessage*, BMessage*);
	status_t HandleNodeMonitor(BMessage*);
	
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
	static status_t StartStopDevices(BInputServerDevice *isd, bool);
	status_t ControlDevices(const char *, input_device_type, unsigned long, BMessage*);

	bool DoMouseAcceleration(long*, long*);
	bool SetMousePos(long*, long*, long, long);
	bool SetMousePos(long*, long*, BPoint);
	bool SetMousePos(long*, long*, float, float);

	bool SafeMode(void);

	static BList   gInputDeviceList;
	static BLocker gInputDeviceListLocker;
	
	static BList   gInputFilterList;
	static BLocker gInputFilterListLocker;
	
	static BList   gInputMethodList;
	static BLocker gInputMethodListLocker;
	
	static DeviceManager	gDeviceManager;
	
private:
	/*void InitTestDevice();
	
	status_t AddInputServerDevice(const char* path);
	status_t AddInputServerFilter(const char* path);
	status_t AddInputServerMethod(const char* path);*/
	
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
	int32			sKeyboardLocks;
	uint16			sKeyboardID;
	mouse_map		sMouseMap;

	key_info		s_key_info;		// current key info
	key_map			s_key_map;		// current key_map
	char			*sChars;		// current keymap chars
	int32			sCharCount;		// current keymap char count
	
	port_id			ISPort;
	port_id      	EventLooperPort;
	thread_id    	ISPortThread;
	static int32 	ISPortWatcher(void *arg);
	void WatchPort();
	
	static bool doStartStopDevice(void*, void*);
	
	//static BList mInputServerDeviceList;
	//static BList mInputServerFilterList;
	//static BList mInputServerMethodList;
	
	// added this to communicate via portlink
	
	BPortLink 		*serverlink;
	AddOnManager 	*fAddOnManager;
	
	BList			fEventsCache;
	
	//fMouseState;
};


#endif
