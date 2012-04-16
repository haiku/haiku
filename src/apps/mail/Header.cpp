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

#include "MailApp.h"
#include "MailSupport.h"
#include "MailWindow.h"
#include "Messages.h"
#include "Header.h"
#include "Utilities.h"
#include "QueryMenu.h"
#include "FieldMsg.h"
#include "Prefs.h"

#include <MailSettings.h>
#include <MailMessage.h>

#include <CharacterSet.h>
#include <CharacterSetRoster.h>
#include <E-mail.h>
#include <Locale.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Query.h>
#include <String.h>
#include <StringView.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <Window.h>
#include <fs_index.h>
#include <fs_info.h>

#include <ctype.h>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#define B_TRANSLATION_CONTEXT "Mail"


using namespace BPrivate;
using std::map;


const char* kDateLabel = B_TRANSLATE("Date:");
const uint32 kMsgFrom = 'hFrm';
const uint32 kMsgEncoding = 'encd';
const uint32 kMsgAddressChosen = 'acsn';

static const float kTextControlDividerOffset = 0;
static const float kMenuFieldDividerOffset = 6;


class QPopupMenu : public QueryMenu {
	public:
		QPopupMenu(const char *title);

	private:
		void AddPersonItem(const entry_ref *ref, ino_t node, BString &name,
			BString &email, const char *attr, BMenu *groupMenu,
			BMenuItem *superItem);

	protected:
		virtual void EntryCreated(const entry_ref &ref, ino_t node);
		virtual void EntryRemoved(ino_t node);

		int32 fGroups; // Current number of "group" submenus.  Includes All People if present.
};


struct CompareBStrings {
	bool
	operator()(const BString *s1, const BString *s2) const
	{
		return (s1->Compare(*s2) < 0);
	}
};


const char*
mail_to_filter(const char* text, int32& length, const text_run_array*& runs)
{
	if (!strncmp(text, "mailto:", 7)) {
		text += 7;
		length -= 7;
		if (runs != NULL)
			runs = NULL;
	}

	return text;
}


static const float kPlainFontSizeScale = 0.9;


//	#pragma mark - THeaderView


