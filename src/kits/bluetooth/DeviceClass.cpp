/*
 * Copyright 2007-2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <bluetooth/DeviceClass.h>

#include <Catalog.h>
#include <Locale.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DeviceClass"


namespace Bluetooth {

void
DeviceClass::GetServiceClass(BString& serviceClass)
{
	static const char *services[] = { B_TRANSLATE_MARK("Positioning"), 
		B_TRANSLATE_MARK("Networking"), B_TRANSLATE_MARK("Rendering"), 
		B_TRANSLATE_MARK("Capturing"), B_TRANSLATE_MARK("Object transfer"),
		B_TRANSLATE_MARK("Audio"), B_TRANSLATE_MARK("Telephony"), 
		B_TRANSLATE_MARK("Information") };
		
	if (ServiceClass() != 0) {
		bool first = true;

		for (uint s = 0; s < (sizeof(services) / sizeof(*services)); s++) {
			if (ServiceClass() & (1 << s)) {
				if (first) {
					first = false;
					serviceClass << services[s];
				} else {
					serviceClass << ", " << services[s];
				}
					
			}
		}

	} else
		serviceClass << B_TRANSLATE("Unspecified");

}


void
DeviceClass::GetMajorDeviceClass(BString& majorClass)
{
	static const char *major_devices[] = { B_TRANSLATE_MARK("Miscellaneous"), 
		B_TRANSLATE_MARK("Computer"), B_TRANSLATE_MARK("Phone"), 
		B_TRANSLATE_MARK("LAN access"), B_TRANSLATE_MARK("Audio/Video"), 
		B_TRANSLATE_MARK("Peripheral"), B_TRANSLATE_MARK("Imaging"), 
		B_TRANSLATE_MARK("Uncategorized") };

	if (MajorDeviceClass() >= sizeof(major_devices) / sizeof(*major_devices))
		majorClass << B_TRANSLATE("Invalid device class!\n");
	else
		majorClass << major_devices[MajorDeviceClass()];

}


void
DeviceClass::GetMinorDeviceClass(BString& minorClass)
{
	uint major = MajorDeviceClass();
	uint minor = MinorDeviceClass();
	
	switch (major) {
		case 0:	/* misc */
			minorClass << " -";
			break;
		case 1:	/* computer */
			switch(minor) {
				case 0:
					minorClass << B_TRANSLATE("Uncategorized");
					break;	
				case 1:
					minorClass << B_TRANSLATE("Desktop workstation");
					break;	
				case 2:
					minorClass << B_TRANSLATE("Server");
					break;
				case 3:
					minorClass << B_TRANSLATE("Laptop");
					break;
				case 4:
					minorClass << B_TRANSLATE("Handheld");
					break;
				case 5:
					minorClass << B_TRANSLATE_COMMENT("Palm", 
						"A palm-held device");
					break;
				case 6:
					minorClass << B_TRANSLATE_COMMENT("Wearable",
						"A wearable computer");
					break;
				}
			break;
		case 2:	/* phone */
			switch(minor) {
				case 0:
					minorClass << B_TRANSLATE("Uncategorized");
					break;
				case 1:
					minorClass << B_TRANSLATE("Cellular");
					break;
				case 2:
					minorClass << B_TRANSLATE("Cordless");
					break;
				case 3:
					minorClass << B_TRANSLATE("Smart phone");
					break;
				case 4:
					minorClass << B_TRANSLATE("Wired modem or voice gateway");
					break;
				case 5:
					minorClass << B_TRANSLATE("Common ISDN access");
					break;
				case 6:
					minorClass << B_TRANSLATE("SIM card reader");
					break;
			}
			break;
		case 3:	/* lan access */
			if (minor == 0) {
				minorClass << B_TRANSLATE("Uncategorized");
				break;
			}
			switch(minor / 8) {
				case 0:
					minorClass << B_TRANSLATE("Fully available");
					break;
				case 1:
					minorClass << B_TRANSLATE("1-17% utilized");
					break;
				case 2:
					minorClass << B_TRANSLATE("17-33% utilized");
					break;
				case 3:
					minorClass << B_TRANSLATE("33-50% utilized");
					break;
				case 4:
					minorClass << B_TRANSLATE("50-67% utilized");
					break;
				case 5:
					minorClass << B_TRANSLATE("67-83% utilized");
					break;
				case 6:
					minorClass << B_TRANSLATE("83-99% utilized");
					break;
				case 7:
					minorClass << B_TRANSLATE("No service available");
					break;
			}
			break;
		case 4:	/* audio/video */
			switch(minor) {
				case 0:
					minorClass << B_TRANSLATE("Uncategorized");
					break;
				case 1:
					minorClass << B_TRANSLATE("Device conforms to the headset profile");
					break;
				case 2:
					minorClass << B_TRANSLATE("Hands-free");
					break;
					/* 3 is reserved */
				case 4:
					minorClass << B_TRANSLATE("Microphone");
					break;
				case 5:
					minorClass << B_TRANSLATE("Loudspeaker");
					break;
				case 6:
					minorClass << B_TRANSLATE("Headphones");
					break;
				case 7:
					minorClass << B_TRANSLATE("Portable audio");
					break;
				case 8:
					minorClass << B_TRANSLATE("Car audio");
					break;
				case 9:
					minorClass << B_TRANSLATE("Set-top box");
					break;
				case 10:
					minorClass << B_TRANSLATE("HiFi audio device");
					break;
				case 11:
					minorClass << B_TRANSLATE("VCR");
					break;
				case 12:
					minorClass << B_TRANSLATE("Video camera");
					break;
				case 13:
					minorClass << B_TRANSLATE("Camcorder");
					break;
				case 14:
					minorClass << B_TRANSLATE("Video monitor");
					break;
				case 15:
					minorClass << B_TRANSLATE("Video display and loudspeaker");
					break;
				case 16:
					minorClass << B_TRANSLATE("Video conferencing");
					break;
					/* 17 is reserved */
				case 18:
					minorClass << B_TRANSLATE("Gaming/Toy");
					break;
			}
			break;
		case 5:	/* peripheral */
		{		
			switch(minor & 48) {
				case 16:
					minorClass << B_TRANSLATE("Keyboard");
					if (minor & 15)
						minorClass << "/";
					break;
				case 32:
					minorClass << B_TRANSLATE("Pointing device");
					if (minor & 15)
						minorClass << "/";
					break;
				case 48:
					minorClass << B_TRANSLATE("Combo keyboard/pointing device");
					if (minor & 15)
						minorClass << "/";							
					break;
			}
			
			switch(minor & 15) {
				case 0:
					break;
				case 1:
					minorClass << B_TRANSLATE("Joystick");
					break;
				case 2:
					minorClass << B_TRANSLATE("Gamepad");
					break;
				case 3:
					minorClass << B_TRANSLATE("Remote control");
					break;
				case 4:
					minorClass << B_TRANSLATE("Sensing device");
					break;
				case 5:
					minorClass << B_TRANSLATE("Digitizer tablet");
					break;
				case 6:
					minorClass << B_TRANSLATE("Card reader");
					break;
				default:
					minorClass << B_TRANSLATE("(reserved)");
					break;
			}
			break;
		}
		case 6:	/* imaging */
			if (minor & 4)
				minorClass << B_TRANSLATE("Display");
			if (minor & 8)
				minorClass << B_TRANSLATE("Camera");
			if (minor & 16)
				minorClass << B_TRANSLATE("Scanner");
			if (minor & 32)
				minorClass << B_TRANSLATE("Printer");
			break;
		case 7: /* wearable */
			switch(minor) {
				case 1:
					minorClass << B_TRANSLATE("Wrist watch");
					break;
				case 2:
					minorClass << B_TRANSLATE_COMMENT("Pager",
					"A small radio device to receive short text messages");
					break;
				case 3:
					minorClass << B_TRANSLATE("Jacket");
					break;
				case 4:
					minorClass << B_TRANSLATE("Helmet");
					break;
				case 5:
					minorClass << B_TRANSLATE("Glasses");
					break;
			}
			break;
		case 8: /* toy */
			switch(minor) {
				case 1:
					minorClass << B_TRANSLATE("Robot");
					break;	
				case 2:
					minorClass << B_TRANSLATE("Vehicle");
					break;
				case 3:
					minorClass << B_TRANSLATE("Doll/Action figure");
					break;
				case 4:
					minorClass << B_TRANSLATE("Controller");
					break;
				case 5:
					minorClass << B_TRANSLATE("Game");
					break;
			}
			break;
		case 63:	/* uncategorised */
			minorClass << "";
			break;
		default:
			minorClass << B_TRANSLATE("Unknown (reserved) minor device class");
			break;
	}
}


