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
#include "AppServer.h"
#include "Desktop.h"
#include "DisplayDriver.h"
#include "PortLink.h"
#include "ServerApp.h"
#include "ServerCursor.h"
#include "ServerProtocol.h"
#include "ServerWindow.h"
#include "DefaultDecorator.h"
#include "RGBColor.h"
#include "BitmapManager.h"
#include "CursorManager.h"

// Globals

//! Used to access the app_server from new_decorator
AppServer *app_server=NULL;

//! Default background color for workspaces
RGBColor workspace_default_color(51,102,160);

/*!
	\brief Constructor
	
	This loads the default fonts, allocates all the major global variables, spawns the main housekeeping
	threads, loads user preferences for the UI and decorator, and allocates various locks.
*/
AppServer::AppServer(void)
#if DISPLAYDRIVER != HWDRIVER
 : BApplication (SERVER_SIGNATURE)
#endif
{
	_mouseport=create_port(100,SERVER_INPUT_PORT);
	_messageport=create_port(100,SERVER_PORT_NAME);

	_applist=new BList(0);
	_quitting_server=false;
	_exit_poller=false;

	// Create the font server and scan the proper directories.
	fontserver=new FontServer;
	fontserver->Lock();
	fontserver->ScanDirectory("/boot/beos/etc/fonts/ttfonts/");

	// TODO: Uncomment when actually put to use. Commented out for speed
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

	// Set up the Desktop
	InitDesktop();

	// Create the cursor manager. Object declared in CursorManager.cpp
	cursormanager=new CursorManager();

	// Default cursor data stored in CursorData.cpp
	
	// TODO: fix the cursor display
	extern int8 default_cursor_data[];
	ServerCursor *sc=new ServerCursor(default_cursor_data);

	cursormanager->AddCursor(sc);
	cursormanager->ChangeCursor(B_CURSOR_DEFAULT, sc->ID());
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
	_driver=GetGfxDriver();

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

	make_decorator=NULL;
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
	int32 msgcode;
	int8 *msgbuffer=NULL,*index;
	ssize_t buffersize,bytesread;
	
	for(;;)
	{
		buffersize=port_buffer_size(appserver->_mouseport);
		if(buffersize>0)
			msgbuffer=new int8[buffersize];
		bytesread=read_port(appserver->_mouseport,&msgcode,msgbuffer,buffersize);

		if (bytesread != B_BAD_PORT_ID && bytesread != B_TIMED_OUT && bytesread != B_WOULD_BLOCK)
		{
			switch(msgcode)
			{
				// We don't need to do anything with these two, so just pass them
				// onto the active application. Eventually, we will end up passing 
				// them onto the window which is currently under the cursor.
				case B_MOUSE_DOWN:
				case B_MOUSE_UP:
				case B_MOUSE_WHEEL_CHANGED:
				{
					if(!msgbuffer)
						break;
					ServerWindow::HandleMouseEvent(msgcode,msgbuffer);
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
					index=msgbuffer;
					if(!index)
						break;

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
						ServerWindow::HandleMouseEvent(msgcode,msgbuffer);
					}
					break;
				}
				case B_KEY_DOWN:
				case B_KEY_UP:
				case B_UNMAPPED_KEY_DOWN:
				case B_UNMAPPED_KEY_UP:
					appserver->HandleKeyMessage(msgcode,msgbuffer);
					break;
				default:
					printf("Server::Poller received unexpected code %lx\n",msgcode);
					break;
			}

		}

		if(buffersize>0)
			delete msgbuffer;

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
	int32 msgcode;
	int8 *msgbuffer=NULL;
	ssize_t buffersize,bytesread;
	
	for(;;)
	{
		buffersize=port_buffer_size(_messageport);
		if(buffersize>0)
			msgbuffer=new int8[buffersize];
		bytesread=read_port(_messageport,&msgcode,msgbuffer,buffersize);

		if (bytesread != B_BAD_PORT_ID && bytesread != B_TIMED_OUT && bytesread != B_WOULD_BLOCK)
		{
			switch(msgcode)
			{
				case AS_CREATE_APP:
				case AS_DELETE_APP:
				case AS_GET_SCREEN_MODE:
				case B_QUIT_REQUESTED:
				case AS_UPDATED_CLIENT_FONTLIST:
				case AS_QUERY_FONTS_CHANGED:
					DispatchMessage(msgcode,msgbuffer);
					break;
				default:
				{
					printf("Server::MainLoop received unexpected code %ld\n",msgcode);
					break;
				}
			}

		}

		if(buffersize>0)
			delete msgbuffer;
		if(msgcode==AS_DELETE_APP || msgcode==B_QUIT_REQUESTED && DISPLAYDRIVER!=HWDRIVER)
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
	
	If the server cannot load the specified decorator, nothing changes.
*/
bool AppServer::LoadDecorator(const char *path)
{
	// Loads a window decorator based on the supplied path and forces a decorator update.
	// If it cannot load the specified decorator, it will retain the current one and
	// return false.
	
	create_decorator *pcreatefunc=NULL;
	get_version *pversionfunc=NULL;
	status_t stat;
	image_id addon;
	
	addon=load_add_on(path);

	// As of now, we do nothing with decorator versions, but the possibility exists
	// that the API will change even though I cannot forsee any reason to do so. If
	// we *did* do anything with decorator versions, the assignment to a global would
	// go here.
		
	// Get the instantiation function
	stat=get_image_symbol(addon, "instantiate_decorator", B_SYMBOL_TYPE_TEXT, (void**)&pversionfunc);
	if(stat!=B_OK)
	{
		unload_add_on(addon);
		return false;
	}
	acquire_sem(_decor_lock);
	make_decorator=pcreatefunc;
	_decorator_id=addon;
	release_sem(_decor_lock);
	return true;
}

/*!
	\brief Message handling function for all messages sent to the app_server
	\param code ID of the message sent
	\param buffer Attachement buffer for the message.
	
*/
void AppServer::DispatchMessage(int32 code, int8 *buffer)
{
	int8 *index=buffer;
	switch(code)
	{
		case AS_CREATE_APP:
		{
			// Create the ServerApp to node monitor a new BApplication
			
			// Attached data:
			// 1) port_id - port to reply to
			// 2) port_id - receiver port of a regular app
			// 3) char * - signature of the regular app

			// Find the necessary data
			port_id reply_port=*((port_id*)index); index+=sizeof(port_id);
			port_id app_port=*((port_id*)index); index+=sizeof(port_id);
			int32 htoken=*((int32*)index); index+=sizeof(int32);
			char *app_signature=(char *)index;
			
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

			PortLink *replylink=new PortLink(reply_port);
			replylink->SetOpCode(AS_SET_SERVER_PORT);
			replylink->Attach((int32)newapp->_receiver);
			replylink->Flush();

			delete replylink;

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
			thread_id srvapp_id=*((thread_id*)buffer);
			
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
			
			PortLink *pl=new PortLink(*((port_id*)index));
			pl->SetOpCode( (needs_update)?SERVER_TRUE:SERVER_FALSE );
			pl->Flush();
			delete pl;
			
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
			
			PortLink *replylink=new PortLink(*((port_id*)index));
			replylink->SetOpCode(AS_GET_SCREEN_MODE);
			replylink->Attach((int16)_driver->GetWidth());
			replylink->Attach((int16)_driver->GetHeight());
			replylink->Attach((int16)_driver->GetDepth());
			replylink->Flush();
			delete replylink;
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
		app->PostMessage(code);
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
			// 9) int32 number of elements in the key state array to follow
			// 10) int8 state of all keys
			
			// Obtain only what data which we'll need
			index+=sizeof(int64);
			int32 scancode=*((int32*)index); index+=sizeof(int32) * 3;
			int32 modifiers=*((int32*)index); index+=sizeof(int8) * 3;
			int8 stringlength=*index; index+=stringlength + sizeof(int8);
			int32 keyindices=*index;
			
			
			// Check for workspace change or safe video mode
			if(scancode>0x01 && scancode<0x0e)
			{
				// F12
				if(scancode==0x0d)
				{
					if(modifiers & B_LEFT_COMMAND_KEY &
						B_LEFT_CONTROL_KEY & B_LEFT_SHIFT_KEY)
					{
						// TODO: Set to Safe Mode here. (DisplayDriver API change)
					}
				}
			
				if(modifiers & B_COMMAND_KEY)
					SetWorkspace(scancode-2);
				
				// Tab key
				if(scancode==0x26 && (modifiers & B_CONTROL_KEY))
				{
					ServerApp *deskbar=FindApp("application/x-vnd.Be-TSKB");
					if(deskbar)
					{
						// I hope this calculation's right ;)
						size_t msgsize=sizeof(int64) + ( sizeof(int32) * 5) +
								( sizeof(int8) * (4+ stringlength + keyindices));
						deskbar->PostMessage(code, msgsize, buffer);
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
			// 8) int32 number of elements in the key state array to follow
			// 9) int8 state of all keys
			
			// Obtain only what data which we'll need
			index+=sizeof(int64);
			int32 scancode=*((int32*)index); index+=sizeof(int32) * 3;
			int32 modifiers=*((int32*)index); index+=sizeof(int8) * 3;
			int8 stringlength=*index; index+=stringlength + sizeof(int8);
			int32 keyindices=*index;
			
			// Send the key_up msg to the Deskbar for the twitcher
			
			// Tab key
			if(scancode==0x26 && (modifiers & B_CONTROL_KEY))
			{
				ServerApp *deskbar=FindApp("application/x-vnd.Be-TSKB");
				if(deskbar)
				{
					// I hope this calculation's right ;)
					size_t msgsize=sizeof(int64) + ( sizeof(int32) * 4) +
							( sizeof(int8) * (4+ stringlength + keyindices));
					deskbar->PostMessage(code, msgsize, buffer);
					break;
				}
			}

			// We got this far, so apparently it's safe to pass to the active
			// window.
			
			// fall through to the next case
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
		}
		case B_MODIFIERS_CHANGED:
		{
			// Attached Data:
			// 1) int64 bigtime_t object of when the message was sent
			// 2) int32 modifiers
			// 3) int32 old modifiers
			// 4) int32 number of elements in the key state array to follow
			// 5) int8 state of all keys
			
			ServerWindow::HandleMouseEvent(code,buffer);
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
	if(!app_server->make_decorator)
		return new DefaultDecorator(rect,wlook,wfeel,wflags);
	
	return app_server->make_decorator(rect,wlook,wfeel,wflags);
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
