/*
 * Copyright 2011-2012, Haiku Inc. All Rights Reserved.
 * Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


//! The standard config view for all protocols.


#include "ProtocolConfigView.h"

#include <stdio.h>
#include <stdlib.h>

#include <Catalog.h>
#include <CheckBox.h>
#include <GridLayout.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <String.h>
#include <TextControl.h>

#include <crypt.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProtocolConfigView"


static const char* kPartialDownloadLimit = "partial_download_limit";

static const uint32 kMsgLeaveOnServer = 'lmos';
static const uint32 kMsgNoPassword = 'none';
static const uint32 kMsgNeedPassword = 'some';


namespace BPrivate {


BodyDownloadConfig::BodyDownloadConfig()
	:
	BView(BRect(0,0,50,50), "body_config", B_FOLLOW_ALL_SIDES, 0)
{
	const char *partial_text = B_TRANSLATE(
		"Partially download messages larger than");

	BRect r(0, 0, 280, 15);
	fPartialBox = new BCheckBox(r, "size_if", partial_text,
		new BMessage('SIZF'));
	fPartialBox->ResizeToPreferred();

	r = fPartialBox->Frame();
	r.OffsetBy(17,r.Height() + 1);
	r.right = r.left + be_plain_font->StringWidth("0000") + 10;
	fSizeBox = new BTextControl(r, "size", "", "", NULL);

	r.OffsetBy(r.Width() + 5,0);
	fBytesLabel = new BStringView(r, "kb", B_TRANSLATE("KiB"));
	AddChild(fBytesLabel);
	fSizeBox->SetDivider(0);

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(fPartialBox);
	AddChild(fSizeBox);
	ResizeToPreferred();
}


void
BodyDownloadConfig::SetTo(BMailProtocolSettings& settings)
{
	int32 limit = 0;
	if (settings.HasInt32(kPartialDownloadLimit))
		limit = settings.FindInt32(kPartialDownloadLimit);
	if (limit < 0) {
		fPartialBox->SetValue(B_CONTROL_OFF);
		fSizeBox->SetText("0");
		fSizeBox->SetEnabled(false);
	} else {
		limit = int32(limit / 1024);
		BString kb;
		kb << limit;
		fSizeBox->SetText(kb);
		fPartialBox->SetValue(B_CONTROL_ON);
		fSizeBox->SetEnabled(true);
	}
}


void
BodyDownloadConfig::MessageReceived(BMessage *msg)
{
	if (msg->what != 'SIZF')
		return BView::MessageReceived(msg);
	fSizeBox->SetEnabled(fPartialBox->Value());
}


void
BodyDownloadConfig::AttachedToWindow()
{
	fPartialBox->SetTarget(this);
	fPartialBox->ResizeToPreferred();
}


void
BodyDownloadConfig::GetPreferredSize(float *width, float *height)
{
	*height = fSizeBox->Frame().bottom + 5;
	*width = 200;
}


status_t
BodyDownloadConfig::Archive(BMessage* into, bool) const
{
	into->RemoveName(kPartialDownloadLimit);
	if (fPartialBox->Value() == B_CONTROL_ON)
		into->AddInt32(kPartialDownloadLimit, atoi(fSizeBox->Text()) * 1024);
	else
		into->AddInt32(kPartialDownloadLimit, -1);

	return B_OK;
}


// #pragma mark -


MailProtocolConfigView::MailProtocolConfigView(uint32 optionsMask)
	:
	BView("protocol_config_view", B_WILL_DRAW),
	fHostControl(NULL),
	fUserControl(NULL),
	fPasswordControl(NULL),
	fLeaveOnServerCheckBox(NULL),
	fRemoveFromServerCheckBox(NULL),
	fBodyDownloadConfig(NULL)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BGridLayout* layout = new BGridLayout(0.f);
	SetLayout(layout);

	if ((optionsMask & B_MAIL_PROTOCOL_HAS_HOSTNAME) != 0) {
		fHostControl = _AddTextControl(layout, "host",
			B_TRANSLATE("Mail server:"));
	}
	if ((optionsMask & B_MAIL_PROTOCOL_HAS_USERNAME) != 0) {
		fUserControl = _AddTextControl(layout, "user",
			B_TRANSLATE("Username:"));
	}

	if ((optionsMask & B_MAIL_PROTOCOL_HAS_PASSWORD) != 0) {
		fPasswordControl = _AddTextControl(layout, "pass",
			B_TRANSLATE("Password:"));
		fPasswordControl->TextView()->HideTyping(true);
	}

	if ((optionsMask & B_MAIL_PROTOCOL_HAS_FLAVORS) != 0) {
		fFlavorField = _AddMenuField(layout, "flavor",
			B_TRANSLATE("Connection type:"));
	}

	if ((optionsMask & B_MAIL_PROTOCOL_HAS_AUTH_METHODS) != 0) {
		fAuthenticationField = _AddMenuField(layout, "auth_method",
			B_TRANSLATE("Login type:"));
	}

	if ((optionsMask & B_MAIL_PROTOCOL_CAN_LEAVE_MAIL_ON_SERVER) != 0) {
		fLeaveOnServerCheckBox = new BCheckBox("leave_mail_on_server",
			B_TRANSLATE("Leave mail on server"),
			new BMessage(kMsgLeaveOnServer));
		layout->AddView(fLeaveOnServerCheckBox, 0, layout->CountRows(), 2);

		fRemoveFromServerCheckBox = new BCheckBox("delete_remote_when_local",
			B_TRANSLATE("Remove mail from server when deleted"), NULL);
		fRemoveFromServerCheckBox->SetEnabled(false);
		layout->AddView(fRemoveFromServerCheckBox, 0, layout->CountRows(), 2);
	}

	if ((optionsMask & B_MAIL_PROTOCOL_PARTIAL_DOWNLOAD) != 0) {
		fBodyDownloadConfig = new BodyDownloadConfig();
		layout->AddView(fBodyDownloadConfig, 0, layout->CountRows(), 2);
	}
}


MailProtocolConfigView::~MailProtocolConfigView()
{
}


void
MailProtocolConfigView::SetTo(BMailProtocolSettings& settings)
{
	BString host = settings.FindString("server");
	if (settings.HasInt32("port"))
		host << ':' << settings.FindInt32("port");

	if (fHostControl != NULL)
		fHostControl->SetText(host.String());
	if (fUserControl != NULL)
		fUserControl->SetText(settings.FindString("username"));

	if (fPasswordControl != NULL) {
		char* password = get_passwd(&settings, "cpasswd");
		if (password != NULL) {
			fPasswordControl->SetText(password);
			delete[] password;
		} else
			fPasswordControl->SetText(settings.FindString("password"));
	}

	if (settings.HasInt32("flavor") && fFlavorField != NULL) {
		if (BMenuItem* item = fFlavorField->Menu()->ItemAt(
				settings.FindInt32("flavor")))
			item->SetMarked(true);
	}

	if (settings.HasInt32("auth_method") && fAuthenticationField != NULL) {
		if (BMenuItem* item = fAuthenticationField->Menu()->ItemAt(
				settings.FindInt32("auth_method"))) {
			item->SetMarked(true);
			_SetCredentialsEnabled(item->Command() != kMsgNoPassword);
		}
	}

	if (fLeaveOnServerCheckBox != NULL) {
		fLeaveOnServerCheckBox->SetValue(settings.FindBool(
			"leave_mail_on_server") ? B_CONTROL_ON : B_CONTROL_OFF);
	}

	if (fRemoveFromServerCheckBox != NULL) {
		fRemoveFromServerCheckBox->SetValue(settings.FindBool(
			"delete_remote_when_local") ? B_CONTROL_ON : B_CONTROL_OFF);
		fRemoveFromServerCheckBox->SetEnabled(
			settings.FindBool("leave_mail_on_server"));
	}

	if (fBodyDownloadConfig != NULL)
		fBodyDownloadConfig->SetTo(settings);
}


void
MailProtocolConfigView::AddFlavor(const char* label)
{
	if (fFlavorField != NULL) {
		fFlavorField->Menu()->AddItem(new BMenuItem(label, NULL));

		if (fFlavorField->Menu()->FindMarked() == NULL)
			fFlavorField->Menu()->ItemAt(0)->SetMarked(true);
	}
}


void
MailProtocolConfigView::AddAuthMethod(const char* label, bool needUserPassword)
{
	if (fAuthenticationField != NULL) {
		fAuthenticationField->Menu()->AddItem(new BMenuItem(label,
			new BMessage(needUserPassword
				? kMsgLeaveOnServer : kMsgNoPassword)));

		if (fAuthenticationField->Menu()->FindMarked() == NULL) {
			BMenuItem* item = fAuthenticationField->Menu()->ItemAt(0);
			item->SetMarked(true);
			MessageReceived(item->Message());
		}
	}
}


BGridLayout*
MailProtocolConfigView::Layout() const
{
	return (BGridLayout*)BView::GetLayout();
}


void
MailProtocolConfigView::AttachedToWindow()
{
	if (fAuthenticationField != NULL)
		fAuthenticationField->Menu()->SetTargetForItems(this);

	if (fLeaveOnServerCheckBox != NULL)
		fLeaveOnServerCheckBox->SetTarget(this);
}


void
MailProtocolConfigView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgNeedPassword:
			_SetCredentialsEnabled(true);
			break;
		case kMsgNoPassword:
			_SetCredentialsEnabled(false);
			break;

		case kMsgLeaveOnServer:
			fRemoveFromServerCheckBox->SetEnabled(
				message->FindInt32("be:value") == B_CONTROL_ON);
			break;
	}
}


status_t
MailProtocolConfigView::Archive(BMessage* into, bool deep) const
{
	if (fHostControl != NULL) {
		int32 port = -1;
		BString hostName = fHostControl->Text();
		if (hostName.FindFirst(':') > -1) {
			port = atol(hostName.String() + hostName.FindFirst(':') + 1);
			hostName.Truncate(hostName.FindFirst(':'));
		}

		if (into->ReplaceString("server", hostName.String()) != B_OK)
			into->AddString("server", hostName.String());

		// since there is no need for the port option, remove it here
		into->RemoveName("port");
		if (port != -1)
			into->AddInt32("port", port);
	} else {
		into->RemoveName("server");
		into->RemoveName("port");
	}

	if (fUserControl != NULL) {
		if (into->ReplaceString("username", fUserControl->Text()) != B_OK)
			into->AddString("username", fUserControl->Text());
	} else
		into->RemoveName("username");

	// remove old unencrypted passwords
	into->RemoveName("password");

	if (fPasswordControl != NULL)
		set_passwd(into, "cpasswd", fPasswordControl->Text());
	else
		into->RemoveName("cpasswd");

	_StoreIndexOfMarked(*into, "flavor", fFlavorField);
	_StoreIndexOfMarked(*into, "auth_method", fAuthenticationField);

	_StoreCheckBox(*into, "leave_mail_on_server", fLeaveOnServerCheckBox);
	_StoreCheckBox(*into, "delete_remote_when_local",
		fRemoveFromServerCheckBox);

	if (fBodyDownloadConfig != NULL)
		return fBodyDownloadConfig->Archive(into, deep);

	return B_OK;
}


BTextControl*
MailProtocolConfigView::_AddTextControl(BGridLayout* layout, const char* name,
	const char* label)
{
	BTextControl* control = new BTextControl(name, label, "", NULL);
	int32 row = layout->CountRows();
	layout->AddItem(control->CreateLabelLayoutItem(), 0, row);
	layout->AddItem(control->CreateTextViewLayoutItem(), 1, row);
	return control;
}


BMenuField*
MailProtocolConfigView::_AddMenuField(BGridLayout* layout, const char* name,
	const char* label)
{
	BPopUpMenu* menu = new BPopUpMenu("");
	BMenuField* field = new BMenuField(name, label, menu);
	int32 row = layout->CountRows();
	layout->AddItem(field->CreateLabelLayoutItem(), 0, row);
	layout->AddItem(field->CreateMenuBarLayoutItem(), 1, row);
	return field;
}


void
MailProtocolConfigView::_StoreIndexOfMarked(BMessage& message, const char* name,
	BMenuField* field) const
{
	int32 index = -1;
	if (field != NULL && field->Menu() != NULL) {
		BMenuItem* item = field->Menu()->FindMarked();
		if (item != NULL)
			index = field->Menu()->IndexOf(item);
	}
	if (message.ReplaceInt32(name, index) != B_OK)
		message.AddInt32(name, index);
}


void
MailProtocolConfigView::_StoreCheckBox(BMessage& message, const char* name,
	BCheckBox* checkBox) const
{
	bool value = checkBox != NULL && checkBox->Value() == B_CONTROL_ON;
	if (value) {
		if (message.ReplaceBool(name, value) != B_OK)
			message.AddBool(name, value);
	} else
		message.RemoveName(name);
}


void
MailProtocolConfigView::_SetCredentialsEnabled(bool enabled)
{
	if (fUserControl != NULL && fPasswordControl != NULL) {
		fUserControl->SetEnabled(enabled);
		fPasswordControl->SetEnabled(enabled);
	}
}


}	// namespace BPrivate
