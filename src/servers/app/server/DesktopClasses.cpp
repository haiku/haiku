//------------------------------------------------------------------------------
//	Copyright (c) 2001-2003, OpenBeOS
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
//	File Name:		DesktopClasses.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Gabe Yoder <gyoder@stny.rr.com>
//	Description:	Classes for managing workspaces and screens
//  
//------------------------------------------------------------------------------
#include <File.h>
#include <Message.h>
#include <stdio.h>
#include <string.h>
#include "DesktopClasses.h"
#include "TokenHandler.h"
#include "ServerWindow.h"
#include "WinBorder.h"
#include "RootLayer.h"
#include "Desktop.h"
#include "DisplayDriver.h"
#include "Decorator.h"


//#define DEBUG_WORKSPACE
//#define DEBUG_SCREEN

#ifdef DEBUG_WORKSPACE
#	include <stdio.h>
#	define STRACE_WS(x) printf x
#else
#	define STRACE_WS(x) ;
#endif

#ifdef DEBUG_SCREEN
#	include <stdio.h>
#	define STRACE_SCREEN(x) printf x
#else
#	define STRACE_SCREEN(x) ;
#endif

// Defined and initialized in AppServer.cpp
extern RGBColor workspace_default_color;
TokenHandler screen_id_handler;

/*!
	\brief Sets up internal variables needed by the Workspace
	\param gcinfo The graphics card info
	\param fbinfo The frame buffer info
	\param gfxdriver DisplayDriver for the associated RootLayer
*/
Workspace::Workspace(const graphics_card_info &gcinfo, const frame_buffer_info &fbinfo, Screen *screen)
{
	_gcinfo=gcinfo;
	_fbinfo=fbinfo;
	
	_screen=screen;
	_rootlayer=new RootLayer(BRect(0,0,fbinfo.display_width-1,fbinfo.display_height-1),
		"Workspace Root",_screen->GetGfxDriver());
	_rootlayer->SetColor(workspace_default_color);
STRACE_WS(("Workspace::Workspace(%s)\n",_rootlayer->GetName()));
}

/*!
	\brief Constructor to accept data from reading R5's settings
*/
Workspace::Workspace(as_workspace_data *data, Screen *screen)
{
	//TODO: Implement
}

/*!
	\brief Deletes the heap memory used by the Workspace
*/
Workspace::~Workspace(void)
{
STRACE_WS(("Workspace::~Workspace(%s)\n",_rootlayer->GetName()));
	if(_rootlayer)
	{
		_rootlayer->PruneTree();
		delete _rootlayer;
	}
}

/*!
	\brief Sets the background color of the workspace
	\param c The new background color
	Note: This does not refresh the display
*/
void Workspace::SetBGColor(const RGBColor &c)
{
STRACE_WS(("Workspace::SetBGColor(): "));
	c.PrintToStream();
	_rootlayer->SetColor(c);
}

/*!
	\brief Returns the background color of the workspace
	\return The background color
*/
RGBColor Workspace::BGColor()
{
	return _rootlayer->GetColor();
}

/*!
	\brief Returns a pointer to the RootLayer object
	\return The RootLayer object
*/
RootLayer *Workspace::GetRoot(void)
{
	return _rootlayer;
}

/*!
	\brief Changes the graphics data and resizes the RootLayer accordingly
	\param gcinfo The new graphics card info
	\param fbinfo The new frame buffer info
*/
void Workspace::SetData(const graphics_card_info &gcinfo, const frame_buffer_info &fbinfo)
{
STRACE_WS(("Workspace::SetData(%s)\n",_rootlayer->GetName()));
	if(_fbinfo.display_width!=fbinfo.display_width || 
		_fbinfo.display_height!=fbinfo.display_height)
	{
		// We won't need to invalidate the new regions as mentioned in the original implementation
		// docs because RootLayer reimplements ResizeBy to handle this
		_rootlayer->ResizeBy( (fbinfo.display_width-_fbinfo.display_width),
			(fbinfo.display_height-_fbinfo.display_height) );
	}
	_gcinfo=gcinfo;
	_fbinfo=fbinfo;
}

