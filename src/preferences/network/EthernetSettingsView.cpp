/*
 * Copyright 2004-2007 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Andre Alves Garzia, andre@andregarzia.com
 * With code from:
 *		Axel Dorfler
 *		Hugo Santos
 */

#include "EthernetSettingsView.h"

#include <Application.h>
#include <Alert.h>
#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <File.h>
#include <StringView.h>
#include <String.h>
#include <Slider.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <TextControl.h>
#include <Screen.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include <SupportDefs.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/sockio.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>




bool EthernetSettingsView::_PrepareRequest(struct ifreq& request, const char* name)
{
	//This function is used for talking direct to the stack. 
	//It´s used by _ShowConfiguration.
	
	if (strlen(name) > IF_NAMESIZE)
		return false;

	strcpy(request.ifr_name, name);
	return true;
}

void EthernetSettingsView::_GatherInterfaces() {
	// iterate over all interfaces and retrieve minimal status

	ifconf config;
	config.ifc_len = sizeof(config.ifc_value);
	if (ioctl(fSocket, SIOCGIFCOUNT, &config, sizeof(struct ifconf)) < 0)
		return;

	uint32 count = (uint32)config.ifc_value;
	if (count == 0)
		return;

	void *buffer = malloc(count * sizeof(struct ifreq));
	if (buffer == NULL)
		return;

	config.ifc_len = count * sizeof(struct ifreq);
	config.ifc_buf = buffer;
	if (ioctl(fSocket, SIOCGIFCONF, &config, sizeof(struct ifconf)) < 0)
		return;

	ifreq *interface = (ifreq *)buffer;

	fInterfaces.MakeEmpty();

	for (uint32 i = 0; i < count; i++) {
		if (strncmp(interface->ifr_name, "loop", 4) && interface->ifr_name[0]) {
			fInterfaces.AddItem(new BString(interface->ifr_name));
			
		}

		interface = (ifreq *)((addr_t)interface + IF_NAMESIZE
			+ interface->ifr_addr.sa_len);
	}

	free(buffer);

}


void EthernetSettingsView::AttachedToWindow()
{
	fOKButton->SetTarget(this);
	fApplyButton->SetTarget(this);
}


void EthernetSettingsView::DetachedFromWindow()
{
	close(fSocket);
}


EthernetSettingsView::EthernetSettingsView(BRect rect) : BView(rect, "EthernetSettingsView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	float defaultWidth = 190;
	float inset = ceilf(be_plain_font->Size() * 0.8);
	BRect frame(inset,inset, defaultWidth, 50);
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	fSocket = socket(AF_INET, SOCK_DGRAM, 0);
	_GatherInterfaces();
	
	BPopUpMenu* devmenu = new  BPopUpMenu("devices");
	
	for (int32 i = 0; i < fInterfaces.CountItems(); i++) {
		BString& name = *fInterfaces.ItemAt(i);
		BString label = name;
		BMessage* info = new BMessage(kMsgInfo);
		info->AddString("interface", name.String());
		BMenuItem* item = new BMenuItem(label.String(), info);
		devmenu->AddItem(item);
		item->SetTarget(this);
		
	}

	
	
	BPopUpMenu* modemenu = new  BPopUpMenu("modes");
	BMenuItem* modeitem = new BMenuItem("dummy", NULL);
	modemenu->AddItem(modeitem);
	
	fDeviceMenuField = new BMenuField(frame, "networkcards", "Adapter:", devmenu);
	fDeviceMenuField->SetDivider(fDeviceMenuField->StringWidth(fDeviceMenuField->Label()) + 8);
	AddChild(fDeviceMenuField);
	fDeviceMenuField->ResizeToPreferred();

	
	fTypeMenuField = new BMenuField(frame, "type", "Mode:", modemenu);
	fTypeMenuField->SetDivider(fTypeMenuField->StringWidth(fTypeMenuField->Label()) + 8);
	fTypeMenuField->MoveTo(fDeviceMenuField->Frame().LeftBottom() + BPoint(0,10));
	AddChild(fTypeMenuField);
	fTypeMenuField->ResizeToPreferred();

	fIPTextControl = new BTextControl(frame, "ip", "IP Address:", "", NULL, B_FOLLOW_TOP, B_WILL_DRAW );
	fIPTextControl->SetDivider(fIPTextControl->StringWidth(fIPTextControl->Label()) + 8);
	fIPTextControl->MoveTo(fTypeMenuField->Frame().LeftBottom() + BPoint(0,10));
	fIPTextControl->ResizeToPreferred();
	AddChild(fIPTextControl);

	fNetMaskTextControl = new BTextControl(frame, "mask", "Netmask:", "", NULL, B_FOLLOW_TOP, B_WILL_DRAW );
	fNetMaskTextControl->SetDivider(fNetMaskTextControl->StringWidth(fNetMaskTextControl->Label()) + 8);
	fNetMaskTextControl->MoveTo(fIPTextControl->Frame().LeftBottom() + BPoint(0,10));
	AddChild(fNetMaskTextControl);
	fNetMaskTextControl->ResizeToPreferred();

	fGatewayTextControl = new BTextControl(frame, "gateway", "Gateway:", "", NULL, B_FOLLOW_TOP, B_WILL_DRAW );
	fGatewayTextControl->SetDivider(fGatewayTextControl->StringWidth(fGatewayTextControl->Label()) + 8);
	fGatewayTextControl->MoveTo(fNetMaskTextControl->Frame().LeftBottom() + BPoint(0,10));
	AddChild(fGatewayTextControl);
	fGatewayTextControl->ResizeToPreferred();
	
	fPrimaryDNSTextControl = new BTextControl(frame, "dns1", "DNS #1:", "", NULL, B_FOLLOW_TOP, B_WILL_DRAW );
	fPrimaryDNSTextControl->SetDivider(fPrimaryDNSTextControl->StringWidth(fPrimaryDNSTextControl->Label()) + 8);
	fPrimaryDNSTextControl->MoveTo(fGatewayTextControl->Frame().LeftBottom() + BPoint(0,10));
	AddChild(fPrimaryDNSTextControl);
	fPrimaryDNSTextControl->ResizeToPreferred();
	
	fSecondaryDNSTextControl = new BTextControl(frame, "dns2", "DNS #2:", "", NULL, B_FOLLOW_TOP, B_WILL_DRAW );
	fSecondaryDNSTextControl->SetDivider(fSecondaryDNSTextControl->StringWidth(fSecondaryDNSTextControl->Label()) + 8);
	fSecondaryDNSTextControl->MoveTo(fPrimaryDNSTextControl->Frame().LeftBottom() + BPoint(0,10));
	AddChild(fSecondaryDNSTextControl);
	fSecondaryDNSTextControl->ResizeToPreferred();

	fApplyButton = new BButton(frame, "apply", "Apply", new BMessage(kMsgApply));
	fApplyButton->ResizeToPreferred();
	fApplyButton->MoveTo(fSecondaryDNSTextControl->Frame().LeftBottom() + BPoint(0,10));
	AddChild(fApplyButton);
	
	fOKButton = new BButton(frame, "ok", "OK", new BMessage(kMsgOK));
	fOKButton->ResizeToPreferred();
	fOKButton->MoveTo(fApplyButton->Frame().RightTop() + BPoint(10,0));
	AddChild(fOKButton);
	
}

