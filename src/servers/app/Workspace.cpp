/*
 * Copyright 2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "Desktop.h"
#include "Workspace.h"
#include "WorkspacePrivate.h"
#include "WindowLayer.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>


static RGBColor kDefaultColor = RGBColor(51, 102, 152);


Workspace::Private::Private()
{
	_SetDefaults();
}


Workspace::Private::~Private()
{
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
	if (fCurrent == NULL)
		fCurrent = fWorkspace.Windows().FirstWindow();
	else
		fCurrent = fCurrent->NextWindow(fWorkspace.Index());

	if (fCurrent == NULL)
		return B_ENTRY_NOT_FOUND;

	_window = fCurrent;

	if (fCurrentWorkspace)
		_leftTop = fCurrent->Frame().LeftTop();
	else
		_leftTop = fCurrent->Anchor(fWorkspace.Index()).position;

	return B_OK;
}


void
Workspace::RewindWindows()
{
	fCurrent = NULL;
}

