// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2004, Haiku
//
//  This software is part of the Haiku distribution and is covered 
//  by the Haiku license.
//
//
//  File:        DevicesInfo.cpp
//  Author:      Jérôme Duval
//  Description: Devices Preferences
//  Created :    April 15, 2004
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include <strings.h>
#include <GraphicsDefs.h>
#include <String.h>
#include <View.h>
#include "DevicesInfo.h"
#include <stdio.h>
#include "isapnpids.h"

const char *base_desc[] = {
	"Legacy",
	"Mass Storage Controller",
	"Network Controller",	// "NIC"
	"Display Controller",
	"Multimedia Device",
	"Memory Controller",
	"Bridge Device",
	"Communication Device",
	"Motherboard Device", // "Generic System Peripheral"
	"Input Device",
	"Docking Station",
	"Processor", // "CPU"
	"Serial Bus Controller"
};

struct subtype_descriptions {
	uchar base, subtype;
	char *name;
} subtype_desc[] = {
	{ 0, 0, "non-VGA" },
	{ 0, 0, "VGA" },
	{ 1, 0, "SCSI Controller" },
	{ 1, 1, "IDE Controller" }, // "IDE"
	{ 1, 2, "Floppy Controller" }, // "Floppy"
	{ 1, 3, "IPI" },
	{ 1, 4, "RAID" },
	{ 2, 0, "Ethernet" },
	{ 2, 1, "Token Ring" },
	{ 2, 2, "FDDI" },
	{ 2, 3, "ATM" },
	{ 3, 0, "VGA" }, // "VGA/8514"
	{ 3, 1, "XGA" },
	{ 4, 0, "Video" },
	{ 4, 1, "Sound" }, // was "Audio"
	{ 5, 0, "RAM" },
	{ 5, 1, "Flash Memory" }, // "Flash"
	{ 6, 0, "Host bridge" }, // "Host"
	{ 6, 1, "ISA bridge" }, // "ISA"
	{ 6, 2, "EISA" },
	{ 6, 3, "MCA" },
	{ 6, 4, "PCI-PCI bridge"}, // "PCI-PCI"
	{ 6, 5, "PCMCIA" },
	{ 6, 6, "NuBus" },
	{ 6, 7, "CardBus bridge" }, // "CardBus"
	{ 7, 0, "Serial Port" }, // "Serial"
	{ 7, 1, "Parallel Port" }, // "Parallel"
	{ 8, 0, "PIC" },
	{ 8, 1, "DMA" },
	{ 8, 2, "Timer" },
	{ 8, 3, "RTC" },
	{ 9, 0, "Keyboard" },
	{ 9, 1, "Digitizer" },
	{ 9, 2, "Mouse" },
	{10, 0, "Generic" },
	{11, 0, "386" },
	{11, 1, "486" },
	{11, 2, "Pentium" },
	{11,16, "Alpha" },
	{11,32, "PowerPC" },
	{11,48, "Coprocessor" },
	{12, 0, "FireWire" }, // was "IEEE 1394"
	{12, 1, "ACCESS" },
	{12, 2, "SSA" },
	{12, 3, "USB" },
	{12, 4, "Fibre Channel" },
	{255,255, NULL }
};


DevicesInfo::DevicesInfo(struct device_info *info, 
	struct device_configuration *current, 
	struct possible_device_configurations *possible)
	: fInfo(info),
	fCurrent(current),
	fPossible(possible) 
{
	struct subtype_descriptions *s = NULL;
		
	if (info->devtype.subtype != 0x80) {
		s = subtype_desc;
	
		while (s->name) {
			if ((info->devtype.base == s->base) && (info->devtype.subtype == s->subtype))
				break;
			s++;
		}
	}
	
	fDeviceName = strdup((info->devtype.base < 13) 
		? ( (s && s->name) ? s->name : base_desc[info->devtype.base]) 
		: "Unknown");
	
	fCardName = fDeviceName;
	if (fInfo->bus == B_ISA_BUS) {
			uint32 id = fInfo->id[0];
			if ((id >> 24 == 'P') 
				&& (((id >> 16) & 0xff) == 'n') 
				&& (((id >> 8) & 0xff) == 'P')) {
			id = fInfo->id[3];
			char string[255];
			sprintf(string, "%c%c%c%x%x%x%x", 
				((uint8)(id >> 2) & 0x1f) + 'A' - 1,
				((uint8)(id & 0x3) << 3) + ((uint8)(id >> 13) & 0x7) + 'A' - 1,
				(uint8)(id >> 8) & 0x1f + 'A' - 1,
				(uint8)(id >> 20) & 0xf,
				(uint8)(id >> 16) & 0xf,
				(uint8)(id >> 28) & 0xf,
				(uint8)((id >> 24) & 0xf));
			for (uint32 i=0; i<ISA_DEVTABLE_LEN; i++)
				if (stricmp(isapnp_devids[i].id, string)==0) {
					fCardName = isapnp_devids[i].devname;
				}
		}
	}
	
	fBaseString = strdup((info->devtype.base < 13) ? base_desc[info->devtype.base] : "Unknown");
	fSubTypeString = strdup((s && s->name) ? s->name : "Unknown");	
}


DevicesInfo::~DevicesInfo()
{
	delete fInfo;
	delete fCurrent;
	delete fPossible;
	delete fDeviceName;
	delete fBaseString;
	delete fSubTypeString;
}


DeviceItem::DeviceItem(DevicesInfo *info, const char* name)
	: BListItem(),
	fInfo(info),
	fWindow(NULL)
{
	fName = strdup(name);
}

DeviceItem::~DeviceItem()
{
	delete fName;
}

/***********************************************************
 * DrawItem
 ***********************************************************/
void 	
DeviceItem::DrawItem(BView *owner, BRect itemRect, bool complete)
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
	
	owner->SetHighColor(kBlack);
	owner->SetFont(&font);
	owner->MovePenTo(point);
	owner->DrawString(fName);
	
	if(fInfo) {
		point += BPoint(222, 0);
		BString string = "enabled";
		if (!(fInfo->GetInfo()->flags & B_DEVICE_INFO_ENABLED))
			switch (fInfo->GetInfo()->config_status) {
				case B_DEV_RESOURCE_CONFLICT: 	string = "disabled by system"; break;
				case B_DEV_DISABLED_BY_USER:	string = "disabled by user"; break;
				default: 						string = "disabled for unknown reason"; break;
			}
			
		owner->MovePenTo(point);
		owner->DrawString(string.String());
	}
}

