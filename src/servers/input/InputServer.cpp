/*****************************************************************************/
// Haiku InputServer
//
// [Description]
//
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2002-2004 Haiku Project
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


#include <stdio.h>
#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Locker.h>
#include <Message.h>
#include <Path.h>
#include <String.h>

#include "InputServer.h"
#include "InputServerTypes.h"

// include app_server headers for communication
#include <PortLink.h>
#include <ServerProtocol.h>

#define X_VALUE "x"
#define Y_VALUE "y"


extern "C" void RegisterDevices(input_device_ref** devices)
{
	CALLED();
};


// Static InputServer member variables.
//
BList   InputServer::gInputDeviceList;
BLocker InputServer::gInputDeviceListLocker;

BList   InputServer::gInputFilterList;
BLocker InputServer::gInputFilterListLocker;

BList   InputServer::gInputMethodList;
BLocker InputServer::gInputMethodListLocker;

DeviceManager InputServer::gDeviceManager;

/*
 *
 */
int main()
{
	InputServer	*myInputServer = new InputServer;
	
	myInputServer->Run();
	
	delete myInputServer;
}


/*
 *  Method: InputServer::InputServer()
 *   Descr: 
 */
InputServer::InputServer(void) : BApplication(INPUTSERVER_SIGNATURE)
{
	CALLED();
	void *pointer=NULL;
	
	EventLoop(pointer);

	gDeviceManager.LoadState();
	
	fAddOnManager = new AddOnManager();
	fAddOnManager->LoadState();
}

/*
 *  Method: InputServer::InputServer()
 *   Descr: 
 */
InputServer::~InputServer(void)
{
	CALLED();
	delete fAddOnManager;
}


/*
 *  Method: InputServer::ArgvReceived()
 *   Descr: 
 */
void
InputServer::ArgvReceived(int32 argc, char** argv)
{
	CALLED();
	if (2 == argc) {
		if (0 == strcmp("-q", argv[1]) ) {
			// :TODO: Shutdown and restart the InputServer.
			printf("InputServer::ArgvReceived - Restarting ...\n");
			status_t   quit_status;
			//BMessenger msgr = BMessenger("application/x-vnd.OpenBeOS-input_server", -1, &quit_status);
			BMessenger msgr = BMessenger(INPUTSERVER_SIGNATURE, -1, &quit_status);
			if (B_OK == quit_status) {
				BMessage   msg  = BMessage(B_QUIT_REQUESTED);
				msgr.SendMessage(&msg);
			} else {
				printf("Unable to send Quit message to running InputServer.");
			}
		}
	}
}


/*
 *  Method: InputServer::InitKeyboardMouseStates()
 *   Descr: 
 */
void
InputServer::InitKeyboardMouseStates(void)
{
	CALLED();
	// This is where we read in the preferences data for the mouse and keyboard as well as
	// determine the screen resolution from the app_server and find the center of the screen
	// sMousePos is then set to the center of the screen.

	sMousePos.x = 200;
	sMousePos.y = 200;

}


/*
 *  Method: InputServer::QuitRequested()
 *   Descr: 
 */
bool
InputServer::QuitRequested(void)
{
	CALLED();
	if (!BApplication::QuitRequested())
		return false;
	
	fAddOnManager->SaveState();
	gDeviceManager.SaveState();
	
	kill_thread(ISPortThread);
	delete_port(EventLooperPort);
	EventLooperPort = -1;
	return true;
}

// ---------------------------------------------------------------
// InputServer::ReadyToRun(void)
//
// Verifies to see if the input_server is able to start.
//
//
// Parameters:
//		None
//
// Returns:
//		B_OK if the
// ---------------------------------------------------------------
void InputServer::ReadyToRun(void)
{
	CALLED();
}


/*
 *  Method: InputServer::MessageReceived()
 *   Descr: 
 */
