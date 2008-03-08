/*
 * Copyright 2005-2008, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "OffscreenWindow.h"

#include <stdio.h>

#include <Debug.h>

#include "BitmapHWInterface.h"
#include "DebugInfoManager.h"
#include "DrawingEngine.h"
#include "ServerBitmap.h"


OffscreenWindow::OffscreenWindow(ServerBitmap* bitmap,
		const char* name, ::ServerWindow* window)
	: Window(bitmap->Bounds(), name,
			B_NO_BORDER_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
			0, 0, window, new DrawingEngine()),
	fBitmap(bitmap),
	fHWInterface(new BitmapHWInterface(fBitmap))
{
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
	fHWInterface->LockExclusiveAccess();
	// Unlike normal Layers, we own the DrawingEngine instance
	delete GetDrawingEngine();
	fHWInterface->Shutdown();
	fHWInterface->UnlockExclusiveAccess();
	delete fHWInterface;
}