THeaderView::THeaderView(BRect rect, BRect windowRect, bool incoming,
	bool resending, uint32 defaultCharacterSet, int32 defaultAccount)
	:
	BBox(rect, "m_header", B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW, B_NO_BORDER),

	fAccountMenu(NULL),
	fEncodingMenu(NULL),
	fAccountID(defaultAccount),
	fAccountTo(NULL),
	fAccount(NULL),
	fBcc(NULL),
	fCc(NULL),
	fSubject(NULL),
	fTo(NULL),
	fDateLabel(NULL),
	fDate(NULL),
	fIncoming(incoming),
	fCharacterSetUserSees(defaultCharacterSet),
	fResending(resending),
	fBccMenu(NULL),
	fCcMenu(NULL),
	fToMenu(NULL),
	fEmailList(NULL)
{
	BMenuField* field;
	BMessage* msg;

	float x = StringWidth( /* The longest title string in the header area */
		B_TRANSLATE("Attachments: ")) + 9;
	float y = TO_FIELD_V;

	BMenuBar* dummy = new BMenuBar(BRect(0, 0, 100, 15), "Dummy");
	AddChild(dummy);
	float width, menuBarHeight;
	dummy->GetPreferredSize(&width, &menuBarHeight);
	dummy->RemoveSelf();
	delete dummy;

	float menuFieldHeight = menuBarHeight + 6;
	float controlHeight = menuBarHeight + floorf(be_plain_font->Size() / 1.15);

	if (!fIncoming) {
		InitEmailCompletion();
		InitGroupCompletion();
	}

	// Prepare the character set selection pop-up menu (we tell the user that
	// it is the Encoding menu, even though it is really the character set).
	// It may appear in the first line, to the right of the From box if the
	// user is reading an e-mail.  It appears on the second line, to the right
	// of the e-mail account menu, if the user is composing a message.  It lets
	// the user quickly select a character set different from the application
	// wide default one, and also shows them which character set is active.  If
	// you are reading a message, you also see an item that says "Automatic"
	// for automatic decoding character set choice.  It can slide around as the
	// window is resized when viewing a message, but not when composing
	// (because the adjacent pop-up menu can't resize dynamically due to a BeOS
	// bug).

	float widestCharacterSet = 0;
	bool markedCharSet = false;
	BMenuItem* item;

	fEncodingMenu = new BPopUpMenu(B_EMPTY_STRING);

	BCharacterSetRoster roster;
	BCharacterSet charset;
	while (roster.GetNextCharacterSet(&charset) == B_OK) {
		BString name(charset.GetPrintName());
		const char* mime = charset.GetMIMEName();
		if (mime)
			name << " (" << mime << ")";

		uint32 convertID;
		if (mime == NULL || strcasecmp(mime, "UTF-8") != 0)
			convertID = charset.GetConversionID();
		else
			convertID = B_MAIL_UTF8_CONVERSION;

		msg = new BMessage(kMsgEncoding);
		msg->AddInt32("charset", convertID);
		fEncodingMenu->AddItem(item = new BMenuItem(name.String(), msg));
		if (convertID == fCharacterSetUserSees && !markedCharSet) {
			item->SetMarked(true);
			markedCharSet = true;
		}
		if (StringWidth(name.String()) > widestCharacterSet)
			widestCharacterSet = StringWidth(name.String());
	}

	msg = new BMessage(kMsgEncoding);
	msg->AddInt32("charset", B_MAIL_US_ASCII_CONVERSION);
	fEncodingMenu->AddItem(item = new BMenuItem("US-ASCII", msg));
	if (fCharacterSetUserSees == B_MAIL_US_ASCII_CONVERSION && !markedCharSet) {
		item->SetMarked(true);
		markedCharSet = true;
	}

	if (!resending && fIncoming) {
		// reading a message, display the Automatic item
		fEncodingMenu->AddSeparatorItem();
		msg = new BMessage(kMsgEncoding);
		msg->AddInt32("charset", B_MAIL_NULL_CONVERSION);
		fEncodingMenu->AddItem(item = new BMenuItem(B_TRANSLATE("Automatic"), msg));
		if (!markedCharSet)
			item->SetMarked(true);
	}

	// First line of the header, From for reading e-mails (includes the
	// character set choice at the right), To when composing (nothing else in
	// the row).

	BRect r;
	char string[20];
	if (fIncoming && !resending) {
		// Set up the character set pop-up menu on the right of "To" box.
		r.Set (windowRect.Width() - widestCharacterSet -
			StringWidth (B_TRANSLATE("Decoding:")) - 2 * SEPARATOR_MARGIN,
				y - 2, windowRect.Width() - SEPARATOR_MARGIN,
				y + menuFieldHeight);
		field = new BMenuField (r, "decoding", B_TRANSLATE("Decoding:"),
			fEncodingMenu, true /* fixedSize */,
			B_FOLLOW_TOP | B_FOLLOW_RIGHT,
			B_WILL_DRAW | B_NAVIGABLE | B_NAVIGABLE_JUMP);
		field->SetDivider(field->StringWidth(B_TRANSLATE("Decoding:")) + 5);
		AddChild(field);
		r.Set(SEPARATOR_MARGIN, y,
			  field->Frame().left - SEPARATOR_MARGIN, y + menuFieldHeight);
		sprintf(string, B_TRANSLATE("From:"));
	} else {
		r.Set(x - 12, y, windowRect.Width() - SEPARATOR_MARGIN,
			y + menuFieldHeight);
		string[0] = 0;
	}

	y += controlHeight;
	fTo = new TTextControl(r, string, new BMessage(TO_FIELD), fIncoming,
		resending, B_FOLLOW_LEFT_RIGHT);
	fTo->SetFilter(mail_to_filter);

	if (!fIncoming || resending) {
		fTo->SetChoiceList(&fEmailList);
		fTo->SetAutoComplete(true);
	} else {
		fTo->SetDivider(x - 12 - SEPARATOR_MARGIN);
		fTo->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	}

	AddChild(fTo);
	msg = new BMessage(FIELD_CHANGED);
	msg->AddInt32("bitmask", FIELD_TO);
	fTo->SetModificationMessage(msg);

	if (!fIncoming || resending) {
		r.right = r.left - 5;
		r.left = r.right - ceilf(be_plain_font->StringWidth(
			B_TRANSLATE("To:")) + 25);
		r.top -= 1;
		fToMenu = new QPopupMenu(B_TRANSLATE("To:"));
		field = new BMenuField(r, "", "", fToMenu, true,
			B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
		field->SetDivider(0.0);
		field->SetEnabled(true);
		AddChild(field);
	}

	// "From:" accounts Menu and Encoding Menu.
	if (!fIncoming || resending) {
		// Put the character set box on the right of the From field.
		r.Set(windowRect.Width() - widestCharacterSet -
			StringWidth(B_TRANSLATE("Encoding:")) - 2 * SEPARATOR_MARGIN,
			y - 2, windowRect.Width() - SEPARATOR_MARGIN, y + menuFieldHeight);
		BMenuField* encodingField = new BMenuField(r, "encoding",
			B_TRANSLATE("Encoding:"), fEncodingMenu, true /* fixedSize */,
			B_FOLLOW_TOP | B_FOLLOW_RIGHT,
			B_WILL_DRAW | B_NAVIGABLE | B_NAVIGABLE_JUMP);
		encodingField->SetDivider(encodingField->StringWidth(
			B_TRANSLATE("Encoding:")) + 5);
		AddChild(encodingField);

		field = encodingField;

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

			msg = new BMessage(kMsgFrom);
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
				fAccountID = ~0UL;
			}
			// default account is invalid, set to marked
			// TODO: do this differently, no casting and knowledge
			// of TMailApp here....
			if (TMailApp* app = dynamic_cast<TMailApp*>(be_app))
				app->SetDefaultAccount(fAccountID);
		}

		r.Set(SEPARATOR_MARGIN, y - 2,
			  field->Frame().left - SEPARATOR_MARGIN, y + menuFieldHeight);
		field = new BMenuField(r, "account", B_TRANSLATE("From:"),
			fAccountMenu, true /* fixedSize */,
			B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT,
			B_WILL_DRAW | B_NAVIGABLE | B_NAVIGABLE_JUMP);
		AddChild(field, encodingField);
		field->SetDivider(x - 12 - SEPARATOR_MARGIN + kMenuFieldDividerOffset);
		field->SetAlignment(B_ALIGN_RIGHT);
		y += controlHeight;
	} else {
		// To: account
		bool account = BMailAccounts().CountAccounts() > 0;

		r.Set(SEPARATOR_MARGIN, y,
			  windowRect.Width() - SEPARATOR_MARGIN, y + menuFieldHeight);
		if (account)
			r.right -= SEPARATOR_MARGIN + ACCOUNT_FIELD_WIDTH;
		fAccountTo = new TTextControl(r, B_TRANSLATE("To:"), NULL, fIncoming,
			false, B_FOLLOW_LEFT_RIGHT);
		fAccountTo->SetEnabled(false);
		fAccountTo->SetDivider(x - 12 - SEPARATOR_MARGIN);
		fAccountTo->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
		AddChild(fAccountTo);

		if (account) {
			r.left = r.right + 6;  r.right = windowRect.Width() - SEPARATOR_MARGIN;
			fAccount = new TTextControl(r, B_TRANSLATE("Account:"), NULL,
				fIncoming, false, B_FOLLOW_RIGHT | B_FOLLOW_TOP);
			fAccount->SetEnabled(false);
			AddChild(fAccount);
		}
		y += controlHeight;
	}

	--y;
	r.Set(SEPARATOR_MARGIN, y,
		windowRect.Width() - SEPARATOR_MARGIN, y + menuFieldHeight);
	y += controlHeight;
	fSubject = new TTextControl(r, B_TRANSLATE("Subject:"),
		new BMessage(SUBJECT_FIELD),fIncoming, false, B_FOLLOW_LEFT_RIGHT);
	AddChild(fSubject);
	(msg = new BMessage(FIELD_CHANGED))->AddInt32("bitmask", FIELD_SUBJECT);
	fSubject->SetModificationMessage(msg);
	fSubject->SetDivider(x - 12 - SEPARATOR_MARGIN);
	fSubject->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	if (fResending)
		fSubject->SetEnabled(false);

	--y;

	if (!fIncoming) {
		r.Set(x - 12, y, CC_FIELD_H + CC_FIELD_WIDTH, y + menuFieldHeight);
		fCc = new TTextControl(r, "", new BMessage(CC_FIELD), fIncoming, false);
		fCc->SetFilter(mail_to_filter);
		fCc->SetChoiceList(&fEmailList);
		fCc->SetAutoComplete(true);
		AddChild(fCc);
		(msg = new BMessage(FIELD_CHANGED))->AddInt32("bitmask", FIELD_CC);
		fCc->SetModificationMessage(msg);

		r.right = r.left - 5;
		r.left = r.right - ceilf(be_plain_font->StringWidth(
			B_TRANSLATE("Cc:")) + 25);
		r.top -= 1;
		fCcMenu = new QPopupMenu(B_TRANSLATE("Cc:"));
		field = new BMenuField(r, "", "", fCcMenu, true,
			B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);

		field->SetDivider(0.0);
		field->SetEnabled(true);
		AddChild(field);

		r.Set(BCC_FIELD_H + be_plain_font->StringWidth(B_TRANSLATE("Bcc:")), y,
			  windowRect.Width() - SEPARATOR_MARGIN, y + menuFieldHeight);
		y += controlHeight;
		fBcc = new TTextControl(r, "", new BMessage(BCC_FIELD),
						fIncoming, false, B_FOLLOW_LEFT_RIGHT);
		fBcc->SetFilter(mail_to_filter);
		fBcc->SetChoiceList(&fEmailList);
		fBcc->SetAutoComplete(true);
		AddChild(fBcc);
		(msg = new BMessage(FIELD_CHANGED))->AddInt32("bitmask", FIELD_BCC);
		fBcc->SetModificationMessage(msg);

		r.right = r.left - 5;
		r.left = r.right - ceilf(be_plain_font->StringWidth(
			B_TRANSLATE("Bcc:")) + 25);
		r.top -= 1;
		fBccMenu = new QPopupMenu(B_TRANSLATE("Bcc:"));
		field = new BMenuField(r, "", "", fBccMenu, true,
			B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
		field->SetDivider(0.0);
		field->SetEnabled(true);
		AddChild(field);
	} else {
		y -= SEPARATOR_MARGIN;
		r.Set(SEPARATOR_MARGIN, y, x - 12 - 1, y + menuFieldHeight);
		fDateLabel = new BStringView(r, "", kDateLabel);
		fDateLabel->SetAlignment(B_ALIGN_RIGHT);
		AddChild(fDateLabel);
		fDateLabel->SetHighColor(0, 0, 0);

		r.Set(r.right + 9, y, windowRect.Width() - SEPARATOR_MARGIN,
			y + menuFieldHeight);
		fDate = new BStringView(r, "", "");
		AddChild(fDate);
		fDate->SetHighColor(0, 0, 0);

		y += controlHeight + 5;
	}
	ResizeTo(Bounds().Width(), y);
}


