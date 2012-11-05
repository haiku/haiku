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


ProtocolConfigView::ProtocolConfigView(BMailAccountSettings& accountSettings,
	const entry_ref& ref, BMailProtocolSettings& settings)
	:
	BBox("protocol"),
	fSettings(settings),
	fConfigView(NULL)
{
	status_t status = _CreateConfigView(ref, settings, accountSettings);
	BView* view = fConfigView;

	if (fConfigView != NULL) {
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
		.Add(view);
}


void
ProtocolConfigView::DetachedFromWindow()
{
	if (fConfigView == NULL)
		return;
	BMessage settings;
	if (fConfigView->Archive(&settings) != B_OK)
		return;

	fSettings.MakeEmpty();
	fSettings.Append(settings);

	RemoveChild(fConfigView);
	delete fConfigView;
	fConfigView = NULL;
	unload_add_on(fImage);
}


status_t
ProtocolConfigView::_CreateConfigView(const entry_ref& ref,
	BMailProtocolSettings& settings, BMailAccountSettings& accountSettings)
{
	BView* (*instantiateConfig)(BMailProtocolSettings& settings,
		BMailAccountSettings& accountSettings);
	BPath path(&ref);
	image_id image = load_add_on(path.Path());
	if (image < 0)
		return image;

	if (get_image_symbol(image, "instantiate_protocol_config_panel",
			B_SYMBOL_TYPE_TEXT, (void**)&instantiateConfig) != B_OK) {
		unload_add_on(image);
		return B_BAD_VALUE;
	}

	fImage = image;
	fConfigView = instantiateConfig(settings, accountSettings);
	return B_OK;
}
