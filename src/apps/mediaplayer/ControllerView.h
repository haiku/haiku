/*
 * Controller.cpp - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
 * Copyright (C) 2007 Stephan Aßmus <superstippi@gmx.de>
 * Copyright (C) 2008-2009 Fredrik Modéen 	<[FirstName]@[LastName].se> (MIT ok)
 *
 * Released under the terms of the MIT license.
 */
#ifndef __CONTROLLER_VIEW_H
#define __CONTROLLER_VIEW_H


#include "TransportControlGroup.h"


class Controller;
class Playlist;
class PlaylistObserver;


class ControllerView : public TransportControlGroup {
public:
						ControllerView(BRect frame, Controller* controller,
							Playlist* playlist);
						~ControllerView();

	// TransportControlGroup interface
	virtual	void		TogglePlaying();
	virtual	void		Stop();
	virtual	void		Rewind();
	virtual	void		Forward();
	virtual	void		SkipBackward();
	virtual	void		SkipForward();
	virtual	void		VolumeChanged(float value);
	virtual	void		ToggleMute();
	virtual	void		PositionChanged(float value);
	virtual	bigtime_t	TimePositionFor(float value);

private:
	void				AttachedToWindow();
	void				MessageReceived(BMessage* message);
	void				Draw(BRect updateRect);

	// ControllerView
			void		_CheckSkippable();

private:
	Controller*			fController;
	Playlist*			fPlaylist;
	PlaylistObserver*	fPlaylistObserver;
};

#endif	// __CONTROLLER_VIEW_H
