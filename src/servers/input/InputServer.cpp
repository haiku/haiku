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
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Locker.h>
#include <Message.h>
#include <Path.h>
#include <String.h>
#include <OS.h>

#include "InputServer.h"
#include "InputServerTypes.h"

// include app_server headers for communication
#include <PortLink.h>
#include <ServerProtocol.h>

#define FAST_MOUSE_MOVED '_FMM'		// received from app_server when screen res changed, but could be sent to too. 


extern "C" void RegisterDevices(input_device_ref** devices)
{
	CALLED();
};

// i don't know the exact signature but this one works
extern "C" status_t _kget_safemode_option_(char* name, uint8 *p1, uint32 *p2);


// Static InputServer member variables.
//
BList   InputServer::gInputDeviceList;
BLocker InputServer::gInputDeviceListLocker;

BList   InputServer::gInputFilterList;
BLocker InputServer::gInputFilterListLocker;

BList   InputServer::gInputMethodList;
BLocker InputServer::gInputMethodListLocker;

DeviceManager InputServer::gDeviceManager;

#if DEBUG == 2
FILE *InputServer::sLogFile = NULL;
#endif

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
InputServer::InputServer(void) : BApplication(INPUTSERVER_SIGNATURE),
	sSafeMode(false),
	fScreen(B_MAIN_SCREEN_ID)
{
#if DEBUG == 2
	if (sLogFile == NULL)
		sLogFile = fopen("/var/log/input_server.log", "a");
#endif

	CALLED();
	
	EventLoop();
	
	uint8 p1;
	uint32 p2 = 1;
		
	if (_kget_safemode_option_("safemode", &p1, &p2) == B_OK)
		sSafeMode = true;
	
	gDeviceManager.LoadState();
	
#ifdef COMPILE_FOR_R5

	if (has_data(find_thread(NULL))) {
		PRINT(("HasData == YES\n")); 
		int32 buffer[2];
		thread_id appThreadId;
		memset(buffer, 0, sizeof(buffer));
		int32 code = receive_data(&appThreadId, buffer, sizeof(buffer));
		PRINT(("tid : %ld, code :%ld\n", appThreadId, code));
		for (int32 i=0; i<2; i++) {
			PRINT(("data : %lx\n", buffer[i]));
		}
		fCursorSem = buffer[0];
		area_id appArea = buffer[1];
		
		fCloneArea = clone_area("isClone", (void**)&fAppBuffer, B_ANY_ADDRESS, B_READ_AREA|B_WRITE_AREA, appArea);
		if (fCloneArea < B_OK) {
			PRINTERR(("clone_area error : %s\n", strerror(fCloneArea)));
		}

		fAsPort = create_port(100, "is_as");

		PRINT(("is_as port :%ld\n", fAsPort));
		
		buffer[1] = fEventLooperPort;
		buffer[0] = fAsPort;
	
		status_t err;
		if ((err = send_data(appThreadId, 0, buffer, sizeof(buffer)))!=B_OK)
			PRINTERR(("error when send_data %s\n", strerror(err)));
	}

#endif

	InitKeyboardMouseStates();
	
	fAddOnManager = new AddOnManager(SafeMode());
	fAddOnManager->LoadState();	
}

/*
 *  Method: InputServer::InputServer()
 *   Descr: 
 */
InputServer::~InputServer(void)
{
	CALLED();
	fAddOnManager->Lock();
	fAddOnManager->Quit();
	
#ifdef COMPILE_FOR_R5
	delete_port(fAsPort);
	delete_area(fCloneArea);
#endif

#if DEBUG == 2
	fclose(sLogFile);
#endif
}


/*
 *  Method: InputServer::ArgvReceived()
 *   Descr: 
 */
