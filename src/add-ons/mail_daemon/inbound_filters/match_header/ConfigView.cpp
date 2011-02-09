/* RuleFilter's config view - performs action depending on matching a header value
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <MenuField.h>
#include <PopUpMenu.h>
#include <Message.h>
#include <TextControl.h>
#include <MenuItem.h>

#include <MailAddon.h>
#include <FileConfigView.h>
#include <MailSettings.h>

#include <MDRLanguage.h>

const uint32 kMsgActionMoveTo = 'argm';
const uint32 kMsgActionDelete = 'argd';
const uint32 kMsgActionSetTo = 'args';
const uint32 kMsgActionReplyWith = 'argr';
const uint32 kMsgActionSetRead = 'arge';


class RuleFilterConfig : public BView {
	public:
		RuleFilterConfig(const BMessage *settings);

		virtual	void MessageReceived(BMessage *msg);
		virtual	void AttachedToWindow();
		virtual	status_t Archive(BMessage *into, bool deep = true) const;
		virtual	void GetPreferredSize(float *width, float *height);
	private:
		BTextControl *attr, *regex;
		BFileControl *arg;
		BPopUpMenu *menu, *outbound;
		BMenuField *outbound_field;
		int staging;
		int32 chain;
};

#include <stdio.h>

RuleFilterConfig::RuleFilterConfig(const BMessage *settings)
	:
	BView(BRect(0,0,260,85),"rulefilter_config", B_FOLLOW_LEFT | B_FOLLOW_TOP,
		0), menu(NULL)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	attr = new BTextControl(BRect(5,5,100,20),"attr",MDR_DIALECT_CHOICE ("If","条件:"),MDR_DIALECT_CHOICE ("header (e.g. Subject)","ヘッダ(例えばSubject)"),NULL);
	attr->SetDivider(be_plain_font->StringWidth(MDR_DIALECT_CHOICE ("If ","条件: "))+ 4);
	if (settings->HasString("attribute"))
		attr->SetText(settings->FindString("attribute"));
	AddChild(attr);

	regex = new BTextControl(BRect(104,5,255,20),"attr",MDR_DIALECT_CHOICE (" has "," が "),MDR_DIALECT_CHOICE ("value (use REGEX: in from of regular expressions like *spam*)","値(正規表現対応)"),NULL);
	regex->SetDivider(be_plain_font->StringWidth(MDR_DIALECT_CHOICE (" has "," が ")) + 4);
	if (settings->HasString("regex"))
		regex->SetText(settings->FindString("regex"));
	AddChild(regex);

	arg = new BFileControl(BRect(5,55,255,80),"arg",NULL,MDR_DIALECT_CHOICE ("this field is based on the action","ここは動作によって意味が変わります"));
	if (BControl *control = (BControl *)arg->FindView("select_file"))
		control->SetEnabled(false);
	if (settings->HasString("argument"))
		arg->SetText(settings->FindString("argument"));

	outbound = new BPopUpMenu(MDR_DIALECT_CHOICE ("<Choose account>","<アカウントを選択>"));

	if (settings->HasInt32("do_what"))
		staging = settings->FindInt32("do_what");
	else
		staging = -1;
	if (staging == 3)
		chain = settings->FindInt32("argument");
	else
		chain = -1;
	printf("Chain: %ld\n",chain);

	BMailAccounts accounts;
	for (int32 i = 0; i < accounts.CountAccounts(); i++) {
		BMailAccountSettings* account = accounts.AccountAt(i);
		if (!account->HasOutbound())
			continue;
		BMenuItem *item = new BMenuItem(account->Name(),
			new BMessage(account->AccountID()));
		outbound->AddItem(item);
		if (account->AccountID() == chain)
			item->SetMarked(true);
	}
}


void RuleFilterConfig::AttachedToWindow() {
	if (menu != NULL)
		return; // We switched back from another tab

	menu = new BPopUpMenu(MDR_DIALECT_CHOICE ("<Choose action>","<動作を選択>"));
	menu->AddItem(new BMenuItem(MDR_DIALECT_CHOICE ("Move to","移動する"), new BMessage(kMsgActionMoveTo)));
	menu->AddItem(new BMenuItem(MDR_DIALECT_CHOICE ("Set flags to","フラグを指定する"), new BMessage(kMsgActionSetTo)));
	menu->AddItem(new BMenuItem(MDR_DIALECT_CHOICE ("Delete message","削除する"), new BMessage(kMsgActionDelete)));
	menu->AddItem(new BMenuItem(MDR_DIALECT_CHOICE ("Reply with","返事を書く"), new BMessage(kMsgActionReplyWith)));
	menu->AddItem(new BMenuItem(MDR_DIALECT_CHOICE ("Set as read","既読にする"), new BMessage(kMsgActionSetRead)));
	menu->SetTargetForItems(this);

	BMenuField *field = new BMenuField(BRect(5,30,210,50),"do_what",MDR_DIALECT_CHOICE ("Then","ならば"),menu);
	field->ResizeToPreferred();
	field->SetDivider(be_plain_font->StringWidth(MDR_DIALECT_CHOICE ("Then","ならば")) + 8);
	AddChild(field);

	outbound_field = new BMenuField(BRect(5,55,255,80),"reply","Foo",outbound);
	outbound_field->ResizeToPreferred();
	outbound_field->SetDivider(0);
	if (staging >= 0) {
		menu->ItemAt(staging)->SetMarked(true);
		MessageReceived(menu->ItemAt(staging)->Message());
	} else {
		AddChild(arg);
	}
}

status_t RuleFilterConfig::Archive(BMessage *into, bool deep) const {
	into->MakeEmpty();
	into->AddInt32("do_what",menu->IndexOf(menu->FindMarked()));
	into->AddString("attribute",attr->Text());
	into->AddString("regex",regex->Text());
	if (into->FindInt32("do_what") == 3)
		into->AddInt32("argument", outbound->FindMarked()->Message()->what);
	else
		into->AddString("argument",arg->Text());

	return B_OK;
}

void RuleFilterConfig::MessageReceived(BMessage *msg) {
	switch (msg->what)
	{
		case kMsgActionMoveTo:
		case kMsgActionSetTo:
			if (arg->FindView("file_path"))
				arg->SetEnabled(true);
			if (BControl *control = (BControl *)arg->FindView("select_file"))
				control->SetEnabled(msg->what == kMsgActionMoveTo);
			if (arg->Parent() == NULL) {
				outbound_field->RemoveSelf();
				AddChild(arg);
			}
			break;
		case kMsgActionDelete:
			arg->SetEnabled(false);
			if (arg->Parent() == NULL) {
				outbound_field->RemoveSelf();
				AddChild(arg);
			}
			break;
		case kMsgActionReplyWith:
			if (outbound->Parent() == NULL) {
				arg->RemoveSelf();
				AddChild(outbound_field);
			}
			break;
		case kMsgActionSetRead:
			arg->SetEnabled(false);
			if (arg->Parent() == NULL) {
				outbound_field->RemoveSelf();
				AddChild(arg);
			}
			break;
		default:
			BView::MessageReceived(msg);
	}
}

void RuleFilterConfig::GetPreferredSize(float *width, float *height) {
	*width = 260;
	*height = 55;
}


BView* instantiate_filter_config_panel(AddonSettings& settings)
{
	return new RuleFilterConfig(&settings.Settings());
}
