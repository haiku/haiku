/*
	SettingsView.cpp
*/
#ifndef _RADIO_BUTTON_H
#include <RadioButton.h>
#endif

#ifndef _STRING_VIEW_H
#include <StringView.h>
#endif

#ifndef _VIEW_H
#include <View.h>
#endif
////////
#ifndef SETTINGS_VIEW_H
#include "SettingsView.h"
#endif
SettingsView::SettingsView(BRect frame):
	BView(frame,"Settings",B_FOLLOW_ALL,B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP | B_PULSE_NEEDED)
	{
		buildView();
	} //SettingsView::SettingsView()
	
void
SettingsView:: buildView()
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	//month/date/year
	//BBox?
	//BRect monthFrame(8.0, 1.0, 150.0, 20.0);
	
	//the calender
	
	//hour/minute/second AM/PM
	
	//the clock
	
	
	//"Clock set to:"-label
	BStringView *fClockSet;
	
	const char *clockset= "Clock set to: ";
	float csLength= be_plain_font->StringWidth(clockset);
	BRect clockSetframe(178.0, 147.0, 178.0+csLength, 157.0);
	fClockSet= new BStringView(clockSetframe, "clockset", clockset);
	AddChild(fClockSet);
	
	//the radiobuttons..
	//local time
	BRadioButton *locButton;
	BRect locFrame(247.0, 144.0, 324.0, 154.0);
	locButton= new BRadioButton(locFrame,"loc", "Local time", NULL);
	AddChild(locButton); 
	//GMT
	BRadioButton *gmtButton;
	BRect gmtFrame(247.0, 160.0, 324.0, 170.0);
	gmtButton= new BRadioButton(gmtFrame,"gmt", "GMT", NULL);
	AddChild(gmtButton);
	
}//SettingsView::buildView()

void
SettingsView::changeRTCSetting()
{
	/*
		edit file "RTC_time_settings" in B_USER_SETTINGS_DIRECTORY
		if GMT is set, file reads "GMT", if set to local time, file reads "local"
		If no such file exists, create one!
		
		if(file_exists){
			open file
			read file
			if content != msgstring
				change content
		}		
		else
			create file RTC_time_settings
			write string from msg to file
					
	*/
	
	
}//SettingsView::changeRTCSetting()

void
SettingsView::Draw(BRect frame)
{
	//inherited::Draw(updateFrame);
	//draw a separator line
	rgb_color black= {0,0,0,255};
	SetLowColor(black);
	
	BPoint start(160.0, 5.0);
	BPoint end(160.0, 175.0);
	StrokeLine(start, end, B_SOLID_LOW);
}//SettingsView::Draw(BRect frame)


