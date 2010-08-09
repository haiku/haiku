/*
 * Copyright 2004-2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andre Alves Garzia, andre@andregarzia.com
 *		Stephan Assmuß
 *		Axel Dörfler
 *		Hugo Santos
 *		Philippe Saint-Pierre
 *		Vegard Wærp
 */


#include "EthernetSettingsView.h"
#include "Settings.h"

#include <Application.h>
#include <Alert.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <File.h>
#include <GridView.h>
#include <GroupView.h>
#include <LayoutItem.h>
#include <Locale.h>
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
#include <NetworkInterface.h>
#include <NetworkRoster.h>
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

#include <NetServer.h>

#include <support/Beep.h>

#include "AutoDeleter.h"


static const uint32 kMsgApply = 'aply';
static const uint32 kMsgRevert = 'rvrt';
static const uint32 kMsgClose = 'clse';
static const uint32 kMsgField = 'fild';
static const uint32 kMsgInfo = 'info';
static const uint32 kMsgStaticMode = 'stcm';
static const uint32 kMsgDHCPMode = 'dynm';
static const uint32 kMsgDisabledMode = 'disa';
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

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "EthernetSettingsView"

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
	modeMenu->AddItem(new BMenuItem(B_TRANSLATE("Static"),
		new BMessage(kMsgStaticMode)));
	modeMenu->AddItem(new BMenuItem(B_TRANSLATE("DHCP"),
		new BMessage(kMsgDHCPMode)));
	modeMenu->AddSeparatorItem();
	modeMenu->AddItem(new BMenuItem(B_TRANSLATE("Disabled"),
		new BMessage(kMsgDisabledMode)));

	fDeviceMenuField = new BMenuField(B_TRANSLATE("Adapter:"), deviceMenu);
	layout->AddItem(fDeviceMenuField->CreateLabelLayoutItem(), 0, 0);
	layout->AddItem(fDeviceMenuField->CreateMenuBarLayoutItem(), 1, 0);

	fTypeMenuField = new BMenuField(B_TRANSLATE("Mode:"), modeMenu);
	layout->AddItem(fTypeMenuField->CreateLabelLayoutItem(), 0, 1);
	layout->AddItem(fTypeMenuField->CreateMenuBarLayoutItem(), 1, 1);

	fIPTextControl = new BTextControl(B_TRANSLATE("IP address:"), "", NULL);
	SetupTextControl(fIPTextControl);

	BLayoutItem* layoutItem = fIPTextControl->CreateTextViewLayoutItem();
	layoutItem->SetExplicitMinSize(BSize(
		fIPTextControl->StringWidth("XXX.XXX.XXX.XXX") + inset,
		B_SIZE_UNSET));

	layout->AddItem(fIPTextControl->CreateLabelLayoutItem(), 0, 2);
	layout->AddItem(layoutItem, 1, 2);

	fNetMaskTextControl = new BTextControl(B_TRANSLATE("Netmask:"), "", NULL);
	SetupTextControl(fNetMaskTextControl);
	layout->AddItem(fNetMaskTextControl->CreateLabelLayoutItem(), 0, 3);
	layout->AddItem(fNetMaskTextControl->CreateTextViewLayoutItem(), 1, 3);

	fGatewayTextControl = new BTextControl(B_TRANSLATE("Gateway:"), "", NULL);
	SetupTextControl(fGatewayTextControl);
	layout->AddItem(fGatewayTextControl->CreateLabelLayoutItem(), 0, 4);
	layout->AddItem(fGatewayTextControl->CreateTextViewLayoutItem(), 1, 4);

	// TODO: Replace the DNS text controls by a BListView with add/remove
	// functionality and so on...
	fPrimaryDNSTextControl = new BTextControl(B_TRANSLATE("DNS #1:"), "",
		NULL);
	SetupTextControl(fPrimaryDNSTextControl);
	layout->AddItem(fPrimaryDNSTextControl->CreateLabelLayoutItem(), 0, 5);
	layout->AddItem(fPrimaryDNSTextControl->CreateTextViewLayoutItem(), 1, 5);

	fSecondaryDNSTextControl = new BTextControl(B_TRANSLATE("DNS #2:"), "",
		NULL);
	SetupTextControl(fSecondaryDNSTextControl);
	layout->AddItem(fSecondaryDNSTextControl->CreateLabelLayoutItem(), 0, 6);
	layout->AddItem(fSecondaryDNSTextControl->CreateTextViewLayoutItem(), 1, 6);

	fDomainTextControl = new BTextControl(B_TRANSLATE("Domain:"), "", NULL);
	SetupTextControl(fDomainTextControl);
	layout->AddItem(fDomainTextControl->CreateLabelLayoutItem(), 0, 7);
	layout->AddItem(fDomainTextControl->CreateTextViewLayoutItem(), 1, 7);

	fErrorMessage = new BStringView("error", "");
	fErrorMessage->SetAlignment(B_ALIGN_LEFT);
	fErrorMessage->SetFont(be_bold_font);
	fErrorMessage->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	layout->AddView(fErrorMessage, 1, 8);

	// button group (TODO: move to window, but take care of
	// enabling/disabling)
	BGroupView* buttonGroup = new BGroupView(B_HORIZONTAL);

	fRevertButton = new BButton(B_TRANSLATE("Revert"),
		new BMessage(kMsgRevert));
	fRevertButton->SetEnabled(false);
	buttonGroup->GroupLayout()->AddView(fRevertButton);

	buttonGroup->GroupLayout()->AddItem(BSpaceLayoutItem::CreateGlue());

	fApplyButton = new BButton(B_TRANSLATE("Apply"), new BMessage(kMsgApply));
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

	fInterfaces.MakeEmpty();

	BNetworkRoster& roster = BNetworkRoster::Default();
	BNetworkInterface interface;
	uint32 cookie = 0;

	while (roster.GetNextInterface(&cookie, interface) == B_OK) {
		if (strncmp(interface.Name(), "loop", 4) && interface.Name()[0]) {
			fInterfaces.AddItem(new BString(interface.Name()));
			fSettings.AddItem(new Settings(interface.Name()));
		}
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
	fDomainTextControl->SetTarget(this);
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
	fDomainTextControl->SetText("");

	bool enableControls = false;
	fTypeMenuField->SetEnabled(settings != NULL);

	if (settings) {
		BMenuItem* item = fDeviceMenuField->Menu()->FindItem(
			settings->Name());
		if (item)
			item->SetMarked(true);

		fIPTextControl->SetText(settings->IP());
		fGatewayTextControl->SetText(settings->Gateway());
		fNetMaskTextControl->SetText(settings->Netmask());

		enableControls = false;
		
		if (settings->IsDisabled())
			item = fTypeMenuField->Menu()->FindItem(B_TRANSLATE("Disabled"));
		else if (settings->AutoConfigure() == true)
			item = fTypeMenuField->Menu()->FindItem(B_TRANSLATE("DHCP"));
		else {
			item = fTypeMenuField->Menu()->FindItem(B_TRANSLATE("Static"));
			enableControls = true;
		}
		if (item)
			item->SetMarked(true);

		if (settings->NameServers().CountItems() >= 2) {
			fSecondaryDNSTextControl->SetText(
				settings->NameServers().ItemAt(1)->String());
		}

		if (settings->NameServers().CountItems() >= 1) {
			fPrimaryDNSTextControl->SetText(
				settings->NameServers().ItemAt(0)->String());
		}
		fDomainTextControl->SetText(settings->Domain());
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
	fDomainTextControl->SetEnabled(enable);
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
		strcmp(fTypeMenuField->Menu()->FindMarked()->Label(),
			B_TRANSLATE("DHCP")) == 0);

	fCurrentSettings->SetDisabled(
		strcmp(fTypeMenuField->Menu()->FindMarked()->Label(),
			B_TRANSLATE("Disabled")) == 0);

	fCurrentSettings->NameServers().MakeEmpty();
	fCurrentSettings->NameServers().AddItem(new BString(
		fPrimaryDNSTextControl->Text()));
	fCurrentSettings->NameServers().AddItem(new BString(
		fSecondaryDNSTextControl->Text()));
	fCurrentSettings->SetDomain(fDomainTextControl->Text());

	fApplyButton->SetEnabled(false);
	fRevertButton->SetEnabled(true);
}


