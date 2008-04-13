/*
 * Copyright 2004-2008 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andre Alves Garzia, andre@andregarzia.com
 *		Stephan Assmuß
 *		Axel Dörfler
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
#include <GridView.h>
#include <GroupView.h>
#include <LayoutItem.h>
#include <Slider.h>
#include <SpaceLayoutItem.h>
#include <StringView.h>
#include <String.h>
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


static const uint32 kMsgApply = 'aply';
static const uint32 kMsgRevert = 'rvrt';
static const uint32 kMsgClose = 'clse';
static const uint32 kMsgField = 'fild';
static const uint32 kMsgInfo = 'info';
static const uint32 kMsgMode = 'mode';
static const uint32	kMsgChange = 'chng';


static void
SetupTextControl(BTextControl *control)
{
	// TODO: Disallow characters, etc.
	// Would be nice to have a real
	// formatted input control
	control->SetModificationMessage(new BMessage(kMsgChange));
}


//	#pragma mark -


EthernetSettingsView::EthernetSettingsView()
	: BView("EthernetSettingsView", 0, NULL),
	fCurrentSettings(NULL)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fSocket = socket(AF_INET, SOCK_DGRAM, 0);
	_GatherInterfaces();

	// build the GUI
	BGroupLayout* rootLayout = new BGroupLayout(B_VERTICAL);
	SetLayout(rootLayout);

	BGridView* controlsGroup = new BGridView();
	BGridLayout* layout = controlsGroup->GridLayout();

	// insets
	float inset = ceilf(be_plain_font->Size() * 0.7);
	rootLayout->SetInsets(inset, inset, inset, inset);
	rootLayout->SetSpacing(inset);
	layout->SetSpacing(inset, inset);

	BPopUpMenu* deviceMenu = new BPopUpMenu("devices");
	for (int32 i = 0; i < fInterfaces.CountItems(); i++) {
		BString& name = *fInterfaces.ItemAt(i);
		BString label = name;
		BMessage* info = new BMessage(kMsgInfo);
		info->AddString("interface", name.String());
		BMenuItem* item = new BMenuItem(label.String(), info);
		deviceMenu->AddItem(item);
	}

	BPopUpMenu* modeMenu = new  BPopUpMenu("modes");
	modeMenu->AddItem(new BMenuItem("Static", new BMessage(kMsgMode)));
	modeMenu->AddItem(new BMenuItem("DHCP", new BMessage(kMsgMode)));
	//modeMenu->AddSeparatorItem();
	//BMenuItem* offItem = new BMenuItem("Disabled", NULL);
	//modeMenu->AddItem(offItem);

	fDeviceMenuField = new BMenuField("Adapter:", deviceMenu);
	layout->AddItem(fDeviceMenuField->CreateLabelLayoutItem(), 0, 0);
	layout->AddItem(fDeviceMenuField->CreateMenuBarLayoutItem(), 1, 0);

	fTypeMenuField = new BMenuField("Mode:", modeMenu);
	layout->AddItem(fTypeMenuField->CreateLabelLayoutItem(), 0, 1);
	layout->AddItem(fTypeMenuField->CreateMenuBarLayoutItem(), 1, 1);

	fIPTextControl = new BTextControl("IP Address:", "", NULL);
	SetupTextControl(fIPTextControl);

	BLayoutItem* layoutItem = fIPTextControl->CreateTextViewLayoutItem();
	layoutItem->SetExplicitMinSize(BSize(
		fIPTextControl->StringWidth("XXX.XXX.XXX.XXX") + inset,
		B_SIZE_UNSET));

	layout->AddItem(fIPTextControl->CreateLabelLayoutItem(), 0, 2);
	layout->AddItem(layoutItem, 1, 2);

	fNetMaskTextControl = new BTextControl("Netmask:", "", NULL);
	SetupTextControl(fNetMaskTextControl);
	layout->AddItem(fNetMaskTextControl->CreateLabelLayoutItem(), 0, 3);
	layout->AddItem(fNetMaskTextControl->CreateTextViewLayoutItem(), 1, 3);

	fGatewayTextControl = new BTextControl("Gateway:", "", NULL);
	SetupTextControl(fGatewayTextControl);
	layout->AddItem(fGatewayTextControl->CreateLabelLayoutItem(), 0, 4);
	layout->AddItem(fGatewayTextControl->CreateTextViewLayoutItem(), 1, 4);

	// TODO: Replace the DNS text controls by a BListView with add/remove
	// functionality and so on...
	fPrimaryDNSTextControl = new BTextControl("DNS #1:", "", NULL);
	SetupTextControl(fPrimaryDNSTextControl);
	layout->AddItem(fPrimaryDNSTextControl->CreateLabelLayoutItem(), 0, 5);
	layout->AddItem(fPrimaryDNSTextControl->CreateTextViewLayoutItem(), 1, 5);

	fSecondaryDNSTextControl = new BTextControl("DNS #2:", "", NULL);
	SetupTextControl(fSecondaryDNSTextControl);
	layout->AddItem(fSecondaryDNSTextControl->CreateLabelLayoutItem(), 0, 6);
	layout->AddItem(fSecondaryDNSTextControl->CreateTextViewLayoutItem(), 1, 6);

	// button group (TODO: move to window, but take care of
	// enabling/disabling)
	BGroupView* buttonGroup = new BGroupView(B_HORIZONTAL);

	fRevertButton = new BButton("Revert", new BMessage(kMsgRevert));
	fRevertButton->SetEnabled(false);
	buttonGroup->GroupLayout()->AddView(fRevertButton);

	buttonGroup->GroupLayout()->AddItem(BSpaceLayoutItem::CreateGlue());

	fApplyButton = new BButton("Apply", new BMessage(kMsgApply));
	buttonGroup->GroupLayout()->AddView(fApplyButton);

	rootLayout->AddView(controlsGroup);
	rootLayout->AddView(buttonGroup);
}


EthernetSettingsView::~EthernetSettingsView()
{
	close(fSocket);
}


bool
EthernetSettingsView::_PrepareRequest(struct ifreq& request, const char* name)
{
	// This function is used for talking direct to the stack.
	// It's used by _ShowConfiguration.

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
	fIPTextControl->SetTarget(this);
	fNetMaskTextControl->SetTarget(this);
	fGatewayTextControl->SetTarget(this);
	fPrimaryDNSTextControl->SetTarget(this);
	fSecondaryDNSTextControl->SetTarget(this);
	fDeviceMenuField->Menu()->SetTargetForItems(this);
	fTypeMenuField->Menu()->SetTargetForItems(this);

	// display settigs of first adapter on startup
	_ShowConfiguration(fSettings.ItemAt(0));
}


void
EthernetSettingsView::DetachedFromWindow()
{
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

	fApplyButton->SetEnabled(false);
	fRevertButton->SetEnabled(true);
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
	BFile file("/etc/resolv.conf",
		B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
	if (file.InitCheck() < B_OK) {
		fprintf(stderr, "failed to open /etc/resolv.conf for writing: %s\n",
			strerror(file.InitCheck()));
		return;
	}

	BString content("# Generated by Network Preflet\n");
	// loop over all adapters
	for (int i = 0; i < fSettings.CountItems(); i++) {
		Settings* settings = fSettings.ItemAt(i);
		for (int j = 0; j < settings->fNameservers.CountItems(); j++) {
			if (settings->fNameservers.ItemAt(j)->Length() > 0) {
				content << "nameserver\t"
					<< settings->fNameservers.ItemAt(j)->String()
					<< "\n";
			}
		}
	}

	file.Write(content.String(), content.Length());
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
			fApplyButton->SetEnabled(true);
			fRevertButton->SetEnabled(true);
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
			fRevertButton->SetEnabled(false);
			break;
		case kMsgApply:
			_SaveConfiguration();
			break;
		case kMsgChange:
			fApplyButton->SetEnabled(true);
			break;
		default:
			BView::MessageReceived(message);
	}
}