void
THeaderView::InitEmailCompletion()
{
	// get boot volume
	BVolume volume;
	BVolumeRoster().GetBootVolume(&volume);

	BQuery query;
	query.SetVolume(&volume);
	query.SetPredicate("META:email=**");
		// Due to R5 BFS bugs, you need two stars, META:email=** for the query.
		// META:email="*" will just return one entry and stop, same with
		// META:email=* and a few other variations.  Grumble.
	query.Fetch();
	entry_ref ref;

	while (query.GetNextRef (&ref) == B_OK) {
		BNode file;
		if (file.SetTo(&ref) == B_OK) {
			// Add the e-mail address as an auto-complete string.
			BString email;
			if (file.ReadAttrString("META:email", &email) >= B_OK)
				fEmailList.AddChoice(email.String());

			// Also add the quoted full name as an auto-complete string.  Can't
			// do unquoted since auto-complete isn't that smart, so the user
			// will have to type a quote mark if he wants to select someone by
			// name.
			BString fullName;
			if (file.ReadAttrString("META:name", &fullName) >= B_OK) {
				if (email.FindFirst('<') < 0) {
					email.ReplaceAll('>', '_');
					email.Prepend("<");
					email.Append(">");
				}
				fullName.ReplaceAll('\"', '_');
				fullName.Prepend("\"");
				fullName << "\" " << email;
				fEmailList.AddChoice(fullName.String());
			}

			// support for 3rd-party People apps.  Looks like a job for
			// multiple keyword (so you can have several e-mail addresses in
			// one attribute, perhaps comma separated) indices!  Which aren't
			// yet in BFS.
			for (int16 i = 2; i < 6; i++) {
				char attr[16];
				sprintf(attr, "META:email%d", i);
				if (file.ReadAttrString(attr, &email) >= B_OK)
					fEmailList.AddChoice(email.String());
			}
		}
	}
}


