/*
	
	MouseView.cpp
	
*/
#include <InterfaceDefs.h>
#include <Button.h>
#include <Box.h>
#include <Bitmap.h>
#include <TranslationUtils.h>
#include <TextControl.h>
#include <Slider.h>
#include <PopUpMenu.h>
#include <MenuField.h>
#include <MenuItem.h>

#include "MouseView.h"
#include "MouseMessages.h"


MouseView::MouseView(BRect rect)
	   	   : BBox(rect, "mouse_view",
					B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP,
					B_PLAIN_BORDER)
{
	BRect			frame;
	BButton			*button;
	BTextControl 	*textcontrol;
	BSlider			*slider;
	BPopUpMenu		*popupmenu;
	BMenuField		*menufield;
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	fDoubleClickBitmap = BTranslationUtils::GetBitmap("double_click_bmap");
	fSpeedBitmap = BTranslationUtils::GetBitmap("speed_bmap");
	fAccelerationBitmap = BTranslationUtils::GetBitmap("acceleration_bmap");
	
	// Add the "Default" button..	
	frame.Set(10,259,85,279);
	button = new BButton(frame,"mouse_defaults","Defaults", new BMessage(BUTTON_DEFAULTS));
	AddChild(button);
	
	// Add the "Revert" button...
	frame.Set(92,259,167,279);
	button = new BButton(frame,"mouse_revert","Revert", new BMessage(BUTTON_REVERT));
	button->SetEnabled(false);
	AddChild(button);
	
	// Create the main box for the controls...
	frame=Bounds();
	frame.left=frame.left+11;
	frame.top=frame.top+11;
	frame.right=frame.right-11;
	frame.bottom=frame.bottom-44;
	fBox = new BBox(frame,"mouse_box",B_FOLLOW_LEFT,B_WILL_DRAW,B_FANCY_BORDER);	
	
	// Add the "Mouse Type" pop up menu
	popupmenu = new BPopUpMenu("Mouse Type Menu");
	popupmenu->AddItem(new BMenuItem("1-Button",new BMessage(POPUP_MOUSE_TYPE)));
	popupmenu->AddItem(new BMenuItem("2-Button",new BMessage(POPUP_MOUSE_TYPE)));
	popupmenu->AddItem(new BMenuItem("3-Button",new BMessage(POPUP_MOUSE_TYPE)));

	frame.Set(18,10,208,20);
	menufield = new BMenuField(frame, "mouse_type", "Mouse Type", popupmenu);
	menufield->SetDivider(menufield->Divider() - 30);
	fBox->AddChild(menufield);
	
	// Add the "Mouse Type" pop up menu
	popupmenu = new BPopUpMenu("Focus Follows Mouse Menu");
	popupmenu->AddItem(new BMenuItem("Disabled",new BMessage(BUTTON_REVERT)));
	popupmenu->AddItem(new BMenuItem("Enabled",new BMessage(BUTTON_REVERT)));
	popupmenu->AddItem(new BMenuItem("Warping",new BMessage(BUTTON_REVERT)));
	popupmenu->AddItem(new BMenuItem("Instant-Warping",new BMessage(BUTTON_REVERT)));

	frame.Set(168,207,440,200);
	menufield = new BMenuField(frame, "Focus follows mouse", "Focus follows mouse:", popupmenu);
	menufield->SetDivider(menufield->Divider() - 30);
	fBox->AddChild(menufield);
		
	// Create the "Double-click speed slider...
	frame.Set(168,10,328,50);
	slider = new BSlider(frame,"double_click_speed","Double-click speed", new BMessage(SLIDER_DOUBLE_CLICK_SPEED),0,1000000,B_BLOCK_THUMB,B_FOLLOW_LEFT,B_WILL_DRAW);
	slider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	slider->SetHashMarkCount(5);
	slider->SetLimitLabels("Slow","Fast");
	fBox->AddChild(slider);
	
	// Create the "Mouse Speed" slider...
	frame.Set(168,75,328,125);
	slider = new BSlider(frame,"mouse_speed","Mouse Speed", new BMessage(SLIDER_MOUSE_SPEED),8192,524287,B_BLOCK_THUMB,B_FOLLOW_LEFT,B_WILL_DRAW);
	slider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	slider->SetHashMarkCount(7);
	slider->SetLimitLabels("Slow","Fast");
	fBox->AddChild(slider);

	// Create the "Mouse Acceleration" slider...
	frame.Set(168,140,328,190);
	slider = new BSlider(frame,"mouse_acceleration","Mouse Acceleration", new BMessage(SLIDER_MOUSE_SPEED),250000,1000000,B_BLOCK_THUMB,B_FOLLOW_LEFT,B_WILL_DRAW);
	slider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	slider->SetHashMarkCount(5);
	slider->SetLimitLabels("Slow","Fast");
	fBox->AddChild(slider);
	
	// Create the "Double-click test area" text box...
	frame=fBox->Bounds();
	frame.left=frame.left+10;
	frame.right=frame.right-230;
	frame.top=frame.bottom-30;
	textcontrol = new BTextControl(frame,"double_click_test_area",NULL,"Double-click test area", new BMessage(DOUBLE_CLICK_TEST_AREA),B_FOLLOW_LEFT,B_WILL_DRAW);
	textcontrol->SetAlignment(B_ALIGN_LEFT,B_ALIGN_CENTER);
	fBox->AddChild(textcontrol);
	
	AddChild(fBox);
	
}

void MouseView::Draw(BRect updateFrame)
{
	inherited::Draw(updateFrame);
	fBox->SetHighColor(120,120,120);
	fBox->SetLowColor(255,255,255);
	// Line above the test area
	fBox->StrokeLine(BPoint(10,199),BPoint(150,199),B_SOLID_HIGH);
	fBox->StrokeLine(BPoint(11,200),BPoint(150,200),B_SOLID_LOW);
	// Line above focus follows mouse
	fBox->StrokeLine(BPoint(170,199),BPoint(362,199),B_SOLID_HIGH);
	fBox->StrokeLine(BPoint(171,200),BPoint(362,200),B_SOLID_LOW);
	// Line in the middle
	fBox->StrokeLine(BPoint(160,10),BPoint(160,230),B_SOLID_HIGH);
	fBox->StrokeLine(BPoint(161,11),BPoint(161,230),B_SOLID_LOW);
	//Draw the icons
	fBox->DrawBitmap(fDoubleClickBitmap,BPoint(341,20));
	fBox->DrawBitmap(fSpeedBitmap,BPoint(331,90));	
	fBox->DrawBitmap(fAccelerationBitmap,BPoint(331,155));

}