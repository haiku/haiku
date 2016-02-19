/*
 * Copyright 2004-2012, Haiku, Inc. All rights reserved.
 * Copyright 2001, Dr. Zoidberg Enterprises. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include "NotifierConfigView.h"

#include <Catalog.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <String.h>
#include <Message.h>

#include <MailFilter.h>
#include <MailSettings.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "NotifierConfigView"


const uint32 kMsgNotifyMethod = 'nomt';


NotifierConfigView::NotifierConfigView()
	:
	BMailSettingsView("notifier_config")
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	BPopUpMenu *menu = new BPopUpMenu(B_EMPTY_STRING, false, false);

	const char *notifyMethods[] = {
		B_TRANSLATE("Beep"),
		B_TRANSLATE("Alert"),
		B_TRANSLATE("Keyboard LEDs"),
		B_TRANSLATE("Central alert"),
		B_TRANSLATE("Central beep"),
		B_TRANSLATE("Log window")
	};
	for (int32 i = 0, j = 1;i < 6; i++, j *= 2) {
		menu->AddItem(new BMenuItem(notifyMethods[i],
			new BMessage(kMsgNotifyMethod)));
	}

	BLayoutBuilder::Group<>(this).Add(
		new BMenuField("notify", B_TRANSLATE("Method:"), menu));
}


void
NotifierConfigView::SetTo(const BMessage *archive)
{
	int32 method = archive->FindInt32("notification_method");
	if (method < 0)
		method = 1;

	BMenuField *field;
	if ((field = dynamic_cast<BMenuField *>(FindView("notify"))) == NULL)
		return;

	for (int32 i = field->Menu()->CountItems(); i-- > 0;) {
		BMenuItem *item = field->Menu()->ItemAt(i);
		item->SetMarked((method & (1L << i)) != 0);
	}
	_UpdateNotifyText();
}


status_t
NotifierConfigView::SaveInto(BMailAddOnSettings& settings) const
{
	int32 method = 0;

	BMenuField *field;
	if ((field = dynamic_cast<BMenuField *>(FindView("notify"))) != NULL) {
		for (int32 i = field->Menu()->CountItems(); i-- > 0;) {
			BMenuItem *item = field->Menu()->ItemAt(i);
			if (item->IsMarked())
				method |= 1L << i;
		}
	}

	return settings.SetInt32("notification_method", method);
}


void
NotifierConfigView::AttachedToWindow()
{
	if (BMenuField *field = dynamic_cast<BMenuField *>(FindView("notify")))
		field->Menu()->SetTargetForItems(this);
}


void
NotifierConfigView::_UpdateNotifyText()
{
	BMenuField *field;
	if ((field = dynamic_cast<BMenuField *>(FindView("notify"))) == NULL)
		return;

	BString label;
	for (int32 i = field->Menu()->CountItems(); i-- > 0;) {
		BMenuItem *item = field->Menu()->ItemAt(i);
		if (!item->IsMarked())
			continue;

		if (label != "")
			label.Prepend(" + ");
		label.Prepend(item->Label());
	}
	if (label == "")
		label = B_TRANSLATE("none");
	field->MenuItem()->SetLabel(label.String());
}


void
NotifierConfigView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case kMsgNotifyMethod:
		{
			BMenuItem *item;
			if (msg->FindPointer("source",(void **)&item) < B_OK)
				break;

			item->SetMarked(!item->IsMarked());
			_UpdateNotifyText();
			break;
		}
		default:
			BView::MessageReceived(msg);
	}
}


// #pragma mark -


BMailSettingsView*
instantiate_filter_settings_view(const BMailAccountSettings& accountSettings,
	const BMailAddOnSettings& settings)
{
	NotifierConfigView* view = new NotifierConfigView();
	view->SetTo(&settings);
	return view;
}
