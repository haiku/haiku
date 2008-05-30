/*	
 * Copyright (c) 2000-2008, Ingo Weinhold <ingo_weinhold@gmx.de>,
 * Copyright (c) 2000-2008, Stephan Aßmus <superstippi@gmx.de>,
 * All Rights Reserved. Distributed under the terms of the MIT license.
 */
#include <Message.h>

#include "PlaybackLOAdapter.h"


PlaybackLOAdapter::PlaybackLOAdapter(BHandler* handler)
	: AbstractLOAdapter(handler)
{
}


PlaybackLOAdapter::PlaybackLOAdapter(const BMessenger& messenger)
	: AbstractLOAdapter(messenger)
{
}


PlaybackLOAdapter::~PlaybackLOAdapter()
{
}


void
PlaybackLOAdapter::PlayModeChanged(int32 mode)
{
	BMessage message(MSG_PLAYBACK_PLAY_MODE_CHANGED);
	message.AddInt32("play mode", mode);
	DeliverMessage(message);
}


void
PlaybackLOAdapter::LoopModeChanged(int32 mode)
{
	BMessage message(MSG_PLAYBACK_LOOP_MODE_CHANGED);
	message.AddInt32("loop mode", mode);
	DeliverMessage(message);
}


void
PlaybackLOAdapter::LoopingEnabledChanged(bool enabled)
{
	BMessage message(MSG_PLAYBACK_LOOPING_ENABLED_CHANGED);
	message.AddBool("looping enabled", enabled);
	DeliverMessage(message);
}


void
PlaybackLOAdapter::VideoBoundsChanged(BRect bounds)
{
	BMessage message(MSG_PLAYBACK_VIDEO_BOUNDS_CHANGED);
	message.AddRect("video bounds", bounds);
	DeliverMessage(message);
}


void
PlaybackLOAdapter::FramesPerSecondChanged(float fps)
{
	BMessage message(MSG_PLAYBACK_FPS_CHANGED);
	message.AddFloat("fps", fps);
	DeliverMessage(message);
}


void
PlaybackLOAdapter::CurrentFrameChanged(double frame)
{
	BMessage message(MSG_PLAYBACK_CURRENT_FRAME_CHANGED);
	message.AddDouble("current frame", frame);
	DeliverMessage(message);
}


void
PlaybackLOAdapter::SpeedChanged(float speed)
{
	BMessage message(MSG_PLAYBACK_SPEED_CHANGED);
	message.AddFloat("speed", speed);
	DeliverMessage(message);
}


void
PlaybackLOAdapter::FrameDropped()
{
	DeliverMessage(MSG_PLAYBACK_FRAME_DROPPED);
}

