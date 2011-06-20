/*
 * Copyright 20011, Haiku Inc. All Rights Reserved.
 * Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


//! The standard config view for all protocols.


#include "ProtocolConfigView.h"

#include <stdio.h>
#include <stdlib.h>

#include <Catalog.h>
#include <CheckBox.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <String.h>
#include <TextControl.h>

#include <crypt.h>


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "ProtocolConfigView"


const char* kPartialDownloadLimit = "partial_download_limit";


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
	fBytesLabel = new BStringView(r, "kb", "KB");
	AddChild(fBytesLabel);
	fSizeBox->SetDivider(0);

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(fPartialBox);
	AddChild(fSizeBox);
	ResizeToPreferred();
}


void
BodyDownloadConfig::SetTo(MailAddonSettings& addonSettings)
{
	const BMessage* settings = &addonSettings.Settings();

	int32 limit = 0;
	if (settings->HasInt32(kPartialDownloadLimit))
		limit = settings->FindInt32(kPartialDownloadLimit);
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


namespace {

//--------------------Support functions and #defines---------------
#define enable_control(name) if (FindView(name) != NULL) ((BControl *)(FindView(name)))->SetEnabled(true)
#define disable_control(name) if (FindView(name) != NULL) ((BControl *)(FindView(name)))->SetEnabled(false)

BTextControl *AddTextField (BRect &rect, const char *name, const char *label);
BMenuField *AddMenuField (BRect &rect, const char *name, const char *label);
float FindWidestLabel(BView *view);

static float sItemHeight;

inline const char *
TextControl(BView *parent,const char *name)
{
	BTextControl *control = (BTextControl *)(parent->FindView(name));
	if (control != NULL)
		return control->Text();

	return "";
}


BTextControl *
AddTextField(BRect &rect, const char *name, const char *label)
{
	BTextControl *text_control = new BTextControl(rect,name,label,"",NULL,B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
//	text_control->SetDivider(be_plain_font->StringWidth(label));
	rect.OffsetBy(0,sItemHeight);
	return text_control;
}


BMenuField *AddMenuField (BRect &rect, const char *name, const char *label) {
	BPopUpMenu *menu = new BPopUpMenu("Select");
	BMenuField *control = new BMenuField(rect,name,label,menu,B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	control->SetDivider(be_plain_font->StringWidth(label) + 6);
	rect.OffsetBy(0,sItemHeight);
	return control;
}


inline BCheckBox *
AddCheckBox(BRect &rect, const char *name, const char *label, BMessage *msg = NULL)
{
	BCheckBox *control = new BCheckBox(rect,name,label,msg);
	rect.OffsetBy(0,sItemHeight);
	return control;
}


inline void
SetTextControl(BView *parent, const char *name, const char *text)
{
	BTextControl *control = (BTextControl *)(parent->FindView(name));
	if (control != NULL)
		control->SetText(text);
}


float
FindWidestLabel(BView *view)
{
	float width = 0;
	for (int32 i = view->CountChildren();i-- > 0;) {
		if (BControl *control = dynamic_cast<BControl *>(view->ChildAt(i))) {
			float labelWidth = control->StringWidth(control->Label());
			if (labelWidth > width)
				width = labelWidth;
		}
	}
	return width;
}

} // unnamed namspace


//----------------Real code----------------------
BMailProtocolConfigView::BMailProtocolConfigView(uint32 options_mask)
	:
	BView (BRect(0,0,100,20), "protocol_config_view", B_FOLLOW_LEFT
		| B_FOLLOW_TOP, B_WILL_DRAW),
	fBodyDownloadConfig(NULL)
{
	BRect rect(5,5,245,25);
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	// determine font height
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	sItemHeight = (int32)(fontHeight.ascent + fontHeight.descent + fontHeight.leading) + 13;
	rect.bottom = rect.top - 2 + sItemHeight;

	if (options_mask & B_MAIL_PROTOCOL_HAS_HOSTNAME)
		AddChild(AddTextField(rect, "host", B_TRANSLATE("Mail server:")));

	if (options_mask & B_MAIL_PROTOCOL_HAS_USERNAME)
		AddChild(AddTextField(rect, "user", B_TRANSLATE("Username:")));

	if (options_mask & B_MAIL_PROTOCOL_HAS_PASSWORD) {
		BTextControl *control = AddTextField(rect, "pass",
			B_TRANSLATE("Password:"));
		control->TextView()->HideTyping(true);
		AddChild(control);
	}

	if (options_mask & B_MAIL_PROTOCOL_HAS_FLAVORS)
		AddChild(AddMenuField(rect, "flavor", B_TRANSLATE("Connection type:")));

	if (options_mask & B_MAIL_PROTOCOL_HAS_AUTH_METHODS)
		AddChild(AddMenuField(rect, "auth_method", B_TRANSLATE("Login type:")));

	// set divider
	float width = FindWidestLabel(this);
	for (int32 i = CountChildren();i-- > 0;) {
		if (BTextControl *text = dynamic_cast<BTextControl *>(ChildAt(i)))
			text->SetDivider(width + 6);
	}

	if (options_mask & B_MAIL_PROTOCOL_CAN_LEAVE_MAIL_ON_SERVER) {
		AddChild(AddCheckBox(rect, "leave_mail_on_server",
			B_TRANSLATE("Leave mail on server"), new BMessage('lmos')));
		BCheckBox* box = AddCheckBox(rect, "delete_remote_when_local",
			B_TRANSLATE("Remove mail from server when deleted"));
		box->SetEnabled(false);
		AddChild(box);
	}

	if (options_mask & B_MAIL_PROTOCOL_PARTIAL_DOWNLOAD) {
		fBodyDownloadConfig = new BodyDownloadConfig();
		fBodyDownloadConfig->MoveBy(0, rect.bottom + 5);
		AddChild(fBodyDownloadConfig);
	}

	// resize views
	float height;
	GetPreferredSize(&width,&height);
	ResizeTo(width,height);
	for (int32 i = CountChildren();i-- > 0;) {
		// this doesn't work with BTextControl, does anyone know why? -- axeld.
		if (BView *view = ChildAt(i))
			view->ResizeTo(width - 10,view->Bounds().Height());
	}
}


BMailProtocolConfigView::~BMailProtocolConfigView()
{
}


void
BMailProtocolConfigView::SetTo(MailAddonSettings& settings)
{
 	const BMessage* archive = &settings.Settings();

	BString host = archive->FindString("server");
	if (archive->HasInt32("port"))
		host << ':' << archive->FindInt32("port");

	SetTextControl(this,"host", host.String());
	SetTextControl(this,"user", archive->FindString("username"));

	char *password = get_passwd(archive, "cpasswd");
	if (password) {
		SetTextControl(this,"pass", password);
		delete[] password;
	} else
		SetTextControl(this,"pass", archive->FindString("password"));

	if (archive->HasInt32("flavor")) {
		BMenuField *menu = (BMenuField *)(FindView("flavor"));
		if (menu != NULL) {
			if (BMenuItem *item = menu->Menu()->ItemAt(archive->FindInt32("flavor")))
				item->SetMarked(true);
		}
	}

	if (archive->HasInt32("auth_method")) {
		BMenuField *menu = (BMenuField *)(FindView("auth_method"));
		if (menu != NULL) {
			if (BMenuItem *item = menu->Menu()->ItemAt(archive->FindInt32("auth_method"))) {
				item->SetMarked(true);
				if (item->Command() != 'none') {
					enable_control("user");
					enable_control("pass");
				}
			}
		}
	}


	BCheckBox *box = (BCheckBox *)(FindView("leave_mail_on_server"));
	if (box != NULL)
		box->SetValue(archive->FindBool("leave_mail_on_server") ? B_CONTROL_ON : B_CONTROL_OFF);

	box = (BCheckBox *)(FindView("delete_remote_when_local"));
	if (box != NULL) {
		box->SetValue(archive->FindBool("delete_remote_when_local") ? B_CONTROL_ON : B_CONTROL_OFF);

		if (archive->FindBool("leave_mail_on_server"))
			box->SetEnabled(true);
		else
			box->SetEnabled(false);
	}

	if (fBodyDownloadConfig)
		fBodyDownloadConfig->SetTo(settings);
}


void
BMailProtocolConfigView::AddFlavor(const char *label)
{
	BMenuField *menu = (BMenuField *)(FindView("flavor"));
	if (menu != NULL) {
		menu->Menu()->AddItem(new BMenuItem(label,NULL));
		if (menu->Menu()->FindMarked() == NULL)
			menu->Menu()->ItemAt(0)->SetMarked(true);
	}
}


void
BMailProtocolConfigView::AddAuthMethod(const char *label,bool needUserPassword)
{
	BMenuField *menu = (BMenuField *)(FindView("auth_method"));
	if (menu != NULL) {
		BMenuItem *item = new BMenuItem(label,new BMessage(needUserPassword ? 'some' : 'none'));

		menu->Menu()->AddItem(item);

		if (menu->Menu()->FindMarked() == NULL) {
			menu->Menu()->ItemAt(0)->SetMarked(true);
			MessageReceived(menu->Menu()->ItemAt(0)->Message());
		}
	}
}


void
BMailProtocolConfigView::AttachedToWindow()
{
	BMenuField *menu = (BMenuField *)(FindView("auth_method"));
	if (menu != NULL)
		menu->Menu()->SetTargetForItems(this);

	BCheckBox *box = (BCheckBox *)(FindView("leave_mail_on_server"));
	if (box != NULL)
		box->SetTarget(this);
}


void
BMailProtocolConfigView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case 'some':
			enable_control("user");
			enable_control("pass");
			break;
		case 'none':
			disable_control("user");
			disable_control("pass");
			break;

		case 'lmos':
			if (msg->FindInt32("be:value") == 1) {
				enable_control("delete_remote_when_local");
			} else {
				disable_control("delete_remote_when_local");
			}
			break;
	}
}


status_t
BMailProtocolConfigView::Archive(BMessage *into, bool deep) const
{
	const char *host = TextControl((BView *)this,"host");
	int32 port = -1;
	BString host_name = host;
	if (host_name.FindFirst(':') > -1) {
		port = atol(host_name.String() + host_name.FindFirst(':') + 1);
		host_name.Truncate(host_name.FindFirst(':'));
	}

	if (into->ReplaceString("server",host_name.String()) != B_OK)
		into->AddString("server",host_name.String());

	// since there is no need for the port option, remove it here
	into->RemoveName("port");
	if (port != -1)
		into->AddInt32("port",port);

	if (into->ReplaceString("username",TextControl((BView *)this,"user")) != B_OK)
		into->AddString("username",TextControl((BView *)this,"user"));

	// remove old unencrypted passwords
	into->RemoveName("password");

	set_passwd(into,"cpasswd",TextControl((BView *)this,"pass"));

	BMenuField *field;
	int32 index = -1;

	if ((field = (BMenuField *)(FindView("flavor"))) != NULL) {
		BMenuItem *item = field->Menu()->FindMarked();
		if (item != NULL)
			index = field->Menu()->IndexOf(item);
	}

	if (into->ReplaceInt32("flavor",index) != B_OK)
		into->AddInt32("flavor",index);

	index = -1;

	if ((field = (BMenuField *)(FindView("auth_method"))) != NULL) {
		BMenuItem *item = field->Menu()->FindMarked();
		if (item != NULL)
			index = field->Menu()->IndexOf(item);
	}

	if (into->ReplaceInt32("auth_method",index) != B_OK)
		into->AddInt32("auth_method",index);

	if (FindView("leave_mail_on_server") != NULL) {
		BControl* control = (BControl*)FindView("leave_mail_on_server");
		bool on = (control->Value() == B_CONTROL_ON);
		if (into->ReplaceBool("leave_mail_on_server", on) != B_OK)
			into->AddBool("leave_mail_on_server", on);

		control = (BControl*)FindView("delete_remote_when_local");
		on = (control->Value() == B_CONTROL_ON);
		if (into->ReplaceBool("delete_remote_when_local", on)) {
			into->AddBool("delete_remote_when_local", on);
		}
	} else {
		if (into->ReplaceBool("leave_mail_on_server", false) != B_OK)
			into->AddBool("leave_mail_on_server", false);

		if (into->ReplaceBool("delete_remote_when_local", false) != B_OK)
			into->AddBool("delete_remote_when_local", false);
	}

	if (fBodyDownloadConfig)
		fBodyDownloadConfig->Archive(into, deep);
	return B_OK;
}


void
BMailProtocolConfigView::GetPreferredSize(float *width, float *height)
{
	float minWidth = 250;
	if (BView *view = FindView("delete_remote_when_local")) {
		float ignore;
		view->GetPreferredSize(&minWidth,&ignore);
	}
	if (minWidth < 250)
		minWidth = 250;
	*width = minWidth + 10;
	*height = (CountChildren() * sItemHeight) + 5;

	if (fBodyDownloadConfig) {
		float bodyW, bodyH;
		fBodyDownloadConfig->GetPreferredSize(&bodyW, &bodyH);
		*height+= bodyH;
	}
}

