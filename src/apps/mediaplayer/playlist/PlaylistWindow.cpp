/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#include "PlaylistWindow.h"

#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <String.h>

#include "CommandStack.h"
#include "PlaylistListView.h"
#include "RWLocker.h"


PlaylistWindow::PlaylistWindow(BRect frame, Playlist* playlist,
		Controller* controller)
	: BWindow(frame, "Playlist", B_DOCUMENT_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS)

	, fLocker(new RWLocker("command stack lock"))
	, fCommandStack(new CommandStack(fLocker))
	, fCommandStackListener(this)
{
	frame = Bounds();

	_CreateMenu(frame);

	frame.right -= B_V_SCROLL_BAR_WIDTH;
	fListView = new PlaylistListView(frame, playlist, controller,
		fCommandStack);

	BScrollView* scrollView = new BScrollView("playlist scrollview",
		fListView, B_FOLLOW_ALL, 0, false, true, B_NO_BORDER);

	fTopView = scrollView;
	AddChild(fTopView);

	// small visual tweak
	if (BScrollBar* scrollBar = scrollView->ScrollBar(B_VERTICAL)) {
		// make it so the frame of the menubar is also the frame of
		// the scroll bar (appears to be)
		scrollBar->MoveBy(0, -1);
		scrollBar->ResizeBy(0, 1);
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
	// TODO add some items: "Open", "Save", "Make Empty"...

	BMenu* editMenu = new BMenu("Edit");
	BMessage* message = new BMessage(B_UNDO);
	fUndoMI = new BMenuItem("Undo", message);
	editMenu->AddItem(fUndoMI);
	message = new BMessage(B_REDO);
	fRedoMI = new BMenuItem("Undo", message);
	editMenu->AddItem(fRedoMI);
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

