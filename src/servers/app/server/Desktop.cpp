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
//	File Name:		Desktop.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Functions used to work with workspaces and general desktop stuff
//  
//------------------------------------------------------------------------------
#include "DisplayDriver.h"
#include "Desktop.h"
#include "DesktopClasses.h"
#include "ServerConfig.h"

#include "ViewDriver.h"
#include "ServerWindow.h"

#if DISPLAYDRIVER == SCREENDRIVER
#include "ScreenDriver.h"
#endif

#if DISPLAYDRIVER == HWDRIVER
#include "AccelerantDriver.h"
#endif

//#define DEBUG_DESKTOP
#ifdef DEBUG_DESKTOP
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

bool ReadOBOSWorkspaceData(void);
bool ReadR5WorkspaceData(void);
void SaveWorkspaceData(void);

//! This namespace encapsulates all globals specifically for the desktop
namespace desktop_private {
	int8 *dragmessage=NULL;
	int32 dragmessagesize=0;
	BLocker draglock,
			layerlock,
			workspacelock,
			optlock;
	BList *screenlist=NULL;
	Screen *activescreen=NULL;
	scroll_bar_info scrollbarinfo;
	menu_info menuinfo;
	mode_mouse ffm_mode;
	bool ffm;
}

//! locks layer access
void lock_layers(void) { desktop_private::layerlock.Lock(); }
//! unlocks layer access
void unlock_layers(void) { desktop_private::layerlock.Unlock(); }
//! locks access to drag and drop data
void lock_dragdata(void) { desktop_private::draglock.Lock(); }
//! unlocks access to drag and drop data
void unlock_dragdata(void) { desktop_private::draglock.Unlock(); }
//! locks access to the workspace list
void lock_workspaces(void) { desktop_private::workspacelock.Lock(); }
//! unlocks access to the workspace list
void unlock_workspaces(void) { desktop_private::workspacelock.Unlock(); }

/*!
	\brief Initializes the desktop for use.
	
	InitDesktop creates all workspaces, starts up the appropriate DisplayDriver, and 
	generally gets everything ready for the server to use.
*/
void InitDesktop(void)
{
STRACE(("Desktop: InitWorkspace\n"));
	desktop_private::screenlist=new BList(0);
	DisplayDriver *tdriver;
	Screen *s=NULL;
	
#ifndef TEST_MODE
	// If we're using the AccelerantDriver for rendering, eventually we will loop through
	// drivers until one can't initialize in order to support multiple monitors. For now,
	// we'll just load one and be done with it.
	tdriver=new AccelerantDriver;

	if(tdriver->Initialize())
	{
		// TODO: figure out how to load the workspace data and do it here.
		// For now, we'll just use 3 workspaces.
		
		s=new Screen(tdriver,3);
		desktop_private::screenlist->AddItem(s);
	}
	else
	{
		// Uh-oh. We don't have a video card. This would be a *bad* thing. Seeing how
		// we can't see anything, we can't do anything, either. It's time to crash now. ;)
		tdriver->Shutdown();
		delete tdriver;
		tdriver=NULL;
STRACE(("\t NULL display driver. OK. We crash now. :P\n"));
	}
#else // end if not TEST_MODE

#if DISPLAYDRIVER == SCREENDRIVER
	tdriver=new ScreenDriver;
#else
	tdriver=new ViewDriver;
#endif

	if(tdriver->Initialize())
	{
		// TODO: figure out how to load the workspace data and do it here.
		// For now, we'll just use 3 workspaces.
		
		s=new Screen(tdriver,3);
		desktop_private::screenlist->AddItem(s);
	}
	else
	{
		// Uh-oh. We don't have a video card. This would be a *bad* thing. Seeing how
		// we can't see anything, we can't do anything, either. It's time to crash now. ;)
		tdriver->Shutdown();
		delete tdriver;
		tdriver=NULL;
STRACE(("\t NULL display driver. OK. We crash now. :P\n"));
	}
#endif
	
	// Once we utilize multiple monitors, we'll make Screen 0 the active one.
	if(tdriver)
	{
		s->Activate();
		desktop_private::activescreen=s;
#ifdef TEST_MODE
		s->SetSpace(0,B_32_BIT_640x480,true);
#endif
	}
}

