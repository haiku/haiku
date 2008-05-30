/*	
 * Copyright (c) 2000-2008, Ingo Weinhold <ingo_weinhold@gmx.de>,
 * Copyright (c) 2000-2008, Stephan Aßmus <superstippi@gmx.de>,
 * All Rights Reserved. Distributed under the terms of the MIT license.
 */
#ifndef PLAYBACK_LO_ADAPTER_H
#define PLAYBACK_LO_ADAPTER_H


#include "AbstractLOAdapter.h"
#include "PlaybackListener.h"


enum {
	MSG_PLAYBACK_PLAY_MODE_CHANGED			= 'ppmc',
	MSG_PLAYBACK_LOOP_MODE_CHANGED			= 'plmc',
	MSG_PLAYBACK_LOOPING_ENABLED_CHANGED	= 'plec',
	MSG_PLAYBACK_VIDEO_BOUNDS_CHANGED		= 'pmbc',
	MSG_PLAYBACK_FPS_CHANGED				= 'pfps',
	MSG_PLAYBACK_CURRENT_FRAME_CHANGED		= 'pcfc',
	MSG_PLAYBACK_SPEED_CHANGED				= 'pspc',
	MSG_PLAYBACK_FRAME_DROPPED				= 'pfdr',
};


class PlaybackLOAdapter : public AbstractLOAdapter, public PlaybackListener {
 public:
								PlaybackLOAdapter(BHandler* handler);
								PlaybackLOAdapter(
									const BMessenger& messenger);
	virtual						~PlaybackLOAdapter();

	virtual	void				PlayModeChanged(int32 mode);
	virtual	void				LoopModeChanged(int32 mode);
	virtual	void				LoopingEnabledChanged(bool enabled);
	virtual	void				VideoBoundsChanged(BRect bounds);
	virtual	void				FramesPerSecondChanged(float fps);
	virtual	void				CurrentFrameChanged(double frame);
	virtual	void				SpeedChanged(float speed);
	virtual	void				FrameDropped();
};

#endif	// PLAYBACK_LO_ADAPTER_H