void
InputServer::ArgvReceived(int32 argc, char** argv)
{
	CALLED();
	if (2 == argc && (0 == strcmp("-q", argv[1]))) {
		PRINT(("InputServer::ArgvReceived - Restarting ...\n"));
		status_t quit_status = B_OK;
		BMessenger msgr = BMessenger(INPUTSERVER_SIGNATURE, -1, &quit_status);
		if (B_OK == quit_status) {
			msgr.SendMessage(B_QUIT_REQUESTED);
		} else {
			PRINTERR(("Unable to send Quit message to running InputServer.\n"));
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
	// This is where we determine the screen resolution from the app_server and find the center of the screen
	// sMousePos is then set to the center of the screen.

	fFrame = fScreen.Frame();
	fMousePos = BPoint(fFrame.right/2, fFrame.bottom/2);
	
	if (LoadKeymap()!=B_OK)
		LoadSystemKeymap();
		
	BMessage *msg = new BMessage(B_MOUSE_MOVED);
	HandleSetMousePosition(msg, msg);
}


#include "SystemKeymap.cpp"

status_t
InputServer::LoadKeymap()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path)!=B_OK)
		return B_BAD_VALUE;
	
	path.Append("Key_map");

	entry_ref ref;
	get_ref_for_path(path.Path(), &ref);

	status_t err;
	
	BFile file(&ref, B_READ_ONLY);
	if ((err = file.InitCheck()) != B_OK)
		return err;
	
	if (file.Read(&fKeys, sizeof(fKeys)) < (ssize_t)sizeof(fKeys))
		return B_BAD_VALUE;
	
	for (uint32 i=0; i<sizeof(fKeys)/4; i++)
		((uint32*)&fKeys)[i] = B_BENDIAN_TO_HOST_INT32(((uint32*)&fKeys)[i]);
	
	if (file.Read(&fCharsSize, sizeof(uint32)) < (ssize_t)sizeof(uint32))
		return B_BAD_VALUE;
	
	fCharsSize = B_BENDIAN_TO_HOST_INT32(fCharsSize);
	if (!fChars)
		delete[] fChars;
	fChars = new char[fCharsSize];
	if (file.Read(fChars, fCharsSize) != fCharsSize)
		return B_BAD_VALUE;
	
	return B_OK;
}


status_t
InputServer::LoadSystemKeymap()
{
	if (!fChars)
		delete[] fChars;
	fKeys = sSystemKeymap;
	fCharsSize = sSystemKeyCharsSize;
	fChars = new char[fCharsSize];
	memcpy(fChars, sSystemKeyChars, fCharsSize);
	
	// we save this keymap to file
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path)!=B_OK)
		return B_BAD_VALUE;
	
	path.Append("Key_map");

	entry_ref ref;
	get_ref_for_path(path.Path(), &ref);
	
	status_t err;
	
	BFile file(&ref, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE );
	if ((err = file.InitCheck()) != B_OK) {
		PRINTERR(("error %s\n", strerror(err)));
		return err;
	}
	
	for (uint32 i=0; i<sizeof(fKeys)/4; i++)
		((uint32*)&fKeys)[i] = B_HOST_TO_BENDIAN_INT32(((uint32*)&fKeys)[i]);
		
	if ((err = file.Write(&fKeys, sizeof(fKeys))) < (ssize_t)sizeof(fKeys)) {
		return err;
	}
	
	for (uint32 i=0; i<sizeof(fKeys)/4; i++)
		((uint32*)&fKeys)[i] = B_BENDIAN_TO_HOST_INT32(((uint32*)&fKeys)[i]);
	
	fCharsSize = B_HOST_TO_BENDIAN_INT32(fCharsSize);
	
	if ((err = file.Write(&fCharsSize, sizeof(uint32))) < (ssize_t)sizeof(uint32)) {
		return B_BAD_VALUE;
	}
	
	fCharsSize = B_BENDIAN_TO_HOST_INT32(fCharsSize);
	
	if ((err = file.Write(fChars, fCharsSize)) < (ssize_t)fCharsSize)
		return err;
	
	return B_OK;
	
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
	
	kill_thread(fISPortThread);
	delete_port(fEventLooperPort);
	fEventLooperPort = -1;
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
	status_t status = B_OK;

	PRINT(("%s what:%c%c%c%c\n", __PRETTY_FUNCTION__, message->what>>24, message->what>>16, message->what>>8, message->what));
	
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
		case IS_WATCH_DEVICES:
		case IS_IS_DEVICE_RUNNING:
		case IS_START_DEVICE:
		case IS_STOP_DEVICE:
		case IS_CONTROL_DEVICES:
		case SYSTEM_SHUTTING_DOWN:
			fAddOnManager->PostMessage(message);
			return;
		default:
		{
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
		
		BMessage msg(IS_CONTROL_DEVICES);
		msg.AddInt32("type", B_POINTING_DEVICE);
		msg.AddInt32("code", B_MOUSE_TYPE_CHANGED);
		status = fAddOnManager->PostMessage(&msg);	
	
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
		
		BMessage msg(IS_CONTROL_DEVICES);
		msg.AddInt32("type", B_POINTING_DEVICE);
		msg.AddInt32("code", B_MOUSE_ACCELERATION_CHANGED);
		status = fAddOnManager->PostMessage(&msg);	
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
		
		BMessage msg(IS_CONTROL_DEVICES);
		msg.AddInt32("type", B_KEYBOARD_DEVICE);
		msg.AddInt32("code", B_KEY_REPEAT_DELAY_CHANGED);
		status = fAddOnManager->PostMessage(&msg);
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
	return reply->AddData("key_info", B_ANY_TYPE, &fKey_info, sizeof(fKey_info));
}