//! Shuts down the graphics subsystem
void ShutdownDesktop(void)
{
STRACE(("Desktop: ShutdownDesktop\n"));
	Screen *s;
	
	for(int32 i=0;i<desktop_private::screenlist->CountItems();i++)
	{
		s=(Screen*)desktop_private::screenlist->ItemAt(i);
		if(s)
			delete s;
	}
	if(!desktop_private::dragmessage)
	{
		delete desktop_private::dragmessage;
		desktop_private::dragmessage=NULL;
		desktop_private::dragmessagesize=0;
	}
}

/*!
	\brief Add a workspace to the desktop
	\param index Place in the workspace list to add the new one.
	
	The default location to add a workspace is at the end of the list. If index 
	is less than 0 or greater than the current workspace count, AddWorkspace will place 
	the workspace at the list's end. Note that it is possible to insert a workspace 
	in the list simply by specifying an index. 
*/
void AddWorkspace(int32 index)
{
STRACE(("Desktop: AddWorkspace(%ld)\n",index+1));
	lock_workspaces();
	lock_layers();
	
	Screen *s;
	
	for(int32 i=0;i<desktop_private::screenlist->CountItems();i++)
	{
		s=(Screen*)desktop_private::screenlist->ItemAt(i);
		if(s)
			s->AddWorkspace(index);
	}
	
	unlock_layers();
	unlock_workspaces();
}

/*!
	\brief Deletes a workspace
	\param index The index to delete
	
	If the workspace specified by index does not refer to a valid workspace, this function 
	will fail. Should the workspace to be deleted be the active one, the previous 
	workspace in the list will be chosen or workspace 0 if workspace 0 is being deleted. 
	This function will fail if there is only 1 workspace used.
*/
void DeleteWorkspace(int32 index)
{
STRACE(("Desktop: DeleteWorkspace(%ld)\n",index+1));
	lock_workspaces();
	lock_layers();
	
	Screen *s;
	
	for(int32 i=0;i<desktop_private::screenlist->CountItems();i++)
	{
		s=(Screen*)desktop_private::screenlist->ItemAt(i);
		if(s)
			s->DeleteWorkspace(index);
	}
	
	unlock_layers();
	unlock_workspaces();
}

/*!
	\brief Returns the number of workspaces available
	\return the number of workspaces available
*/
int32 CountWorkspaces(void)
{
	lock_workspaces();
	lock_layers();
	
	int32 count=desktop_private::activescreen->CountWorkspaces();

	unlock_layers();
	unlock_workspaces();

	return count;
}

/*!
	\brief Sets the number of workspaces available
	\param count The new number of available workspaces
	
	This function will fail if count is less than 1 or greater than 32. The limit of 32 was 
	set by BeOS. Making the limit higher will cause unreachable workspaces because of the 
	API interface used to access them. Were this to change, the only limiting factor for 
	workspace count would be the hardware itself.
	
*/
void SetWorkspaceCount(int32 count)
{
STRACE(("Desktop: SetWorkspaceCount(%ld)\n",count));
	if(count<1 || count>32)
		return;
	lock_workspaces();
	Screen *scr;
	for(int32 i=0;i<desktop_private::screenlist->CountItems();i++)
	{
		scr=(Screen*)desktop_private::screenlist->ItemAt(i);
		if(!scr)
			continue;
		scr->SetWorkspaceCount(count);
	}
	unlock_workspaces();
}

/*!
	\brief Returns the index of the active workspace
	\return the index of the active workspace or -1 if an internal error occurred.
*/
int32 CurrentWorkspace(void)
{
	lock_workspaces();
	int32 id=-1;

	if(desktop_private::activescreen)
		id=desktop_private::activescreen->CurrentWorkspace();
	unlock_workspaces();

	return id;
}

