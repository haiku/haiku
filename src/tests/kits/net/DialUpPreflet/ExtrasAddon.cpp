//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------
// ExtrasAddon saves the loaded settings.
// ExtrasView saves the current settings.
//-----------------------------------------------------------------------

#include "ExtrasAddon.h"

#include "MessageDriverSettingsUtils.h"

#include <PPPDefs.h>
#include <settings_tools.h>


#define MSG_UPDATE_CONTROLS		'UPDC'


ExtrasAddon::ExtrasAddon(BMessage *addons)
	: DialUpAddon(addons),
	fSettings(NULL),
	fProfile(NULL),
	fExtrasView(NULL)
{
}


ExtrasAddon::~ExtrasAddon()
{
}


bool
ExtrasAddon::LoadSettings(BMessage *settings, BMessage *profile, bool isNew)
{
	fIsNew = isNew;
	fDoesDialOnDemand = fAskBeforeDialing = fDoesAutoRedial = false;
	fSettings = settings;
	fProfile = profile;
	
	if(fExtrasView)
		fExtrasView->Reload();
			// reset all views (empty settings)
	
	if(!settings || !profile || isNew)
		return true;
	
	BMessage parameter;
	int32 index = 0;
	const char *value;
	if(FindMessageParameter(PPP_DIAL_ON_DEMAND_KEY, *fSettings, &parameter, &index)
			&& parameter.FindString(MDSU_VALUES, &value) == B_OK) {
		if(get_boolean_value(value, false))
			fDoesDialOnDemand = true;
		
		parameter.AddBool(MDSU_VALID, true);
		fSettings->ReplaceMessage(MDSU_PARAMETERS, index, &parameter);
	}
	
	index = 0;
	if(FindMessageParameter(PPP_ASK_BEFORE_DIALING_KEY, *fSettings, &parameter, &index)
			&& parameter.FindString(MDSU_VALUES, &value) == B_OK) {
		if(get_boolean_value(value, false))
			fAskBeforeDialing = true;
		
		parameter.AddBool(MDSU_VALID, true);
		fSettings->ReplaceMessage(MDSU_PARAMETERS, index, &parameter);
	}
	
	index = 0;
	if(FindMessageParameter(PPP_AUTO_REDIAL_KEY, *fSettings, &parameter, &index)
			&& parameter.FindString(MDSU_VALUES, &value) == B_OK) {
		if(get_boolean_value(value, false))
			fDoesAutoRedial = true;
		
		parameter.AddBool(MDSU_VALID, true);
		fSettings->ReplaceMessage(MDSU_PARAMETERS, index, &parameter);
	}
	
	if(fExtrasView)
		fExtrasView->Reload();
			// reload new settings
	
	return true;
}


void
ExtrasAddon::IsModified(bool *settings, bool *profile) const
{
	*settings = *profile = false;
	
	if(!fSettings || !fExtrasView)
		return;
	
	*settings = DoesDialOnDemand() != fExtrasView->DoesDialOnDemand()
		|| AskBeforeDialing() != fExtrasView->AskBeforeDialing()
		|| DoesAutoRedial() != fExtrasView->DoesAutoRedial();
}


bool
ExtrasAddon::SaveSettings(BMessage *settings, BMessage *profile, bool saveTemporary)
{
	if(!fSettings || !settings)
		return false;
	
	BMessage parameter;
	if(fExtrasView->DoesDialOnDemand()) {
		parameter.MakeEmpty();
		parameter.AddString(MDSU_NAME, PPP_DIAL_ON_DEMAND_KEY);
		parameter.AddString(MDSU_VALUES, "enabled");
		settings->AddMessage(MDSU_PARAMETERS, &parameter);
		
		if(fExtrasView->AskBeforeDialing()) {
			parameter.MakeEmpty();
			parameter.AddString(MDSU_NAME, PPP_ASK_BEFORE_DIALING_KEY);
			parameter.AddString(MDSU_VALUES, "enabled");
			settings->AddMessage(MDSU_PARAMETERS, &parameter);
		}
	}
	
	if(fExtrasView->DoesAutoRedial()) {
		parameter.MakeEmpty();
		parameter.AddString(MDSU_NAME, PPP_AUTO_REDIAL_KEY);
		parameter.AddString(MDSU_VALUES, "enabled");
		settings->AddMessage(MDSU_PARAMETERS, &parameter);
	}
	
	return true;
}


bool
ExtrasAddon::GetPreferredSize(float *width, float *height) const
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
ExtrasAddon::CreateView(BPoint leftTop)
{
	if(!fExtrasView) {
		BRect rect;
		Addons()->FindRect(DUN_TAB_VIEW_RECT, &rect);
		fExtrasView = new ExtrasView(this, rect);
		fExtrasView->Reload();
	}
	
	fExtrasView->MoveTo(leftTop);
	return fExtrasView;
}


ExtrasView::ExtrasView(ExtrasAddon *addon, BRect frame)
	: BView(frame, "Extras", B_FOLLOW_NONE, 0),
	fAddon(addon)
{
	BRect rect = Bounds();
	rect.InsetBy(10, 10);
	rect.bottom = rect.top + 15;
	fDialOnDemand = new BRadioButton(rect, "DialOnDemand", "Dial On Demand",
		new BMessage(MSG_UPDATE_CONTROLS));
	rect.top = rect.bottom + 3;
	rect.bottom = rect.top + 15;
	rect.left += 15;
	fAskBeforeDialing = new BCheckBox(rect, "AskBeforeDialing", "Ask Before Dialing",
		NULL);
	rect.left -= 15;
	rect.top = rect.bottom + 5;
	rect.bottom = rect.top + 15;
	fDialManually = new BRadioButton(rect, "DialManually", "Dial Manually",
		new BMessage(MSG_UPDATE_CONTROLS));
	rect.top = rect.bottom + 20;
	rect.bottom = rect.top + 15;
	fAutoRedial = new BCheckBox(rect, "AutoRedial", "Auto-Redial", NULL);
	
	AddChild(fDialOnDemand);
	AddChild(fAskBeforeDialing);
	AddChild(fDialManually);
	AddChild(fAutoRedial);
}


void
ExtrasView::Reload()
{
	if(Addon()->DoesDialOnDemand() || Addon()->IsNew())
		fDialOnDemand->SetValue(1);
	else
		fDialManually->SetValue(1);
	
	fAskBeforeDialing->SetValue(Addon()->AskBeforeDialing());
	fAutoRedial->SetValue(Addon()->DoesAutoRedial());
	
	if(!Addon()->Settings())
		return;
	
	UpdateControls();
}


void
ExtrasView::AttachedToWindow()
{
	SetViewColor(Parent()->ViewColor());
	fDialOnDemand->SetTarget(this);
	fDialManually->SetTarget(this);
}


void
ExtrasView::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case MSG_UPDATE_CONTROLS:
			UpdateControls();
		break;
		
		default:
			BView::MessageReceived(message);
	}
}


void
ExtrasView::UpdateControls()
{
	if(fDialManually->Value())
		fAskBeforeDialing->SetEnabled(false);
	else
		fAskBeforeDialing->SetEnabled(true);
	
	fAskBeforeDialing->SetValue(1);
		// this should be enabled by default
}
