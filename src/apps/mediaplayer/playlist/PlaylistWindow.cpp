/*
 * Copyright 2007-2009, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus 	<superstippi@gmx.de>
 *		Fredrik Modéen	<fredrik@modeen.se>
 */

#include "PlaylistWindow.h"

#include <stdio.h>

#include <Alert.h>
#include <Application.h>
#include <Autolock.h>
#include <Entry.h>
#include <File.h>
#include <Roster.h>
#include <Path.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <NodeInfo.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <String.h>
#include <Box.h>
#include <Button.h>
#include <FilePanel.h>

#include "CommandStack.h"
#include "MainApp.h"
#include "PlaylistListView.h"
#include "RWLocker.h"

// TODO:
// Maintaining a playlist file on disk is a bit tricky. The playlist ref should
// be discarded when the user
// * loads a new playlist via Open,
// * loads a new playlist via dropping it on the MainWindow,
// * loads a new playlist via dropping it into the ListView while replacing
//   the contents,
// * replacing the contents by other stuff.


enum {
	// file
	M_PLAYLIST_OPEN							= 'open',
	M_PLAYLIST_SAVE							= 'save',
	M_PLAYLIST_SAVE_AS						= 'svas',
	M_PLAYLIST_SAVE_RESULT					= 'psrs',

	// edit
	M_PLAYLIST_EMPTY						= 'emty',
	M_PLAYLIST_RANDOMIZE					= 'rand',

	M_PLAYLIST_REMOVE						= 'rmov'
};

#define SPACE 5

PlaylistWindow::PlaylistWindow(BRect frame, Playlist* playlist,
		Controller* controller)
	: BWindow(frame, "Playlist", B_DOCUMENT_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS),
	  fPlaylist(playlist),
	  fLocker(new RWLocker("command stack lock")),
	  fCommandStack(new CommandStack(fLocker)),
	  fCommandStackListener(this)
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

		case M_PLAYLIST_OPEN: {
			BMessenger target(this);
			BMessage result(B_REFS_RECEIVED);
			BMessage appMessage(M_SHOW_OPEN_PANEL);
			appMessage.AddMessenger("target", target);
			appMessage.AddMessage("message", &result);
			appMessage.AddString("title", "Open Playlist");
			appMessage.AddString("label", "Open");
			be_app->PostMessage(&appMessage);
			break;
		}
		case M_PLAYLIST_SAVE: {
			if (fSavedPlaylistRef != entry_ref()) {
				_SavePlaylist(fSavedPlaylistRef);
				break;
			} else {
				// FALL THROUGH
			}
		}
		case M_PLAYLIST_SAVE_AS: {
			BMessenger target(this);
			BMessage result(M_PLAYLIST_SAVE_RESULT);
			BMessage appMessage(M_SHOW_SAVE_PANEL);
			appMessage.AddMessenger("target", target);
			appMessage.AddMessage("message", &result);
			appMessage.AddString("title", "Save Playlist");
			appMessage.AddString("label", "Save");
			be_app->PostMessage(&appMessage);
			break;
		}
		case M_PLAYLIST_SAVE_RESULT: {
			_SavePlaylist(message);
			break;
		}

		case M_PLAYLIST_EMPTY:
			fListView->RemoveAll();
			break;
		case M_PLAYLIST_RANDOMIZE:
			fListView->Randomize();
			break;
		case M_PLAYLIST_REMOVE:
			fListView->RemoveSelected();
			break;
		case M_PLAYLIST_REMOVE_AND_PUT_INTO_TRASH:
		{
printf("M_PLAYLIST_REMOVE_AND_PUT_INTO_TRASH\n");
message->PrintToStream();
			int32 index;
			if (message->FindInt32("playlist index", &index) == B_OK)
				fListView->RemoveToTrash(index);
			else
				fListView->RemoveSelectionToTrash();
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
	fileMenu->AddItem(new BMenuItem("Open"B_UTF8_ELLIPSIS,
		new BMessage(M_PLAYLIST_OPEN), 'O'));
	fileMenu->AddItem(new BMenuItem("Save As"B_UTF8_ELLIPSIS,
		new BMessage(M_PLAYLIST_SAVE_AS), 'S', B_SHIFT_KEY));
//	fileMenu->AddItem(new BMenuItem("Save",
//		new BMessage(M_PLAYLIST_SAVE), 'S'));

	fileMenu->AddSeparatorItem();

	fileMenu->AddItem(new BMenuItem("Close",
		new BMessage(B_QUIT_REQUESTED), 'W'));

	BMenu* editMenu = new BMenu("Edit");
	fUndoMI = new BMenuItem("Undo", new BMessage(B_UNDO), 'Z');
	editMenu->AddItem(fUndoMI);
	fRedoMI = new BMenuItem("Redo", new BMessage(B_REDO), 'Z', B_SHIFT_KEY);
	editMenu->AddItem(fRedoMI);
	editMenu->AddSeparatorItem();
	editMenu->AddItem(new BMenuItem("Randomize",
		new BMessage(M_PLAYLIST_RANDOMIZE), 'R'));
	editMenu->AddSeparatorItem();
	editMenu->AddItem(new BMenuItem("Remove (Del)",
		new BMessage(M_PLAYLIST_REMOVE)/*, B_DELETE, 0*/));
	editMenu->AddItem(new BMenuItem("Remove and Put into Trash",
		new BMessage(M_PLAYLIST_REMOVE_AND_PUT_INTO_TRASH), 'T'));
	editMenu->AddItem(new BMenuItem("Remove All",
		new BMessage(M_PLAYLIST_EMPTY), 'N'));

	menuBar->AddItem(editMenu);

	AddChild(menuBar);
	fileMenu->SetTargetForItems(this);
	editMenu->SetTargetForItems(this);

	menuBar->ResizeToPreferred();
	frame = Bounds();
	frame.top = menuBar->Frame().bottom + 1;
}


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


