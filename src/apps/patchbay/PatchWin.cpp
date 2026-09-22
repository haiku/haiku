/* PatchWin.cpp
 * ------------
 * Implements the main PatchBay window class.
 *
 * Copyright 2013-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Revisions by:
 * 		Pete Goodeve
 *		Philippe Houdoin
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */
 
#include "PatchWin.h"

#include "PatchApp.h"
#include "PatchView.h"

PatchWin::PatchWin()
	:
	BWindow(BRect(50, 50, 450, 450), kApplicationName, B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE)
{
	BRect r = Bounds();
	fPatchView = new PatchView(r);
	AddChild(fPatchView);
	Show();
}
