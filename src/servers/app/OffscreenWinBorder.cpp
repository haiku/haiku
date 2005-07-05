/*
 * Copyright 2005, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:  Stephan Aßmus <superstippi@gmx.de>
 */

#include <stdio.h>

#include <Debug.h>
#include "DebugInfoManager.h"

#include "BitmapHWInterface.h"
#include "DisplayDriverPainter.h"
#include "ServerBitmap.h"

#include "OffscreenWinBorder.h"


// constructor
OffscreenWinBorder::OffscreenWinBorder(ServerBitmap* bitmap,
									   const char* name,
									   ServerWindow* window)
	: WinBorder(bitmap->Bounds(), name,
				B_NO_BORDER_WINDOW_LOOK,
				B_NORMAL_WINDOW_FEEL,
				0, 0, window,
				new DisplayDriverPainter()),
	  fBitmap(bitmap),
	  fHWInterface(new BitmapHWInterface(fBitmap))
{
	fDriver->SetHWInterface(fHWInterface);
	fDriver->Initialize();
	fDriver->Update();

#ifndef NEW_CLIPPING
	fFull.Set(fFrame);
#endif
}

// destructor
OffscreenWinBorder::~OffscreenWinBorder()
{
	fHWInterface->WriteLock();
	// Unlike normal Layers, we own the DisplayDriver instance
	fDriver->Shutdown();
	delete fDriver;
	fHWInterface->Shutdown();
	fHWInterface->WriteUnlock();
	delete fHWInterface;
}

void
OffscreenWinBorder::Draw(const BRect &r)
{
	// Nothing to do here
}

void
OffscreenWinBorder::MoveBy(float x, float y)
{
	// Nothing to do here
}

void
OffscreenWinBorder::ResizeBy(float x, float y)
{
	// Nothing to do here
}

// SetTopLayer
void
OffscreenWinBorder::SetTopLayer(Layer* layer)
{
	WinBorder::SetTopLayer(layer);

#ifndef NEW_CLIPPING
fTopLayer->fFull.Set(fFrame.OffsetToCopy(0.0, 0.0));
fTopLayer->fHidden	= false;
fTopLayer->fVisible = fTopLayer->fFull;
#endif
}

