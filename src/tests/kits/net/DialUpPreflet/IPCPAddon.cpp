//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------
// IPCPAddon saves the loaded settings.
// IPCPWindow saves the current settings.
//-----------------------------------------------------------------------

#include "IPCPAddon.h"

#include "MessageDriverSettingsUtils.h"

#include <Box.h>
#include <Button.h>

#include <PPPDefs.h>
#include <IPCP.h>
	// from IPCP addon

#define MSG_CHANGE_SETTINGS			'CHGS'
#define MSG_RESET_SETTINGS			'RSTS'


static const char kFriendlyName[] = "IPv4 (TCP/IP): Internet Protocol";
static const char kTechnicalName[] = "IPCP";
static const char kKernelModuleName[] = "ipcp";


IPCPAddon::IPCPAddon(BMessage *addons)
	: DialUpAddon(addons),
	fSettings(NULL),
	fProfile(NULL)
{
	float windowHeight = 3 * 20 // text controls
		+ 35 // buttons
		+ 5 * 5 + 10 + 20; // space between controls and bottom
	BRect rect(350, 250, 650, 250 + windowHeight);
	fIPCPWindow = new IPCPWindow(this, rect);
}


IPCPAddon::~IPCPAddon()
{
}


const char*
IPCPAddon::FriendlyName() const
{
	return kFriendlyName;
}


const char*
IPCPAddon::TechnicalName() const
{
	return kTechnicalName;
}


const char*
IPCPAddon::KernelModuleName() const
{
	return kKernelModuleName;
}


