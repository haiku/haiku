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
//	File Name:		DesktopClasses.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Gabe Yoder <gyoder@stny.rr.com>
//	Description:	Classes for managing workspaces and screens
//  
//------------------------------------------------------------------------------
#ifndef DESKTOPCLASSES_H
#define DESKTOPCLASSES_H

#include <SupportDefs.h>
#include <Accelerant.h>
#include <GraphicsDefs.h>
#include <GraphicsCard.h>
#include <Window.h>	// for workspace defs

class DisplayDriver;
class ServerWindow;
class RGBColor;
class Screen;
class WinBorder;
class RootLayer;
class Layer;

/*!
	\class Workspace DesktopClasses.h
	\brief Object used to handle all things Workspace related.
	
	Doesn't actually do a whole lot except to couple some associated data with a 
	RootLayer.
*/
typedef struct
{
	int index;
	display_timing timing;
	color_space space;
	int res_w;
	int res_h;
	int32 flags;
	int32 bgcolor;
} as_workspace_data;

class Workspace
{
public:
	Workspace(const graphics_card_info &gcinfo, const frame_buffer_info &fbinfo, Screen *screen);
	Workspace(as_workspace_data *data, Screen *screen);
	~Workspace(void);
	void SetBGColor(const RGBColor &c);
	RGBColor BGColor();
	RootLayer *GetRoot(void);
	void SetData(const graphics_card_info &gcinfo, const frame_buffer_info &fbinfo);
	void GetData(graphics_card_info *gcinfo, frame_buffer_info *fbinfo);
	void SetSpace(int32 res);
	//! Returns the screen to which the workspace belongs
	Screen *GetScreen(void) { return _screen; }
protected:
	RootLayer *_rootlayer;
	graphics_card_info _gcinfo;
	frame_buffer_info _fbinfo;
	Screen *_screen;
};

/*!
	\class Screen DesktopClasses.h
	\brief Handles each DisplayDriver/video card pair and associated management.
	
	There is only one per monitor. Manages all workspaces displayed on that 
	particular monitor and also Window-Desktop management for all workspaces therein.
*/
class Screen
{
public:
	Screen(DisplayDriver *gfxmodule);
	~Screen(void);
	void AddWorkspace(int32 index=-1);
	void AddWorkspace(Workspace *workspace,int32 index=-1);
	void DeleteWorkspace(int32 index);
	int32 CountWorkspaces(void);
	void SetWorkspaceCount(int32 count);
	int32 CurrentWorkspace(void);
	void SetWorkspace(int32 index);
	void Activate(bool active=true);
	DisplayDriver *GetGfxDriver(void);
	status_t SetSpace(int32 index, int32 res,bool stick=true);
	void AddWindow(ServerWindow *win, int32 workspace=B_CURRENT_WORKSPACE);
	void RemoveWindow(ServerWindow *win);
	WinBorder* GetWindowAt( BPoint pt );
	ServerWindow *ActiveWindow(void);
	void SetActiveWindow(ServerWindow *win);
	Layer* GetRootLayer(int32 workspace=B_CURRENT_WORKSPACE);
	bool IsInitialized(void);
	Workspace *GetWorkspace(int32 index);
	Workspace *GetActiveWorkspace(void);
	//! Returns the unique identifier for the screen
	screen_id GetID(void) { return _id; }
protected:
	int32 _resolution;
	ServerWindow *_activewin;
	int32 _currentworkspace;
	int32 _workspacecount;
	BList *_workspacelist;
	DisplayDriver *_driver;
	bool _init, _active;
	Workspace *_activeworkspace;
	graphics_card_info _gcinfo;
	frame_buffer_info _fbinfo;
	screen_id _id;
};

#endif
