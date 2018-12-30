/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2001, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

BeMail(TM), Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or
registered trademarks of Be Incorporated in the United States and other
countries. Other brand product names are registered trademarks or trademarks
of their respective holders. All rights reserved.
*/


#include "Header.h"

#include <stdio.h>

#include <ControlLook.h>
#include <DateTimeFormat.h>
#include <E-mail.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <ObjectList.h>
#include <PopUpMenu.h>
#include <Query.h>
#include <String.h>
#include <StringView.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <Window.h>
#include <fs_index.h>
#include <fs_info.h>

#include <MailSettings.h>
#include <MailMessage.h>

#include "MailApp.h"
#include "MailSupport.h"
#include "MailWindow.h"
#include "Messages.h"
#include "Utilities.h"
#include "QueryMenu.h"
#include "FieldMsg.h"
#include "Prefs.h"


#define B_TRANSLATION_CONTEXT "Mail"


using namespace BPrivate;
using std::map;


const uint32 kMsgFrom = 'hFrm';
const uint32 kMsgAddressChosen = 'acsn';


struct CompareBStrings {
	bool
	operator()(const BString *s1, const BString *s2) const
	{
		return (s1->Compare(*s2) < 0);
	}
};


class LabelView : public BStringView {
public:
								LabelView(const char* label);

			bool				IsEnabled() const
									{ return fEnabled; }
			void				SetEnabled(bool enabled);

	virtual	void				Draw(BRect updateRect);

private:
			bool				fEnabled;
};


class HeaderTextControl : public BTextControl {
public:
								HeaderTextControl(const char* label,
									const char* name, BMessage* message);

	virtual	void				AttachedToWindow();
	virtual	void				SetEnabled(bool enabled);
	virtual	void				Draw(BRect updateRect);
	virtual	void				MessageReceived(BMessage* message);

private:
			void				_UpdateTextViewColors();
};


// #pragma mark - LabelView


LabelView::LabelView(const char* label)
	:
	BStringView("label", label),
	fEnabled(true)
{
	SetAlignment(B_ALIGN_RIGHT);
}


void
LabelView::SetEnabled(bool enabled)
{
	if (enabled != fEnabled) {
		fEnabled = enabled;
		Invalidate();
	}
}


void
LabelView::Draw(BRect updateRect)
{
	if (Text() != NULL) {
		BRect rect = Bounds();
		// TODO: solve this better (alignment of label and text)
		rect.top++;

		rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
		uint32 flags = 0;
		if (!IsEnabled())
			flags |= BControlLook::B_DISABLED;

		be_control_look->DrawLabel(this, Text(), rect, updateRect,
			base, flags, BAlignment(Alignment(), B_ALIGN_MIDDLE));
	}
}


// #pragma mark - HeaderTextControl


HeaderTextControl::HeaderTextControl(const char* label, const char* name,
	BMessage* message)
	:
	BTextControl(label, name, message)
{
}


void
HeaderTextControl::AttachedToWindow()
{
	BTextControl::AttachedToWindow();
	_UpdateTextViewColors();
}


void
HeaderTextControl::SetEnabled(bool enabled)
{
	BTextControl::SetEnabled(enabled);
	_UpdateTextViewColors();
}


void
HeaderTextControl::Draw(BRect updateRect)
{
	bool enabled = IsEnabled();
	bool active = TextView()->IsFocus() && Window()->IsActive();

	BRect rect = TextView()->Frame();
	rect.InsetBy(-2, -2);

	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	uint32 flags = 0;
	if (!enabled)
		flags = BControlLook::B_DISABLED;

	if (enabled) {
		if (active)
			flags |= BControlLook::B_FOCUSED;

		be_control_look->DrawTextControlBorder(this, rect, updateRect, base,
			flags);
	}

	if (Label() != NULL) {
		rect = CreateLabelLayoutItem()->Frame().OffsetByCopy(-Frame().left,
			-Frame().top);
		// TODO: solve this better (alignment of label and text)
		rect.top++;

		alignment labelAlignment;
		GetAlignment(&labelAlignment, NULL);

		be_control_look->DrawLabel(this, Label(), rect, updateRect,
			base, flags, BAlignment(labelAlignment, B_ALIGN_MIDDLE));
	}
}


void
HeaderTextControl::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case M_SELECT:
		{
			BTextView* textView = TextView();
			if (textView != NULL)
				textView->SelectAll();
			break;
		}

		default:
			BTextControl::MessageReceived(message);
			break;
	}
}


