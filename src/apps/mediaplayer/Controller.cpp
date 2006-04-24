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
#include <stdio.h>
#include <string.h>
#include <Debug.h>

#include "Controller.h"
#include "VideoView.h"

void 
HandleError(const char *text, status_t err)
{
	if (err != B_OK) { 
		printf("%s. error 0x%08x (%s)\n",text, (int)err, strerror(err));
		fflush(NULL);
		exit(1);
	}
}


Controller::Controller()
 :	fVideoView(NULL)
{
}


Controller::~Controller()
{
}


void
Controller::SetVideoView(VideoView *view)
{
	fVideoView = view;
}


status_t
Controller::SetTo(const entry_ref &ref)
{
	return B_OK;
}


int
Controller::VideoTrackCount()
{
	return 3;
}


int
Controller::AudioTrackCount()
{
	return 6;
}


void
Controller::VolumeUp()
{
}


void
Controller::VolumeDown()
{
}


bool
Controller::IsOverlayActive()
{
	return true;
}


void
Controller::LockBitmap()
{
}


void
Controller::UnlockBitmap()
{
}


BBitmap *
Controller::Bitmap()
{
	return NULL;
}


void
Controller::Stop()
{
}


void
Controller::Play()
{
}


void
Controller::Pause()
{
}


bool
Controller::IsPaused()
{
	return false;
}
