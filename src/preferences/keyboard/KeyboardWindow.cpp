/*
 * Copyright 2004-2006, the Haiku project. All rights reserved.
 * Distributed under the terms of the Haiku License.
 *
 * Authors in chronological order:
 *  mccall@digitalparadise.co.uk
 *  Jérôme Duval
 *  Marcus Overhagen
*/
#include <Button.h>
#include <Message.h>
#include <Screen.h>
#include <Slider.h>
#include <TextControl.h>

#include "KeyboardMessages.h"
#include "KeyboardWindow.h"
#include "KeyboardView.h"

KeyboardWindow::KeyboardWindow()
 :	BWindow(BRect(0,0,200,200), "Keyboard", B_TITLED_WINDOW,
 			B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{	
	MoveTo(fSettings.WindowCorner());

	// Code to make sure that the window doesn't get drawn off screen...
	BScreen screen;
	if (!(screen.Frame().right >= Frame().right && screen.Frame().bottom >= Frame().bottom))
		MoveTo((screen.Frame().right-Bounds().right)*.5,(screen.Frame().bottom-Bounds().bottom)*.5);
	
	fView = new KeyboardView(Bounds());
	AddChild(fView);

	BSlider *slider = (BSlider *)FindView("key_repeat_rate");
	if (slider !=NULL) 
		slider->SetValue(fSettings.KeyboardRepeatRate());
	
	slider = (BSlider *)FindView("delay_until_key_repeat");
	if (slider !=NULL) 
		slider->SetValue(fSettings.KeyboardRepeatDelay());
	
#ifdef DEBUG
	fSettings.Dump();	
#endif

	Show();
}


bool
KeyboardWindow::QuitRequested()
{
	fSettings.SetWindowCorner(Frame().LeftTop());
	
#ifdef DEBUG
	fSettings.Dump();	
#endif
	
	be_app->PostMessage(B_QUIT_REQUESTED);
	
	return(true);
}

void
KeyboardWindow::MessageReceived(BMessage *message)
{
	BSlider *slider=NULL;
	BButton *button=NULL;
			
	switch(message->what) {
		case BUTTON_DEFAULTS:{
				fSettings.Defaults();
				
				slider = (BSlider *)FindView("key_repeat_rate");
				if (slider !=NULL) 
					slider->SetValue(fSettings.KeyboardRepeatRate());
				
				slider = (BSlider *)FindView("delay_until_key_repeat");
				if (slider !=NULL) 
					slider->SetValue(fSettings.KeyboardRepeatDelay());
				
				button = (BButton *)FindView("keyboard_revert");
		  		if (button !=NULL) 
		  			button->SetEnabled(true);
			}
			break;
		case BUTTON_REVERT:{
				fSettings.Revert();
				
				slider = (BSlider *)FindView("key_repeat_rate");
				if (slider !=NULL) 
					slider->SetValue(fSettings.KeyboardRepeatRate());	    		
		    			
				slider = (BSlider *)FindView("delay_until_key_repeat");
				if (slider !=NULL) 
					slider->SetValue(fSettings.KeyboardRepeatDelay());	    		
				
				button = (BButton *)FindView("keyboard_revert");
		  		if (button !=NULL) 
		  			button->SetEnabled(false);
	  		}
			break;
		case SLIDER_REPEAT_RATE:{
				int32 rate;
				if (message->FindInt32("be:value", &rate)!=B_OK)
					break;
				fSettings.SetKeyboardRepeatRate(rate);
				
				button = (BButton *)FindView("keyboard_revert");
		  		if (button !=NULL) 
	  				button->SetEnabled(true);
			}
			break;
		case SLIDER_DELAY_RATE:{
				int32 delay;
				if (message->FindInt32("be:value", &delay)!=B_OK)
					break;
				// We need to look at the value from the slider and make it "jump"
				// to the next notch along. Setting the min and max values of the
				// slider to 1 and 4 doesn't work like the real Keyboard app.
				if (delay < 375000)
					delay = 250000;
				if ((delay >= 375000)&&(delay < 625000))
					delay = 500000;
				if ((delay >= 625000)&&(delay < 875000))
					delay = 750000;
				if (delay >= 875000)
					delay = 1000000;
				
				fSettings.SetKeyboardRepeatDelay(delay);
				
				slider = (BSlider *)FindView("delay_until_key_repeat");
				if (slider !=NULL) 
					slider->SetValue(delay);

				button = (BButton *)FindView("keyboard_revert");
	  			if (button !=NULL) 
	  				button->SetEnabled(true);
	  		}
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}