void
HeaderTextControl::_UpdateTextViewColors()
{
	BTextView* textView = TextView();

	BFont font;
	textView->GetFontAndColor(0, &font);

	rgb_color textColor;
	if (!textView->IsEditable() || IsEnabled())
		textColor = ui_color(B_DOCUMENT_TEXT_COLOR);
	else {
		textColor = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
			B_DISABLED_LABEL_TINT);
	}

	textView->SetFontAndColor(&font, B_FONT_ALL, &textColor);

	rgb_color color;
	if (!textView->IsEditable())
		color = ui_color(B_PANEL_BACKGROUND_COLOR);
	else if (IsEnabled())
		color = ui_color(B_DOCUMENT_BACKGROUND_COLOR);
	else {
		color = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
			B_LIGHTEN_2_TINT);
	}

	textView->SetViewColor(color);
	textView->SetLowColor(color);
}


// #pragma mark - THeaderView


THeaderView::THeaderView(bool incoming, bool resending, int32 defaultAccount)
	:
	fAccountMenu(NULL),
	fAccountID(defaultAccount),
	fFromControl(NULL),
	fCcControl(NULL),
	fBccControl(NULL),
	fDateControl(NULL),
	fIncoming(incoming),
	fResending(resending)
{
	// From
	if (fIncoming) {
		fFromControl = new HeaderTextControl(B_TRANSLATE("From:"), NULL, NULL);
		fFromControl->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
		fFromControl->SetEnabled(false);
	}

	// From accounts menu
	BMenuField* fromField = NULL;
	if (!fIncoming || fResending) {
		// And now the "from account" pop-up menu, on the left side, taking the
		// remaining space.

		fAccountMenu = new BPopUpMenu(B_EMPTY_STRING);

		BMailAccounts accounts;
		bool marked = false;
		for (int32 i = 0; i < accounts.CountAccounts(); i++) {
			BMailAccountSettings* account = accounts.AccountAt(i);
			BString name = account->Name();
			name << ":   " << account->RealName() << "  <"
				<< account->ReturnAddress() << ">";

			BMessage* msg = new BMessage(kMsgFrom);
			BMenuItem *item = new BMenuItem(name, msg);

			msg->AddInt32("id", account->AccountID());

			if (defaultAccount == account->AccountID()) {
				item->SetMarked(true);
				marked = true;
			}
			fAccountMenu->AddItem(item);
		}

		if (!marked) {
			BMenuItem *item = fAccountMenu->ItemAt(0);
			if (item != NULL) {
				item->SetMarked(true);
				fAccountID = item->Message()->FindInt32("id");
			} else {
				fAccountMenu->AddItem(
					item = new BMenuItem(B_TRANSLATE("<none>"), NULL));
				item->SetEnabled(false);
				fAccountID = ~(int32)0;
			}
			// default account is invalid, set to marked
			// TODO: do this differently, no casting and knowledge
			// of TMailApp here....
			if (TMailApp* app = dynamic_cast<TMailApp*>(be_app))
				app->SetDefaultAccount(fAccountID);
		}

		fromField = new BMenuField("account", B_TRANSLATE("From:"),
			fAccountMenu, B_WILL_DRAW | B_NAVIGABLE | B_NAVIGABLE_JUMP);
		fromField->SetAlignment(B_ALIGN_RIGHT);
	}

	// To
	fToLabel = new LabelView(B_TRANSLATE("To:"));
	fToControl = new AddressTextControl(B_TRANSLATE("To:"),
		new BMessage(TO_FIELD));
	if (fIncoming || fResending) {
		fToLabel->SetEnabled(false);
		fToControl->SetEditable(false);
		fToControl->SetEnabled(false);
	}

	BMessage* msg = new BMessage(FIELD_CHANGED);
	msg->AddInt32("bitmask", FIELD_TO);
	fToControl->SetModificationMessage(msg);

	// Carbon copy
	fCcLabel = new LabelView(B_TRANSLATE("Cc:"));
	fCcControl = new AddressTextControl("cc", new BMessage(CC_FIELD));
	if (fIncoming) {
		fCcLabel->SetEnabled(false);
		fCcControl->SetEditable(false);
		fCcControl->SetEnabled(false);
		fCcControl->Hide();
		fCcLabel->Hide();
	}
	msg = new BMessage(FIELD_CHANGED);
	msg->AddInt32("bitmask", FIELD_CC);
	fCcControl->SetModificationMessage(msg);

	// Blind carbon copy
	if (!fIncoming) {
		fBccControl = new AddressTextControl("bcc", new BMessage(BCC_FIELD));
		msg = new BMessage(FIELD_CHANGED);
		msg->AddInt32("bitmask", FIELD_BCC);
		fBccControl->SetModificationMessage(msg);
	}

	// Subject
	fSubjectControl = new HeaderTextControl(B_TRANSLATE("Subject:"), NULL,
		new BMessage(SUBJECT_FIELD));
	msg = new BMessage(FIELD_CHANGED);
	msg->AddInt32("bitmask", FIELD_SUBJECT);
	fSubjectControl->SetModificationMessage(msg);
	fSubjectControl->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	if (fIncoming || fResending)
		fSubjectControl->SetEnabled(false);

	// Date
	if (fIncoming) {
		fDateControl = new HeaderTextControl(B_TRANSLATE("Date:"), NULL, NULL);
		fDateControl->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
		fDateControl->SetEnabled(false);
	}

	BGridLayout* layout = GridLayout();

	if (fIncoming) {
		layout->SetHorizontalSpacing(0);
		layout->SetVerticalSpacing(0);
	} else
		layout->SetVerticalSpacing(B_USE_HALF_ITEM_SPACING);

	int32 row = 0;
	if (fromField != NULL) {
		layout->AddItem(fromField->CreateLabelLayoutItem(), 0, row);
		layout->AddItem(fromField->CreateMenuBarLayoutItem(), 1, row++, 3, 1);
	} else if (fFromControl != NULL) {
		layout->AddItem(fFromControl->CreateLabelLayoutItem(), 0, row);
		layout->AddItem(fFromControl->CreateTextViewLayoutItem(), 1, row);
	}

	if (fIncoming && !fResending) {
		layout->AddView(fToLabel, 2, row);
		layout->AddView(fToControl, 3, row++);
	} else {
		row++;
		layout->AddView(fToLabel, 0, row);
		layout->AddView(fToControl, 1, row++, 3, 1);
	}

	if (fDateControl != NULL) {
		layout->AddItem(fDateControl->CreateLabelLayoutItem(), 0, row);
		layout->AddItem(fDateControl->CreateTextViewLayoutItem(), 1, row);
	}

	if (fIncoming && (fCcControl != NULL)) {
		layout->AddView(fCcLabel, 2, row);
		layout->AddView(fCcControl, 3, row++);
	} else {
		row++;
		layout->AddView(fCcLabel, 0, row);
		layout->AddView(fCcControl, 1, row, 1, 1);
	}

	if (fBccControl != NULL) {
		layout->AddView(new LabelView(B_TRANSLATE("Bcc:")), 2, row);
		layout->AddView(fBccControl, 3, row++);
	} else
		row++;
	layout->AddItem(fSubjectControl->CreateLabelLayoutItem(), 0, row);
	layout->AddItem(fSubjectControl->CreateTextViewLayoutItem(), 1, row++,
		3, 1);
}


