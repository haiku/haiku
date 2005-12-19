#include <Alert.h>
#include <Autolock.h>
#include <Debug.h>
#include <Entry.h>
#include <Node.h>
#include <NodeInfo.h>

#include "FileTypeApp.h"
#include "FileTypeView.h"
#include "FileTypeWindow.h"

FileTypeWindow::FileTypeWindow(const BList * entryList)
	: BWindow(BRect(100,100,380,300),"File Type",B_TITLED_WINDOW,
	          B_NOT_ZOOMABLE|B_NOT_RESIZABLE|B_ASYNCHRONOUS_CONTROLS)
{
	initStatus = B_ERROR;
	fEntryList = 0;
	if (entryList == 0) {
		initStatus = B_BAD_VALUE;
		return;
	} 
	fMenuBar = new BMenuBar(BRect(0,0,0,0),"menubar");
	AddChild(fMenuBar);
	
	BRect viewFrame = Bounds();
	viewFrame.top = fMenuBar->Bounds().Height()+1;
	fView = new FileTypeView(viewFrame);
	AddChild(fView);
	fView->MakeFocus(true);	
	
	fFileMenu = new BMenu("File");
	fMenuBar->AddItem(fFileMenu);
	fSaveItem = new BMenuItem("Save",new BMessage(B_SAVE_REQUESTED), 'S');
	fFileMenu->AddItem(fSaveItem);
	fFileMenu->AddSeparatorItem();
	fCloseItem = new BMenuItem("Close",new BMessage(B_QUIT_REQUESTED), 'W');
	fFileMenu->AddItem(fCloseItem);
	
	SetEntries(entryList);
	initStatus = B_OK;
	Show();
}
	
FileTypeWindow::~FileTypeWindow()
{
	if (fEntryList != 0) {
		for (int32 i = 0 ; (i < fEntryList->CountItems()) ; i++) {
			delete static_cast<BEntry*>(fEntryList->ItemAt(i));
		}
		fEntryList->MakeEmpty();
		delete fEntryList;
	}
}

status_t
FileTypeWindow::InitCheck() const
{
	return initStatus;
}

void
FileTypeWindow::MessageReceived(BMessage * message)
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
FileTypeWindow::Quit()
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
FileTypeWindow::QuitRequested()
{
	if (fView->IsClean()) {
		return true;
	}

	BAlert * saveAlert;
	BString alertText("Would you like to save changes to file type attributes of ");
	alertText << SummarizeEntries();
	alertText << "? ";
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

status_t
FileTypeWindow::SaveRequested()
{
	status_t result = B_OK;
	
	BString fileType(fView->GetFileType());
	BString preferredApplication(fView->GetPreferredApplication());
	
	if (fEntryList != 0) {
		for (int32 i = 0 ; (i < fEntryList->CountItems()) ; i++) {
			BNode node(static_cast<BEntry*>(fEntryList->ItemAt(i)));
			if ((result = node.InitCheck()) != B_OK) {
				// save errors are ignored: there's usually no good way for the user to recover
				continue; // can't proceed with an invalid node
			}		
			BNodeInfo nodeInfo(&node);
			if ((result = nodeInfo.InitCheck()) != B_OK) {
				// save errors are ignored: there's usually no good way for the user to recover
				continue; // can't proceed with an invalid nodeinfo
			}
			if ((result = nodeInfo.SetType(fileType.String())) != B_OK) {
				// save errors are ignored: there's usually no good way for the user to recover
			}
			if ((result = nodeInfo.SetPreferredApp(preferredApplication.String())) != B_OK) {
				// save errors are ignored: there's usually no good way for the user to recover
			}
		}
	}
	
	fView->SetFileType(fileType.String());
	fView->SetPreferredApplication(preferredApplication.String());	
	return result;
}

void
FileTypeWindow::SetEntries(const BList * entryList)
{
	fEntryList = new BList(*entryList);
	
	BString title = SummarizeEntries();
	title.Append(" File Type");
	SetTitle(strdup(title.String()));
	
	BString * fileType = 0;
	BString * preferredApplication = 0;
	for (int32 i = 0 ; (i < fEntryList->CountItems()) ; i++) {
		BNode node(static_cast<BEntry*>(fEntryList->ItemAt(i)));
		if (node.InitCheck() != B_OK) {
			// errors are ignored: there's usually no good way for the user to recover
			continue; // can't proceed with an invalid node
		}		
		BNodeInfo nodeInfo(&node);
		if (nodeInfo.InitCheck() != B_OK) {
			// errors are ignored: there's usually no good way for the user to recover
			continue; // can't proceed with an invalid nodeinfo
		}
		char string[MAXPATHLEN];
		switch (nodeInfo.GetType(string)) {
		case B_ENTRY_NOT_FOUND:
			strcpy(string,"");
		case B_OK:
			if (fileType == 0) {
				fileType = new BString(string);
			} else if (fileType->Compare(string) != 0) {
				fileType->SetTo("");
				if (preferredApplication && (preferredApplication->Length() == 0)) {
					break; // stop now, don't waste time checking the rest
				}
			}
		break;
		default:
			// errors are ignored: there's usually no good way for the user to recover
			break;
		}
		switch (nodeInfo.GetPreferredApp(string)) {
		case B_ENTRY_NOT_FOUND:
			strcpy(string,"");
		case B_OK:
			if (preferredApplication == 0) {
				preferredApplication = new BString(string);
			} else if (preferredApplication->Compare(string) != 0) {
				preferredApplication->SetTo("");
				if (fileType && (fileType->Length() == 0)) {
					break; // stop now, don't waste time checking the rest
				}
			}
		break;
		default:
			// errors are ignored: there's usually no good way for the user to recover
			break;
		}
	}
	if (fileType != 0) {
		fView->SetFileType(fileType->String());
		delete fileType;
	}
	if ((preferredApplication != 0) && (preferredApplication->Length() > 0)) {
		fView->SetPreferredApplication(preferredApplication->String());
		delete preferredApplication;
	}
}

const char *
FileTypeWindow::SummarizeEntries()
{
	switch (fEntryList->CountItems()) {
	case 0:
		return "<error>";
	break;
	case 1: {
		char string[MAXPATHLEN];
		static_cast<BEntry*>(fEntryList->FirstItem())->GetName(string);
		return strdup(string);
	}
	break;
	default:
		return "[Multiple Files]";
	}
}
