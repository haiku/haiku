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


#define DUMPED_BLOCK_SIZE 16

void
dumpBlock(const uint8 *buffer, int size, const char *prefix)
{
	int i;
	
	for (i = 0; i < size;) {
		int start = i;

		printf(prefix);
		for (; i < start+DUMPED_BLOCK_SIZE; i++) {
			if (!(i % 4))
				printf(" ");

			if (i >= size)
				printf("  ");
			else
				printf("%02x", *(unsigned char *)(buffer + i));
		}
		printf("  ");

		for (i = start; i < start + DUMPED_BLOCK_SIZE; i++) {
			if (i < size) {
				char c = buffer[i];

				if (c < 30)
					printf(".");
				else
					printf("%c", c);
			} else
				break;
		}
		printf("\n");
	}
}


status_t
printBlock(DataEditor &editor, const char *text)
{
	puts(text);

	const uint8 *buffer;
	status_t status = editor.GetViewBuffer(&buffer);
	if (status < B_OK) {
		fprintf(stderr, "Could not get block: %s\n", strerror(status));
		return status;
	}

	dumpBlock(buffer, editor.FileSize() > editor.ViewOffset() + editor.ViewSize() ?
			editor.ViewSize() : editor.FileSize() - editor.ViewOffset(),
		"\t");

	return B_OK;
}


//	#pragma mark -


DiskProbe::DiskProbe()
	: BApplication(kSignature),
	fOpenWindow(NULL),
	fWindowCount(0),
	fWindowPosition(30, 30, 500, 500)
{
	fFilePanel = new BFilePanel();

	// ToDo: maintain the window position on disk

	/*
	DataEditor editor;
	status_t status = editor.SetTo("/boot/beos/apps/DiskProbe");
	if (status < B_OK) {
		fprintf(stderr, "Could not set editor: %s\n", strerror(status));
		return;
	}

	editor.SetBlockSize(512);
	editor.SetViewOffset(0);
	editor.SetViewSize(32);

	BAutolock locker(editor.Locker());

	if (printBlock(editor, "before (location 0):") < B_OK)
		return;

	editor.Replace(5, (const uint8 *)"*REPLACED*", 10);
	if (printBlock(editor, "after (location 0):") < B_OK)
		return;

	editor.SetViewOffset(700);
	if (printBlock(editor, "before (location 700):") < B_OK)
		return;

	editor.Replace(713, (const uint8 *)"**REPLACED**", 12);
	if (printBlock(editor, "after (location 700):") < B_OK)
		return;

	status = editor.Undo();
	if (status < B_OK)
		fprintf(stderr, "Could not undo: %s\n", strerror(status));
	if (printBlock(editor, "undo (location 700):") < B_OK)
		return;

	editor.SetViewOffset(0);
	if (printBlock(editor, "after (location 0):") < B_OK)
		return;

	status = editor.Undo();
	if (status < B_OK)
		fprintf(stderr, "Could not undo: %s\n", strerror(status));
	if (printBlock(editor, "undo (location 0):") < B_OK)
		return;

	status = editor.Redo();
	if (status < B_OK)
		fprintf(stderr, "Could not redo: %s\n", strerror(status));
	if (printBlock(editor, "redo (location 0):") < B_OK)
		return;

	status = editor.Redo();
	editor.SetViewOffset(700);
	if (status < B_OK)
		fprintf(stderr, "Could not redo: %s\n", strerror(status));
	if (printBlock(editor, "redo (location 700):") < B_OK)
		return;

	status = editor.Redo();
	if (status == B_OK)
		fprintf(stderr, "Could apply redo (non-expected behaviour)!\n");

	status = editor.Undo();
	if (status < B_OK)
		fprintf(stderr, "Could not undo: %s\n", strerror(status));
	if (printBlock(editor, "undo (location 700):") < B_OK)
		return;
	*/
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
	// Do we already have that window open?
	for (int32 i = CountWindows(); i-- > 0; ) {
		ProbeWindow *window = dynamic_cast<ProbeWindow *>(WindowAt(i));
		if (window == NULL)
			continue;
		
		if (window->EntryRef() == ref) {
			window->Activate(true);
			return B_OK;
		}
	}

	// Does the file really exists?
	BFile file;
	status_t status = file.SetTo(&ref, B_READ_ONLY);
	if (status < B_OK)
		return status;

	// cascade window
	BRect rect = fWindowPosition;
	rect.OffsetBy(fWindowCount * 15, fWindowCount * 15);

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
