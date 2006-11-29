/*
 * Copyright 2005, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "BitmapHWInterface.h"
#include "DrawingEngine.h"
#include "OffscreenWindowLayer.h"
#include "ServerBitmap.h"

#include <Debug.h>
#include "DebugInfoManager.h"

#include <stdio.h>


OffscreenWindowLayer::OffscreenWindowLayer(ServerBitmap* bitmap,
		const char* name, ::ServerWindow* window)
	: WindowLayer(bitmap->Bounds(), name,
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


OffscreenWindowLayer::~OffscreenWindowLayer()
{
	fHWInterface->WriteLock();
	// Unlike normal Layers, we own the DrawingEngine instance
	delete GetDrawingEngine();
	fHWInterface->Shutdown();
	fHWInterface->WriteUnlock();
	delete fHWInterface;
}

