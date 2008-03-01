/*
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef __VIDEO_WINDOW_H
#define __VIDEO_WINDOW_H

#include <Window.h>

class VideoNode;
class VideoView;

class VideoWindow : public BWindow
{
public:

private:
	
private:
	VideoNode *		fVideoNode;
	VideoView *		fVideoView;
};

#endif
