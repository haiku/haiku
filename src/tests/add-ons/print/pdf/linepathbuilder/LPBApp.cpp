#include "LPBApp.h"

BMessage* NewMessage(uint32 what, uint32 data)
{
	BMessage* m = new BMessage(what);
	m->AddInt32("data", (int32)data);
	return m;
}


AppWindow::AppWindow(BRect aRect) 
	: BWindow(aRect, APPLICATION, B_TITLED_WINDOW, 0) {
	// add menu bar
	BRect rect = BRect(0,0,aRect.Width(), aRect.Height());
	menubar = new BMenuBar(rect, "menu_bar");
	BMenu *menu; 

	menu = new BMenu("Test");
	menu->AddItem(new BMenuItem("About ...", new BMessage(B_ABOUT_REQUESTED)));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q')); 
	menubar->AddItem(menu);

	menu = new BMenu("Line Cap");
	menu->AddItem(new BMenuItem("Round",  NewMessage(CAP_MSG, B_ROUND_CAP)));
	menu->AddItem(new BMenuItem("Butt",   NewMessage(CAP_MSG, B_BUTT_CAP)));
	menu->AddItem(new BMenuItem("Square", NewMessage(CAP_MSG, B_SQUARE_CAP)));
	menubar->AddItem(menu);

	menu = new BMenu("Line Join");
	menu->AddItem(new BMenuItem("Round",  NewMessage(JOIN_MSG, B_ROUND_JOIN)));
	menu->AddItem(new BMenuItem("Miter",  NewMessage(JOIN_MSG, B_MITER_JOIN)));
	menu->AddItem(new BMenuItem("Bevel",  NewMessage(JOIN_MSG, B_BEVEL_JOIN)));
	menu->AddItem(new BMenuItem("Butt",   NewMessage(JOIN_MSG, B_BUTT_JOIN)));
	menu->AddItem(new BMenuItem("Square", NewMessage(JOIN_MSG, B_SQUARE_JOIN)));
	menubar->AddItem(menu);

	menu = new BMenu("Path");
	menu->AddItem(new BMenuItem("Open", new BMessage(OPEN_MSG)));
	menu->AddItem(new BMenuItem("Close", new BMessage(CLOSE_MSG)));
	menubar->AddItem(menu);

	AddChild(menubar);
	// add path view
	aRect.Set(0, menubar->Bounds().Height()+1, aRect.Width(), aRect.Height());
	view = NULL;
	AddChild(view = new PathView(aRect));
	// make window visible
	Show();
}

void AppWindow::MessageReceived(BMessage *message) {
	int32 data;
	message->FindInt32("data", &data);
	
	switch(message->what) {
	case MENU_APP_NEW: 
		break; 
	case B_ABOUT_REQUESTED:
		AboutRequested();
		break;
	case CAP_MSG:
		view->SetLineMode((cap_mode)data, view->LineJoinMode(), view->LineMiterLimit());
		view->Invalidate();
		break;
	case JOIN_MSG:
		view->SetLineMode(view->LineCapMode(), (join_mode)data, view->LineMiterLimit());
		view->Invalidate();
		break;
	case OPEN_MSG:
	case CLOSE_MSG:
		view->SetClose(message->what == CLOSE_MSG);
		view->Invalidate();
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
		APPLICATION " " VERSION "\nThis program is freeware under BSD/MIT license.\n\n"
		"Written 2002.\n\n"
		"By Michael Pfeiffer.\n\n"
		"EMail: michael.pfeiffer@utanet.at.","Close");
		about->Go();
}

App::App() : BApplication("application/x-vnd.obos.LinePathBuilder") {
	BRect aRect;
	// set up a rectangle and instantiate a new window
	aRect.Set(100, 80, 410, 380);
	window = NULL;
	window = new AppWindow(aRect);		
}

int main(int argc, char *argv[]) { 
	be_app = NULL;
	App app;
	be_app->Run();
	return 0;
}
