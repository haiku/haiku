#include <Locker.h>
#include <Window.h>
#include <String.h>
#include <Button.h>

#include <Alert.h>

#include "RefreshWindow.h"
#include "RefreshView.h"
#include "RefreshSlider.h"
#include "Constants.h"

RefreshWindow::RefreshWindow(BRect frame, int32 value)
	: BWindow(frame, "Refresh Rate", B_MODAL_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE, B_ALL_WORKSPACES)
{
	int32 Value = value * 10;

	frame = Bounds();
	
	fRefreshView = new RefreshView(frame, "RefreshView");
	
	AddChild(fRefreshView);
	
	BRect SliderRect;
	
	SliderRect.Set(10.0, 35.0, 299.0, 60.0);
	
	fRefreshSlider = new RefreshSlider(SliderRect);
	
	fRefreshSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fRefreshSlider->SetHashMarkCount(10);
	fRefreshSlider->SetLimitLabels("45.0", "84.5");
	fRefreshSlider->SetKeyIncrementValue(1);
	fRefreshSlider->SetValue(Value);
	fRefreshSlider->SetSnoozeAmount(1);
	fRefreshSlider->SetModificationMessage(new BMessage(SLIDER_MODIFICATION_MSG));
	
	fRefreshView->AddChild(fRefreshSlider);
	
	BRect ButtonRect;
	
	ButtonRect.Set(219.0, 97.0, 230.0, 120.0);
	
	fDoneButton = new BButton(ButtonRect, "DoneButton", "Done", 
	new BMessage(BUTTON_DONE_MSG));
	
	fDoneButton->ResizeToPreferred();
	fDoneButton->MakeDefault(true);
	
	fRefreshView->AddChild(fDoneButton);
	
	ButtonRect.Set(130.0, 97.0, 200.0, 120.0);
	
	fCancelButton = new BButton(ButtonRect, "CancelButton", "Cancel", 
	new BMessage(BUTTON_CANCEL_MSG));
	
	fCancelButton->ResizeToPreferred();
	
	fRefreshView->AddChild(fCancelButton);
	
	Show();
	
	PostMessage(SLIDER_INVOKE_MSG);
}

bool RefreshWindow::QuitRequested()
{
	return(true);
}

void RefreshWindow::WindowActivated(bool active)
{
	if (active == true)
		fRefreshSlider->MakeFocus(true);
	else
		fRefreshSlider->MakeFocus(false);
}

void RefreshWindow::MessageReceived(BMessage* message)
{
	switch(message->what)
	{
		case BUTTON_DONE_MSG:
		{
			BMessenger Messenger("application/x-vnd.RR-SCRN");
	
			BMessage Message(SET_CUSTOM_REFRESH_MSG);
			
			float Value;
			
			Value = (float)fRefreshSlider->Value() / 10;
					
			Message.AddFloat("refresh", Value);
			
			Messenger.SendMessage(&Message);
			
			PostMessage(B_QUIT_REQUESTED);
			
			break;
		}
	
		case BUTTON_CANCEL_MSG:
		{
			PostMessage(B_QUIT_REQUESTED);
			
			break;
		}
		
		case SLIDER_INVOKE_MSG:
		{
			fRefreshSlider->MakeFocus(true);
			
			break;
		}
	
		default:
			BWindow::MessageReceived(message);
		
			break;
	}
}