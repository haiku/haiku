// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003-2004, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        DevicesWindow.cpp
//  Author:      Sikosis, Jérôme Duval
//  Description: Devices Preferences
//  Created :    March 04, 2003
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

// Includes -------------------------------------------------------------------------------------------------- //
#include <Alert.h>
#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <ClassInfo.h>
#include <File.h>
#include <FindDirectory.h>
#include <MenuBar.h>
#include <Menu.h>
#include <MenuItem.h>
#include <OutlineListView.h>
#include <Path.h>
#include <Screen.h>
#include <ScrollView.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <String.h>
#include <StringView.h>

#include "DevicesWindows.h"
#include "DevicesInfo.h"

const uint32 MENU_FILE_ABOUT_DEVICES = 'mfad';
const uint32 MENU_FILE_QUIT = 'mfqt';
const uint32 MENU_DEVICES_REMOVE_JUMPERED_DEVICE = 'mdrj';
const uint32 MENU_DEVICES_NEW_JUMPERED_DEVICE = 'mdnj';
const uint32 MENU_DEVICES_NEW_JUMPERED_DEVICE_CUSTOM = 'mdcj';
const uint32 MENU_DEVICES_NEW_JUMPERED_DEVICE_MODEM = 'mdcm';
const uint32 MENU_DEVICES_RESOURCE_USAGE = 'mdru';
const uint32 BTN_CONFIGURE = 'bcfg';
const uint32 SELECTION_CHANGED = 'slch';


// CenterWindowOnScreen -- Centers the BWindow to the Current Screen
void CenterWindowOnScreen(BWindow* w)
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
DevicesWindow::DevicesWindow(BRect frame) : BWindow (frame, "Devices", B_TITLED_WINDOW, B_NORMAL_WINDOW_FEEL , 0)
{
	InitWindow();
	CenterWindowOnScreen(this);
	
	// Load User Settings
    BPath path;
    find_directory(B_USER_SETTINGS_DIRECTORY,&path);
    path.Append("Devices_Settings",true);
    BFile file(path.Path(),B_READ_ONLY);
    BMessage msg;
    msg.Unflatten(&file);
    LoadSettings (&msg);
    
    status_t error;

	if ((error = init_cm_wrapper()) < 0) {
		printf("Error initializing configuration manager (%s)\n", strerror(error));
		exit(1);
	}
    
    InitDevices(B_ISA_BUS);
    InitDevices(B_PCI_BUS);
    
    for (int32 i=fList.CountItems()-1; i>=0; i--) {
		DevicesInfo *deviceInfo = (DevicesInfo *) fList.ItemAt(i);
		struct device_info *info = deviceInfo->GetInfo();
		BListItem *item = systemMenu;
		if (info->bus == B_ISA_BUS)
			item = systemMenu;
		else if (info->bus == B_PCI_BUS)
			item = pciMenu;
		
		outline->AddUnder(new DeviceItem(deviceInfo, deviceInfo->GetName()), item);
	}
	
	if (outline->CountItemsUnder(jumperedMenu, true)==0)
		outline->AddUnder(new BStringItem("<no devices>"), jumperedMenu);
	if (outline->CountItemsUnder(pciMenu, true)==0)
		outline->AddUnder(new BStringItem("<no devices>"), pciMenu);
	if (outline->CountItemsUnder(isaMenu, true)==0)
		outline->AddUnder(new BStringItem("<no devices>"), isaMenu);
	if (outline->CountItemsUnder(systemMenu, true)==0)
		outline->AddUnder(new BStringItem("<no devices>"), systemMenu);
	
	Show();
}
// ---------------------------------------------------------------------------------------------------------- //


// DevicesWindow - Destructor
DevicesWindow::~DevicesWindow()
{
	void *item;
	while ((item = fList.RemoveItem((int32)0)))
		delete item;
	uninit_cm_wrapper();
}
// ---------------------------------------------------------------------------------------------------------- //


