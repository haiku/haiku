/*
 * Controller.h - Media Player for the Haiku Operating System
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
#ifndef __CONTROLLER_H
#define __CONTROLLER_H

#include <MediaDefs.h>
#include <MediaNode.h>

class VideoView;

class Controller
{
public:
							Controller();
	virtual 				~Controller();
	
	status_t				SetTo(const entry_ref &ref);

	void					SetVideoView(VideoView *view);
	
	bool					IsOverlayActive();

	void					LockBitmap();
	void					UnlockBitmap();
	BBitmap *				Bitmap();
	
	void					VolumeUp();
	void					VolumeDown();
	
	int						VideoTrackCount();
	int						AudioTrackCount();

private:

private:
	VideoView *				fVideoView;
};


#endif