void
InputServer::MessageReceived(BMessage *message)
{
	CALLED();
	
	BMessage reply;
	status_t status;
	
	switch(message->what)
	{
		case IS_SET_METHOD:
			HandleSetMethod(message);
			break;
		case IS_GET_MOUSE_TYPE: 
			status = HandleGetSetMouseType(message, &reply);
			break;	
		case IS_SET_MOUSE_TYPE:
			status = HandleGetSetMouseType(message, &reply);
			break;
		case IS_GET_MOUSE_ACCELERATION:
			status = HandleGetSetMouseAcceleration(message, &reply);
			break;
		case IS_SET_MOUSE_ACCELERATION:
			status = HandleGetSetMouseAcceleration(message, &reply);
			break;
		case IS_GET_KEY_REPEAT_DELAY:
			status = HandleGetSetKeyRepeatDelay(message, &reply);
			break;
		case IS_SET_KEY_REPEAT_DELAY:
			status = HandleGetSetKeyRepeatDelay(message, &reply);
			break;
		case IS_GET_KEY_INFO:
			status = HandleGetKeyInfo(message, &reply);
			break;
		case IS_GET_MODIFIERS:
			status = HandleGetModifiers(message, &reply);
			break;
		case IS_SET_MODIFIER_KEY:
			status = HandleSetModifierKey(message, &reply);
			break;
		case IS_SET_KEYBOARD_LOCKS:
			status = HandleSetKeyboardLocks(message, &reply);
			break;
		case IS_GET_MOUSE_SPEED:
			status = HandleGetSetMouseSpeed(message, &reply);
			break;
		case IS_SET_MOUSE_SPEED:
			status = HandleGetSetMouseSpeed(message, &reply);
			break;
		case IS_SET_MOUSE_POSITION:
			status = HandleSetMousePosition(message, &reply);
			break;
		case IS_GET_MOUSE_MAP:
			status = HandleGetSetMouseMap(message, &reply);
			break;
		case IS_SET_MOUSE_MAP:
			status = HandleGetSetMouseMap(message, &reply);
			break;
		case IS_GET_KEYBOARD_ID:
			status = HandleGetKeyboardID(message, &reply);
			break;
		case IS_GET_CLICK_SPEED:
			status = HandleGetSetClickSpeed(message, &reply);
			break;
		case IS_SET_CLICK_SPEED:
			status = HandleGetSetClickSpeed(message, &reply);
			break;
		case IS_GET_KEY_REPEAT_RATE:
			status = HandleGetSetKeyRepeatRate(message, &reply);
			break;
		case IS_SET_KEY_REPEAT_RATE:
			status = HandleGetSetKeyRepeatRate(message, &reply);
			break;
		case IS_GET_KEY_MAP:
			status = HandleGetSetKeyMap(message, &reply);
			break;
		case IS_RESTORE_KEY_MAP:
			status = HandleGetSetKeyMap(message, &reply);
			break;
		case IS_FOCUS_IM_AWARE_VIEW:
			status = HandleFocusUnfocusIMAwareView(message, &reply);
			break;
		case IS_UNFOCUS_IM_AWARE_VIEW:
			status = HandleFocusUnfocusIMAwareView(message, &reply);
			break;

		// device looper related
		case IS_FIND_DEVICES:
			status = HandleFindDevices(message, &reply);
			break;
		case IS_WATCH_DEVICES:
			status = HandleWatchDevices(message, &reply);
			break;
		case IS_IS_DEVICE_RUNNING:
			status = HandleIsDeviceRunning(message, &reply);
			break;
		case IS_START_DEVICE:
			status = HandleStartStopDevices(message, &reply);
			break;
		case IS_STOP_DEVICE:
			status = HandleStartStopDevices(message, &reply);
			break;
		case IS_CONTROL_DEVICES:
			status = HandleControlDevices(message, &reply);
			break;
		case SYSTEM_SHUTTING_DOWN:
			status = HandleSystemShuttingDown(message, &reply);
			break;
		default:
		{
			PRINT(("Default message ... \n"));
			PRINT_OBJECT(*message);
			BMessenger app_server("application/x-vnd.Be-APPS", -1, NULL);
			if (app_server.IsValid()) {
				//app_server->SendMessage(message);
			}
			return;		
		}
	}
	
	reply.AddInt32("status", status);
	message->SendReply(&reply);
}


/*
 *  Method: InputServer::HandleSetMethod()
 *   Descr: 
 */
void
InputServer::HandleSetMethod(BMessage *)
{
}


/*
 *  Method: InputServer::HandleGetSetMouseType()
 *   Descr: 
 */
status_t
InputServer::HandleGetSetMouseType(BMessage* message,
                                     BMessage* reply)
{
	status_t status;
	int32	type;
	if (message->FindInt32("mouse_type", &type)==B_OK) {
		fMouseSettings.SetMouseType(type);
		status = ControlDevices(NULL, B_POINTING_DEVICE, B_MOUSE_TYPE_CHANGED, NULL);
	} else
		status = reply->AddInt32("mouse_type", 	fMouseSettings.MouseType());
	return status;
}


/*
 *  Method: InputServer::HandleGetSetMouseAcceleration()
 *   Descr: 
 */
status_t
InputServer::HandleGetSetMouseAcceleration(BMessage* message,
                                             BMessage* reply)
{
	status_t status;
	int32 factor;
	if (message->FindInt32("speed", &factor) == B_OK) {
		fMouseSettings.SetAccelerationFactor(factor);
		status = ControlDevices(NULL, B_POINTING_DEVICE, B_MOUSE_ACCELERATION_CHANGED, NULL);
	} else
		status = reply->AddInt32("speed", fMouseSettings.AccelerationFactor());
	return status;
}


/*
 *  Method: InputServer::HandleGetSetKeyRepeatDelay()
 *   Descr: 
 */
