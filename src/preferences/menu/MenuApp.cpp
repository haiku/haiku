/*
 * Copyright 2002-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		<unknown, please fill in who knows>
 *		Jack Burton
 *		Vasilis Kaoutsis, kaoutsis@sch.gr
 */


#include "MenuApp.h"
#include "MenuWindow.h"	
#include "msg.h"


MenuApp::MenuApp()
	: BApplication("application/x-vnd.Haiku-Menu")
{
	fMenuWindow = new MenuWindow(BRect(100, 100, 400, 300));
	fMenuWindow->Show();
}
	
	
void
MenuApp::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case UPDATE_WINDOW:
		case CLICK_OPEN_MSG:
		case ALLWAYS_TRIGGERS_MSG:
		case CTL_MARKED_MSG:
		case ALT_MARKED_MSG:
		case COLOR_SCHEME_OPEN_MSG:
		case MENU_COLOR:
			fMenuWindow->PostMessage(msg);
			break;
					
		default:
			BApplication::MessageReceived(msg);
			break;
	}
}


//	#pragma mark -


int
main(int, char**)
{
	MenuApp app;
	app.Run();

	return 0;
}
