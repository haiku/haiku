#include <Application.h>
#include <Button.h>
#include <MessageRunner.h>
#include <Window.h>

#include "AlertWindow.h"
#include "AlertView.h"
#include "Constants.h"

AlertWindow::AlertWindow(BRect frame)
	: BWindow(frame, "Revert", B_MODAL_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL, B_NOT_RESIZABLE | B_NOT_ZOOMABLE, B_ALL_WORKSPACES)
{
	frame = Bounds();
	
	fAlertView = new AlertView(frame, "ScreenView");
	
	AddChild(fAlertView);
	
	BRect buttonRect;	
	buttonRect.Set(215.0, 59.0, 400.0, 190.0);
	
	fKeepButton = new BButton(buttonRect, "KeepButton", "Keep", 
		new BMessage(BUTTON_KEEP_MSG));
	
	fKeepButton->AttachedToWindow();
	fKeepButton->ResizeToPreferred();
	
	fAlertView->AddChild(fKeepButton);
	
	buttonRect.Set(130.0, 59.0, 400.0, 190.0);
	
	fRevertButton = new BButton(buttonRect, "RevertButton", "Revert", 
		new BMessage(BUTTON_REVERT_MSG));
	
	fRevertButton->AttachedToWindow();
	fRevertButton->ResizeToPreferred();
	
	fAlertView->AddChild(fRevertButton);
	
	fRunner = new BMessageRunner(this, new BMessage(DIM_COUNT_MSG), 1000000, 10);
}


bool
AlertWindow::QuitRequested()
{
	delete fRunner;
	
	return BWindow::QuitRequested();
}


void
AlertWindow::MessageReceived(BMessage *message)
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
			fAlertView->count -= 1;
			
			fAlertView->Invalidate(BRect(180.0, 20.0, 260.0, 50.0));
			
			if (fAlertView->count == 0)
				PostMessage(BUTTON_REVERT_MSG);
		
			break;
		}
		
		default:
			BWindow::MessageReceived(message);
			
			break;
	}
}
