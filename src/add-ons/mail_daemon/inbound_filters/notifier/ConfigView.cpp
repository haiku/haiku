/* ConfigView - the configuration view for the Notifier filter
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include "ConfigView.h"

#include <Catalog.h>
#include <CheckBox.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <String.h>
#include <Message.h>

#include <MailAddon.h>
#include <MailSettings.h>


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "ConfigView"


const uint32 kMsgNotifyMethod = 'nomt';


ConfigView::ConfigView()
	:	BView(BRect(0,0,10,10),"notifier_config",B_FOLLOW_LEFT | B_FOLLOW_TOP,0)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	// determine font height
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	float itemHeight = (int32)(fontHeight.ascent + fontHeight.descent
		+ fontHeight.leading) + 6;
	
	BRect frame(5,2,250,itemHeight + 2);
	BPopUpMenu *menu = new BPopUpMenu(B_EMPTY_STRING,false,false);

	const char *notifyMethods[] = {
		B_TRANSLATE("Beep"),
		B_TRANSLATE("Alert"),
		B_TRANSLATE("Keyboard LEDs"),
		B_TRANSLATE("Central alert"),
		B_TRANSLATE("Central beep"),
		B_TRANSLATE("Log window")
	};
	for (int32 i = 0,j = 1;i < 6;i++,j *= 2) {
		menu->AddItem(new BMenuItem(notifyMethods[i],
			new BMessage(kMsgNotifyMethod)));
	}

	BMenuField *field = new BMenuField(frame,"notify", B_TRANSLATE("Method:"),
		menu);
	field->ResizeToPreferred();
	field->SetDivider(field->StringWidth(B_TRANSLATE("Method:")) + 6);
	AddChild(field);

	ResizeToPreferred();
}		


void ConfigView::AttachedToWindow()
{
	if (BMenuField *field = dynamic_cast<BMenuField *>(FindView("notify")))
		field->Menu()->SetTargetForItems(this);
}


void ConfigView::SetTo(const BMessage *archive)
{
	int32 method = archive->FindInt32("notification_method");
	if (method < 0)
		method = 1;

	BMenuField *field;
	if ((field = dynamic_cast<BMenuField *>(FindView("notify"))) == NULL)
		return;

	for (int32 i = field->Menu()->CountItems();i-- > 0;)
	{
		BMenuItem *item = field->Menu()->ItemAt(i);
		item->SetMarked((method & (1L << i)) != 0);
	}
	UpdateNotifyText();
}


void ConfigView::UpdateNotifyText()
{
	BMenuField *field;
	if ((field = dynamic_cast<BMenuField *>(FindView("notify"))) == NULL)
		return;

	BString label;
	for (int32 i = field->Menu()->CountItems();i-- > 0;)
	{
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


void ConfigView::MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case kMsgNotifyMethod:
		{
			BMenuItem *item;
			if (msg->FindPointer("source",(void **)&item) < B_OK)
				break;
			
			item->SetMarked(!item->IsMarked());
			UpdateNotifyText();
			break;
		}
		default:
			BView::MessageReceived(msg);
	}
}


status_t ConfigView::Archive(BMessage *into, bool) const
{
	int32 method = 0;

	BMenuField *field;
	if ((field = dynamic_cast<BMenuField *>(FindView("notify"))) != NULL)
	{
		for (int32 i = field->Menu()->CountItems();i-- > 0;)
		{
			BMenuItem *item = field->Menu()->ItemAt(i);
			if (item->IsMarked())
				method |= 1L << i;
		}
	}

	if (into->ReplaceInt32("notification_method",method) != B_OK)
		into->AddInt32("notification_method",method);

	return B_OK;
}

	
void ConfigView::GetPreferredSize(float *width, float *height)
{
	*width = 258;
	*height = ChildAt(0)->Bounds().Height() + 8;
}


BView*
instantiate_filter_config_panel(AddonSettings& settings)
{
	ConfigView *view = new ConfigView();
	view->SetTo(&settings.Settings());
	return view;
}


BString
descriptive_name()
{
	return B_TRANSLATE("New mails notification");
}
