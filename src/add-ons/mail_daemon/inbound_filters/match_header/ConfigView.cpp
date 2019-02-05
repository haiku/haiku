/*
 * Copyright 2004-2016, Haiku, Inc. All rights reserved.
 * Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>

#include <Catalog.h>
#include <LayoutBuilder.h>
#include <MailFilter.h>
#include <MailSettings.h>
#include <MailSettingsView.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <TextControl.h>

#include <FileConfigView.h>

#include "MatchHeaderSettings.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ConfigView"


using namespace BPrivate;


static const uint32 kMsgActionChanged = 'actC';


class RuleFilterConfig : public BMailSettingsView {
public:
								RuleFilterConfig(
									const BMailAddOnSettings& settings);

	virtual status_t			SaveInto(BMailAddOnSettings& settings) const;

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				AttachedToWindow();

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


RuleFilterConfig::RuleFilterConfig(const BMailAddOnSettings& addOnSettings)
	:
	BMailSettingsView("rulefilter_config"),
	fActionMenu(NULL)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	MatchHeaderSettings settings(addOnSettings);
	fAction = settings.Action();

	fAttributeControl = new BTextControl("attr", B_TRANSLATE("If"), NULL, NULL);
	fAttributeControl->SetToolTip(
		B_TRANSLATE("Header field (e.g. Subject, From, " B_UTF8_ELLIPSIS ")"));
	fAttributeControl->SetText(settings.Attribute());

	fRegexControl = new BTextControl("regex", B_TRANSLATE("has"), NULL, NULL);
	fRegexControl->SetToolTip(B_TRANSLATE("Wildcard value like \"*spam*\".\n"
		"Prefix with \"REGEX:\" in order to use regular expressions."));
	fRegexControl->SetText(settings.Expression());

	fFileControl = new FileControl("arg", NULL,
		B_TRANSLATE("this field is based on the action"));
	if (BControl* control = (BControl*)fFileControl->FindView("select_file"))
		control->SetEnabled(false);
	fFileControl->SetText(settings.MoveTarget());

	fFlagsControl = new BTextControl("flags", NULL, NULL);
	fFlagsControl->SetText(settings.SetFlagsTo());

	// Populate account menu

	fAccountMenu = new BPopUpMenu(B_TRANSLATE("<Choose account>"));
	fAccountID = settings.ReplyAccount();

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

	fAccountField = new BMenuField("reply", B_TRANSLATE("Account"),
		fAccountMenu);

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

	BMenuField* actionField = new BMenuField("action", B_TRANSLATE("Then"),
		fActionMenu);
	if (fAction >= 0) {
		BMenuItem* item = fActionMenu->ItemAt(fAction);
		if (item != NULL) {
			item->SetMarked(true);
			MessageReceived(item->Message());
		}
	}

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


status_t
RuleFilterConfig::SaveInto(BMailAddOnSettings& settings) const
{
	int32 action = fActionMenu->IndexOf(fActionMenu->FindMarked());
	settings.SetInt32("action", action);
	settings.SetString("attribute", fAttributeControl->Text());
	settings.SetString("regex", fRegexControl->Text());

	switch (action) {
		case ACTION_MOVE_TO:
			settings.SetString("move target", fFileControl->Text());
			break;

		case ACTION_SET_FLAGS_TO:
			settings.SetString("set flags", fFlagsControl->Text());
			break;

		case ACTION_REPLY_WITH:
		{
			BMenuItem* item = fAccountMenu->FindMarked();
			if (item != NULL) {
				settings.SetInt32("account",
					item->Message()->FindInt32("account id"));
			}
			break;
		}
	}

	return B_OK;
}


void
RuleFilterConfig::AttachedToWindow()
{
	fActionMenu->SetTargetForItems(this);
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


BMailSettingsView*
instantiate_filter_settings_view(const BMailAccountSettings& accountSettings,
	const BMailAddOnSettings& settings)
{
	return new RuleFilterConfig(settings);
}
