/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "DiskProbe.h"
#include "DataEditor.h"
#include "ProbeWindow.h"
#include "OpenWindow.h"

#include <Application.h>
#include <Autolock.h>
#include <Alert.h>
#include <TextView.h>
#include <FilePanel.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>

#include <stdio.h>
#include <string.h>


static const char *kSignature = "application/x-vnd.OpenBeOS-DiskProbe";

static const uint32 kCascadeOffset = 20;


class DiskProbe : public BApplication {
	public:
		DiskProbe();
		virtual ~DiskProbe();

		virtual void ReadyToRun();

		virtual void RefsReceived(BMessage *message);
		virtual void ArgvReceived(int32 argc, char **argv);
		virtual void MessageReceived(BMessage *message);

		virtual void AboutRequested();
		virtual bool QuitRequested();

	private:
		status_t Probe(entry_ref &ref);

		BFilePanel	*fFilePanel;
		BWindow		*fOpenWindow;
		uint32		fWindowCount;
		BRect		fWindowPosition;
};


//-----------------


DiskProbe::DiskProbe()
	: BApplication(kSignature),
	fOpenWindow(NULL),
	fWindowCount(0),
	fWindowPosition(50, 50, 550, 500)
{
	fFilePanel = new BFilePanel();

	// ToDo: maintain the window position on disk
}


DiskProbe::~DiskProbe()
{
	delete fFilePanel;
}


void 
DiskProbe::ReadyToRun()
{
	// are there already windows open?
	if (CountWindows() != 1)
		return;

	// if not, ask the user to open a file
	PostMessage(kMsgOpenOpenWindow);
}


/** Opens a window containing the file pointed to by the entry_ref.
 *	This function will fail if that file doesn't exist or could not
 *	be opened.
 *	It will check if there already is a window that probes the
 *	file in question and will activate it in that case.
 *	This function must be called with the application looper locked.
 */

status_t 
DiskProbe::Probe(entry_ref &ref)
{
	int32 probeWindows = 0;

	// Do we already have that window open?
	for (int32 i = CountWindows(); i-- > 0; ) {
		ProbeWindow *window = dynamic_cast<ProbeWindow *>(WindowAt(i));
		if (window == NULL)
			continue;
		
		if (window->EntryRef() == ref) {
			window->Activate(true);
			return B_OK;
		}
		probeWindows++;
	}

	// Does the file really exists?
	BFile file;
	status_t status = file.SetTo(&ref, B_READ_ONLY);
	if (status < B_OK)
		return status;

	// cascade window
	BRect rect = fWindowPosition;
	rect.OffsetBy(probeWindows * kCascadeOffset, probeWindows * kCascadeOffset);

	BWindow *window = new ProbeWindow(rect, &ref);
	window->Show();
	fWindowCount++;

	return B_OK;
}


void 
DiskProbe::RefsReceived(BMessage *message)
{
	int32 index = 0;
	entry_ref ref;
	while (message->FindRef("refs", index++, &ref) == B_OK) {
		Probe(ref);
	}
}


void 
DiskProbe::ArgvReceived(int32 argc, char **argv)
{
	BMessage *message = Looper()->CurrentMessage();

	BDirectory currentDirectory;
	if (message)
		currentDirectory.SetTo(message->FindString("cwd"));

	for (int i = 1 ; i < argc ; i++) {
		BPath path;
		if (argv[i][0] == '/')
			path.SetTo(argv[i]);
		else
			path.SetTo(&currentDirectory, argv[i]);

		BEntry entry(path.Path());
		entry_ref ref;
		status_t status;
		if ((status = entry.InitCheck()) != B_OK
			|| (status = entry.GetRef(&ref)) != B_OK
			|| (status = Probe(ref)) != B_OK) {
			char buffer[512];
			snprintf(buffer, sizeof(buffer), "Could not open file \"%s\": %s",
				argv[i], strerror(status));
			(new BAlert("DiskProbe", buffer, "Ok", NULL, NULL,
				B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
			continue;
		}
	}
}


void 
DiskProbe::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgOpenOpenWindow:
			if (fOpenWindow == NULL) {
				fOpenWindow = new OpenWindow();
				fOpenWindow->Show();
				fWindowCount++;
			} else
				fOpenWindow->Activate(true);
			break;

		case kMsgOpenWindowClosed:
			fOpenWindow = NULL;
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
DiskProbe::AboutRequested()
{
	BAlert *alert = new BAlert("about", "DiskProbe\n"
		"\twritten by Axel Dörfler\n"
		"\tCopyright 2004, OpenBeOS.\n\n\n"
		"original Be version by Robert Polic\n\n", "Ok");
	BTextView *view = alert->TextView();
	BFont font;

	view->SetStylable(true);

	view->GetFont(&font);
	font.SetSize(24);
	font.SetFace(B_BOLD_FACE); 			
	view->SetFontAndColor(0, 9, &font);

	alert->Go();
}


bool 
DiskProbe::QuitRequested()
{
	return true;
}


//	#pragma mark -


int 
main(int argc, char **argv)
{
	DiskProbe probe;

	probe.Run();
	return 0;
}