void
THeaderView::InitGroupCompletion()
{
	// get boot volume
	BVolume volume;
	BVolumeRoster().GetBootVolume(&volume);

	// Build a list of all unique groups and the addresses they expand to.
	BQuery query;
	query.SetVolume(&volume);
	query.SetPredicate("META:group=**");
	query.Fetch();

	map<BString *, BString *, CompareBStrings> groupMap;
	entry_ref ref;
	BNode file;
	while (query.GetNextRef(&ref) == B_OK) {
		if (file.SetTo(&ref) != B_OK)
			continue;

		BString groups;
		if (file.ReadAttrString("META:group", &groups) < B_OK || groups.Length() == 0)
			continue;

		BString address;
		file.ReadAttrString("META:email", &address);

		// avoid adding an empty address
		if (address.Length() == 0)
			continue;

		char *group = groups.LockBuffer(groups.Length());
		char *next = strchr(group, ',');

		for (;;) {
			if (next)
				*next = 0;

			while (*group && *group == ' ')
				group++;

			BString *groupString = new BString(group);
			BString *addressListString = NULL;

			// nobody is in this group yet, start it off
			if (groupMap[groupString] == NULL) {
				addressListString = new BString(*groupString);
				addressListString->Append(" ");
				groupMap[groupString] = addressListString;
			} else {
				addressListString = groupMap[groupString];
				addressListString->Append(", ");
				delete groupString;
			}

			// Append the user's address to the end of the string with the
			// comma separated list of addresses.  If not present, add the
			// < and > brackets around the address.

			if (address.FindFirst ('<') < 0) {
				address.ReplaceAll ('>', '_');
				address.Prepend ("<");
				address.Append(">");
			}
			addressListString->Append(address);

			if (!next)
				break;

			group = next + 1;
			next = strchr(group, ',');
		}
	}

	map<BString *, BString *, CompareBStrings>::iterator iter;
	for (iter = groupMap.begin(); iter != groupMap.end();) {
		BString *group = iter->first;
		BString *addr = iter->second;
		fEmailList.AddChoice(addr->String());
		++iter;
		groupMap.erase(group);
		delete group;
		delete addr;
	}
}