status_t
InputServer::HandleGetSetKeyRepeatDelay(BMessage* message,
                                          BMessage* reply)
{
	status_t status;
	bigtime_t delay;
	if (message->FindInt64("delay", &delay) == B_OK) {
		fKeyboardSettings.SetKeyboardRepeatDelay(delay);
		status = ControlDevices(NULL, B_KEYBOARD_DEVICE, B_KEY_REPEAT_DELAY_CHANGED, NULL);
	} else
		status = reply->AddInt64("delay", fKeyboardSettings.KeyboardRepeatDelay());
	return status;
}


/*
 *  Method: InputServer::HandleGetKeyInfo()
 *   Descr: 
 */
status_t
InputServer::HandleGetKeyInfo(BMessage *message,
                              BMessage *reply)
{
	return reply->AddData("key_info", B_ANY_TYPE, &s_key_info, sizeof(s_key_info));
}


/*
 *  Method: InputServer::HandleGetModifiers()
 *   Descr: 
 */
status_t
InputServer::HandleGetModifiers(BMessage *message,
                                BMessage *reply)
{
	return reply->AddInt32("modifiers", s_key_info.modifiers);
}


/*
 *  Method: InputServer::HandleSetModifierKey()
 *   Descr: 
 */
status_t
InputServer::HandleSetModifierKey(BMessage *message,
                                  BMessage *reply)
{
	status_t status = B_ERROR;
	int32 modifier, key;
	if (message->FindInt32("modifier", &modifier) == B_OK
		&& message->FindInt32("key", &key) == B_OK) {
	
		// TODO : impact the keymap
	
		// is SetModifierKey a keymap change ?
		status = ControlDevices(NULL, B_KEYBOARD_DEVICE, B_KEY_MAP_CHANGED, NULL);
	}
	return status;
}


/*
 *  Method: InputServer::HandleSetKeyboardLocks()
 *   Descr: 
 */
status_t
InputServer::HandleSetKeyboardLocks(BMessage *message,
                                    BMessage *reply)
{
	status_t status = B_ERROR;
	if (message->FindInt32("locks", &sKeyboardLocks) == B_OK)
		status = ControlDevices(NULL, B_KEYBOARD_DEVICE, B_KEY_LOCKS_CHANGED, NULL);
	
	return status;
}


/*
 *  Method: InputServer::HandleGetSetMouseSpeed()
 *   Descr: 
 */
status_t
InputServer::HandleGetSetMouseSpeed(BMessage* message,
                                      BMessage* reply)
{
	status_t status;
	int32 speed;
	if (message->FindInt32("speed", &speed) == B_OK) {
		fMouseSettings.SetMouseSpeed(speed);
		status = ControlDevices(NULL, B_POINTING_DEVICE, B_MOUSE_SPEED_CHANGED, NULL);
	} else
		status = reply->AddInt32("speed", fMouseSettings.MouseSpeed());
	return status;
}


/*
 *  Method: InputServer::HandleSetMousePosition()
 *   Descr: 
 */
status_t
InputServer::HandleSetMousePosition(BMessage *message, BMessage *outbound)
{
	
	// this assumes that both supplied pointers are identical
	
	ASSERT(outbound == message);
		
	sMousePos.x = 200;
	sMousePos.y = 200;

	int32 xValue, 
		  yValue;
    
    message->FindInt32("x",xValue);
    PRINT(("[HandleSetMousePosition] x = %lu:\n",xValue));
	
   	switch(message->what){
   		case B_MOUSE_MOVED:{
    		// get point and button from msg
    		if((outbound->FindInt32(X_VALUE,&xValue) == B_OK) && (outbound->FindInt32(Y_VALUE,&yValue) == B_OK)){
				sMousePos.x += xValue;
				sMousePos.y += yValue;
				outbound->ReplaceInt32(X_VALUE,sMousePos.x); 
				outbound->ReplaceInt32(Y_VALUE,sMousePos.y);
	   			}
    		break;
    		}
   				// Should be some Mouse Down and Up code here ..
   				// Along with some Key Down and up codes ..
   		default:
      		break;
   			
		}
	
	return B_OK;
}


/*
 *  Method: InputServer::HandleGetMouseMap()
 *   Descr: 
 */
status_t
InputServer::HandleGetSetMouseMap(BMessage* message,
                                    BMessage* reply)
{
	status_t status;
	mouse_map *map;
	ssize_t    size;
	
	if (message->FindData("mousemap", B_RAW_TYPE, (const void**)&map, &size) == B_OK) {
		fMouseSettings.SetMapping(*map);
		status = ControlDevices(NULL, B_POINTING_DEVICE, B_MOUSE_MAP_CHANGED, NULL);
	} else {
		mouse_map 	map;
		fMouseSettings.Mapping(map);
		status = reply->AddData("mousemap", B_RAW_TYPE, &map, sizeof(mouse_map) );
	} 
	return status;
}


