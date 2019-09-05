/*
 * Controller.cpp - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
 * Copyright (C) 2007 Stephan Aßmus <superstippi@gmx.de>
 * Copyright (C) 2008-2009 Fredrik Modéen 	<[FirstName]@[LastName].se> (MIT ok)
 *
 * Released under the terms of the MIT license.
 */


#include "ControllerView.h"

#include <Autolock.h>
#include <Message.h>
#include <stdio.h>
#include <string.h>

#include "Controller.h"
#include "Playlist.h"
#include "PlaylistObserver.h"


ControllerView::ControllerView(BRect frame, Controller* controller,
		Playlist* playlist)
	:
	TransportControlGroup(frame, true, true, false),
	fController(controller),
	fPlaylist(playlist),
	fPlaylistObserver(new PlaylistObserver(this))
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
ControllerView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_PLAYLIST_ITEM_ADDED:
		case MSG_PLAYLIST_ITEM_REMOVED:
		case MSG_PLAYLIST_ITEMS_SORTED:
		case MSG_PLAYLIST_CURRENT_ITEM_CHANGED:
			_CheckSkippable();
			break;

		case MSG_PLAYLIST_IMPORT_FAILED:
			break;

		default:
			TransportControlGroup::MessageReceived(message);
			break;
	}
}


// #pragma mark -


void
ControllerView::TogglePlaying()
{
	BAutolock _(fPlaylist);
	if (fPlaylist->CurrentItemIndex() == fPlaylist->CountItems() - 1
		&& Position() == 1.0) {
		// Reached end of playlist and end of last item
		// -> start again from the first item.
		fPlaylist->SetCurrentItemIndex(0, true);
	} else
		fController->TogglePlaying();
}


void
ControllerView::Stop()
{
	fController->Stop();
}


void
ControllerView::Rewind()
{
	// TODO: make it so this function is called repeatedly
	//printf("ControllerView::Rewind()\n");
}


void
ControllerView::Forward()
{
	// TODO: make it so this function is called repeatedly
	//printf("ControllerView::Forward()\n");
}


void
ControllerView::SkipBackward()
{
	BAutolock _(fPlaylist);
	int32 index = fPlaylist->CurrentItemIndex() - 1;
	if (index < 0)
		index = 0;
	fPlaylist->SetCurrentItemIndex(index, true);
}


void
ControllerView::SkipForward()
{
	BAutolock _(fPlaylist);
	int32 index = fPlaylist->CurrentItemIndex() + 1;
	if (index >= fPlaylist->CountItems())
		index = fPlaylist->CountItems() - 1;
	fPlaylist->SetCurrentItemIndex(index, true);
}


void
ControllerView::VolumeChanged(float value)
{
	fController->SetVolume(value);
}


void
ControllerView::ToggleMute()
{
	fController->ToggleMute();
}


void
ControllerView::PositionChanged(float value)
{
	// 0.0 ... 1.0
	fController->SetPosition(value);
}


// #pragma mark -


void
ControllerView::_CheckSkippable()
{
	BAutolock _(fPlaylist);

	bool canSkipNext, canSkipPrevious;
	fPlaylist->GetSkipInfo(&canSkipPrevious, &canSkipNext);
	SetSkippable(canSkipPrevious, canSkipNext);
}
