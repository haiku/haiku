//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		AppServer.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	main manager object for the app_server
//  
//------------------------------------------------------------------------------
#include <AppDefs.h>
#include <PortMessage.h>
#include <Entry.h>
#include <Path.h>
#include <Directory.h>
#include <PortMessage.h>
#include <PortLink.h>

#include <Session.h>

#include <File.h>
#include <Message.h>
#include "AppServer.h"
#include "ColorSet.h"
#include "Desktop.h"
#include "DisplayDriver.h"
#include "ServerApp.h"
#include "ServerCursor.h"
#include "ServerProtocol.h"
#include "ServerWindow.h"
#include "DefaultDecorator.h"
#include "RGBColor.h"
#include "BitmapManager.h"
#include "CursorManager.h"
#include "Utils.h"
#include "FontServer.h"

//#define DEBUG_KEYHANDLING
#ifdef DEBUG_KEYHANDLING
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif
// Globals

//! Used to access the app_server from new_decorator
AppServer *app_server=NULL;

//! Default background color for workspaces
RGBColor workspace_default_color(51,102,160);

//! System-wide GUI color object
ColorSet gui_colorset;

/*!
	\brief Constructor
	
	This loads the default fonts, allocates all the major global variables, spawns the main housekeeping
	threads, loads user preferences for the UI and decorator, and allocates various locks.
*/
#ifdef TEST_MODE
AppServer::AppServer(void) : BApplication (SERVER_SIGNATURE)
#else
AppServer::AppServer(void)
#endif
{
	_mouseport=create_port(200,SERVER_INPUT_PORT);
	_messageport=create_port(200,SERVER_PORT_NAME);

	_applist=new BList(0);
	_quitting_server=false;
	_exit_poller=false;
	_ssindex=1;
	make_decorator=NULL;

	// Create the font server and scan the proper directories.
	fontserver=new FontServer;
	fontserver->Lock();

	// Used for testing purposes

	// TODO: Uncomment when actually put to use. Commented out for speed
	fontserver->ScanDirectory("/boot/beos/etc/fonts/ttfonts/");
//	fontserver->ScanDirectory("/boot/beos/etc/fonts/PS-Type1/");
//	fontserver->ScanDirectory("/boot/home/config/fonts/ttfonts/");
//	fontserver->ScanDirectory("/boot/home/config/fonts/psfonts/");
	fontserver->SaveList();

	if(!fontserver->SetSystemPlain(DEFAULT_PLAIN_FONT_FAMILY,DEFAULT_PLAIN_FONT_STYLE,DEFAULT_PLAIN_FONT_SIZE))
		printf("Couldn't set plain to %s, %s %d pt\n",DEFAULT_PLAIN_FONT_FAMILY,
				DEFAULT_PLAIN_FONT_STYLE,DEFAULT_PLAIN_FONT_SIZE);
	if(!fontserver->SetSystemBold(DEFAULT_BOLD_FONT_FAMILY,DEFAULT_BOLD_FONT_STYLE,DEFAULT_BOLD_FONT_SIZE))
		printf("Couldn't set bold to %s, %s %d pt\n",DEFAULT_BOLD_FONT_FAMILY,
				DEFAULT_BOLD_FONT_STYLE,DEFAULT_BOLD_FONT_SIZE);
	if(!fontserver->SetSystemFixed(DEFAULT_FIXED_FONT_FAMILY,DEFAULT_FIXED_FONT_STYLE,DEFAULT_FIXED_FONT_SIZE))
		printf("Couldn't set fixed to %s, %s %d pt\n",DEFAULT_FIXED_FONT_FAMILY,
				DEFAULT_FIXED_FONT_STYLE,DEFAULT_FIXED_FONT_SIZE);
	fontserver->Unlock();
	
	
	// Load the GUI colors here and set the global set to the values contained therein. If this
	// is not possible, set colors to the defaults
	if(!LoadGUIColors(&gui_colorset))
		gui_colorset.SetToDefaults();

	InitDecorators();
		
	// Set up the Desktop
	InitDesktop();

	// Create the cursor manager. Object declared in CursorManager.cpp
	cursormanager=new CursorManager();
	cursormanager->SetCursor(B_CURSOR_DEFAULT);
	
	// Create the bitmap allocator. Object declared in BitmapManager.cpp
	bitmapmanager=new BitmapManager();

	// This is necessary to mediate access between the Poller and app_server threads
	_active_lock=create_sem(1,"app_server_active_sem");

	// This locker is for app_server and Picasso to vy for control of the ServerApp list
	_applist_lock=create_sem(1,"app_server_applist_sem");

	// This locker is to mediate access to the make_decorator pointer
	_decor_lock=create_sem(1,"app_server_decor_sem");
	
	// Get the driver first - Poller thread utilizes the thing
	_driver=GetGfxDriver(ActiveScreen());

	// Spawn our input-polling thread
	_poller_id=spawn_thread(PollerThread, "Poller", B_NORMAL_PRIORITY, this);
	if (_poller_id >= 0)
		resume_thread(_poller_id);

	// Spawn our thread-monitoring thread
	_picasso_id = spawn_thread(PicassoThread,"Picasso", B_NORMAL_PRIORITY, this);
	if (_picasso_id >= 0)
		resume_thread(_picasso_id);

	_active_app=-1;
	_p_active_app=NULL;
	decorator_name="Default";
}

