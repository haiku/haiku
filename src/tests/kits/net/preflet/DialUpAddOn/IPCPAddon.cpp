/* -----------------------------------------------------------------------
 * Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 * ----------------------------------------------------------------------- */

//-----------------------------------------------------------------------
// IPCPAddon saves the loaded settings.
// IPCPView saves the current settings.
//-----------------------------------------------------------------------

#include "IPCPAddon.h"

#include "MessageDriverSettingsUtils.h"
#include <stl_algobase.h>
	// for max()

#include <StringView.h>

#include <PPPDefs.h>
#include <IPCP.h>
	// from IPCP addon


// GUI constants
static const uint32 kDefaultButtonWidth = 80;

// message constants
static const uint32 kMsgUpdateControls = 'UCTL';

// labels
#ifdef LANG_GERMAN
static const char *kLabelIPCP = "TCP/IP";
static const char *kLabelIPAddress = "IP Adresse: ";
static const char *kLabelPrimaryDNS = "Primärer DNS: ";
static const char *kLabelSecondaryDNS = "Sekundärer DNS: ";
static const char *kLabelOptional = "(Optional)";
static const char *kLabelExtendedOptions = "Erweiterte Optionen:";
static const char *kLabelEnabled = "TCP/IP-Protokoll Verwenden";
#else
static const char *kLabelIPCP = "TCP/IP";
static const char *kLabelIPAddress = "IP Address: ";
static const char *kLabelPrimaryDNS = "Primary DNS: ";
static const char *kLabelSecondaryDNS = "Secondary DNS: ";
static const char *kLabelOptional = "(Optional)";
static const char *kLabelExtendedOptions = "Extended Options:";
static const char *kLabelEnabled = "Enable TCP/IP Protocol";
#endif

// add-on descriptions
static const char *kKernelModuleName = "ipcp";


IPCPAddon::IPCPAddon(BMessage *addons)
	: DialUpAddon(addons),
	fSettings(NULL),
	fProfile(NULL)
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
IPCPAddon::CreateView(BPoint leftTop)
{
	if(!fIPCPView) {
		BRect rect;
		Addons()->FindRect(DUN_TAB_VIEW_RECT, &rect);
		fIPCPView = new IPCPView(this, rect);
		fIPCPView->Reload();
	}
	
	fIPCPView->MoveTo(leftTop);
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


IPCPView::IPCPView(IPCPAddon *addon, BRect frame)
	: BView(frame, kLabelIPCP, B_FOLLOW_NONE, 0),
	fAddon(addon)
{
	BRect rect = Bounds();
	rect.InsetBy(10, 10);
	rect.bottom = rect.top + 20;
	BRect optionalRect(rect);
	rect.right -= 75;
	fIPAddress = new BTextControl(rect, "ip", kLabelIPAddress, NULL, NULL);
	optionalRect.left = rect.right + 5;
	optionalRect.bottom = optionalRect.top + 15;
	AddChild(new BStringView(optionalRect, "optional_1", kLabelOptional));
	rect.top = rect.bottom + 5;
	rect.bottom = rect.top + 20;
	fPrimaryDNS = new BTextControl(rect, "primaryDNS", kLabelPrimaryDNS, NULL, NULL);
	optionalRect.top = rect.top;
	optionalRect.bottom = optionalRect.top + 15;
	AddChild(new BStringView(optionalRect, "optional_2", kLabelOptional));
	rect.top = rect.bottom + 5;
	rect.bottom = rect.top + 20;
	fSecondaryDNS = new BTextControl(rect, "secondaryDNS", kLabelSecondaryDNS, NULL,
		NULL);
	optionalRect.top = rect.top;
	optionalRect.bottom = optionalRect.top + 15;
	AddChild(new BStringView(optionalRect, "optional_3", kLabelOptional));
	rect.top = rect.bottom + 50;
	rect.bottom = rect.top + 10;
	AddChild(new BStringView(rect, "expert", kLabelExtendedOptions));
	rect.top = rect.bottom + 5;
	rect.bottom = rect.top + 15;
	fIsEnabled = new BCheckBox(rect, "isEnabled", kLabelEnabled,
		new BMessage(kMsgUpdateControls));
	
	// set divider of text controls
	float controlWidth = max(max(StringWidth(fIPAddress->Label()),
		StringWidth(fPrimaryDNS->Label())), StringWidth(fSecondaryDNS->Label()));
	fIPAddress->SetDivider(controlWidth + 5);
	fPrimaryDNS->SetDivider(controlWidth + 5);
	fSecondaryDNS->SetDivider(controlWidth + 5);
	
	AddChild(fIsEnabled);
	AddChild(fIPAddress);
	AddChild(fPrimaryDNS);
	AddChild(fSecondaryDNS);
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