void
THeaderView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case B_SIMPLE_DATA:
		{
			BTextView *textView = dynamic_cast<BTextView *>(Window()->CurrentFocus());
			if (dynamic_cast<TTextControl *>(textView->Parent()) != NULL)
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

		case kMsgEncoding:
		{
			BMessage message(*msg);
			int32 charSet;

			if (msg->FindInt32("charset", &charSet) == B_OK)
				fCharacterSetUserSees = charSet;

			message.what = CHARSET_CHOICE_MADE;
			message.AddInt32 ("charset", fCharacterSetUserSees);
			Window()->PostMessage (&message, Window());
			
			BMessage message2(FIELD_CHANGED);
			// field doesn't matter; no special processing for this field
			// it's just to turn on the save button
			message2.AddInt32("bitmask", 0);
			Window()->PostMessage(&message2, Window());
			
			break;
		}
	}
}


void
THeaderView::AttachedToWindow()
{
	if (fToMenu) {
		fToMenu->SetTargetForItems(fTo);
		fToMenu->SetPredicate("META:email=**");
	}
	if (fCcMenu) {
		fCcMenu->SetTargetForItems(fCc);
		fCcMenu->SetPredicate("META:email=**");
	}
	if (fBccMenu) {
		fBccMenu->SetTargetForItems(fBcc);
		fBccMenu->SetPredicate("META:email=**");
	}
	if (fTo)
		fTo->SetTarget(Looper());
	if (fSubject)
		fSubject->SetTarget(Looper());
	if (fCc)
		fCc->SetTarget(Looper());
	if (fBcc)
		fBcc->SetTarget(Looper());
	if (fAccount)
		fAccount->SetTarget(Looper());
	if (fAccountMenu)
		fAccountMenu->SetTargetForItems(this);
	if (fEncodingMenu)
		fEncodingMenu->SetTargetForItems(this);

	BBox::AttachedToWindow();
}


