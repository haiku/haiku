/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered
trademarks of Be Incorporated in the United States and other countries. Other
brand product names are registered trademarks or trademarks of their respective
holders.
All rights reserved.
*/


#include "BarWindow.h"

#include <stdio.h>

#include <Application.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Debug.h>
#include <File.h>
#include <Locale.h>
#include <MenuItem.h>
#include <MessageFilter.h>
#include <Screen.h>

#include "BarApp.h"
#include "BarMenuBar.h"
#include "BarView.h"
#include "DeskbarMenu.h"
#include "PublicCommands.h"
#include "StatusView.h"
#include "tracker_private.h"

#include <MessagePrivate.h>


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "MainWindow"


// This is a very ugly hack to be able to call the private
// BMenuBar::StartMenuBar() method from the TBarWindow::ShowBeMenu() method.
// Don't do this at home -- but why the hell is this method private?
#if __MWERKS__
	#define BMenuBar_StartMenuBar_Hack StartMenuBar__8BMenuBarFlbbP5BRect
#elif __GNUC__ <= 2
	#define BMenuBar_StartMenuBar_Hack StartMenuBar__8BMenuBarlbT2P5BRect
#elif __GNUC__ > 2
	#define BMenuBar_StartMenuBar_Hack _ZN8BMenuBar12StartMenuBarElbbP5BRect
#else
#	error "You may want to port this ugly hack to your compiler ABI"
#endif
extern "C" void
	BMenuBar_StartMenuBar_Hack(BMenuBar*, int32, bool, bool, BRect*);


TDeskbarMenu* TBarWindow::sDeskbarMenu = NULL;


TBarWindow::TBarWindow()
	:
	BWindow(BRect(-1000.0f, -1000.0f, -1000.0f, -1000.0f),
		B_TRANSLATE_SYSTEM_NAME("Deskbar"), B_BORDERED_WINDOW,
		B_WILL_ACCEPT_FIRST_CLICK | B_NOT_ZOOMABLE | B_NOT_CLOSABLE
		| B_NOT_MINIMIZABLE | B_NOT_MOVABLE | B_NOT_RESIZABLE
		| B_AVOID_FRONT | B_ASYNCHRONOUS_CONTROLS,
		B_ALL_WORKSPACES)
{
	desk_settings* settings = ((TBarApp*)be_app)->Settings();
	if (settings->alwaysOnTop)
		SetFeel(B_FLOATING_ALL_WINDOW_FEEL);
	fBarView = new TBarView(Bounds(), settings->vertical, settings->left,
		settings->top, settings->ampmMode, settings->state, settings->width,
		settings->showTime);
	AddChild(fBarView);

	RemoveShortcut('H', B_COMMAND_KEY | B_CONTROL_KEY);
	AddShortcut('F', B_COMMAND_KEY, new BMessage(kFindButton));
}


void
TBarWindow::MenusBeginning()
{
	BPath path;
	entry_ref ref;

	find_directory (B_USER_DESKBAR_DIRECTORY, &path);
	get_ref_for_path(path.Path(), &ref);

	BEntry entry(&ref, true);
	if (entry.InitCheck() == B_OK && entry.IsDirectory()) {
		//	need the entry_ref to the actual item
		entry.GetRef(&ref);
		//	set the nav directory to the deskbar folder
		sDeskbarMenu->SetNavDir(&ref);
	} else if (!entry.Exists()) {
		//	the deskbar folder does not exist
		//	create one now
		BDirectory dir;
		if (entry.GetParent(&dir) == B_OK) {
			BDirectory deskbarDir;
			dir.CreateDirectory("deskbar", &deskbarDir);
			if (deskbarDir.GetEntry(&entry) == B_OK
				&& entry.GetRef(&ref) == B_OK)
				sDeskbarMenu->SetNavDir(&ref);
		}
	} else {
		//	this really should never happen
		TRESPASS();
		return;
	}

	sDeskbarMenu->NeedsToRebuild();
	sDeskbarMenu->ResetTargets();

	fBarView->SetEventMask(0);
		// This works around a BeOS bug - the menu is quit with every
		// B_MOUSE_DOWN the window receives...

	BWindow::MenusBeginning();
}


void
TBarWindow::MenusEnded()
{
	BWindow::MenusEnded();

	if (sDeskbarMenu->LockLooper()) {
		// TODO: is this ok?
		sDeskbarMenu->RemoveItems(0, sDeskbarMenu->CountItems(), true);
		sDeskbarMenu->UnlockLooper();
	}

	fBarView->UpdateEventMask();
}


