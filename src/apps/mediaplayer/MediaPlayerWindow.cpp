#include <Autolock.h>
#include <File.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Roster.h>
#include <Window.h>
#include <stdlib.h>

#include "Constants.h"
#include "MediaPlayerApp.h"
#include "MediaPlayerView.h"
#include "MediaPlayerWindow.h"

using namespace BPrivate;

MediaPlayerWindow::MediaPlayerWindow(BRect frame)
	: BWindow(frame,"Media Player",B_TITLED_WINDOW,0)
{
	InitWindow();
	Show();
}

MediaPlayerWindow::MediaPlayerWindow(BRect frame, entry_ref *ref)
	: BWindow(frame,"Media Player",B_TITLED_WINDOW,0)
{
	InitWindow();
	OpenFile(ref);
	Show();
}

MediaPlayerWindow::~MediaPlayerWindow()
{
}

void
MediaPlayerWindow::InitWindow()
{
	fMenuBar = new BMenuBar(BRect(0,0,0,0),"menubar");
	AddChild(fMenuBar);

	BRect viewFrame;
	viewFrame = Bounds();
	viewFrame.top = fMenuBar->Bounds().Height()+1; // 021021
	viewFrame.left = 0; // 021021
	
	fPlaverView = new MediaPlayerView(viewFrame, this);
	fPlaverView->MakeFocus(true);

	BMenu * menu;
	BMenuItem * menuItem;
	
	// File menu
	fMenuBar->AddItem(menu = new BMenu("File"));
	
	fOpenFileMenu = new BMenu("Open File...");
	menu->AddItem(menuItem = new BMenuItem(fOpenFileMenu, new BMessage(MENU_OPEN_FILE)));
	menuItem->SetShortcut('O',0);
	fOpenURLMenu = new BMenu("Open URL...");
	menu->AddItem(menuItem = new BMenuItem(fOpenURLMenu, new BMessage(MENU_OPEN_URL)));
	menuItem->SetShortcut('U',0);
	menu->AddItem(fFileInfoItem = new BMenuItem("File Info...", new BMessage(MENU_FILE_INFO), 'I'));
	fFileInfoItem->SetEnabled(false);
	menu->AddSeparatorItem();

	menu->AddItem(fCloseItem = new BMenuItem("Close", new BMessage(MENU_CLOSE), 'W'));
	menu->AddItem(fQuitItem = new BMenuItem("Quit", new BMessage(MENU_QUIT), 'Q'));
	
	// View menu
	fMenuBar->AddItem(menu = new BMenu("View"));
	
	menu->AddItem(fMiniModeItem = new BMenuItem("Mini mode", new BMessage(MENU_MINI_MODE), 'T'));
	menu->AddItem(fHalfScaleItem = new BMenuItem("50% scale", new BMessage(MENU_HALF_SCALE), '0'));
	fHalfScaleItem->SetEnabled(false);
	menu->AddItem(fNormalScaleItem = new BMenuItem("100% scale", new BMessage(MENU_NORMAL_SCALE), '1'));
	fNormalScaleItem->SetEnabled(false);
	menu->AddItem(fDoubleScaleItem = new BMenuItem("200% scale", new BMessage(MENU_DOUBLE_SCALE), '2'));
	fDoubleScaleItem->SetEnabled(false);
	menu->AddItem(fFullScreenItem = new BMenuItem("Full screen", new BMessage(MENU_FULL_SCREEN), 'F'));
	fFullScreenItem->SetEnabled(false);

	// Settings menu
	fMenuBar->AddItem(menu = new BMenu("Settings"));
	menu->AddItem(fApplicationPreferencesItem = new BMenuItem("Application Preferences...",
	              new BMessage(MENU_PREFERENCES)));
	menu->AddItem(fLoopItem = new BMenuItem("Loop", new BMessage(MENU_LOOP), 'L'));
	fLoopItem->SetEnabled(false);
	menu->AddItem(fPreserveVideoTimingItem = new BMenuItem("Preserve Video Timing",
	              new BMessage(MENU_PRESERVE_VIDEO_TIMING)));
	fPreserveVideoTimingItem->SetEnabled(false);

}

void
MediaPlayerWindow::MessageReceived(BMessage *message)
{
	if(message->WasDropped()) {
		entry_ref ref;
		if(message->FindRef("refs",0,&ref)==B_OK) {
			message->what = B_REFS_RECEIVED;
			be_app->PostMessage(message);
		}
	}
	
	switch(message->what){
		case MENU_CLOSE: {
			if(this->QuitRequested()) {
				BAutolock lock(this);
				Quit();
			}
		}
		break;
		case MENU_QUIT:
			be_app->PostMessage(B_QUIT_REQUESTED);
		break;
		default:
			BWindow::MessageReceived(message);
		break;
	}
}

void
MediaPlayerWindow::MenusBeginning()
{
	// set up the recent documents menu
	BMessage documents;
	be_roster->GetRecentDocuments(&documents,9,NULL,APP_SIGNATURE);
	
	// delete old items.. 
	//    shatty: it would be preferable to keep the old
	//            menu around instead of continuously thrashing
	//            the menu, but unfortunately there does not
	//            seem to be a straightforward way to update it
	// going backwards may simplify memory management
	for (int i = fOpenFileMenu->CountItems()-1 ; (i >= 0) ; i--) {
		delete fOpenFileMenu->RemoveItem(i);
	}
	// add new items
	int count = 0;
	entry_ref ref;
	while (documents.FindRef("refs",count++,&ref) == B_OK) {
		if ((ref.device != -1) && (ref.directory != -1)) {
			// sanity check passed
			BMessage * openRecent = new BMessage(B_REFS_RECEIVED);
			openRecent->AddRef("refs",&ref);
			BMenuItem * item = new BMenuItem(ref.name,openRecent);
			item->SetTarget(be_app);
			fOpenFileMenu->AddItem(item);
		}
	}
}

void
MediaPlayerWindow::Quit()
{
	media_player_app->CloseDocument();	
	BWindow::Quit();
}

bool
MediaPlayerWindow::QuitRequested()
{
	return true;
}

void
MediaPlayerWindow::OpenFile(entry_ref *ref)
{
	BFile file;
	status_t fileinit;
	
	fileinit = file.SetTo(ref, B_READ_ONLY);
	
	if (fileinit == B_OK) {
		// allocate media file etc.
	} else {
		// complain
	}

	be_roster->AddToRecentDocuments(ref,APP_SIGNATURE);
}