status_t
THeaderView::LoadMessage(BEmailMessage *mail)
{
	//	Set the date on this message
	const char *dateField = mail->Date();
	char string[256];
	sprintf(string, "%s", dateField != NULL ? dateField : "Unknown");
	fDate->SetText(string);

	//	Set contents of header fields
	if (fIncoming && !fResending) {
		if (fBcc != NULL)
			fBcc->SetEnabled(false);

		if (fCc != NULL)
			fCc->SetEnabled(false);

		if (fAccount != NULL)
			fAccount->SetEnabled(false);

		if (fAccountTo != NULL)
			fAccountTo->SetEnabled(false);

		fSubject->SetEnabled(false);
		fTo->SetEnabled(false);
	}

	//	Set Subject: & From: fields
	fSubject->SetText(mail->Subject());
	fTo->SetText(mail->From());

	//	Set Account/To Field
	if (fAccountTo != NULL)
		fAccountTo->SetText(mail->To());

	BString accountName;
	if (fAccount != NULL && mail->GetAccountName(accountName) == B_OK)
		fAccount->SetText(accountName);

	return B_OK;
}


//	#pragma mark - TTextControl


TTextControl::TTextControl(BRect rect, const char *label, BMessage *msg,
		bool incoming, bool resending, int32 resizingMode)
	: BComboBox(rect, "happy", label, msg, resizingMode),
	fRefDropMenu(NULL)
	//:BTextControl(rect, "happy", label, "", msg, resizingMode)
{
	strcpy(fLabel, label);
	fCommand = msg != NULL ? msg->what : 0UL;
	fIncoming = incoming;
	fResending = resending;
}


void
TTextControl::AttachedToWindow()
{
	SetHighColor(0, 0, 0);
	// BTextControl::AttachedToWindow();
	BComboBox::AttachedToWindow();

	SetDivider(Divider() + kTextControlDividerOffset);
}


void
TTextControl::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case B_SIMPLE_DATA: {
			if (fIncoming && !fResending)
				return;

			int32 buttons = -1;
			BPoint point;
			if (msg->FindInt32("buttons", &buttons) != B_OK)
				buttons = B_PRIMARY_MOUSE_BUTTON;

			if (buttons != B_PRIMARY_MOUSE_BUTTON
				&& msg->FindPoint("_drop_point_", &point) != B_OK)
				return;

			BMessage message(REFS_RECEIVED);
			bool enclosure = false;
			BString addressList;
				// Batch up the addresses to be added, since we can only
				// insert a few times before deadlocking since inserting
				// sends a notification message to the window BLooper,
				// which is busy doing this insert.  BeOS message queues
				// are annoyingly limited in their design.

			entry_ref ref;
			for (int32 index = 0;msg->FindRef("refs", index, &ref) == B_OK; index++) {
				BFile file(&ref, B_READ_ONLY);
				if (file.InitCheck() == B_NO_ERROR) {
					BNodeInfo info(&file);
					char type[B_FILE_NAME_LENGTH];
					info.GetType(type);

					if (fCommand != SUBJECT_FIELD
						&& !strcmp(type,"application/x-person")) {
						// add person's E-mail address to the To: field

						BString attr = "";
						if (buttons == B_PRIMARY_MOUSE_BUTTON) {
							if (msg->FindString("attr", &attr) < B_OK)
								attr = "META:email"; // If not META:email3 etc.
						} else {
							BNode node(&ref);
							node.RewindAttrs();

							char buffer[B_ATTR_NAME_LENGTH];

							delete fRefDropMenu;
							fRefDropMenu = new BPopUpMenu("RecipientMenu");

							while (node.GetNextAttrName(buffer) == B_OK) {
								if (strstr(buffer, "email") <= 0)
									continue;

								attr = buffer;

								BString address;
								node.ReadAttrString(buffer, &address);
								if (address.Length() <= 0)
									continue;

								BMessage *itemMsg = new BMessage(kMsgAddressChosen);
								itemMsg->AddString("address", address.String());
								itemMsg->AddRef("ref", &ref);

								BMenuItem *item = new BMenuItem(address.String(),
									itemMsg);
								fRefDropMenu->AddItem(item);
							}

							if (fRefDropMenu->CountItems() > 1) {
								fRefDropMenu->SetTargetForItems(this);
								fRefDropMenu->Go(point, true, true, true);
								return;
							} else {
								delete fRefDropMenu;
								fRefDropMenu = NULL;
							}
						}

						BString email;
						file.ReadAttrString(attr.String(), &email);

						// we got something...
						if (email.Length() > 0) {
							// see if we can get a username as well
							BString name;
							file.ReadAttrString("META:name", &name);

							BString	address;
							if (name.Length() == 0) {
								// if we have no Name, just use the email address
								address = email;
							} else {
								// otherwise, pretty-format it
								address << "\"" << name << "\" <" << email << ">";
							}

							if (addressList.Length() > 0)
								addressList << ", ";
							addressList << address;
						}
					} else {
						enclosure = true;
						message.AddRef("refs", &ref);
					}
				}
			}

			if (addressList.Length() > 0) {
				BTextView *textView = TextView();
				int end = textView->TextLength();
				if (end != 0) {
					textView->Select(end, end);
					textView->Insert(", ");
				}
				textView->Insert(addressList.String());
			}

			if (enclosure)
				Window()->PostMessage(&message, Window());
			break;
		}

		case M_SELECT:
		{
			BTextView *textView = (BTextView *)ChildAt(0);
			if (textView != NULL)
				textView->Select(0, textView->TextLength());
			break;
		}

		case kMsgAddressChosen: {
			BString display;
			BString address;
			entry_ref ref;

			if (msg->FindString("address", &address) != B_OK
				|| msg->FindRef("ref", &ref) != B_OK)
				return;

			if (address.Length() > 0) {
				BString name;
				BNode node(&ref);

				display = address;

				node.ReadAttrString("META:name", &name);
				if (name.Length() > 0) {
					display = "";
					display << "\"" << name << "\" <" << address << ">";
				}

				BTextView *textView = TextView();
				int end = textView->TextLength();
				if (end != 0) {
					textView->Select(end, end);
					textView->Insert(", ");
				}
				textView->Insert(display.String());
			}
			break;
		}

		default:
			// BTextControl::MessageReceived(msg);
			BComboBox::MessageReceived(msg);
	}
}


