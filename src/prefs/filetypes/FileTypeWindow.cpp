#include <Debug.h>

#include <FileTypeView.h>
#include <FileTypeWindow.h>

FileTypeWindow::FileTypeWindow(BEntryList entries)
	: BWindow(BRect(250,150,350,400),"File Type",B_TITLED_WINDOW,
	          B_NOT_ZOOMABLE|B_NOT_RESIZABLE|B_ASYNCHRONOUS_CONTROLS)
{
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
	fFileMenu->AddSeperator();
	fCloseItem = new BMenuItem("Close",new BMessage(B_QUIT_REQUESTED), 'W');
	fFileMenu->AddItem(fCloseItem);
	
	SetEntries(entries);
	Show();
}
	
FileTypeWindow::~FileTypeWindow()
{
}

void
FileTypeWindow::MessageReceived(BMessage *message)
{
	switch(message->what){
		case B_SAVE_REQUESTED:
			SaveRequested();
		break;
		default:
		BWindow::MessageReceived(message);
		break;
	}
}

bool
FileTypeWindow::QuitRequested()
{
	if (fView->Clean()) {
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
	buttonIndex = saveAlert->Go();

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
FileTypeWindow::SaveRequested(BMessage * message)
{
	status_t result = B_OK;
	
	const char * fileType = fView->GetFileType();
	const char * preferredApplication = fView->GetPreferredApplication();
	
	if ((result = fEntries->Rewind()) != B_OK) {
		return result; // can't proceed with no entries
	}
	BEntry entry;
	while (fEntries->GetNextEntry(&entry,true) != B_OK) {
		BNode node(&entry);
		if ((result = node.InitCheck()) != B_OK) {
			// save errors are ignored: there's usually no good way for the user to recover
			continue; // can't proceed with an invalid node
		}		
		BNodeInfo nodeInfo(&node);
		if ((result = nodeInfo.InitCheck()) != B_OK) {
			// save errors are ignored: there's usually no good way for the user to recover
			continue; // can't proceed with an invalid nodeinfo
		}
		if ((result = nodeInfo.SetType(fileType)) != B_OK) {
			// save errors are ignored: there's usually no good way for the user to recover
		}
		if ((result = nodeInfo.SetPreferredApp(preferredApplication)) != B_OK) {
			// save errors are ignored: there's usually no good way for the user to recover
		}
	}
	return result;
}

void
FileTypeWindow::SetEntries(BEntryList entries)
{
	fEntries = entries;
	
	BString title = SummarizeEntries();
	title.Append(" File Type");
	SetTitle(title);
	
	if (fEntries->Rewind() != B_OK) {
		return;
	}
	BString * fileType = 0;
	BString * preferredApplication = 0;
	BEntry entry;
	while (fEntries->GetNextEntry(&entry,true) != B_OK) {
		BNode node(&entry);
		if ((result = node.InitCheck()) != B_OK) {
			// errors are ignored: there's usually no good way for the user to recover
			continue; // can't proceed with an invalid node
		}		
		BNodeInfo nodeInfo(&node);
		if ((result = nodeInfo.InitCheck()) != B_OK) {
			// errors are ignored: there's usually no good way for the user to recover
			continue; // can't proceed with an invalid nodeinfo
		}
		char string[MAXPATHLEN];
		if ((result = nodeInfo.GetType(string)) != B_OK) {
			// errors are ignored: there's usually no good way for the user to recover
		} else {
			if (fileType == 0) {
				fileType = new BString(string);
			} else if (fileType->Compare(string) != 0) {
				fileType.SetTo("");
				if (preferredApplication && (preferredApplication->Length() == 0)) {
					break; // stop now, don't waste time checking the rest
				}
			}
		}
		if ((result = nodeInfo.GetPreferredApp(string)) != B_OK) {
			// errors are ignored: there's usually no good way for the user to recover
		} else {
			if (preferredApplication == 0) {
				preferredApplication = new BString(string);
			} else if (preferredApplication->Compare(string) != 0) {
				preferredApplication.SetTo("");
				if (fileType && (fileType->Length() == 0)) {
					break; // stop now, don't waste time checking the rest
				}
			}
		}
	}
	fView->SetFileType(fileType.String());
	fView->SetPreferredApplication(preferredApplication.String());
	delete fileType;
	delete preferredApplication;
}

const char *
FileTypeWindow::SummarizeEntries()
{
	if (fEntries->Rewind() != B_OK) {
		return "<error>";
	}
	const char * result = "";
	dirent dent;
	if (fEntries->GetNextDirents(&dent,sizeof(dirent),1) != 0) {
		result = strdup(dent.d_name);
		if (fEntries->GetNextDirents(&dent,sizeof(dirent),1) != 0) {
			result = "[Multiple Files]";
		}
	}
	return result;
}