/*
 *  Method: InputServer::HandleGetKeyboardID()
 *   Descr: 
 */
status_t
InputServer::HandleGetKeyboardID(BMessage *message,
                                      BMessage *reply)
{
	return reply->AddInt16("id", sKeyboardID);
}


/*
 *  Method: InputServer::HandleGetSetClickSpeed()
 *   Descr: 
 */
status_t
InputServer::HandleGetSetClickSpeed(BMessage *message,
                                      BMessage *reply)
{
	status_t status = B_ERROR;
	bigtime_t	click_speed;
	if (message->FindInt64("speed", &click_speed) == B_OK) {
		fMouseSettings.SetClickSpeed(click_speed);
		status = ControlDevices(NULL, B_POINTING_DEVICE, B_CLICK_SPEED_CHANGED, NULL);
	} else
		status = reply->AddInt64("speed", fMouseSettings.ClickSpeed());
	return status;
}


/*
 *  Method: InputServer::HandleGetSetKeyRepeatRate()
 *   Descr: 
 */
status_t
InputServer::HandleGetSetKeyRepeatRate(BMessage* message,
                                         BMessage* reply)
{
	status_t status;
	int32	key_repeat_rate;
	if (message->FindInt32("rate", &key_repeat_rate) == B_OK) {
		fKeyboardSettings.SetKeyboardRepeatRate(key_repeat_rate);
		status = ControlDevices(NULL, B_KEYBOARD_DEVICE, B_KEY_REPEAT_RATE_CHANGED, NULL);
	} else
		status = reply->AddInt32("rate", fKeyboardSettings.KeyboardRepeatRate());
	return status;
}


/*
 *  Method: InputServer::HandleGetSetKeyMap()
 *   Descr: 
 */
status_t
InputServer::HandleGetSetKeyMap(BMessage *message,
                                     BMessage *reply)
{
	status_t status;
	if (message->what == IS_GET_KEY_MAP) {
		status = reply->AddData("keymap", B_ANY_TYPE, &s_key_map, sizeof(s_key_map));
		if (status == B_OK)
			status = reply->AddData("key_buffer", B_ANY_TYPE, sChars, sCharCount);
	} else {
		// TODO : reload keymap
		
		status = ControlDevices(NULL, B_KEYBOARD_DEVICE, B_KEY_MAP_CHANGED, NULL);
	}
	return status;
}


/*
 *  Method: InputServer::HandleFocusUnfocusIMAwareView()
 *   Descr: 
 */
status_t
InputServer::HandleFocusUnfocusIMAwareView(BMessage *,
                                           BMessage *)
{
}


/*
 *  Method: InputServer::HandleFindDevices()
 *   Descr: 
 */
status_t
InputServer::HandleFindDevices(BMessage *message,
                                     BMessage *reply)
{
	
}


/*
 *  Method: InputServer::HandleWatchDevices()
 *   Descr: 
 */
status_t
InputServer::HandleWatchDevices(BMessage *message,
                                     BMessage *reply)
{
}


/*
 *  Method: InputServer::HandleIsDeviceRunning()
 *   Descr: 
 */
status_t
InputServer::HandleIsDeviceRunning(BMessage *message,
                                     BMessage *reply)
{
	
}


/*
 *  Method: InputServer::HandleStartStopDevices()
 *   Descr: 
 */
status_t
InputServer::HandleStartStopDevices(BMessage *message,
                                     BMessage *reply)
{
}


/*
 *  Method: InputServer::HandleControlDevices()
 *   Descr: 
 */
status_t
InputServer::HandleControlDevices(BMessage *message,
                                     BMessage *reply)
{
}


/*
 *  Method: InputServer::HandleSystemShuttingDown()
 *   Descr: 
 */
status_t
InputServer::HandleSystemShuttingDown(BMessage *message,
                                     BMessage *reply)
{
	status_t status;
	return status;
}


/*
 *  Method: InputServer::EnqueueDeviceMessage()
 *   Descr: 
 */
status_t 
InputServer::EnqueueDeviceMessage(BMessage *message)
{
	CALLED();
	
	status_t  	err;
	
	ssize_t length = message->FlattenedSize();
	char buffer[length];
	if ((err = message->Flatten(buffer,length)) < B_OK)
		return err;
	return write_port(EventLooperPort, 0, buffer, length);
}


/*
 *  Method: InputServer::EnqueueMethodMessage()
 *   Descr: 
 */
status_t
InputServer::EnqueueMethodMessage(BMessage *message)
{
	CALLED();
	
	status_t  	err;
	
	ssize_t length = message->FlattenedSize();
	char buffer[length];
	if ((err = message->Flatten(buffer,length)) < B_OK)
		return err;
	return write_port(EventLooperPort, 0, buffer, length);
}


/*
 *  Method: InputServer::UnlockMethodQueue()
 *   Descr: 
 */
status_t 
InputServer::UnlockMethodQueue(void)
{
	return 0;
}