/*!
	\brief Destructor
	
	Reached only when the server is asked to shut down in Test mode. Kills all apps, shuts down the 
	desktop, kills the housekeeping threads, etc.
*/
AppServer::~AppServer(void)
{

	ServerApp *tempapp;
	int32 i;
	acquire_sem(_applist_lock);
	for(i=0;i<_applist->CountItems();i++)
	{
		tempapp=(ServerApp *)_applist->ItemAt(i);
		if(tempapp!=NULL)
			delete tempapp;
	}
	delete _applist;
	release_sem(_applist_lock);

	delete bitmapmanager;
	delete cursormanager;

	ShutdownDesktop();

	// If these threads are still running, kill them - after this, if exit_poller
	// is deleted, who knows what will happen... These things will just return an
	// error and fail if the threads have already exited.
	kill_thread(_poller_id);
	kill_thread(_picasso_id);

	delete fontserver;
	
	make_decorator=NULL;
}

/*!
	\brief Thread function for polling and handling input messages
	\param data Pointer to the app_server to which the thread belongs
	\return Throwaway value - always 0
*/
int32 AppServer::PollerThread(void *data)
{
	// This thread handles nothing but input messages for mouse and keyboard
	AppServer *appserver=(AppServer*)data;
	int8 *index;
	PortQueue mousequeue(appserver->_mouseport);
	PortMessage *msg;
	
	for(;;)
	{
		if(!mousequeue.MessagesWaiting())
			mousequeue.GetMessagesFromPort(true);
		else
			mousequeue.GetMessagesFromPort(false);

		msg=mousequeue.GetMessageFromQueue();
		if(!msg)
			continue;
		
		switch(msg->Code())
		{
			// We don't need to do anything with these two, so just pass them
			// onto the active application. Eventually, we will end up passing 
			// them onto the window which is currently under the cursor.
			case B_MOUSE_DOWN:
			case B_MOUSE_UP:
			case B_MOUSE_WHEEL_CHANGED:
			{
				if(!msg->Buffer())
					break;
				ServerWindow::HandleMouseEvent(msg);
				break;
			}
			
			// Process the cursor and then pass it on
			case B_MOUSE_MOVED:
			{
				// Attached data:
				// 1) int64 - time of mouse click
				// 2) float - x coordinate of mouse click
				// 3) float - y coordinate of mouse click
				// 4) int32 - buttons down
				
				index=(int8*)msg->Buffer();
				if(!index)
					break;
				
				// Skip past the message code
				index += sizeof(int32);
				
				// Time sent is not necessary for cursor processing.
				index += sizeof(int64);
				
				float tempx=0,tempy=0;
				tempx=*((float*)index);
				index+=sizeof(float);
				tempy=*((float*)index);
				index+=sizeof(float);
				
				if(appserver->_driver)
				{
					appserver->_driver->MoveCursorTo(tempx,tempy);
					ServerWindow::HandleMouseEvent(msg);
				}
				break;
			}
			case B_KEY_DOWN:
			case B_KEY_UP:
			case B_UNMAPPED_KEY_DOWN:
			case B_UNMAPPED_KEY_UP:
			case B_MODIFIERS_CHANGED:
				appserver->HandleKeyMessage(msg->Code(),(int8*)msg->Buffer());
				break;
			default:
				printf("Server::Poller received unexpected code %lx\n",msg->Code());
				break;
		}
		
		delete msg;
		
		if(appserver->_exit_poller)
			break;
	}
	return 0;
}

