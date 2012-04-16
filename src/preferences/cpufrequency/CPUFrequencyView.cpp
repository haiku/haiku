/*
 * Copyright 2009-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */


#include "CPUFrequencyView.h"

#include "StatusView.h"

#include <Box.h>
#include <Catalog.h>
#include <Deskbar.h>
#include <GroupView.h>
#include <Locale.h>
#include <MenuField.h>
#include <SpaceLayoutItem.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "CPU Frequency View"


const char* kCPUFreqPreferencesFile = "CPUFrequency";
const char* kPrefSignature = "application/x-vnd.Haiku-CPUFrequencyPref";
const char* kPreferencesFileName = "CPUFrequency";

const uint32 kInstallIntoDeskbar = '&iid';
const uint32 kIntegrationTimeChanged = '&itc';

const bigtime_t	kMilliSecond = 1000;


CPUFrequencyView::CPUFrequencyView(BRect frame,
	PreferencesStorage<freq_preferences>* storage)
	:
	BView(frame, "CPUFrequencyView", B_FOLLOW_NONE, B_WILL_DRAW),
		fStorage(storage)
{
	BGroupLayout* mainLayout = new BGroupLayout(B_VERTICAL);
	SetLayout(mainLayout);
	mainLayout->SetSpacing(10);
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	// stepping policy
	BRect rect = Bounds();
	rect.InsetBy(5, 5);
	BBox *policyBox = new BBox(rect, "policyBox");
	policyBox->SetLabel(B_TRANSLATE("Stepping policy"));
	BGroupLayout* policyLayout = new BGroupLayout(B_VERTICAL);
	policyLayout->SetInsets(10, policyBox->TopBorderOffset() * 2 + 10, 10, 10);
	policyLayout->SetSpacing(10);
	policyBox->SetLayout(policyLayout);
	mainLayout->AddView(policyBox);

	fPolicyMenu = new BMenu(B_TRANSLATE("Stepping policy: "));
	BMenuField *menuField = new BMenuField("", fPolicyMenu);
	policyLayout->AddView(menuField);

	// dynamic stepping
	BBox *dynamicBox = new BBox(rect, "dynamicBox");
	dynamicBox->SetLabel(B_TRANSLATE("Dynamic stepping"));
	BGroupLayout* dynamicLayout = new BGroupLayout(B_VERTICAL);
	dynamicLayout->SetInsets(10, dynamicBox->TopBorderOffset() * 2 + 10, 10,
		10);
	dynamicLayout->SetSpacing(10);
	dynamicBox->SetLayout(dynamicLayout);
	mainLayout->AddView(dynamicBox);

	fColorStepView = new ColorStepView(frame);
	fColorStepView->SetFrequencys(fDriverInterface.GetCpuFrequencyStates());

	fIntegrationTime = new BTextControl(BRect(0,0,Bounds().Width(),10),
		"intergal", B_TRANSLATE("Integration time [ms]: "), "",
		new BMessage(kIntegrationTimeChanged));

	dynamicLayout->AddView(fColorStepView);
	dynamicLayout->AddView(fIntegrationTime);

	// status view
	BBox *statusBox = new BBox(rect, "statusBox");
	statusBox->SetLabel(B_TRANSLATE("CPU frequency status view"));
	BGroupLayout* statusLayout = new BGroupLayout(B_HORIZONTAL);
	statusLayout->SetInsets(10, statusBox->TopBorderOffset() * 2 + 10, 10, 10);
	statusLayout->SetSpacing(10);
	statusBox->SetLayout(statusLayout);
	mainLayout->AddView(statusBox);

	fStatusView = new StatusView(BRect(0, 0, 5, 5), false, fStorage);
	fStatusView->ShowPopUpMenu(false);

	fInstallButton = new BButton("installButton",
		B_TRANSLATE("Install replicant into Deskbar"),
		new BMessage(kInstallIntoDeskbar));

	statusLayout->AddView(fStatusView);
	statusLayout->AddView(fInstallButton);
	statusLayout->AddItem(BSpaceLayoutItem::CreateGlue());
}


