/*
 * Copyright 2005-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "Desktop.h"
#include "Workspace.h"
#include "WorkspacePrivate.h"
#include "Window.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>


static rgb_color kDefaultColor = (rgb_color){ 51, 102, 152, 255 };


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
Workspace::Private::SetColor(const rgb_color& color)
{
	fColor = color;
}


void
Workspace::Private::RestoreConfiguration(const BMessage& settings)
{
	rgb_color color;
	if (settings.FindInt32("color", (int32 *)&color) == B_OK)
		fColor = color;

	fStoredScreenConfiguration.Restore(settings);
	fCurrentScreenConfiguration.Restore(settings);
}


/*!	\brief Store the workspace configuration in a message
*/
void
Workspace::Private::StoreConfiguration(BMessage& settings)
{
	fStoredScreenConfiguration.Store(settings);
	settings.AddInt32("color", *(int32 *)&fColor);
}


void
Workspace::Private::_SetDefaults()
{
	fColor = kDefaultColor;
}


//	#pragma mark -


Workspace::Workspace(Desktop& desktop, int32 index)
	:
	fWorkspace(desktop.WorkspaceAt(index)),
	fDesktop(desktop),
	fCurrentWorkspace(index == desktop.CurrentWorkspace())
{
	RewindWindows();
}


Workspace::~Workspace()
{
}


const rgb_color&
Workspace::Color() const
{
	return fWorkspace.Color();
}


void
Workspace::SetColor(const rgb_color& color, bool makeDefault)
{
	if (color == Color())
		return;

	fWorkspace.SetColor(color);
	fDesktop.RedrawBackground();
	if (makeDefault)
		fDesktop.StoreWorkspaceConfiguration(fWorkspace.Index());
}


status_t
Workspace::GetNextWindow(Window*& _window, BPoint& _leftTop)
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


status_t
Workspace::GetPreviousWindow(Window*& _window, BPoint& _leftTop)
{
	if (fCurrent == NULL)
		fCurrent = fWorkspace.Windows().LastWindow();
	else
		fCurrent = fCurrent->PreviousWindow(fWorkspace.Index());

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