/*
 *  Method: InputServer::HandleGetModifiers()
 *   Descr: 
 */
status_t
InputServer::HandleGetModifiers(BMessage *message,
                                BMessage *reply)
{
	return reply->AddInt32("modifiers", fKey_info.modifiers);
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
	
		switch (modifier) {
			case B_CAPS_LOCK:
				fKeys.caps_key = key;
				break;
			case B_NUM_LOCK:
				fKeys.num_key = key;
				break;
			case B_SCROLL_LOCK:
				fKeys.num_key = key;
				break;
			case B_LEFT_SHIFT_KEY:
				fKeys.left_shift_key = key;
				break;
			case B_RIGHT_SHIFT_KEY:
				fKeys.right_shift_key = key;
				break;
			case B_LEFT_COMMAND_KEY:
				fKeys.left_command_key = key;
				break;
			case B_RIGHT_COMMAND_KEY:
				fKeys.right_command_key = key;
				break;
			case B_LEFT_CONTROL_KEY:
				fKeys.left_control_key = key;
				break;
			case B_RIGHT_CONTROL_KEY:
				fKeys.right_control_key = key;
				break;
			case B_LEFT_OPTION_KEY:
				fKeys.left_option_key = key;
				break;
			case B_RIGHT_OPTION_KEY:
				fKeys.right_option_key = key;
				break;
			case B_MENU_KEY:
				fKeys.menu_key = key;
				break;
			default:
				return B_ERROR;
		}
		
		//TODO : unmap the key ?
		
		BMessage msg(IS_CONTROL_DEVICES);
		msg.AddInt32("type", B_KEYBOARD_DEVICE);
		msg.AddInt32("code", B_KEY_MAP_CHANGED);
		status = fAddOnManager->PostMessage(&msg);
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
	if (message->FindInt32("locks", (int32*)&fKeys.lock_settings) == B_OK) {
		BMessage msg(IS_CONTROL_DEVICES);
		msg.AddInt32("type", B_KEYBOARD_DEVICE);
		msg.AddInt32("code", B_KEY_LOCKS_CHANGED);
		status = fAddOnManager->PostMessage(&msg);
	}
	
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
		
		BMessage msg(IS_CONTROL_DEVICES);
		msg.AddInt32("type", B_POINTING_DEVICE);
		msg.AddInt32("code", B_MOUSE_SPEED_CHANGED);
		status = fAddOnManager->PostMessage(&msg);
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
	CALLED();
	
	// this assumes that both supplied pointers are identical
	
	ASSERT(outbound == message);
	
	int32 xValue, yValue;	
	
   	switch(message->what){
   		case B_MOUSE_MOVED:
   		case B_MOUSE_DOWN:
   		case B_MOUSE_UP:
   		case FAST_MOUSE_MOVED:
   			// get point and button from msg
    		if((outbound->FindInt32("x", &xValue) == B_OK) 
    			&& (outbound->FindInt32("y", &yValue) == B_OK)) {
				fMousePos.x += xValue;
				fMousePos.y -= yValue;
				fMousePos.ConstrainTo(fFrame);
				outbound->RemoveName("x"); 
				outbound->RemoveName("y");
				outbound->AddPoint("where", fMousePos);
				outbound->AddInt32("modifiers", fKey_info.modifiers);
				PRINT(("new position : %f, %f, %ld, %ld\n", fMousePos.x, fMousePos.y, xValue, yValue));
	   		}

#ifdef COMPILE_FOR_R5
			if (fAppBuffer) {
				fAppBuffer[0] =
       		    	(0x3 << 30)
               		| ((uint32)fMousePos.x & 0x7fff) << 15
               		| ((uint32)fMousePos.y & 0x7fff);

				PRINT(("released cursorSem with value : %08lx\n", fAppBuffer[0]));
                	release_sem(fCursorSem);
        	}
#endif
		break;
    	
    	// some Key Down and up codes ..
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
		
		BMessage msg(IS_CONTROL_DEVICES);
		msg.AddInt32("type", B_POINTING_DEVICE);
		msg.AddInt32("code", B_MOUSE_MAP_CHANGED);
		status = fAddOnManager->PostMessage(&msg);
	
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
		
		BMessage msg(IS_CONTROL_DEVICES);
		msg.AddInt32("type", B_POINTING_DEVICE);
		msg.AddInt32("code", B_CLICK_SPEED_CHANGED);
		status = fAddOnManager->PostMessage(&msg);
	
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
	
		BMessage msg(IS_CONTROL_DEVICES);
		msg.AddInt32("type", B_KEYBOARD_DEVICE);
		msg.AddInt32("code", B_KEY_REPEAT_RATE_CHANGED);
		status = fAddOnManager->PostMessage(&msg);
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
	CALLED();
	status_t status;
	if (message->what == IS_GET_KEY_MAP) {
		status = reply->AddData("keymap", B_ANY_TYPE, &fKeys, sizeof(fKeys));
		if (status == B_OK)
			status = reply->AddData("key_buffer", B_ANY_TYPE, fChars, fCharsSize);
	} else {
		if (LoadKeymap()!=B_OK)
			LoadSystemKeymap();
		
		BMessage msg(IS_CONTROL_DEVICES);
		msg.AddInt32("type", B_KEYBOARD_DEVICE);
		msg.AddInt32("code", B_KEY_MAP_CHANGED);
		status = fAddOnManager->PostMessage(&msg);
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
	// TODO
	return B_OK;
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
	return write_port(fEventLooperPort, 0, buffer, length);
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
	return write_port(fEventLooperPort, 0, buffer, length);
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
InputServer::EventLoop()
{
	CALLED();
	fEventLooperPort = create_port(100, "obos_is_event_port");
	if(fEventLooperPort < 0) {
		PRINTERR(("OBOS InputServer: create_port error: (0x%x) %s\n", fEventLooperPort, strerror(fEventLooperPort)));
	} 
	fISPortThread = spawn_thread(ISPortWatcher, "_input_server_event_loop_", B_REAL_TIME_DISPLAY_PRIORITY+3, this);
	resume_thread(fISPortThread);

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

	if (fEventsCache.CountItems()>0) {

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
	
	// BPortLink is incompatible with R5 one
#ifndef COMPILE_FOR_R5
	port_id pid = find_port(SERVER_INPUT_PORT);

	BPortLink *appsvrlink = new BPortLink(pid);
   	switch(message->what){
   		case B_MOUSE_MOVED:{
			uint32 buttons;
			BPoint pt;
    			message->FindPoint("where",&pt);
    			int64 time=(int64)real_time_clock();
    			appsvrlink->StartMessage(B_MOUSE_MOVED);
    			appsvrlink->Attach(&time,sizeof(int64));
    			appsvrlink->Attach(&pt.x,sizeof(float));
                        appsvrlink->Attach(&pt.y,sizeof(float));
    			message->FindInt32("buttons",buttons);
    			appsvrlink->Attach(&buttons,sizeof(uint32));
    			appsvrlink->Flush();
    			PRINT(("B_MOUSE_MOVED: x = %lu: y = %lu: time = %llu: buttons = %lu\n",pt.x,pt.y,time,buttons));
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
#else
	status_t  	err;
	
	ssize_t length = message->FlattenedSize();
	char buffer[length];
	if ((err = message->Flatten(buffer,length)) < B_OK)
		return err;
	
	write_port(fAsPort, 0, buffer, length);
	
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
	
	BInputServerFilter *current_filter;
	int32               filter_index  = 0;

	while (NULL != (current_filter = (BInputServerFilter*)gInputFilterList.ItemAt(filter_index) ) ) {
		// Apply the current filter to all available event messages.
		//		
		int32 event_index = 0;
		BMessage *current_event;
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
InputServer::StartStopDevices(const char *name, input_device_type type, bool doStart)
{
	CALLED();

	for (int i = gInputDeviceList.CountItems() - 1; i >= 0; i--) {
		InputDeviceListItem* item = (InputDeviceListItem*)gInputDeviceList.ItemAt(i);
		if (!item)
			continue;
			
		if ((name && strcmp(name, item->mDev.name) == 0) || item->mDev.type == type) {
			if (!item->mIsd)
				return B_ERROR;
				
			input_device_ref   dev = item->mDev;
			
			if (doStart) {
				PRINT(("  Starting: %s\n", dev.name));
				item->mIsd->Start(dev.name, dev.cookie);
				item->mStarted = true;
			} else {
				PRINT(("  Stopping: %s\n", dev.name));
				item->mIsd->Stop(dev.name, dev.cookie);
				item->mStarted = false;
			}
			if (name)
				return B_OK;
		}
	}

	if (name)
		return B_ERROR;
	else
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
InputServer::ControlDevices(const char* name, input_device_type type, uint32 code, BMessage* msg)
{	
	CALLED();
	for (int i = gInputDeviceList.CountItems() - 1; i >= 0; i--) {
		InputDeviceListItem* item = (InputDeviceListItem*)gInputDeviceList.ItemAt(i);
		if (!item)
			continue;
			
		if ((name && strcmp(name, item->mDev.name) == 0) || item->mDev.type == type) {
			if (!item->mIsd)
				return B_ERROR;
				
			input_device_ref   dev = item->mDev;
			
			item->mIsd->Control(dev.name, dev.cookie, code, msg);
			
			if (name)
				return B_OK;
		}
	}

	if (name)
		return B_ERROR;
	else
		return B_OK;
	
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
	return sSafeMode;
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
		ssize_t    	length = port_buffer_size(fEventLooperPort);
		PRINT(("[Event Looper] BMessage Size = %lu\n", length));
		
		int32     	code;
		char buffer[length];
		
		status_t err = read_port(fEventLooperPort, &code, buffer, length);
		if(err != length) {
			if(err >= 0) {
				PRINTERR(("InputServer: failed to read full packet (read %lu of %lu)\n", err, length));
			} else {
				PRINTERR(("InputServer: read_port error: (0x%lx) %s\n", err, strerror(err)));
			}
			continue;
		}
		
		BMessage *event = new BMessage;
	
		if ((err = event->Unflatten(buffer)) < 0) {
			PRINTERR(("[InputServer] Unflatten() error: (0x%lx) %s\n", err, strerror(err)));
			delete event;
		} else {
			// This is where the message should be processed.	

			// here we test for a message coming from app_server, screen resolution change could have happened
			if (event->what == FAST_MOUSE_MOVED) {
				BRect frame = fScreen.Frame();
				if (frame != fFrame) {
					fMousePos.x = fMousePos.x * frame.Width() / fFrame.Width();
					fMousePos.y = fMousePos.y * frame.Height() / fFrame.Height();
					fFrame = frame;
				}
			}
			
			HandleSetMousePosition(event, event);
						
			BList list;
			list.AddItem(event);
			DispatchEvents(&list);
			
			PRINT(("Event written to port\n"));
		}
	}

}

