/*

ResourceUsageWindow

Author: Sikosis

(C)2003-2004 OBOS - Released under the MIT License

*/

// Includes ------------------------------------------------------------------------------------------ //
#include <Application.h>
#include <List.h>
#include <ListView.h>
#include <Path.h>
#include <Screen.h>
#include <ScrollView.h>
#include <stdio.h>
#include <String.h>
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
	BRect rlist;
	r = Bounds();
    rtab = Bounds();
    rtab.top += 10;
    rlist = Bounds();
    rlist.top += 10;
    rlist.left += 12;
    rlist.right -= 15;
    rlist.bottom -= 47;
    
    ptrIRQView = new IRQView(r);
        
	// Create the TabView and Tabs
	tabView = new BTabView(rtab,"resource_usage_tabview");
	tabView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	rtab = tabView->Bounds();
	rtab.InsetBy(5,5);
	tab = new BTab();
	tabView->AddTab(ptrIRQView, tab);
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
	
	// Create the ListViews
	BListView *IRQListView;
	BListView *DMAListView;
	
	IRQListView = new BListView(rlist, "IRQListView", B_SINGLE_SELECTION_LIST, B_FOLLOW_LEFT | B_FOLLOW_TOP,
						B_WILL_DRAW | B_NAVIGABLE);
	DMAListView = new BListView(rlist, "DMAListView", B_SINGLE_SELECTION_LIST, B_FOLLOW_LEFT | B_FOLLOW_TOP,
						B_WILL_DRAW | B_NAVIGABLE);
						
	BString tmp;
	tmp.SetTo("0       Timer");
	Lock();
	IRQListView->AddItem(new BStringItem(tmp.String()));
	Unlock();
    ptrIRQView->AddChild(IRQListView);
    
    // write function to add items to IRQ list 
    
    // write another function that just deals with getting the IRQ list
    
	
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