/*!
	\brief Obtains the graphics card info and frame buffer info
	\param gcinfo The graphics card info structure that receives the data
	\param fbinfo The frame buffer info structure that receives the data
*/
void Workspace::GetData(graphics_card_info *gcinfo, frame_buffer_info *fbinfo)
{
	*gcinfo=_gcinfo;
	*fbinfo=_fbinfo;
}

/*!
	
	\brief Changes the size and mode of the workspace
	\param res The new resolution mode of the workspace
*/
void Workspace::SetSpace(int32 res)
{
STRACE_WS(("Workspace::SetSpace(%ld) unimplemented\n",res));
	// TODO: Implement
}

/*!
	\brief Sets up internal variables needed by Screen
	\param gfxmodule Pointer to the uninitialized display driver to use
	\param workspaces The number of workspaces on this screen
*/
Screen::Screen(DisplayDriver *gfxmodule, uint8 workspaces)
{
STRACE_SCREEN(("Screen::Screen(%s,%u)\n",gfxmodule?"driver":"NULL",workspaces));
	_workspacelist=NULL;
	_driver=gfxmodule;
	_resolution=0;
	_activewin=NULL;
	_currentworkspace=-1;
	_activeworkspace=NULL;
	_workspacecount=0;
	_init=false;
	_active=false;
	_id.id=screen_id_handler.GetToken();

	if (_driver)
	{
		_init=true;

		_fbinfo.bits_per_pixel=_driver->GetDepth();
		_fbinfo.bytes_per_row=_driver->GetBytesPerRow();
		_fbinfo.width=_driver->GetWidth();
		_fbinfo.height=_driver->GetHeight();
		_fbinfo.display_width=_driver->GetWidth();
		_fbinfo.display_height=_driver->GetHeight();
		_fbinfo.display_x=_driver->GetWidth();
		_fbinfo.display_y=_driver->GetHeight();

		_gcinfo.width=_driver->GetWidth();
		_gcinfo.height=_driver->GetHeight();
		_gcinfo.bytes_per_row=_driver->GetBytesPerRow();
		_gcinfo.bits_per_pixel=_driver->GetDepth();
		
		// right now, we won't do anything with the gcinfo structure. ** LAZY PROGRAMMER ALERT ** :P

		_workspacelist = new BList(workspaces);
		_workspacecount = workspaces;
		for (int i=0; i<workspaces; i++)
		{
			_workspacelist->AddItem(new Workspace(_gcinfo,_fbinfo,this));
		}
		_resolution=_driver->GetMode();
		
		if(workspaces>0)
		{
			_currentworkspace=0;
			_activeworkspace=(Workspace*)_workspacelist->ItemAt(0);
			_workspacecount=workspaces;
			_activeworkspace->GetRoot()->Show();
//			_activeworkspace->GetRoot()->RequestDraw();
		}
	}
}

/*!
	\brief Deletes the heap memory used by the Screen and shuts down the driver
*/
Screen::~Screen(void)
{
STRACE_SCREEN(("Screen::~Screen\n"));
	if ( _workspacelist )
	{
		int i;
		for (i=0; i<_workspacecount; i++)
			delete (Workspace *)_workspacelist->ItemAt(i);
		delete _workspacelist;
	}
	if ( _driver )
		_driver->Shutdown();
}

/*!
	\brief Adds a workspace with default settings to the screen object
	\param index The position within the workspace list (default = -1 = end)
*/
void Screen::AddWorkspace(int32 index)
{
STRACE_SCREEN(("Screen::AddWorkspace(%ld)\n",index+1));
	Workspace *workspace = new Workspace(_gcinfo,_fbinfo,this);
	if ( (index == -1) || !_workspacelist->AddItem(workspace,index) )
		_workspacelist->AddItem(workspace);
}

