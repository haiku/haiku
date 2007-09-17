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
#include "settings.h"

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

#include <Directory.h>
#include <FindDirectory.h>
#include <fs_interface.h>
#include <Path.h>

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




bool
EthernetSettingsView::_PrepareRequest(struct ifreq& request, const char* name)
{
	//This function is used for talking direct to the stack. 
	//It´s used by _ShowConfiguration.
	
	if (strlen(name) > IF_NAMESIZE)
		return false;

	strcpy(request.ifr_name, name);
	return true;
}

void
EthernetSettingsView::_GatherInterfaces() {
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

	ifreq* interface = (ifreq *)buffer;

	fInterfaces.MakeEmpty();

	for (uint32 i = 0; i < count; i++) {
		if (strncmp(interface->ifr_name, "loop", 4) && interface->ifr_name[0])
		{
			fInterfaces.AddItem(new BString(interface->ifr_name));
			fSettings.AddItem(new Settings(interface->ifr_name));
			
		}

		interface = (ifreq *)((addr_t)interface + IF_NAMESIZE
			+ interface->ifr_addr.sa_len);
	}

	free(buffer);

}


void
EthernetSettingsView::AttachedToWindow()
{
	fApplyButton->SetTarget(this);
	fDeviceMenuField->Menu()->SetTargetForItems(this); 
	
	// Display first adapter by default.
	
	BMessage* info = new BMessage(kMsgInfo);
	info->AddString("interface", fSettings.ItemAt(0)->GetName());
	_ShowConfiguration(info);
	
}


void
EthernetSettingsView::DetachedFromWindow()
{
}


EthernetSettingsView::EthernetSettingsView(BRect rect)
: BView(rect, "EthernetSettingsView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	float defaultWidth = 190;
	float inset = ceilf(be_plain_font->Size() * 0.8);
	BRect frame(inset,inset, defaultWidth, 50);
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	fSettings.MakeEmpty();
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
		
		
	}

	
	
	BPopUpMenu* modemenu = new  BPopUpMenu("modes");
	BMenuItem* staticitem = new BMenuItem("Static", NULL);
	modemenu->AddItem(staticitem);
	BMenuItem* dhcpitem = new BMenuItem("DHCP", NULL);
	modemenu->AddItem(dhcpitem);
	//BMenuItem* offitem = new BMenuItem("Disconnected", NULL);
	//modemenu->AddItem(offitem);
	
	fDeviceMenuField = new BMenuField(frame, "networkcards", "Adapter:", devmenu);
	fDeviceMenuField->SetDivider(
		fDeviceMenuField->StringWidth(fDeviceMenuField->Label()) + 8);
	AddChild(fDeviceMenuField);
	fDeviceMenuField->ResizeToPreferred();

	
	fTypeMenuField = new BMenuField(frame, "type", "Mode:", modemenu);
	fTypeMenuField->SetDivider(
		fTypeMenuField->StringWidth(fTypeMenuField->Label()) + 8);
	fTypeMenuField->MoveTo(fDeviceMenuField->Frame().LeftBottom() + BPoint(0,10));
	AddChild(fTypeMenuField);
	fTypeMenuField->ResizeToPreferred();

	fIPTextControl = new BTextControl(frame, "ip", "IP Address:", "", NULL, 
	B_FOLLOW_TOP, B_WILL_DRAW );
	fIPTextControl->SetDivider(
		fIPTextControl->StringWidth(fIPTextControl->Label()) + 8);
	fIPTextControl->MoveTo(fTypeMenuField->Frame().LeftBottom() + BPoint(0,10));
	fIPTextControl->ResizeToPreferred();
	AddChild(fIPTextControl);

	fNetMaskTextControl = new BTextControl(frame, "mask", "Netmask:", "", NULL,
	B_FOLLOW_TOP, B_WILL_DRAW );
	fNetMaskTextControl->SetDivider(
		fNetMaskTextControl->StringWidth(fNetMaskTextControl->Label()) + 8);
	fNetMaskTextControl->MoveTo(
		fIPTextControl->Frame().LeftBottom() + BPoint(0,10));
	AddChild(fNetMaskTextControl);
	fNetMaskTextControl->ResizeToPreferred();

	fGatewayTextControl = new BTextControl(frame, "gateway", "Gateway:", "",
		NULL, B_FOLLOW_TOP, B_WILL_DRAW );
	fGatewayTextControl->SetDivider(
		fGatewayTextControl->StringWidth(fGatewayTextControl->Label()) + 8);
	fGatewayTextControl->MoveTo(
		fNetMaskTextControl->Frame().LeftBottom() + BPoint(0,10));
	AddChild(fGatewayTextControl);
	fGatewayTextControl->ResizeToPreferred();
	
	fPrimaryDNSTextControl = new BTextControl(
		frame, "dns1", "DNS #1:", "", NULL, B_FOLLOW_TOP, B_WILL_DRAW );
	fPrimaryDNSTextControl->SetDivider(
		fPrimaryDNSTextControl->StringWidth(fPrimaryDNSTextControl->Label()) + 8);
	fPrimaryDNSTextControl->MoveTo(
		fGatewayTextControl->Frame().LeftBottom() + BPoint(0,10));
	AddChild(fPrimaryDNSTextControl);
	fPrimaryDNSTextControl->ResizeToPreferred();
	
	fSecondaryDNSTextControl = new BTextControl(
		frame, "dns2", "DNS #2:", "", NULL, B_FOLLOW_TOP, B_WILL_DRAW );
	fSecondaryDNSTextControl->SetDivider(
		fSecondaryDNSTextControl->StringWidth(
			fSecondaryDNSTextControl->Label()) + 8);
	fSecondaryDNSTextControl->MoveTo(
		fPrimaryDNSTextControl->Frame().LeftBottom() + BPoint(0,10));
	AddChild(fSecondaryDNSTextControl);
	fSecondaryDNSTextControl->ResizeToPreferred();

	fApplyButton = new BButton(frame, "apply", "Apply", new BMessage(kMsgApply));
	fApplyButton->ResizeToPreferred();
	fApplyButton->MoveTo(
		fSecondaryDNSTextControl->Frame().LeftBottom() + BPoint(0,10));
	AddChild(fApplyButton);
	
	
}

