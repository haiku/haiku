/*
 * Copyright 2007-2009, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef CONTROLLER_OBSERVER_H
#define CONTROLLER_OBSERVER_H


#include "AbstractLOAdapter.h"
#include "Controller.h"


enum {
	MSG_CONTROLLER_FILE_FINISHED			= 'cnff',
	MSG_CONTROLLER_FILE_CHANGED				= 'cnfc',

	MSG_CONTROLLER_VIDEO_TRACK_CHANGED		= 'cnvt',
	MSG_CONTROLLER_AUDIO_TRACK_CHANGED		= 'cnat',
	MSG_CONTROLLER_SUB_TITLE_TRACK_CHANGED	= 'cnst',

	MSG_CONTROLLER_VIDEO_STATS_CHANGED		= 'cnvs',
	MSG_CONTROLLER_AUDIO_STATS_CHANGED		= 'cnas',

	MSG_CONTROLLER_PLAYBACK_STATE_CHANGED	= 'cnps',
	MSG_CONTROLLER_POSITION_CHANGED			= 'cnpc',
	MSG_CONTROLLER_SEEK_HANDLED				= 'cnsh',
	MSG_CONTROLLER_VOLUME_CHANGED			= 'cnvc',
	MSG_CONTROLLER_MUTED_CHANGED			= 'cnmc'
};

enum {
	OBSERVE_FILE_CHANGES					= 0x0001,
	OBSERVE_TRACK_CHANGES					= 0x0002,
	OBSERVE_STAT_CHANGES					= 0x0004,
	OBSERVE_PLAYBACK_STATE_CHANGES			= 0x0008,
	OBSERVE_POSITION_CHANGES				= 0x0010,
	OBSERVE_VOLUME_CHANGES					= 0x0020,

	OBSERVE_ALL_CHANGES						= 0xffff
};


class ControllerObserver
	: public Controller::Listener, public AbstractLOAdapter {
public:
						ControllerObserver(BHandler* target,
							uint32 observeFlags = OBSERVE_ALL_CHANGES);
	virtual				~ControllerObserver();

	// Controller::Listener interface
	virtual	void		FileFinished();
	virtual	void		FileChanged(PlaylistItem* item, status_t result);

	virtual	void		VideoTrackChanged(int32 index);
	virtual	void		AudioTrackChanged(int32 index);
	virtual	void		SubTitleTrackChanged(int32 index);

	virtual	void		VideoStatsChanged();
	virtual	void		AudioStatsChanged();

	virtual	void		PlaybackStateChanged(uint32 state);
	virtual	void		PositionChanged(float position);
	virtual	void		SeekHandled(int64 seekFrame);
	virtual	void		VolumeChanged(float volume);
	virtual	void		MutedChanged(bool muted);

private:
			uint32		fObserveFlags;
};

#endif // CONTROLLER_OBSERVER_H
