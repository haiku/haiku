/*

ResourceUsageWindow

Author: Sikosis

(C)2003 OBOS - Released under the MIT License

*/

// Includes ------------------------------------------------------------------------------------------ //
#include <Application.h>
#include <Path.h>
#include <Screen.h>
#include <ScrollView.h>
#include <stdio.h>
#include <string.h>
#include <TabView.h>
#include <Window.h>
#include <View.h>

#include "Devices.h"
#include "DevicesWindows.h"
#include "DevicesViews.h"
// -------------------------------------------------------------------------------------------------- //

// CenterWindowOnScreen -- Centers the BWindow to the Current Screen
static void CenterWindowOnScreen(BWindow* w)
{
	BRect screenFrame = (BScreen(B_MAIN_SCREEN_ID).Frame());
	BPoint pt;
	pt.x = screenFrame.Width()/2 - w->Bounds().Width()/2;
	pt.y = screenFrame.Height()/2 - w->Bounds().Height()/2;

	if (screenFrame.Contains(pt))
		w->MoveTo(pt);
}
// -------------------------------------------------------------------------------------------------- //


// ResourceUsageWindow - Constructor
ResourceUsageWindow::ResourceUsageWindow(BRect frame) : BWindow (frame, "Resource Usage", B_TITLED_WINDOW, B_MODAL_SUBSET_WINDOW_FEEL , 0)
{
	InitWindow();
	CenterWindowOnScreen(this);
	Show();
}
// -------------------------------------------------------------------------------------------------- //


// ResourceUsageWindow - Destructor
ResourceUsageWindow::~ResourceUsageWindow()
{
	//exit(0);
}
// -------------------------------------------------------------------------------------------------- //


// ResourceUsageWindow::InitWindow -- Initialization Commands here
void ResourceUsageWindow::InitWindow(void)
{
	BRect r;
	BRect rtab;
	r = Bounds(); // the whole view
    rtab = Bounds();
    
	rtab.InsetBy(0,10);
	tabView = new BTabView(rtab,"resource_usage_tabview");
	tabView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	rtab = tabView->Bounds();
	rtab.InsetBy(5,5);
	tab = new BTab();
	tabView->AddTab(new IRQView(r), tab);
	tab->SetLabel("IRQ");
	tab = new BTab();
	tabView->AddTab(new DMAView(r), tab);
	tab->SetLabel("DMA");
	tab = new BTab();
	tabView->AddTab(new IORangeView(r), tab);
	tab->SetLabel("IO Range");
	tab = new BTab();
	tabView->AddTab(new MemoryRangeView(r), tab);
	tab->SetLabel("Memory Range");
	
	// Create the Views
	AddChild(ptrResourceUsageView = new ResourceUsageView(r));
	ptrResourceUsageView->AddChild(tabView);
}
// -------------------------------------------------------------------------------------------------- //


// ResourceUsageWindow::MessageReceived -- receives messages
void ResourceUsageWindow::MessageReceived (BMessage *message)
{
	switch(message->what)
	{
		default:
			BWindow::MessageReceived(message);
			break;
	}
}
// -------------------------------------------------------------------------------------------------- //

