#include "MenuApp.h"
#include "MenuWindow.h"	
#include "msg.h"

MenuApp::MenuApp()
	: BApplication("application/x-vnd.Be-GGUI")
{
	fMenuWindow = new MenuWindow(BRect(100, 100, 400, 300));
	fMenuWindow->Show();
}
	
	
void
MenuApp::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		//others
		case UPDATE_WINDOW:
		case CLICK_OPEN_MSG:
		case ALLWAYS_TRIGGERS_MSG:
		case CTL_MARKED_MSG:
		case ALT_MARKED_MSG:
		case COLOR_SCHEME_MSG:
		case MENU_COLOR:
			fMenuWindow->PostMessage(msg);
			break;
					
		default:
			BApplication::MessageReceived(msg);
			break;
	}
}


int main()
{
	new MenuApp();
	be_app->Run();
	delete be_app;
}
	
