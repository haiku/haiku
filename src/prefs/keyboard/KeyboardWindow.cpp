/*
 * KeyboardWindow.cpp
 * Keyboard mccall@digitalparadise.co.uk
 *
 */
 
#include <Application.h>
#include <TextControl.h>
#include <Message.h>
#include <Button.h>
#include <Slider.h>
#include <Screen.h>
#include <stdio.h>

#include "KeyboardMessages.h"
#include "KeyboardWindow.h"
#include "KeyboardView.h"
#include "Keyboard.h"

#define KEYBOARD_WINDOW_RIGHT	229
#define KEYBOARD_WINDOW_BOTTTOM	221

KeyboardWindow::KeyboardWindow()
				: BWindow(BRect(0,0,KEYBOARD_WINDOW_RIGHT,KEYBOARD_WINDOW_BOTTTOM), "Keyboard", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE )
{
	BScreen screen;
	BSlider *slider=NULL;

	MoveTo(dynamic_cast<KeyboardApplication *>(be_app)->WindowCorner());

	// Code to make sure that the window doesn't get drawn off screen...
	if (!(screen.Frame().right >= Frame().right && screen.Frame().bottom >= Frame().bottom))
		MoveTo((screen.Frame().right-Bounds().right)*.5,(screen.Frame().bottom-Bounds().bottom)*.5);
	
	BuildView();
	AddChild(fView);

	slider = (BSlider *)FindView("key_repeat_rate");
	if (slider !=NULL) slider->SetValue(dynamic_cast<KeyboardApplication *>(be_app)->KeyboardRepeatRate());
	printf("On start repeat rate: %ld\n", dynamic_cast<KeyboardApplication *>(be_app)->KeyboardRepeatRate());
	
	slider = (BSlider *)FindView("delay_until_key_repeat");
	if (slider !=NULL) slider->SetValue(dynamic_cast<KeyboardApplication *>(be_app)->KeyboardRepeatDelay());
	printf("On start repeat delay: %ld\n", dynamic_cast<KeyboardApplication *>(be_app)->KeyboardRepeatDelay());

	Show();

}

void
KeyboardWindow::BuildView()
{
	fView = new KeyboardView(Bounds());	
}

bool
KeyboardWindow::QuitRequested()
{
	BSlider *slider=NULL;

	dynamic_cast<KeyboardApplication *>(be_app)->SetWindowCorner(BPoint(Frame().left,Frame().top));
	
	slider = (BSlider *)FindView("key_repeat_rate");
	if (slider !=NULL) dynamic_cast<KeyboardApplication *>(be_app)->SetKeyboardRepeatRate(slider->Value());
	printf("On quit repeat rate: %ld\n", dynamic_cast<KeyboardApplication *>(be_app)->KeyboardRepeatRate());
	
	slider = (BSlider *)FindView("delay_until_key_repeat");
	if (slider !=NULL) dynamic_cast<KeyboardApplication *>(be_app)->SetKeyboardRepeatDelay(slider->Value());
	printf("On quit repeat delay: %ld\n", dynamic_cast<KeyboardApplication *>(be_app)->KeyboardRepeatDelay());

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
			if (set_key_repeat_rate(200)!=B_OK) 
	   			be_app->PostMessage(ERROR_DETECTED);
			slider = (BSlider *)FindView("key_repeat_rate");
			if (slider !=NULL) slider->SetValue(200);
			
			if (set_key_repeat_delay(250000)!=B_OK) 
	   			be_app->PostMessage(ERROR_DETECTED);
			slider = (BSlider *)FindView("delay_until_key_repeat");
			if (slider !=NULL) slider->SetValue(250000);
			
			button = (BButton *)FindView("keyboard_revert");
	  		if (button !=NULL) button->SetEnabled(true);
			}
			break;
		case BUTTON_REVERT:{
			if (set_key_repeat_rate(dynamic_cast<KeyboardApplication *>(be_app)->KeyboardRepeatRate())!=B_OK) 
	    		be_app->PostMessage(ERROR_DETECTED);
			slider = (BSlider *)FindView("key_repeat_rate");
			if (slider !=NULL) slider->SetValue(dynamic_cast<KeyboardApplication *>(be_app)->KeyboardRepeatRate());	    		
	    			
			if (set_key_repeat_delay(dynamic_cast<KeyboardApplication *>(be_app)->KeyboardRepeatDelay())!=B_OK) 
	    		be_app->PostMessage(ERROR_DETECTED);
			slider = (BSlider *)FindView("delay_until_key_repeat");
			if (slider !=NULL) slider->SetValue(dynamic_cast<KeyboardApplication *>(be_app)->KeyboardRepeatDelay());	    		
			
			button = (BButton *)FindView("keyboard_revert");
	  		if (button !=NULL) button->SetEnabled(false);
	  		}
			break;
		case SLIDER_REPEAT_RATE:{
			if (set_key_repeat_rate(message->FindInt32("be:value"))!=B_OK) 
	    		be_app->PostMessage(ERROR_DETECTED);

			button = (BButton *)FindView("keyboard_revert");
	  		if (button !=NULL) button->SetEnabled(true);
			}
			break;
		case SLIDER_DELAY_RATE:{
				bigtime_t        rate;
				rate=message->FindInt32("be:value");
				// We need to look at the value from the slider and make it "jump"
				// to the next notch along. Setting the min and max values of the
				// slider to 1 and 4 doesn't work like the real Keyboard app.
				if (rate < 375000)
					rate = 250000;
				if ((rate >= 375000)&&(rate < 625000))
					rate = 500000;
				if ((rate >= 625000)&&(rate < 875000))
					rate = 750000;
				if (rate >= 875000)
					rate = 1000000;
				
				if (set_key_repeat_delay(rate)!=B_OK) 
	    			be_app->PostMessage(ERROR_DETECTED);
	    		slider = (BSlider *)FindView("delay_until_key_repeat");
				if (slider !=NULL) slider->SetValue(rate);

				button = (BButton *)FindView("keyboard_revert");
	  			if (button !=NULL) button->SetEnabled(true);
	  		}
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
	
}

KeyboardWindow::~KeyboardWindow()
{

}