EthernetSettingsView::~EthernetSettingsView()
{
	close(fSocket);
}

void
EthernetSettingsView::_ShowConfiguration(BMessage* message)
{
	

			
	// Clear the inputs.
	fIPTextControl->SetText("");
	fGatewayTextControl->SetText("");
	fNetMaskTextControl->SetText("");
	fPrimaryDNSTextControl->SetText("");
	fSecondaryDNSTextControl->SetText("");
	
	
 	const char* name;
	if (message->FindString("interface", &name) != B_OK)
		return;
	
	int i;
	for (i=0; i<fSettings.CountItems();i++) {
		if (strcmp(fSettings.ItemAt(i)->GetName(), name) == 0) {
			fDeviceMenuField->Menu()->FindItem(name)->SetMarked(true);
			fIPTextControl->SetText(fSettings.ItemAt(i)->GetIP());
			fGatewayTextControl->SetText(fSettings.ItemAt(i)->GetGateway());
			fNetMaskTextControl->SetText(fSettings.ItemAt(i)->GetNetmask());
			
			if (fSettings.ItemAt(i)->GetAutoConfigure() == true) {
				fTypeMenuField->Menu()->FindItem("DHCP")->SetMarked(true);
				fTypeMenuField->Menu()->FindItem("Static")->SetMarked(false);

			} else {
				fTypeMenuField->Menu()->FindItem("Static")->SetMarked(true);
				fTypeMenuField->Menu()->FindItem("DHCP")->SetMarked(false);

			}
			fTypeMenuField->Menu()->SetLabelFromMarked(true);
			fDeviceMenuField->Menu()->SetLabelFromMarked(true);

			
			if (fSettings.ItemAt(i)->fNameservers.CountItems() == 2) {
				fSecondaryDNSTextControl->SetText(
					fSettings.ItemAt(i)->fNameservers.ItemAt(1)->String());
			} 
			
			if (fSettings.ItemAt(i)->fNameservers.CountItems() >= 1) {
				fPrimaryDNSTextControl->SetText(
						fSettings.ItemAt(i)->fNameservers.ItemAt(0)->String());
			}
		} 
	}	
}

