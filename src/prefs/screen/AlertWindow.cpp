#include <MessageRunner.h>
#include <Application.h>
#include <Messenger.h>
#include <Message.h>
#include <Button.h>
#include <Window.h>

#include <Beep.h>

#include "AlertWindow.h"
#include "AlertView.h"
#include "Constants.h"

AlertWindow::AlertWindow(BRect frame)
	: BWindow(frame, "Revert", B_MODAL_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL, B_NOT_RESIZABLE | B_NOT_ZOOMABLE, B_ALL_WORKSPACES)
{
	frame = Bounds();
	
	fAlertView = new AlertView(frame, "ScreenView");
	
	AddChild(fAlertView);
	
	BRect ButtonRect;
	
	ButtonRect.Set(215.0, 59.0, 400.0, 190.0);
	
	fKeepButton = new BButton(ButtonRect, "KeepButton", "Keep", 
	new BMessage(BUTTON_KEEP_MSG));
	
	fKeepButton->AttachedToWindow();
	fKeepButton->ResizeToPreferred();
	
	fAlertView->AddChild(fKeepButton);
	
	ButtonRect.Set(130.0, 59.0, 400.0, 190.0);
	
	fRevertButton = new BButton(ButtonRect, "RevertButton", "Revert", 
	new BMessage(BUTTON_REVERT_MSG));
	
	fRevertButton->AttachedToWindow();
	fRevertButton->ResizeToPreferred();
	
	fAlertView->AddChild(fRevertButton);
	
	BMessenger Messenger(this);
	
	fRunner = new BMessageRunner(Messenger, new BMessage(DIM_COUNT_MSG), 1000000, 10);
	
	Show();
}

bool AlertWindow::QuitRequested()
{
	delete fRunner;

	Quit();
	
	return(true);
}

void AlertWindow::MessageReceived(BMessage *message)
{
	switch (message->what)
	{
		case BUTTON_KEEP_MSG:
		{
			be_app->PostMessage(MAKE_INITIAL_MSG);
		
			PostMessage(B_QUIT_REQUESTED);
		
			break;
		}
		
		case BUTTON_REVERT_MSG:
		{
			be_app->PostMessage(SET_INITIAL_MODE_MSG);
		
			PostMessage(B_QUIT_REQUESTED);
		
			break;
		}
		
		case DIM_COUNT_MSG:
		{
			fAlertView->Count = fAlertView->Count - 1;
			
			fAlertView->Invalidate(BRect(180.0, 20.0, 260.0, 50.0));
			
			if (fAlertView->Count == 0)
				PostMessage(BUTTON_REVERT_MSG);
		
			break;
		}
		
		default:
			BWindow::MessageReceived(message);
			
			break;
	}
}