/*!
	\brief Returns the workspace object for the active screen at the given index
*/
Workspace *WorkspaceAt(int32 index)
{
	lock_workspaces();
	Workspace *w=NULL;
	if(desktop_private::activescreen)
		w=desktop_private::activescreen->GetWorkspace(index);
	unlock_workspaces();
	return w;
}

/*!
	\brief Sets the active workspace
	\param workspace Index of the workspace to activate
	
	This function will do nothing if passed an index which does not refer to a valid 
	workspace.
*/
void SetWorkspace(int32 workspace)
{
STRACE(("Desktop: SetWorkspace(%ld)\n",workspace+1));
	lock_workspaces();
	if(workspace<0 || workspace>(CountWorkspaces()-1))
	{
		unlock_workspaces();
		return;
	}

	Screen *scr;
	for(int32 i=0;i<desktop_private::screenlist->CountItems();i++)
	{
		scr=(Screen*)desktop_private::screenlist->ItemAt(i);
		if(!scr)
			continue;
		scr->SetWorkspace(workspace);
	}
	unlock_workspaces();
}

/*!
	\brief Sets the active screen. Currently unused.
	\param id ID of the screen to activate.
*/
void SetScreen(screen_id id)
{
STRACE(("Desktop: SetScreen(%ld)\n",id.id));
	Screen *scr;
	for(int32 i=0;i<desktop_private::screenlist->CountItems();i++)
	{
		scr=(Screen*)desktop_private::screenlist->ItemAt(i);
		if(!scr)
			continue;
		if(scr->GetID().id==id.id)
		{
			desktop_private::activescreen->Activate(false);
			desktop_private::activescreen=scr;
			desktop_private::activescreen->Activate(true);
		}
	}
}

/*!
	\brief Returns the number of screens available. Currently unused.
	\return the number of screens available.
*/
int32 CountScreens(void)
{
	return desktop_private::screenlist->CountItems();;
}

/*!
	\brief Returns the index of the active screen. Currently unused.
	\return the index of the active screen
*/
screen_id ActiveScreen(void)
{
	return desktop_private::activescreen->GetID();
}

/*!
	\brief Returns the active screen.
	\return The currently active screen
*/
Screen *GetActiveScreen(void)
{
	return desktop_private::activescreen;
}

/*!
	\brief Obtains the DisplayDriver object used for the specified screen.
	\param screen ID of the screen to get the DisplayDriver for.
	\return The DisplayDriver or NULL if not available for that screen ID
	
	Like the other multiscreen functions, this is unused. Because multiple screens are 
	not utilized, specifying an ID other than B_MAIN_SCREEN_ID will return NULL 
*/
DisplayDriver *GetGfxDriver(screen_id screen)
{
	return (desktop_private::activescreen)?desktop_private::activescreen->GetGfxDriver():NULL;
}

/*!
	\brief Sets the video parameters of the specified workspace
	\param index The workspace to change
	\param res Resolution constant from GraphicsDefs.h
	\param stick If false, resolution will revert to its old value on next boot
	\param screen ID of the screen to change. Currently unused.
	\return B_OK if successful, B_ERROR if not.
	
	Because of the lack of outside multimonitor support, the screen ID is ignored for now.
*/
status_t SetSpace(int32 index, int32 res, screen_id screen, bool stick)
{

STRACE(("Desktop: SetSpace(%ld,%ld,%s,%ld)\n",index+1,res,
	stick?"stick":"non-stick",screen.id));

	desktop_private::workspacelock.Lock();
	status_t stat=desktop_private::activescreen->SetSpace(index,res,stick);
	desktop_private::workspacelock.Unlock();
	return stat;
}

