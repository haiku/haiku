/*
 * Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
 */


//!	Config views for the account, protocols, and filters.


#include "ConfigViews.h"
#include "CenterContainer.h"

#include <TextControl.h>
#include <ListView.h>
#include <PopUpMenu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Button.h>
#include <Looper.h>
#include <Path.h>
#include <Alert.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Directory.h>
#include <Catalog.h>
#include <Locale.h>

#include <string.h>

#include <MailSettings.h>

#include "FilterConfigView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Config Views"


// AccountConfigView
const uint32 kMsgAccountNameChanged = 'anmc';

// ProtocolsConfigView
const uint32 kMsgProtocolChanged = 'prch';


BView*
CreateConfigView(entry_ref addon, MailAddonSettings& settings,
	BMailAccountSettings& accountSettings, image_id* image)
{
	BView* (*instantiate_config)(MailAddonSettings& settings,
		BMailAccountSettings& accountSettings);
	BPath path(&addon);
	*image = load_add_on(path.Path());
	if (image < 0)
		return NULL;

	if (get_image_symbol(*image, "instantiate_config_panel", B_SYMBOL_TYPE_TEXT,
		(void **)&instantiate_config) != B_OK) {
		unload_add_on(*image);
		*image = -1;
		return NULL;
	}

	BView* view = (*instantiate_config)(settings, accountSettings);
	return view;
}


AccountConfigView::AccountConfigView(BRect rect, BMailAccountSettings* account)
	:
	BBox(rect),
	fAccount(account)
{
	SetLabel(B_TRANSLATE("Account settings"));

	rect = Bounds().InsetByCopy(8, 8);
	rect.top += 10;
	CenterContainer *view = new CenterContainer(rect, false);
	view->SetSpacing(5);

	// determine font height
	font_height fontHeight;
	view->GetFontHeight(&fontHeight);
	int32 height = (int32)(fontHeight.ascent + fontHeight.descent
		+ fontHeight.leading) + 5;

	rect = view->Bounds();
	rect.bottom = height + 5;

	float labelWidth = view->StringWidth(B_TRANSLATE("Account name:")) + 6;

	view->AddChild(fNameControl = new BTextControl(rect, NULL,
		B_TRANSLATE("Account name:"), NULL,
		new BMessage(kMsgAccountNameChanged)));
	fNameControl->SetDivider(labelWidth);
	view->AddChild(fRealNameControl = new BTextControl(rect, NULL,
		B_TRANSLATE("Real name:"), NULL, NULL));
	fRealNameControl->SetDivider(labelWidth);
	view->AddChild(fReturnAddressControl = new BTextControl(rect, NULL,
		B_TRANSLATE("Return address:"), NULL, NULL));
	fReturnAddressControl->SetDivider(labelWidth);
//			control->TextView()->HideTyping(true);

	float w, h;
	view->GetPreferredSize(&w, &h);
	ResizeTo(w + 15, h + 22);
	view->ResizeTo(w, h);

	AddChild(view);
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


//	#pragma mark -


InProtocolsConfigView::InProtocolsConfigView(BMailAccountSettings* account)
	:
	BBox(BRect(0, 0, 100, 100)),
	fAccount(account),
	fConfigView(NULL)
{
	BString label = "Can't find protocol.";
	entry_ref protocol = fAccount->InboundPath();
	MailAddonSettings& inboundSettings = fAccount->InboundSettings();

	fConfigView = CreateConfigView(protocol, inboundSettings, *account,
		&fImageID);

	if (fConfigView) {
		float w = fConfigView->Bounds().Width();
		float h = fConfigView->Bounds().Height();
		fConfigView->MoveTo(3, 13);
		ResizeTo(w + 6, h + 16);
		AddChild(fConfigView);

		fConfigView->MoveTo(3, 21);
		ResizeBy(0, 8);
		if (CenterContainer *container
			= dynamic_cast<CenterContainer *>(Parent())) {
			container->Layout();
		}
		label = protocol.name;

		fConfigView->MoveTo(3, 21);
		ResizeBy(0, 8);
	}
	SetLabel(label);
}


void
InProtocolsConfigView::DetachedFromWindow()
{
	if (fConfigView == NULL)
		return;
	BMessage settings;
	if (fConfigView->Archive(&settings) != B_OK)
		return;
	fAccount->InboundSettings().EditSettings() = settings;

	RemoveChild(fConfigView);
	delete fConfigView;
	fConfigView = NULL;
	unload_add_on(fImageID);
}


OutProtocolsConfigView::OutProtocolsConfigView(BMailAccountSettings* account)
	:
	BBox(BRect(0, 0, 100, 100)),
	fAccount(account),
	fConfigView(NULL)
{
	BString label = "Can't find protocol.";
	entry_ref protocol = fAccount->OutboundPath();
	MailAddonSettings& outboundSettings = fAccount->OutboundSettings();
	fConfigView = CreateConfigView(protocol, outboundSettings, *account,
		&fImageID);

	if (fConfigView) {
		float w = fConfigView->Bounds().Width();
		float h = fConfigView->Bounds().Height();
		fConfigView->MoveTo(3, 13);
		ResizeTo(w + 6, h + 16);
		AddChild(fConfigView);

		fConfigView->MoveTo(3, 21);
		ResizeBy(0, 8);
		if (CenterContainer *container
			= dynamic_cast<CenterContainer *>(Parent())) {
			container->Layout();
		}
		label = protocol.name;

		fConfigView->MoveTo(3, 21);
		ResizeBy(0, 8);
	}

	SetLabel(label);
}


void
OutProtocolsConfigView::DetachedFromWindow()
{
	if (fConfigView == NULL)
		return;
	BMessage settings;
	if (fConfigView->Archive(&settings) != B_OK)
		return;
	fAccount->OutboundSettings().EditSettings() = settings;

	RemoveChild(fConfigView);
	delete fConfigView;
	fConfigView = NULL;
	unload_add_on(fImageID);
}