const char*
THeaderView::From() const
{
	return fFromControl != NULL ? fFromControl->Text() : NULL;
}


void
THeaderView::SetFrom(const char* from)
{
	if (fFromControl != NULL)
		fFromControl->SetText(from);
}


bool
THeaderView::IsToEmpty() const
{
	return To() == NULL || To()[0] == '\0';
}


const char*
THeaderView::To() const
{
	return fToControl->Text();
}


void
THeaderView::SetTo(const char* to)
{
	fToControl->SetText(to);
}


bool
THeaderView::IsCcEmpty() const
{
	return Cc() == NULL || Cc()[0] == '\0';
}


const char*
THeaderView::Cc() const
{
	return fCcControl != NULL ? fCcControl->Text() : NULL;
}


void
THeaderView::SetCc(const char* cc)
{
	fCcControl->SetText(cc);

	if (fIncoming) {
		if (cc != NULL && cc[0] != '\0') {
			if (fCcControl->IsHidden(this)) {
				fCcControl->Show();
				fCcLabel->Show();
			}
		} else if (!fCcControl->IsHidden(this)) {
			fCcControl->Hide();
			fCcLabel->Hide();
		}
	}
}


bool
THeaderView::IsBccEmpty() const
{
	return Bcc() == NULL || Bcc()[0] == '\0';
}