/*!
	\brief Adds a window to the desktop so that it will be displayed
	\param win Window to add
	\param workspace index of the workspace to add the window to
	\param screen Optional screen specifier. Currently unused.
*/
void AddWindowToDesktop(ServerWindow *win, int32 workspace, screen_id screen)
{

STRACE(("Desktop: AddWindowToDesktop(%s,%ld,%ld)\n",win?win->GetTitle():"NULL",
	workspace+1,screen.id));

	// Workspace() will be non-NULL if it has already been added to the desktop
	if(!win || win->GetWorkspace())
		return;
	
	desktop_private::workspacelock.Lock();
	desktop_private::layerlock.Lock();

	Workspace *w=desktop_private::activescreen->GetActiveWorkspace();
	win->SetWorkspace(w);
	desktop_private::activescreen->AddWindow(win,workspace);
	
	desktop_private::layerlock.Unlock();
	desktop_private::workspacelock.Unlock();
}

WinBorder* WindowContainsPoint( BPoint pt ){

STRACE(("Desktop: WindowContainsPoint(%s,%f,%f)\n",win?win->GetTitle():"NULL",
	pt.x, pt.y));

	WinBorder		*wb;

	desktop_private::workspacelock.Lock();
	desktop_private::layerlock.Lock();

	wb = desktop_private::activescreen->GetWindowAt( pt );
	
	desktop_private::layerlock.Unlock();
	desktop_private::workspacelock.Unlock();
	
	return wb;
}
/*!
	\brief Removes a window from the desktop
	\param win Window to remove
*/
void RemoveWindowFromDesktop(ServerWindow *win)
{
STRACE(("Desktop: RemoveWindowFromDesktop(%s)\n",win?win->GetTitle():"NULL"));
	lock_workspaces();
	lock_layers();
	
	win->SetWorkspace(NULL);
	desktop_private::activescreen->RemoveWindow(win);
	
	unlock_layers();
	unlock_workspaces();
}

/*!
	\brief Returns the active window in the current workspace of the active screen
	\return The active window in the current workspace of the active screen
*/
ServerWindow *GetActiveWindow(void)
{
	lock_workspaces();
	ServerWindow *w=desktop_private::activescreen->ActiveWindow();
	unlock_workspaces();

	return w;
}

/*!
	\brief Sets the active window in the current workspace of the active screen
	\param win The window to activate
	
	If the window is not in the current workspace of the active screen, this call fails
*/
void SetActiveWindow(ServerWindow *win)
{
STRACE(("Desktop: SetActiveWindow(%s)\n",win?win->GetTitle():"NULL"));
	lock_workspaces();
	Workspace *w=desktop_private::activescreen->GetActiveWorkspace();
	if(win && win->GetWorkspace()!=w)
	{
		desktop_private::workspacelock.Unlock();
		return;
	}
	
	ServerWindow *oldwin=desktop_private::activescreen->ActiveWindow();
	ActivateWindow(oldwin,win);
	
	unlock_workspaces();
}

/*!
	\brief Returns the root layer for the specified workspace
	\param workspace Index of the workspace to use
	\param screen Index of the screen to use. Currently ignored.
	\return The root layer or NULL if there was an invalid parameter.
*/
Layer *GetRootLayer(int32 workspace, screen_id screen)
{
	lock_workspaces();
	lock_layers();
	Layer *r=desktop_private::activescreen->GetRootLayer();
	unlock_layers();
	unlock_workspaces();

	return r;
}

/*!
	\brief Assigns a flattened BMessage to the global drag data.
	\param size Size of the data buffer
	\param flattened Flattened BMessage data buffer
	
	This assigns a BMessage in its flattened state to the internal storage. Only 
	one message can be stored at a time. Once an assignment is made, another cannot 
	be made until the current one is emptied. If additional calls are made while 
	there is a drag message assigned, the function will fail and the new assignment 
	will not be made. Note that this merely passes a pointer around. No copies are 
	made and the caller is responsible for freeing any allocated objects from the heap.
*/
void set_drag_message(int32 size, int8 *flattened)
{
	lock_dragdata();

	if(desktop_private::dragmessage)
	{
		desktop_private::draglock.Unlock();
		return;
	}
	desktop_private::dragmessage=flattened;
	desktop_private::dragmessagesize=size;

	unlock_dragdata();
}

