/*
 * Copyright 2002-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mattias Sundblad
 *		Andrew Bachmann
 *		Jonas Sundström
 */


#include "Constants.h"
#include "StyledEditApp.h"
#include "StyledEditWindow.h"

#include <Alert.h>
#include <Autolock.h>
#include <Catalog.h>
#include <Locale.h>
#include <MenuBar.h>
#include <CharacterSet.h>
#include <CharacterSetRoster.h>
#include <FilePanel.h>
#include <MenuItem.h>
#include <Message.h>
#include <Path.h>
#include <Screen.h>

#include <stdio.h>


using namespace BPrivate;


BRect gWindowRect(7 - 15, 26 - 15, 507, 426);


namespace
{
	void
	cascade()
	{
		BScreen screen;
		BRect screenBorder = screen.Frame();
		float left = gWindowRect.left + 15;
		if (left + gWindowRect.Width() > screenBorder.right)
			left = 7;

		float top = gWindowRect.top + 15;
		if (top + gWindowRect.Height() > screenBorder.bottom)
			top = 26;

		gWindowRect.OffsetTo(BPoint(left, top));
	}


	void
	uncascade()
	{
		BScreen screen;
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

		gWindowRect.OffsetTo(BPoint(left, top));
	}
}


//	#pragma mark -


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Open_and_SaveAsPanel"


StyledEditApp::StyledEditApp()
	:
	BApplication(APP_SIGNATURE),
	fOpenPanel(NULL)
{
	B_TRANSLATE_MARK_SYSTEM_NAME_VOID("StyledEdit");

	fOpenPanel = new BFilePanel();
	fOpenAsEncoding = 0;

	BMenuBar* menuBar
		= dynamic_cast<BMenuBar*>(fOpenPanel->Window()->FindView("MenuBar"));
	if (menuBar != NULL) {
		fOpenPanelEncodingMenu = new BMenu(B_TRANSLATE("Encoding"));
		fOpenPanelEncodingMenu->SetRadioMode(true);

		menuBar->AddItem(fOpenPanelEncodingMenu);

		BCharacterSetRoster roster;
		BCharacterSet charset;
		while (roster.GetNextCharacterSet(&charset) == B_NO_ERROR) {
			BString name;
			if (charset.GetFontID() == B_UNICODE_UTF8)
				name = B_TRANSLATE("Default");
			else
				name = charset.GetPrintName();

			const char* mime = charset.GetMIMEName();
			if (mime != NULL) {
				name.Append(" (");
				name.Append(mime);
				name.Append(")");
			}
			BMenuItem* item
				= new BMenuItem(name.String(), new BMessage(OPEN_AS_ENCODING));
			item->SetTarget(this);
			fOpenPanelEncodingMenu->AddItem(item);
			if (charset.GetFontID() == fOpenAsEncoding)
				item->SetMarked(true);
		}
	} else
		fOpenPanelEncodingMenu = NULL;

	fWindowCount = 0;
	fNextUntitledWindow = 1;
	fBadArguments = false;
}


StyledEditApp::~StyledEditApp()
{
	delete fOpenPanel;
}


void
StyledEditApp::MessageReceived(BMessage* message)
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
				&& fOpenPanelEncodingMenu != NULL) {
				fOpenAsEncoding = (uint32)fOpenPanelEncodingMenu->IndexOf(
					(BMenuItem*)ptr);
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


status_t
StyledEditApp::OpenDocument(entry_ref* ref, BMessage* message)
{
	// traverse eventual symlink
	BEntry entry(ref, true);
	entry.GetRef(ref);

	if (entry.IsDirectory()) {
		BPath path(&entry);
		fprintf(stderr,
			"Can't open directory \"%s\" for editing.\n",
			path.Path());
		return B_ERROR;
	}

	BEntry parent;
	entry.GetParent(&parent);

	if (!entry.Exists() && !parent.Exists()) {
		fprintf(stderr,
			"Can't create file. Missing parent directory.\n");
		return B_ERROR;
	}

	BWindow* window = NULL;
	StyledEditWindow* document = NULL;

	for (int32 index = 0; ; index++) {
		window = WindowAt(index);
		if (window == NULL)
			break;

		document = dynamic_cast<StyledEditWindow*>(window);
		if (document == NULL)
			continue;

		if (document->IsDocumentEntryRef(ref)) {
			if (document->Lock()) {
				document->Activate();
				document->Unlock();
				if (message != NULL)
					document->PostMessage(message);
				return B_OK;
			}
		}
	}

	cascade();
	document = new StyledEditWindow(gWindowRect, ref, fOpenAsEncoding);

	if (message != NULL)
		document->PostMessage(message);

	fWindowCount++;

	return B_OK;
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
StyledEditApp::RefsReceived(BMessage* message)
{
	int32 index = 0;
	entry_ref ref;

	while (message->FindRef("refs", index, &ref) == B_OK) {
		int32 line;
		if (message->FindInt32("be:line", index, &line) != B_OK)
			line = -1;
		int32 start, length;
		if (message->FindInt32("be:selection_length", index, &length) != B_OK
			|| message->FindInt32("be:selection_offset", index, &start) != B_OK)
		{
			start = -1;
			length = -1;
		}

		BMessage* selection = NULL;
		if (line >= 0 || (start >= 0 && length >= 0)) {
			selection = new BMessage(UPDATE_LINE_SELECTION);
			if (line >= 0)
				selection->AddInt32("be:line", line);
			if (start >= 0) {
				selection->AddInt32("be:selection_offset", start);
				selection->AddInt32("be:selection_length", max_c(0, length));
			}
		}

		OpenDocument(&ref, selection);
		index++;
	}
}


void
StyledEditApp::ArgvReceived(int32 argc, char* argv[])
{
	// If StyledEdit is already running and gets invoked again
	// we need to account for a possible mismatch in current
	// working directory. The paths of the new arguments are
	// relative to the cwd of the invocation, if they are not
	// absolute. This cwd we find as a string named "cwd" in
	// the BLooper's current message.

	const char* cwd = "";
	BMessage* message = CurrentMessage();

	if (message != NULL) {
		if (message->FindString("cwd", &cwd) != B_OK)
			cwd = "";
	}

	for (int i = 1 ; (i < argc) ; i++) {
		BPath path;
		if (argv[i][0] == '/') {
			path.SetTo(argv[i]);
		} else {
			path.SetTo(cwd, argv[i]);
				// patch relative paths only
		}

		entry_ref ref;
		get_ref_for_path(path.Path(), &ref);

		status_t status;
		status = OpenDocument(&ref);

		if (status != B_OK && IsLaunching())
			fBadArguments = true;
	}
}


void
StyledEditApp::ReadyToRun()
{
	if (fWindowCount > 0)
		return;

	if (fBadArguments)
		Quit();
	else
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

