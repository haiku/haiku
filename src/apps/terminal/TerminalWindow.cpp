#include <stdlib.h>
#include <Alert.h>
#include <Autolock.h>
#include <Debug.h>
#include <Clipboard.h>
#include <File.h>
#include <FindDirectory.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Path.h>
#include <PrintJob.h>
#include <Roster.h>
#include <ScrollView.h>
#include <StorageDefs.h>
#include <String.h>
#include <TextControl.h>
#include <TranslationUtils.h>
#include <Window.h>
#include "Constants.h"
#include "ToggleScrollView.h"
#include "TerminalApp.h"
#include "TerminalTextView.h"
#include "TerminalWindow.h"

TerminalWindow::TerminalWindow(BPoint topLeft, int32 id)
	: BWindow(BRect(topLeft,topLeft+BPoint(560,390)),
			  "terminal",B_DOCUMENT_WINDOW,0)
{
	status_t status = InitWindow(id);
	// TODO: handle error status
	Show();
}

TerminalWindow::TerminalWindow(entry_ref *ref, int32 id)
	: BWindow(BRect(7,16,567,406),"terminal",B_DOCUMENT_WINDOW,0)
{
	status_t status = InitWindow(id,ref);
	// TODO: handle error status
	Show();
}

TerminalWindow::~TerminalWindow()
{
	delete fLogToFilePanel;
	delete fWriteSelectionPanel;
	delete fSaveAsSettingsFilePanel;
	
//	if (fSaveMessage)
//		delete fSaveMessage;
//	if (fPrintSettings)
//		delete fPrintSettings;
}

	
status_t
TerminalWindow::InitWindow(int32 id, entry_ref * settingsRef)
{
	status_t ignore = RestoreSettings(settingsRef);
	
	BString unTitled;
	unTitled.SetTo("Terminal ");
	unTitled << id;
	SetTitle(unTitled.String());
	
	// Add menubar
	fMenuBar = new BMenuBar(BRect(0,0,0,0),"menubar");
	
	AddChild(fMenuBar);

	// Add textview and scrollview
	BRect viewFrame;
	BRect textBounds;
	
	viewFrame = Bounds();
	
	viewFrame.top = fMenuBar->Bounds().Height()+1;
	viewFrame.left = 0;
	
	textBounds = viewFrame;
	textBounds.OffsetTo(B_ORIGIN);
	textBounds.InsetBy(TEXT_INSET, TEXT_INSET);

	fTextView = new TerminalTextView(viewFrame, textBounds, this);
	fTextView->SetStylable(true);
	
	fScrollView = new ToggleScrollView("scrollview", fTextView, 0, false, true, B_PLAIN_BORDER);
	AddChild(fScrollView);
	fTextView->MakeFocus(true);	

	// Terminal menu
	fTerminal = new BMenu("Terminal");
	fMenuBar->AddItem(fTerminal);
	
	fSwitchTerminals = new BMenuItem("Switch Terminals", new BMessage(TERMINAL_SWITCH_TERMINAL), 'G');
	fTerminal->AddItem(fSwitchTerminals);
	fSwitchTerminals->SetTrigger('T');
	fSwitchTerminals->SetTarget(be_app);
	
	fStartNewTerminal = new BMenuItem("Start New Terminal", new BMessage(TERMINAL_START_NEW_TERMINAL), 'N');
	fTerminal->AddItem(fStartNewTerminal);
	fStartNewTerminal->SetTarget(be_app);
	
	fLogToFile = new BMenuItem("Log to File...", new BMessage(TERMINAL_LOG_TO_FILE));
	fTerminal->AddItem(fLogToFile);
	
	// Edit menu
	fEdit = new BMenu("Edit");
	fMenuBar->AddItem(fEdit);
	
	fCopy = new BMenuItem("Copy", new BMessage(B_COPY), 'C');
	fEdit->AddItem(fCopy);
	fCopy->SetTarget(fTextView);
	
	fPaste = new BMenuItem("Paste", new BMessage(B_PASTE), 'V');
	fEdit->AddItem(fPaste);
	fPaste->SetTarget(fTextView);

	fEdit->AddSeparatorItem();
	
	fSelectAll = new BMenuItem("Select All", new BMessage(B_SELECT_ALL), 'A');
	fEdit->AddItem(fSelectAll);
	fSelectAll->SetTarget(fTextView);
	
	fWriteSelection = new BMenuItem("Write Selection...", new BMessage(EDIT_WRITE_SELECTION));
	fEdit->AddItem(fWriteSelection);
	
	fClearAll = new BMenuItem("Clear All", new BMessage(EDIT_CLEAR_ALL), 'L');
	fEdit->AddItem(fClearAll);
	fClearAll->SetTarget(fTextView);

	fEdit->AddSeparatorItem();
	
	fFind = new BMenuItem("Find...", new BMessage(EDIT_FIND),'F');
	fEdit->AddItem(fFind);
	
	fFindBackward = new BMenuItem("Find Backward",new BMessage(EDIT_FIND_BACKWARD),'[');
	fEdit->AddItem(fFindBackward);
	
	fFindForward = new BMenuItem("Find Forward", new BMessage(EDIT_FIND_FORWARD),']');
	fEdit->AddItem(fFindForward);
	
	// Settings menu
	fSettings = new BMenu("Settings");
	fMenuBar->AddItem(fSettings);

	fWindowSize = new BMenu("Window Size");
	fWindowSize->SetRadioMode(true);
	fSettings->AddItem(fWindowSize);
	
	BMenuItem * menuItem;
	BMessage * itemMessage;
	fWindowSize->AddItem(menuItem = new BMenuItem("80x24", itemMessage = new BMessage(SETTINGS_WINDOW_SIZE)));
	itemMessage->AddInt32("columns", 80);
	itemMessage->AddInt32("lines", 24);
	menuItem->SetMarked(true);
	
	fWindowSize->AddItem(menuItem = new BMenuItem("80x25", itemMessage = new BMessage(SETTINGS_WINDOW_SIZE)));
	itemMessage->AddInt32("columns", 80);
	itemMessage->AddInt32("lines", 25);

	fWindowSize->AddItem(menuItem = new BMenuItem("80x40", itemMessage = new BMessage(SETTINGS_WINDOW_SIZE)));
	itemMessage->AddInt32("columns", 80);
	itemMessage->AddInt32("lines", 40);

	fWindowSize->AddItem(menuItem = new BMenuItem("132x24", itemMessage = new BMessage(SETTINGS_WINDOW_SIZE)));
	itemMessage->AddInt32("columns", 132);
	itemMessage->AddInt32("lines", 24);

	fWindowSize->AddItem(menuItem = new BMenuItem("132x25", itemMessage = new BMessage(SETTINGS_WINDOW_SIZE)));
	itemMessage->AddInt32("columns", 132);
	itemMessage->AddInt32("lines", 25);
	
//	//Available fonts
//	font_family plain_family;
//	font_style plain_style;
//	be_plain_font->GetFamilyAndStyle(&plain_family,&plain_style);
//	fCurrentFontItem = 0;
//
//	int32 numFamilies = count_font_families();
//	for ( int32 i = 0; i < numFamilies; i++ ) {
//		font_family localfamily;
//		if ( get_font_family ( i, &localfamily ) == B_OK ) {
//			subMenu=new BMenu(localfamily);
//			subMenu->SetRadioMode(true);
//			fFontMenu->AddItem(menuItem = new BMenuItem(subMenu, new BMessage(FONT_FAMILY)));
//			if (!strcmp(plain_family,localfamily)) {
//				menuItem->SetMarked(true);
//				fCurrentFontItem = menuItem;
//			}
//			int32 numStyles=count_font_styles(localfamily);
//			for(int32 j = 0;j<numStyles;j++){
//				font_style style;
//				uint32 flags;
//				if( get_font_style(localfamily,j,&style,&flags)==B_OK){
//					subMenu->AddItem(menuItem = new BMenuItem(style, new BMessage(FONT_STYLE)));
//					if (!strcmp(plain_style,style)) {
//						menuItem->SetMarked(true);
//					}
//				}
//			}
//		}
//	}	

	// build file panels lazily				
	fLogToFilePanel = 0;
	fWriteSelectionPanel = 0;
	fSaveAsSettingsFilePanel = 0;
}