void
EthernetSettingsView::_SaveConfiguration()
{
	_ApplyControlsToConfiguration();
	_SaveDNSConfiguration();
	_SaveAdaptersConfiguration();
	if (fCurrentSettings->AutoConfigure())
		_TriggerAutoConfig(fCurrentSettings->Name());
}


void
EthernetSettingsView::_SaveDNSConfiguration()
{
	BPath path;
	if (find_directory(B_COMMON_SETTINGS_DIRECTORY, &path) != B_OK)
		return;

	path.Append("network/resolv.conf");

	BFile file(path.Path(), B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
	if (file.InitCheck() != B_OK) {
		fprintf(stderr, "failed to open %s for writing: %s\n", path.Path(),
			strerror(file.InitCheck()));
		return;
	}

	BString content("# Generated by Network Preflet\n");
	// loop over all adapters
	for (int i = 0; i < fSettings.CountItems(); i++) {
		Settings* settings = fSettings.ItemAt(i);
		for (int j = 0; j < settings->NameServers().CountItems(); j++) {
			if (settings->NameServers().ItemAt(j)->Length() > 0) {
				content << "nameserver\t"
					<< settings->NameServers().ItemAt(j)->String()
					<< "\n";
			}
		}
		if (strlen(settings->Domain()) > 0) {
			content << "domain\t"
				<< settings->Domain()
				<< "\n";
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
		Settings* settings = fSettings.ItemAt(i);
		
		if (settings->AutoConfigure())
			continue;

		if (fp == NULL) {
			fp = fopen(path.Path(), "w");
			if (fp == NULL) {
				fprintf(stderr, "failed to open file %s to write "
					"configuration: %s\n", path.Path(), strerror(errno));
				return;
			}
		}

		fprintf(fp, "interface %s {\n",
			settings->Name());
			
		if (settings->IsDisabled())
			fprintf(fp, "\tdisabled\ttrue\n");
		else {	
			fprintf(fp, "\taddress {\n");
			fprintf(fp, "\t\tfamily\tinet\n");
			fprintf(fp, "\t\taddress\t%s\n", settings->IP());
			fprintf(fp, "\t\tgateway\t%s\n", settings->Gateway());
			fprintf(fp, "\t\tmask\t%s\n", settings->Netmask());
			fprintf(fp, "\t}\n");
		}
		fprintf(fp, "}\n\n");
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
EthernetSettingsView::_TriggerAutoConfig(const char* device)
{
	BMessenger networkServer(kNetServerSignature);
	if (!networkServer.IsValid()) {
		(new BAlert("error", B_TRANSLATE("The net_server needs to run for "
			"the auto configuration!"), B_TRANSLATE("OK")))->Go();
		return B_ERROR;
	}

	BMessage message(kMsgConfigureInterface);
	message.AddString("device", device);
	BMessage address;
	address.AddString("family", "inet");
	address.AddBool("auto_config", true);
	message.AddMessage("address", &address);

	BMessage reply;
	status_t status = networkServer.SendMessage(&message, &reply);
	if (status != B_OK) {
		BString errorMessage(
			B_TRANSLATE("Sending auto-config message failed: "));
		errorMessage << strerror(status);
		(new BAlert("error", errorMessage.String(), B_TRANSLATE("OK")))->Go();
		return status;
	} else if (reply.FindInt32("status", &status) == B_OK
			&& status != B_OK) {
		BString errorMessage(B_TRANSLATE("Auto-configuring failed: "));
		errorMessage << strerror(status);
		(new BAlert("error", errorMessage.String(), "OK"))->Go();
		return status;
	}

	return B_OK;
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


bool
MatchPattern(const char* string, const char* pattern)
{
	regex_t compiled;
	bool result = regcomp(&compiled, pattern, REG_NOSUB | REG_EXTENDED) == 0
		&& regexec(&compiled, string, 0, NULL, 0) == 0;
	regfree(&compiled);

	return result;
}


bool
EthernetSettingsView::_ValidateControl(BTextControl* control)
{
	static const char* pattern = "^(25[0-5]|2[0-4][0-9]|[01][0-9]{2}|[0-9]"
		"{1,2})(\\.(25[0-5]|2[0-4][0-9]|[01][0-9]{2}|[0-9]{1,2})){3}$";

	if (control->IsEnabled() && !MatchPattern(control->Text(), pattern)) {
		control->MakeFocus();
		BString errorMessage;
		errorMessage << control->Label();
		errorMessage.RemoveLast(":");
		errorMessage << " is invalid";
		fErrorMessage->SetText(errorMessage.String());
		beep();
		return false;
	}
	return true;
}


void
EthernetSettingsView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgStaticMode:
		case kMsgDHCPMode:
		case kMsgDisabledMode:
			_EnableTextControls(message->what == kMsgStaticMode);
			fApplyButton->SetEnabled(true);
			fRevertButton->SetEnabled(true);
			break;
		case kMsgInfo: {
		 	const char* name;
			if (message->FindString("interface", &name) != B_OK)
				break;
			for (int32 i = 0; i < fSettings.CountItems(); i++) {
				Settings* settings = fSettings.ItemAt(i);
				if (strcmp(settings->Name(), name) == 0) {
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
			if (_ValidateControl(fIPTextControl)
				&& _ValidateControl(fNetMaskTextControl)
				&& (strlen(fGatewayTextControl->Text()) == 0
					|| _ValidateControl(fGatewayTextControl))
				&& (strlen(fPrimaryDNSTextControl->Text()) == 0
					|| _ValidateControl(fPrimaryDNSTextControl))
				&& (strlen(fSecondaryDNSTextControl->Text()) == 0
					|| _ValidateControl(fSecondaryDNSTextControl)))
				_SaveConfiguration();
			break;
		case kMsgChange:
			fErrorMessage->SetText("");
			fApplyButton->SetEnabled(true);
			break;
		default:
			BView::MessageReceived(message);
	}
}
