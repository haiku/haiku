//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------
// PPPoEAddon saves the loaded settings.
// PPPoEView saves the current settings.
//-----------------------------------------------------------------------

#include "PPPoEAddon.h"

#include "MessageDriverSettingsUtils.h"

#include <Box.h>
#include <Button.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Messenger.h>
#include <PopUpMenu.h>
#include <StringView.h>
#include <Window.h>

#include <PPPDefs.h>
#include <PPPoE.h>
	// from PPPoE addon

#define MSG_SHOW_SERVICE_WINDOW		'SHSW'
#define MSG_CHANGE_SERVICE			'SELD'
#define MSG_RESET_SERVICE			'SELA'


static const char kFriendlyName[] = "DSL, Cable, etc.";
static const char kTechnicalName[] = "PPPoE";
static const char kKernelModuleName[] = "pppoe";


PPPoEAddon::PPPoEAddon(BMessage *addons)
	: DialUpAddon(addons),
	fSettings(NULL),
	fProfile(NULL),
	fPPPoEView(NULL)
{
	fHeight = 20 // interface name control
		+ 25 // service button
		+ 5; // space between controls
}


PPPoEAddon::~PPPoEAddon()
{
	delete fPPPoEView;
		// this may have been set to NULL from the view's destructor!
}


const char*
PPPoEAddon::FriendlyName() const
{
	return kFriendlyName;
}


const char*
PPPoEAddon::TechnicalName() const
{
	return kTechnicalName;
}


const char*
PPPoEAddon::KernelModuleName() const
{
	return kKernelModuleName;
}


bool
PPPoEAddon::LoadSettings(BMessage *settings, BMessage *profile, bool isNew)
{
	fIsNew = isNew;
	fInterfaceName = fACName = fServiceName = "";
	fSettings = settings;
	fProfile = profile;
	if(!settings || !profile || isNew) {
		if(fPPPoEView)
			fPPPoEView->Reload();
		return true;
	}
	
	BMessage device;
	int32 deviceIndex = 0;
	if(!FindMessageParameter(PPP_DEVICE_KEY, *fSettings, device, &deviceIndex))
		return false;
			// error: no device
	
	BString name;
	if(device.FindString(MDSU_VALUES, &name) != B_OK || name != kKernelModuleName)
		return false;
			// error: no device
	
	BMessage parameter;
	int32 index = 0;
	if(!FindMessageParameter(PPPoE_INTERFACE_KEY, device, parameter, &index)
			|| parameter.FindString(MDSU_VALUES, &fInterfaceName) != B_OK)
		return false;
			// error: no interface
	else {
		parameter.AddBool(MDSU_VALID, true);
		device.ReplaceMessage(MDSU_PARAMETERS, index, &parameter);
	}
	
	index = 0;
	if(!FindMessageParameter(PPPoE_AC_NAME_KEY, device, parameter, &index)
			|| parameter.FindString(MDSU_VALUES, &fACName) != B_OK)
		fACName = "";
	else {
		parameter.AddBool(MDSU_VALID, true);
		device.ReplaceMessage(MDSU_PARAMETERS, index, &parameter);
	}
	
	index = 0;
	if(!FindMessageParameter(PPPoE_SERVICE_NAME_KEY, device, parameter, &index)
			|| parameter.FindString(MDSU_VALUES, &fServiceName) != B_OK)
		fServiceName = "";
	else {
		parameter.AddBool(MDSU_VALID, true);
		device.ReplaceMessage(MDSU_PARAMETERS, index, &parameter);
	}
	
	device.AddBool(MDSU_VALID, true);
	fSettings->ReplaceMessage(MDSU_PARAMETERS, deviceIndex, &device);
	
	return true;
}


void
PPPoEAddon::IsModified(bool& settings, bool& profile) const
{
	if(!fSettings) {
		settings = profile = false;
		return;
	}
	
	settings = profile = false;
	settings = (fInterfaceName != fPPPoEView->InterfaceName()
		|| fACName != fPPPoEView->ACName()
		|| fServiceName != fPPPoEView->ServiceName());
}