bool
TTextControl::HasFocus()
{
	return TextView()->IsFocus();
}


//	#pragma mark - QPopupMenu


QPopupMenu::QPopupMenu(const char *title)
	: QueryMenu(title, true),
	fGroups(0)
{
}


void
QPopupMenu::AddPersonItem(const entry_ref *ref, ino_t node, BString &name,
	BString &email, const char *attr, BMenu *groupMenu, BMenuItem *superItem)
{
	BString	label;
	BString	sortKey;
		// For alphabetical order sorting, usually last name.

	// if we have no Name, just use the email address
	if (name.Length() == 0) {
		label = email;
		sortKey = email;
	} else {
		// otherwise, pretty-format it
		label << name << " (" << email << ")";

		// Extract the last name (last word in the name),
		// removing trailing and leading spaces.
		const char *nameStart = name.String();
		const char *string = nameStart + strlen(nameStart) - 1;
		const char *wordEnd;

		while (string >= nameStart && isspace(*string))
			string--;
		wordEnd = string + 1; // Points to just after last word.
		while (string >= nameStart && !isspace(*string))
			string--;
		string++; // Point to first letter in the word.
		if (wordEnd > string)
			sortKey.SetTo(string, wordEnd - string);
		else // Blank name, pretend that the last name is after it.
			string = nameStart + strlen(nameStart);

		// Append the first names to the end, so that people with the same last
		// name get sorted by first name.  Note no space between the end of the
		// last name and the start of the first names, but that shouldn't
		// matter for sorting.
		sortKey.Append(nameStart, string - nameStart);
	}

	// The target (a TTextControl) will examine all the People files specified
	// and add the emails and names to the string it is displaying (same code
	// is used for drag and drop of People files).
	BMessage *msg = new BMessage(B_SIMPLE_DATA);
	msg->AddRef("refs", ref);
	msg->AddInt64("node", node);
	if (attr) // For nonstandard e-mail attributes, like META:email3
		msg->AddString("attr", attr);
	msg->AddString("sortkey", sortKey);

	BMenuItem *newItem = new BMenuItem(label.String(), msg);
	if (fTargetHandler)
		newItem->SetTarget(fTargetHandler);

	// If no group, just add it to ourself; else add it to group menu
	BMenu *parentMenu = groupMenu ? groupMenu : this;
	if (groupMenu) {
		// Add ref to group super item.
		BMessage *superMsg = superItem->Message();
		superMsg->AddRef("refs", ref);
	}

	// Add it to the appropriate menu.  Use alphabetical order by sortKey to
	// insert it in the right spot (a dumb linear search so this will be slow).
	// Start searching from the end of the menu, since the main menu includes
	// all the groups at the top and we don't want to mix it in with them.
	// Thus the search starts at the bottom and ends when we hit a separator
	// line or the top of the menu.

	int32 index = parentMenu->CountItems();
	while (index-- > 0) {
		BMenuItem *item = parentMenu->ItemAt(index);
		if (item == NULL ||	dynamic_cast<BSeparatorItem *>(item) != NULL)
			break;

		BMessage *message = item->Message();
		BString key;

		// Stop when testKey < sortKey.
		if (message != NULL
			&& message->FindString("sortkey", &key) == B_OK
			&& ICompare(key, sortKey) < 0)
			break;
	}

	if (!parentMenu->AddItem(newItem, index + 1)) {
		fprintf (stderr, "QPopupMenu::AddPersonItem: Unable to add menu "
			"item \"%s\" at index %ld.\n", sortKey.String(), index + 1);
		delete newItem;
	}
}