/*
 *  Method: InputServer::LockMethodQueue()
 *   Descr: 
 */
status_t 
InputServer::LockMethodQueue(void)
{
	return 0;
}


/*
 *  Method: InputServer::SetNextMethod()
 *   Descr: 
 */
status_t
InputServer::SetNextMethod(bool)
{
	return 0;
}


/*
 *  Method: InputServer::SetActiveMethod()
 *   Descr: 
 */
/* 
InputServer::SetActiveMethod(_BMethodAddOn_ *)
{
	return 0;
}
*/


/*
 *  Method: InputServer::MethodReplicant()
 *   Descr: 
 */
const BMessenger* 
InputServer::MethodReplicant(void)
{
	return NULL;
}


/*
 *  Method: InputServer::EventLoop()
 *   Descr: 
 */
status_t
InputServer::EventLoop(void *)
{
	CALLED();
	EventLooperPort = create_port(100, "obos_is_event_port");
	if(EventLooperPort < 0) {
		_sPrintf("OBOS InputServer: create_port error: (0x%x) %s\n",EventLooperPort,strerror(EventLooperPort));
	} 
	ISPortThread = spawn_thread(ISPortWatcher, "_input_server_event_loop_", B_REAL_TIME_DISPLAY_PRIORITY+3, this);
	resume_thread(ISPortThread);

	return 0;
}


/*
 *  Method: InputServer::EventLoopRunning()
 *   Descr: 
 */
bool 
InputServer::EventLoopRunning(void)
{
	return true;
}


/*
 *  Method: InputServer::DispatchEvents()
 *   Descr: 
 */
bool
InputServer::DispatchEvents(BList *eventList)
{
	CALLED();
	
	CacheEvents(eventList);

	if (fEventsCache.CountItems()>50) {

		BMessage *event;
		
		for ( int32 i = 0; NULL != (event = (BMessage *)fEventsCache.ItemAt(i)); i++ ) {
			// now we must send each event to the app_server
			DispatchEvent(event);
			
			delete event;
		}
		
		fEventsCache.MakeEmpty();
		
	}
	return true;
}// end DispatchEvents()