// DevicesWindow::InitWindow -- Initialization Commands here
void DevicesWindow::InitWindow(void)
{	
	BRect r;
	r = Bounds(); // the whole view
	
	BMenuItem *mniRemoveJumperedDevice;
	
	// Add the menu bar
	menubar = new BMenuBar(r, "menu_bar");

	// Add File menu to menu bar
	BMenu *menu = new BMenu("File");
	menu->AddItem(new BMenuItem("About Devices" B_UTF8_ELLIPSIS, new BMessage(MENU_FILE_ABOUT_DEVICES), 0));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Quit", new BMessage(MENU_FILE_QUIT), 'Q'));
	menubar->AddItem(menu);
	
	menu = new BMenu("Devices");
	menubar->AddItem(menu);
	
	BMenu *jumperedDevicesMenu = new BMenu("New Jumpered Device");
	jumperedDevicesMenu->AddItem(new BMenuItem("Custom ...", new BMessage(MENU_DEVICES_NEW_JUMPERED_DEVICE_CUSTOM), 0));
	jumperedDevicesMenu->AddItem(new BMenuItem("Modem ...", new BMessage(MENU_DEVICES_NEW_JUMPERED_DEVICE_MODEM), 0));
	
	//DevicesMenu->AddItem(new BMenuItem("New Jumpered Device", new BMessage(MENU_DEVICES_NEW_JUMPERED_DEVICE), NULL));
	menu->AddItem(jumperedDevicesMenu);
	
	menu->AddItem(mniRemoveJumperedDevice = new BMenuItem("Remove Jumpered Device", new BMessage(MENU_DEVICES_REMOVE_JUMPERED_DEVICE), 'R'));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Resource Usage", new BMessage(MENU_DEVICES_RESOURCE_USAGE), 'U'));
	
	mniRemoveJumperedDevice->SetEnabled(false);
	
	// Create the StringViews
	stvDeviceName = new BStringView(BRect(16, 16, 150, 40), "DeviceName", "Device Name");
	stvCurrentState = new BStringView(BRect(248, 16, r.right-10, 40), "CurrentState", "Current State");
	
	float fCurrentHeight = 279;
	float fCurrentGap = 15;
	stvIRQ = new BStringView(BRect(r.left+22,r.top+fCurrentHeight,r.right-13,r.top+fCurrentHeight+20),"IRQ","IRQs:", B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	fCurrentHeight = fCurrentHeight + fCurrentGap;
	stvDMA = new BStringView(BRect(r.left+22,r.top+fCurrentHeight,r.right-13,r.top+fCurrentHeight+20),"DMA","DMAs:", B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	fCurrentHeight = fCurrentHeight + fCurrentGap;
	stvIORanges = new BStringView(BRect(r.left+22,r.top+fCurrentHeight,r.right-13,r.top+fCurrentHeight+20),"IORanges","IO Ranges:", B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	fCurrentHeight = fCurrentHeight + fCurrentGap;
	stvMemoryRanges = new BStringView(BRect(r.left+22,r.top+fCurrentHeight,r.right-13,r.top+fCurrentHeight+20),"MemoryRanges","Memory Ranges:", B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	fCurrentHeight = fCurrentHeight + fCurrentGap;
	
	// Create the OutlineView
	BRect outlinerect(r.left+14,r.top+45,r.right-28,r.top+262);
	BScrollView      *outlinesv;

	outline = new BOutlineListView(outlinerect,"devices_list", B_SINGLE_SELECTION_LIST, B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP_BOTTOM);
	outline->AddItem(systemMenu = new DeviceItem(NULL, "System Devices"));
	outline->AddItem(isaMenu = new DeviceItem(NULL, "ISA/Plug and Play Devices"));
	outline->AddItem(pciMenu = new DeviceItem(NULL, "PCI Devices"));
	outline->AddItem(jumperedMenu = new DeviceItem(NULL, "Jumpered Devices"));
	
	outline->SetSelectionMessage(new BMessage(SELECTION_CHANGED));
	outline->SetInvocationMessage(new BMessage(BTN_CONFIGURE));
	
	// Add ScrollView to Devices Window
	outlinesv = new BScrollView("scroll_devices", outline, B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP_BOTTOM, 0, false, true, B_FANCY_BORDER);
	
	// Setup the OutlineView 
	outline->AllAttached();
	outlinesv->SetBorderHighlighted(true);
	
	// Back and Fore ground Colours
	outline->SetHighColor(0,0,0,0);
	outline->SetLowColor(255,255,255,0);
	
	// Font Settings
	BFont OutlineFont;
	OutlineFont.SetSpacing(B_CHAR_SPACING);
	OutlineFont.SetSize(12);
	outline->SetFont(&OutlineFont,B_FONT_SHEAR | B_FONT_SPACING);
	
	// Create BBox (or Frame)
	BBox  *BottomFrame;
	BRect BottomFrameRect(r.left+11,r.bottom-124,r.right-12,r.bottom-12);
	BottomFrame = new BBox(BottomFrameRect, "BottomFrame", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW, B_FANCY_BORDER);
	BRect bottomFrameBounds = BottomFrame->Bounds();
	
	// Create BButton - Configure
	BRect ConfigureButtonRect(bottomFrameBounds.left+290, bottomFrameBounds.bottom-37,
			bottomFrameBounds.right-13, bottomFrameBounds.bottom-13);
	btnConfigure = new BButton(ConfigureButtonRect,"btnConfigure","Configure", new BMessage(BTN_CONFIGURE), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW | B_NAVIGABLE);
	btnConfigure->MakeDefault(true);
	btnConfigure->SetEnabled(false);
	
	// Create a basic View
	BBox *background = new BBox(Bounds(), "background", B_FOLLOW_ALL_SIDES, B_WILL_DRAW, B_PLAIN_BORDER);
	AddChild(background);
	
	// Add Child(ren)	- in reverse order to avoid one control overwriting another
	background->AddChild(BottomFrame);
	BottomFrame->AddChild(btnConfigure);
	background->AddChild(outlinesv);
	background->AddChild(stvMemoryRanges);
	background->AddChild(stvIORanges);
	background->AddChild(stvDMA);
	background->AddChild(stvIRQ);
	background->AddChild(stvDeviceName);
	background->AddChild(stvCurrentState);
	background->AddChild(menubar);
	
	// Set Window Limits
	float xsize = 396 + (be_plain_font->Size()-10)*40;
	if (xsize > 443)
		xsize = 443;
	SetSizeLimits(xsize,xsize,400,BScreen().Frame().Height());
}
// ---------------------------------------------------------------------------------------------------------- //


// DevicesWindow::QuitRequested -- Post a message to the app to quit
bool DevicesWindow::QuitRequested()
{
	SaveSettings();
	be_app->PostMessage(B_QUIT_REQUESTED);
    return true;
}
// ---------------------------------------------------------------------------------------------------------- //


// DevicesWindow::LoadSettings -- Loads your current settings
void DevicesWindow::LoadSettings(BMessage *msg)
{
	BRect frame;

	if (B_OK == msg->FindRect("windowframe",&frame)) {
		MoveTo(frame.left,frame.top);
		ResizeTo(frame.right-frame.left,frame.bottom-frame.top);
	}
}
// -------------------------------------------------------------------------------------------------------------- //


// DevicesWindow::SaveSettings -- Saves the Users settings
void DevicesWindow::SaveSettings(void)
{
	BMessage msg;
	msg.AddRect("windowframe",Frame());
	
    BPath path;
    status_t result = find_directory(B_USER_SETTINGS_DIRECTORY,&path);
    if (result == B_OK)
    {
	    path.Append("Devices_Settings",true);
    	BFile file(path.Path(),B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	    msg.Flatten(&file);
	}    
}
// ------------------------------------------------------------------------------- //


// DevicesWindow::MessageReceived -- receives messages
void 
DevicesWindow::MessageReceived (BMessage *message)
{
	switch(message->what)
	{
		case MENU_FILE_ABOUT_DEVICES:
			{
				(new BAlert("", "Devices\n\n\t'Just write a simple app to add a jumpered\n\tdevice to the system', he said.\n\n\tAnd many years later another he said\n\t'recreate it' ;)", "OK"))->Go();
			}	
			break;
		case MENU_FILE_QUIT:
			{
				QuitRequested();
			}	
			break;
		case MENU_DEVICES_NEW_JUMPERED_DEVICE_MODEM:
			{
				ptrModemWindow = new ModemWindow(BRect(0,0,200,194), BMessenger(this));
			}
			break;		
		case MENU_DEVICES_RESOURCE_USAGE:
			{
				ptrResourceUsageWindow = new ResourceUsageWindow(BRect(0,0,430,350), fList);
			}
			break;
		case SELECTION_CHANGED:
			{
				UpdateDeviceInfo();
			}
			break;
		case MODEM_ADDED:
			{
				
			}
			break;
		case BTN_CONFIGURE:
			{
				DeviceItem *devItem = dynamic_cast<DeviceItem *>(outline->ItemAt(outline->CurrentSelection()));
				if (devItem && devItem->GetInfo()) {
					if (devItem->GetWindow()!=NULL)
						devItem->GetWindow()->Activate();
					else
						new ConfigurationWindow(BRect(150,150,562,602), devItem);
				}
			}
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
	UpdateIfNeeded();
}
// ---------------------------------------------------------------------------------------------------------- //

void
DevicesWindow::InitDevices(bus_type bus)
{
	status_t size;
	uint64 cookie;
	struct device_info dummy, *info;
	struct device_configuration *current; 
	struct possible_device_configurations *possible;
	
	cookie = 0;
	while (get_next_device_info(bus, &cookie, &dummy, sizeof(dummy)) == B_OK) {
		info = (struct device_info *)malloc(dummy.size);
		if (!info) {
			printf("Out of core\n");
			break;
		}

		if (get_device_info_for(cookie, info, dummy.size) != B_OK) {
			printf("Error getting device information\n");
			break;
		}

		size = get_size_of_current_configuration_for(cookie);
		if (size < B_OK) {
			printf("Error getting the size of the current configuration\n");
			break;
		}
		current = (struct device_configuration *)malloc(size);
		if (!current) {
			printf("Out of core\n");
			break;
		}

		if (get_current_configuration_for(cookie, current, size) != B_OK) {
			printf("Error getting the current configuration\n");
			break;
		}

		size = get_size_of_possible_configurations_for(cookie);
		if (size < B_OK) {
			printf("Error getting the size of the possible configurations\n");
			break;
		}
		possible = (struct possible_device_configurations *)malloc(size);
		if (!possible) {
			printf("Out of core\n");
			break;
		}

		if (get_possible_configurations_for(cookie, possible, size) != B_OK) {
			printf("Error getting the possible configurations\n");
			break;
		}
		
		fList.AddItem(new DevicesInfo(info, current, possible));			
	}
	
}

void 
DevicesWindow::UpdateDeviceInfo()
{
	DeviceItem *item = dynamic_cast<DeviceItem *>(outline->ItemAt(outline->CurrentSelection()));
	if (item) {
		DevicesInfo *deviceInfo = item->GetInfo();
		if (deviceInfo) {
			struct device_configuration *current = deviceInfo->GetCurrent();
			resource_descriptor r;
			
			{
				BString text = "IRQs: ";
				bool first = true;
				int32 num = count_resource_descriptors_of_type(current, B_IRQ_RESOURCE);
				
				for (int32 i=0;i<num;i++) {
					get_nth_resource_descriptor_of_type(current, i, B_IRQ_RESOURCE,
							&r, sizeof(resource_descriptor));
					uint32 mask = r.d.m.mask;
					int i = 0;
					if (!mask)
						continue;
					
					for (;mask;mask>>=1,i++)
						if (mask & 1) {
							char value[50];
							sprintf(value, "%s%d", first ? "" : ", ", i);
							text += value;
							first = false;
						}
				}
				stvIRQ->SetText(text.String());
			}
			
			{
				BString text = "DMAs: ";
				bool first = true;
				int32 num = count_resource_descriptors_of_type(current, B_DMA_RESOURCE);
				
				for (int32 i=0;i<num;i++) {
					get_nth_resource_descriptor_of_type(current, i, B_DMA_RESOURCE,
							&r, sizeof(resource_descriptor));
					uint32 mask = r.d.m.mask;
					int i = 0;
					if (!mask)
						continue;
					
					for (;mask;mask>>=1,i++)
						if (mask & 1) {
							char value[50];
							sprintf(value, "%s%d", first ? "" : ", ", i);
							text += value;
							first = false;
						}
				}
				stvDMA->SetText(text.String());
			}
			
			{
				BString text = "IO Ranges: ";
				bool first = true;
				int32 num = count_resource_descriptors_of_type(current, B_IO_PORT_RESOURCE);
				
				for (int32 i=0;i<num;i++) {
					get_nth_resource_descriptor_of_type(current, i, B_IO_PORT_RESOURCE,
							&r, sizeof(resource_descriptor));
					char value[50];
					sprintf(value, "%s0x%04lx - 0x%04lx", first ? "" : ", ", r.d.r.minbase, r.d.r.minbase + r.d.r.len - 1);
					text += value;
					first = false;
				}
				if (text.Length()>70) {
					text.Truncate(70);
					text += "...";
				}
				stvIORanges->SetText(text.String());
			}
			
			{
				BString text = "Memory Ranges: ";
				bool first = true;
				int32 num = count_resource_descriptors_of_type(current, B_MEMORY_RESOURCE);
				
				for (int32 i=0;i<num;i++) {
					get_nth_resource_descriptor_of_type(current, i, B_MEMORY_RESOURCE,
							&r, sizeof(resource_descriptor));
					char value[50];
					sprintf(value, "%s0x%06lx - 0x%06lx", first ? "" : ", ", r.d.r.minbase, r.d.r.minbase + r.d.r.len - 1);
					text += value;
					first = false;
				}
				if (text.Length()>70) {
					text.Truncate(70);
					text += "...";
				}
				stvMemoryRanges->SetText(text.String());
			}
			
			btnConfigure->SetEnabled(true);
		} else {
			BlankDeviceInfoBox();
		}
	} else {
		BlankDeviceInfoBox();
	}
}

void
DevicesWindow::BlankDeviceInfoBox()
{
	stvIRQ->SetText("IRQs: ");
	stvDMA->SetText("DMAs: ");
	stvIORanges->SetText("IO Ranges: ");
	stvMemoryRanges->SetText("Memory Ranges: ");
	btnConfigure->SetEnabled(false);
}
