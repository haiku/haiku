
#include <Autolock.h>
#include "Constants.h"
#include "StyledEditApp.h"
#include "StyledEditWindow.h"

BRect windowRect(7,26,507,426);

StyledEditApp * styled_edit_app;

StyledEditApp::StyledEditApp()
	: BApplication(APP_SIGNATURE)
{
	fOpenPanel= new BFilePanel();
	BMenuBar * menuBar =
	   dynamic_cast<BMenuBar*>(fOpenPanel->Window()->FindView("MenuBar"));
	   
	fOpenPanelEncodingMenu= new BMenu("Encoding");
	menuBar->AddItem(fOpenPanelEncodingMenu);

	// TODO: add encodings
	
	fWindowCount= 0;
	fNext_Untitled_Window= 1;
	styled_edit_app = this;
} /***StyledEditApp::StyledEditApp()***/


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
		default:
			BApplication::MessageReceived(message);
		break;
	} 
}

void
StyledEditApp::OpenDocument()
{
	new StyledEditWindow(windowRect,fNext_Untitled_Window++);
	windowRect.OffsetBy(15,15); // TODO: wrap around screen
	fWindowCount++;
}

void
StyledEditApp::OpenDocument(entry_ref * ref)
{
	new StyledEditWindow(windowRect,ref);
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
	
	refNum=0;
	do {
		if((err= message->FindRef("refs", refNum, &ref)) != B_OK)
			return;
		OpenDocument(&ref);
		refNum++;
	} while(1);		
} /***StyledEditApp::RefsReceived();***/

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