/*!
	\brief Thread function for watching for dead apps
	\param data Pointer to the app_server to which the thread belongs
	\return Throwaway value - always 0
*/
int32 AppServer::PicassoThread(void *data)
{
	int32 i;
	AppServer *appserver=(AppServer*)data;
	ServerApp *app;
	for(;;)
	{
		acquire_sem(appserver->_applist_lock);
		for(i=0;i<appserver->_applist->CountItems(); i++)
		{
			app=(ServerApp*)appserver->_applist->ItemAt(i);
			if(!app)
			{
				printf("PANIC: NULL app in app list\n");
				continue;
			}
			app->PingTarget();
		}
		release_sem(appserver->_applist_lock);
		
		// if poller thread has to exit, so do we - I just was too lazy
		// to rename the variable name. ;)
		if(appserver->_exit_poller)
			break;

		// we do this every other second so as not to suck *too* many CPU cycles
		snooze(2000000);
	}
	return 0;
}

/*!
	\brief The call that starts it all...
	\return Always 0
*/
thread_id AppServer::Run(void)
{
	MainLoop();
	return 0;
}


//! Main message-monitoring loop for the regular message port - no input messages!
void AppServer::MainLoop(void)
{
	PortMessage pmsg;
	
	for(;;)
	{
		if(pmsg.ReadFromPort(_messageport)==B_OK)
		{
			switch(pmsg.Code())
			{
				case AS_CREATE_APP:
				case AS_DELETE_APP:
				case AS_GET_SCREEN_MODE:
				case B_QUIT_REQUESTED:
				case AS_UPDATED_CLIENT_FONTLIST:
				case AS_QUERY_FONTS_CHANGED:
				case AS_SET_UI_COLORS:
				case AS_GET_UI_COLOR:
				case AS_SET_DECORATOR:
				case AS_GET_DECORATOR:
				case AS_R5_SET_DECORATOR:
					DispatchMessage(&pmsg);
					break;
				default:
				{
					printf("Server::MainLoop received unexpected code %ld(offset %ld)\n",
							pmsg.Code(),pmsg.Code()-SERVER_TRUE);
					break;
				}
			}

		}

		if(pmsg.Code()==AS_DELETE_APP || (pmsg.Code()==B_QUIT_REQUESTED && DISPLAYDRIVER!=HWDRIVER))
		{
			if(_quitting_server==true && _applist->CountItems()==0)
				break;
		}
	}
	// Make sure our polling thread has exited
	if(find_thread("Poller")!=B_NAME_NOT_FOUND)
		kill_thread(_poller_id);
}

/*!
	\brief Loads the specified decorator and sets the system's decorator to it.
	\param path Path to the decorator to load
	\return True if successful, false if not.
	
	If the server cannot load the specified decorator, nothing changes. Passing a 
	NULL string to this function sets the decorator	to the internal one.
*/
bool AppServer::LoadDecorator(const char *path)
{
	// Loads a window decorator based on the supplied path and forces a decorator update.
	// If it cannot load the specified decorator, it will retain the current one and
	// return false. Note that passing a NULL string to this function sets the decorator
	// to the internal one.

	// passing the string "Default" will set the window decorator to the app_server's
	// internal one
	if(!path)
	{
		make_decorator=NULL;
		return true;
	}
	
	create_decorator *pcreatefunc=NULL;
	status_t stat;
	image_id addon;
	
	addon=load_add_on(path);
	if(addon<0)
		return false;

	// As of now, we do nothing with decorator versions, but the possibility exists
	// that the API will change even though I cannot forsee any reason to do so. If
	// we *did* do anything with decorator versions, the assignment to a global would
	// go here.
		
	// Get the instantiation function
	stat=get_image_symbol(addon, "instantiate_decorator", B_SYMBOL_TYPE_TEXT, (void**)&pcreatefunc);
	if(stat!=B_OK)
	{
		unload_add_on(addon);
		return false;
	}
	
	BPath temppath(path);
	decorator_name=temppath.Leaf();
	
	acquire_sem(_decor_lock);
	make_decorator=pcreatefunc;
	_decorator_id=addon;
	release_sem(_decor_lock);
	return true;
}

