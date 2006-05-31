/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ApplicationTypesWindow.h"
#include "ApplicationTypeWindow.h"
#include "FileTypes.h"
#include "FileTypesWindow.h"
#include "FileTypeWindow.h"

#include <AppFileInfo.h>
#include <Application.h>
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
		void _WindowClosed();

		BFilePanel	*fFilePanel;
		BMessenger	fFilePanelTarget;
		BWindow		*fTypesWindow;
		BWindow		*fApplicationTypesWindow;
		uint32		fWindowCount;
		uint32		fTypeWindowCount;
		BRect		fTypesWindowFrame;
		BRect		fApplicationTypesWindowFrame;
};


FileTypes::FileTypes()
	: BApplication(kSignature),
	fTypesWindow(NULL),
	fApplicationTypesWindow(NULL),
	fWindowCount(0),
	fTypeWindowCount(0)
{
	fFilePanel = new BFilePanel(B_OPEN_PANEL, NULL, NULL,
		B_FILE_NODE | B_DIRECTORY_NODE, false);

	fTypesWindowFrame = BRect(80.0f, 80.0f, 600.0f, 480.0f);
	fApplicationTypesWindowFrame = BRect(100.0f, 100.0f, 540.0f, 480.0f);
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

	// filter out applications and entries we can't open
	int32 index = 0;
	entry_ref ref;
	while (message->FindRef("refs", index++, &ref) == B_OK) {
		BEntry entry;
		BFile file;

		status_t status = entry.SetTo(&ref, traverseLinks);
		if (status == B_OK)
			status = file.SetTo(&entry, B_READ_ONLY);

		if (status != B_OK) {
			// file cannot be opened

			char buffer[1024];
			snprintf(buffer, sizeof(buffer),
				"Could not open \"%s\":\n"
				"%s",
				ref.name, strerror(status));

			(new BAlert("FileTypes request",
				buffer, "Ok", NULL, NULL,
				B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();

			message->RemoveData("refs", --index);
			continue;
		}

		if (!is_application(file)) {
			if (entry.GetRef(&ref) == B_OK)
				message->ReplaceRef("refs", index - 1, &ref);
			continue;
		}

		// remove application from list
		message->RemoveData("refs", --index);

		// There are some refs left that want to be handled by the type window
		BPoint point(100.0f + 20.0f * fTypeWindowCount, 110.0f + 20.0f * fTypeWindowCount);

		BWindow* window = new ApplicationTypeWindow(point, entry);
		window->Show();

		fTypeWindowCount++;
		fWindowCount++;
	}

	if (message->FindRef("refs", &ref) != B_OK)
		return;

	// There are some refs left that want to be handled by the type window
	BPoint point(100.0f + 20.0f * fTypeWindowCount, 110.0f + 20.0f * fTypeWindowCount);

	BWindow* window = new FileTypeWindow(point, *message);
	window->Show();

	fTypeWindowCount++;
	fWindowCount++;
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
FileTypes::_WindowClosed()
{
	if (--fWindowCount == 0 && !fFilePanel->IsShowing())
		PostMessage(B_QUIT_REQUESTED);
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
			_WindowClosed();
			break;

		case kMsgOpenApplicationTypesWindow:
			if (fApplicationTypesWindow == NULL) {
				fApplicationTypesWindow = new ApplicationTypesWindow(
					fApplicationTypesWindowFrame);
				fApplicationTypesWindow->Show();
				fWindowCount++;
			} else
				fApplicationTypesWindow->Activate(true);
			break;
		case kMsgApplicationTypesWindowClosed:
			fApplicationTypesWindow = NULL;
			_WindowClosed();
			break;

		case kMsgTypeWindowClosed:
			fTypeWindowCount--;
			// supposed to fall through

		case kMsgWindowClosed:
			_WindowClosed();
			break;


		case kMsgOpenFilePanel:
		{
			// the open file panel sends us a message when it's done
			const char* subTitle;
			if (message->FindString("title", &subTitle) != B_OK)
				subTitle = "Open File";

			int32 what;
			if (message->FindInt32("message", &what) != B_OK)
				what = B_REFS_RECEIVED;

			BMessenger target;
			if (message->FindMessenger("target", &target) != B_OK)
				target = be_app_messenger;

			BString title = "FileTypes";
			if (subTitle != NULL && subTitle[0]) {
				title.Append(": ");
				title.Append(subTitle);
			}

			fFilePanel->SetMessage(new BMessage(what));
			fFilePanel->Window()->SetTitle(title.String());
			fFilePanel->SetTarget(target);

			if (!fFilePanel->IsShowing())
				fFilePanel->Show();
			break;
		}

		case B_CANCEL:
			if (fWindowCount == 0)
				PostMessage(B_QUIT_REQUESTED);
			break;

		case B_SIMPLE_DATA:
			RefsReceived(message);
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


bool
is_application(BFile& file)
{
	BAppFileInfo appInfo(&file);
	if (appInfo.InitCheck() != B_OK)
		return false;

	char type[B_MIME_TYPE_LENGTH];
	if (appInfo.GetType(type) != B_OK
		|| strcasecmp(type, B_APP_MIME_TYPE))
		return false;

	return true;
}


void
error_alert(const char* message, status_t status, alert_type type)
{
	char warning[512];
	if (status != B_OK)
		snprintf(warning, sizeof(warning), "%s:\n\t%s\n", message, strerror(status));

	(new BAlert("FileTypes Request",
		status == B_OK ? message : warning,
		"Ok", NULL, NULL, B_WIDTH_AS_USUAL, type))->Go();
}


int
main(int argc, char **argv)
{
	FileTypes probe;

	probe.Run();
	return 0;
}