void
DeviceClass::DumpDeviceClass(BString& string)
{
	string << B_TRANSLATE("Service classes: ");
	GetServiceClass(string);
	string << " | ";
	string << B_TRANSLATE("Major class: ");
	GetMajorDeviceClass(string);
	string << " | ";
	string << B_TRANSLATE("Minor class: ");
	GetMinorDeviceClass(string);
	string << ".";
}


void
DeviceClass::Draw(BView* view, const BPoint& point)
{
	rgb_color	kBlack = { 0,0,0,0 };
	rgb_color	kBlue = { 28,110,157,0 };
	rgb_color	kWhite = { 255,255,255,0 };


	view->SetHighColor(kBlue);
	view->FillRoundRect(BRect(point.x + IconInsets, point.y + IconInsets, 
		point.x + IconInsets + PixelsForIcon, point.y + IconInsets + PixelsForIcon), 5, 5);

	view->SetHighColor(kWhite);

	switch (MajorDeviceClass()) {

		case 2: // phone
			view->StrokeRoundRect(BRect(point.x + IconInsets + uint(PixelsForIcon/4),
				 point.y + IconInsets + 6,
				 point.x + IconInsets + uint(PixelsForIcon*3/4),
			 	 point.y + IconInsets + PixelsForIcon - 2), 2, 2);
			view->StrokeRect(BRect(point.x + IconInsets + uint(PixelsForIcon/4) + 4,
			 	 point.y + IconInsets + 10,
				 point.x + IconInsets + uint(PixelsForIcon*3/4) - 4,
			 	 point.y + IconInsets + uint(PixelsForIcon*3/4)));
			view->StrokeLine(BPoint(point.x + IconInsets + uint(PixelsForIcon/4) + 4,
				 point.y + IconInsets + PixelsForIcon - 6), 
				 BPoint(point.x + IconInsets + uint(PixelsForIcon*3/4) - 4,
				 point.y + IconInsets + PixelsForIcon - 6));
			view->StrokeLine(BPoint(point.x + IconInsets + uint(PixelsForIcon/4) + 4,
				 point.y + IconInsets + PixelsForIcon - 4), 
				 BPoint(point.x + IconInsets + uint(PixelsForIcon*3/4) - 4,
				 point.y + IconInsets + PixelsForIcon - 4));
			view->StrokeLine(BPoint(point.x + IconInsets + uint(PixelsForIcon/4) + 4,
				 point.y + IconInsets + 2), 
				 BPoint(point.x + IconInsets + uint(PixelsForIcon/4) + 4,
				 point.y + IconInsets + 6));
			break;
		case 3: // LAN
			view->StrokeLine(BPoint(point.x + IconInsets + uint(PixelsForIcon/4),
				 point.y + IconInsets + uint(PixelsForIcon*3/8)),
				BPoint(point.x + IconInsets + uint(PixelsForIcon*3/4),
				 point.y + IconInsets + uint(PixelsForIcon*3/8)));
			view->StrokeLine(BPoint(point.x + IconInsets + uint(PixelsForIcon*5/8),
				 point.y + IconInsets + uint(PixelsForIcon/8)));			
			view->StrokeLine(BPoint(point.x + IconInsets + uint(PixelsForIcon*3/4),
				 point.y + IconInsets + uint(PixelsForIcon*5/8)),
				BPoint(point.x + IconInsets + uint(PixelsForIcon/4),
				 point.y + IconInsets + uint(PixelsForIcon*5/8)));
			view->StrokeLine(BPoint(point.x + IconInsets + uint(PixelsForIcon*3/8),
				 point.y + IconInsets + uint(PixelsForIcon*7/8)));
			break;
		case 4: // audio/video
			view->StrokeRect(BRect(point.x + IconInsets + uint(PixelsForIcon/4),
				 point.y + IconInsets + uint(PixelsForIcon*3/8),
				 point.x + IconInsets + uint(PixelsForIcon*3/8),
			 	 point.y + IconInsets + uint(PixelsForIcon*5/8)));
			view->StrokeLine(BPoint(point.x + IconInsets + uint(PixelsForIcon*3/8),
				 point.y + IconInsets + uint(PixelsForIcon*3/8)),
				BPoint(point.x + IconInsets + uint(PixelsForIcon*3/4),
				 point.y + IconInsets + uint(PixelsForIcon/8)));
			view->StrokeLine(BPoint(point.x + IconInsets + uint(PixelsForIcon*3/4),
				 point.y + IconInsets + uint(PixelsForIcon*7/8)));
			view->StrokeLine(BPoint(point.x + IconInsets + uint(PixelsForIcon*3/8),
				 point.y + IconInsets + uint(PixelsForIcon*5/8)));
			break;			
		default: // Bluetooth Logo
			view->StrokeLine(BPoint(point.x + IconInsets + uint(PixelsForIcon/4),
				 point.y + IconInsets + uint(PixelsForIcon*3/4)),
				BPoint(point.x + IconInsets + uint(PixelsForIcon*3/4),
				 point.y + IconInsets + uint(PixelsForIcon/4)));
			view->StrokeLine(BPoint(point.x + IconInsets + uint(PixelsForIcon/2),
				 point.y + IconInsets +2));
			view->StrokeLine(BPoint(point.x + IconInsets + uint(PixelsForIcon/2),
				 point.y + IconInsets + PixelsForIcon - 2));
			view->StrokeLine(BPoint(point.x + IconInsets + uint(PixelsForIcon*3/4),
				 point.y + IconInsets + uint(PixelsForIcon*3/4)));
			view->StrokeLine(BPoint(point.x + IconInsets + uint(PixelsForIcon/4), 
				point.y + IconInsets + uint(PixelsForIcon/4)));
			break;
	}
	
	view->SetHighColor(kBlack);
}


}
