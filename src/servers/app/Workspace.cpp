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


Workspace::Workspace()
	:
	fWindows(20, true)
		// this list owns its items
{
	_SetDefaults();
}


Workspace::~Workspace()
{
}


bool
Workspace::AddWindow(WindowLayer* window, BPoint* position)
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
Workspace::RemoveWindow(WindowLayer* window)
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
Workspace::RemoveWindowAt(int32 index)
{
	window_layer_info* info = fWindows.RemoveItemAt(index);
	delete info;
}


void
Workspace::SetDisplaysFromDesktop(Desktop* desktop)
{
}


void
Workspace::SetColor(const RGBColor& color)
{
	fColor = color;
}


void
Workspace::SetSettings(BMessage& settings)
{
}


void
Workspace::GetSettings(BMessage& settings)
{
}


void
Workspace::_SetDefaults()
{
	fColor.SetColor(kDefaultColor);
}