bool
IPCPAddon::LoadSettings(BMessage *settings, BMessage *profile, bool isNew)
{
	fIsNew = isNew;
	fIPAddress = fPrimaryDNS = fSecondaryDNS = "";
	fSettings = settings;
	fProfile = profile;
	if(!settings || !profile || isNew) {
		if(fIPCPWindow)
			fIPCPWindow->Reload();
		return true;
	}
	
	BMessage protocol;
	
	// settings
	int32 protocolIndex = FindIPCPProtocol(*fSettings, protocol);
	if(protocolIndex < 0)
		return false;
	
	protocol.AddBool(MDSU_VALID, true);
	fSettings->ReplaceMessage(MDSU_PARAMETERS, protocolIndex, &protocol);
	
	// profile
	protocolIndex = FindIPCPProtocol(*fProfile, protocol);
	if(protocolIndex < 0)
		return false;
	
	// the "Local" side parameter
	BMessage local;
	int32 localSideIndex = 0;
	if(!FindMessageParameter(IPCP_LOCAL_SIDE_KEY, protocol, local, &localSideIndex)) {
		protocol.AddBool(MDSU_VALID, true);
		fProfile->ReplaceMessage(MDSU_PARAMETERS, protocolIndex, &protocol);
		return true;
	}
	
	// now load the supported parameters (the client-relevant subset)
	BString name;
	BMessage parameter;
	int32 index = 0;
	if(!FindMessageParameter(IPCP_IP_ADDRESS_KEY, local, parameter, &index)
			|| parameter.FindString(MDSU_VALUES, &fIPAddress) != B_OK)
		fIPAddress = "";
	else {
		if(fIPAddress == "auto")
			fIPAddress = "";
		
		parameter.AddBool(MDSU_VALID, true);
		local.ReplaceMessage(MDSU_PARAMETERS, index, &parameter);
	}
	
	index = 0;
	if(!FindMessageParameter(IPCP_PRIMARY_DNS_KEY, local, parameter, &index)
			|| parameter.FindString(MDSU_VALUES, &fPrimaryDNS) != B_OK)
		fPrimaryDNS = "";
	else {
		if(fPrimaryDNS == "auto")
			fPrimaryDNS = "";
		
		parameter.AddBool(MDSU_VALID, true);
		local.ReplaceMessage(MDSU_PARAMETERS, index, &parameter);
	}
	
	index = 0;
	if(!FindMessageParameter(IPCP_SECONDARY_DNS_KEY, local, parameter, &index)
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
	
	return true;
}


void
IPCPAddon::IsModified(bool& settings, bool& profile) const
{
	// some part of this work is done by the "Protocols" tab, thus we do not need to
	// check whether we are a new protocol or not
	settings = false;
	
	if(!fSettings) {
		profile = false;
		return;
	}
	
	profile = (fIPAddress != fIPCPWindow->IPAddress()
		|| fPrimaryDNS != fIPCPWindow->PrimaryDNS()
		|| fSecondaryDNS != fIPCPWindow->SecondaryDNS());
}


bool
IPCPAddon::SaveSettings(BMessage *settings, BMessage *profile, bool saveTemporary)
{
	if(!fSettings || !settings)
		return false;
	
	BMessage protocol, local;
	protocol.AddString(MDSU_NAME, PPP_PROTOCOL_KEY);
	protocol.AddString(MDSU_VALUES, kKernelModuleName);
	settings->AddMessage(MDSU_PARAMETERS, &protocol);
		// the settings contain a simple "protocol ipcp" string
	
	// now create the profile with all subparameters
	local.AddString(MDSU_NAME, IPCP_LOCAL_SIDE_KEY);
	bool needsLocal = false;
	
	if(fIPCPWindow->IPAddress() && strlen(fIPCPWindow->IPAddress()) > 0) {
		// save IP address, too
		needsLocal = true;
		BMessage ip;
		ip.AddString(MDSU_NAME, IPCP_IP_ADDRESS_KEY);
		ip.AddString(MDSU_VALUES, fIPCPWindow->IPAddress());
		local.AddMessage(MDSU_PARAMETERS, &ip);
	}
	
	if(fIPCPWindow->PrimaryDNS() && strlen(fIPCPWindow->PrimaryDNS()) > 0) {
		// save service name, too
		needsLocal = true;
		BMessage dns;
		dns.AddString(MDSU_NAME, IPCP_PRIMARY_DNS_KEY);
		dns.AddString(MDSU_VALUES, fIPCPWindow->PrimaryDNS());
		local.AddMessage(MDSU_PARAMETERS, &dns);
	}
	
	if(fIPCPWindow->SecondaryDNS() && strlen(fIPCPWindow->SecondaryDNS()) > 0) {
		// save service name, too
		needsLocal = true;
		BMessage dns;
		dns.AddString(MDSU_NAME, IPCP_SECONDARY_DNS_KEY);
		dns.AddString(MDSU_VALUES, fIPCPWindow->SecondaryDNS());
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
	if(width)
		*width = fIPCPWindow->Bounds().Width();
	if(height)
		*height = fIPCPWindow->Bounds().Height();
	
	return true;
}


BView*
IPCPAddon::CreateView(BPoint leftTop)
{
	fIPCPWindow->MoveTo(leftTop);
	fIPCPWindow->Show();
	return NULL;
}


int32
IPCPAddon::FindIPCPProtocol(BMessage& message, BMessage& protocol) const
{
	BString name;
	for(int32 index = 0; FindMessageParameter(PPP_PROTOCOL_KEY, message, protocol,
			&index); index++)
		if(protocol.FindString(MDSU_VALUES, &name) == B_OK
				&& name == kKernelModuleName)
			return index;
	
	return -1;
}


IPCPWindow::IPCPWindow(IPCPAddon *addon, BRect frame)
	: BWindow(frame, "IPCP Settings", B_MODAL_WINDOW,
		B_NOT_RESIZABLE | B_NOT_CLOSABLE),
	fAddon(addon)
{
	BRect rect = Bounds();
	BView *backgroundView = new BView(rect, "backgroundView", B_FOLLOW_NONE, 0);
	backgroundView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	rect.InsetBy(5, 5);
	
	float boxHeight = 20 // space at top
		+ 3 * 20 // size of controls
		+ 4 * 5; // space between controls and bottom
	rect.bottom = rect.top + boxHeight;
	BBox *ipcpBox = new BBox(rect, "IPCP");
	ipcpBox->SetLabel("IPCP");
	rect = ipcpBox->Bounds();
	rect.InsetBy(10, 0);
	rect.top = 20;
	rect.bottom = rect.top + 20;
	fIPAddress = new BTextControl(rect, "ip", "IP Address: ", NULL, NULL);
	rect.top = rect.bottom + 5;
	rect.bottom = rect.top + 20;
	fPrimaryDNS = new BTextControl(rect, "primaryDNS", "Primary DNS: ", NULL, NULL);
	rect.top = rect.bottom + 5;
	rect.bottom = rect.top + 20;
	fSecondaryDNS = new BTextControl(rect, "secondaryDNS", "Secondary DNS: ", NULL,
		NULL);
	
	// set divider of text controls
	float ipAddressWidth = backgroundView->StringWidth(fIPAddress->Label()) + 5;
	float primaryDNSWidth = backgroundView->StringWidth(fPrimaryDNS->Label()) + 5;
	float secondaryDNSWidth = backgroundView->StringWidth(fSecondaryDNS->Label()) + 5;
	float controlWidth;
	if(ipAddressWidth > primaryDNSWidth) {
		if(ipAddressWidth > secondaryDNSWidth)
			controlWidth = ipAddressWidth;
		else
			controlWidth = secondaryDNSWidth;
	} else {
		if(primaryDNSWidth > secondaryDNSWidth)
			controlWidth = primaryDNSWidth;
		else
			controlWidth = secondaryDNSWidth;
	}
	
	fIPAddress->SetDivider(controlWidth);
	fPrimaryDNS->SetDivider(controlWidth);
	fSecondaryDNS->SetDivider(controlWidth);
	
	// add buttons to service window
	rect = ipcpBox->Frame();
	rect.top = rect.bottom + 10;
	rect.bottom = rect.top + 25;
	float buttonWidth = (rect.Width() - 10) / 2;
	rect.right = rect.left + buttonWidth;
	fCancelButton = new BButton(rect, "CancelButton", "Cancel",
		new BMessage(MSG_RESET_SETTINGS));
	rect.left = rect.right + 10;
	rect.right = rect.left + buttonWidth;
	fOKButton = new BButton(rect, "OKButton", "OK", new BMessage(MSG_CHANGE_SETTINGS));
	
	ipcpBox->AddChild(fIPAddress);
	ipcpBox->AddChild(fPrimaryDNS);
	ipcpBox->AddChild(fSecondaryDNS);
	backgroundView->AddChild(ipcpBox);
	backgroundView->AddChild(fCancelButton);
	backgroundView->AddChild(fOKButton);
	AddChild(backgroundView);
	SetDefaultButton(fOKButton);
	Run();
		// this must be set in order for Reload() to work properly
}


void
IPCPWindow::Reload()
{
	Lock();
	fIPAddress->SetText(Addon()->IPAddress());
	fPreviousIPAddress = Addon()->IPAddress();
	fPrimaryDNS->SetText(Addon()->PrimaryDNS());
	fPreviousPrimaryDNS = Addon()->PrimaryDNS();
	fSecondaryDNS->SetText(Addon()->SecondaryDNS());
	fPreviousSecondaryDNS = Addon()->SecondaryDNS();
	Unlock();
}


void
IPCPWindow::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case MSG_CHANGE_SETTINGS:
			Hide();
			fPreviousIPAddress = fIPAddress->Text();
			fPreviousPrimaryDNS = fPrimaryDNS->Text();
			fPreviousSecondaryDNS = fSecondaryDNS->Text();
		break;
		
		case MSG_RESET_SETTINGS:
			Hide();
			fIPAddress->SetText(fPreviousIPAddress.String());
			fPrimaryDNS->SetText(fPreviousPrimaryDNS.String());
			fPrimaryDNS->SetText(fPreviousPrimaryDNS.String());
		break;
		
		default:
			BWindow::MessageReceived(message);
	}
}
