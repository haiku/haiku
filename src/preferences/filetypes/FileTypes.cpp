/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "FileTypes.h"
#include "FileTypesWindow.h"

#include <Application.h>
//#include <Screen.h>
//#include <Autolock.h>
#include <Alert.h>
#include <TextView.h>
#include <FilePanel.h>
#include <FindDirectory.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>

#include <stdio.h>
#include <string.h>


const char *kSignature = "application/x-vnd.Haiku-FileTypes";

class FileTypes : public BApplication {
	public:
		FileTypes();
		virtual ~FileTypes();

		virtual void ReadyToRun();

		virtual void RefsReceived(BMessage* message);
		virtual void ArgvReceived(int32 argc, char** argv);
		virtual void MessageReceived(BMessage* message);

		virtual void AboutRequested();
		virtual bool QuitRequested();

	private:
		BFilePanel	*fFilePanel;
		BWindow		*fTypesWindow;
		uint32		fWindowCount;
		BRect		fTypesWindowFrame;
};


FileTypes::FileTypes()
	: BApplication(kSignature),
	fTypesWindow(NULL),
	fWindowCount(0)
{
	fFilePanel = new BFilePanel();
	fTypesWindowFrame = BRect(80.0f, 80.0f, 540.0f, 440.0f);
		// TODO: read from settings
}


FileTypes::~FileTypes()
{
	delete fFilePanel;
}


void
FileTypes::ReadyToRun()
{
	// are there already windows open?
	if (CountWindows() != 1)
		return;

	// if not, open the FileTypes window
	PostMessage(kMsgOpenTypesWindow);
}


void
FileTypes::RefsReceived(BMessage *message)
{
	bool traverseLinks = (modifiers() & B_SHIFT_KEY) == 0;

	int32 index = 0;
	entry_ref ref;
	while (message->FindRef("refs", index++, &ref) == B_OK) {
		BEntry entry;
		status_t status = entry.SetTo(&ref, traverseLinks);
		if (status == B_OK) {
			// TODO: open FileType window
		}

		if (status != B_OK) {
			char buffer[1024];
			snprintf(buffer, sizeof(buffer),
				"Could not open \"%s\":\n"
				"%s",
				ref.name, strerror(status));

			(new BAlert("FileTypes request",
				buffer, "Ok", NULL, NULL,
				B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
		}
	}
}


void
FileTypes::ArgvReceived(int32 argc, char **argv)
{
	BMessage *message = CurrentMessage();

	BDirectory currentDirectory;
	if (message)
		currentDirectory.SetTo(message->FindString("cwd"));

	BMessage refs;

	for (int i = 1 ; i < argc ; i++) {
		BPath path;
		if (argv[i][0] == '/')
			path.SetTo(argv[i]);
		else
			path.SetTo(&currentDirectory, argv[i]);

		status_t status;
		entry_ref ref;		
		BEntry entry;

		if ((status = entry.SetTo(path.Path(), false)) != B_OK
			|| (status = entry.GetRef(&ref)) != B_OK) {
			fprintf(stderr, "Could not open file \"%s\": %s\n",
				path.Path(), strerror(status));
			continue;
		}

		refs.AddRef("refs", &ref);
	}

	RefsReceived(&refs);
}


void
FileTypes::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgOpenTypesWindow:
			if (fTypesWindow == NULL) {
				fTypesWindow = new FileTypesWindow(fTypesWindowFrame);
				fTypesWindow->Show();
				fWindowCount++;
			} else
				fTypesWindow->Activate(true);
			break;

		case kMsgTypesWindowClosed:
			fTypesWindow = NULL;
			// supposed to fall through
		case kMsgWindowClosed:
			if (--fWindowCount == 0 && !fFilePanel->IsShowing())
				PostMessage(B_QUIT_REQUESTED);
			break;

		case kMsgOpenFilePanel:
			fFilePanel->Show();
			break;
		case B_CANCEL:
			if (fWindowCount == 0)
				PostMessage(B_QUIT_REQUESTED);
			break;

		default:
			BApplication::MessageReceived(message);
			break;
	}
}


void
FileTypes::AboutRequested()
{
	BAlert *alert = new BAlert("about", "FileTypes\n"
		"\twritten by Axel Dörfler\n"
		"\tCopyright 2006, Haiku.\n", "Ok");
	BTextView *view = alert->TextView();
	BFont font;

	view->SetStylable(true);

	view->GetFont(&font);
	font.SetSize(18);
	font.SetFace(B_BOLD_FACE); 			
	view->SetFontAndColor(0, 9, &font);

	alert->Go();
}


bool
FileTypes::QuitRequested()
{
	return true;
}


//	#pragma mark -


int
main(int argc, char **argv)
{
	FileTypes probe;

	probe.Run();
	return 0;
}
