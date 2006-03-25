/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Rafael Romo
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "RefreshWindow.h"
#include "RefreshView.h"
#include "RefreshSlider.h"
#include "Constants.h"

#include <Alert.h>
#include <Application.h>
#include <Button.h>
#include <String.h>
#include <Window.h>


RefreshWindow::RefreshWindow(BRect frame, float current, float min, float max)
	: BWindow(frame, "Refresh Rate", B_MODAL_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE,
		B_ALL_WORKSPACES)
{
	BRect bounds(Bounds());
	bounds.InsetBy(-1, -1);

	fRefreshView = new RefreshView(bounds, "RefreshView");
	AddChild(fRefreshView);

	min = ceilf(min);
	max = floorf(max);

	BRect sliderRect;
	BString maxRefresh;
	maxRefresh << (uint32)min;
	BString minRefresh;
	minRefresh << (uint32)max;

	sliderRect.Set(10.0, 35.0, 299.0, 60.0);

	fRefreshSlider = new RefreshSlider(sliderRect);

	fRefreshSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fRefreshSlider->SetHashMarkCount(uint32(max - min) / 5 + 1);
	fRefreshSlider->SetLimitLabels(minRefresh.String(), maxRefresh.String());
	fRefreshSlider->SetKeyIncrementValue(1);
	fRefreshSlider->SetValue(rint(current * 10));
	fRefreshSlider->SetSnoozeAmount(1);
	fRefreshSlider->SetModificationMessage(new BMessage(SLIDER_MODIFICATION_MSG));

	fRefreshView->AddChild(fRefreshSlider);

	BRect ButtonRect(219.0, 97.0, 230.0, 120.0);

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
}


void
RefreshWindow::WindowActivated(bool active)
{
	fRefreshSlider->MakeFocus(active);
}


void
RefreshWindow::MessageReceived(BMessage* message)
{
	switch(message->what)
	{
		case BUTTON_DONE_MSG:
		{
			BMessage message(SET_CUSTOM_REFRESH_MSG);
		
			float value = (float)fRefreshSlider->Value() / 10;
					
			message.AddFloat("refresh", value);
			be_app->PostMessage(&message);
			
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