/*!
	\brief Gets the current drag data
	\param size Receives the size of the drag data buffer
	\return the drag data buffer or NULL if size is NULL
	
	Retrieve the current drag message in use. This function will fail if a NULL pointer 
	is passed to it. Note that this does not free the current drag message from use. 
	The size of the flattened message is stored in the size parameter. Also, note that if 
	there is no message in use, it will return NULL and set size to 0.
*/
int8 *get_drag_message(int32 *size)
{
	int8 *ptr=NULL;
	
	lock_dragdata();

	if(desktop_private::dragmessage)
	{
		ptr=desktop_private::dragmessage;
		*size=desktop_private::dragmessagesize;
	}
	unlock_dragdata();
	return ptr;
}

//! Empties current drag data and allows for new data to be assigned
void empty_drag_message(void)
{
	lock_dragdata();

	if(desktop_private::dragmessage)
		delete desktop_private::dragmessage;
	desktop_private::dragmessage=NULL;
	desktop_private::dragmessagesize=0;

	unlock_dragdata();
}


/*!
	\brief Thread-safe way of obtaining the current scrollbar behavior info
	\return Current scroll behavior data
*/
scroll_bar_info GetScrollBarInfo(void)
{
	scroll_bar_info sbi;
	desktop_private::optlock.Lock();
	sbi=desktop_private::scrollbarinfo;
	desktop_private::optlock.Unlock();
	return sbi;
}

/*!
	\brief Thread-safe way of setting the current scrollbar behavior info
	\param info Behavior data structure
*/
void SetScrollBarInfo(const scroll_bar_info &info)
{
	desktop_private::optlock.Lock();
	desktop_private::scrollbarinfo=info;
	desktop_private::optlock.Unlock();
}

/*!
	\brief Thread-safe way of obtaining the current menu behavior info
	\return Current menu behavior data
*/
menu_info GetMenuInfo(void)
{
	menu_info mi;
	desktop_private::optlock.Lock();
	mi=desktop_private::menuinfo;
	desktop_private::optlock.Unlock();
	return mi;
}

/*!
	\brief Thread-safe way of setting the current menu behavior info
	\param info Behavior data structure
*/
void SetMenuInfo(const menu_info &info)
{
	desktop_private::optlock.Lock();
	desktop_private::menuinfo=info;
	desktop_private::optlock.Unlock();
}

/*!
	\brief See if the the system is in focus-follows-mouse mode
	\return True if enabled, false if disabled
*/
bool GetFFMouse(void)
{
	bool value;
	desktop_private::optlock.Lock();
	value=desktop_private::ffm;
	desktop_private::optlock.Unlock();
	return value;
}

/*!
	\brief Enable or disable focus-follows-mouse for system
	\param info Enable/Disable flag
*/
void SetFFMouse(const bool &value)
{
	desktop_private::optlock.Lock();
	desktop_private::ffm=value;
	desktop_private::optlock.Unlock();
}

/*!
	\brief Obtain the current focus-follows-mouse behavior
	\return Current menu behavior data
*/
mode_mouse GetFFMouseMode(void)
{
	mode_mouse value;
	desktop_private::optlock.Lock();
	value=desktop_private::ffm_mode;
	desktop_private::optlock.Unlock();
	return value;
}

/*!
	\brief Set the focus-follows-mouse behavior
	\param info Behavior data structure
*/
void SetFFMouseMode(const mode_mouse &value)
{
	desktop_private::optlock.Lock();
	desktop_private::ffm_mode=value;
	desktop_private::optlock.Unlock();
}