bool
PPPoEAddon::SaveSettings(BMessage *settings, BMessage *profile, bool saveTemporary)
{
	if(!fSettings || !settings || !fPPPoEView->InterfaceName()
			|| strlen(fPPPoEView->InterfaceName()) == 0)
		return false;
			// TODO: tell user that an interface is needed (if we fail because of this)
	
	BMessage device, interface;
	device.AddString(MDSU_NAME, PPP_DEVICE_KEY);
	device.AddString(MDSU_VALUES, kKernelModuleName);
	
	interface.AddString(MDSU_NAME, PPPoE_INTERFACE_KEY);
	interface.AddString(MDSU_VALUES, fPPPoEView->InterfaceName());
	device.AddMessage(MDSU_PARAMETERS, &interface);
	
	if(fPPPoEView->ACName() && strlen(fPPPoEView->ACName()) > 0) {
		// save access concentrator, too
		BMessage ac;
		ac.AddString(MDSU_NAME, PPPoE_AC_NAME_KEY);
		ac.AddString(MDSU_VALUES, fPPPoEView->ACName());
		device.AddMessage(MDSU_PARAMETERS, &ac);
	}
	
	if(fPPPoEView->ServiceName() && strlen(fPPPoEView->ServiceName()) > 0) {
		// save service name, too
		BMessage service;
		service.AddString(MDSU_NAME, PPPoE_SERVICE_NAME_KEY);
		service.AddString(MDSU_VALUES, fPPPoEView->ServiceName());
		device.AddMessage(MDSU_PARAMETERS, &service);
	}
	
	settings->AddMessage(MDSU_PARAMETERS, &device);
	
	return true;
}


bool
PPPoEAddon::GetPreferredSize(float *width, float *height) const
{
	float viewWidth;
	if(Addons()->FindFloat(DUN_DEVICE_VIEW_WIDTH, &viewWidth) != B_OK)
		viewWidth = 270;
			// default value
	
	if(width)
		*width = viewWidth;
	if(height)
		*height = fHeight;
	
	return true;
}


BView*
PPPoEAddon::CreateView(BPoint leftTop)
{
	if(!fPPPoEView) {
		float width;
		if(!Addons()->FindFloat(DUN_DEVICE_VIEW_WIDTH, &width))
			width = 270;
				// default value
		
		BRect rect(0, 0, width, fHeight);
		fPPPoEView = new PPPoEView(this, rect);
	}
	
	fPPPoEView->MoveTo(leftTop);
	fPPPoEView->Reload();
	return fPPPoEView;
}


