/*

Devices - DevicesWindow by Sikosis

(C)2003 OBOS

*/


// Includes -------------------------------------------------------------------------------------------------- //
#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <Button.h>
#include <MenuBar.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Path.h>
#include <Screen.h>
#include <ScrollView.h>
#include <Shape.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <StringView.h>
#include <Window.h>
#include <View.h>
#include "Devices.h"
#include "DevicesViews.h"
#include "DevicesWindows.h"

const uint32 MENU_FILE_ABOUT_DEVICES = 'mfad';
const uint32 MENU_FILE_QUIT = 'mfqt';
const uint32 MENU_DEVICES_REMOVE_JUMPERED_DEVICE = 'mdrj';
const uint32 MENU_DEVICES_NEW_JUMPERED_DEVICE = 'mdnj';
const uint32 MENU_DEVICES_RESOURCE_USAGE = 'mdru';


// CenterWindowOnScreen -- Centers the BWindow to the Current Screen
static void CenterWindowOnScreen(BWindow* w)
{
	BRect	screenFrame = (BScreen(B_MAIN_SCREEN_ID).Frame());
	BPoint 	pt;
	pt.x = screenFrame.Width()/2 - w->Bounds().Width()/2;
	pt.y = screenFrame.Height()/2 - w->Bounds().Height()/2;

	if (screenFrame.Contains(pt))
		w->MoveTo(pt);
}
// ---------------------------------------------------------------------------------------------------------- //


// DevicesWindow - Constructor
DevicesWindow::DevicesWindow(BRect frame) : BWindow (frame, "OBOS Devices Alpha", B_TITLED_WINDOW, B_NORMAL_WINDOW_FEEL , 0)
{
	InitWindow();
	CenterWindowOnScreen(this);
	Show();
}


// DevicesWindow - Destructor
DevicesWindow::~DevicesWindow()
{
	exit(0);
}


// DevicesWindow::InitWindow -- Initialization Commands here
void DevicesWindow::InitWindow(void)
{	
	int LeftMargin = 16;
	int RightMargin = 246;
	BRect r;
	r = Bounds(); // the whole view
	
	BMenu *FileMenu;
	BMenu *DevicesMenu;
	
	// Add the menu bar
	menubar = new BMenuBar(r, "menu_bar");

	// Add File menu to menu bar
	FileMenu = new BMenu("File");
	FileMenu->AddItem(new BMenuItem("About Devices" B_UTF8_ELLIPSIS, new BMessage(MENU_FILE_ABOUT_DEVICES), NULL));
	FileMenu->AddSeparatorItem();
	FileMenu->AddItem(new BMenuItem("Quit", new BMessage(MENU_FILE_QUIT), 'Q'));
	
	DevicesMenu = new BMenu("Devices");
	DevicesMenu->AddItem(new BMenuItem("New Jumpered Device", new BMessage(MENU_DEVICES_NEW_JUMPERED_DEVICE), NULL));
	DevicesMenu->AddItem(new BMenuItem("Remove Jumpered Device", new BMessage(MENU_DEVICES_REMOVE_JUMPERED_DEVICE), 'R'));
	DevicesMenu->AddSeparatorItem();
	DevicesMenu->AddItem(new BMenuItem("Resource Usage", new BMessage(MENU_DEVICES_RESOURCE_USAGE), 'U'));
	
	menubar->AddItem(FileMenu);
	menubar->AddItem(DevicesMenu);
	
	stvDeviceName = new BStringView(BRect(LeftMargin, 16, 150, 40), "DeviceName", "Device Name");
	stvCurrentState = new BStringView(BRect(RightMargin, 16, r.right-10, 40), "CurrentState", "Current State");
	
	// Create a basic View
	AddChild(ptrDevicesView = new DevicesView(r));
	
	ptrDevicesView->AddChild(menubar);
	ptrDevicesView->AddChild(stvDeviceName);
	ptrDevicesView->AddChild(stvCurrentState);
}
// ---------------------------------------------------------------------------------------------------------- //


// DevicesWindow::FrameResized
void DevicesWindow::FrameResized (float width, float height)
{
	BRect r;
	r = Bounds();
	
	// enforce width but not height
}
// ---------------------------------------------------------------------------------------------------------- //


// DevicesWindow::QuitRequested -- Post a message to the app to quit
bool DevicesWindow::QuitRequested()
{
   be_app->PostMessage(B_QUIT_REQUESTED);
   return true;
}
// ---------------------------------------------------------------------------------------------------------- //


// DevicesWindow::MessageReceived -- receives messages
void DevicesWindow::MessageReceived (BMessage *message)
{
	switch(message->what)
	{
		case MENU_FILE_ABOUT_DEVICES:
			(new BAlert("", "Devices\n\n\t'Just write a simple app to add a jumpered\n\tdevice to the system', he said.\n\n\tAnd many years later another he said\n\t'recreate it' ;)", "OK"))->Go();
			break;
		case MENU_FILE_QUIT:
			QuitRequested();
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}
// ---------------------------------------------------------------------------------------------------------- //


