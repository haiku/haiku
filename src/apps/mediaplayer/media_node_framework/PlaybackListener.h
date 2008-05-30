/*	
 * Copyright (c) 2000-2008, Ingo Weinhold <ingo_weinhold@gmx.de>,
 * Copyright (c) 2000-2008, Stephan Aßmus <superstippi@gmx.de>,
 * All Rights Reserved. Distributed under the terms of the MIT license.
 */

/*!	This class listens to a PlaybackManager
	
	The hooks are called by PlaybackManager after it executed a command,
	to keep every listener informed. FrameDropped() is something the nodes
	can call and it is passed onto the contollers, so that they can respond
	by displaying some kind of warning. */

#ifndef PLAYBACK_LISTENER_H
#define PLAYBACK_LISTENER_H

#include <Rect.h>
#include <SupportDefs.h>

class PlaybackListener {
 public:
								PlaybackListener();
	virtual						~PlaybackListener();

	virtual	void				PlayModeChanged(int32 mode);
	virtual	void				LoopModeChanged(int32 mode);
	virtual	void				LoopingEnabledChanged(bool enabled);

	virtual	void				VideoBoundsChanged(BRect bounds);
	virtual	void				FramesPerSecondChanged(float fps);
	virtual	void				SpeedChanged(float speed);

	virtual	void				CurrentFrameChanged(double frame);

	virtual	void				FrameDropped();
};


#endif	// PLAYBACK_LISTENER_H