int 
InputServer::DispatchEvent(BMessage *message)
{
	CALLED();
	
	// variables
	int32 xValue, 
		  yValue;
    uint32 buttons = 0;
    
    message->FindInt32("x",xValue);
    PRINT(("[DispatchEvent] x = %lu:\n", xValue));
	
	port_id pid = find_port(SERVER_INPUT_PORT);

	// BPortLink is incompatible with R5 one
#ifndef COMPILE_FOR_R5

	BPortLink *appsvrlink = new BPortLink(pid);
   	switch(message->what){
   		case B_MOUSE_MOVED:{
    		// get point and button from msg
    		if((message->FindInt32(X_VALUE,&xValue) == B_OK) && (message->FindInt32(Y_VALUE,&yValue) == B_OK)){
    			int64 time=(int64)real_time_clock();
    			appsvrlink->StartMessage(B_MOUSE_MOVED);
    			appsvrlink->Attach(&time,sizeof(int64));
    			appsvrlink->Attach((float)xValue);
    			appsvrlink->Attach((float)yValue);
    			message->FindInt32("buttons",buttons);
    			appsvrlink->Attach(&buttons,sizeof(int32));
    			appsvrlink->Flush();
    			PRINT(("B_MOUSE_MOVED: x = %lu: y = %lu: time = %llu: buttons = %lu\n",xValue,yValue,time,buttons));
    			}
    		break;
    		}
    	case B_MOUSE_DOWN:{

			BPoint pt;
			int32 buttons,clicks,mod;
			int64 time=(int64)real_time_clock();
			
			if(message->FindPoint("where",&pt)!=B_OK ||
					message->FindInt32("modifiers",&mod)!=B_OK ||
					message->FindInt32("buttons",&buttons)!=B_OK ||
					message->FindInt32("clicks",&clicks)!=B_OK)
				break;
		
			appsvrlink->StartMessage(B_MOUSE_DOWN);
			appsvrlink->Attach(&time, sizeof(int64));
			appsvrlink->Attach(&pt.x,sizeof(float));
			appsvrlink->Attach(&pt.y,sizeof(float));
			appsvrlink->Attach(&mod, sizeof(uint32));
			appsvrlink->Attach(&buttons, sizeof(uint32));
			appsvrlink->Attach(&clicks, sizeof(uint32));
			appsvrlink->Flush();
    		break;
    		}
    	case B_MOUSE_UP:{
			BPoint pt;
			int32 mod;
			int64 time=(int64)real_time_clock();

			if(message->FindPoint("where",&pt)!=B_OK ||
					message->FindInt32("modifiers",&mod)!=B_OK)
				break;
			
			appsvrlink->StartMessage(B_MOUSE_UP);
			appsvrlink->Attach(&time, sizeof(int64));
			appsvrlink->Attach(&pt.x,sizeof(float));
			appsvrlink->Attach(&pt.y,sizeof(float));
			appsvrlink->Attach(&mod, sizeof(uint32));
			appsvrlink->Flush();
    		break;
    		}
    	case B_MOUSE_WHEEL_CHANGED:{
			float x,y;
			message->FindFloat("be:wheel_delta_x",&x);
			message->FindFloat("be:wheel_delta_y",&y);
			int64 time=real_time_clock();
			
			appsvrlink->StartMessage(B_MOUSE_WHEEL_CHANGED);
			appsvrlink->Attach(&time,sizeof(int64));
			appsvrlink->Attach(x);
			appsvrlink->Attach(y);
			appsvrlink->Flush();
			break;
    		}
		case B_KEY_DOWN:{
			bigtime_t systime;
			int32 scancode, asciicode,repeatcount,modifiers;
			int8 utf8data[3];
			BString string;
			int8 keyarray[16];

			systime=(int64)real_time_clock();
			message->FindInt32("key",&scancode);
			message->FindInt32("be:key_repeat",&repeatcount);
			message->FindInt32("modifiers",&modifiers);
			message->FindInt32("raw_char",&asciicode);
			message->FindInt8("byte",0,utf8data);
			message->FindInt8("byte",1,utf8data+1);
			message->FindInt8("byte",2,utf8data+2);
			message->FindString("bytes",&string);
			for(int8 i=0;i<15;i++)
				message->FindInt8("states",i,&keyarray[i]);
			appsvrlink->StartMessage(B_KEY_DOWN);
			appsvrlink->Attach(&systime,sizeof(bigtime_t));
			appsvrlink->Attach(scancode);
			appsvrlink->Attach(asciicode);
			appsvrlink->Attach(repeatcount);
			appsvrlink->Attach(modifiers);
			appsvrlink->Attach(utf8data,sizeof(int8)*3);
			appsvrlink->Attach(string.Length()+1);
			appsvrlink->Attach(string.String());
			appsvrlink->Attach(keyarray,sizeof(int8)*16);
			appsvrlink->Flush();
			break;
		}
		case B_KEY_UP:{
			bigtime_t systime;
			int32 scancode, asciicode,modifiers;
			int8 utf8data[3];
			BString string;
			int8 keyarray[16];

			systime=(int64)real_time_clock();
			message->FindInt32("key",&scancode);
			message->FindInt32("raw_char",&asciicode);
			message->FindInt32("modifiers",&modifiers);
			message->FindInt8("byte",0,utf8data);
			message->FindInt8("byte",1,utf8data+1);
			message->FindInt8("byte",2,utf8data+2);
			message->FindString("bytes",&string);
			for(int8 i=0;i<15;i++)
				message->FindInt8("states",i,&keyarray[i]);
			appsvrlink->StartMessage(B_KEY_UP);
			appsvrlink->Attach(&systime,sizeof(bigtime_t));
			appsvrlink->Attach(scancode);
			appsvrlink->Attach(asciicode);
			appsvrlink->Attach(modifiers);
			appsvrlink->Attach(utf8data,sizeof(int8)*3);
			appsvrlink->Attach(string.Length()+1);
			appsvrlink->Attach(string.String());
			appsvrlink->Attach(keyarray,sizeof(int8)*16);
			appsvrlink->Flush();
			break;
		}
		case B_UNMAPPED_KEY_DOWN:{
			bigtime_t systime;
			int32 scancode,modifiers;
			int8 keyarray[16];

			systime=(int64)real_time_clock();
			message->FindInt32("key",&scancode);
			message->FindInt32("modifiers",&modifiers);
			for(int8 i=0;i<15;i++)
				message->FindInt8("states",i,&keyarray[i]);
			appsvrlink->StartMessage(B_UNMAPPED_KEY_DOWN);
			appsvrlink->Attach(&systime,sizeof(bigtime_t));
			appsvrlink->Attach(scancode);
			appsvrlink->Attach(modifiers);
			appsvrlink->Attach(keyarray,sizeof(int8)*16);
			appsvrlink->Flush();
			break;
		}
		case B_UNMAPPED_KEY_UP:{
			bigtime_t systime;
			int32 scancode,modifiers;
			int8 keyarray[16];

			systime=(int64)real_time_clock();
			message->FindInt32("key",&scancode);
			message->FindInt32("modifiers",&modifiers);
			for(int8 i=0;i<15;i++)
				message->FindInt8("states",i,&keyarray[i]);
			appsvrlink->StartMessage(B_UNMAPPED_KEY_UP);
			appsvrlink->Attach(&systime,sizeof(bigtime_t));
			appsvrlink->Attach(scancode);
			appsvrlink->Attach(modifiers);
			appsvrlink->Attach(keyarray,sizeof(int8)*16);
			appsvrlink->Flush();
			break;
		}
		case B_MODIFIERS_CHANGED:{
			bigtime_t systime;
			int32 scancode,modifiers,oldmodifiers;
			int8 keyarray[16];

			systime=(int64)real_time_clock();
			message->FindInt32("key",&scancode);
			message->FindInt32("modifiers",&modifiers);
			message->FindInt32("be:old_modifiers",&oldmodifiers);
			for(int8 i=0;i<15;i++)
				message->FindInt8("states",i,&keyarray[i]);
			appsvrlink->StartMessage(B_MODIFIERS_CHANGED);
			appsvrlink->Attach(&systime,sizeof(bigtime_t));
			appsvrlink->Attach(scancode);
			appsvrlink->Attach(modifiers);
			appsvrlink->Attach(oldmodifiers);
			appsvrlink->Attach(keyarray,sizeof(int8)*16);
			appsvrlink->Flush();
			break;
		}
   		default:
      		break;
   			
		}
	delete appsvrlink;
	
#endif	// COMPILE_FOR_R5

    return true;
}

