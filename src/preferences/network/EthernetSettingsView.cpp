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

#include "AutoDeleter.h"


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
EthernetSettingsView::_GatherInterfaces()
{
	// iterate over all interfaces and retrieve minimal status

	ifconf config;
	config.ifc_len = sizeof(config.ifc_value);
	if (ioctl(fSocket, SIOCGIFCOUNT, &config, sizeof(struct ifconf)) < 0)
		return;

	uint32 count = (uint32)config.ifc_value;
	if (count == 0)
		return;

	void* buffer = malloc(count * sizeof(struct ifreq));
	if (buffer == NULL)
		return;

	MemoryDeleter deleter(buffer);

	config.ifc_len = count * sizeof(struct ifreq);
	config.ifc_buf = buffer;
	if (ioctl(fSocket, SIOCGIFCONF, &config, sizeof(struct ifconf)) < 0)
		return;

	ifreq* interface = (ifreq*)buffer;

	fInterfaces.MakeEmpty();

	for (uint32 i = 0; i < count; i++) {
		if (strncmp(interface->ifr_name, "loop", 4) && interface->ifr_name[0]) {
			fInterfaces.AddItem(new BString(interface->ifr_name));
			fSettings.AddItem(new Settings(interface->ifr_name));
		}

		interface = (ifreq*)((addr_t)interface + IF_NAMESIZE
			+ interface->ifr_addr.sa_len);
	}
}


void
EthernetSettingsView::AttachedToWindow()
{
	fApplyButton->SetTarget(this);
	fRevertButton->SetTarget(this);
	fDeviceMenuField->Menu()->SetTargetForItems(this); 
	fTypeMenuField->Menu()->SetTargetForItems(this); 
	
	// display settigs of first adapter on startup
	_ShowConfiguration(fSettings.ItemAt(0));
}


void
EthernetSettingsView::DetachedFromWindow()
{
}


