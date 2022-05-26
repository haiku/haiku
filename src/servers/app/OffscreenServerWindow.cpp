/*
 * Copyright 2005-2008, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "OffscreenWindow.h"
#include "ServerBitmap.h"

#include "OffscreenServerWindow.h"


OffscreenServerWindow::OffscreenServerWindow(const char *title, ServerApp *app,
		port_id clientPort, port_id looperPort, int32 handlerID,
		ServerBitmap* bitmap)
	: ServerWindow(title, app, clientPort, looperPort, handlerID),
	fBitmap(bitmap, true)
{
}


OffscreenServerWindow::~OffscreenServerWindow()
{
}


void
OffscreenServerWindow::SendMessageToClient(const BMessage* msg, int32 target,
	bool usePreferred) const
{
	// We're a special kind of window. The client BWindow thread is not running,
	// so we cannot post messages to the client. In order to not mess arround
	// with all the other code, we simply make this function virtual and
	// don't do anything in this implementation.
}


Window*
OffscreenServerWindow::MakeWindow(BRect frame, const char* name,
	window_look look, window_feel feel, uint32 flags, uint32 workspace)
{
	return new OffscreenWindow(fBitmap, name, this);
}
