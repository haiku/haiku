/*
 * Copyright 2003-2004 Waldemar Kornewald. All rights reserved.
 * Copyright 2017 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

//-----------------------------------------------------------------------
// ConnectionOptionsAddon saves the loaded settings.
// ConnectionOptionsView saves the current settings.
//-----------------------------------------------------------------------

#include "ConnectionOptionsAddon.h"

#include "MessageDriverSettingsUtils.h"

#include <LayoutBuilder.h>

#include <PPPDefs.h>
#include <settings_tools.h>


// message constants
static const uint32 kMsgUpdateControls = 'UCTL';

// labels
static const char *kLabelConnectionOptions = "Options";
static const char *kLabelDialOnDemand = "Connect Automatically When Needed";
static const char *kLabelAskBeforeDialing = "Ask Before Dialing";
static const char *kLabelAutoRedial = "Redial Automatically";


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
	/* FIXME!
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
	*/

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
		// parameter.AddString(MDSU_NAME, PPP_DIAL_ON_DEMAND_KEY); // FIXME
		parameter.AddString(MDSU_VALUES, "enabled");
		settings->AddMessage(MDSU_PARAMETERS, &parameter);

		if(fConnectionOptionsView->AskBeforeDialing()) {
			parameter.MakeEmpty();
			// parameter.AddString(MDSU_NAME, PPP_ASK_BEFORE_DIALING_KEY); // FIXME
			parameter.AddString(MDSU_VALUES, "enabled");
			settings->AddMessage(MDSU_PARAMETERS, &parameter);
		}
	}

	if(fConnectionOptionsView->DoesAutoRedial()) {
		parameter.MakeEmpty();
		// parameter.AddString(MDSU_NAME, PPP_AUTO_REDIAL_KEY); // FIXME
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
ConnectionOptionsAddon::CreateView()
{
	if(!fConnectionOptionsView) {
		fConnectionOptionsView = new ConnectionOptionsView(this);
		fConnectionOptionsView->Reload();
	}

	return fConnectionOptionsView;
}


ConnectionOptionsView::ConnectionOptionsView(ConnectionOptionsAddon *addon)
	: BView(kLabelConnectionOptions, 0),
	fAddon(addon)
{
	fDialOnDemand = new BCheckBox("DialOnDemand", kLabelDialOnDemand,
		new BMessage(kMsgUpdateControls));
	fAskBeforeDialing = new BCheckBox("AskBeforeDialing", kLabelAskBeforeDialing,
		NULL);
	fAutoRedial = new BCheckBox("AutoRedial", kLabelAutoRedial, NULL);

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_HALF_ITEM_SPACING)
		.SetInsets(B_USE_HALF_ITEM_INSETS)
		.Add(fDialOnDemand)
		.Add(fAskBeforeDialing)
		.Add(fAutoRedial)
		.AddGlue()
	.End();
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