void
QPopupMenu::EntryCreated(const entry_ref &ref, ino_t node)
{
	BNode file;
	if (file.SetTo(&ref) < B_OK)
		return;

	// Make sure the pop-up menu is ready for additions.  Need a bunch of
	// groups at the top, a divider line, and miscellaneous people added below
	// the line.

	int32 items = CountItems();
	if (!items)
		AddSeparatorItem();

	// Does the file have a group attribute?  OK to have none.
	BString groups;
	const char *kNoGroup = "NoGroup!";
	file.ReadAttrString("META:group", &groups);
	if (groups.Length() <= 0)
		groups = kNoGroup;

	// Add the e-mail address to the all people group.  Then add it to all the
	// group menus that it exists in (based on the comma separated list of
	// groups from the People file), optionally making the group menu if it
	// doesn't exist.  If it's in the special NoGroup!  list, then add it below
	// the groups.

	bool allPeopleGroupDone = false;
	BMenu *groupMenu;
	do {
		BString group;

		if (!allPeopleGroupDone) {
			// Create the default group for all people, if it doesn't exist yet.
			group = "All People";
			allPeopleGroupDone = true;
		} else {
			// Break out the next group from the comma separated string.
			int32 comma;
			if ((comma = groups.FindFirst(',')) > 0) {
				groups.MoveInto(group, 0, comma);
				groups.Remove(0, 1);
			} else
				group.Adopt(groups);
		}

		// trim white spaces
		int32 i = 0;
		for (i = 0; isspace(group.ByteAt(i)); i++) {}
		if (i)
			group.Remove(0, i);
		for (i = group.Length() - 1; isspace(group.ByteAt(i)); i--) {}
		group.Truncate(i + 1);

		groupMenu = NULL;
		BMenuItem *superItem = NULL; // Corresponding item for group menu.

		if (group.Length() > 0 && group != kNoGroup) {
			BMenu *sub;

			// Look for submenu with label == group name
			for (int32 i = 0; i < items; i++) {
				if ((sub = SubmenuAt(i)) != NULL) {
					superItem = sub->Superitem();
					if (!strcmp(superItem->Label(), group.String())) {
						groupMenu = sub;
						i++;
						break;
					}
				}
			}

			// If no submenu, create one
			if (!groupMenu) {
				// Find where it should go (alphabetical)
				int32 mindex = 0;
				for (; mindex < fGroups; mindex++) {
					if (strcmp(ItemAt(mindex)->Label(), group.String()) > 0)
						break;
				}

				groupMenu = new BMenu(group.String());
				groupMenu->SetFont(be_plain_font);
				AddItem(groupMenu, mindex);

				superItem = groupMenu->Superitem();
				superItem->SetMessage(new BMessage(B_SIMPLE_DATA));
				if (fTargetHandler)
					superItem->SetTarget(fTargetHandler);

				fGroups++;
			}
		}

		BString	name;
		file.ReadAttrString("META:name", &name);

		BString email;
		file.ReadAttrString("META:email", &email);

		if (email.Length() != 0 || name.Length() != 0)
			AddPersonItem(&ref, node, name, email, NULL, groupMenu, superItem);

		// support for 3rd-party People apps
		for (int16 i = 2; i < 6; i++) {
			char attr[16];
			sprintf(attr, "META:email%d", i);
			if (file.ReadAttrString(attr, &email) >= B_OK && email.Length() > 0)
				AddPersonItem(&ref, node, name, email, attr, groupMenu, superItem);
		}
	} while (groups.Length() > 0);
}


void
QPopupMenu::EntryRemoved(ino_t /*node*/)
{
}