void
CPUFrequencyView::MessageReceived(BMessage* message)
{
	freq_preferences* pref = fStorage->GetPreferences();
	bool configChanged = false;

	switch (message->what) {
		case kUpdatedPreferences:
			fStatusView->MessageReceived(message);
			if (pref->mode == DYNAMIC) {
				fColorStepView->SetEnabled(true);
				fIntegrationTime->SetEnabled(true);
			}
			else {
				fColorStepView->SetEnabled(false);
				fIntegrationTime->SetEnabled(false);
			}
			configChanged = true;
			break;

		case kSteppingChanged:
			// from ColorStepView
			pref->stepping_threshold = fColorStepView->GetSliderPosition();
			fStorage->SavePreferences();
			configChanged = true;
			break;

		case kIntegrationTimeChanged:
			_ReadIntegrationTime();
			fStorage->SavePreferences();
			configChanged = true;
			break;

		case kInstallIntoDeskbar:
			_InstallReplicantInDeskbar();
			break;

		case kRevertMsg:
		case kDefaultMsg:
			fStatusView->UpdateCPUFreqState();
			_UpdateViews();
			break;
		default:
			BView::MessageReceived(message);
	}

	if (configChanged)
		Window()->PostMessage(kConfigChangedMsg);
}


void
CPUFrequencyView::AttachedToWindow()
{
	fFrequencyMenu = new FrequencyMenu(fPolicyMenu, this, fStorage,
		&fDriverInterface);
	AddFilter(fFrequencyMenu);

	fColorStepView->SetTarget(this);
	fIntegrationTime->SetTarget(this);
	fInstallButton->SetTarget(this);

	_UpdateViews();
}


void
CPUFrequencyView::DetachedFromWindow()
{
	// emty menu for the case the view is attached again
	while (true) {
		BMenuItem* item = fPolicyMenu->RemoveItem(int32(0));
		if (!item)
			break;
		delete item;
	}
	if (RemoveFilter(fFrequencyMenu))
		delete fFrequencyMenu;

	_ReadIntegrationTime();
}


status_t
our_image(image_info& image)
{
	int32 cookie = 0;
	while (get_next_image_info(B_CURRENT_TEAM, &cookie, &image) == B_OK) {
		if ((char *)our_image >= (char *)image.text
			&& (char *)our_image <= (char *)image.text + image.text_size)
			return B_OK;
	}

	return B_ERROR;
};


void
CPUFrequencyView::_InstallReplicantInDeskbar()
{
	image_info info;
	entry_ref ref;
	if (our_image(info) == B_OK
		&& get_ref_for_path(info.name, &ref) == B_OK) {
		BDeskbar deskbar;
		deskbar.AddItem(&ref);
	}
}


void
CPUFrequencyView::_UpdateViews()
{
	fFrequencyMenu->UpdateMenu();
	freq_preferences* pref = fStorage->GetPreferences();
	fColorStepView->SetSliderPosition(pref->stepping_threshold);
	if (pref->mode == DYNAMIC) {
		fColorStepView->SetEnabled(true);
		fIntegrationTime->SetEnabled(true);
	}
	else {
		fColorStepView->SetEnabled(false);
		fIntegrationTime->SetEnabled(false);
	}

	BString out;
	out << pref->integration_time / kMilliSecond;
	fIntegrationTime->SetText(out.String());
}


void
CPUFrequencyView::_ReadIntegrationTime()
{
	freq_preferences* pref = fStorage->GetPreferences();
	bigtime_t integration_time = atoi(fIntegrationTime->Text()) * kMilliSecond;
	if (integration_time == 0) {
		BString out;
		out << pref->integration_time / kMilliSecond;
		fIntegrationTime->SetText(out.String());
	}
	else {
		pref->integration_time = integration_time;
	}
}