void
EthernetSettingsView::_ApplyControlsToConfiguration()
{
			
	int i;
	for (i = 0; i < fSettings.CountItems(); i++) {
		

		if (strcmp(fSettings.ItemAt(i)->
			GetName(), fDeviceMenuField->Menu()->FindMarked()->Label()) == 0) {
				fSettings.ItemAt(i)->
				SetIP(fIPTextControl->Text());
				fSettings.ItemAt(i)->
					SetNetmask(fNetMaskTextControl->Text());
					fSettings.ItemAt(i)->
					SetGateway(fGatewayTextControl->Text());
				if (strcmp(fTypeMenuField->Menu()->FindMarked()->Label(), "DHCP")
					== 0)
					fSettings.ItemAt(i)->SetAutoConfigure(true);
				else
					fSettings.ItemAt(i)->SetAutoConfigure(false);

					
				fSettings.ItemAt(i)->fNameservers.MakeEmpty();
				fSettings.ItemAt(i)->fNameservers.AddItem(new BString(
					fPrimaryDNSTextControl->Text()));
				fSettings.ItemAt(i)->fNameservers.AddItem(new BString(
					fSecondaryDNSTextControl->Text()));
					
		}
	}
} 


void
EthernetSettingsView::_SaveConfiguration()
{
	_ApplyControlsToConfiguration();
	_SaveDNSConfiguration();
	_SaveAdaptersConfiguration();
}

void
EthernetSettingsView::_SaveDNSConfiguration()
{
	FILE* fp;
	if ((fp = fopen("/etc/resolv.conf", "w")) != NULL) {
		fprintf(fp, "# Generated by Network Preflet\n");
		int i;
		for (i = 0; i < fSettings.CountItems(); i++) {
			// loop all the adapters.
			int j;
			for (j = 0; j < fSettings.ItemAt(i)->fNameservers.CountItems(); j++) {
				if (strcmp(
					fSettings.ItemAt(i)->fNameservers.ItemAt(j)->String(), "") != 0) {
						fprintf(fp, "nameserver\t%s\n", 
							fSettings.ItemAt(i)->fNameservers.ItemAt(j)->String());
					}
				}
			}
		fclose(fp);	
	}
	
}

void
EthernetSettingsView::_SaveAdaptersConfiguration()
{
	BPath path;
	status_t status = _GetPath("interfaces", path);
	if (status < B_OK)
		return;
		
	FILE* fp = NULL;
	bool allDHCP = true;
	// loop over all adapters. open the settings file only once,
	// append the settins of each non-autoconfiguring adapter
	for (int i = 0; i < fSettings.CountItems(); i++) {
		if (fSettings.ItemAt(i)->GetAutoConfigure())
			continue;
		
		allDHCP = false;
		if (fp == NULL) {
			fp = fopen(path.Path(), "w");
			if (fp == NULL) {
				fprintf(stderr, "failed to open file %s to write "
					"configuration: %s\n", path.Path(), strerror(errno));
				return;
			}
		}

		fprintf(fp, "interface %s {\n\t\taddress {\n",
			fSettings.ItemAt(i)->GetName());
		fprintf(fp, "\t\t\tfamily\tinet\n");
		fprintf(fp, "\t\t\taddress\t%s\n", 
			fSettings.ItemAt(i)->GetIP());
		fprintf(fp, "\t\t\tgateway\t%s\n", 
			fSettings.ItemAt(i)->GetGateway());
		fprintf(fp, "\t\t\tmask\t%s\n", 
			fSettings.ItemAt(i)->GetNetmask());
		fprintf(fp, "\t\t}\n}\n\n");
	}
	if (fp) {
		printf("%s saved.\n", path.Path());
		fclose(fp);
	}
	
	if (allDHCP) {
		// all configuration is DHCP, so delete interfaces file.
		remove(path.Path());
	}
}

status_t
EthernetSettingsView::_GetPath(const char* name, BPath& path)
{
	if (find_directory(B_COMMON_SETTINGS_DIRECTORY, &path, true) != B_OK)
		return B_ERROR;

	path.Append("network");
	create_directory(path.Path(), 0755);

	if (name != NULL)
		path.Append(name);
	return B_OK;
}

void
EthernetSettingsView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgInfo:
			_ShowConfiguration(message);
			break;
		case kMsgApply:
			_SaveConfiguration();
			break;
		default:
			BView::MessageReceived(message);
	}
}