EthernetSettingsView::~EthernetSettingsView()
{
}

void EthernetSettingsView::_ShowConfiguration(BMessage* message)
{
	
 	const char *name;
	if (message->FindString("interface", &name) != B_OK)
		return;

	ifreq request;
	if (!_PrepareRequest(request, name))
		return;

	BString text = "dummy";
	char address[32];
	sockaddr_in* inetAddress = NULL;
	
	// Obtain IP.	
	if (ioctl(fSocket, SIOCGIFADDR, &request,
			sizeof(request)) < 0) {
		return;
	}

	inetAddress = (sockaddr_in*)&request.ifr_addr;
	if (inet_ntop(AF_INET, &inetAddress->sin_addr, address,
				sizeof(address)) == NULL) {
			return;
		}
		
	fIPTextControl->SetText(address);
	
	// Obtain netmask.
	if (ioctl(fSocket, SIOCGIFNETMASK, &request,
			sizeof(request)) < 0) {
		return;
	}
	
	inetAddress = (sockaddr_in*)&request.ifr_mask;
	if (inet_ntop(AF_INET, &inetAddress->sin_addr, address,
				sizeof(address)) == NULL) {
			return;
		}
		
	fNetMaskTextControl->SetText(address);
	
	// Obtain gateway
	
	char *gwAddress;
	ifconf config;
	config.ifc_len = sizeof(config.ifc_value);
	if (ioctl(fSocket, SIOCGRTSIZE, &config, sizeof(struct ifconf)) < 0)
		return;
		
	uint32 size = (uint32)config.ifc_value;
	if (size == 0)
		return;
	
	void *buffer = malloc(size);
	if (buffer == NULL)
		return;
	
	config.ifc_len = size;
	config.ifc_buf = buffer;
	
	if (ioctl(fSocket, SIOCGRTTABLE, &config, sizeof(struct ifconf)) < 0)
		return;
		
	ifreq *interface = (ifreq *)buffer;
	ifreq *end = (ifreq *)((uint8 *)buffer + size);
	
	
	while (interface < end) {
		route_entry& route = interface->ifr_route;
	
		
		if (route.flags & RTF_GATEWAY) {
			inetAddress = (sockaddr_in *)route.gateway;
					
			
			gwAddress = inet_ntoa(inetAddress->sin_addr);
			fGatewayTextControl->SetText(gwAddress);
		}
		
		int32 addressSize = 0;
		if (route.destination != NULL)
			addressSize += route.destination->sa_len;
		if (route.mask != NULL)
			addressSize += route.mask->sa_len;
		if (route.gateway != NULL)
			addressSize += route.gateway->sa_len;
			
		interface = (ifreq *)((addr_t)interface + IF_NAMESIZE + sizeof(route_entry) + addressSize);
	
	}
	
	free(buffer);	
	
	// Obtain DNS
	
	//resolv.conf is always actual.
	BFile file;
	BString* line;
	int i = 0;
	char* ch = "0";
	
	if (file.SetTo("/etc/resolv.conf", B_READ_ONLY) != B_OK)
		return;
		
	off_t length;
	
	file.GetSize(&length);
	
	/* 
	The idea here is to iterate over resolv.conf
	and look for the nameservers.
	
	Both netserver and DHCPClient will write to resolv.conf
	so we can trust that the information on the file is actual.
	
	I need help with this routine :$
	*/
	

	
}

void EthernetSettingsView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgInfo:
			_ShowConfiguration(message);
			break;
		default:
			BView::MessageReceived(message);
	}
}
