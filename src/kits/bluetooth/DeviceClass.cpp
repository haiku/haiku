/*
 * Copyright 2007-2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

//#include <bluetooth/DiscoveryAgent.h>
//#include <bluetooth/DiscoveryListener.h>
//#include <bluetooth/RemoteDevice.h>
#include <bluetooth/DeviceClass.h>
//#include <bluetooth/bluetooth_error.h>
//#include <bluetooth/HCI/btHCI_command.h>
//#include <bluetooth/HCI/btHCI_event.h>
//
//#include <bluetoothserver_p.h>
//#include <CommandManager.h>
//
//
//#include "KitSupport.h"

namespace Bluetooth {


void
DeviceClass::GetServiceClass(BString& serviceClass)
{
	static const char *services[] = { "Positioning", "Networking",
				"Rendering", "Capturing", "Object Transfer",
				"Audio", "Telephony", "Information" };
	
	serviceClass = "Service Classes: ";
	
	if (GetServiceClass() != 0) {
		
		for (uint s = 0; s < (sizeof(services) / sizeof(*services)); s++) {
			if (GetServiceClass() & (1 << s)) {
				serviceClass << services[s];
				if (s != 0)
					serviceClass << ", ";
			}
		}
		
	} else
		serviceClass <<  "Unspecified";

}


void
DeviceClass::GetMajorDeviceClass(BString& majorClass)
{
	static const char *major_devices[] = { "Miscellaneous", "Computer", "Phone",
				"LAN Access", "Audio/Video", "Peripheral", "Imaging", "Uncategorized" };

	majorClass << "Major Class: ";

	if (GetMajorDeviceClass() >= sizeof(major_devices) / sizeof(*major_devices))
		majorClass << "Invalid Device Class!\n";
	else
		majorClass << major_devices[GetMajorDeviceClass()];

}


void
DeviceClass::GetMinorDeviceClass(BString& minorClass)
{
	uint major = GetMajorDeviceClass();
	uint minor = GetMinorDeviceClass();
	
	minorClass << "Minor Class: ";
	
	switch (major) {
		case 0:	/* misc */
			minorClass << " -";
		break;
		case 1:	/* computer */
			switch(minor) {
				case 0:
					minorClass << "Uncategorized";
				break;	
				case 1:
					minorClass << "Desktop workstation";
				break;	
				case 2:
					minorClass << "Server";
				break;
				case 3:
					minorClass << "Laptop";
				break;
				case 4:
					minorClass << "Handheld";
				break;
				case 5:
					minorClass << "Palm";
				break;
				case 6:
					minorClass << "Wearable";
				break;
				}
				break;
		case 2:	/* phone */
			switch(minor) {
				case 0:
					minorClass << "Uncategorized";
				break;
				case 1:
					minorClass << "Cellular";
				break;
				case 2:
					minorClass << "Cordless";
				break;
				case 3:
					minorClass << "Smart phone";
				break;
				case 4:
					minorClass << "Wired modem or voice gateway";
				break;
				case 5:
					minorClass << "Common ISDN Access";
				break;
				case 6:
					minorClass << "Sim Card Reader";
				break;
			}
			break;
		case 3:	/* lan access */
			if (minor == 0) {
				minorClass << "Uncategorized";
				break;
			}
			switch(minor / 8) {
				case 0:
					minorClass << "Fully available";
				break;
				case 1:
					minorClass << "1-17% utilized";
				break;
				case 2:
					minorClass << "17-33% utilized";
				break;
				case 3:
					minorClass << "33-50% utilized";
				break;
				case 4:
					minorClass << "50-67% utilized";
				break;
				case 5:
					minorClass << "67-83% utilized";
				break;
				case 6:
					minorClass << "83-99% utilized";
				break;
				case 7:
					minorClass << "No service available";
				break;
			}
		break;
		case 4:	/* audio/video */
			switch(minor) {
				case 0:
					minorClass << "Uncategorized";
				break;
				case 1:
					minorClass << "Device conforms to the Headset profile";
				break;
				case 2:
					minorClass << "Hands-free";
				break;
					/* 3 is reserved */
				case 4:
					minorClass << "Microphone";
				break;
				case 5:
					minorClass << "Loudspeaker";
				break;
				case 6:
					minorClass << "Headphones";
				break;
				case 7:
					minorClass << "Portable Audio";
				break;
				case 8:
					minorClass << "Car Audio";
				break;
				case 9:
					minorClass << "Set-top box";
				break;
				case 10:
					minorClass << "HiFi Audio Device";
				break;
				case 11:
					minorClass << "VCR";
				break;
				case 12:
					minorClass << "Video Camera";
				break;
				case 13:
					minorClass << "Camcorder";
				break;
				case 14:
					minorClass << "Video Monitor";
				break;
				case 15:
					minorClass << "Video Display and Loudspeaker";
				break;
				case 16:
					minorClass << "Video Conferencing";
				break;
					/* 17 is reserved */
				case 18:
					minorClass << "Gaming/Toy";
				break;
			}
		break;
		case 5:	/* peripheral */ {		
			switch(minor & 48) {
				case 16:
					minorClass << "Keyboard";
					if (minor & 15)
						minorClass << "/";
				break;
				case 32:
					minorClass << "Pointing device";
					if (minor & 15)
						minorClass << "/";
				break;
				case 48:
					minorClass << "Combo keyboard/pointing device";
					if (minor & 15)
						minorClass << "/";							
				break;
			}
			
			switch(minor & 15) {
				case 0:
				break;
				case 1:
					minorClass << "Joystick";
				break;
				case 2:
					minorClass << "Gamepad";
				break;
				case 3:
					minorClass << "Remote control";
				break;
				case 4:
					minorClass << "Sensing device";
				break;
				case 5:
					minorClass << "Digitizer tablet";
				break;
				case 6:
					minorClass << "Card reader";
				break;
				default:
					minorClass << "(reserved)";
				break;
			}
		}
		case 6:	/* imaging */
			if (minor & 4)
				minorClass << "Display";
			if (minor & 8)
				minorClass << "Camera";
			if (minor & 16)
				minorClass << "Scanner";
			if (minor & 32)
				minorClass << "Printer";
		break;
		case 7: /* wearable */
			switch(minor) {
				case 1:
					minorClass << "Wrist Watch";
				break;
				case 2:
					minorClass << "Pager";
				break;
				case 3:
					minorClass << "Jacket";
				break;
				case 4:
					minorClass << "Helmet";
				break;
				case 5:
					minorClass << "Glasses";
				break;
			}
		break;
		case 8: /* toy */
			switch(minor) {
				case 1:
					minorClass << "Robot";
				break;	
				case 2:
					minorClass << "Vehicle";
				break;
				case 3:
					minorClass << "Doll / Action Figure";
				break;
				case 4:
					minorClass << "Controller";
				break;
				case 5:
					minorClass << "Game";
				break;
			}
		break;
		case 63:	/* uncategorised */
			minorClass << "";
		break;
		default:
			minorClass << "Unknown (reserved) minor device class";
		break;
	}
}


void
DeviceClass::DumpDeviceClass(BString& string)
{

	GetServiceClass(string);
	string << " | ";
	GetMajorDeviceClass(string);
	string << " | ";
	GetMinorDeviceClass(string);
	string << ".";
	
}

}
