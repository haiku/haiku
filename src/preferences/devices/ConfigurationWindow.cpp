// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003-2004, Haiku
//
//  This software is part of the Haiku distribution and is covered 
//  by the Haiku license.
//
//
//  File:        ConfigurationWindow.cpp
//  Author:      Jérôme Duval
//  Description: Devices Preferences
//  Created :    April 22, 2004
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~


// Includes ------------------------------------------------------------------------------------------ //
#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <ByteOrder.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Screen.h>
#include <stdio.h>
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>
#include <TabView.h>
#include <TextView.h>
#include <Window.h>
#include <View.h>

#include "DevicesWindows.h"
#include "pcihdr.h"

#define CONFIGURATION_CHANGED 'cfch'
#define IRQ_CHANGED 'irch'
#define DMA_CHANGED 'irch'

class RangeConfItem : public BListItem
{
	public:
		RangeConfItem(uint32 lowAddress, uint32 highAddress);
		~RangeConfItem();
		virtual void DrawItem(BView *, BRect, bool = false);
		static int Compare(const void *firstArg, const void *secondArg);
	private:
		uint32 fLowAddress, fHighAddress;
};


RangeConfItem::RangeConfItem(uint32 lowAddress, uint32 highAddress)
	: BListItem(),
	fLowAddress(lowAddress), 
	fHighAddress(highAddress)
{
}

RangeConfItem::~RangeConfItem()
{
}

/***********************************************************
 * DrawItem
 ***********************************************************/
void 	
RangeConfItem::DrawItem(BView *owner, BRect itemRect, bool complete)
{
	rgb_color kBlack = { 0,0,0,0 };
	rgb_color kHighlight = { 156,154,156,0 };
		
	if (IsSelected() || complete) {
		rgb_color color;
		if (IsSelected())
			color = kHighlight;
		else
			color = owner->ViewColor();
		
		owner->SetHighColor(color);
		owner->SetLowColor(color);
		owner->FillRect(itemRect);
		owner->SetHighColor(kBlack);
		
	} else {
		owner->SetLowColor(owner->ViewColor());
	}
	
	BFont font = be_plain_font;
	font_height	finfo;
	font.GetHeight(&finfo);
	
	BPoint point = BPoint(itemRect.left + 4, itemRect.bottom - finfo.descent + 1);
	owner->SetFont(be_plain_font);
	owner->SetHighColor(kBlack);
	owner->MovePenTo(point);
	
	char string[20];
		
	if (fLowAddress >= 0) {	
		sprintf(string, "0x%04lx", fLowAddress);
		owner->DrawString(string);
	}
	
	point += BPoint(75, 0);
	owner->MovePenTo(point);
	owner->StrokeLine(point + BPoint(0,-3), point + BPoint(4,-3));
	
	point += BPoint(15, 0);
	owner->MovePenTo(point);
	owner->SetFont(be_plain_font);
		
	if (fHighAddress >= 0) {	
		sprintf(string, "0x%04lx", fHighAddress);
		owner->DrawString(string);
	}
}

int 
RangeConfItem::Compare(const void *firstArg, const void *secondArg)
{
	const RangeConfItem *item1 = *static_cast<const RangeConfItem * const *>(firstArg);
	const RangeConfItem *item2 = *static_cast<const RangeConfItem * const *>(secondArg);
	
	if (item1->fLowAddress < item2->fLowAddress) {
		return -1;
	} else if (item1->fLowAddress > item2->fLowAddress) {
		return 1;
	} else 
		return 0;

}


// -------------------------------------------------------------------------------------------------- //


// ConfigurationWindow - Constructor
ConfigurationWindow::ConfigurationWindow(BRect frame, DeviceItem *item) 
	: BWindow (frame, item->GetInfo()->GetCardName(), B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL , 
		B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE|B_NOT_RESIZABLE),
	fItem(item)	
{
	CenterWindowOnScreen(this);
	fItem->SetWindow(this);	
	InitWindow();
	
	Show();
}
// -------------------------------------------------------------------------------------------------- //