static void
display_save_alert(const char* message)
{
	BAlert* alert = new BAlert("Save Error", message, "Ok", NULL, NULL,
		B_WIDTH_AS_USUAL, B_STOP_ALERT);
	alert->Go(NULL);
}


static void
display_save_alert(status_t error)
{
	BString errorMessage("Saving the playlist failed.\n\nError: ");
	errorMessage << strerror(error);
	display_save_alert(errorMessage.String());
}


void
PlaylistWindow::_SavePlaylist(const BMessage* message)
{
	entry_ref ref;
	const char* name;
	if (message->FindRef("directory", &ref) != B_OK
		|| message->FindString("name", &name) != B_OK) {
		display_save_alert("Internal error (malformed message). "
			"Saving the Playlist failed.");
		return;
	}

	BString tempName(name);
	tempName << system_time();

	BPath origPath(&ref);
	BPath tempPath(&ref);
	if (origPath.InitCheck() != B_OK || tempPath.InitCheck() != B_OK
		|| origPath.Append(name) != B_OK
		|| tempPath.Append(tempName.String()) != B_OK) {
		display_save_alert("Internal error (out of memory). "
			"Saving the Playlist failed.");
		return;
	}

	BEntry origEntry(origPath.Path());
	BEntry tempEntry(tempPath.Path());
	if (origEntry.InitCheck() != B_OK || tempEntry.InitCheck() != B_OK) {
		display_save_alert("Internal error (out of memory). "
			"Saving the Playlist failed.");
		return;
	}

	_SavePlaylist(origEntry, tempEntry, name);
}


void
PlaylistWindow::_SavePlaylist(const entry_ref& ref)
{
	BString tempName(ref.name);
	tempName << system_time();
	entry_ref tempRef(ref);
	tempRef.set_name(tempName.String());

	BEntry origEntry(&ref);
	BEntry tempEntry(&tempRef);

	_SavePlaylist(origEntry, tempEntry, ref.name);
}


void
PlaylistWindow::_SavePlaylist(BEntry& origEntry, BEntry& tempEntry,
	const char* finalName)
{
	class TempEntryRemover {
	public:
		TempEntryRemover(BEntry* entry)
			: fEntry(entry)
		{
		}
		~TempEntryRemover()
		{
			if (fEntry)
				fEntry->Remove();
		}
		void Detach()
		{
			fEntry = NULL;
		}
	private:
		BEntry* fEntry;
	} remover(&tempEntry);

	BFile file(&tempEntry, B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
	if (file.InitCheck() != B_OK) {
		BString errorMessage("Saving the playlist failed:\n\nError: ");
		errorMessage << strerror(file.InitCheck());
		display_save_alert(errorMessage.String());
		return;
	}

	AutoLocker<Playlist> lock(fPlaylist);
	if (!lock.IsLocked()) {
		display_save_alert("Internal error (locking failed). "
			"Saving the Playlist failed.");
		return;
	}

	status_t ret = fPlaylist->Flatten(&file);
	if (ret != B_OK) {
		display_save_alert(ret);
		return;
	}
	lock.Unlock();

	if (origEntry.Exists()) {
		// TODO: copy attributes
	}

	// clobber original entry, if it exists
	tempEntry.Rename(finalName, true);
	remover.Detach();

	BNodeInfo info(&file);
	info.SetType("application/x-vnd.haiku-playlist");
}