EthernetSettingsView::EthernetSettingsView(BRect frame)
	: BView(frame, "EthernetSettingsView", B_FOLLOW_ALL, B_WILL_DRAW)
	, fInterfaces()
	, fSettings()
	, fCurrentSettings(NULL)
{
	float inset = ceilf(be_plain_font->Size() * 0.8);
	frame.OffsetTo(inset, inset);
	frame.right = StringWidth("IP Address XXX.XXX.XXX.XXX") + 50;
	frame.bottom = frame.top + 15; // just a starting point
	BPoint spacing(0, inset);
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	fSocket = socket(AF_INET, SOCK_DGRAM, 0);
	_GatherInterfaces();
	
	BPopUpMenu* devmenu = new BPopUpMenu("devices");
	for (int32 i = 0; i < fInterfaces.CountItems(); i++) {
		BString& name = *fInterfaces.ItemAt(i);
		BString label = name;
		BMessage* info = new BMessage(kMsgInfo);
		info->AddString("interface", name.String());
		BMenuItem* item = new BMenuItem(label.String(), info);
		devmenu->AddItem(item);
	}

	BPopUpMenu* modemenu = new  BPopUpMenu("modes");
	modemenu->AddItem(new BMenuItem("Static", new BMessage(kMsgMode)));
	modemenu->AddItem(new BMenuItem("DHCP", new BMessage(kMsgMode)));
	//BMenuItem* offitem = new BMenuItem("Disconnected", NULL);
	//modemenu->AddItem(offitem);
	
	fDeviceMenuField = new BMenuField(frame, "networkcards", "Adapter:", devmenu);
	AddChild(fDeviceMenuField);
	fDeviceMenuField->ResizeToPreferred();
	
	fTypeMenuField = new BMenuField(frame, "type", "Mode:", modemenu);
	fTypeMenuField->MoveTo(fDeviceMenuField->Frame().LeftBottom() + spacing);
	AddChild(fTypeMenuField);
	fTypeMenuField->ResizeToPreferred();

	fIPTextControl = new BTextControl(frame, "ip", "IP Address:", "", NULL);
	fIPTextControl->MoveTo(fTypeMenuField->Frame().LeftBottom() + spacing);
	fIPTextControl->ResizeToPreferred();
	AddChild(fIPTextControl);

	fNetMaskTextControl = new BTextControl(frame, "mask", "Netmask:", "", NULL);
	fNetMaskTextControl->MoveTo(
		fIPTextControl->Frame().LeftBottom() + spacing);
	AddChild(fNetMaskTextControl);
	fNetMaskTextControl->ResizeToPreferred();

	fGatewayTextControl = new BTextControl(frame, "gateway", "Gateway:", "",
		NULL);
	fGatewayTextControl->MoveTo(
		fNetMaskTextControl->Frame().LeftBottom() + spacing);
	AddChild(fGatewayTextControl);
	fGatewayTextControl->ResizeToPreferred();
	
	fPrimaryDNSTextControl = new BTextControl(frame, "dns1", "DNS #1:", "",
		NULL);
	fPrimaryDNSTextControl->MoveTo(
		fGatewayTextControl->Frame().LeftBottom() + spacing);
	AddChild(fPrimaryDNSTextControl);
	fPrimaryDNSTextControl->ResizeToPreferred();
	
	fSecondaryDNSTextControl = new BTextControl(frame, "dns2", "DNS #2:", "",
		NULL);
	fSecondaryDNSTextControl->MoveTo(
		fPrimaryDNSTextControl->Frame().LeftBottom() + spacing);
	AddChild(fSecondaryDNSTextControl);
	fSecondaryDNSTextControl->ResizeToPreferred();

	fRevertButton = new BButton(frame, "revert", "Revert",
		new BMessage(kMsgRevert));
	fRevertButton->ResizeToPreferred();
	fRevertButton->MoveTo(
		fSecondaryDNSTextControl->Frame().LeftBottom() + spacing);
	AddChild(fRevertButton);

	fApplyButton = new BButton(frame, "apply", "Apply", new BMessage(kMsgApply));
	fApplyButton->ResizeToPreferred();
	fApplyButton->MoveTo(
		fSecondaryDNSTextControl->Frame().RightBottom() + spacing
		+ BPoint(-fApplyButton->Frame().Width(), 0));
	AddChild(fApplyButton);

	ResizeTo(frame.Width() + 2 * inset, fApplyButton->Frame().bottom + inset);

	// take care of label alignment
	float maxLabelWidth
		= fDeviceMenuField->StringWidth(fDeviceMenuField->Label());
	maxLabelWidth = max_c(maxLabelWidth,
		fTypeMenuField->StringWidth(fTypeMenuField->Label()));
	maxLabelWidth = max_c(maxLabelWidth,
		fIPTextControl->StringWidth(fIPTextControl->Label()));
	maxLabelWidth = max_c(maxLabelWidth,
		fNetMaskTextControl->StringWidth(fNetMaskTextControl->Label()));
	maxLabelWidth = max_c(maxLabelWidth,
		fGatewayTextControl->StringWidth(fGatewayTextControl->Label()));
	maxLabelWidth = max_c(maxLabelWidth,
		fPrimaryDNSTextControl->StringWidth(fPrimaryDNSTextControl->Label()));
	maxLabelWidth = max_c(maxLabelWidth,
		fSecondaryDNSTextControl->StringWidth(
			fSecondaryDNSTextControl->Label()));

	fDeviceMenuField->SetDivider(maxLabelWidth + 8);
	fTypeMenuField->SetDivider(maxLabelWidth + 8);

	fIPTextControl->SetDivider(maxLabelWidth + 8);
	fNetMaskTextControl->SetDivider(maxLabelWidth + 8);
	fGatewayTextControl->SetDivider(maxLabelWidth + 8);
	fPrimaryDNSTextControl->SetDivider(maxLabelWidth + 8);
	fSecondaryDNSTextControl->SetDivider(maxLabelWidth + 8);
}

EthernetSettingsView::~EthernetSettingsView()
{
	close(fSocket);
}

