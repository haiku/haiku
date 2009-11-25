// PatchWin.cpp
// ------------
// Implements the main PatchBay window class.
//
// Copyright 1999, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#include <Application.h>
#include "PatchWin.h"
#include "PatchView.h"

PatchWin::PatchWin()
	: BWindow(BRect(50,50,450,450), "Patch Bay", B_TITLED_WINDOW, 0)
{
	BRect r = Bounds();
	m_patchView = new PatchView(r);
	AddChild(m_patchView);
	Show();
}

bool PatchWin::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

