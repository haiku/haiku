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
//				Gabe Yoder <gyoder@stny.rr.com>
//	Description:		Implements the Screen class which provides all
//				infrastructure for handling a video card/monitor
//				pair.
//				Implements the Workspace class which provides all
//				infrastructure for drawing the screen.
//  
//------------------------------------------------------------------------------

#include "DesktopClasses.h"

/*!
	\brief Sets up internal variables needed by the Workspace
	\param gcinfo The graphics card info
	\param fbinfo The frame buffer info
*/
Workspace::Workspace(const graphics_card_info &gcinfo, const frame_buffer_info &fbinfo)
{
	_gcinfo=gcinfo;
	_fbinfo=fbinfo;
	
	//TODO: create the root layer here based on gcinfo and fbinfo.
	_rootlayer=NULL;

	/* From Docs
     1) Set background color to RGB(51,102,160)
2) Copy frame_buffer_info and graphics_card_info structure values
3) Create a RootLayer object using the values from the two structures
     */
}

/*!
	\brief Deletes the heap memory used by the Workspace
*/
Workspace::~Workspace(void)
{
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
	_gcinfo=gcinfo;
	_fbinfo=fbinfo;
	/* From Docs:
     1) Copy the two structures to the internal one
2) Resize the RootLayer
3) If the RootLayer was resized larger, Invalidate the new areas
*/
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
	\brief Sets up internal variables needed by Screen
	\param gfxmodule Pointer to the uninitialized display driver to use
	\param workspaces The number of workspaces on this screen
*/
Screen::Screen(DisplayDriver *gfxmodule, uint8 workspaces)
{
	int i;

	_workspacelist=NULL;
	_resolution=0;
	_activewin=NULL;
	_currentworkspace=-1;
	_activeworkspace=NULL;
	_workspacecount=0;
	_init=false;
	_driver = gfxmodule;

	if ( _driver && _driver->Initialize() )
	{
		_init = true;
		/* TODO: Get approriate display driver info and record it
		   in _gcinfo and _fbinfo
		 */

		_workspacelist = new BList(workspaces);
		_workspacecount = workspaces;
		for (i=0; i<workspaces; i++)
		{
			_workspacelist->AddItem(new Workspace(_gcinfo,_fbinfo));
		}
	}
}

/*!
	\brief Deletes the heap memory used by the Screen and shuts down the driver
*/
Screen::~Screen(void)
{
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
	Workspace *workspace = new Workspace(_gcinfo,_fbinfo);
	if ( (index == -1) || !_workspacelist->AddItem(workspace,index) )
		_workspacelist->AddItem(workspace);
}

/*!
	\brief Deletes the workspace at the specified index
	\param index The position within the workspace list
*/
void Screen::DeleteWorkspace(int32 index)
{
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
	if ( (index >= 0) && (index <= _workspacecount-1) )
	{
		_currentworkspace = index;
		_activeworkspace = (Workspace *)_workspacelist->ItemAt(index);
	}
}

/*!
	\brief Activates the Screen
	\param active (Defaults to true)
*/
void Screen::Activate(bool active)
{
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
	\brief 
	\param index
	\param res
	\param stick (Default is true)
	\return 
*/
status_t Screen::SetSpace(int32 index, int32 res,bool stick)
{
	return B_OK;
}

/*!
	\brief 
	\param win
	\param workspace (Default is B_CURRENT_WORKSPACE)
*/
void Screen::AddWindow(ServerWindow *win, int32 workspace)
{
}

/*!
	\brief 
	\param win
*/
void Screen::RemoveWindow(ServerWindow *win)
{
}

/*!
	\brief 
	\return 
*/
ServerWindow *Screen::ActiveWindow(void)
{
	return _activewin;
}

/*!
	\brief 
	\param win
*/
void Screen::SetActiveWindow(ServerWindow *win)
{
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

