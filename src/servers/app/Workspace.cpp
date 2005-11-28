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

#include <stdio.h>
#include <stdlib.h>


static RGBColor kDefaultColor = RGBColor(51, 102, 152);


Workspace::Workspace()
	:
	fWindows(20)
{
	_SetDefaults();
}


Workspace::~Workspace()
{
}


void
Workspace::SetWindows(const BObjectList<WindowLayer>& windows)
{
	fWindows.MakeEmpty();
	fWindows.AddList((BObjectList<WindowLayer> *)&windows);
}


bool
Workspace::AddWindow(WindowLayer* window)
{
	return fWindows.AddItem(window);
}


void
Workspace::RemoveWindow(WindowLayer* window)
{
	fWindows.RemoveItem(window);
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

