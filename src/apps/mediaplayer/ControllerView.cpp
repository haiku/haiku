/*
 * Controller.cpp - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
 * Copyright (C) 2007 Stephan Aßmus <superstippi@gmx.de>
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
#include "ControllerView.h"

#include <Message.h>
#include <stdio.h>
#include <string.h>

#include "Controller.h"
#include "Playlist.h"
#include "PlaylistObserver.h"


ControllerView::ControllerView(BRect frame, Controller* controller,
		Playlist* playlist)
 :	TransportControlGroup(frame)
 ,	fController(controller)
 ,	fPlaylist(playlist)
 ,	fPlaylistObserver(new PlaylistObserver(this))
{
	fPlaylist->AddListener(fPlaylistObserver);
}


ControllerView::~ControllerView()
{
	fPlaylist->RemoveListener(fPlaylistObserver);
	delete fPlaylistObserver;
}


void
ControllerView::AttachedToWindow()
{
	TransportControlGroup::AttachedToWindow();
}


void
ControllerView::Draw(BRect updateRect)
{
	TransportControlGroup::Draw(updateRect);
}


void
ControllerView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case MSG_PLAYLIST_REF_ADDED:
		case MSG_PLAYLIST_REF_REMOVED:
			CheckSkippable();
			break;

		default:
			TransportControlGroup::MessageReceived(msg);
	}
}

// #pragma mark -


uint32
ControllerView::EnabledButtons()
{
	// TODO: superflous
	return 0xffffffff;
}


void
ControllerView::TogglePlaying()
{
	printf("ControllerView::TogglePlaying()\n");
	if (fController->IsPaused() || fController->IsStopped())
		fController->Play();
	else
		fController->Pause();
}


void
ControllerView::Stop()
{
	printf("ControllerView::Stop()\n");
	fController->Stop();
}


void
ControllerView::Rewind()
{
	// TODO: make it so this function is called repeatedly
	printf("ControllerView::Rewind()\n");
}


void
ControllerView::Forward()
{
	// TODO: make it so this function is called repeatedly
	printf("ControllerView::Forward()\n");
}


void
ControllerView::SkipBackward()
{
	printf("ControllerView::SkipBackward()\n");
	fPlaylist->SetCurrentRefIndex(fPlaylist->CurrentRefIndex() - 1);
}


void
ControllerView::SkipForward()
{
	printf("ControllerView::SkipForward()\n");
	fPlaylist->SetCurrentRefIndex(fPlaylist->CurrentRefIndex() + 1);
}


void
ControllerView::VolumeChanged(float value)
{
	printf("ControllerView::VolumeChanged(%.4f)\n", value);
	fController->SetVolume(value);
}


void
ControllerView::ToggleMute()
{
	printf("ControllerView::ToggleMute()\n");
}


void
ControllerView::PositionChanged(float value)
{
	printf("ControllerView::PositionChanged(%.2f)\n", value);
	// 0.0 ... 1.0
	fController->SetPosition(value);
}


// #pragma mark -


void
ControllerView::CheckSkippable()
{
	bool canSkipNext, canSkipPrevious;
	fPlaylist->GetSkipInfo(&canSkipPrevious, &canSkipNext);
	SetSkippable(canSkipPrevious, canSkipNext);
}