void
TBarWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kFindButton:
		{
			BMessenger tracker(kTrackerSignature);
			tracker.SendMessage(message);
			break;
		}

		case 'gloc':
			GetLocation(message);
			break;

		case 'sloc':
			SetLocation(message);
			break;

		case 'gexp':
			IsExpanded(message);
			break;

		case 'sexp':
			Expand(message);
			break;

		case 'info':
			ItemInfo(message);
			break;

		case 'exst':
			ItemExists(message);
			break;

		case 'cwnt':
			CountItems(message);
			break;

		case 'adon':
		case 'icon':
			AddItem(message);
			break;

		case 'remv':
			RemoveItem(message);
			break;

		case 'iloc':
			GetIconFrame(message);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
TBarWindow::Minimize(bool minimize)
{
	// Don't allow the Deskbar to be minimized
	if (!minimize)
		BWindow::Minimize(false);
}


void
TBarWindow::SaveSettings()
{
	fBarView->SaveSettings();
}


bool
TBarWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);

	return BWindow::QuitRequested();
}


void
TBarWindow::WorkspaceActivated(int32 workspace, bool active)
{
	BWindow::WorkspaceActivated(workspace, active);

	if (active && !(fBarView->Expando() && fBarView->Vertical()))
		fBarView->UpdatePlacement();
	else {
		BRect screenFrame = (BScreen(fBarView->Window())).Frame();
		fBarView->SizeWindow(screenFrame);
		fBarView->PositionWindow(screenFrame);
		fBarView->Invalidate();
	}
}


void
TBarWindow::ScreenChanged(BRect size, color_space depth)
{
	BWindow::ScreenChanged(size, depth);

	fBarView->UpdatePlacement();
}


void
TBarWindow::SetDeskbarMenu(TDeskbarMenu* menu)
{
	sDeskbarMenu = menu;
}


TDeskbarMenu*
TBarWindow::DeskbarMenu()
{
	return sDeskbarMenu;
}


void
TBarWindow::ShowDeskbarMenu()
{
	BMenuBar* menuBar = fBarView->BarMenuBar();
	if (menuBar == NULL)
		menuBar = KeyMenuBar();

	if (menuBar == NULL)
		return;

	BMenuBar_StartMenuBar_Hack(menuBar, 0, true, true, NULL);
}


void
TBarWindow::ShowTeamMenu()
{
	int32 index = 0;
	if (fBarView->BarMenuBar() == NULL)
		index = 2;

	if (KeyMenuBar() == NULL)
		return;

	BMenuBar_StartMenuBar_Hack(KeyMenuBar(), index, true, true, NULL);
}


// determines the actual location of the window

deskbar_location
TBarWindow::DeskbarLocation() const
{
	bool left = fBarView->Left();
	bool top = fBarView->Top();

	if (fBarView->AcrossTop())
		return B_DESKBAR_TOP;

	if (fBarView->AcrossBottom())
		return B_DESKBAR_BOTTOM;

	if (left && top)
		return B_DESKBAR_LEFT_TOP;

	if (!left && top)
		return B_DESKBAR_RIGHT_TOP;

	if (left && !top)
		return B_DESKBAR_LEFT_BOTTOM;

	return B_DESKBAR_RIGHT_BOTTOM;
}


void
TBarWindow::GetLocation(BMessage* message)
{
	BMessage reply('rply');
	reply.AddInt32("location", (int32)DeskbarLocation());
	reply.AddBool("expanded", fBarView->Expando());

	message->SendReply(&reply);
}


void
TBarWindow::SetDeskbarLocation(deskbar_location location, bool newExpandState)
{
	// left top and right top are the only two that
	// currently pay attention to expand, ignore for all others

	bool left = false, top = true, vertical, expand;

	switch (location) {
		case B_DESKBAR_TOP:
			left = true;
			top = true;
			vertical = false;
			expand = true;
			break;

		case B_DESKBAR_BOTTOM:
			left = true;
			top = false;
			vertical = false;
			expand = true;
			break;

		case B_DESKBAR_LEFT_TOP:
			left = true;
			top = true;
			vertical = true;
			expand = newExpandState;
			break;

		case B_DESKBAR_RIGHT_TOP:
			left = false;
			top = true;
			vertical = true;
			expand = newExpandState;
			break;

		case B_DESKBAR_LEFT_BOTTOM:
			left = true;
			top = false;
			vertical = true;
			expand = false;
			break;

		case B_DESKBAR_RIGHT_BOTTOM:
			left = false;
			top = false;
			vertical = true;
			expand = false;
			break;

		default:
			left = true;
			top = true;
			vertical = false;
			expand = true;
			break;
	}

	fBarView->ChangeState(expand, vertical, left, top);
}


void
TBarWindow::SetLocation(BMessage* message)
{
	deskbar_location location;
	bool expand;
	if (message->FindInt32("location", (int32*)&location) == B_OK
		&& message->FindBool("expand", &expand) == B_OK)
		SetDeskbarLocation(location, expand);
}


void
TBarWindow::IsExpanded(BMessage* message)
{
	BMessage reply('rply');
	reply.AddBool("expanded", fBarView->Expando());
	message->SendReply(&reply);
}


