/*
 * Copyright 2007-2012, Haiku, Inc. All rights reserved.
 * Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


//!	Config views for the account, protocols, and filters.


#include "ConfigViews.h"

#include <Alert.h>
#include <Button.h>
#include <Catalog.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <ListView.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Looper.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <TextControl.h>

#include <string.h>

#include <MailSettings.h>

#include "FilterConfigView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Config Views"


// AccountConfigView
const uint32 kMsgAccountNameChanged = 'anmc';

// ProtocolsConfigView
const uint32 kMsgProtocolChanged = 'prch';


// #pragma mark -


AccountConfigView::AccountConfigView(BMailAccountSettings* account)
	:
	BBox("account"),
	fAccount(account)
{
	SetLabel(B_TRANSLATE("Account settings"));

	fNameControl = new BTextControl(NULL, B_TRANSLATE("Account name:"), NULL,
		new BMessage(kMsgAccountNameChanged));
	fRealNameControl = new BTextControl(NULL, B_TRANSLATE("Real name:"), NULL,
		NULL);
	fReturnAddressControl = new BTextControl(NULL,
		B_TRANSLATE("Return address:"), NULL, NULL);

	BView* contents = new BView(NULL, 0);
	AddChild(contents);

	BLayoutBuilder::Grid<>(contents, 0.f)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(fNameControl->CreateLabelLayoutItem(), 0, 0)
		.Add(fNameControl->CreateTextViewLayoutItem(), 1, 0)
		.Add(fRealNameControl->CreateLabelLayoutItem(), 0, 1)
		.Add(fRealNameControl->CreateTextViewLayoutItem(), 1, 1)
		.Add(fReturnAddressControl->CreateLabelLayoutItem(), 0, 2)
		.Add(fReturnAddressControl->CreateTextViewLayoutItem(), 1, 2)
		.AddGlue(0, 3);
}


void
AccountConfigView::DetachedFromWindow()
{
	fAccount->SetName(fNameControl->Text());
	fAccount->SetRealName(fRealNameControl->Text());
	fAccount->SetReturnAddress(fReturnAddressControl->Text());
}


void
AccountConfigView::AttachedToWindow()
{
	UpdateViews();
	fNameControl->SetTarget(this);
}


void
AccountConfigView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case kMsgAccountNameChanged:
			fAccount->SetName(fNameControl->Text());
			break;

		default:
			BView::MessageReceived(msg);
	}
}


void
AccountConfigView::UpdateViews()
{
	fNameControl->SetText(fAccount->Name());
	fRealNameControl->SetText(fAccount->RealName());
	fReturnAddressControl->SetText(fAccount->ReturnAddress());
}


// #pragma mark -


ProtocolSettingsView::ProtocolSettingsView(const entry_ref& ref,
	const BMailAccountSettings& accountSettings,
	BMailProtocolSettings& settings)
	:
	BBox("protocol"),
	fSettings(settings),
	fSettingsView(NULL)
{
	status_t status = _CreateSettingsView(ref, accountSettings, settings);
	BView* view = fSettingsView;

	if (status == B_OK) {
		SetLabel(ref.name);
	} else {
		BString text(B_TRANSLATE("An error occurred while creating the "
			"config view: %error."));
		text.ReplaceAll("%error", strerror(status));
		view = new BStringView("error", text.String());

		SetLabel(B_TRANSLATE("Error!"));
	}

	BView* contents = new BView(NULL, 0);
	AddChild(contents);

	BLayoutBuilder::Group<>(contents, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(view)
		.AddGlue();
}


void
ProtocolSettingsView::DetachedFromWindow()
{
	if (fSettingsView == NULL)
		return;

	if (fSettingsView->SaveInto(fSettings) != B_OK)
		return;

	// We need to remove the settings view before unloading its add-on
	fSettingsView->RemoveSelf();
	delete fSettingsView;
	fSettingsView = NULL;
	unload_add_on(fImage);
}


status_t
ProtocolSettingsView::_CreateSettingsView(const entry_ref& ref,
	const BMailAccountSettings& accountSettings,
	BMailProtocolSettings& settings)
{
	BMailSettingsView* (*instantiateConfig)(
		const BMailAccountSettings& accountSettings,
		BMailProtocolSettings& settings);
	BPath path(&ref);
	image_id image = load_add_on(path.Path());
	if (image < 0)
		return image;

	if (get_image_symbol(image, "instantiate_protocol_settings_view",
			B_SYMBOL_TYPE_TEXT, (void**)&instantiateConfig) != B_OK) {
		unload_add_on(image);
		return B_MISSING_SYMBOL;
	}

	fImage = image;
	fSettingsView = instantiateConfig(accountSettings, settings);
	return B_OK;
}
