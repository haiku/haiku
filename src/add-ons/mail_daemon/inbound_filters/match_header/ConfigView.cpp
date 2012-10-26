/*
 * Copyright 2004-2012, Haiku, Inc. All rights reserved.
 * Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>

#include <Catalog.h>
#include <LayoutBuilder.h>
#include <MailFilter.h>
#include <MailSettings.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <TextControl.h>

#include <FileConfigView.h>

#include "RuleFilter.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ConfigView"


using namespace BPrivate;


static const uint32 kMsgActionChanged = 'actC';


class RuleFilterConfig : public BView {
public:
								RuleFilterConfig(const BMessage& settings);

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				AttachedToWindow();
	virtual	status_t			Archive(BMessage* into, bool deep = true) const;

private:
			void				_SetVisible(BView* view, bool visible);

private:
			BTextControl*		fAttributeControl;
			BTextControl*		fRegexControl;
			FileControl*		fFileControl;
			BTextControl*		fFlagsControl;
			BPopUpMenu*			fActionMenu;
			BPopUpMenu*			fAccountMenu;
			BMenuField*			fAccountField;
			int					fAction;
			int32				fAccountID;
};


RuleFilterConfig::RuleFilterConfig(const BMessage& settings)
	:
	BView("rulefilter_config", 0),
	fActionMenu(NULL)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	if (settings.HasInt32("do_what"))
		fAction = settings.FindInt32("do_what");
	else
		fAction = -1;

	fAttributeControl = new BTextControl("attr", B_TRANSLATE("If"),
		B_TRANSLATE("header (e.g. Subject)"), NULL);
	if (settings.HasString("attribute"))
		fAttributeControl->SetText(settings.FindString("attribute"));

	fRegexControl = new BTextControl("regex", B_TRANSLATE("has"),
		B_TRANSLATE("value (use REGEX: in from of regular expressions like "
			"*spam*)"), NULL);
	if (settings.HasString("regex"))
		fRegexControl->SetText(settings.FindString("regex"));

	fFileControl = new FileControl("arg", NULL,
		B_TRANSLATE("this field is based on the action"));
	if (BControl* control = (BControl*)fFileControl->FindView("select_file"))
		control->SetEnabled(false);
	if (fAction == ACTION_MOVE_TO && settings.HasString("argument"))
		fFileControl->SetText(settings.FindString("argument"));

	fFlagsControl = new BTextControl("flags", NULL, NULL);
	if (fAction == ACTION_SET_FLAGS_TO && settings.HasString("argument"))
		fFlagsControl->SetText(settings.FindString("argument"));

	// Populate account menu

	fAccountMenu = new BPopUpMenu(B_TRANSLATE("<Choose account>"));

	if (fAction == ACTION_REPLY_WITH)
		fAccountID = settings.FindInt32("argument");
	else
		fAccountID = -1;

	BMailAccounts accounts;
	for (int32 i = 0; i < accounts.CountAccounts(); i++) {
		BMailAccountSettings* account = accounts.AccountAt(i);
		if (!account->HasOutbound())
			continue;

		BMessage* message = new BMessage();
		message->AddInt32("account id", account->AccountID());

		BMenuItem* item = new BMenuItem(account->Name(), message);
		fAccountMenu->AddItem(item);
		if (account->AccountID() == fAccountID)
			item->SetMarked(true);
	}

	fAccountField = new BMenuField("reply", "Foo", fAccountMenu);
	if (fAction >= 0) {
		BMenuItem* item = fActionMenu->ItemAt(fAction);
		if (item != NULL) {
			item->SetMarked(true);
			MessageReceived(item->Message());
		}
	}

	// Popuplate action menu

	fActionMenu = new BPopUpMenu(B_TRANSLATE("<Choose action>"));

	const struct {
		rule_action	action;
		const char*	label;
	} kActions[] = {
		{ACTION_MOVE_TO, B_TRANSLATE("Move to")},
		{ACTION_SET_FLAGS_TO, B_TRANSLATE("Set flags to")},
		{ACTION_DELETE_MESSAGE, B_TRANSLATE("Delete message")},
		{ACTION_REPLY_WITH, B_TRANSLATE("Reply with")},
		{ACTION_SET_AS_READ, B_TRANSLATE("Set as read")},
	};
	for (size_t i = 0; i < sizeof(kActions) / sizeof(kActions[0]); i++) {
		BMessage* message = new BMessage(kMsgActionChanged);
		message->AddInt32("action", (int32)kActions[i].action);

		fActionMenu->AddItem(new BMenuItem(kActions[i].label, message));
	}

	BMenuField* actionField = new BMenuField("do_what", B_TRANSLATE("Then"),
		fActionMenu);

	// Build layout

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.AddGroup(B_HORIZONTAL)
			.Add(fAttributeControl->CreateLabelLayoutItem())
			.Add(fAttributeControl->CreateTextViewLayoutItem())
			.Add(fRegexControl->CreateLabelLayoutItem())
			.Add(fRegexControl->CreateTextViewLayoutItem())
		.End()
		.AddGroup(B_HORIZONTAL)
			.Add(actionField->CreateLabelLayoutItem())
			.Add(actionField->CreateMenuBarLayoutItem())
		.End()
		.Add(fFileControl)
		.Add(fAccountField);
}


void
RuleFilterConfig::AttachedToWindow()
{
	fActionMenu->SetTargetForItems(this);
}


status_t
RuleFilterConfig::Archive(BMessage *into, bool deep) const
{
	into->MakeEmpty();
	into->AddInt32("do_what", fActionMenu->IndexOf(fActionMenu->FindMarked()));
	into->AddString("attribute", fAttributeControl->Text());
	into->AddString("regex", fRegexControl->Text());
	if (into->FindInt32("do_what") == ACTION_REPLY_WITH) {
		BMenuItem* item = fAccountMenu->FindMarked();
		if (item != NULL) {
			into->AddInt32("argument",
				item->Message()->FindInt32("account id"));
		}
	} else
		into->AddString("argument", fFileControl->Text());

	return B_OK;
}


void
RuleFilterConfig::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgActionChanged:
			fAction = message->FindInt32("action");

			_SetVisible(fFileControl, fAction == ACTION_MOVE_TO);
			_SetVisible(fFlagsControl, fAction == ACTION_SET_FLAGS_TO);
			_SetVisible(fAccountField, fAction == ACTION_REPLY_WITH);
			break;

		default:
			BView::MessageReceived(message);
	}
}


void
RuleFilterConfig::_SetVisible(BView* view, bool visible)
{
	while (visible && view->IsHidden(view))
		view->Show();
	while (!visible && !view->IsHidden(view))
		view->Hide();
}


// #pragma mark -


BView*
instantiate_filter_config_panel(BMailAddOnSettings& settings)
{
	return new RuleFilterConfig(settings);
}
