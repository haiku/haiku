/*
 * Copyright 2003-2005, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

//-----------------------------------------------------------------------
// ConnectionOptionsAddon saves the loaded settings.
// ConnectionOptionsView saves the current settings.
//-----------------------------------------------------------------------

#include "ConnectionOptionsAddon.h"

#include "MessageDriverSettingsUtils.h"

#include <PPPDefs.h>
#include <settings_tools.h>


// message constants
static const uint32 kMsgUpdateControls = 'UCTL';

// labels
static const char *kLabelConnectionOptions = "Options";
static const char *kLabelAskBeforeConnecting = "Ask Before Connecting";
static const char *kLabelAutoReconnect = "Reconnect Automatically";


ConnectionOptionsAddon::ConnectionOptionsAddon(BMessage *addons)
	: DialUpAddon(addons),
	fSettings(NULL),
	fConnectionOptionsView(NULL)
{
	CreateView(BPoint(0,0));
	fDeleteView = true;
}


ConnectionOptionsAddon::~ConnectionOptionsAddon()
{
	if(fDeleteView)
		delete fConnectionOptionsView;
}


bool
ConnectionOptionsAddon::LoadSettings(BMessage *settings, bool isNew)
{
	fIsNew = isNew;
	fAskBeforeConnecting = fDoesAutoReconnect = false;
	fSettings = settings;
	
	fConnectionOptionsView->Reload();
		// reset all views (empty settings)
	
	if(!settings || isNew)
		return true;
	
	BMessage parameter;
	int32 index = 0;
	const char *value;
	if(FindMessageParameter(PPP_ASK_BEFORE_CONNECTING_KEY, *fSettings, &parameter,
			&index) && parameter.FindString(MDSU_VALUES, &value) == B_OK) {
		if(get_boolean_value(value, false))
			fAskBeforeConnecting = true;
		
		parameter.AddBool(MDSU_VALID, true);
		fSettings->ReplaceMessage(MDSU_PARAMETERS, index, &parameter);
	}
	
	index = 0;
	if(FindMessageParameter(PPP_AUTO_RECONNECT_KEY, *fSettings, &parameter, &index)
			&& parameter.FindString(MDSU_VALUES, &value) == B_OK) {
		if(get_boolean_value(value, false))
			fDoesAutoReconnect = true;
		
		parameter.AddBool(MDSU_VALID, true);
		fSettings->ReplaceMessage(MDSU_PARAMETERS, index, &parameter);
	}
	
	fConnectionOptionsView->Reload();
		// reload new settings
	
	return true;
}


void
ConnectionOptionsAddon::IsModified(bool *settings) const
{
	*settings = false;
	
	if(!fSettings || !fConnectionOptionsView)
		return;
	
	*settings = AskBeforeConnecting() != fConnectionOptionsView->AskBeforeConnecting()
		|| DoesAutoReconnect() != fConnectionOptionsView->DoesAutoReconnect();
}


bool
ConnectionOptionsAddon::SaveSettings(BMessage *settings)
{
	if(!fSettings || !settings)
		return false;
	
	BMessage parameter;
	if(fConnectionOptionsView->DoesAutoReconnect()) {
		parameter.MakeEmpty();
		parameter.AddString(MDSU_NAME, PPP_AUTO_RECONNECT_KEY);
		parameter.AddString(MDSU_VALUES, "enabled");
		settings->AddMessage(MDSU_PARAMETERS, &parameter);
	}
	
	return true;
}


bool
ConnectionOptionsAddon::GetPreferredSize(float *width, float *height) const
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
ConnectionOptionsAddon::CreateView(BPoint leftTop)
{
	if(!fConnectionOptionsView) {
		BRect rect;
		Addons()->FindRect(DUN_TAB_VIEW_RECT, &rect);
		fConnectionOptionsView = new ConnectionOptionsView(this, rect);
	}
	
	fDeleteView = false;
	
	fConnectionOptionsView->MoveTo(leftTop);
	fConnectionOptionsView->Reload();
	return fConnectionOptionsView;
}


ConnectionOptionsView::ConnectionOptionsView(ConnectionOptionsAddon *addon, BRect frame)
	: BView(frame, kLabelConnectionOptions, B_FOLLOW_NONE, 0),
	fAddon(addon)
{
	BRect rect = Bounds();
	rect.InsetBy(10, 10);
	rect.bottom = rect.top + 15;
	fAskBeforeConnecting = new BCheckBox(rect, "AskBeforeConnecting",
		kLabelAskBeforeConnecting, NULL);
	rect.top = rect.bottom + 5;
	rect.bottom = rect.top + 15;
	fAutoReconnect = new BCheckBox(rect, "AutoReconnect", kLabelAutoReconnect, NULL);
	
	AddChild(fAskBeforeConnecting);
	AddChild(fAutoReconnect);
}


void
ConnectionOptionsView::Reload()
{
	fAskBeforeConnecting->SetValue(Addon()->AskBeforeConnecting());
	fAutoReconnect->SetValue(Addon()->DoesAutoReconnect());
	
	if(!Addon()->Settings())
		return;
}


void
ConnectionOptionsView::AttachedToWindow()
{
	SetViewColor(Parent()->ViewColor());
}
