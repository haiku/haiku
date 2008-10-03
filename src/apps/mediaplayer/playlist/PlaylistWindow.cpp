/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus 	<superstippi@gmx.de>
 *		Fredrik Modéen	<fredrik@modeen.se>
 */

#include "PlaylistWindow.h"

#include <stdio.h>

#include <Application.h>
#include <Roster.h>
#include <Path.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <String.h>
#include <Box.h>
#include <Button.h>
#include <FilePanel.h>

#include "CommandStack.h"
#include "PlaylistListView.h"
#include "RWLocker.h"

#define DEBUG 1

enum {
	// file
	M_PLAYLIST_OPEN			= 'open',
	M_PLAYLIST_SAVE			= 'save',

	// edit
	M_PLAYLIST_EMPTY		= 'emty',
	M_PLAYLIST_RANDOMIZE	= 'rand'
};

#define SPACE 5

PlaylistWindow::PlaylistWindow(BRect frame, Playlist* playlist,
		Controller* controller)
	: BWindow(frame, "Playlist", B_DOCUMENT_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS)
	, fOpenPanel(NULL)
	, fSavePanel(NULL)
	, fLocker(new RWLocker("command stack lock"))
	, fCommandStack(new CommandStack(fLocker))
	, fCommandStackListener(this)
{
	frame = Bounds();

	_CreateMenu(frame);
		// will adjust frame to account for menubar

	frame.right -= B_V_SCROLL_BAR_WIDTH;
	fListView = new PlaylistListView(frame, playlist, controller,
		fCommandStack);

	BScrollView* scrollView = new BScrollView("playlist scrollview", fListView,
		B_FOLLOW_ALL_SIDES, 0, false, true, B_NO_BORDER);

	fTopView = 	scrollView;
	AddChild(fTopView);

	// small visual tweak
	if (BScrollBar* scrollBar = scrollView->ScrollBar(B_VERTICAL)) {
		// make it so the frame of the menubar is also the frame of
		// the scroll bar (appears to be)
		scrollBar->MoveBy(0, -1);
		scrollBar->ResizeBy(0, -(B_H_SCROLL_BAR_HEIGHT - 2));
	}

	fCommandStack->AddListener(&fCommandStackListener);
	_ObjectChanged(fCommandStack);
}


PlaylistWindow::~PlaylistWindow()
{
	// give listeners a chance to detach themselves
	fTopView->RemoveSelf();
	delete fTopView;

	fCommandStack->RemoveListener(&fCommandStackListener);
	delete fCommandStack;
	delete fLocker;

	delete fOpenPanel;
	delete fSavePanel;
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

		case B_UNDO:
			fCommandStack->Undo();
			break;
		case B_REDO:
			fCommandStack->Redo();
			break;

		case MSG_OBJECT_CHANGED: {
			Notifier* notifier;
			if (message->FindPointer("object", (void**)&notifier) == B_OK)
				_ObjectChanged(notifier);
			break;
		}

		case B_REFS_RECEIVED:
		case B_SIMPLE_DATA: {
			// only accept this message when it comes from the
			// player window, _not_ when it is dropped in this window
			// outside of the playlist!
			int32 appendIndex;
			if (message->FindInt32("append_index", &appendIndex) == B_OK) {
				fListView->RefsReceived(message, appendIndex);
			}
			break;
		}

		case M_PLAYLIST_SAVE:
			if (!fSavePanel)
				fSavePanel = new BFilePanel(B_SAVE_PANEL);
			fSavePanel->Show();
			break;
		case B_SAVE_REQUESTED:
			printf("We are saving\n");
			//Use fListView and have a SaveToFile?
			break;
		case M_PLAYLIST_OPEN:
			if (!fOpenPanel)
				fOpenPanel = new BFilePanel(B_OPEN_PANEL);
			fOpenPanel->Show();
			break;

		case M_PLAYLIST_EMPTY:
			fListView->RemoveAll();
			break;
		case M_PLAYLIST_RANDOMIZE:
			fListView->Randomize();
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


// #pragma mark -


void
PlaylistWindow::_CreateMenu(BRect& frame)
{
	frame.bottom = 15;
	BMenuBar* menuBar = new BMenuBar(frame, "main menu");
	BMenu* fileMenu = new BMenu("Playlist");
	menuBar->AddItem(fileMenu);
//	fileMenu->AddItem(new BMenuItem("Open"B_UTF8_ELLIPSIS,
//		new BMessage(M_PLAYLIST_OPEN), 'O'));
//	fileMenu->AddItem(new BMenuItem("Save"B_UTF8_ELLIPSIS,
//		new BMessage(M_PLAYLIST_SAVE), 'S'));
//	fileMenu->AddSeparatorItem();
	fileMenu->AddItem(new BMenuItem("Close",
		new BMessage(B_QUIT_REQUESTED), 'W'));

	BMenu* editMenu = new BMenu("Edit");
	fUndoMI = new BMenuItem("Undo", new BMessage(B_UNDO), 'Z');
	editMenu->AddItem(fUndoMI);
	fRedoMI = new BMenuItem("Redo", new BMessage(B_REDO), 'Z', B_SHIFT_KEY);
	editMenu->AddItem(fRedoMI);
	editMenu->AddSeparatorItem();
	editMenu->AddItem(new BMenuItem("Make Empty",
		new BMessage(M_PLAYLIST_EMPTY), 'N'));
	editMenu->AddItem(new BMenuItem("Randomize",
		new BMessage(M_PLAYLIST_RANDOMIZE), 'R'));
	menuBar->AddItem(editMenu);

	AddChild(menuBar);
	fileMenu->SetTargetForItems(this);
	editMenu->SetTargetForItems(this);

	menuBar->ResizeToPreferred();
	frame = Bounds();
	frame.top = menuBar->Frame().bottom + 1;
}


// _ObjectChanged
void
PlaylistWindow::_ObjectChanged(const Notifier* object)
{
	if (object == fCommandStack) {
		// relable Undo item and update enabled status
		BString label("Undo");
		fUndoMI->SetEnabled(fCommandStack->GetUndoName(label));
		if (fUndoMI->IsEnabled())
			fUndoMI->SetLabel(label.String());
		else
			fUndoMI->SetLabel("<nothing to undo>");

		// relable Redo item and update enabled status
		label.SetTo("Redo");
		fRedoMI->SetEnabled(fCommandStack->GetRedoName(label));
		if (fRedoMI->IsEnabled())
			fRedoMI->SetLabel(label.String());
		else
			fRedoMI->SetLabel("<nothing to redo>");
	}
}

