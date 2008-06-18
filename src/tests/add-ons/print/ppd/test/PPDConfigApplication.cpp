/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#include "PPDConfigApplication.h"
#include "PrinterSelection.h"

AppWindow::AppWindow(BRect aRect) 
	: BWindow(aRect, APPLICATION, B_TITLED_WINDOW, 0) {
	// add menu bar
	BRect rect = BRect(0, 0, aRect.Width(), aRect.Height());
	fMenuBar = new BMenuBar(rect, "menu_bar");
	BMenu *menu; 

	menu = new BMenu("File");
	menu->AddItem(new BMenuItem("About ...", new BMessage(B_ABOUT_REQUESTED), 'A'));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q')); 
	fMenuBar->AddItem(menu);

	AddChild(fMenuBar);
	
	float x = aRect.Width() / 2 - 3;
	float right = rect.right - 3;
	
	// add view
	aRect.Set(0, fMenuBar->Bounds().Height()+1, x, aRect.Height());
	PrinterSelectionView* printerSelection = new PrinterSelectionView(aRect, 
		"printer-selection", 
		B_FOLLOW_TOP_BOTTOM, 
		B_WILL_DRAW);
	AddChild(printerSelection);
	printerSelection->SetMessage(new BMessage('prnt'));
	printerSelection->SetTarget(this);
	
	aRect.left = x + 3;
	aRect.right = right;
	AddChild(fConfig = new PPDConfigView(aRect, "ppd-config", 
		B_FOLLOW_ALL_SIDES, 
		B_WILL_DRAW));

	// make window visible
	Show();
}

void AppWindow::MessageReceived(BMessage *message) {
const char* file;
	switch(message->what) {
	case MENU_APP_NEW: 
		break; 
	case B_ABOUT_REQUESTED:
		AboutRequested();
		break;
	case 'prnt':
		if (message->FindString("file", &file) == B_OK) {
			BMessage settings;
			fConfig->Set(file, settings);
		}
		break;
	default:
		BWindow::MessageReceived(message);
	}
}


bool AppWindow::QuitRequested() {
	be_app->PostMessage(B_QUIT_REQUESTED);
	return(true);
}

void AppWindow::AboutRequested() {
	BAlert *about = new BAlert(APPLICATION, 
		APPLICATION " " VERSION "\nPrototype for PPD printer selection and configuration.\n\n"
		"Written 2008.\n\n"
		"By Michael Pfeiffer.\n\n"
		"EMail: laplace@users.sourceforge.net.","Close");
		about->Go();
}

PPDConfigApplication::PPDConfigApplication() : BApplication(SIGNATURE) {
	BRect aRect;
	// set up a rectangle and instantiate a new window
	aRect.Set(100, 80, 950, 580);
	window = NULL;
	window = new AppWindow(aRect);		
}

int main(int argc, char *argv[]) { 
	PPDConfigApplication app;
	app.Run();
	return 0;
}
