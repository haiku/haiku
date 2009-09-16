/*
 * Copyright 2004-2008 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andre Alves Garzia, andre@andregarzia.com
 *		Stephan Assmuß
 *		Axel Dörfler
 * 		Fredrik Modéen
 *		Hugo Santos
 *		Philippe Saint-Pierre
 */

#include "EthernetSettingsView.h"
#include "Setting.h"

#include <InterfaceKit.h>
#include <GridView.h>
#include <GroupView.h>
#include <LayoutItem.h>
#include <SpaceLayoutItem.h>

#include <File.h>
#include <Path.h>

#include <Directory.h>
#include <FindDirectory.h>

#include <errno.h>
#include <stdio.h>

#include <NetServer.h>

#include <support/Beep.h>

static const uint32 kMsgApply = 'aply';
static const uint32 kMsgRevert = 'rvrt';
static const uint32 kMsgClose = 'clse';
static const uint32 kMsgField = 'fild';
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
EthernetSettingsView::EthernetSettingsView(Setting* setting)
	: BView("EthernetSettingsView", 0, NULL),
	fCurrentSettings(setting)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

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

	BPopUpMenu* modeMenu = new  BPopUpMenu("modes");
	modeMenu->AddItem(new BMenuItem("Static", new BMessage(kMsgMode)));
	modeMenu->AddItem(new BMenuItem("DHCP", new BMessage(kMsgMode)));

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

	fErrorMessage = new BStringView("error", "");
	fErrorMessage->SetAlignment(B_ALIGN_LEFT);
	fErrorMessage->SetFont(be_bold_font);
	fErrorMessage->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	layout->AddView(fErrorMessage, 1, 7);

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
EthernetSettingsView::AttachedToWindow()
{
	fApplyButton->SetTarget(this);
	fRevertButton->SetTarget(this);
	fIPTextControl->SetTarget(this);
	fNetMaskTextControl->SetTarget(this);
	fGatewayTextControl->SetTarget(this);
	fPrimaryDNSTextControl->SetTarget(this);
	fSecondaryDNSTextControl->SetTarget(this);
	fTypeMenuField->Menu()->SetTargetForItems(this);

	// display settigs of first adapter on startup
	_ShowConfiguration(fCurrentSettings);
}


void
EthernetSettingsView::DetachedFromWindow()
{
}


void
EthernetSettingsView::_ShowConfiguration(Setting* setting)
{
	// Clear the inputs.
	fIPTextControl->SetText("");
	fGatewayTextControl->SetText("");
	fNetMaskTextControl->SetText("");
	fPrimaryDNSTextControl->SetText("");
	fSecondaryDNSTextControl->SetText("");

	bool enableControls = false;
	fTypeMenuField->SetEnabled(setting != NULL);

	if (setting) {
		fIPTextControl->SetText(setting->GetIP());
		fGatewayTextControl->SetText(setting->GetGateway());
		fNetMaskTextControl->SetText(setting->GetNetmask());

		BMenuItem* item;
		if (setting->GetAutoConfigure() == true)
			item = fTypeMenuField->Menu()->FindItem("DHCP");
		else
			item = fTypeMenuField->Menu()->FindItem("Static");
		if (item)
			item->SetMarked(true);

		enableControls = setting->GetAutoConfigure() == false;

		if (setting->fNameservers.CountItems() >= 2) {
			fSecondaryDNSTextControl->SetText(
				setting->fNameservers.ItemAt(1)->String());
		}

		if (setting->fNameservers.CountItems() >= 1) {
			fPrimaryDNSTextControl->SetText(
					setting->fNameservers.ItemAt(0)->String());
		}
	}

	//We don't want to enable loop
	if (strcmp(fCurrentSettings->GetName(), "loop") == 0) {
		_EnableTextControls(false);
		fTypeMenuField->SetEnabled(false);
	} else
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
	if (fCurrentSettings->GetAutoConfigure())
		_TriggerAutoConfig(fCurrentSettings->GetName());
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
	for (int j = 0; j < fCurrentSettings->fNameservers.CountItems(); j++) {
		if (fCurrentSettings->fNameservers.ItemAt(j)->Length() > 0) {
			content << "nameserver\t"
				<< fCurrentSettings->fNameservers.ItemAt(j)->String() << "\n";
		}
	}
	file.Write(content.String(), content.Length());
}


void
EthernetSettingsView::_SaveAdaptersConfiguration()
{
	BPath path;
	status_t status = _GetPath("interfaces", path);
	printf("Path = %s\n", path.Path());
	if (status < B_OK)
		return;

	FILE* fp = NULL;
	if (fCurrentSettings->GetAutoConfigure())
		return;

	if (fp == NULL) {
		fp = fopen(path.Path(), "w");
		if (fp == NULL) {
			fprintf(stderr, "failed to open file %s to write "
				"configuration: %s\n", path.Path(), strerror(errno));
			return;
		}
	}

	fprintf(fp, "interface %s {\n\t\taddress {\n",fCurrentSettings->GetName());
	fprintf(fp, "\t\t\tfamily\tinet\n");
	fprintf(fp, "\t\t\taddress\t%s\n",fCurrentSettings->GetIP());
	fprintf(fp, "\t\t\tgateway\t%s\n",fCurrentSettings->GetGateway());
	fprintf(fp, "\t\t\tmask\t%s\n",fCurrentSettings->GetNetmask());
	fprintf(fp, "\t\t}\n}\n\n");

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
		(new BAlert("error", "The net_server needs to run for the auto "
			"configuration!", "Ok"))->Go();
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
		BString errorMessage("Sending auto-config message failed: ");
		errorMessage << strerror(status);
		(new BAlert("error", errorMessage.String(), "Ok"))->Go();
		return status;
	} else if (reply.FindInt32("status", &status) == B_OK
			&& status != B_OK) {
		BString errorMessage("Auto-configuring failed: ");
		errorMessage << strerror(status);
		(new BAlert("error", errorMessage.String(), "Ok"))->Go();
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
		case kMsgMode:
			if (BMenuItem* item = fTypeMenuField->Menu()->FindMarked())
					_EnableTextControls(strcmp(item->Label(), "DHCP") != 0);
			fApplyButton->SetEnabled(true);
			fRevertButton->SetEnabled(true);					
			break;
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
				//We don't want to enable loop
				if (strcmp(fCurrentSettings->GetName(), "loop") == 0)
					fApplyButton->SetEnabled(false);
				else
					fApplyButton->SetEnabled(true);
			break;
		default:
			BView::MessageReceived(message);
	}
}
