/*
	
	TimeView.cpp
	
*/
#include <InterfaceDefs.h>
#include <TabView.h>
#include <View.h>

#include "TimeView.h"
#include "TimeMessages.h"


TimeView::TimeView(BRect rect)
	   	   : BBox(rect, "time_view",
					B_FOLLOW_ALL, B_FRAME_EVENTS | B_NAVIGABLE_JUMP,
					B_PLAIN_BORDER)
{
	BTabView	*fTabView;
	BTab		*fTab;
	BView		*fView;
	BRect		frame;


	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	frame = Bounds();
	frame.top = 100;
	fTabView = new BTabView(frame,"tab_view");


	// Settings Tab...
	fTab = new BTab();
	fView = new BView(BRect(fTabView->Bounds().InsetByCopy(10, 10)),"settings_view",B_FOLLOW_ALL,
			B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP);
	fView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fTabView->AddTab(fView,fTab);
	fTab->SetLabel("Settings");

	// Time Zone Tab...
	fTab = new BTab();		
	fView = new BView(BRect(fTabView->Bounds().InsetByCopy(10, 10)),"time_zone_view",B_FOLLOW_ALL,
			B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP);
	fView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fTabView->AddTab(fView,fTab);
	fTab->SetLabel("Time Zone");

	// Finally lets add the TabView to the view...	
	AddChild(fTabView);

}


void TimeView::Draw(BRect updateFrame)
{
	inherited::Draw(updateFrame);

}
