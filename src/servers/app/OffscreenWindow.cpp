/*
 * Copyright 2005-2008, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "OffscreenWindow.h"

#include <new>
#include <stdio.h>

#include <Debug.h>

#include <WindowPrivate.h>

#include "BitmapHWInterface.h"
#include "DrawingEngine.h"
#include "ServerBitmap.h"

using std::nothrow;


OffscreenWindow::OffscreenWindow(ServerBitmap* bitmap,
		const char* name, ::ServerWindow* window)
	: Window(bitmap->Bounds(), name,
			B_NO_BORDER_WINDOW_LOOK, kOffscreenWindowFeel,
			0, 0, window, new (nothrow) DrawingEngine()),
	fBitmap(bitmap),
	fHWInterface(new (nothrow) BitmapHWInterface(fBitmap))
{
	if (!fHWInterface || !GetDrawingEngine())
		return;

	fHWInterface->Initialize();
	GetDrawingEngine()->SetHWInterface(fHWInterface);

	fVisibleRegion.Set(fFrame);
	fVisibleContentRegion.Set(fFrame);
	fVisibleContentRegionValid = true;
	fContentRegion.Set(fFrame);
	fContentRegionValid = true;
}


OffscreenWindow::~OffscreenWindow()
{
	if (GetDrawingEngine())
		GetDrawingEngine()->SetHWInterface(NULL);

	if (fHWInterface) {
		fHWInterface->LockExclusiveAccess();
		fHWInterface->Shutdown();
		fHWInterface->UnlockExclusiveAccess();
		delete fHWInterface;
	}
}

