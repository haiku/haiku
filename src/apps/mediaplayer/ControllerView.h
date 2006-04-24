/*
 * Controller.cpp - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
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
class Player;

class ControllerView : public TransportControlGroup
{
public:
					ControllerView(BRect frame, Controller *ctrl, Playlist *pl, Player *p);
					~ControllerView();
					
private:
	void			AttachedToWindow();
	void			MessageReceived(BMessage *msg);
	void			Draw(BRect updateRect);

	// TransportControlGroup interface
	virtual	uint32	EnabledButtons();
	virtual	void	TogglePlaying();
	virtual	void	Stop();
	virtual	void	Rewind();
	virtual	void	Forward();
	virtual	void	SkipBackward();
	virtual	void	SkipForward();
	virtual	void	SetVolume(float value);
	virtual	void	ToggleMute();
	
private:
	Controller *	fController;
	Playlist *		fPlaylist;
	Player *		fPlayer;
};

#endif