void
TBarWindow::Expand(BMessage* message)
{
	bool expand;
	if (message->FindBool("expand", &expand) == B_OK) {
		bool vertical = fBarView->Vertical();
		bool left = fBarView->Left();
		bool top = fBarView->Top();
		fBarView->ChangeState(expand, vertical, left, top);
	}
}


void
TBarWindow::ItemInfo(BMessage* message)
{
	BMessage replyMsg;
	const char* name;
	int32 id;
	DeskbarShelf shelf;
	if (message->FindInt32("id", &id) == B_OK) {
		if (fBarView->ItemInfo(id, &name, &shelf) == B_OK) {
			replyMsg.AddString("name", name);
#if SHELF_AWARE
			replyMsg.AddInt32("shelf", (int32)shelf);
#endif
		}
	} else if (message->FindString("name", &name) == B_OK) {
		if (fBarView->ItemInfo(name, &id, &shelf) == B_OK) {
			replyMsg.AddInt32("id", id);
#if SHELF_AWARE
			replyMsg.AddInt32("shelf", (int32)shelf);
#endif
		}
	}

	message->SendReply(&replyMsg);
}


void
TBarWindow::ItemExists(BMessage* message)
{
	BMessage replyMsg;
	const char* name;
	int32 id;
	DeskbarShelf shelf;

#if SHELF_AWARE
	if (message->FindInt32("shelf", (int32*)&shelf) != B_OK)
#endif
		shelf = B_DESKBAR_TRAY;

	bool exists = false;
	if (message->FindInt32("id", &id) == B_OK)
		exists = fBarView->ItemExists(id, shelf);
	else if (message->FindString("name", &name) == B_OK)
		exists = fBarView->ItemExists(name, shelf);

	replyMsg.AddBool("exists", exists);
	message->SendReply(&replyMsg);
}


void
TBarWindow::CountItems(BMessage* message)
{
	DeskbarShelf shelf;

#if SHELF_AWARE
	if (message->FindInt32("shelf", (int32*)&shelf) != B_OK)
#endif
		shelf = B_DESKBAR_TRAY;

	BMessage reply('rply');
	reply.AddInt32("count", fBarView->CountItems(shelf));
	message->SendReply(&reply);
}


void
TBarWindow::AddItem(BMessage* message)
{
	DeskbarShelf shelf = B_DESKBAR_TRAY; 
	entry_ref ref;
	int32 id = 999;
	BMessage reply;
	status_t err = B_ERROR;

	BMessage archivedView;
	if (message->FindMessage("view", &archivedView) == B_OK) {
#if SHELF_AWARE
		message->FindInt32("shelf", &shelf);
#endif
		BMessage* archive = new BMessage(archivedView);
		err = fBarView->AddItem(archive, shelf, &id);
		if (err < B_OK)
			delete archive;
	} else if (message->FindRef("addon", &ref) == B_OK) {
		BEntry entry(&ref);
		err = entry.InitCheck();
		if (err == B_OK)
			err = fBarView->AddItem(&entry, shelf, &id);
	}

	if (err == B_OK)
		reply.AddInt32("id", id);
	else
		reply.AddInt32("error", err);

	message->SendReply(&reply);
}


void
TBarWindow::RemoveItem(BMessage* message)
{
	int32 id;
	const char* name;

	// ids ought to be unique across all shelves, assuming, of course,
	// that sometime in the future there may be more than one
#if SHELF_AWARE
	if (message->FindInt32("shelf", (int32*)&shelf) == B_OK) {
		if (message->FindString("name", &name) == B_OK)
			fBarView->RemoveItem(name, shelf);
	} else {
#endif
		if (message->FindInt32("id", &id) == B_OK) {
			fBarView->RemoveItem(id);
		// remove the following two lines if and when the
		// shelf option returns
		} else if (message->FindString("name", &name) == B_OK)
			fBarView->RemoveItem(name, B_DESKBAR_TRAY);

#if SHELF_AWARE
	}
#endif
}


void
TBarWindow::GetIconFrame(BMessage* message)
{
	BRect frame(0, 0, 0, 0);

	const char* name;
	int32 id;
	if (message->FindInt32("id", &id) == B_OK)
		frame = fBarView->IconFrame(id);
	else if (message->FindString("name", &name) == B_OK)
		frame = fBarView->IconFrame(name);

	BMessage reply('rply');
	reply.AddRect("frame", frame);
	message->SendReply(&reply);
}


bool
TBarWindow::_IsFocusMessage(BMessage* message)
{
	BMessage::Private messagePrivate(message);
	if (!messagePrivate.UsePreferredTarget())
		return false;

	bool feedFocus;
	if (message->HasInt32("_token")
		&& (message->FindBool("_feed_focus", &feedFocus) != B_OK || !feedFocus))
		return false;

	return true;
}