/*!
	\brief Adds a workspace object to the screen
	\param workspace The workspace to add.
	\param index Optional index to insert workspace at. Defaults to adding to end of list.
	
	This function will do nothing if workspace is NULL.
*/
void Screen::AddWorkspace(Workspace *workspace,int32 index)
{
STRACE_SCREEN(("Screen::AddWorkspace(%s)\n",(workspace && workspace->GetRoot())?workspace->GetRoot()->GetName():"NULL"));
	if ( (index==-1) || !_workspacelist->AddItem(workspace,index) )
		_workspacelist->AddItem(workspace);
}

/*!
	\brief Deletes the workspace at the specified index
	\param index The position within the workspace list
*/
void Screen::DeleteWorkspace(int32 index)
{
STRACE_SCREEN(("Screen::DeleteWorkspace(%ld)\n",index+1));
	Workspace *workspace;
	workspace = (Workspace *)_workspacelist->RemoveItem(index);
	if ( workspace )
		delete workspace;
}

/*!
	\brief Returns the number of workspaces handled by the screen object
	\return The number of workspaces
*/
int32 Screen::CountWorkspaces(void)
{
	return _workspacecount;
}

/*!
	\brief Sets the number of available workspaces to count
	\param count The new number of available workspaces (1 <= count <= 32)
	If count is less than the current count, workspaces are deleted from the
	end.  Any workspaces added are added to the end of the list.
*/
void Screen::SetWorkspaceCount(int32 count)
{
	int i;

	if ( count < 1 )
		count = 1;
	if ( count > 32 )
		count = 32;
	if ( _workspacecount == count )
		return;
	for (i=_workspacecount; i<count; i++)
		AddWorkspace();
	for (i=_workspacecount; i>count; i--)
		DeleteWorkspace(i-1);
	_workspacecount = count;
	if ( _currentworkspace > count-1 )
		SetWorkspace(count-1);

STRACE_SCREEN(("Screen::SetWorkspaceCount(%ld)\n",count));
}

/*!
	\brief Returns the active workspace index
	\return The active workspace index
*/
int32 Screen::CurrentWorkspace(void)
{
	return _currentworkspace;
}

/*!
	\brief Sets the active workspace
	\param index The index of the new active workspace
*/
void Screen::SetWorkspace(int32 index)
{
STRACE_SCREEN(("Screen::SetWorkspace(%ld)\n",index+1));
	if ( (index >= 0) && (index <= _workspacecount-1) )
	{
		_currentworkspace = index;
		_activeworkspace = (Workspace *)_workspacelist->ItemAt(index);
	}
}

/*!
	\brief Changes the active status of the screen
	\param active Flag - should the screen be active?
*/
void Screen::Activate(bool active)
{
STRACE_SCREEN(("Screen::Activate(%s)\n",active?"active":"inactive"));
	_active=active;
}

/*!
	\brief Returns a pointer to the display driver used by the Screen
	\return The display driver
*/
DisplayDriver *Screen::GetGfxDriver(void)
{
	return _driver;
}

/*!
	\brief Changes the screen's attributes - depth, size, etc.
	\param index Workspace to change
	\param res Resolution constant defined in GraphicsDefs.h
	\param stick Make the change persistent across reboots
	\return B_OK if succcessful, B_ERROR if not
*/
status_t Screen::SetSpace(int32 index, int32 res,bool stick)
{
STRACE_SCREEN(("Screen::SetSpace(%ld,%ld,%s)\n",index,res,stick?"stick":"non-stick"));
	// the specified workspace isn't active, so this should be easy...
	Workspace *wkspc=(Workspace*)_workspacelist->ItemAt(index);
	if(!wkspc)
		return B_ERROR;
	wkspc->SetSpace(res);

	if(index==_currentworkspace)
	{
		// change the statistics of the current workspace
		if(_driver)
			_driver->SetMode(res);
	}
	return B_OK;
}

