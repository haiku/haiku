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
//#include "ServerWindow.h"

namespace desktop_private {
	int8 *dragmessage;
	int32 dragmessagesize;
	sem_id draglock;
}

void InitDesktop(void)
{
}

void ShutdownDesktop(void)
{
}

void AddWorkspace(int32 index=-1)
{
}

void DeleteWorkspace(int32 index)
{
}

int32 CountWorkspaces(void)
{
	return 0;
}

void SetWorkspaceCount(int32 count)
{
}

int32 CurrentWorkspace(screen_id screen=B_MAIN_SCREEN_ID)
{
	return 0;
}

void SetWorkspace(int32 workspace, screen_id screen=B_MAIN_SCREEN_ID)
{
}

void SetScreen(screen_id id)
{
}

int32 CountScreens(void)
{
	return 0;
}

screen_id ActiveScreen(void)
{
	return B_MAIN_SCREEN_ID;
}

DisplayDriver *GetGfxDriver(screen_id screen=B_MAIN_SCREEN_ID)
{
	return NULL;
}

status_t SetSpace(int32 index, int32 res, bool stick=true, screen_id screen=B_MAIN_SCREEN_ID)
{
	return B_ERROR;
}

void AddWindowToDesktop(ServerWindow *win, int32 workspace=B_CURRENT_WORKSPACE, screen_id screen=B_MAIN_SCREEN_ID)
{
}

void RemoveWindowFromDesktop(ServerWindow *win)
{
}

ServerWindow *GetActiveWindow(void)
{
	return NULL;
}

void SetActiveWindow(ServerWindow *win)
{
}

Layer *GetRootLayer(int32 workspace=B_CURRENT_WORKSPACE, screen_id screen=B_MAIN_SCREEN_ID)
{
	return NULL;
}

void set_drag_message(int32 size, int8 *flattened)
{
}

int8 *get_drag_message(int32 *size)
{
	return NULL;
}

void empty_drag_message(void)
{
}

