/*
 * Copyright 2016-2017, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#include "NetworkConnectionConfigView.h"

#include <stdio.h>
#include <stdlib.h>

#include <LayoutBuilder.h>
#include <MenuField.h>
#include <TextControl.h>

#include "Settings.h"
#include "SettingsDescription.h"
#include "TargetHostInterfaceInfo.h"


enum {
	MSG_NET_CONFIG_INPUT_CHANGED = 'ncic'
};


static const char* kHostSetting = "hostname";
static const char* kPortSetting = "port";


NetworkConnectionConfigView::NetworkConnectionConfigView()
	:
	ConnectionConfigView("NetworkConnectionConfig"),
	fProtocolField(NULL),
	fHostInput(NULL),
	fPortInput(NULL),
	fSettings(NULL),
	fHostSetting(NULL),
	fPortSetting(NULL)
{
}


NetworkConnectionConfigView::~NetworkConnectionConfigView()
{
	if (fSettings != NULL)
		fSettings->ReleaseReference();
}


void
NetworkConnectionConfigView::AttachedToWindow()
{
	ConnectionConfigView::AttachedToWindow();

	fHostInput->SetTarget(this);
	fPortInput->SetTarget(this);
}


void
NetworkConnectionConfigView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_NET_CONFIG_INPUT_CHANGED:
		{
			fSettings->SetValue(fHostSetting, fHostInput->Text());
			uint16 port = (uint16)strtoul(fPortInput->Text(), NULL, 10);
			fSettings->SetValue(fPortSetting, port);
			NotifyConfigurationChanged(fSettings);
			break;
		}
		default:
		{
			ConnectionConfigView::MessageReceived(message);
			break;
		}
	}
}


status_t
NetworkConnectionConfigView::InitSpecific()
{
	SettingsDescription* description
		= InterfaceInfo()->GetSettingsDescription();

	for (int32 i = 0; i < description->CountSettings(); i++) {
		Setting* setting = description->SettingAt(i);
		if (strcmp(setting->ID(), kPortSetting) == 0)
			fPortSetting = setting;
		else if (strcmp(setting->ID(), kHostSetting) == 0)
			fHostSetting = setting;
	}

	if (fPortSetting == NULL || fHostSetting == NULL)
		return B_BAD_VALUE;

	fSettings = new Settings(description);
	fPortInput = new BTextControl("port_input", "Port:", "",
					new BMessage(MSG_NET_CONFIG_INPUT_CHANGED));

	BLayoutItem* textLayoutItem;
	BLayoutBuilder::Group<>(this, B_HORIZONTAL)
		.Add((fHostInput = new BTextControl("host_input", "Host:", "",
				new BMessage(MSG_NET_CONFIG_INPUT_CHANGED))))
		.Add(fPortInput->CreateLabelLayoutItem())
		.Add((textLayoutItem = fPortInput->CreateTextViewLayoutItem()))
	.End();

	// since port numbers are limited to 5 digits, there's no need for
	// the port input to expand farther than that. Instead, let the
	// host field take the extra space.
	textLayoutItem->SetExplicitMaxSize(BSize(
			be_plain_font->StringWidth("999999"), B_SIZE_UNSET));


	BString buffer;
	buffer.SetToFormat("%" B_PRIu16, fPortSetting->DefaultValue().ToUInt16());
	fPortInput->SetText(buffer);

	fHostInput->SetModificationMessage(
		new BMessage(MSG_NET_CONFIG_INPUT_CHANGED));
	fPortInput->SetModificationMessage(
		new BMessage(MSG_NET_CONFIG_INPUT_CHANGED));

	return B_OK;
}


