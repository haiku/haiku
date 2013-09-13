/* PatchWin.cpp
 * ------------
 * Implements the main PatchBay window class.
 *
 * Copyright 2013, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Revisions by Pete Goodeve
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */
 
#include "PatchWin.h"

#include <Application.h>

#include "PatchView.h"


PatchWin::PatchWin()
	:
	BWindow(BRect(50, 50, 450, 450), "Patch Bay", B_TITLED_WINDOW, 0)
{
	BRect r = Bounds();
	fPatchView = new PatchView(r);
	AddChild(fPatchView);
	Show();
}


bool
PatchWin::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
