/*

ResourceUsageWindow

Author: Sikosis

(C)2003 OBOS - Released under the MIT License

*/

// Includes ------------------------------------------------------------------------------------------ //
#include <Alert.h>
#include <Application.h>
#include <Button.h>
#include <ListItem.h>
#include <ListView.h>
#include <Path.h>
#include <Screen.h>
#include <ScrollView.h>
#include <stdio.h>
#include <string.h>
#include <TextControl.h>
#include <Window.h>
#include <View.h>

#include "Devices.h"
#include "DevicesWindows.h"
#include "DevicesViews.h"
// -------------------------------------------------------------------------------------------------- //

// CenterWindowOnScreen -- Centers the BWindow to the Current Screen
static void CenterWindowOnScreen(BWindow* w)
{
	BRect	screenFrame = (BScreen(B_MAIN_SCREEN_ID).Frame());	BPoint	pt;
	pt.x = screenFrame.Width()/2 - w->Bounds().Width()/2;
	pt.y = screenFrame.Height()/2 - w->Bounds().Height()/2;

	if (screenFrame.Contains(pt))
		w->MoveTo(pt);
}
// -------------------------------------------------------------------------------------------------- //

// ResourceUsageWindow - Constructor
ResourceUsageWindow::ResourceUsageWindow(BRect frame) : BWindow (frame, "ResourceUsageWindow", B_TITLED_WINDOW, B_NORMAL_WINDOW_FEEL , 0)
{
	InitWindow();
	CenterWindowOnScreen(this);
	Show();
}
// -------------------------------------------------------------------------------------------------- //

// ResourceUsageWindow - Destructor
ResourceUsageWindow::~ResourceUsageWindow()
{
	exit(0);
}
// -------------------------------------------------------------------------------------------------- //

// ResourceUsageWindow::InitWindow -- Initialization Commands here
void ResourceUsageWindow::InitWindow(void)
{
	BRect r;
	r = Bounds(); // the whole view
	
	/*lsvProjectFiles = new BListView(BRect(5,5,200,80), "ltvProjectFiles",
					  B_SINGLE_SELECTION_LIST, B_FOLLOW_LEFT | B_FOLLOW_TOP,
					  B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS);

	//BListView *list = new BListView(r, "City", B_MULTIPLE_SELECTION_LIST);
	lsvProjectFiles->AddItem(new BStringItem("Brisbane"));
	lsvProjectFiles->AddItem(new BStringItem("Sydney"));
	lsvProjectFiles->AddItem(new BStringItem("Melbourne"));
	lsvProjectFiles->AddItem(new BStringItem("Adelaide"));
	
//	lsiItem = new BListItem();
	
//	lsvProjectFiles.AddItem(lsiItem);*/
	
	// Create the Views
	AddChild(ptrResourceUsageView = new ResourceUsageView(r));
	//ptrResourceUsageView->AddChild(...);
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

