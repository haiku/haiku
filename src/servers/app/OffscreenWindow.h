/*
 * Copyright 2005-2008, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef OFFSCREEN_WINDOW_H
#define OFFSCREEN_WINDOW_H


#include "Window.h"

#include <AutoDeleter.h>


class BitmapHWInterface;
class ServerBitmap;

class OffscreenWindow : public Window {
public:
							OffscreenWindow(ServerBitmap* bitmap,
								const char* name, ::ServerWindow* window);
	virtual					~OffscreenWindow();

	virtual	bool			IsOffscreenWindow() const
								{ return true; }

private:
	ServerBitmap*			fBitmap;
	ObjectDeleter<BitmapHWInterface>
							fHWInterface;
};

#endif	// OFFSCREEN_WINDOW_H