//! Loads decorator settings on disk or the default if settings are invalid
void AppServer::InitDecorators(void)
{
	BMessage settings;

	BDirectory dir,newdir;
	if(dir.SetTo(SERVER_SETTINGS_DIR)==B_ENTRY_NOT_FOUND)
		create_directory(SERVER_SETTINGS_DIR,0777);

	BString path(SERVER_SETTINGS_DIR);
	path+="DecoratorSettings";
	BFile file(path.String(),B_READ_ONLY);

	if(file.InitCheck()==B_OK)
	{
		if(settings.Unflatten(&file)==B_OK)
		{
			BString itemtext;
			if(settings.FindString("decorator",&itemtext)==B_OK)
			{
				path.SetTo(DECORATORS_DIR);
				path+=itemtext;
				if(LoadDecorator(path.String()))
					return;
			}
		}
	}

	// We got this far, so something must have gone wrong. We set make_decorator
	// to NULL so that the decorator allocation routine knows to utilize the included
	// default decorator instead of an addon.
	make_decorator=NULL;
}

/*!
	\brief Message handling function for all messages sent to the app_server
	\param code ID of the message sent
	\param buffer Attachement buffer for the message.
	
*/
void AppServer::DispatchMessage(PortMessage *msg)
{
	switch(msg->Code())
	{
		case AS_CREATE_APP:
		{
			// Create the ServerApp to node monitor a new BApplication
			
			// Attached data:
			// 1) port_id - receiver port of a regular app
			// 2) int32 - handler token of the regular app
			// 2) char * - signature of the regular app
			// 3) port_id - port to reply to

			// Find the necessary data
			port_id reply_port;
			port_id app_port;
			int32 htoken;
			char *app_signature;

			msg->Read<int32>(&app_port);
			msg->Read<int32>(&htoken);
			msg->ReadString(&app_signature);
			msg->Read<int32>(&reply_port);
			
			// Create the ServerApp subthread for this app
			acquire_sem(_applist_lock);
			
			port_id r=create_port(DEFAULT_MONITOR_PORT_SIZE,app_signature);
			if(r==B_NO_MORE_PORTS || r==B_BAD_VALUE)
			{
				release_sem(_applist_lock);
				printf("No more ports left. Time to crash. Have a nice day! :)\n");
				break;
			}
			ServerApp *newapp=new ServerApp(app_port,r,htoken,app_signature);
			_applist->AddItem(newapp);
			
			release_sem(_applist_lock);

			acquire_sem(_active_lock);
			_p_active_app=newapp;
			_active_app=_applist->CountItems()-1;

			PortLink replylink(reply_port);
			replylink.SetOpCode(AS_SET_SERVER_PORT);
			replylink.Attach<int32>(newapp->_receiver);
			replylink.Flush();

			// This is necessary because PortLink::ReadString allocates memory
			delete app_signature;
			
			release_sem(_active_lock);
			
			newapp->Run();
			break;
		}
		case AS_DELETE_APP:
		{
			// Delete a ServerApp. Received only from the respective ServerApp when a
			// BApplication asks it to quit.
			
			// Attached Data:
			// 1) thread_id - thread ID of the ServerApp to be deleted
			
			int32 i, appnum=_applist->CountItems();
			ServerApp *srvapp;
			thread_id srvapp_id;
			msg->Read<thread_id>(&srvapp_id);
			
			// Run through the list of apps and nuke the proper one
			for(i=0;i<appnum;i++)
			{
				srvapp=(ServerApp *)_applist->ItemAt(i);

				if(srvapp!=NULL && srvapp->_monitor_thread==srvapp_id)
				{
					acquire_sem(_applist_lock);
					srvapp=(ServerApp *)_applist->RemoveItem(i);
					if(srvapp)
					{
						delete srvapp;
						srvapp=NULL;
					}
					release_sem(_applist_lock);
					acquire_sem(_active_lock);

					if(_applist->CountItems()==0)
					{
						// active==-1 signifies that no other apps are running - NOT good
						_active_app=-1;
					}
					else
					{
						// we actually still have apps running, so make a new one active
						if(_active_app>0)
							_active_app--;
						else
							_active_app=0;
					}
					_p_active_app=(_active_app>-1)?(ServerApp*)_applist->ItemAt(_active_app):NULL;
					release_sem(_active_lock);
					break;	// jump out of our for() loop
				}

			}
			break;
		}
		case AS_UPDATED_CLIENT_FONTLIST:
		{
			// received when the client-side global font list has been
			// refreshed
			fontserver->Lock();
			fontserver->FontsUpdated();
			fontserver->Unlock();
			break;
		}
		case AS_QUERY_FONTS_CHANGED:
		{
			// Client application is asking if the font list has changed since
			// the last client-side refresh

			// Attached data:
			// 1) port_id reply port
			
			fontserver->Lock();
			bool needs_update=fontserver->FontsNeedUpdated();
			fontserver->Unlock();
			
			// Seeing how the client merely wants an answer, we'll skip the PortLink
			// and all its overhead and just write the code to port.
			port_id replyport;
			msg->Read<port_id>(&replyport);
			
			write_port(replyport, (needs_update)?SERVER_TRUE:SERVER_FALSE, NULL,0);
			break;
		}
		case AS_SET_UI_COLORS:
		{
			// Client application is asking to set all the system colors at once
			// using a ColorSet object
			
			// Attached data:
			// 1) ColorSet new colors to use
			
			gui_colorset.Lock();
			msg->Read<ColorSet>(&gui_colorset);
			gui_colorset.Unlock();
			Broadcast(AS_UPDATE_COLORS);
			break;
		}
		case AS_SET_DECORATOR:
		{
			// Received from an application when the user wants to set the window
			// decorator to a new one
			
			// Attached Data:
			// char * name of the decorator in the decorators path to use
			
			char *decname;
			msg->ReadString(&decname);
			if(decname)
			{
				if(strcmp(decname,"Default")!=0)
				{
					BString decpath;
					decpath.SetTo(DECORATORS_DIR);
					decpath+=decname;
					if(LoadDecorator(decpath.String()))
						Broadcast(AS_UPDATE_DECORATOR);
				}
				else
				{
					LoadDecorator(NULL);
					Broadcast(AS_UPDATE_DECORATOR);
				}
			}
			delete decname;
			
			break;
		}
		case AS_GET_DECORATOR:
		{
			// TODO: get window decorator's name and return it to the sender
			// Attached Data:
			// 1) port_id reply port
			
			port_id replyport;
			msg->Read<port_id>(&replyport);
			PortLink replylink(replyport);
			replylink.SetOpCode(AS_GET_DECORATOR);
			replylink.AttachString(decorator_name.String());
			replylink.Flush();
			break;
		}
		case AS_R5_SET_DECORATOR:
		{
			// Sort of supports Tracker's nifty Easter Egg. It was easy to do and 
			// it's kind of neat, so why not?
			
			// Attached Data:
			// char * name of the decorator in the decorators path to use
			printf("AppServer::AS_R5_SET_DECORATOR unimplemented\n");
			
			int32 decindex;
			msg->Read<int32>(&decindex);
			
			BString decpath;
			decpath.SetTo(DECORATORS_DIR);
			switch(decindex)
			{
				case 0: decpath+="BeOS"; break;
				case 1: decpath+="AmigaOS"; break;
				case 2: decpath+="Windows"; break;
				case 3: decpath+="MacOS"; break;
				default:
					break;
			}
			if(LoadDecorator(decpath.String()))
				Broadcast(AS_UPDATE_DECORATOR);

			break;
		}
		case AS_GET_SCREEN_MODE:
		{
			// Synchronous message call to get the stats on the current screen mode
			// in the app_server. Simply a hack in place for the Input Server until
			// BScreens are done.
			
			// Attached Data:
			// 1) port_id - port to reply to
			
			// Returned Data:
			// 1) int32 width
			// 2) int32 height
			// 3) int depth
			
			port_id replyport;
			msg->Read<port_id>(&replyport);
			
			PortLink replylink(replyport);
			replylink.SetOpCode(AS_GET_SCREEN_MODE);
			replylink.Attach<int16>(_driver->GetWidth());
			replylink.Attach<int16>(_driver->GetHeight());
			replylink.Attach<int16>(_driver->GetDepth());
			replylink.Flush();
			break;
		}
		case B_QUIT_REQUESTED:
		{
			// Attached Data:
			// none
			
			// We've been asked to quit, so (for now) broadcast to all
			// test apps to quit. This situation will occur only when the server
			// is compiled as a regular Be application.
			if(DISPLAYDRIVER==HWDRIVER)
				break;
			
			Broadcast(AS_QUIT_APP);

			// So when we delete the last ServerApp, we can exit the server
			_quitting_server=true;
			_exit_poller=true;
			break;
		}
		case AS_SET_SYSCURSOR_DEFAULTS:
		{
			cursormanager->SetDefaults();
			break;
		}
		default:
			// we should never get here.
			break;
	}
}

