/* BMailProtocolConfigView - the standard config view for all protocols
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <TextControl.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <String.h>
#include <Message.h>
#include <MenuItem.h>
#include <CheckBox.h>

#include <stdlib.h>
#include <stdio.h>

#include <MDRLanguage.h>

class _EXPORT BMailProtocolConfigView;

#include <crypt.h>

#include "ProtocolConfigView.h"


namespace {

//--------------------Support functions and #defines---------------
#define enable_control(name) if (FindView(name) != NULL) ((BControl *)(FindView(name)))->SetEnabled(true)
#define disable_control(name) if (FindView(name) != NULL) ((BControl *)(FindView(name)))->SetEnabled(false)

BTextControl *AddTextField (BRect &rect, const char *name, const char *label);
BMenuField *AddMenuField (BRect &rect, const char *name, const char *label);
float FindWidestLabel(BView *view);

static float gItemHeight;

inline const char *TextControl(BView *parent,const char *name) {
	BTextControl *control = (BTextControl *)(parent->FindView(name));
	if (control != NULL)
		return control->Text();
		
	return "";
}

BTextControl *AddTextField (BRect &rect, const char *name, const char *label) {
	BTextControl *text_control = new BTextControl(rect,name,label,"",NULL,B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
//	text_control->SetDivider(be_plain_font->StringWidth(label));
	rect.OffsetBy(0,gItemHeight);
	return text_control;
}

BMenuField *AddMenuField (BRect &rect, const char *name, const char *label) {
	BPopUpMenu *menu = new BPopUpMenu("Select");
	BMenuField *control = new BMenuField(rect,name,label,menu,B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	control->SetDivider(be_plain_font->StringWidth(label) + 6);
	rect.OffsetBy(0,gItemHeight);
	return control;
}

inline BCheckBox *AddCheckBox(BRect &rect, const char *name, const char *label, BMessage *msg = NULL) {
	BCheckBox *control = new BCheckBox(rect,name,label,msg);
	rect.OffsetBy(0,gItemHeight);
	return control;
}

inline void SetTextControl(BView *parent, const char *name, const char *text) {
	BTextControl *control = (BTextControl *)(parent->FindView(name));
	if (control != NULL)
		control->SetText(text);
}

float FindWidestLabel(BView *view)
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


//----------------Real code----------------------
BMailProtocolConfigView::BMailProtocolConfigView(uint32 options_mask) : BView (BRect(0,0,100,20), "protocol_config_view", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW) {
	BRect rect(5,5,245,25);
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	// determine font height
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	gItemHeight = (int32)(fontHeight.ascent + fontHeight.descent + fontHeight.leading) + 13;
	rect.bottom = rect.top - 2 + gItemHeight;

	if (options_mask & B_MAIL_PROTOCOL_HAS_HOSTNAME)
		AddChild(AddTextField(rect,"host",MDR_DIALECT_CHOICE ("Mail Host:","サーバ名　：")));
	
	if (options_mask & B_MAIL_PROTOCOL_HAS_USERNAME)
		AddChild(AddTextField(rect,"user",MDR_DIALECT_CHOICE ("User Name:","ユーザーID：")));
	
	if (options_mask & B_MAIL_PROTOCOL_HAS_PASSWORD) {
		BTextControl *control = AddTextField(rect,"pass",MDR_DIALECT_CHOICE ("Password:","パスワード："));
		control->TextView()->HideTyping(true);
		AddChild(control);
	}
	
	if (options_mask & B_MAIL_PROTOCOL_HAS_FLAVORS)
		AddChild(AddMenuField(rect,"flavor","Connection Type:"));
	
	if (options_mask & B_MAIL_PROTOCOL_HAS_AUTH_METHODS)
		AddChild(AddMenuField(rect,"auth_method",MDR_DIALECT_CHOICE ("Authentication Method:","認証方法　：")));

	// set divider
	float width = FindWidestLabel(this);
	for (int32 i = CountChildren();i-- > 0;) {
		if (BTextControl *text = dynamic_cast<BTextControl *>(ChildAt(i)))
			text->SetDivider(width + 6);
	}

	if (options_mask & B_MAIL_PROTOCOL_CAN_LEAVE_MAIL_ON_SERVER) {
		AddChild(AddCheckBox(rect,"leave_mail_remote",MDR_DIALECT_CHOICE ("Leave Mail On Server","受信後にサーバ内のメールを削除しない"),new BMessage('lmos')));
		BCheckBox *box = AddCheckBox(rect,"delete_remote_when_local",MDR_DIALECT_CHOICE ("Delete Mail From Server When Deleted Locally","端末で削除されたらサーバ保存分も削除"));
		box->SetEnabled(false);
		AddChild(box);
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

BMailProtocolConfigView::~BMailProtocolConfigView() {}

void BMailProtocolConfigView::SetTo(BMessage *archive) {
	BString host = archive->FindString("server");
	if (archive->HasInt32("port"))
		host << ':' << archive->FindInt32("port");

	SetTextControl(this,"host",host.String());
	SetTextControl(this,"user",archive->FindString("username"));

	char *password = get_passwd(archive,"cpasswd");
	if (password)
	{
		SetTextControl(this,"pass",password);
		delete password;
	}
	else
		SetTextControl(this,"pass",archive->FindString("password"));
	
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

	BCheckBox *box;
	
	box = (BCheckBox *)(FindView("leave_mail_remote"));
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
}

void BMailProtocolConfigView::AddFlavor(const char *label) {
	BMenuField *menu = (BMenuField *)(FindView("flavor"));
	if (menu != NULL) {
		menu->Menu()->AddItem(new BMenuItem(label,NULL));
		if (menu->Menu()->FindMarked() == NULL)
			menu->Menu()->ItemAt(0)->SetMarked(true);
	}
}

void BMailProtocolConfigView::AddAuthMethod(const char *label,bool needUserPassword) {
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

void BMailProtocolConfigView::AttachedToWindow() {
	BMenuField *menu = (BMenuField *)(FindView("auth_method"));
	if (menu != NULL)
		menu->Menu()->SetTargetForItems(this);
		
	BCheckBox *box = (BCheckBox *)(FindView("leave_mail_remote"));
	if (box != NULL)
		box->SetTarget(this);
}

void BMailProtocolConfigView::MessageReceived(BMessage *msg) {
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

status_t BMailProtocolConfigView::Archive(BMessage *into, bool) const {
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
		
	if (FindView("leave_mail_remote") != NULL) {
		if (into->ReplaceBool("leave_mail_on_server",((BControl *)(FindView("leave_mail_remote")))->Value() == B_CONTROL_ON) != B_OK)
			into->AddBool("leave_mail_on_server",((BControl *)(FindView("leave_mail_remote")))->Value() == B_CONTROL_ON);
			
		if (into->ReplaceBool("delete_remote_when_local",((BControl *)(FindView("delete_remote_when_local")))->Value() == B_CONTROL_ON) != B_OK)
			into->AddBool("delete_remote_when_local",((BControl *)(FindView("delete_remote_when_local")))->Value() == B_CONTROL_ON);
	} else {
		if (into->ReplaceBool("leave_mail_on_server",false) != B_OK)
			into->AddBool("leave_mail_on_server",false);
			
		if (into->ReplaceBool("delete_remote_when_local",false) != B_OK)
			into->AddBool("delete_remote_when_local",false);
	}
		
	return B_OK;
}
	
void BMailProtocolConfigView::GetPreferredSize(float *width, float *height) {
	float minWidth;
	if (BView *view = FindView("delete_remote_when_local")) {
		float ignore;
		view->GetPreferredSize(&minWidth,&ignore);
	}
	if (minWidth < 250)
		minWidth = 250;
	*width = minWidth + 10;
	*height = (CountChildren() * gItemHeight) + 5;
}

}	// namespace
