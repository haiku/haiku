/*
 * Copyright (C) 2006-2008 Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 * Copyright (C) 2008 Maurice Kalinowski <haiku@kaldience.com>. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef __VIDEO_WINDOW_H
#define __VIDEO_WINDOW_H


#include <Window.h>


class VideoView;


class VideoWindow : public BWindow
{
public:
	VideoWindow(BRect size, VideoView* view);
	~VideoWindow();
	void MessageReceived(BMessage *msg);

private:
	VideoView *		fVideoView;
};

#endif
