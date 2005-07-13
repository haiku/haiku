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
// Copyright (c) 2002-2005 Haiku Project
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


#include <Deskbar.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Roster.h>
#include <Locker.h>
#include <Message.h>
#include <String.h>
#include <OS.h>
#include <driver_settings.h>

#include <stdio.h>

#include "InputServer.h"
#include "InputServerTypes.h"
#include "kb_mouse_driver.h"
#include "MethodReplicant.h"

#ifndef USE_R5_STYLE_COMM
// include app_server headers for communication
#include <PortLink.h>
#include <ServerProtocol.h>
#endif

#define FAST_MOUSE_MOVED '_FMM'		// received from app_server when screen res changed, but could be sent to too. 


extern "C" void RegisterDevices(input_device_ref** devices)
{
	CALLED();
};

#ifdef COMPILE_FOR_R5
extern "C" status_t _kget_safemode_option_(const char *parameter, char *buffer, size_t *_bufferSize);
#else
extern "C" status_t _kern_get_safemode_option(const char *parameter, char *buffer, size_t *_bufferSize);
#endif


// Static InputServer member variables.
//
BList   InputServer::gInputDeviceList;
BLocker InputServer::gInputDeviceListLocker("is_device_queue_sem");

BList   InputServer::gInputFilterList;
BLocker InputServer::gInputFilterListLocker("is_filter_queue_sem");

BList   InputServer::gInputMethodList;
BLocker InputServer::gInputMethodListLocker("is_method_queue_sem");

DeviceManager InputServer::gDeviceManager;

KeymapMethod InputServer::gKeymapMethod;

#if DEBUG == 2
FILE *InputServer::sLogFile = NULL;
#endif


extern "C" _EXPORT BView* instantiate_deskbar_item();

