//MainControlWindow.cpp

#include <Application.h>
#include <Alert.h>
#include <Message.h>
#include <TranslatorRoster.h>
#include "WorkWindow.h"
#include "MainControlWindow.h"

MainControlWindow::MainControlWindow()
	: BWindow(BRect(100, 100, 220, 230), "Multi Test", B_TITLED_WINDOW, B_WILL_DRAW)
{
	fbtnAddView = new BButton(BRect(10, 10, 110, 60), "Add Test Window", "Add Test Window",
		new BMessage(ADD_VIEW_BUTTON_ID));
	AddChild(fbtnAddView);
	
	fbtnAddTranslators = new BButton(BRect(10, 70, 110, 120), "Add Translators", "Add Translators",
		new BMessage(ADD_TRANSLATORS_BUTTON_ID));
	AddChild(fbtnAddTranslators);
	
	for (int i = 0; i < 5; i++) {
		WorkWindow *pWindow;
		pWindow = new WorkWindow(BRect(100 + (10*i), 100, 300 + (10*i), 300));
		pWindow->Show();
	}
}

bool
MainControlWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return BWindow::QuitRequested();
}

void
MainControlWindow::MessageReceived(BMessage *pMsg)
{
	switch (pMsg->what) {
		case ADD_VIEW_BUTTON_ID:
			//BAlert *pAlert;
			//pAlert = new BAlert("Title", "text", "OK");
			//pAlert->Go();
			WorkWindow *pWindow;
			pWindow = new WorkWindow(BRect(100, 100, 300, 300));
			pWindow->Show();
			break;
			
		case ADD_TRANSLATORS_BUTTON_ID:
			BTranslatorRoster *pRoster;
			pRoster = BTranslatorRoster::Default();
			pRoster->AddTranslators();
			break;
			
		default:
			BWindow::MessageReceived(pMsg);
			break;
	}
}