/*
 *  Method: InputServer::CacheEvents()
 *   Descr: 
 */
bool 
InputServer::CacheEvents(BList *eventsToCache)
{
	CALLED();
	
	FilterEvents(eventsToCache);

	fEventsCache.AddList(eventsToCache);
	eventsToCache->MakeEmpty();

	return true;
}


/*
 *  Method: InputServer::GetNextEvents()
 *   Descr: 
 */
const BList* 
InputServer::GetNextEvents(BList *)
{
	return NULL;
}


/*
 *  Method: InputServer::FilterEvents()
 *  Descr:  This method applies all defined filters to each event in the
 *          supplied list.  The supplied list is modified to reflect the
 *          output of the filters.
 *          The method returns true if the filters were applied to all
 *          events without error and false otherwise.
 */
bool
InputServer::FilterEvents(BList *eventsToFilter)
{
	CALLED();
	
	if (NULL == eventsToFilter)
		return false;
	
	BInputServerFilter* current_filter;
	BMessage*           current_event;
	int32               filter_index  = 0;
	int32               event_index   = 0;

	while (NULL != (current_filter = (BInputServerFilter*)gInputFilterList.ItemAt(filter_index) ) ) {
		// Apply the current filter to all available event messages.
		//		
		while (NULL != (current_event = (BMessage*)eventsToFilter->ItemAt(event_index) ) ) {
			// Storage for new event messages generated by the filter.
			//
			BList out_list;
			
			// Apply the current filter to the current event message.
			//
			PRINT(("InputServer::FilterEvents Filter called\n"));
			filter_result result = current_filter->Filter(current_event, &out_list);
			if (B_DISPATCH_MESSAGE == result) {
				// Use the result in current_message; ignore out_list.
				//
				event_index++;

				// Free resources associated with items in out_list.
				//
				void *out_item; 
				for (int32 i = 0; NULL != (out_item = out_list.ItemAt(i) ); i++)
					delete out_item;
			} else if (B_SKIP_MESSAGE == result) {
				// Use the result in out_list (if any); ignore current message.
				//
				eventsToFilter->RemoveItem(event_index);
				eventsToFilter->AddList(&out_list, event_index);
				event_index += out_list.CountItems();
				
				// NOTE: eventsToFilter now owns out_list's items.
			} else {
				// Error - Free resources associated with items in out_list and return.
				//
				 void* out_item;
				for (int32 i = 0; NULL != (out_item = out_list.ItemAt(i) ); i++)
					delete out_item;
				return false;
			}
			
			// NOTE: The BList destructor frees out_lists's resources here.
			//       It does NOT free the resources associated with out_list's
			//       member items - those should either already be deleted or
			//       should be owned by eventsToFilter.
		} // while()
	
		filter_index++;
	
	} // while()
	
	return true;	
}


/*
 *  Method: InputServer::SanitizeEvents()
 *   Descr: 
 */
bool 
InputServer::SanitizeEvents(BList *)
{
	return true;
}


/*
 *  Method: InputServer::MethodizeEvents()
 *   Descr: 
 */
bool 
InputServer::MethodizeEvents(BList *,
                             bool)
{
	CALLED();
	
	return true;
}


/*
 *  Method: InputServer::StartStopDevices()
 *   Descr: 
 */
status_t 
InputServer::StartStopDevices(const char*       deviceName,
                                       input_device_type deviceType,
                                       bool              doStart)
{
	CALLED();
	for (int i = gInputDeviceList.CountItems() - 1; i >= 0; i--)
	{
		PRINT(("Device #%d\n", i));
		InputDeviceListItem* item = (InputDeviceListItem*)gInputDeviceList.ItemAt(i);
		if (NULL == item)
			continue;
			
		BInputServerDevice* isd = item->mIsd;
		input_device_ref   dev = item->mDev;
		
		PRINT(("Hey\n"));
		
		if (NULL != isd)
			continue;
			
		if (deviceType == dev.type) {
			if (doStart) {
				PRINT(("  Starting: %s\n", dev.name));
				isd->Start(dev.name, dev.cookie);
				item->mStarted = true;
			} else {
				PRINT(("  Stopping: %s\n", dev.name));
				isd->Stop(dev.name, dev.cookie);
				item->mStarted = false;
			}
		}
	}
	EXIT();
	
	return B_OK;
}