/*!
	\brief Adds a Window to the desktop
	\param win Window to add to the desktop
	\param workspace Workspace to add the window to
	
	The ServerWindow's WindowBorder object is added to the RootLayer of the workspace 
	in question. If the window has already been added to another screen, this function 
	will remove it from its old parent and add it to the current one.
*/
void Screen::AddWindow(ServerWindow *win, int32 workspace)
{
STRACE_SCREEN(("Screen::AddWindow(%s,%ld)\n",win?win->GetTitle():"NULL", workspace+1));
	if(!win || !win->_winborder)
		return;
	
	Layer *rl=GetRootLayer(workspace);
	if(rl)
		rl->AddChild(win->_winborder);
}

/*!
	\brief Removes a Window from the desktop
	\param win The window to remove
	
	The window will remove itself from whatever screen it has been added to, or if it has not been 
	added to the desktop, it will do nothing.
*/
void Screen::RemoveWindow(ServerWindow *win)
{
STRACE_SCREEN(("Screen::RemoveWindow(%s)\n",win?win->GetTitle():"NULL"));
	if(!win || !win->_winborder)
		return;
	
	win->_winborder->RemoveSelf();
}
/*!
	\brief Returns the WinBorder taht contains the point
	\param	The point
	\return The WinBorder that contains the point
*/
WinBorder* Screen::GetWindowAt( BPoint pt ){
	WinBorder	*wb;
	Layer 	*rl = GetRootLayer();
	Layer	*child;
	for(child = rl->_bottomchild; child!=NULL; child = child->_uppersibling)
	{
		if(child->_hidden)
			continue;
		wb = dynamic_cast<WinBorder*>(child);
		if(wb)
		{
			BRegion reg;
			wb->GetDecorator()->GetFootprint(&reg);
			if(reg.Contains(pt))
				return wb;
		}
	}
	return NULL;
}

/*!
	\brief Returns the active window in the current workspace
	\return The active window in the current workspace
*/
ServerWindow *Screen::ActiveWindow(void)
{
	return _activewin;
}

/*!
	\brief Activates a window on the desktop
	\param win The window to activate
	
	If the given window has not been added to the desktop, it will be automatically added 
	to the current workspace. If, for some reason, the window belongs to a workspace on 
	another screen, this function will fail.
*/
void Screen::SetActiveWindow(ServerWindow *win)
{
	if(win)
	{
		if(win->GetWorkspace()==NULL)
		{
			// has not been added to desktop
			AddWindowToDesktop(win,win->GetWorkspaceIndex(),_id);
		}
		Workspace *wksp=win->GetWorkspace();
		if(wksp->GetScreen()!=this)
			return;
		set_active_winborder(win->_winborder);
	}
	else
		set_active_winborder(NULL);
	_activewin=win;
}

/*!
	\brief Returns the RootLayer of the specified workspace
	\param workspace The index of the workspace (Default is B_CURRENT_WORKSPACE)
	\return The RootLayer object
*/
Layer *Screen::GetRootLayer(int32 workspace)
{
	return (Layer*)_activeworkspace->GetRoot();
}

/*!
	\brief Indicates whether the Screen is initialized
	\return True if initialized, false if not
*/
bool Screen::IsInitialized(void)
{
	return _init;
}

/*!
	\brief Returns a pointer to the active Workspace object
	\return The active workspace object
*/
Workspace *Screen::GetActiveWorkspace(void)
{
	return _activeworkspace;
}

/*!
	\brief Returns a pointer to the Workspace at the specified index
	\return A pointer to the Workspace at the specified index
*/
Workspace *Screen::GetWorkspace(int32 index)
{
	if(index==_currentworkspace)
		return _activeworkspace;

	return (Workspace*)_workspacelist->ItemAt(index);
}
