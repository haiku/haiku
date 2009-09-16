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
#ifdef LANG_GERMAN
static const char *kLabelConnectionOptions = "Optionen";
static const char *kLabelDialOnDemand = "Automatisch Verbinden Bei Zugriff Auf "
										"Internet";
static const char *kLabelAskBeforeDialing = "Vor Dem Verbinden Fragen";
static const char *kLabelAutoRedial = "Verbindung Automatisch Wiederherstellen";
#else
static const char *kLabelConnectionOptions = "Options";
static const char *kLabelDialOnDemand = "Connect Automatically When Needed";
static const char *kLabelAskBeforeDialing = "Ask Before Dialing";
static const char *kLabelAutoRedial = "Redial Automatically";
#endif


ConnectionOptionsAddon::ConnectionOptionsAddon(BMessage *addons)
	: DialUpAddon(addons),
	fSettings(NULL),
	fProfile(NULL),
	fConnectionOptionsView(NULL)
{
}


ConnectionOptionsAddon::~ConnectionOptionsAddon()
{
}


bool
ConnectionOptionsAddon::LoadSettings(BMessage *settings, BMessage *profile, bool isNew)
{
	fIsNew = isNew;
	fDoesDialOnDemand = fAskBeforeDialing = fDoesAutoRedial = false;
	fSettings = settings;
	fProfile = profile;
	
	if(fConnectionOptionsView)
		fConnectionOptionsView->Reload();
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
	
	if(fConnectionOptionsView)
		fConnectionOptionsView->Reload();
			// reload new settings
	
	return true;
}


void
ConnectionOptionsAddon::IsModified(bool *settings, bool *profile) const
{
	*settings = *profile = false;
	
	if(!fSettings || !fConnectionOptionsView)
		return;
	
	*settings = DoesDialOnDemand() != fConnectionOptionsView->DoesDialOnDemand()
		|| AskBeforeDialing() != fConnectionOptionsView->AskBeforeDialing()
		|| DoesAutoRedial() != fConnectionOptionsView->DoesAutoRedial();
}


bool
ConnectionOptionsAddon::SaveSettings(BMessage *settings, BMessage *profile, bool saveTemporary)
{
	if(!fSettings || !settings)
		return false;
	
	BMessage parameter;
	if(fConnectionOptionsView->DoesDialOnDemand()) {
		parameter.MakeEmpty();
		parameter.AddString(MDSU_NAME, PPP_DIAL_ON_DEMAND_KEY);
		parameter.AddString(MDSU_VALUES, "enabled");
		settings->AddMessage(MDSU_PARAMETERS, &parameter);
		
		if(fConnectionOptionsView->AskBeforeDialing()) {
			parameter.MakeEmpty();
			parameter.AddString(MDSU_NAME, PPP_ASK_BEFORE_DIALING_KEY);
			parameter.AddString(MDSU_VALUES, "enabled");
			settings->AddMessage(MDSU_PARAMETERS, &parameter);
		}
	}
	
	if(fConnectionOptionsView->DoesAutoRedial()) {
		parameter.MakeEmpty();
		parameter.AddString(MDSU_NAME, PPP_AUTO_REDIAL_KEY);
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
		fConnectionOptionsView->Reload();
	}
	
	fConnectionOptionsView->MoveTo(leftTop);
	return fConnectionOptionsView;
}


ConnectionOptionsView::ConnectionOptionsView(ConnectionOptionsAddon *addon, BRect frame)
	: BView(frame, kLabelConnectionOptions, B_FOLLOW_NONE, 0),
	fAddon(addon)
{
	BRect rect = Bounds();
	rect.InsetBy(10, 10);
	rect.bottom = rect.top + 15;
	fDialOnDemand = new BCheckBox(rect, "DialOnDemand", kLabelDialOnDemand,
		new BMessage(kMsgUpdateControls));
	rect.top = rect.bottom + 3;
	rect.bottom = rect.top + 15;
	rect.left += 20;
	fAskBeforeDialing = new BCheckBox(rect, "AskBeforeDialing", kLabelAskBeforeDialing,
		NULL);
	rect.left -= 20;
	rect.top = rect.bottom + 20;
	rect.bottom = rect.top + 15;
	fAutoRedial = new BCheckBox(rect, "AutoRedial", kLabelAutoRedial, NULL);
	
	AddChild(fDialOnDemand);
	AddChild(fAskBeforeDialing);
	AddChild(fAutoRedial);
}


void
ConnectionOptionsView::Reload()
{
	fDialOnDemand->SetValue(Addon()->DoesDialOnDemand() || Addon()->IsNew());
		// this is enabled by default
	fAskBeforeDialing->SetValue(Addon()->AskBeforeDialing());
	fAutoRedial->SetValue(Addon()->DoesAutoRedial());
	
	if(!Addon()->Settings())
		return;
	
	UpdateControls();
}


void
ConnectionOptionsView::AttachedToWindow()
{
	SetViewColor(Parent()->ViewColor());
	fDialOnDemand->SetTarget(this);
}


void
ConnectionOptionsView::MessageReceived(BMessage *message)
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
ConnectionOptionsView::UpdateControls()
{
	fAskBeforeDialing->SetEnabled(fDialOnDemand->Value());
	fAskBeforeDialing->SetValue(fDialOnDemand->Value());
}