PPPoEView::PPPoEView(PPPoEAddon *addon, BRect frame)
	: BView(frame, "PPPoEView", B_FOLLOW_NONE, 0),
	fAddon(addon)
{
	BRect rect = Bounds();
	rect.InsetBy(5, 0);
	rect.bottom = 20;
	fInterfaceName = new BTextControl(rect, "interface", "Interface Name: ", NULL,
		NULL);
	fInterfaceName->SetDivider(StringWidth(fInterfaceName->Label()) + 5);
	// TODO: add "Query" pop-up menu to allow quickly choosing interface from list
	rect.top = rect.bottom + 5;
	rect.bottom = rect.top + 45;
	fServiceButton = new BButton(rect, "ServiceButton", "Service",
		new BMessage(MSG_SHOW_SERVICE_WINDOW));
	fServiceButton->ResizeToPreferred();
	
	// create service window
	float boxHeight = 20 // space at top
		+ 2 * 20 // size of controls
		+ 2 * 5; // space between controls and bottom
	float windowHeight = boxHeight
		+ 35 // buttons
		+ 2 * 5; // space between controls and bottom
	rect.Set(350, 250, 650, 250 + windowHeight);
	fServiceWindow = new BWindow(rect, "Service", B_MODAL_WINDOW,
		B_NOT_RESIZABLE | B_NOT_CLOSABLE);
	rect = fServiceWindow->Bounds();
	BView *serviceView = new BView(rect, "serviceView", B_FOLLOW_NONE, 0);
	serviceView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	rect.InsetBy(5, 5);
	rect.bottom = rect.top + boxHeight;
	BBox *serviceBox = new BBox(rect, "Service");
	serviceBox->SetLabel("Service");
	rect = serviceBox->Bounds();
	rect.InsetBy(10, 0);
	rect.top = 20;
	rect.bottom = rect.top + 20;
	fACName = new BTextControl(rect, "ac", "AC: ", NULL, NULL);
	rect.top = rect.bottom + 5;
	rect.bottom = rect.top + 20;
	fServiceName = new BTextControl(rect, "service", "Service: ", NULL, NULL);
	
	// set divider of text controls
	float acNameWidth = StringWidth(fACName->Label()) + 5;
	float serviceNameWidth = StringWidth(fServiceName->Label()) + 5;
	if(acNameWidth > serviceNameWidth)
		serviceNameWidth = acNameWidth;
	else
		acNameWidth = serviceNameWidth;
	fACName->SetDivider(acNameWidth);
	fServiceName->SetDivider(serviceNameWidth);
	
	// add buttons to service window
	rect = serviceBox->Frame();
	rect.top = rect.bottom + 10;
	rect.bottom = rect.top + 25;
	float buttonWidth = (rect.Width() - 10) / 2;
	rect.right = rect.left + buttonWidth;
	fCancelButton = new BButton(rect, "CancelButton", "Cancel",
		new BMessage(MSG_RESET_SERVICE));
	rect.left = rect.right + 10;
	rect.right = rect.left + buttonWidth;
	fOKButton = new BButton(rect, "OKButton", "OK", new BMessage(MSG_CHANGE_SERVICE));
	
	serviceBox->AddChild(fACName);
	serviceBox->AddChild(fServiceName);
	serviceView->AddChild(serviceBox);
	serviceView->AddChild(fCancelButton);
	serviceView->AddChild(fOKButton);
	fServiceWindow->AddChild(serviceView);
	fServiceWindow->SetDefaultButton(fOKButton);
	fServiceWindow->Run();
	
	AddChild(fInterfaceName);
	AddChild(fServiceButton);
}


PPPoEView::~PPPoEView()
{
	Addon()->UnregisterView();
}


void
PPPoEView::Reload()
{
	fInterfaceName->SetText(Addon()->InterfaceName());
	fServiceWindow->Lock();
	fACName->SetText(Addon()->ACName());
	fPreviousACName = Addon()->ACName();
	fServiceName->SetText(Addon()->ServiceName());
	fPreviousServiceName = Addon()->ServiceName();
	fServiceWindow->Unlock();
}


void
PPPoEView::AttachedToWindow()
{
	SetViewColor(Parent()->ViewColor());
	fInterfaceName->SetTarget(this);
	fServiceButton->SetTarget(this);
	fACName->SetTarget(this);
	fServiceName->SetTarget(this);
	fCancelButton->SetTarget(this);
	fOKButton->SetTarget(this);
}


void
PPPoEView::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case MSG_SHOW_SERVICE_WINDOW:
			fServiceWindow->Show();
		break;
		
		case MSG_CHANGE_SERVICE:
			fServiceWindow->Hide();
			fServiceWindow->Lock();
			fPreviousACName = fACName->Text();
			fPreviousServiceName = fServiceName->Text();
			fServiceWindow->Unlock();
		break;
		
		case MSG_RESET_SERVICE:
			fServiceWindow->Hide();
			fServiceWindow->Lock();
			fACName->SetText(fPreviousACName.String());
			fServiceName->SetText(fPreviousServiceName.String());
			fServiceWindow->Unlock();
		break;
		
		default:
			BView::MessageReceived(message);
	}
}
