/*
 * Copyright 2004-2012, Haiku, Inc. All rights reserved.
 * Copyright 2002 Alexander G. M. Smith.
 * Distributed under the terms of the MIT License.
 */


/*!	SpamFilter's configuration view.  Lets the user change various settings
	related to the add-on, but not the spamdbm server.
*/


#include <stdlib.h>
#include <stdio.h>

#include <Alert.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <MailFilter.h>
#include <MailSettingsView.h>
#include <Message.h>
#include <Messenger.h>
#include <Roster.h>
#include <StringView.h>
#include <TextControl.h>
#include <FindDirectory.h>
#include <Entry.h>
#include <Path.h>

#include <FileConfigView.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SpamFilterConfig"


class SpamFilterConfig : public BMailSettingsView {
public:
								SpamFilterConfig(const BMessage* settings);

	virtual status_t			SaveInto(BMailAddOnSettings& settings) const;

private:
			BCheckBox*			fSubjectHintCheckBox;
			BCheckBox*			fAutoTrainingCheckBox;
			float				fGenuineCutoffRatio;
			BTextControl*		fGenuineCutoffRatioTextControl;
			BCheckBox*			fNoWordsMeansSpamCheckBox;
			float				fSpamCutoffRatio;
			BTextControl*		fSpamCutoffRatioTextControl;
};


SpamFilterConfig::SpamFilterConfig(const BMessage* settings)
	:
	BMailSettingsView("spamfilter_config"),
	fSubjectHintCheckBox(NULL),
	fAutoTrainingCheckBox(NULL),
	fGenuineCutoffRatioTextControl(NULL),
	fNoWordsMeansSpamCheckBox(NULL),
	fSpamCutoffRatioTextControl(NULL)
{
	bool subjectHint;
	bool autoTraining;
	bool noWordsMeansSpam;
	if (settings->FindBool("AddMarkerToSubject", &subjectHint) != B_OK)
		subjectHint = false;
	if (settings->FindBool("AutoTraining", &autoTraining) != B_OK)
		autoTraining = true;
	if (settings->FindBool("NoWordsMeansSpam", &noWordsMeansSpam) != B_OK)
		noWordsMeansSpam = true;

	if (settings->FindFloat("GenuineCutoffRatio", &fGenuineCutoffRatio) != B_OK)
		fGenuineCutoffRatio = 0.01f;
	if (settings->FindFloat("SpamCutoffRatio", &fSpamCutoffRatio) != B_OK)
		fSpamCutoffRatio = 0.99f;

	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	fSubjectHintCheckBox = new BCheckBox("AddToSubject",
		B_TRANSLATE("Add spam rating to start of subject"), NULL);
	fSubjectHintCheckBox->SetValue(subjectHint);

	BString number;
	number.SetToFormat("%06.4f", (double)fSpamCutoffRatio);
	fSpamCutoffRatioTextControl	= new BTextControl("spamcutoffratio",
		B_TRANSLATE("Spam above:"), number.String(), NULL);

	fNoWordsMeansSpamCheckBox = new BCheckBox("NoWordsMeansSpam",
		B_TRANSLATE("or empty e-mail"), NULL);
	fNoWordsMeansSpamCheckBox->SetValue(noWordsMeansSpam);

	number.SetToFormat("%08.6f", (double)fGenuineCutoffRatio);
	fGenuineCutoffRatioTextControl = new BTextControl("genuinecutoffratio",
		B_TRANSLATE("Genuine below and uncertain above:"),
		number.String(), NULL);

	fAutoTrainingCheckBox = new BCheckBox("autoTraining",
		B_TRANSLATE("Learn from all incoming e-mail"), NULL);
	fAutoTrainingCheckBox->SetValue(autoTraining);

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(fSubjectHintCheckBox)
		.AddGroup(B_HORIZONTAL)
			.Add(fSpamCutoffRatioTextControl->CreateLabelLayoutItem())
			.Add(fSpamCutoffRatioTextControl->CreateTextViewLayoutItem())
			.Add(fNoWordsMeansSpamCheckBox)
		.End()
		.AddGroup(B_HORIZONTAL)
			.Add(fGenuineCutoffRatioTextControl->CreateLabelLayoutItem())
			.Add(fGenuineCutoffRatioTextControl->CreateTextViewLayoutItem())
		.End()
		.Add(fAutoTrainingCheckBox);
}


status_t
SpamFilterConfig::SaveInto(BMailAddOnSettings& settings) const
{
	settings.MakeEmpty();

	status_t status = settings.AddBool("AddMarkerToSubject",
		fSubjectHintCheckBox->Value() == B_CONTROL_ON);

	if (status == B_OK) {
		status = settings.AddBool("AutoTraining",
			fAutoTrainingCheckBox->Value() == B_CONTROL_ON);
	}
	if (status == B_OK) {
		status = settings.AddBool("NoWordsMeansSpam",
			fNoWordsMeansSpamCheckBox->Value() == B_CONTROL_ON);
	}
	if (status == B_OK) {
		status = settings.AddFloat("GenuineCutoffRatio",
			atof(fGenuineCutoffRatioTextControl->Text()));
	}
	if (status == B_OK) {
		status = settings.AddFloat("SpamCutoffRatio",
			atof(fSpamCutoffRatioTextControl->Text()));
	}

	return status;
}


// #pragma mark -


BMailSettingsView*
instantiate_filter_settings_view(const BMailAccountSettings& accountSettings,
	const BMailAddOnSettings& settings)
{
	return new SpamFilterConfig(&settings);
}
