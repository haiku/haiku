#include <stdio.h>

#include "InputServer.h"
#include "Path.h"
#include "Directory.h"
#include "FindDirectory.h"
#include "Entry.h"

#include "InputServerDeviceListEntry.h"


/*
 *
 */
int main(int argc, char* argv[])
{
	InputServer	*myInputServer;
	
	myInputServer = new InputServer;
	
	myInputServer->Run();
	
	delete myInputServer;

	return(0);
}
/*
thread_id InputServer::Run()
{
	InitDevices();
	
	return 0;
}
*/
/*
 *  Method: InputServer::InputServer()
 *   Descr: 
 */
InputServer::InputServer(void) : BApplication("application/x-vnd.OpenBeOS-input_server")
{
	void *pointer;
	
	InitDevices();
	EventLoop(pointer);
	

}

/*
 *  Method: InputServer::InputServer()
 *   Descr: 
 */
InputServer::~InputServer(void)
{
}


/*
 *  Method: InputServer::ArgvReceived()
 *   Descr: 
 */
void InputServer::ArgvReceived(long,
                          char **)
{
}


/*
 *  Method: InputServer::InitKeyboardMouseStates()
 *   Descr: 
 */
void InputServer::InitKeyboardMouseStates(void)
{
}


/*
 *  Method: InputServer::InitDevices()
 *   Descr: 
 */
void InputServer::InitDevices(void)
{
	BDirectory  dir;
	BPath       addon_dir;
	BPath       addon_path;
	BEntry      entry;
	directory_which addon_dirs[] =
	            {
	                B_BEOS_ADDONS_DIRECTORY,
	                B_COMMON_ADDONS_DIRECTORY,
	                B_USER_ADDONS_DIRECTORY
	            };
	const int   addon_dir_count = sizeof(addon_dirs) / sizeof(directory_which);

	printf("InputServer::InitDevices - Enter\n");
	
	// Find all Input Server Devices in each of the predefined
	// addon directories.
	//
	for (int i = 0; i < addon_dir_count; i++)
	{
		if (B_OK == find_directory(addon_dirs[i], &addon_dir) )
		{
			addon_dir.Append("input_server/devices");
			dir.SetTo(addon_dir.Path() );
			while (B_NO_ERROR == dir.GetNextEntry(&entry, false) )
			{
				entry.GetPath(&addon_path);
				printf("Adding %s . . .\n", addon_path.Path() );
				AddInputServerDevice(addon_path.Path() );
			}
		}
	}
	printf("InputServer::InitDevices - Exit\n");
}


/*
 *  Method: InputServer::AddInputServerDevice()
 *   Descr: 
 */
status_t InputServer::AddInputServerDevice(const char* path)
{
	image_id addon_image= load_add_on(path);
	if (B_ERROR != addon_image)
	{
		status_t            isd_status = B_NO_INIT;
		BInputServerDevice* (*func)()  = NULL;
		if (B_OK == get_image_symbol(addon_image, "instantiate_input_device", B_SYMBOL_TYPE_TEXT, (void**)&func) )
		{
			if (NULL != func)
			{
				/*
			    // :DANGER: Only reenable this section if this
			    //          InputServer can start and manage the
			    //          devices, otherwise the system will hang.
			    //			    
				BInputServerDevice* isd = (*func)();
				if (NULL != isd)
				{
					status_t isd_status = isd->InitCheck();
					mInputServerDeviceList.AddItem(
						new InputServerDeviceListEntry(path, isd_status, isd) );
				}
				*/
				mInputServerDeviceList.AddItem(
					new InputServerDeviceListEntry(path, B_NO_INIT, NULL) );
			}
		}
		if (B_OK != isd_status)
		{
			// Free resources associated with ISD's
			// that failed to initialize.
			//
			unload_add_on(addon_image);
		}
	}
	return 0;
}


/*
 *  Method: InputServer::InitFilters()
 *   Descr: 
 */
