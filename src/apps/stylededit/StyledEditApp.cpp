
#include <Autolock.h>
#include <Path.h>
#include <MenuItem.h>
#include <CharacterSet.h>
#include <CharacterSetRoster.h>
#include "Constants.h"
#include "StyledEditApp.h"
#include "StyledEditWindow.h"

using namespace BPrivate;

BRect windowRect(7,26,507,426);

StyledEditApp * styled_edit_app;

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

	status_t status = B_OK;
	BCharacterSetRoster roster;
	BCharacterSet charset;
	while (roster.GetNextCharacterSet(&charset) == B_NO_ERROR) {
		BString name(charset.GetPrintName());
		const char * mime = charset.GetMIMEName();
		if (mime) {
			name.Append(" (");
			name.Append(mime);
			name.Append(")");
		}
		BMenuItem * item = new BMenuItem(name.String(),new BMessage(OPEN_AS_ENCODING));
		item->SetTarget(this);
		fOpenPanelEncodingMenu->AddItem(item);
		if (charset.GetFontID() == fOpenAsEncoding) {
			item->SetMarked(true);
		}
	}
	
	fWindowCount= 0;
	fNext_Untitled_Window= 1;
	styled_edit_app = this;
} /***StyledEditApp::StyledEditApp()***/

void StyledEditApp::DispatchMessage(BMessage *msg, BHandler *handler)
{
	if ( msg->what == B_ARGV_RECEIVED ) {
		int32 argc;
		if (msg->FindInt32("argc",&argc) != B_OK) {
			argc=0;
		}
		const char ** argv = new (const char*)[argc];
		for (int arg = 0; (arg < argc) ; arg++) {
			if (msg->FindString("argv",arg,&argv[arg]) != B_OK) {
				argv[arg] = "";
			}
		}
		const char * cwd;
		if (msg->FindString("cwd",&cwd) != B_OK) {
			cwd = "";
		}
		ArgvReceived(argc, argv, cwd);
	} else {
		BApplication::DispatchMessage(msg,handler);
	}
}


void
StyledEditApp::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case MENU_NEW:
			OpenDocument();
		break;
		case MENU_OPEN:
			fOpenPanel->Show(); //
		break;
		case B_SILENT_RELAUNCH:
			OpenDocument();
		break;
		case OPEN_AS_ENCODING:
			void * ptr;
			if (message->FindPointer("source",&ptr) == B_OK) {
				if (fOpenPanelEncodingMenu != 0) {
					fOpenAsEncoding = (uint32)fOpenPanelEncodingMenu->IndexOf((BMenuItem*)ptr);
				}
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
	new StyledEditWindow(windowRect,fNext_Untitled_Window++,fOpenAsEncoding);
	windowRect.OffsetBy(15,15); // TODO: wrap around screen
	fWindowCount++;
}

void
StyledEditApp::OpenDocument(entry_ref * ref)
{
	new StyledEditWindow(windowRect,ref,fOpenAsEncoding);
	windowRect.OffsetBy(15,15); // TODO: wrap around screen
	fWindowCount++;
}

void
StyledEditApp::CloseDocument()
{
	fWindowCount--;
	if (fWindowCount == 0) {
		BAutolock lock(this);
		Quit();
	}
}

void
StyledEditApp::RefsReceived(BMessage *message)
{
	int32		refNum;
	entry_ref	ref;
	status_t	err;
	
	refNum = 0;
	do {
		err = message->FindRef("refs", refNum, &ref);
		if (err != B_OK)
			return;
		OpenDocument(&ref);
		refNum++;
	} while (true);
} /***StyledEditApp::RefsReceived();***/

void
StyledEditApp::ArgvReceived(int32 argc, const char *argv[], const char * cwd)
{
	for (int i = 1 ; (i < argc) ; i++) {
		BPath path;
		if (argv[i][0] == '/') {
			path.SetTo(argv[i]);
		} else {
			path.SetTo(cwd,argv[i]);
		}
		if (path.InitCheck() != B_OK) {
			continue; // TODO: alert the user?
		}
		
		entry_ref ref;
		if (get_ref_for_path(path.Path(), &ref) != B_OK) {
			continue; // TODO: alert the user?
		}
		
		OpenDocument(&ref);
	}
}

void 
StyledEditApp::ReadyToRun() 
{
	if (fWindowCount == 0) {
		OpenDocument();
	}
}

int32
StyledEditApp::NumberOfWindows()
{
 	
 	return fWindowCount;

}/***StyledEditApp::NumberOfWindows()***/

int
main()
{
	StyledEditApp	styledEdit;
	styledEdit.Run();
	return 0;
}