/*
 *  Method: InputServer::StartStopDevices()
 *   Descr: 
 */
status_t 
InputServer::StartStopDevices(BInputServerDevice *isd,
                                       bool              doStart)
{
	CALLED();
	for (int i = gInputDeviceList.CountItems() - 1; i >= 0; i--)
	{
		PRINT(("%s Device #%d\n", __PRETTY_FUNCTION__, i));
		InputDeviceListItem* item = (InputDeviceListItem*)gInputDeviceList.ItemAt(i);
		if (NULL != item && isd == item->mIsd)
		{
			input_device_ref   dev = item->mDev;
			
			if (doStart) {
				PRINT(("  Starting: %s\n", dev.name));
				isd->Start(dev.name, dev.cookie);
				item->mStarted = true;
			} else {
				PRINT(("  Stopping: %s\n", dev.name));
				isd->Stop(dev.name, dev.cookie);
				item->mStarted = false;
			}
		}
	}
	EXIT();
	
	return B_OK;
}


/*
 *  Method: InputServer::ControlDevices()
 *   Descr: 
 */
status_t 
InputServer::ControlDevices(const char* deviceName,
                            input_device_type    deviceType,
                            unsigned long        command,
                            BMessage*            message)
{
	status_t status = B_OK;
	
	for (int i = gInputDeviceList.CountItems() - 1; i >= 0; i--)
	{
		PRINT(("%s Device #%d\n", __PRETTY_FUNCTION__, i));
		InputDeviceListItem* item = (InputDeviceListItem*)gInputDeviceList.ItemAt(i);
		if (NULL != item)
		{
			BInputServerDevice* isd = item->mIsd;
			input_device_ref   dev = item->mDev;
			if ( (NULL != isd) )
			{
				PRINT(("  Controlling: %s\n", dev.name));
				if (deviceType == dev.type)
				{
					// :TODO: Descriminate based on Device Name also.
					
					// :TODO: Pass non-NULL Device Name and Cookie.
					
					status = isd->Control(NULL /*Name*/, NULL /*Cookie*/, command, message);
				}
			}
		}
	}

	return status;
}


/*
 *  Method: InputServer::DoMouseAcceleration()
 *   Descr: 
 */
bool 
InputServer::DoMouseAcceleration(long *,
                                 long *)
{
	return true;
}


/*
 *  Method: InputServer::SetMousePos()
 *   Descr: 
 */
bool 
InputServer::SetMousePos(long *,
                         long *,
                         long,
                         long)
{
	return true;
}


/*
 *  Method: InputServer::SetMousePos()
 *   Descr: 
 */
bool 
InputServer::SetMousePos(long *,
                         long *,
                         BPoint)
{
	return true;
}


/*
 *  Method: InputServer::SetMousePos()
 *   Descr: 
 */
bool 
InputServer::SetMousePos(long *,
                         long *,
                         float,
                         float)
{
	return true;
}


/*
 *  Method: InputServer::SafeMode()
 *   Descr: 
 */
bool
InputServer::SafeMode(void)
{
	return true;
}


int32 
InputServer::ISPortWatcher(void *arg)
{
	InputServer *self = (InputServer*)arg;
	self->WatchPort();
	return B_NO_ERROR;
}


void 
InputServer::WatchPort()
{
	
	while (true) { 
		// Block until we find the size of the next message
		ssize_t    	length = port_buffer_size(EventLooperPort);
		PRINT(("[Event Looper] BMessage Size = %lu\n", length));
		
		int32     	code;
		char buffer[length];
		
		status_t err = read_port(EventLooperPort, &code, buffer, length);
		if(err != length) {
			if(err >= 0) {
				printf("InputServer: failed to read full packet (read %lu of %lu)\n", err, length);
			} else {
				printf("InputServer: read_port error: (0x%lx) %s\n", err, strerror(err));
			}
			continue;
		}
		
		BMessage *event = new BMessage;
	
		if ((err = event->Unflatten(buffer)) < 0) {
			printf("[InputServer] Unflatten() error: (0x%lx) %s\n", err, strerror(err));
			delete event;
		} else {
			// This is where the message should be processed.	
			//PRINT_OBJECT(*event);

			HandleSetMousePosition(event, event);
						
			//DispatchEvent(&event);
			BList list;
			list.AddItem(event);
			DispatchEvents(&list);
			
			PRINT(("Event written to port\n"));
		}
	}

}