const char*
THeaderView::Bcc() const
{
	return fBccControl != NULL ? fBccControl->Text() : NULL;
}


void
THeaderView::SetBcc(const char* bcc)
{
	if (fBccControl != NULL)
		fBccControl->SetText(bcc);
}


bool
THeaderView::IsSubjectEmpty() const
{
	return Subject() == NULL || Subject()[0] == '\0';
}


const char*
THeaderView::Subject() const
{
	return fSubjectControl->Text();
}


void
THeaderView::SetSubject(const char* subject)
{
	fSubjectControl->SetText(subject);
}


bool
THeaderView::IsDateEmpty() const
{
	return Date() == NULL || Date()[0] == '\0';
}


const char*
THeaderView::Date() const
{
	return fDateControl != NULL ? fDateControl->Text() : NULL;
}


void
THeaderView::SetDate(time_t date)
{
	fDate = date;

	if (fDateControl != NULL) {
		BDateTimeFormat formatter;

		BString string;
		formatter.Format(string, date, B_FULL_DATE_FORMAT,
			B_MEDIUM_TIME_FORMAT);
		SetDate(string);
	}
}


void
THeaderView::SetDate(const char* date)
{
	if (fDateControl != NULL)
		fDateControl->SetText(date);
}


int32
THeaderView::AccountID() const
{
	return fAccountID;
}


const char*
THeaderView::AccountName() const
{
	BMenuItem* menuItem = fAccountMenu->FindMarked();
	if (menuItem != NULL)
		return menuItem->Label();

	return NULL;
}


void
THeaderView::SetAccount(int32 id)
{
	fAccountID = id;

	if (fAccountMenu == NULL)
		return;

	for (int32 i = fAccountMenu->CountItems(); i-- > 0;) {
		BMenuItem* item = fAccountMenu->ItemAt(i);
		if (item == NULL)
			continue;

		BMessage* message = item->Message();
		if (message->GetInt32("id", -1) == id) {
			item->SetMarked(true);
			break;
		}
	}
}


void
THeaderView::SetAccount(const char* name)
{
	if (fAccountMenu == NULL)
		return;

	BMenuItem* item = fAccountMenu->FindItem(name);
	if (item != NULL) {
		item->SetMarked(true);
		fAccountID = item->Message()->GetInt32("id", -1);
	}
}


status_t
THeaderView::SetFromMessage(BEmailMessage* mail)
{
	// Set Subject:, From:, To: & Cc: fields
	SetSubject(mail->Subject());
	SetFrom(mail->From());
	SetTo(mail->To());
	SetCc(mail->CC());

	BString accountName;
	if (mail->GetAccountName(accountName) == B_OK)
		SetAccount(accountName);

	// Set the date on this message
	time_t date = mail->Date();
	if (date <= 0) {
		const char* dateField = mail->HeaderField("Date");
		SetDate(dateField != NULL ? dateField : B_TRANSLATE("Unknown"));
	} else
		SetDate(date);

	return B_OK;
}


void
THeaderView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case B_SIMPLE_DATA:
		{
			BTextView* textView = dynamic_cast<BTextView*>(Window()->CurrentFocus());
			if (dynamic_cast<AddressTextControl *>(textView->Parent()) != NULL)
				textView->Parent()->MessageReceived(msg);
			else {
				BMessage message(*msg);
				message.what = REFS_RECEIVED;
				Window()->PostMessage(&message, Window());
			}
			break;
		}

		case kMsgFrom:
		{
			BMenuItem *item;
			if (msg->FindPointer("source", (void **)&item) >= B_OK)
				item->SetMarked(true);

			int32 account;
			if (msg->FindInt32("id",(int32 *)&account) >= B_OK)
				fAccountID = account;

			BMessage message(FIELD_CHANGED);
			// field doesn't matter; no special processing for this field
			// it's just to turn on the save button
			message.AddInt32("bitmask", 0);
			Window()->PostMessage(&message, Window());
			break;
		}
	}
}


void
THeaderView::AttachedToWindow()
{
	fToControl->SetTarget(Looper());
	fSubjectControl->SetTarget(Looper());
	fCcControl->SetTarget(Looper());
	if (fBccControl != NULL)
		fBccControl->SetTarget(Looper());
	if (fAccountMenu != NULL)
		fAccountMenu->SetTargetForItems(this);

	BView::AttachedToWindow();
}
