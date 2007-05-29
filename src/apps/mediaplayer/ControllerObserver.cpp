/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "ControllerObserver.h"

#include <Message.h>


ControllerObserver::ControllerObserver(BHandler* target, uint32 observeFlags)
	: Controller::Listener()
	, AbstractLOAdapter(target)
	, fObserveFlags(observeFlags)
{
}


ControllerObserver::~ControllerObserver()
{
}


void
ControllerObserver::FileFinished()
{
	if (!(fObserveFlags & OBSERVE_FILE_CHANGES))
		return;

	BMessage message(MSG_CONTROLLER_FILE_FINISHED);

	DeliverMessage(message);
}


void
ControllerObserver::FileChanged()
{
	if (!(fObserveFlags & OBSERVE_FILE_CHANGES))
		return;

	BMessage message(MSG_CONTROLLER_FILE_CHANGED);

	DeliverMessage(message);
}


void
ControllerObserver::VideoTrackChanged(int32 index)
{
	if (!(fObserveFlags & OBSERVE_TRACK_CHANGES))
		return;

	BMessage message(MSG_CONTROLLER_VIDEO_TRACK_CHANGED);
	message.AddInt32("index", index);

	DeliverMessage(message);
}


void
ControllerObserver::AudioTrackChanged(int32 index)
{
	if (!(fObserveFlags & OBSERVE_TRACK_CHANGES))
		return;

	BMessage message(MSG_CONTROLLER_AUDIO_TRACK_CHANGED);
	message.AddInt32("index", index);

	DeliverMessage(message);
}


void
ControllerObserver::VideoStatsChanged()
{
	if (!(fObserveFlags & OBSERVE_STAT_CHANGES))
		return;

	BMessage message(MSG_CONTROLLER_VIDEO_STATS_CHANGED);

	DeliverMessage(message);
}


void
ControllerObserver::AudioStatsChanged()
{
	if (!(fObserveFlags & OBSERVE_STAT_CHANGES))
		return;

	BMessage message(MSG_CONTROLLER_AUDIO_STATS_CHANGED);

	DeliverMessage(message);
}


void
ControllerObserver::PlaybackStateChanged(uint32 state)
{
	if (!(fObserveFlags & OBSERVE_PLAYBACK_STATE_CHANGES))
		return;

	BMessage message(MSG_CONTROLLER_PLAYBACK_STATE_CHANGED);
	message.AddInt32("state", state);

	DeliverMessage(message);
}


void
ControllerObserver::PositionChanged(float position)
{
	if (!(fObserveFlags & OBSERVE_POSITION_CHANGES))
		return;

	BMessage message(MSG_CONTROLLER_POSITION_CHANGED);
	message.AddFloat("position", position);

	DeliverMessage(message);
}


void
ControllerObserver::VolumeChanged(float volume)
{
	if (!(fObserveFlags & OBSERVE_VOLUME_CHANGES))
		return;

	BMessage message(MSG_CONTROLLER_VOLUME_CHANGED);
	message.AddFloat("volume", volume);

	DeliverMessage(message);
}