void InputServer::InitFilters(void)
{
	BDirectory  dir;
	BPath       addon_dir;
	BPath       addon_path;
	BEntry      entry;
	directory_which addon_dirs[] =
	            {
	                B_BEOS_ADDONS_DIRECTORY,
	                B_COMMON_ADDONS_DIRECTORY,
	                B_USER_ADDONS_DIRECTORY
	            };
	const int   addon_dir_count = sizeof(addon_dirs) / sizeof(directory_which);

	printf("InputServer::InitFilters - Enter\n");
	
	// Find all Input Server Devices in each of the predefined
	// addon directories.
	//
	for (int i = 0; i < addon_dir_count; i++)
	{
		if (B_OK == find_directory(addon_dirs[i], &addon_dir) )
		{
			addon_dir.Append("input_server/filters");
			dir.SetTo(addon_dir.Path() );
			while (B_NO_ERROR == dir.GetNextEntry(&entry, false) )
			{
				entry.GetPath(&addon_path);
				printf("Adding %s . . .\n", addon_path.Path() );
				AddInputServerDevice(addon_path.Path() );
			}
		}
	}
	printf("InputServer::InitDevices - Exit\n");
}


/*
 *  Method: InputServer::InitMethods()
 *   Descr: 
 */
void InputServer::InitMethods(void)
{
	BDirectory  dir;
	BPath       addon_dir;
	BPath       addon_path;
	BEntry      entry;
	directory_which addon_dirs[] =
	            {
	                B_BEOS_ADDONS_DIRECTORY,
	                B_COMMON_ADDONS_DIRECTORY,
	                B_USER_ADDONS_DIRECTORY
	            };
	const int   addon_dir_count = sizeof(addon_dirs) / sizeof(directory_which);

	printf("InputServer::InitDevices - Enter\n");
	
	// Find all Input Server Devices in each of the predefined
	// addon directories.
	//
	for (int i = 0; i < addon_dir_count; i++)
	{
		if (B_OK == find_directory(addon_dirs[i], &addon_dir) )
		{
			addon_dir.Append("input_server/devices");
			dir.SetTo(addon_dir.Path() );
			while (B_NO_ERROR == dir.GetNextEntry(&entry, false) )
			{
				entry.GetPath(&addon_path);
				printf("Adding %s . . .\n", addon_path.Path() );
				AddInputServerDevice(addon_path.Path() );
			}
		}
	}
	printf("InputServer::InitDevices - Exit\n");
}


/*
 *  Method: InputServer::QuitRequested()
 *   Descr: 
 */
bool InputServer::QuitRequested(void)
{
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
}


/*
 *  Method: InputServer::MessageReceived()
 *   Descr: 
 */
void InputServer::MessageReceived(BMessage *message)
{
BMessenger *app_server;
	switch(message->what)
	{
		case SET_METHOD:
		{
			//HandleSetMethod();
			break;
		}
		case GET_MOUSE_TYPE: 
		{
			//HandleGetSetMouseType();
			break;	
		}
		case SET_MOUSE_TYPE:
		{
			//HandleGetSetMouseType();
			break;
		}
		case GET_MOUSE_ACCELERATION:
		{
			//HandleGetSetMouseAcceleration();
			break;
		}
		case SET_MOUSE_ACCELERATION:
		{
			//HandleGetSetMouseAcceleration();
			break;
		} 
		case GET_KEY_REPEAT_DELAY:
		{
			//HandleGetSetKeyRepeatDelay();
			break;
		} 
		case SET_KEY_REPEAT_DELAY:
		{
			//HandleGetSetKeyRepeatDelay();
			break;
		} 
		case GET_KEY_INFO:
		{
			//HandleGetKeyInfo();
			break;
		} 
		case GET_MODIFIERS:
		{
			//HandleGetModifiers();
			break;
		} 
		case SET_MODIFIER_KEY:
		{
			//HandleSetModifierKey();
			break;
		} 
		case SET_KEYBOARD_LOCKS:
		{
			//HandleSetKeyboardLocks();
			break;
		} 
		case GET_MOUSE_SPEED:
		{
			//HandleGetSetMouseSpeed();
			break;
		} 
		case SET_MOUSE_SPEED:
		{
			//HandleGetSetMouseSpeed();
			break;
		} 
		case SET_MOUSE_POSITION:
		{
			//HandleSetMousePosition();
			break;
		} 
		case GET_MOUSE_MAP:
		{
			//HandleGetSetMouseMap();
			break;
		} 
		case SET_MOUSE_MAP:
		{
			//HandleGetSetMouseMap();
			break;
		} 
		case GET_KEYBOARD_ID:
		{
			//HandleGetKeyboardID();
			break;
		} 
		case GET_CLICK_SPEED:
		{
			//HandleGetSetClickSpeed();
			break;
		} 
		case SET_CLICK_SPEED:
		{
			//HandleGetSetClickSpeed();
			break;
		} 
		case GET_KEY_REPEAT_RATE:
		{
			//HandleGetSetKeyRepeatRate();
			break;
		} 
		case SET_KEY_REPEAT_RATE:
		{
			//HandleGetSetKeyRepeatRate();
			break;
		} 
		case GET_KEY_MAP:
		{
			//HandleGetSetKeyMap();
			break;
		} 
		case SET_KEY_MAP:
		{
			//HandleGetSetKeyMap();
			break;
		} 
		case FOCUS_IM_AWARE_VIEW:
		{
			//HandleFocusUnfocusIMAwareView();
			break;
		} 
		case UNFOCUS_IM_AWARE_VIEW:
		{
			//HandleFocusUnfocusIMAwareView();
			break;
		} 
		
		default:
		{
		app_server = new BMessenger("application/x-vnd.Be-APPS", -1, NULL);
		if (app_server->IsValid())
		{
			app_server->SendMessage(message);
			
		}
		delete app_server;
		break;
		}
	}
}


