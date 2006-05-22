/*
 * VideoView.h - Media Player for the Haiku Operating System
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
#ifndef __VIDEO_VIEW_H
#define __VIDEO_VIEW_H

#include <View.h>
#include "Controller.h"

class VideoView : public BView
{
public:
					VideoView(BRect frame, const char *name, uint32 resizeMask, uint32 flags);
					~VideoView();

	void			SetController(Controller *controller);
	
	void			RemoveVideoDisplay();
	void			RemoveOverlay();
					
	bool			IsOverlaySupported();

	void			OverlayLockAcquire();
	void			OverlayLockRelease();

	void			OverlayScreenshotPrepare();
	void			OverlayScreenshotCleanup();
	
	void			DrawFrame();

private:
	void			AttachedToWindow();
	void			MessageReceived(BMessage *msg);
	void			Draw(BRect updateRect);
	
private:
	Controller *	fController;
	bool			fOverlayActive;
	rgb_color		fOverlayKeyColor;
};

#endif