status_t
TerminalWindow::RestoreSettings(entry_ref * settingsRef)
{
	status_t result = B_OK;
	BFile settingsFile;
	if (settingsRef != 0) {
		result = settingsFile.SetTo(settingsRef,B_READ_ONLY);
		if (result != B_OK) {
			return result;
		}
	} else {
		char settingsDirectory[B_PATH_NAME_LENGTH];
		result = find_directory(B_USER_SETTINGS_DIRECTORY,0,true,
		                        settingsDirectory,B_PATH_NAME_LENGTH);
		if (result != B_OK) {
			return result;
		}
		BPath settingsFilePath(settingsDirectory,"Terminal");
		result = settingsFilePath.InitCheck();
		if (result != B_OK) {
			return result;
		}
		result = settingsFile.SetTo(settingsFilePath.Path(),B_READ_ONLY);
		if (result != B_OK) {
			return result;
		}
	}
	// TODO : actually read the settings file	
	return B_OK;
}

void
TerminalWindow::MessageReceived(BMessage *message)
{
	switch(message->what){
		case B_COPY:
			fTextView->Copy(be_clipboard);
		break;
		case B_PASTE:
			fTextView->Paste(be_clipboard);
		break;			
		case EDIT_CLEAR_ALL:
			fTextView->Clear();
		break;
//		case FONT_SIZE:
//		{
//			float fontSize;
//			message->FindFloat("size",&fontSize);
//			SetFontSize(fontSize);
//		}
//		break;
//		case FONT_FAMILY:
//			{
//			const char * fontFamily = 0, * fontStyle = 0;
//			void * ptr;
//			if (message->FindPointer("source",&ptr) == B_OK) {
//				fCurrentFontItem = static_cast<BMenuItem*>(ptr);
//				fontFamily = fCurrentFontItem->Label();
//			}
//			SetFontStyle(fontFamily, fontStyle);
//			}
//		break;
//		case FONT_STYLE:
//			{
//			const char * fontFamily = 0, * fontStyle = 0;
//			void * ptr;
//			if (message->FindPointer("source",&ptr) == B_OK) {
//				BMenuItem * item = static_cast<BMenuItem*>(ptr);
//				fontStyle = item->Label();
//				BMenu * menu = item->Menu();
//				if (menu != 0) {
//					fCurrentFontItem = menu->Superitem();
//					if (fCurrentFontItem != 0) {
//						fontFamily = fCurrentFontItem->Label();
//					}
//				}
//			}
//			SetFontStyle(fontFamily, fontStyle);
//			}
//		break;
		default:
		BWindow::MessageReceived(message);
		break;
	}
}

void
TerminalWindow::MenusBeginning()
{
}

void
TerminalWindow::Quit()
{
	terminal_app->CloseTerminal();
	BWindow::Quit();
}
	
bool
TerminalWindow::QuitRequested()
{
	return true;
}
	
