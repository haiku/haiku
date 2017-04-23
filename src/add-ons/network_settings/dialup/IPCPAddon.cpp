/*
 * Copyright 2003-2004 Waldemar Kornewald. All rights reserved.
 * Copyright 2017 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

//-----------------------------------------------------------------------
// IPCPAddon saves the loaded settings.
// IPCPView saves the current settings.
//-----------------------------------------------------------------------

#include "IPCPAddon.h"

#include "MessageDriverSettingsUtils.h"

#include <StringView.h>
#include <LayoutBuilder.h>
#include <SpaceLayoutItem.h>

#include <PPPDefs.h>
#include <IPCP.h>
	// from IPCP addon


// GUI constants
static const uint32 kDefaultButtonWidth = 80;

// message constants
static const uint32 kMsgUpdateControls = 'UCTL';

// labels
static const char *kLabelIPCP = "TCP/IP";
static const char *kLabelIPAddress = "IP Address: ";
static const char *kLabelPrimaryDNS = "Primary DNS: ";
static const char *kLabelSecondaryDNS = "Secondary DNS: ";
static const char *kLabelOptional = "(Optional)";
static const char *kLabelExtendedOptions = "Extended Options:";
static const char *kLabelEnabled = "Enable TCP/IP Protocol";

// add-on descriptions
static const char *kKernelModuleName = "ipcp";


IPCPAddon::IPCPAddon(BMessage *addons)
	: DialUpAddon(addons),
	fSettings(NULL),
	fProfile(NULL),
	fIPCPView(NULL)
{
}


IPCPAddon::~IPCPAddon()
{
}


bool
IPCPAddon::LoadSettings(BMessage *settings, BMessage *profile, bool isNew)
{
	fIsNew = isNew;
	fIsEnabled = false;
	fIPAddress = fPrimaryDNS = fSecondaryDNS = "";
	fSettings = settings;
	fProfile = profile;

	if(fIPCPView)
		fIPCPView->Reload();
			// reset all views (empty settings)

	if(!settings || !profile || isNew)
		return true;

	BMessage protocol;

	// settings
	int32 protocolIndex = FindIPCPProtocol(*fSettings, &protocol);
	if(protocolIndex < 0)
		return true;

	protocol.AddBool(MDSU_VALID, true);
	fSettings->ReplaceMessage(MDSU_PARAMETERS, protocolIndex, &protocol);

	// profile
	protocolIndex = FindIPCPProtocol(*fProfile, &protocol);
	if(protocolIndex < 0)
		return true;

	fIsEnabled = true;

	// the "Local" side parameter
	BMessage local;
	int32 localSideIndex = 0;
	if(!FindMessageParameter(IPCP_LOCAL_SIDE_KEY, protocol, &local, &localSideIndex))
		local.MakeEmpty();
			// just fall through and pretend we have an empty "Local" side parameter

	// now load the supported parameters (the client-relevant subset)
	BString name;
	BMessage parameter;
	int32 index = 0;
	if(!FindMessageParameter(IPCP_IP_ADDRESS_KEY, local, &parameter, &index)
			|| parameter.FindString(MDSU_VALUES, &fIPAddress) != B_OK)
		fIPAddress = "";
	else {
		if(fIPAddress == "auto")
			fIPAddress = "";

		parameter.AddBool(MDSU_VALID, true);
		local.ReplaceMessage(MDSU_PARAMETERS, index, &parameter);
	}

	index = 0;
	if(!FindMessageParameter(IPCP_PRIMARY_DNS_KEY, local, &parameter, &index)
			|| parameter.FindString(MDSU_VALUES, &fPrimaryDNS) != B_OK)
		fPrimaryDNS = "";
	else {
		if(fPrimaryDNS == "auto")
			fPrimaryDNS = "";

		parameter.AddBool(MDSU_VALID, true);
		local.ReplaceMessage(MDSU_PARAMETERS, index, &parameter);
	}

	index = 0;
	if(!FindMessageParameter(IPCP_SECONDARY_DNS_KEY, local, &parameter, &index)
			|| parameter.FindString(MDSU_VALUES, &fSecondaryDNS) != B_OK)
		fSecondaryDNS = "";
	else {
		if(fSecondaryDNS == "auto")
			fSecondaryDNS = "";

		parameter.AddBool(MDSU_VALID, true);
		local.ReplaceMessage(MDSU_PARAMETERS, index, &parameter);
	}

	local.AddBool(MDSU_VALID, true);
	protocol.ReplaceMessage(MDSU_PARAMETERS, localSideIndex, &local);
	protocol.AddBool(MDSU_VALID, true);
	fProfile->ReplaceMessage(MDSU_PARAMETERS, protocolIndex, &protocol);

	if(fIPCPView)
		fIPCPView->Reload();

	return true;
}


void
IPCPAddon::IsModified(bool *settings, bool *profile) const
{
	if(!fSettings) {
		*settings = *profile = false;
		return;
	}

	*settings = fIsEnabled != fIPCPView->IsEnabled();
	*profile = (*settings || fIPAddress != fIPCPView->IPAddress()
		|| fPrimaryDNS != fIPCPView->PrimaryDNS()
		|| fSecondaryDNS != fIPCPView->SecondaryDNS());
}


bool
IPCPAddon::SaveSettings(BMessage *settings, BMessage *profile, bool saveTemporary)
{
	if(!fSettings || !settings)
		return false;

	if(!fIPCPView->IsEnabled())
		return true;

	BMessage protocol, local;
	protocol.AddString(MDSU_NAME, PPP_PROTOCOL_KEY);
	protocol.AddString(MDSU_VALUES, kKernelModuleName);
	settings->AddMessage(MDSU_PARAMETERS, &protocol);
		// the settings contain a simple "protocol ipcp" string

	// now create the profile with all subparameters
	local.AddString(MDSU_NAME, IPCP_LOCAL_SIDE_KEY);
	bool needsLocal = false;

	if(fIPCPView->IPAddress() && strlen(fIPCPView->IPAddress()) > 0) {
		// save IP address, too
		needsLocal = true;
		BMessage ip;
		ip.AddString(MDSU_NAME, IPCP_IP_ADDRESS_KEY);
		ip.AddString(MDSU_VALUES, fIPCPView->IPAddress());
		local.AddMessage(MDSU_PARAMETERS, &ip);
	}

	if(fIPCPView->PrimaryDNS() && strlen(fIPCPView->PrimaryDNS()) > 0) {
		// save service name, too
		needsLocal = true;
		BMessage dns;
		dns.AddString(MDSU_NAME, IPCP_PRIMARY_DNS_KEY);
		dns.AddString(MDSU_VALUES, fIPCPView->PrimaryDNS());
		local.AddMessage(MDSU_PARAMETERS, &dns);
	}

	if(fIPCPView->SecondaryDNS() && strlen(fIPCPView->SecondaryDNS()) > 0) {
		// save service name, too
		needsLocal = true;
		BMessage dns;
		dns.AddString(MDSU_NAME, IPCP_SECONDARY_DNS_KEY);
		dns.AddString(MDSU_VALUES, fIPCPView->SecondaryDNS());
		local.AddMessage(MDSU_PARAMETERS, &dns);
	}

	if(needsLocal)
		protocol.AddMessage(MDSU_PARAMETERS, &local);

	profile->AddMessage(MDSU_PARAMETERS, &protocol);

	return true;
}


bool
IPCPAddon::GetPreferredSize(float *width, float *height) const
{
	BRect rect;
	if(Addons()->FindRect(DUN_TAB_VIEW_RECT, &rect) != B_OK)
		rect.Set(0, 0, 200, 300);
			// set default values

	if(width)
		*width = rect.Width();
	if(height)
		*height = rect.Height();

	return true;
}


BView*
IPCPAddon::CreateView()
{
	if (!fIPCPView) {
		fIPCPView = new IPCPView(this);
		fIPCPView->Reload();
	}

	return fIPCPView;
}


int32
IPCPAddon::FindIPCPProtocol(const BMessage& message, BMessage *protocol) const
{
	if(!protocol)
		return -1;

	BString name;
	for(int32 index = 0; FindMessageParameter(PPP_PROTOCOL_KEY, message, protocol,
			&index); index++)
		if(protocol->FindString(MDSU_VALUES, &name) == B_OK
				&& name == kKernelModuleName)
			return index;

	return -1;
}


IPCPView::IPCPView(IPCPAddon *addon)
	: BView(kLabelIPCP, 0),
	fAddon(addon)
{
	fIPAddress = new BTextControl("ip", kLabelIPAddress, NULL, NULL);
	fPrimaryDNS = new BTextControl("primaryDNS", kLabelPrimaryDNS, NULL, NULL);
	fSecondaryDNS = new BTextControl("secondaryDNS", kLabelSecondaryDNS, NULL,
		NULL);
	fIsEnabled = new BCheckBox("isEnabled", kLabelEnabled,
		new BMessage(kMsgUpdateControls));

	BLayoutBuilder::Grid<>(this, B_USE_HALF_ITEM_SPACING)
		.SetInsets(B_USE_HALF_ITEM_INSETS)
		.Add(fIsEnabled, 0, 0)
		.Add(new BStringView("expert", kLabelExtendedOptions), 0, 1)
		.Add(fIPAddress, 0, 2)
		.Add(new BStringView("optional_1", kLabelOptional), 1, 2)
		.Add(fPrimaryDNS, 0, 3)
		.Add(new BStringView("optional_2", kLabelOptional), 1, 3)
		.Add(fSecondaryDNS, 0, 4)
		.Add(new BStringView("optional_3", kLabelOptional), 1, 4)
		.Add(BSpaceLayoutItem::CreateGlue(), 0, 5)
	.End();
}


void
IPCPView::Reload()
{
	fIsEnabled->SetValue(Addon()->IsEnabled() || Addon()->IsNew());
		// enable TCP/IP by default
	fIPAddress->SetText(Addon()->IPAddress());
	fPrimaryDNS->SetText(Addon()->PrimaryDNS());
	fSecondaryDNS->SetText(Addon()->SecondaryDNS());

	UpdateControls();
}


void
IPCPView::AttachedToWindow()
{
	SetViewColor(Parent()->ViewColor());
	fIsEnabled->SetTarget(this);
}


void
IPCPView::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case kMsgUpdateControls:
			UpdateControls();
		break;

		default:
			BView::MessageReceived(message);
	}
}


void
IPCPView::UpdateControls()
{
	fIPAddress->SetEnabled(IsEnabled());
	fPrimaryDNS->SetEnabled(IsEnabled());
	fSecondaryDNS->SetEnabled(IsEnabled());
}