/*!
	\brief Read in settings from R5's workspace settings file in ~/config/settings
	\return True if successful, false unable to read and parse file.
*/
bool ReadR5WorkspaceData(void)
{
	//TODO: Add read checks for fscsnf calls
	
	// Should there be multiple graphics devices, I assume that we'd probably need to loop through
	// each device and read the workspace data for each. For now, we just assume that there is
	// just one.
	
	FILE *file=fopen("/boot/home/config/settings/app_server_settings",B_READ_ONLY);
	if(!file)
		return false;
	
	char devicepath[B_PATH_NAME_LENGTH];
	as_workspace_data wdata;
	int workspace_count=3;
	
	Screen *active=GetActiveScreen();
	
	// read in device
	// Device <devpath>
	fscanf(file,"Device %s",devicepath);
	
	// read workspace count
	// Workspaces # {
	fscanf(file,"Workspaces %d {",&workspace_count);
	
	// read workspace config for all 32 (0-31)
	
	for(int i=0; i<32;i++)
	{
		// Workspace # {
		fscanf(file,"Workspace %d {",&wdata.index);
	
		// timing: display_timing struct in Accelerant.h
		fscanf(file,"timing %lu %u %u %u %u %u %u %u %u %lu",&wdata.timing.pixel_clock,
		(unsigned int*)&(wdata.timing.h_display),(unsigned int*)&(wdata.timing.h_sync_start),
		(unsigned int*)&(wdata.timing.h_sync_end),(unsigned int*)&(wdata.timing.h_total),
		(unsigned int*)&(wdata.timing.v_display),(unsigned int*)&(wdata.timing.v_sync_start),
		(unsigned int*)&(wdata.timing.v_sync_end),(unsigned int*)&(wdata.timing.v_total),
		&wdata.timing.flags);
		
		// colorspace: member of color_space struct, like RGB15, RGB32, etc.
		fscanf(file,"colorspace %lx",(long*)&wdata.space);
		
		// virtual width height
		fscanf(file,"virtual %d %d",&wdata.res_w, &wdata.res_h);
		
		// flags:
		fscanf(file,"flags %lx",&wdata.flags);
		
		// color: RGB hexcode
		fscanf(file,"color %lx",&wdata.bgcolor);
		// }
		
		// Create a workspace and add it to the active screen.
		Workspace *w=new Workspace(&wdata,active);
		active->AddWorkspace(w);
	}
	
	scroll_bar_info sbi;
	
	// read interface block
	// Interface {
	// read scrollbars block
	// Scrollbars {
	int bflag;
	
	// get the four scrollbar values
	// proportional 1
	fscanf(file,"proportional %d",&bflag);
	sbi.proportional=(bflag==1)?true:false;

	// arrows 2
	fscanf(file,"arrows %d",&bflag);
	sbi.double_arrows=(bflag==2)?true:false;
	
	// knobtype 0
	fscanf(file,"knobtype %ld",&sbi.knob);

	// knobsize 15
	fscanf(file,"knobsize %ld",&sbi.min_knob_size);
	// }
	
	// read options block
	// Options {
	
	// get focus-follows-mouse block
	// ffm #
	// FFM values:
	// 0: Disabled
	// 1: Enabled
	// 2: Warping
	// 3: Instant Warping
	int ffm;
	fscanf(file,"ffm %d",&ffm);
	SetFFMouse(ffm);
	
	// skip decor listing
	fclose(file);
	
	return true;
}

/*!
	\brief Reads in workspace config data using the regular system file
	\return True if settings were successfully loaded. False if not
	
	The file's location is defined in ServerConfig.h - SERVER_SETTINGS_DIR/WORKSPACE_SETTINGS_NAME.	
*/
bool ReadOBOSWorkspaceData(void)
{
	//TODO: Implement
	return false;
}

//! Saves workspace config to SERVER_SETTINGS_DIR/WORKSPACE_SETTINGS_NAME as defined in ServerConfig.h
void SaveWorkspaceData(void)
{
	//TODO: Implement
}
