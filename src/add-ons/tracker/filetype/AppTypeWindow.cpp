#include <Alert.h>
#include <Autolock.h>
#include <Debug.h>
#include <Entry.h>
#include <Node.h>
#include <NodeInfo.h>
#include <String.h>
#include <unistd.h>

#include <FileTypeApp.h>
#include <AppTypeView.h>
#include <AppTypeWindow.h>

AppTypeWindow::AppTypeWindow(const BEntry * entry)
	: BWindow(BRect(100,100,400,520),"Application Type",B_TITLED_WINDOW,
	          B_NOT_ZOOMABLE|B_NOT_RESIZABLE|B_ASYNCHRONOUS_CONTROLS)
{
	initStatus = B_ERROR;
	fEntry = 0;
	if (entry == 0) {
		initStatus = B_BAD_VALUE;
		return;
	} 
	fMenuBar = new BMenuBar(BRect(0,0,0,0),"menubar");
	AddChild(fMenuBar);
	
	BRect viewFrame = Bounds();
	viewFrame.top = fMenuBar->Bounds().Height()+1;
	fView = new AppTypeView(viewFrame);
	AddChild(fView);
	fView->MakeFocus(true);	
	
	fFileMenu = new BMenu("File");
	fMenuBar->AddItem(fFileMenu);
	fSaveItem = new BMenuItem("Save",new BMessage(B_SAVE_REQUESTED), 'S');
	fFileMenu->AddItem(fSaveItem);
	fFileMenu->AddSeparatorItem();
	fCloseItem = new BMenuItem("Close",new BMessage(B_QUIT_REQUESTED), 'W');
	fFileMenu->AddItem(fCloseItem);
	
	SetEntry(entry);
	initStatus = B_OK;
	Show();
}
	
AppTypeWindow::~AppTypeWindow()
{
	delete fEntry;
}

status_t
AppTypeWindow::InitCheck() const
{
	return initStatus;
}

void
AppTypeWindow::MessageReceived(BMessage * message)
{
	switch (message->what) {
		case B_SAVE_REQUESTED:
			SaveRequested();
		break;
		default:
			BWindow::MessageReceived(message);
		break;
	}
}

void
AppTypeWindow::Quit()
{
	{
		// This is in its own scope because it must be released
		// before the call to BWindow::Quit()
		BAutolock lock(file_type_app);
		file_type_app->Quit();
	}
	BWindow::Quit();
}

bool
AppTypeWindow::QuitRequested()
{
	if (fView->IsClean()) {
		return true;
	}

	if (!fEntry) {
		// no entry to save to!
		return true;
	}
		
	BAlert * saveAlert;
	char name[MAXPATHLEN];
	fEntry->GetName(name);
	BString alertText("Would you like to save changes to file type attributes of ");
	alertText << name << "?";
	saveAlert = new BAlert("savealert",alertText.String(), "Cancel", "Don't Save","Save",
		B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
	saveAlert->SetShortcut(0, B_ESCAPE);
	saveAlert->SetShortcut(1,'d');
	saveAlert->SetShortcut(2,'s');
	int32 buttonIndex = saveAlert->Go();

	if (buttonIndex==0) { 		//"cancel": dont save, dont close the window
		return false;
	} else if (buttonIndex==1) { // "don't save": just close the window
		return true;
	} else if (SaveRequested() == B_OK) {
		return true;
	} else {
		// save errors are ignored: there's usually no good way for the user to recover
		return true;
	}
}

static const char * application_file_types = "BEOS:FILE_TYPES";
static const char * application_flags_name = "BEOS:APP_FLAGS";
static const char * application_large_standard_icon_name = "BEOS:L:STD_ICON";
static const char * application_median_standard_icon_name = "BEOS:M:STD_ICON";
static const char * application_signature_name = "BEOS:APP_SIG";
static const char * application_version_name = "BEOS:APP_VERSION";

status_t
AppTypeWindow::SaveRequested()
{
	status_t result = B_OK;
	
	// TODO : save new settings to all attributes and resources matching the name
	
	return result;
}

void
AppTypeWindow::SetEntry(const BEntry * entry)
{
	fEntry = new BEntry(*entry);

	char name[MAXPATHLEN];
	entry->GetName(name);
	BString title(name);
	title.Append(" Application Type");
	SetTitle(strdup(title.String()));
	
	// TODO : set old settings


}
