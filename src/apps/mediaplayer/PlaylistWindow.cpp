/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#include "PlaylistWindow.h"

#include <ScrollBar.h>
#include <ScrollView.h>

#include "PlaylistListView.h"


PlaylistWindow::PlaylistWindow(BRect frame, Playlist* playlist,
		Controller* controller)
	: BWindow(frame, "Playlist", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	frame = Bounds();
	frame.right -= B_V_SCROLL_BAR_WIDTH;
	PlaylistListView* listView = new PlaylistListView(frame, playlist,
		controller);

	fTopView = new BScrollView("playlist scrollview",
		listView, B_FOLLOW_ALL, 0, false, true, B_NO_BORDER);

	AddChild(fTopView);
}


PlaylistWindow::~PlaylistWindow()
{
	// give listeners a chance to detach themselves
	fTopView->RemoveSelf();
	delete fTopView;
}


bool
PlaylistWindow::QuitRequested()
{
	Hide();
	return false;
}


void
PlaylistWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_MODIFIERS_CHANGED:
			if (LastMouseMovedView())
				PostMessage(message, LastMouseMovedView());
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}