void
EthernetSettingsView::_ShowConfiguration(Settings* settings)
{
	fCurrentSettings = settings;

	// Clear the inputs.
	fIPTextControl->SetText("");
	fGatewayTextControl->SetText("");
	fNetMaskTextControl->SetText("");
	fPrimaryDNSTextControl->SetText("");
	fSecondaryDNSTextControl->SetText("");

	bool enableControls = false;
	fTypeMenuField->SetEnabled(settings != NULL);

	if (settings) {
		BMenuItem* item = fDeviceMenuField->Menu()->FindItem(
			settings->GetName());
		if (item)
			item->SetMarked(true);

		fIPTextControl->SetText(settings->GetIP());
		fGatewayTextControl->SetText(settings->GetGateway());
		fNetMaskTextControl->SetText(settings->GetNetmask());

		if (settings->GetAutoConfigure() == true)
			item = fTypeMenuField->Menu()->FindItem("DHCP");
		else
			item = fTypeMenuField->Menu()->FindItem("Static");
		if (item)
			item->SetMarked(true);

		enableControls = settings->GetAutoConfigure() == false;

		if (settings->fNameservers.CountItems() >= 2) {
			fSecondaryDNSTextControl->SetText(
				settings->fNameservers.ItemAt(1)->String());
		} 

		if (settings->fNameservers.CountItems() >= 1) {
			fPrimaryDNSTextControl->SetText(
					settings->fNameservers.ItemAt(0)->String());
		}
	}

	_EnableTextControls(enableControls);
}


void
EthernetSettingsView::_EnableTextControls(bool enable)
{
	fIPTextControl->SetEnabled(enable);
	fGatewayTextControl->SetEnabled(enable);
	fNetMaskTextControl->SetEnabled(enable);
	fPrimaryDNSTextControl->SetEnabled(enable);
	fSecondaryDNSTextControl->SetEnabled(enable);
}


void
EthernetSettingsView::_ApplyControlsToConfiguration()
{
	if (!fCurrentSettings)
		return;

	fCurrentSettings->SetIP(fIPTextControl->Text());
	fCurrentSettings->SetNetmask(fNetMaskTextControl->Text());
	fCurrentSettings->SetGateway(fGatewayTextControl->Text());

	fCurrentSettings->SetAutoConfigure(
		strcmp(fTypeMenuField->Menu()->FindMarked()->Label(), "DHCP") == 0);

	fCurrentSettings->fNameservers.MakeEmpty();
	fCurrentSettings->fNameservers.AddItem(new BString(
		fPrimaryDNSTextControl->Text()));
	fCurrentSettings->fNameservers.AddItem(new BString(
		fSecondaryDNSTextControl->Text()));
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
	FILE* fp = fopen("/etc/resolv.conf", "w");
	if (fp == NULL)
		return;

	fprintf(fp, "# Generated by Network Preflet\n");
	// loop over all adapters
	for (int i = 0; i < fSettings.CountItems(); i++) {
		Settings* settings = fSettings.ItemAt(i);
		for (int j = 0; j < settings->fNameservers.CountItems(); j++) {
			if (settings->fNameservers.ItemAt(j)->Length() > 0) {
				fprintf(fp, "nameserver\t%s\n", 
					settings->fNameservers.ItemAt(j)->String());
			}
		}
	}
	fclose(fp);	
}


void
EthernetSettingsView::_SaveAdaptersConfiguration()
{
	BPath path;
	status_t status = _GetPath("interfaces", path);
	if (status < B_OK)
		return;
		
	FILE* fp = NULL;
	// loop over all adapters. open the settings file only once,
	// append the settins of each non-autoconfiguring adapter
	for (int i = 0; i < fSettings.CountItems(); i++) {
		if (fSettings.ItemAt(i)->GetAutoConfigure())
			continue;

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
	} else {
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
		case kMsgMode:
			if (BMenuItem* item = fTypeMenuField->Menu()->FindMarked())
				_EnableTextControls(strcmp(item->Label(), "DHCP") != 0);
			break;
		case kMsgInfo: {
		 	const char* name;
			if (message->FindString("interface", &name) != B_OK)
				break;
			for (int32 i = 0; i < fSettings.CountItems(); i++) {
				Settings* settings = fSettings.ItemAt(i);
				if (strcmp(settings->GetName(), name) == 0) {
					_ShowConfiguration(settings);
					break;
				}
			}
			break;
		}
		case kMsgRevert:
			_ShowConfiguration(fCurrentSettings);
			break;
		case kMsgApply:
			_SaveConfiguration();
			break;
		default:
			BView::MessageReceived(message);
	}
}
