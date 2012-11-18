/*
 * Controller.cpp - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
 * Copyright (C) 2007 Stephan Aßmus <superstippi@gmx.de>
 * Copyright (C) 2008-2009 Fredrik Modéen 	<[FirstName]@[LastName].se> (MIT ok)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
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
