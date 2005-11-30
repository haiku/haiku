/*
 * Copyright 2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "RootLayer.h"
#include "Workspace.h"
#include "WindowLayer.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>


static RGBColor kDefaultColor = RGBColor(51, 102, 152);
const BPoint kInvalidWindowPosition = BPoint(INFINITY, INFINITY);


Workspace::Private::Private()
	:
	fWindows(20, true)
		// this list owns its items
{
	_SetDefaults();
}


Workspace::Private::~Private()
{
}


bool
Workspace::Private::AddWindow(WindowLayer* window, BPoint* position)
{
	window_layer_info* info = new (nothrow) window_layer_info;
	if (info == NULL)
		return false;

	info->position = position ? *position : kInvalidWindowPosition;
	info->window = window;

	bool success = fWindows.AddItem(info);
	if (!success)
		delete info;

	return success;
}


void
Workspace::Private::RemoveWindow(WindowLayer* window)
{
	for (int32 i = fWindows.CountItems(); i-- > 0;) {
		window_layer_info* info = fWindows.ItemAt(i);

		if (info->window == window) {
			fWindows.RemoveItemAt(i);
			delete info;
			return;
		}
	}
}


void
Workspace::Private::RemoveWindowAt(int32 index)
{
	window_layer_info* info = fWindows.RemoveItemAt(index);
	delete info;
}


void
Workspace::Private::SetDisplaysFromDesktop(Desktop* desktop)
{
}


void
Workspace::Private::SetColor(const RGBColor& color)
{
	fColor = color;
}


void
Workspace::Private::SetSettings(BMessage& settings)
{
}


void
Workspace::Private::GetSettings(BMessage& settings)
{
}


void
Workspace::Private::_SetDefaults()
{
	fColor.SetColor(kDefaultColor);
}


//	#pragma mark -


Workspace::Workspace(Desktop& desktop, int32 index)
	:
	fWorkspace(desktop.WorkspaceAt(index)),
	fDesktop(desktop),
	fCurrentWorkspace(index == desktop.CurrentWorkspace())
{
	fDesktop.Lock();
	RewindWindows();
}


Workspace::~Workspace()
{
	fDesktop.Unlock();
}


const RGBColor&
Workspace::Color() const
{
	return fWorkspace.Color();
}


status_t
Workspace::GetNextWindow(WindowLayer*& _window, BPoint& _leftTop)
{
	if (fCurrentWorkspace) {
		if (fCurrent == NULL)
			fCurrent = (WindowLayer*)fDesktop.RootLayer()->FirstChild();
		else
			fCurrent = (WindowLayer*)fCurrent->NextLayer();
		
		if (fCurrent == NULL)
			return B_ENTRY_NOT_FOUND;

		_window = fCurrent;
		_leftTop = fCurrent->Frame().LeftTop();
		return B_OK;
	}

	window_layer_info* info = fWorkspace.WindowAt(fIndex++);
	if (info == NULL)
		return B_ENTRY_NOT_FOUND;

	_window = info->window;
	_leftTop = info->position;
	return B_OK;
}


void
Workspace::RewindWindows()
{
	fIndex = 0;
	fCurrent = NULL;
}

