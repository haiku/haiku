/*
 * Copyright 2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, <axeld@pinc-software.de>
 */


#include "ServiceView.h"

#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <StringView.h>
#include <TextView.h>

#include <NetServer.h>


static const uint32 kMsgToggleService = 'tgls';


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ServiceView"


ServiceView::ServiceView(const char* name, const char* executable,
	const char* title, const char* description, BNetworkSettings& settings)
	:
	BView("service", 0),
	fName(name),
	fExecutable(executable),
	fSettings(settings)
{
	BStringView* titleView = new BStringView("service", title);
	titleView->SetFont(be_bold_font);
	titleView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	BTextView* descriptionView = new BTextView("description");
	descriptionView->SetText(description);
	descriptionView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	descriptionView->MakeEditable(false);

	fEnableButton = new BButton("toggler", B_TRANSLATE("Enable"),
		new BMessage(kMsgToggleService));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(titleView)
		.Add(descriptionView)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(fEnableButton);

	SetExplicitMinSize(BSize(200, B_SIZE_UNSET));
	_UpdateEnableButton();
}


ServiceView::~ServiceView()
{
}


void
ServiceView::SettingsUpdated(uint32 which)
{
	if (which == BNetworkSettings::kMsgServiceSettingsUpdated)
		_UpdateEnableButton();
}


void
ServiceView::AttachedToWindow()
{
	fEnableButton->SetTarget(this);
}


void
ServiceView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgToggleService:
			if (IsEnabled())
				Disable();
			else
				Enable();

			_UpdateEnableButton();
			break;
		default:
			BView::MessageReceived(message);
			break;
	}
}


bool
ServiceView::IsEnabled() const
{
	BMessage request(kMsgIsServiceRunning);
	request.AddString("name", fName);

	BMessenger networkServer(kNetServerSignature);
	BMessage reply;
	status_t status = networkServer.SendMessage(&request, &reply);
	if (status == B_OK)
		return reply.GetBool("running");

	return false;
}


void
ServiceView::Enable()
{
	BNetworkServiceSettings settings;
	settings.SetName(fName);
	settings.AddArgument(fExecutable);

	BMessage service;
	if (settings.GetMessage(service) == B_OK)
		fSettings.AddService(service);
}


void
ServiceView::Disable()
{
	fSettings.RemoveService(fName);
}


void
ServiceView::_UpdateEnableButton()
{
	fEnableButton->SetLabel(IsEnabled()
		? B_TRANSLATE("Disable") : B_TRANSLATE("Enable"));
}