BView *
instantiate_deskbar_item()
{
	return new MethodReplicant(INPUTSERVER_SIGNATURE);
}


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
	fScreen(B_MAIN_SCREEN_ID),
	fBLWindow(NULL),
	fIMAware(false)
{
#if DEBUG == 2
	if (sLogFile == NULL)
		sLogFile = fopen("/var/log/input_server.log", "a");
#endif

	CALLED();

	EventLoop();

	char parameter[32];
	size_t parameterLength = sizeof(parameter);

#ifdef COMPILE_FOR_R5
	if (_kget_safemode_option_(B_SAFEMODE_SAFE_MODE, parameter, &parameterLength) == B_OK) {
#else
	if (_kern_get_safemode_option(B_SAFEMODE_SAFE_MODE, parameter, &parameterLength) == B_OK) {
#endif
		if (!strcasecmp(parameter, "enabled") || !strcasecmp(parameter, "on")
			|| !strcasecmp(parameter, "true") || !strcasecmp(parameter, "yes")
			|| !strcasecmp(parameter, "enable") || !strcmp(parameter, "1"))
			sSafeMode = true;
	}

	gDeviceManager.LoadState();

#ifdef R5_CURSOR_COMM
	if (has_data(find_thread(NULL))) {
		PRINT(("HasData == YES\n")); 
		int32 buffer[2];
		thread_id appThreadId;
		memset(buffer, 0, sizeof(buffer));
		int32 code = receive_data(&appThreadId, buffer, sizeof(buffer));
		(void)code;	// suppresses warning in non-debug build
		PRINT(("tid : %ld, code :%ld\n", appThreadId, code));
		for (int32 i = 0; i < 2; i++) {
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
#ifdef APPSERVER_PORTLINK_COMM
	port_id input_port = find_port(SERVER_INPUT_PORT);
	if (input_port == B_NAME_NOT_FOUND)
		PRINTERR(("input_server couldn't find app_server's input port\n"));
	fAppServerLink = new BPrivate::PortLink(input_port);
#endif 

	InitKeyboardMouseStates();

	fAddOnManager = new AddOnManager(SafeMode());
	fAddOnManager->LoadState();

	BMessenger messenger(this);
	BRoster().StartWatching(messenger, B_REQUEST_LAUNCHED);
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
	
#ifdef R5_CURSOR_COMM
	delete_port(fAsPort);
	fAsPort = -1;
	fAppBuffer = NULL;
	delete_area(fCloneArea);
#endif // R5_CURSOR_COMM

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
	// fMousePos is then set to the center of the screen.

	fFrame = fScreen.Frame();
	if (fFrame == BRect(0,0,0,0))
		fFrame = BRect(0,0,800,600);
	fMousePos = BPoint(fFrame.right/2, fFrame.bottom/2);
	
	if (LoadKeymap()!=B_OK)
		LoadSystemKeymap();
		
	BMessage *msg = new BMessage(B_MOUSE_MOVED);
	HandleSetMousePosition(msg, msg);

	fActiveMethod = &gKeymapMethod;
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
	if (file.Read(fChars, fCharsSize) != (signed)fCharsSize)
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
		
	PostMessage(SYSTEM_SHUTTING_DOWN);
	
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
		case IS_METHOD_REGISTER:
			fAddOnManager->PostMessage(message);
			return;
		case B_SOME_APP_LAUNCHED: {
			const char *signature;
			if (message->FindString("be:signature", &signature)==B_OK) {
				PRINT(("input_server : %s\n", signature));
				if (strcmp(signature, "application/x-vnd.Be-TSKB")==0) {
				
				}
			}
			return;

		}
			break;
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
InputServer::HandleSetMethod(BMessage *message)
{
	CALLED();
	uint32 cookie;
	if (message->FindInt32("cookie", (int32*)&cookie) == B_OK) {
		BInputServerMethod *method = (BInputServerMethod*)cookie;
		PRINT(("%s cookie %p\n", __PRETTY_FUNCTION__, method));
		SetActiveMethod(method);
	}
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
    		if ((outbound->FindInt32("x", &xValue) == B_OK) 
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
		else if (outbound->FindPoint("where", &fMousePos) == B_OK) {
			outbound->RemoveName("where");
			fMousePos.ConstrainTo(fFrame);
			outbound->AddPoint("where", fMousePos);
			outbound->AddInt32("modifiers", fKey_info.modifiers);
			PRINT(("new position : %f, %f\n", fMousePos.x, fMousePos.y));
		}
#ifdef R5_CURSOR_COMM
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
		case B_KEY_DOWN:
		case B_KEY_UP:
		case B_UNMAPPED_KEY_DOWN:
		case B_UNMAPPED_KEY_UP:
		case B_MODIFIERS_CHANGED: {
			ssize_t size = 0;
			uint8 *data = NULL;
			outbound->FindInt32("modifiers", (int32*)&fKey_info.modifiers);
			if ((outbound->FindData("states", B_UINT8_TYPE, (const void**)&data, &size) == B_OK) 
				&& (size == (ssize_t)sizeof(fKey_info.key_states))) {
				memcpy(fKey_info.key_states, data, size);
			}
		}
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
InputServer::HandleFocusUnfocusIMAwareView(BMessage *message,
                                           BMessage *reply)
{
	CALLED();

	BMessenger messenger;
	status_t status = message->FindMessenger("view", &messenger);

	if (status != B_OK)
		return status;

	// check if current view is ours

	if (message->what == IS_FOCUS_IM_AWARE_VIEW) {
		PRINT(("HandleFocusUnfocusIMAwareView : entering\n"));
		fIMAware = true;
	} else {
		PRINT(("HandleFocusUnfocusIMAwareView : leaving\n"));
		fIMAware = false;
	}

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
	PRINT(("%s what:%c%c%c%c\n", __PRETTY_FUNCTION__, message->what>>24, message->what>>16, message->what>>8, message->what));
#ifdef DEBUG
	if (message->what == 'IMEV') {
		int32 code;
		message->FindInt32("be:opcode", &code);
		PRINT(("%s be:opcode %li\n", __PRETTY_FUNCTION__, code));
	}
#endif
	LockMethodQueue();

	fMethodQueue.AddItem(message);

	write_port_etc(fEventLooperPort, 0, NULL, 0, B_RELATIVE_TIMEOUT, 0);

	UnlockMethodQueue();

	return B_OK;
}


/*
 *  Method: InputServer::UnlockMethodQueue()
 *   Descr: 
 */
status_t 
InputServer::UnlockMethodQueue()
{
	gInputMethodListLocker.Unlock();
	return B_OK;
}


/*
 *  Method: InputServer::LockMethodQueue()
 *   Descr: 
 */
status_t 
InputServer::LockMethodQueue()
{
	gInputMethodListLocker.Lock();
	return B_OK;
}


/*
 *  Method: InputServer::SetNextMethod()
 *   Descr: 
 */
status_t
InputServer::SetNextMethod(bool direction)
{
	LockMethodQueue();
	
	int32 index = gInputMethodList.IndexOf(fActiveMethod);
	index += (direction ? 1 : -1);
	
	if (index < -1)
		index = gInputMethodList.CountItems() - 1;
	if (index >= gInputMethodList.CountItems())
		index = -1;

	BInputServerMethod *method = &gKeymapMethod;

	if (index != -1)
		method = (BInputServerMethod *)gInputMethodList.ItemAt(index);
		
	SetActiveMethod(method);

	UnlockMethodQueue();
	return B_OK;
}


/*
 *  Method: InputServer::SetActiveMethod()
 *   Descr: 
 */
void
InputServer::SetActiveMethod(BInputServerMethod *method)
{
	CALLED();
	if (fActiveMethod)
		fActiveMethod->fOwner->MethodActivated(false);
	fActiveMethod = method;
	
	if (fActiveMethod) {
                fActiveMethod->fOwner->MethodActivated(true);
	}
}


/*
 *  Method: InputServer::MethodReplicant()
 *   Descr: 
 */
const BMessenger* 
InputServer::MethodReplicant()
{
	return fReplicantMessenger;
}


void
InputServer::SetMethodReplicant(const BMessenger *messenger)
{
	fReplicantMessenger = messenger;
}


/*
 *  Method: InputServer::EventLoop()
 *   Descr: 
 */
status_t
InputServer::EventLoop()
{
	CALLED();
	fEventLooperPort = create_port(100, "haiku_is_event_port");
	if(fEventLooperPort < 0) {
		PRINTERR(("InputServer: create_port error: (0x%x) %s\n", fEventLooperPort, strerror(fEventLooperPort)));
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
	return fEventLooperPort > -1;
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
			if (event->what == B_INPUT_METHOD_EVENT && !fIMAware) {
				SERIAL_PRINT(("IME received\n"));
				int32 opcode = -1;
				event->FindInt32("be:opcode", &opcode);
				BList events;
				if (!fBLWindow && opcode == B_INPUT_METHOD_STARTED) {
					fBLWindow = new BottomlineWindow(be_plain_font);
				}
				
				if (fBLWindow) {
					fBLWindow->HandleInputMethodEvent(event, &events);
				
					if (!events.IsEmpty()) {
						fBLWindow->PostMessage(B_QUIT_REQUESTED);
						fBLWindow = NULL;

						fEventsCache.AddList(&events);
					}
				}
			} else {
				DispatchEvent(event);
			}

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

	switch (message->what) {
		case B_KEY_DOWN:
		case B_KEY_UP:
		case B_UNMAPPED_KEY_UP:
		case B_UNMAPPED_KEY_DOWN: {
			uint32 modifiers;
			ssize_t size;
			uint8 *data;	
			if (message->FindData("states", B_UINT8_TYPE, (const void**)&data, &size) != B_OK)
				message->AddData("states", B_UINT8_TYPE, fKey_info.key_states, 16);
			if (message->FindInt32("modifiers", (int32*)&modifiers)!=B_OK)
				message->AddInt32("modifiers", fKey_info.modifiers);

			break;
		}
	}
	
#ifdef APPSERVER_PORTLINK_COMM
	if (!fAppServerLink) {
		debugger("InputServer::DispatchEvent(): app_server link not valid\n");
		return false;
	}
	
	switch(message->what){
		case B_MOUSE_MOVED:{
			uint32 buttons;
			BPoint pt;
			int64 time;
    			message->FindPoint("where",&pt);
			message->FindInt64("when", &time);
			fAppServerLink->StartMessage(B_MOUSE_MOVED);
    			fAppServerLink->Attach(&time,sizeof(int64));
    			fAppServerLink->Attach(&pt.x,sizeof(float));
				fAppServerLink->Attach(&pt.y,sizeof(float));
    			message->FindInt32("buttons",buttons);
    			fAppServerLink->Attach(&buttons,sizeof(uint32));
    			fAppServerLink->Flush();
    			PRINT(("B_MOUSE_MOVED: x = %f: y = %f: time = %llu: buttons = %lu\n",pt.x,pt.y,time,buttons));
    		break;
    		}
    	case B_MOUSE_DOWN:{

			BPoint pt;
			int32 buttons,clicks,mod;
			int64 time;
			message->FindInt64("when", &time);
			
			if(message->FindPoint("where",&pt)!=B_OK ||
					message->FindInt32("modifiers",&mod)!=B_OK ||
					message->FindInt32("buttons",&buttons)!=B_OK ||
					message->FindInt32("clicks",&clicks)!=B_OK)
				break;
		
			fAppServerLink->StartMessage(B_MOUSE_DOWN);
			fAppServerLink->Attach(&time, sizeof(int64));
			fAppServerLink->Attach(&pt.x,sizeof(float));
			fAppServerLink->Attach(&pt.y,sizeof(float));
			fAppServerLink->Attach(&mod, sizeof(uint32));
			fAppServerLink->Attach(&buttons, sizeof(uint32));
			fAppServerLink->Attach(&clicks, sizeof(uint32));
			fAppServerLink->Flush();
    		break;
    		}
    	case B_MOUSE_UP:{
			BPoint pt;
			int32 mod;
			int64 time;
			message->FindInt64("when", &time);

			if(message->FindPoint("where",&pt)!=B_OK ||
					message->FindInt32("modifiers",&mod)!=B_OK)
				break;
			
			fAppServerLink->StartMessage(B_MOUSE_UP);
			fAppServerLink->Attach(&time, sizeof(int64));
			fAppServerLink->Attach(&pt.x,sizeof(float));
			fAppServerLink->Attach(&pt.y,sizeof(float));
			fAppServerLink->Attach(&mod, sizeof(uint32));
			fAppServerLink->Flush();
    		break;
    		}
    	case B_MOUSE_WHEEL_CHANGED:{
			float x,y;
			message->FindFloat("be:wheel_delta_x",&x);
			message->FindFloat("be:wheel_delta_y",&y);
			bigtime_t time;
			message->FindInt64("when", &time);
			
			fAppServerLink->StartMessage(B_MOUSE_WHEEL_CHANGED);
			fAppServerLink->Attach(&time,sizeof(int64));
			fAppServerLink->Attach(x);
			fAppServerLink->Attach(y);
			fAppServerLink->Flush();
			break;
    		}
		case B_KEY_DOWN:{
			bigtime_t time;
			int32 scancode, asciicode,repeatcount,modifiers;
			int8 utf8data[3];
			BString string;
			uint8 keyarray[16];

			message->FindInt64("when", &time);
			message->FindInt32("key",&scancode);
			message->FindInt32("be:key_repeat",&repeatcount);
			message->FindInt32("modifiers",&modifiers);
			message->FindInt32("raw_char",&asciicode);
			message->FindInt8("byte",0,utf8data);
			message->FindInt8("byte",1,utf8data+1);
			message->FindInt8("byte",2,utf8data+2);
			message->FindString("bytes",&string);
			uint8 *data = NULL;
			ssize_t size = 0;
			if ((message->FindData("states", B_UINT8_TYPE, (const void**)&data, &size) == B_OK)
				&& (size == (ssize_t)sizeof(keyarray))) {
				memcpy(keyarray, data, size);
			}
			fAppServerLink->StartMessage(B_KEY_DOWN);
			fAppServerLink->Attach(&time,sizeof(bigtime_t));
			fAppServerLink->Attach(scancode);
			fAppServerLink->Attach(asciicode);
			fAppServerLink->Attach(repeatcount);
			fAppServerLink->Attach(modifiers);
			fAppServerLink->Attach(utf8data,sizeof(int8)*3);
			fAppServerLink->AttachString(string.String());
			fAppServerLink->Attach(keyarray,sizeof(int8)*16);
			fAppServerLink->Flush();
			break;
		}
		case B_KEY_UP:{
			bigtime_t time;
			int32 scancode, asciicode,modifiers;
			int8 utf8data[3];
			BString string;
			uint8 keyarray[16];

			message->FindInt64("when", &time);
			message->FindInt32("key",&scancode);
			message->FindInt32("raw_char",&asciicode);
			message->FindInt32("modifiers",&modifiers);
			message->FindInt8("byte",0,utf8data);
			message->FindInt8("byte",1,utf8data+1);
			message->FindInt8("byte",2,utf8data+2);
			message->FindString("bytes",&string);
			uint8 *data = NULL;
			ssize_t size = 0;
			if ((message->FindData("states", B_UINT8_TYPE, (const void**)&data, &size) == B_OK)
				&& (size == (ssize_t)sizeof(keyarray))) {
				memcpy(keyarray, data, size);
			}

			fAppServerLink->StartMessage(B_KEY_UP);
			fAppServerLink->Attach(&time,sizeof(bigtime_t));
			fAppServerLink->Attach(scancode);
			fAppServerLink->Attach(asciicode);
			fAppServerLink->Attach(modifiers);
			fAppServerLink->Attach(utf8data,sizeof(int8)*3);
			fAppServerLink->AttachString(string.String());
			fAppServerLink->Attach(keyarray,sizeof(int8)*16);
			fAppServerLink->Flush();
			break;
		}
		case B_UNMAPPED_KEY_DOWN:{
			bigtime_t time;
			int32 scancode,modifiers;
			int8 keyarray[16];

			message->FindInt64("when", &time);
			message->FindInt32("key",&scancode);
			message->FindInt32("modifiers",&modifiers);
			for(int8 i=0;i<15;i++)
				message->FindInt8("states",i,&keyarray[i]);
			fAppServerLink->StartMessage(B_UNMAPPED_KEY_DOWN);
			fAppServerLink->Attach(&time,sizeof(bigtime_t));
			fAppServerLink->Attach(scancode);
			fAppServerLink->Attach(modifiers);
			fAppServerLink->Attach(keyarray,sizeof(int8)*16);
			fAppServerLink->Flush();
			break;
		}
		case B_UNMAPPED_KEY_UP:{
			bigtime_t time;
			int32 scancode,modifiers;
			int8 keyarray[16];

			message->FindInt64("when", &time);
			message->FindInt32("key",&scancode);
			message->FindInt32("modifiers",&modifiers);
			for(int8 i=0;i<15;i++)
				message->FindInt8("states",i,&keyarray[i]);
			fAppServerLink->StartMessage(B_UNMAPPED_KEY_UP);
			fAppServerLink->Attach(&time,sizeof(bigtime_t));
			fAppServerLink->Attach(scancode);
			fAppServerLink->Attach(modifiers);
			fAppServerLink->Attach(keyarray,sizeof(int8)*16);
			fAppServerLink->Flush();
			break;
		}
		case B_MODIFIERS_CHANGED:{
			bigtime_t time;
			int32 scancode,modifiers,oldmodifiers;
			int8 keyarray[16];

			message->FindInt64("when", &time);
			message->FindInt32("key",&scancode);
			message->FindInt32("modifiers",&modifiers);
			message->FindInt32("be:old_modifiers",&oldmodifiers);
			for(int8 i=0;i<15;i++)
				message->FindInt8("states",i,&keyarray[i]);
			fAppServerLink->StartMessage(B_MODIFIERS_CHANGED);
			fAppServerLink->Attach(&time,sizeof(bigtime_t));
			fAppServerLink->Attach(scancode);
			fAppServerLink->Attach(modifiers);
			fAppServerLink->Attach(oldmodifiers);
			fAppServerLink->Attach(keyarray,sizeof(int8)*16);
			fAppServerLink->Flush();
			break;
		}
   		default:
      		break;
   			
		}
	
#else // APPSERVER_PORTLINK_COMM

	status_t  	err;
	
	ssize_t length = message->FlattenedSize();
	char buffer[length];
	if ((err = message->Flatten(buffer,length)) < B_OK)
		return err;
	
	if (fAsPort>0)
		write_port(fAsPort, 0, buffer, length);
	
#endif	// APPSERVER_PORTLINK_COMM

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

	SanitizeEvents(eventsToCache);

	MethodizeEvents(eventsToCache, true);
	
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
					delete (BMessage*)out_item;
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
					delete (BMessage*)out_item;
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
InputServer::SanitizeEvents(BList *events)
{
	CALLED();
	int32 index = 0;
	BMessage *event;
	while (NULL != (event = (BMessage*)events->ItemAt(index) ) ) {
		switch (event->what) {
			case B_KEY_DOWN:
			case B_UNMAPPED_KEY_DOWN:
				// we scan for Alt+Space key down events which means we change to next input method
				// Note : Shift+Alt+Space key allows to change to the previous input method

				if ((fKey_info.modifiers & B_COMMAND_KEY) 
					&& (fKey_info.key_states[KEY_Spacebar >> 3] & (1 << (7 - (KEY_Spacebar % 8))))) {
					SetNextMethod(!fKey_info.modifiers & B_SHIFT_KEY);
				
					// this event isn't sent to the user
					events->RemoveItem(index);
					delete event;
					continue;
				}
				break;
		}

		index++;
	}	

	return true;
}


/*
 *  Method: InputServer::MethodizeEvents()
 *   Descr: 
 */
bool 
InputServer::MethodizeEvents(BList *events,
                             bool)
{
	CALLED();
	
	if (fActiveMethod) {

		BList newList;
		newList.AddList(&fMethodQueue);
		fMethodQueue.MakeEmpty();
		
		for (int32 i=0; i<events->CountItems(); i++) {
			BMessage *item = (BMessage *)events->ItemAt(i);
			BList filterList;
			filter_result res = fActiveMethod->Filter(item, &filterList);
			switch (res) {
				case B_SKIP_MESSAGE:
					delete item;
				case B_DISPATCH_MESSAGE:
					if (filterList.CountItems()>0) {
						newList.AddList(&filterList);
						delete item;
					} else
						newList.AddItem(item);
			}

			newList.AddList(&fMethodQueue);
			fMethodQueue.MakeEmpty();
		}

		events->MakeEmpty();
		events->AddList(&newList);
	}

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
			
		if ((name && strcmp(name, item->mDev.name) == 0) || (!name && item->mDev.type == type)) {
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
InputServer::DoMouseAcceleration(int32 *x,
                                 int32 *y)
{
	CALLED();
	int32 speed = fMouseSettings.MouseSpeed() >> 15;

	// ToDo: implement mouse acceleration
	(void)speed;
	//*y = *x * speed;
	PRINT(("DoMouse : %ld %ld %ld %ld\n", *x, *y, speed, fMouseSettings.MouseSpeed()));
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
	CALLED();
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
	CALLED();
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
	CALLED();
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

		if (length == 0) {
			BList list;
			DispatchEvents(&list);
			continue;
		}
		
		BMessage *event = new BMessage;
	
		if ((err = event->Unflatten(buffer)) < 0) {
			PRINTERR(("[InputServer] Unflatten() error: (0x%lx) %s\n", err, strerror(err)));
			delete event;
			continue;
		} 
		
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

