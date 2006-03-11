/*
 * Copyright 2002-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mattias Sundblad
 *		Andrew Bachmann
 */


#include <Alert.h>
#include <Autolock.h>
#include <Path.h>
#include <MenuItem.h>
#include <CharacterSet.h>
#include <CharacterSetRoster.h>
#include <Screen.h>
#include <stdio.h>

#include "Constants.h"
#include "StyledEditApp.h"
#include "StyledEditWindow.h"

using namespace BPrivate;

StyledEditApp * styled_edit_app;
BRect gWindowRect(7-15, 26-15, 507, 426);


void
cascade()
{
	BScreen screen(NULL);
	BRect screenBorder = screen.Frame();
	float left = gWindowRect.left + 15;
	if (left + gWindowRect.Width() > screenBorder.right)
		left = 7;

	float top = gWindowRect.top + 15;
	if (top + gWindowRect.Height() > screenBorder.bottom)
		top = 26;

	gWindowRect.OffsetTo(BPoint(left,top));	
}


void
uncascade()
{
	BScreen screen(NULL);
	BRect screenBorder = screen.Frame();

	float left = gWindowRect.left - 15;
	if (left < 7) {
		left = screenBorder.right - gWindowRect.Width() - 7;
		left = left - ((int)left % 15) + 7;
	}

	float top = gWindowRect.top - 15;
	if (top < 26) {
		top = screenBorder.bottom - gWindowRect.Height() - 26;
		top = top - ((int)left % 15) + 26;
	}

	gWindowRect.OffsetTo(BPoint(left,top));	
}


StyledEditApp::StyledEditApp()
	: BApplication(APP_SIGNATURE)
{
	fOpenPanel= new BFilePanel();
	BMenuBar * menuBar =
		dynamic_cast<BMenuBar*>(fOpenPanel->Window()->FindView("MenuBar"));

	fOpenAsEncoding = 0;
	fOpenPanelEncodingMenu= new BMenu("Encoding");
	menuBar->AddItem(fOpenPanelEncodingMenu);
	fOpenPanelEncodingMenu->SetRadioMode(true);

	BCharacterSetRoster roster;
	BCharacterSet charset;
	while (roster.GetNextCharacterSet(&charset) == B_NO_ERROR) {
		BString name(charset.GetPrintName());
		const char* mime = charset.GetMIMEName();
		if (mime) {
			name.Append(" (");
			name.Append(mime);
			name.Append(")");
		}
		BMenuItem* item = new BMenuItem(name.String(), new BMessage(OPEN_AS_ENCODING));
		item->SetTarget(this);
		fOpenPanelEncodingMenu->AddItem(item);
		if (charset.GetFontID() == fOpenAsEncoding)
			item->SetMarked(true);
	}

	fWindowCount = 0;
	fNextUntitledWindow = 1;

	styled_edit_app = this;
}


void
StyledEditApp::DispatchMessage(BMessage *msg, BHandler *handler)
{
	if (msg->what == B_ARGV_RECEIVED) {
		int32 argc;
		if (msg->FindInt32("argc", &argc) != B_OK)
			argc = 0;

		const char** argv = new const char*[argc];
		for (int arg = 0; arg < argc; arg++) {
			if (msg->FindString("argv", arg, &argv[arg]) != B_OK) {
				argv[arg] = "";
			}
		}
		const char* cwd;
		if (msg->FindString("cwd", &cwd) != B_OK)
			cwd = "";

		ArgvReceived(argc, argv, cwd);
	} else
		BApplication::DispatchMessage(msg, handler);
}


void
StyledEditApp::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case MENU_NEW:
			OpenDocument();
			break;
		case MENU_OPEN:
			fOpenPanel->Show();
			break;
		case B_SILENT_RELAUNCH:
			OpenDocument();
			break;
		case OPEN_AS_ENCODING:
			void* ptr;
			if (message->FindPointer("source", &ptr) == B_OK
				&& fOpenPanelEncodingMenu != 0) {
				fOpenAsEncoding = (uint32)fOpenPanelEncodingMenu->IndexOf((BMenuItem*)ptr);
			}
			break;

		default:
			BApplication::MessageReceived(message);
			break;
	} 
}


void
StyledEditApp::OpenDocument()
{
	cascade();
	new StyledEditWindow(gWindowRect, fNextUntitledWindow++, fOpenAsEncoding);
	fWindowCount++;
}


void
StyledEditApp::OpenDocument(entry_ref* ref)
{
	cascade();
	new StyledEditWindow(gWindowRect, ref, fOpenAsEncoding);
	fWindowCount++;
}


void
StyledEditApp::CloseDocument()
{
	uncascade();
	fWindowCount--;
	if (fWindowCount == 0) {
		BAutolock lock(this);
		Quit();
	}
}


void
StyledEditApp::RefsReceived(BMessage *message)
{
	int32 index = 0;
	entry_ref ref;

	while (message->FindRef("refs", index++, &ref) == B_OK) {
		OpenDocument(&ref);
	}
}


void
StyledEditApp::ArgvReceived(int32 argc, const char* argv[], const char* cwd)
{
	for (int i = 1 ; (i < argc) ; i++) {
		BPath path;
		if (argv[i][0] == '/')
			path.SetTo(argv[i]);
		else
			path.SetTo(cwd, argv[i]);

		if (path.InitCheck() != B_OK) {
			fprintf(stderr, "Setting path failed: \"");
			if (argv[i][0] == '/')
				fprintf(stderr, "%s\".\n", argv[i]);
			else
				fprintf(stderr, "%s/%s\".\n", cwd, argv[i]);
			continue;
		}

		entry_ref ref;
		if (get_ref_for_path(path.Path(), &ref) != B_OK) {
			fprintf(stderr, "Entry not found: \"%s\".\n", path.Path());
			continue;
		}
		OpenDocument(&ref);
	}
}


void 
StyledEditApp::ReadyToRun() 
{
	if (fWindowCount == 0)
		OpenDocument();
}


int32
StyledEditApp::NumberOfWindows()
{
 	return fWindowCount;

}


//	#pragma mark -


int
main(int argc, char** argv)
{
	StyledEditApp styledEdit;
	styledEdit.Run();
	return 0;
}

