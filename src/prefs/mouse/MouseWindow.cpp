/*
 * MouseWindow.cpp
 * Mouse mccall@digitalparadise.co.uk
 *
 */
 
#include <Application.h>
#include <Message.h>
#include <Screen.h>
#include <Slider.h>
#include <Button.h>
#include <Menu.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <Beep.h>

#include "MouseMessages.h"
#include "MouseWindow.h"
#include "MouseView.h"
#include "Mouse.h"

#define MOUSE_WINDOW_RIGHT		397
#define MOUSE_WINDOW_BOTTTOM	293

MouseWindow::MouseWindow()
				: BWindow(BRect(0,0,MOUSE_WINDOW_RIGHT,MOUSE_WINDOW_BOTTTOM), "Mouse", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE )
{
	BScreen screen;
	BSlider *slider=NULL;
	BMenuField *menufield=NULL;

	MoveTo(dynamic_cast<MouseApplication *>(be_app)->WindowCorner());

	// Code to make sure that the window doesn't get drawn off screen...
	if (!(screen.Frame().right >= Frame().right && screen.Frame().bottom >= Frame().bottom))
		MoveTo((screen.Frame().right-Bounds().right)*.5,(screen.Frame().bottom-Bounds().bottom)*.5);
	
	BuildView();
	AddChild(fView);


	menufield = (BMenuField *)FindView("mouse_type");
	if (menufield !=NULL)
		menufield->Menu()->ItemAt((dynamic_cast<MouseApplication *>(be_app)->MouseType())-1)->SetMarked(true);
				
	slider = (BSlider *)FindView("double_click_speed");
	if (slider !=NULL) slider->SetValue((dynamic_cast<MouseApplication *>(be_app)->ClickSpeed()));

	slider = (BSlider *)FindView("mouse_speed");
	if (slider !=NULL) slider->SetValue((dynamic_cast<MouseApplication *>(be_app)->MouseSpeed()));
	Show();

}

void
MouseWindow::BuildView()
{
	fView = new MouseView(Bounds());	
}

bool
MouseWindow::QuitRequested()
{

	BSlider *slider=NULL;
	BMenuField *menufield=NULL;

	dynamic_cast<MouseApplication *>(be_app)->SetWindowCorner(BPoint(Frame().left,Frame().top));
	
	slider = (BSlider *)FindView("double_click_speed");
	if (slider !=NULL)
		dynamic_cast<MouseApplication *>(be_app)->SetClickSpeed(slider->Value());
	

	
	menufield = (BMenuField *)FindView("mouse_type");
	if (menufield !=NULL) {
		BMenu *menu;
		menu = menufield->Menu();
		dynamic_cast<MouseApplication *>(be_app)->SetMouseType((mouse_type)(menu->IndexOf(menu->FindMarked())+1));
	}
	
	be_app->PostMessage(B_QUIT_REQUESTED);	
	return(true);
}

void
MouseWindow::MessageReceived(BMessage *message)
{
	BSlider *slider=NULL;
	BButton *button=NULL;
	BMenuField *menufield=NULL;

	switch(message->what) {
		case BUTTON_DEFAULTS: {
			if (set_click_speed(500000)!=B_OK) 
	   			be_app->PostMessage(ERROR_DETECTED);
			slider = (BSlider *)FindView("double_click_speed");
			if (slider !=NULL) slider->SetValue(500000);
			
			button = (BButton *)FindView("mouse_revert");
	  		if (button !=NULL) button->SetEnabled(true);
		}
		break;
		case POPUP_MOUSE_TYPE: {
			beep();
			menufield = (BMenuField *)FindView("mouse_type");
			if (menufield !=NULL) {
				BMenu *menu;
				menu = menufield->Menu();
				if (set_mouse_type(menu->IndexOf(menu->FindMarked())+1) !=B_OK)
	   				be_app->PostMessage(ERROR_DETECTED);
			}
		}
		break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
	
}

MouseWindow::~MouseWindow()
{

}