/*!
	\brief Send a quick (no attachments) message to all applications
	
	Quite useful for notification for things like server shutdown, system 
	color changes, etc.
*/
void AppServer::Broadcast(int32 code)
{
	int32 i;
	ServerApp *app;
	
	acquire_sem(_applist_lock);
	for(i=0;i<_applist->CountItems(); i++)
	{
		app=(ServerApp*)_applist->ItemAt(i);
		if(!app)
			continue;
		app->PostMessage(code, sizeof(int32), (int8*)&code);
	}
	release_sem(_applist_lock);
}

/*!
	\brief Handles all incoming key messages
	\param code ID code of the key message
	\param buffer Attachment buffer of the keyboard message. Always non-NULL.
	
	This function handles 5 message codes: B_KEY_UP, B_KEY_DOWN, 
	B_UNMAPPED_KEY_UP, B_UNMAPPED_KEY_DOWN, and B_MODIFIERS_CHANGED. It filters
	out any combinations which are app_server specific:
	# Left Ctrl+Alt+Shift+F12 - set to 640x480x256@60Hz
	# Alt + Function Key - Change workspace
	# Control+Tab - Send to Deskbar for the Twitcher
*/
void AppServer::HandleKeyMessage(int32 code, int8 *buffer)
{
	int8 *index=buffer;
	
	switch(code)
	{
		case B_KEY_DOWN:
		{
			// Attached Data:
			// 1) int64 bigtime_t object of when the message was sent
			// 2) int32 raw key code (scancode)
			// 3) int32 modifier-independent ASCII code for the character
			// 4) int32 repeat count
			// 5) int32 modifiers
			// 6) int8[3] UTF-8 data generated
			// 7) int8 number of bytes to follow containing the 
			//		generated string
			// 8) Character string generated by the keystroke
			// 9) int8[16] state of all keys
			
			// Obtain only what data which we'll need
			index+=sizeof(int64);
			int32 scancode=*((int32*)index); index+=sizeof(int32) * 3;
			int32 modifiers=*((int32*)index); index+=sizeof(int32) + (sizeof(int8) * 3);
			int8 stringlength=*index; index+=stringlength;
			STRACE(("Key Down: 0x%lx\n",scancode));
			if(DISPLAYDRIVER == HWDRIVER)
			{
				// Check for workspace change or safe video mode
				if(scancode>0x01 && scancode<0x0e)
				{
					if(scancode==0x0d)
					{
						if(modifiers & (B_LEFT_COMMAND_KEY |
							B_LEFT_CONTROL_KEY | B_LEFT_SHIFT_KEY))
						{
							// TODO: Set to Safe Mode here. (DisplayDriver API change)
							STRACE(("Safe Video Mode invoked - code unimplemented\n"));
							break;
						}
					}
				}
			
				if(modifiers & B_CONTROL_KEY)
				{
					STRACE(("Set Workspace %ld\n",scancode-1));
					SetWorkspace(scancode-2);
					break;
				}	

				// Tab key
				if(scancode==0x26 && (modifiers & B_CONTROL_KEY))
				{
					ServerApp *deskbar=FindApp("application/x-vnd.Be-TSKB");
					if(deskbar)
					{
						printf("Send Twitcher message key to Deskbar - unimplmemented\n");
						break;
					}
				}

				// PrintScreen
				if(scancode==0xe)
				{
					if(_driver)
					{
						char filename[128];
						BEntry entry;
						
						sprintf(filename,"/boot/home/screen%ld.png",_ssindex);
						entry.SetTo(filename);
						
						while(entry.Exists())
						{
							_ssindex++;
							sprintf(filename,"/boot/home/screen%ld.png",_ssindex);
						}
						_ssindex++;
						_driver->DumpToFile(filename);
						break;
					}
				}
			}
			else
			{
				// F12
				if(scancode>0x1 && scancode<0xe)
				{
					if(scancode==0xd)
					{
						if(modifiers & (B_LEFT_CONTROL_KEY | B_LEFT_SHIFT_KEY | B_LEFT_OPTION_KEY))
						{
							// TODO: Set to Safe Mode here. (DisplayDriver API change)
							STRACE(("Safe Video Mode invoked - code unimplemented\n"));
							break;
						}
					}
					if(modifiers & (B_LEFT_SHIFT_KEY | B_LEFT_CONTROL_KEY))
					{
						STRACE(("Set Workspace %ld\n",scancode-1));
						SetWorkspace(scancode-2);
						break;
					}	
				}
				
				//Tab
				if(scancode==0x26 && (modifiers & B_SHIFT_KEY))
				{
					STRACE(("Twitcher\n"));
					ServerApp *deskbar=FindApp("application/x-vnd.Be-TSKB");
					if(deskbar)
					{
						printf("Send Twitcher message key to Deskbar - unimplmemented\n");
						break;
					}
				}

				// Pause/Break
				if(scancode==0x7f)
				{
					if(_driver)
					{
						char filename[128];
						BEntry entry;
						
						sprintf(filename,"/boot/home/screen%ld.png",_ssindex);
						entry.SetTo(filename);
						
						while(entry.Exists())
						{
							_ssindex++;
							sprintf(filename,"/boot/home/screen%ld.png",_ssindex);
						}
						_ssindex++;

						_driver->DumpToFile(filename);
						break;
					}
				}
			}
			
			// We got this far, so apparently it's safe to pass to the active
			// window.
			ServerWindow::HandleKeyEvent(code, buffer);
			break;
		}
		case B_KEY_UP:
		{
			// Attached Data:
			// 1) int64 bigtime_t object of when the message was sent
			// 2) int32 raw key code (scancode)
			// 3) int32 modifier-independent ASCII code for the character
			// 4) int32 modifiers
			// 5) int8[3] UTF-8 data generated
			// 6) int8 number of bytes to follow containing the 
			//		generated string
			// 7) Character string generated by the keystroke
			// 8) int8[16] state of all keys
			
			// Obtain only what data which we'll need
			index+=sizeof(int64);
			int32 scancode=*((int32*)index); index+=sizeof(int32) * 3;
			int32 modifiers=*((int32*)index); index+=sizeof(int8) * 3;
			int8 stringlength=*index; index+=stringlength + sizeof(int8);
			STRACE(("Key Up: 0x%lx\n",scancode));
			
			if(DISPLAYDRIVER==HWDRIVER)
			{
				// Tab key
				if(scancode==0x26 && (modifiers & B_CONTROL_KEY))
				{
					ServerApp *deskbar=FindApp("application/x-vnd.Be-TSKB");
					if(deskbar)
					{
						printf("Send Twitcher message key to Deskbar - unimplmemented\n");
						break;
					}
				}
			}
			else
			{
				if(scancode==0x26 && (modifiers & B_LEFT_SHIFT_KEY))
				{
					ServerApp *deskbar=FindApp("application/x-vnd.Be-TSKB");
					if(deskbar)
					{
						printf("Send Twitcher message key to Deskbar - unimplmemented\n");
						break;
					}
				}
			}

			// We got this far, so apparently it's safe to pass to the active
			// window.
			
			// fall through to the next case
			ServerWindow::HandleKeyEvent(code,buffer);
			break;
		}
		case B_UNMAPPED_KEY_DOWN:
		{
			// Attached Data:
			// 1) int64 bigtime_t object of when the message was sent
			// 2) int32 raw key code (scancode)
			// 3) int32 modifiers
			// 4) int32 number of elements in the key state array to follow
			// 5) int8 state of all keys

			// fall through to the next case
#ifdef DEBUG_KEYHANDLING
index+=sizeof(bigtime_t);
printf("Unmapped Key Down: 0x%lx\n",*((int32*)index));
#endif
			ServerWindow::HandleKeyEvent(code,buffer);
			break;
		}
		case B_UNMAPPED_KEY_UP:
		{
			// Attached Data:
			// 1) int64 bigtime_t object of when the message was sent
			// 2) int32 raw key code (scancode)
			// 3) int32 modifiers
			// 4) int32 number of elements in the key state array to follow
			// 5) int8 state of all keys

			// fall through to the next case
#ifdef DEBUG_KEYHANDLING
index+=sizeof(bigtime_t);
printf("Unmapped Key Up: 0x%lx\n",*((int32*)index));
#endif
			ServerWindow::HandleKeyEvent(code,buffer);
			break;
		}
		case B_MODIFIERS_CHANGED:
		{
			// Attached Data:
			// 1) int64 bigtime_t object of when the message was sent
			// 2) int32 modifiers
			// 3) int32 old modifiers
			// 4) int32 number of elements in the key state array to follow
			// 5) int8 state of all keys
			
#ifdef DEBUG_KEYHANDLING
index+=sizeof(bigtime_t);
printf("Modifiers Changed\n");
#endif
			ServerWindow::HandleKeyEvent(code,buffer);
			break;
		}
		default:
			break;
	}
}