/*
 *  Method: InputServer::HandleSetMethod()
 *   Descr: 
 */
void InputServer::HandleSetMethod(BMessage *)
{
}


/*
 *  Method: InputServer::HandleGetSetMouseType()
 *   Descr: 
 */
void InputServer::HandleGetSetMouseType(BMessage *,
                                   BMessage *)
{
}


/*
 *  Method: InputServer::HandleGetSetMouseAcceleration()
 *   Descr: 
 */
void InputServer::HandleGetSetMouseAcceleration(BMessage *,
                                           BMessage *)
{
}


/*
 *  Method: InputServer::HandleGetSetKeyRepeatDelay()
 *   Descr: 
 */
void InputServer::HandleGetSetKeyRepeatDelay(BMessage *,
                                        BMessage *)
{
}


/*
 *  Method: InputServer::HandleGetKeyInfo()
 *   Descr: 
 */
void InputServer::HandleGetKeyInfo(BMessage *,
                              BMessage *)
{
}


/*
 *  Method: InputServer::HandleGetModifiers()
 *   Descr: 
 */
void InputServer::HandleGetModifiers(BMessage *,
                                BMessage *)
{
}


/*
 *  Method: InputServer::HandleSetModifierKey()
 *   Descr: 
 */
void InputServer::HandleSetModifierKey(BMessage *,
                                  BMessage *)
{
}


/*
 *  Method: InputServer::HandleSetKeyboardLocks()
 *   Descr: 
 */
void InputServer::HandleSetKeyboardLocks(BMessage *,
                                    BMessage *)
{
}


/*
 *  Method: InputServer::HandleGetSetMouseSpeed()
 *   Descr: 
 */
void InputServer::HandleGetSetMouseSpeed(BMessage *,
                                    BMessage *)
{
}


/*
 *  Method: InputServer::HandleSetMousePosition()
 *   Descr: 
 */
void InputServer::HandleSetMousePosition(BMessage *,
                                    BMessage *)
{
}


/*
 *  Method: InputServer::HandleGetSetMouseMap()
 *   Descr: 
 */
void InputServer::HandleGetSetMouseMap(BMessage *,
                                  BMessage *)
{
}


/*
 *  Method: InputServer::HandleGetKeyboardID()
 *   Descr: 
 */
void InputServer::HandleGetKeyboardID(BMessage *,
                                 BMessage *)
{
}


/*
 *  Method: InputServer::HandleGetSetClickSpeed()
 *   Descr: 
 */
void InputServer::HandleGetSetClickSpeed(BMessage *,
                                    BMessage *)
{
}


/*
 *  Method: InputServer::HandleGetSetKeyRepeatRate()
 *   Descr: 
 */
void InputServer::HandleGetSetKeyRepeatRate(BMessage *,
                                       BMessage *)
{
}


/*
 *  Method: InputServer::HandleGetSetKeyMap()
 *   Descr: 
 */
void InputServer::HandleGetSetKeyMap(BMessage *,
                                BMessage *)
{
}


/*
 *  Method: InputServer::HandleFocusUnfocusIMAwareView()
 *   Descr: 
 */
void InputServer::HandleFocusUnfocusIMAwareView(BMessage *,
                                           BMessage *)
{
}


/*
 *  Method: InputServer::EnqueueDeviceMessage()
 *   Descr: 
 */
