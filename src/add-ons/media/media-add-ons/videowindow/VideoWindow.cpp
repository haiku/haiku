/*
 * Copyright (C) 2006-2008 Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 * Copyright (C) 2008 Maurice Kalinowski <haiku@kaldience.com>. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#include "VideoWindow.h"
#include "VideoView.h"


VideoWindow::VideoWindow(BRect size, VideoView* view)
		: BWindow(size, "Video Window", B_TITLED_WINDOW, 0)
		, fVideoView(view)
{
	if (fVideoView)
		AddChild(fVideoView);
}


VideoWindow::~VideoWindow()
{
}


void
VideoWindow::MessageReceived(BMessage *msg)
{
	if (msg)
		BWindow::MessageReceived(msg);
}
