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

#if DISPLAYDRIVER == SCREENDRIVER
#include "ScreenDriver.h"
#endif

#if DISPLAYDRIVER == HWDRIVER
#include "AccelerantDriver.h"
#endif

#include "ServerWindow.h"

//! This namespace encapsulates all globals specifically for the desktop
namespace desktop_private {
	int8 *dragmessage=NULL;
	int32 dragmessagesize=0;
	BLocker draglock,
			layerlock,
			workspacelock;
	BList *screenlist=NULL;
	Screen *activescreen=NULL;
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
	desktop_private::screenlist=new BList(0);
	DisplayDriver *tdriver;
	Screen *s=NULL;
	
#if DISPLAYDRIVER == HWDRIVER

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
	}
#else

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
	}
#endif
	
	// Once we utilize multiple monitors, we'll make Screen 0 the active one.
	if(tdriver)
	{
		s->Activate();
		desktop_private::activescreen=s;
	}
}

//! Shuts down the graphics subsystem
void ShutdownDesktop(void)
{
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
void AddWorkspace(int32 index=-1)
{
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
}

/*!
	\brief Returns the index of the active workspace
	\return the index of the active workspace
*/
int32 CurrentWorkspace(void)
{
	return 0;
}

/*!
	\brief Sets the active workspace
	\param workspace Index of the workspace to activate
	
	This function will do nothing if passed an index which does not refer to a valid 
	workspace.
*/
void SetWorkspace(int32 workspace)
{
}

/*!
	\brief Sets the active screen. Currently unused.
	\param id ID of the screen to activate.
*/
void SetScreen(screen_id id)
{
}

/*!
	\brief Returns the number of screens available. Currently unused.
	\return the number of screens available.
*/
int32 CountScreens(void)
{
	return 0;
}

/*!
	\brief Returns the index of the active screen. Currently unused.
	\return the index of the active screen
*/
screen_id ActiveScreen(void)
{
	return B_MAIN_SCREEN_ID;
}

/*!
	\brief Obtains the DisplayDriver object used for the specified screen.
	\param screen ID of the screen to get the DisplayDriver for.
	\return The DisplayDriver or NULL if not available for that screen ID
	
	Like the other multiscreen functions, this is unused. Because multiple screens are 
	not utilized, specifying an ID other than B_MAIN_SCREEN_ID will return NULL 
*/
DisplayDriver *GetGfxDriver(screen_id screen=B_MAIN_SCREEN_ID)
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
status_t SetSpace(int32 index, int32 res, bool stick=true, screen_id screen=B_MAIN_SCREEN_ID)
{
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
void AddWindowToDesktop(ServerWindow *win, int32 workspace=B_CURRENT_WORKSPACE, screen_id screen=B_MAIN_SCREEN_ID)
{
	// Workspace() will be non-NULL if it has already been added to the desktop
	if(!win || win->GetWorkspace())
		return;
	
	desktop_private::workspacelock.Lock();
	desktop_private::layerlock.Lock();

	Workspace *w=desktop_private::activescreen->GetActiveWorkspace();
	win->SetWorkspace(w);
	
	desktop_private::layerlock.Unlock();
	desktop_private::workspacelock.Unlock();
}

/*!
	\brief Removes a window from the desktop
	\param win Window to remove
*/
void RemoveWindowFromDesktop(ServerWindow *win)
{
	desktop_private::workspacelock.Lock();
	desktop_private::layerlock.Lock();
	
	win->SetWorkspace(NULL);
	
	desktop_private::layerlock.Unlock();
	desktop_private::workspacelock.Unlock();
}

/*!
	\brief Returns the active window in the current workspace of the active screen
	\return The active window in the current workspace of the active screen
*/
ServerWindow *GetActiveWindow(void)
{
	desktop_private::workspacelock.Lock();
	ServerWindow *w=desktop_private::activescreen->ActiveWindow();
	desktop_private::workspacelock.Unlock();

	return w;
}

/*!
	\brief Sets the active window in the current workspace of the active screen
	\param win The window to activate
	
	If the window is not in the current workspace of the active screen, this call fails
*/
void SetActiveWindow(ServerWindow *win)
{
	desktop_private::workspacelock.Lock();
	Workspace *w=desktop_private::activescreen->GetActiveWorkspace();
	if(win->GetWorkspace()!=w)
	{
		desktop_private::workspacelock.Unlock();
		return;
	}
	
	ServerWindow *oldwin=desktop_private::activescreen->ActiveWindow();
	ActivateWindow(oldwin,win);
	
	
	desktop_private::workspacelock.Unlock();
}

/*!
	\brief Returns the root layer for the specified workspace
	\param workspace Index of the workspace to use
	\param screen Index of the screen to use. Currently ignored.
	\return The root layer or NULL if there was an invalid parameter.
*/
Layer *GetRootLayer(int32 workspace=B_CURRENT_WORKSPACE, screen_id screen=B_MAIN_SCREEN_ID)
{
	desktop_private::workspacelock.Lock();
	Layer *r=desktop_private::activescreen->GetRootLayer();
	desktop_private::workspacelock.Unlock();

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
	desktop_private::draglock.Lock();

	if(desktop_private::dragmessage)
	{
		desktop_private::draglock.Unlock();
		return;
	}
	desktop_private::dragmessage=flattened;
	desktop_private::dragmessagesize=size;

	desktop_private::draglock.Unlock();
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
	
	desktop_private::draglock.Lock();

	if(desktop_private::dragmessage)
	{
		ptr=desktop_private::dragmessage;
		*size=desktop_private::dragmessagesize;
	}
	desktop_private::draglock.Unlock();
	return ptr;
}

//! Empties current drag data and allows for new data to be assigned
void empty_drag_message(void)
{
	desktop_private::draglock.Lock();

	if(desktop_private::dragmessage)
		delete desktop_private::dragmessage;
	desktop_private::dragmessage=NULL;
	desktop_private::dragmessagesize=0;

	desktop_private::draglock.Unlock();
}