/*!
	\brief Finds the application with the given signature
	\param sig MIME signature of the application to find
	\return the corresponding ServerApp or NULL if not found
	
	This call should be made only when necessary because it locks the app list 
	while it does its searching.
*/
ServerApp *AppServer::FindApp(const char *sig)
{
	if(!sig)
		return NULL;

	ServerApp *foundapp=NULL;

	acquire_sem(_applist_lock);

	for(int32 i=0; i<_applist->CountItems();i++)
	{
		foundapp=(ServerApp*)_applist->ItemAt(i);
		if(foundapp && foundapp->_signature==sig)
		{
			release_sem(_applist_lock);
			return foundapp;
		}
	}

	release_sem(_applist_lock);
	
	// couldn't find a match
	return NULL;
}

/*!
	\brief Creates a new decorator instance
	\param rect Frame size
	\param title Title string for the "window"
	\param wlook Window look type. See Window.h
	\param wfeel Window feel type. See Window.h
	\param wflags Window flags. See Window.h
	
	If a decorator has not been set, we use the default one packaged in with the app_server 
	being that we can't do anything with a window without one.
*/
Decorator *new_decorator(BRect rect, const char *title, int32 wlook, int32 wfeel,
	int32 wflags, DisplayDriver *ddriver)
{
	Decorator *dec=NULL;
	if(!app_server->make_decorator)
		dec=new DefaultDecorator(rect,wlook,wfeel,wflags);
	else
		dec=app_server->make_decorator(rect,wlook,wfeel,wflags);

	gui_colorset.Lock();
	dec->SetDriver(ddriver);
	dec->SetColors(gui_colorset);
	dec->SetFont(fontserver->GetSystemPlain());
	dec->SetTitle(title);
	gui_colorset.Unlock();
	
	return dec;
}

/*!
	\brief Entry function to run the entire server
	\param argc Number of command-line arguments present
	\param argv String array of the command-line arguments
	\return -1 if the app_server is already running, 0 if everything's OK.
*/
int main( int argc, char** argv )
{
	// There can be only one....
	if(find_port(SERVER_PORT_NAME)!=B_NAME_NOT_FOUND)
		return -1;

	app_server=new AppServer();
	app_server->Run();
	delete app_server;
	return 0;
}
