/*	
 * Copyright (c) 2000-2008, Ingo Weinhold <ingo_weinhold@gmx.de>,
 * Copyright (c) 2000-2008, Stephan Aßmus <superstippi@gmx.de>,
 * All Rights Reserved. Distributed under the terms of the MIT license.
 */
#include "PlaybackListener.h"

#include <stdio.h>


PlaybackListener::PlaybackListener()
{
}


PlaybackListener::~PlaybackListener()
{
}


void
PlaybackListener::PlayModeChanged(int32 mode)
{
}


void
PlaybackListener::LoopModeChanged(int32 mode)
{
}


void
PlaybackListener::LoopingEnabledChanged(bool enabled)
{
}


void
PlaybackListener::VideoBoundsChanged(BRect bounds)
{
}


void
PlaybackListener::FramesPerSecondChanged(float fps)
{
}


void
PlaybackListener::CurrentFrameChanged(double frame)
{
}


void
PlaybackListener::SpeedChanged(float speed)
{
}


void
PlaybackListener::FrameDropped()
{
}