status_t InputServer::EnqueueDeviceMessage(BMessage *)
{
	return 0;
}


/*
 *  Method: InputServer::EnqueueMethodMessage()
 *   Descr: 
 */
status_t InputServer::EnqueueMethodMessage(BMessage *)
{
	return 0;
}


/*
 *  Method: InputServer::UnlockMethodQueue()
 *   Descr: 
 */
status_t InputServer::UnlockMethodQueue(void)
{
	return 0;
}


/*
 *  Method: InputServer::LockMethodQueue()
 *   Descr: 
 */
status_t InputServer::LockMethodQueue(void)
{
	return 0;
}


/*
 *  Method: InputServer::SetNextMethod()
 *   Descr: 
 */
status_t InputServer::SetNextMethod(bool)
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
const BMessenger* InputServer::MethodReplicant(void)
{
	return NULL;
}


/*
 *  Method: InputServer::EventLoop()
 *   Descr: 
 */
status_t InputServer::EventLoop(void *)
{

	EventLooperPort = create_port(29,"is_event_port");
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
bool InputServer::EventLoopRunning(void)
{
	return true;
}


/*
 *  Method: InputServer::DispatchEvents()
 *   Descr: 
 */
bool InputServer::DispatchEvents(BList *)
{
	return true;
}


/*
 *  Method: InputServer::CacheEvents()
 *   Descr: 
 */
bool InputServer::CacheEvents(BList *)
{
	return true;
}


/*
 *  Method: InputServer::GetNextEvents()
 *   Descr: 
 */
const BList* InputServer::GetNextEvents(BList *)
{
	return NULL;
}


/*
 *  Method: InputServer::FilterEvents()
 *   Descr: 
 */
bool InputServer::FilterEvents(BList *)
{
	return true;
}


/*
 *  Method: InputServer::SanitizeEvents()
 *   Descr: 
 */
bool InputServer::SanitizeEvents(BList *)
{
	return true;
}


/*
 *  Method: InputServer::MethodizeEvents()
 *   Descr: 
 */
bool InputServer::MethodizeEvents(BList *,
                             bool)
{
	return true;
}


/*
 *  Method: InputServer::StartStopDevices()
 *   Descr: 
 */
status_t InputServer::StartStopDevices(const char* deviceName, input_device_type deviceType, bool doStart)
{
	return 0;
}


/*
 *  Method: InputServer::ControlDevices()
 *   Descr: 
 */
status_t InputServer::ControlDevices(const char *,
                            input_device_type,
                            unsigned long,
                            BMessage *)
{
	return 0;
}


/*
 *  Method: InputServer::DoMouseAcceleration()
 *   Descr: 
 */
bool InputServer::DoMouseAcceleration(long *,
                                 long *)
{
	return true;
}


/*
 *  Method: InputServer::SetMousePos()
 *   Descr: 
 */
bool InputServer::SetMousePos(long *,
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
bool InputServer::SetMousePos(long *,
                         long *,
                         BPoint)
{
	return true;
}


/*
 *  Method: InputServer::SetMousePos()
 *   Descr: 
 */
bool InputServer::SetMousePos(long *,
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
bool InputServer::SafeMode(void)
{
	return true;
}

int32 InputServer::ISPortWatcher(void *arg)
{
	InputServer *self = (InputServer*)arg;
	self->WatchPort();
	return (B_NO_ERROR);
}

void InputServer::WatchPort()
{
	int32 code;
	ssize_t length;
	char *buffer;
	BMessage *event;
	status_t err;

	while (true) { 
		// Block until we find the size of the next message
		length = port_buffer_size(EventLooperPort);
		buffer = (char*)malloc(length);
		event = NULL;
		event = new BMessage();
		err = read_port(EventLooperPort, &code, buffer, length);
		if(err != length) {
			if(err >= 0) {
				_sPrintf("InputServer: failed to read full packet (read %u of %u)\n",err,length);
			} else {
				_sPrintf("InputServer: read_port error: (0x%x) %s\n",err,strerror(err));
			}
		} else if ((err = event->Unflatten(buffer)) < 0) {
			_sPrintf("InputServer: (0x%x) %s\n",err,strerror(err));
		} else {
			// add the event
			//CacheEvents(event);	
			event = NULL;
		}
		free( buffer);
		if(event!=NULL) {
			delete(event);
			event = NULL;  
		} 
	}

}