// ConfigurationWindow - Destructor
ConfigurationWindow::~ConfigurationWindow()
{
}
// -------------------------------------------------------------------------------------------------- //

// ConfigurationWindow::InitWindow -- Initialization Commands here
void 
ConfigurationWindow::InitWindow(void)
{
	DevicesInfo *devicesInfo = fItem->GetInfo();
	
	BRect bRect = Bounds();
	
	BBox *background = new BBox(bRect, "background", B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW, B_NO_BORDER);
	AddChild(background);
	
	BRect rtab = Bounds();
	BRect rlist = Bounds();
	rtab.top += 10;
    rlist.top += 11;
    rlist.left += 13;
    rlist.right -= 13;
    rlist.bottom -= 44;
    
    // Create the TabView and Tabs
	BTabView *tabView = new BTabView(rtab,"resource_usage_tabview");
	tabView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	rtab = tabView->Bounds();
	rtab.InsetBy(5,5);
	
	// Create the Views
	BBox *resources = new BBox(rlist, "resources", 
		B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
		
		
	BMenuItem *menuItem;
	fConfigurationMenu = new BPopUpMenu("<empty>");
	fConfigurationMenu->AddItem(menuItem = new BMenuItem("Current Configuration", 
		new BMessage(CONFIGURATION_CHANGED)));
	menuItem->SetMarked(true);
	fConfigurationMenu->AddItem(menuItem = new BMenuItem("Default Configuration", 
		new BMessage(CONFIGURATION_CHANGED)));
	menuItem->SetEnabled(false);
	fConfigurationMenu->AddItem(menuItem = new BMenuItem("Disable Device", 
		new BMessage(CONFIGURATION_CHANGED)));
	menuItem->SetEnabled(false);
	fConfigurationMenu->AddSeparatorItem();
	fConfigurationMenu->AddItem(menuItem = new BMenuItem("Possible Configuration 0", 
		new BMessage(CONFIGURATION_CHANGED)));
	
	BMenuField *configurationMenuField = new BMenuField(BRect(0,0,174 + (be_plain_font->Size() - 12) * 10,18), "configurationMenuField", 
		NULL, fConfigurationMenu, true);
	resources->SetLabel(configurationMenuField);
	
	fIRQMenu[0] = new BPopUpMenu("<empty>");
	fIRQMenu[1] = new BPopUpMenu("<empty>");
	fIRQMenu[2] = new BPopUpMenu("<empty>");
	fDMAMenu[0] = new BPopUpMenu("<empty>");
	fDMAMenu[1] = new BPopUpMenu("<empty>");
	fDMAMenu[2] = new BPopUpMenu("<empty>");
	
	BRect fieldRect;
	fieldRect.top = 25;
	fieldRect.left = 22;
	fieldRect.right = fieldRect.left + 72;
	fieldRect.bottom = fieldRect.top + 18;
	
	BMenuField *field;
	fIRQField[0] = field = new BMenuField(fieldRect, "IRQ1Field", "IRQs", fIRQMenu[0], true);
	field->SetEnabled(false);
	field->SetDivider(33);
	resources->AddChild(field);
	fieldRect.left = 25;
	fieldRect.OffsetBy(71, 0);
	fIRQField[1] = field = new BMenuField(fieldRect, "IRQ2Field", " and", fIRQMenu[1], true);
	field->SetEnabled(false);
	field->SetDivider(30);
	resources->AddChild(field);
	fieldRect.OffsetBy(71, 0);
	fIRQField[2] = field = new BMenuField(fieldRect, "IRQ3Field", " and", fIRQMenu[2], true);
	field->SetEnabled(false);
	field->SetDivider(30);
	resources->AddChild(field);
	
	fieldRect.OffsetBy(-142, 23);
	fieldRect.left = 14;
	fDMAField[0] = field = new BMenuField(fieldRect, "DMA1Field", "DMAs", fDMAMenu[0], true);
	field->SetEnabled(false);
	field->SetDivider(41);
	resources->AddChild(field);
	fieldRect.left = 25;
	fieldRect.OffsetBy(71, 0);
	fDMAField[1] = field = new BMenuField(fieldRect, "DMA2Field", " and", fDMAMenu[1], true);
	field->SetEnabled(false);
	field->SetDivider(30);
	resources->AddChild(field);
	fieldRect.OffsetBy(71, 0);
	fDMAField[2] = field = new BMenuField(fieldRect, "DMA3Field", " and", fDMAMenu[2], true);
	field->SetEnabled(false);
	field->SetDivider(30);
	resources->AddChild(field);
	
	fIRQMenu[0]->Superitem()->SetLabel("");
	fIRQMenu[1]->Superitem()->SetLabel("");
	fIRQMenu[2]->Superitem()->SetLabel("");
	fDMAMenu[0]->Superitem()->SetLabel("");
	fDMAMenu[1]->Superitem()->SetLabel("");
	fDMAMenu[2]->Superitem()->SetLabel("");
		
	BRect boxRect = resources->Bounds();
	boxRect.top = 74;
    boxRect.left += 13;
    boxRect.right -= 11;
    boxRect.bottom = boxRect.top + 141;
	BBox *ioPortBox = new BBox(boxRect, "ioPort", 
		B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	ioPortBox->SetLabel(new BStringView(BRect(0,0,150,15), "ioPortLabel", " IO Port Ranges "));
	resources->AddChild(ioPortBox);
	boxRect.OffsetBy(0, 149);
	boxRect.bottom = boxRect.top + 140;
	BBox *memoryBox = new BBox(boxRect, "memory", 
		B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	memoryBox->SetLabel(new BStringView(BRect(0,0,150,15), "memoryLabel", " Memory Ranges "));
	resources->AddChild(memoryBox);
	
	ioListView = new BListView(BRect(14,22,164,127), "ioListView");
	memoryListView = new BListView(BRect(14,22,164,127), "memoryListView");
	
	BScrollView *ioScrollView = new BScrollView("ioScrollView", ioListView, B_FOLLOW_LEFT|B_FOLLOW_TOP, 
		0, false, true, B_FANCY_BORDER);
	ioPortBox->AddChild(ioScrollView);
	BScrollView *memoryScrollView = new BScrollView("memoryScrollView", memoryListView, B_FOLLOW_LEFT|B_FOLLOW_TOP, 
		0, false, true, B_FANCY_BORDER);
	memoryBox->AddChild(memoryScrollView);
	
	
	struct device_configuration *current = devicesInfo->GetCurrent();
	resource_descriptor r;
	
	{
		int32 k = 0;
		int32 num = count_resource_descriptors_of_type(current, B_IRQ_RESOURCE);
		
		for (int32 i=0;i<num;i++) {
			get_nth_resource_descriptor_of_type(current, i, B_IRQ_RESOURCE,
					&r, sizeof(resource_descriptor));
			uint32 mask = r.d.m.mask;
			int16 j = 0;
			if (!mask)
				continue;
			
			for (;mask;mask>>=1,j++)
				if (mask & 1) {
					char value[50];
					sprintf(value, "%d", j);
					BMenuItem *item = new BMenuItem(value, new BMessage(IRQ_CHANGED));
					fIRQMenu[k]->AddItem(item);
					item->SetMarked(true);
					fIRQField[k]->SetEnabled(true);
				}
		}
	}
	
	{
		int32 k = 0;
		int32 num = count_resource_descriptors_of_type(current, B_DMA_RESOURCE);
		
		for (int32 i=0;i<num;i++) {
			get_nth_resource_descriptor_of_type(current, i, B_DMA_RESOURCE,
					&r, sizeof(resource_descriptor));
			uint32 mask = r.d.m.mask;
			int16 j = 0;
			if (!mask)
				continue;
			
			for (;mask;mask>>=1,j++)
				if (mask & 1) {
					char value[50];
					sprintf(value, "%d", j);
					BMenuItem *item = new BMenuItem(value, new BMessage(DMA_CHANGED));
					fDMAMenu[k]->AddItem(item);
					item->SetMarked(true);
					fDMAField[k++]->SetEnabled(true);
				}
		}
	}
	
	{
		int32 num = count_resource_descriptors_of_type(current, B_IO_PORT_RESOURCE);
		
		for (int32 i=0;i<num;i++) {
			get_nth_resource_descriptor_of_type(current, i, B_IO_PORT_RESOURCE,
					&r, sizeof(resource_descriptor));
				
				ioListView->AddItem(new RangeConfItem(r.d.r.minbase, 
					r.d.r.minbase + r.d.r.len - 1));
		}
	
		ioListView->SortItems(&RangeConfItem::Compare);
		
	}
	
	{
		int32 num = count_resource_descriptors_of_type(current, B_MEMORY_RESOURCE);
		
		for (int32 i=0;i<num;i++) {
			get_nth_resource_descriptor_of_type(current, i, B_MEMORY_RESOURCE,
					&r, sizeof(resource_descriptor));
				
				memoryListView->AddItem(new RangeConfItem(r.d.r.minbase, 
					r.d.r.minbase + r.d.r.len - 1));
		}
	
		memoryListView->SortItems(&RangeConfItem::Compare);
		
	}
	
	BBox *info = new BBox(rlist, "info", 
		B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW, B_NO_BORDER);
		
	bool isISAPnP = false;
	BString compatibleIDString;
	
	if (devicesInfo->GetInfo()->bus == B_ISA_BUS) {
			uint32 id = devicesInfo->GetInfo()->id[0];
			if ((id >> 24 == 'P') 
				&& (((id >> 16) & 0xff) == 'n') 
				&& (((id >> 8) & 0xff) == 'P')) {
				isISAPnP = true;
				
				char string[255];
				sprintf(string, "Compatible ID #%ld:", 
						devicesInfo->GetInfo()->id[3]);
				compatibleIDString = string;
			}
	} 
		
	BRect labelRect(1, 10, 132, 30);
	BStringView *label = new BStringView(labelRect, "Card Name", "Card Name:");
	label->SetAlignment(B_ALIGN_RIGHT);
	info->AddChild(label);
	labelRect.OffsetBy(0, 18);
	label = new BStringView(labelRect, "Device Name", "Device Name:");
	label->SetAlignment(B_ALIGN_RIGHT);
	info->AddChild(label);
	labelRect.OffsetBy(0, 18);
	label = new BStringView(labelRect, "Logical Device Name", "Logical Device Name:");
	label->SetAlignment(B_ALIGN_RIGHT);
	info->AddChild(label);
	labelRect.OffsetBy(0, 18);
	label = new BStringView(labelRect, "Bus Type", "Bus Type:");
	label->SetAlignment(B_ALIGN_RIGHT);
	info->AddChild(label);
	labelRect.OffsetBy(0, 18);
	label = new BStringView(labelRect, "Device Type", "Device Type:");
	label->SetAlignment(B_ALIGN_RIGHT);
	info->AddChild(label);
	labelRect.OffsetBy(0, 36);
	label = new BStringView(labelRect, "Vendor", "Vendor:");
	label->SetAlignment(B_ALIGN_RIGHT);
	info->AddChild(label);
	labelRect.OffsetBy(0, 18);
	label = new BStringView(labelRect, "Card ID", "Card ID:");
	label->SetAlignment(B_ALIGN_RIGHT);
	info->AddChild(label);
	if (isISAPnP) {
		labelRect.OffsetBy(0, 18);
		label = new BStringView(labelRect, "Logical Device ID", "Logical Device ID:");
		label->SetAlignment(B_ALIGN_RIGHT);
		info->AddChild(label);
		labelRect.OffsetBy(0, 18);
		label = new BStringView(labelRect, "Compatible ID#", compatibleIDString.String());
		label->SetAlignment(B_ALIGN_RIGHT);
		info->AddChild(label);
	}	
	labelRect.OffsetBy(0, 18);
	label = new BStringView(labelRect, "Current State", "Current State:");
	label->SetAlignment(B_ALIGN_RIGHT);
	info->AddChild(label);
	labelRect.OffsetBy(0, 18);
	label = new BStringView(labelRect, "Resource Conflicts", "Resource Conflicts:");
	label->SetAlignment(B_ALIGN_RIGHT);
	info->AddChild(label);
	
	BString cardName = "<Unknown>";
	
	BString logicalDeviceName = "<Unknown>";
	
	BString bus;
	switch (devicesInfo->GetInfo()->bus) {
		case B_ISA_BUS:						bus = "ISA device"; break;
		case B_PCI_BUS:						bus = "PCI device"; break;
		case B_PCMCIA_BUS:					bus = "PCMCIA device"; break;
		default:							bus = "<Unknown>"; break;
	}
	
	BString deviceType1, deviceType2;
	deviceType1 += devicesInfo->GetBaseString();
	deviceType1 += " (";
	deviceType1 += devicesInfo->GetSubTypeString();
	deviceType1 += ")";
	char typeString[255];
	sprintf(typeString, "Base: %x  Subtype: %x  Interface: %x", 
		devicesInfo->GetInfo()->devtype.base,
		devicesInfo->GetInfo()->devtype.subtype,
		devicesInfo->GetInfo()->devtype.interface);
	deviceType2 = typeString;
	BString vendor;
	BString vendorString = "<Unknown>";
	BString cardID = "";
	BString resourceConflicts = "None";
	BString logicalString;
	
	switch (devicesInfo->GetInfo()->bus) {
		case B_ISA_BUS:	
			{
				char string[255];
				if (isISAPnP) {
					uint32 id = devicesInfo->GetInfo()->id[1];
					sprintf(string, "%c%c%c  Product: %x%x%x  Revision: %d", 
						((uint8)(id >> 2) & 0x1f) + 'A' - 1,
						((uint8)(id & 0x3) << 3) + ((uint8)(id >> 13) & 0x7) + 'A' - 1,
						(uint8)(id >> 8) & 0x1f + 'A' - 1,
						(uint8)(id >> 20) & 0xf,
						(uint8)(id >> 16) & 0xf,
						(uint8)(id >> 28) & 0xf,
						(uint8)((id >> 24) & 0xf));
					vendor = string;
										
					sprintf(string, "%08lx", devicesInfo->GetInfo()->id[2]);
					cardID = string;
					
					id = devicesInfo->GetInfo()->id[3];
					sprintf(string, "%c%c%c  Product: %x%x%x  Revision: %d", 
						((uint8)(id >> 2) & 0x1f) + 'A' - 1,
						((uint8)(id & 0x3) << 3) + ((uint8)(id >> 13) & 0x7) + 'A' - 1,
						(uint8)(id >> 8) & 0x1f + 'A' - 1,
						(uint8)(id >> 20) & 0xf,
						(uint8)(id >> 16) & 0xf,
						(uint8)(id >> 28) & 0xf,
						(uint8)((id >> 24) & 0xf));
					logicalString = string;
					
					cardName = devicesInfo->GetCardName();
					
				} else {
					uint32 id = devicesInfo->GetInfo()->id[2];
					sprintf(string, "%c%c%c  Product: %x%x  Revision: %d", 
						((uint8)(id >> 2) & 0x1f) + 'A' - 1,
						((uint8)(id & 0x3) << 3) + ((uint8)(id >> 13) & 0x7) + 'A' - 1,
						(uint8)(id >> 8) & 0x1f + 'A' - 1,
						(uint8)(id >> 16) & 0xf,
						(uint8)(id >> 20) & 0xf,
						(uint8)((id >> 24) & 0xff));
					vendor = string;			
					
					cardID = "0";
				} 
				
			}
			break;
		case B_PCI_BUS:
			{
				for (uint32 i=0; i<PCI_VENTABLE_LEN; i++)
					if (PciVenTable[i].VenId == devicesInfo->GetInfo()->id[0] & 0xffff) {
						vendorString = PciVenTable[i].VenFull;
						break;
					}
			
				char string[255];
				sprintf(string, "%s (0x%04lx)", vendorString.String(), devicesInfo->GetInfo()->id[0] & 0xffff);
				vendor = string;
				sprintf(string, "%04lx", devicesInfo->GetInfo()->id[1] & 0xffff);
				cardID = string;
				
				for (uint32 i = 0; i < PCI_DEVTABLE_LEN; i++) {
					if (PciDevTable[i].VenId == devicesInfo->GetInfo()->id[0] & 0xffff &&
						PciDevTable[i].DevId == devicesInfo->GetInfo()->id[1] & 0xffff) {
							cardName = PciDevTable[i].ChipDesc;
							break;
					}
				}
			}						
			break;
		case B_PCMCIA_BUS:					vendor = "XX"; break;
		default:							vendor = "<Unknown>"; break;
	}
	
	BString state = "Enabled";
	if (!(devicesInfo->GetInfo()->flags & B_DEVICE_INFO_ENABLED))
		switch (devicesInfo->GetInfo()->config_status) {
			case B_DEV_RESOURCE_CONFLICT: 	state = "Disabled by System"; break;
			case B_DEV_DISABLED_BY_USER:	state = "Disabled by User"; break;
			default: 						state = "Disabled for Unknown Reason"; break;
		}
	
	labelRect = BRect(137, 10, 400, 30);
	label = new BStringView(labelRect, "Card Name", cardName.String());
	info->AddChild(label);
	labelRect.OffsetBy(0, 18);
	label = new BStringView(labelRect, "Device Name", devicesInfo->GetDeviceName());
	info->AddChild(label);
	labelRect.OffsetBy(0, 18);
	label = new BStringView(labelRect, "Logical Device Name", logicalDeviceName.String());
	info->AddChild(label);
	labelRect.OffsetBy(0, 18);
	label = new BStringView(labelRect, "Bus Type", bus.String());
	info->AddChild(label);
	labelRect.OffsetBy(0, 18);
	label = new BStringView(labelRect, "Device Type", deviceType1.String());
	info->AddChild(label);
	labelRect.OffsetBy(0, 18);
	label = new BStringView(labelRect, "Device Type", deviceType2.String());
	info->AddChild(label);
	labelRect.OffsetBy(0, 18);
	label = new BStringView(labelRect, "Vendor", vendor.String());
	info->AddChild(label);
	labelRect.OffsetBy(0, 18);
	label = new BStringView(labelRect, "Card ID", cardID.String());
	info->AddChild(label);
	if (isISAPnP) {
		labelRect.OffsetBy(0, 18);
		label = new BStringView(labelRect, "Logical Device ID", logicalString.String());
		info->AddChild(label);
		labelRect.OffsetBy(0, 18);
		label = new BStringView(labelRect, "Compatible ID", logicalString.String());
		info->AddChild(label);
	}
	labelRect.OffsetBy(0, 18);
	label = new BStringView(labelRect, "Current State", state.String());
	info->AddChild(label);
	labelRect.OffsetBy(0, 18);
	label = new BStringView(labelRect, "Resource Conflicts", resourceConflicts.String());
	info->AddChild(label);
	
							
	BTab *tab = new BTab();
	tabView->AddTab(resources, tab);
	tab->SetLabel("Resources");
	tab = new BTab();
	tabView->AddTab(info, tab);
	tab->SetLabel("Info");

	background->AddChild(tabView);
	
	ioListView->Select(0);
	memoryListView->Select(0);
}
// -------------------------------------------------------------------------------------------------- //


// ConfigurationWindow::MessageReceived -- receives messages
void 
ConfigurationWindow::MessageReceived (BMessage *message)
{
	switch(message->what)
	{
		default:
			BWindow::MessageReceived(message);
			break;
	}
}
// -------------------------------------------------------------------------------------------------- //

// ConfigurationWindow::QuitRequested -- Post a message to the app to quit
bool 
ConfigurationWindow::QuitRequested()
{
	fItem->SetWindow(NULL);
    return true;
}
// ---------------------------------------------------------------------------------------------------------- //





