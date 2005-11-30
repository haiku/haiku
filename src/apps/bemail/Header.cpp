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
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

BeMail(TM), Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

//--------------------------------------------------------------------
//	
//	Header.cpp
//
//--------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include <StorageKit.h>
#include <InterfaceKit.h>
#include <StringView.h>
#include <E-mail.h>
#include <String.h>
#include <PopUpMenu.h>
#include <fs_index.h>
#include <fs_info.h>
#include <map>

#include <MailSettings.h>
#include <MailMessage.h>

#include <String.h>
#include <CharacterSet.h>
#include <CharacterSetRoster.h>

using namespace BPrivate;
using std::map;

#include "Mail.h"
#include "Header.h"
#include "Utilities.h"
#include "QueryMenu.h"
#include "FieldMsg.h"
#include "Prefs.h"

#include <MDRLanguage.h>

extern uint32 gDefaultChain;

const char	*kDateLabel = "Date:";
const uint32 kMsgFrom = 'hFrm';
const uint32 kMsgEncoding = 'encd';
const uint32 kMsgAddressChosen = 'acsn';


class QPopupMenu : public QueryMenu {
	public:
		QPopupMenu(const char *title);

	private:		
		void AddPersonItem(const entry_ref *ref, ino_t node, BString &name, BString &email,
							const char *attr, BMenu *groupMenu, BMenuItem *superItem);

	protected:
		virtual void EntryCreated(const entry_ref &ref, ino_t node);
		virtual void EntryRemoved(ino_t node);

		int32 fGroups; // Current number of "group" submenus.  Includes All People if present.
};

//====================================================================

struct CompareBStrings
{
	bool operator()(const BString *s1, const BString *s2) const
	{
		return (s1->Compare(*s2) < 0);
	}
};

//====================================================================


THeaderView::THeaderView (
	BRect rect,
	BRect windowRect,
	bool incoming,
	BEmailMessage *mail,
	bool resending,
	uint32 defaultCharacterSet
	) :	BBox(rect, "m_header", B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW, B_NO_BORDER),
		fAccountMenu(NULL),
		fEncodingMenu(NULL),
		fChain(gDefaultChain),
		fAccountTo(NULL),
		fAccount(NULL),
		fBcc(NULL),
		fCc(NULL),
		fSubject(NULL),
		fTo(NULL),
		fDate(NULL),
		fIncoming(incoming),
		fCharacterSetUserSees(defaultCharacterSet),
		fResending(resending),
		fBccMenu(NULL),
		fCcMenu(NULL),
		fToMenu(NULL),
		fEmailList(NULL)
{
	BMenuField	*field;
	BMessage *msg;

	BFont font = *be_plain_font;
	font.SetSize(FONT_SIZE);
	SetFont(&font);
	float x = font.StringWidth( /* The longest title string in the header area */
		MDR_DIALECT_CHOICE ("Enclosures: ","添付ファイル：")) + 9;
	float y = TO_FIELD_V;

	if (!fIncoming)
	{
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
	bool marked_charset = false;
	BMenuItem * item;

	fEncodingMenu = new BPopUpMenu (B_EMPTY_STRING);

	BCharacterSetRoster roster;
	BCharacterSet charset;
	while (roster.GetNextCharacterSet(&charset) == B_NO_ERROR) {
		BString name(charset.GetPrintName());
		const char * mime = charset.GetMIMEName();
		if (mime) {
			name << " (" << mime << ")";
		}
		msg = new BMessage(kMsgEncoding);
		uint32 convert_id;
		if ((mime == 0) || (strcasecmp(mime, "UTF-8") != 0)) {
			convert_id = charset.GetConversionID();
		} else {
			convert_id = B_MAIL_UTF8_CONVERSION;
		}
		msg->AddInt32("charset", convert_id);
		fEncodingMenu->AddItem(item = new BMenuItem(name.String(), msg));
		if (convert_id == fCharacterSetUserSees && !marked_charset) {
			item->SetMarked(true);
			marked_charset = true;
		}
		if (font.StringWidth(name.String()) > widestCharacterSet)
			widestCharacterSet = font.StringWidth(name.String());
	}

	msg = new BMessage(kMsgEncoding);
	msg->AddInt32("charset", B_MAIL_US_ASCII_CONVERSION);
	fEncodingMenu->AddItem(item = new BMenuItem("US-ASCII", msg));
	if (fCharacterSetUserSees == B_MAIL_US_ASCII_CONVERSION && !marked_charset) {
		item->SetMarked(true);
		marked_charset = true;
	}

	if (!resending && fIncoming) {
		// reading a message, display the Automatic item
		fEncodingMenu->AddSeparatorItem();
		msg = new BMessage(kMsgEncoding);
		msg->AddInt32("charset", B_MAIL_NULL_CONVERSION);
		fEncodingMenu->AddItem(item = new BMenuItem("Automatic", msg));
		if (!marked_charset) {
			item->SetMarked(true);
		}
	}

	// First line of the header, From for reading e-mails (includes the
	// character set choice at the right), To when composing (nothing else in
	// the row).

	BRect r;
	char string[20];
	if (fIncoming && !resending)
	{
		// Set up the character set pop-up menu on the right of "To" box.
		r.Set (windowRect.Width() - widestCharacterSet -
			font.StringWidth (DECODING_TEXT) - 2 * SEPARATOR_MARGIN,
			y - 2, windowRect.Width() - SEPARATOR_MARGIN, y + TO_FIELD_HEIGHT + 2);
		field = new BMenuField (r, "decoding", DECODING_TEXT, fEncodingMenu,
			true /* fixedSize */,
			B_FOLLOW_TOP | B_FOLLOW_RIGHT,
			B_WILL_DRAW | B_NAVIGABLE | B_NAVIGABLE_JUMP);
		field->SetFont (&font);
		field->SetDivider (font.StringWidth(DECODING_TEXT) + 5);
		AddChild(field);
		r.Set(x - font.StringWidth(FROM_TEXT) - 11, y,
			  field->Frame().left - SEPARATOR_MARGIN, y + TO_FIELD_HEIGHT);
		sprintf(string, FROM_TEXT);
	}
	else
	{
		r.Set(x - 11, y, windowRect.Width() - SEPARATOR_MARGIN, y + TO_FIELD_HEIGHT);
		string[0] = 0;
	}
	y += FIELD_HEIGHT;
	fTo = new TTextControl(r, string, new BMessage(TO_FIELD), fIncoming, resending,
						   B_FOLLOW_LEFT_RIGHT);

	if (!fIncoming || resending)
	{
		fTo->SetChoiceList(&fEmailList);
		fTo->SetAutoComplete(true);
	}
	AddChild(fTo);
	msg = new BMessage(FIELD_CHANGED);
	msg->AddInt32("bitmask", FIELD_TO);
	fTo->SetModificationMessage(msg);

	if (!fIncoming || resending)
	{
		r.right = r.left + 8;
		r.left = r.right - be_plain_font->StringWidth(TO_TEXT) - 30;
		r.top -= 1;
		fToMenu = new QPopupMenu(TO_TEXT);
		field = new BMenuField(r, "", "", fToMenu, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
		field->SetDivider(0.0);
		field->SetEnabled(true);
		AddChild(field);
	}

	// "From:" accounts Menu and Encoding Menu.
	if (!fIncoming || resending)
	{
		// Put the character set box on the right of the From field.
		r.Set (windowRect.Width() - widestCharacterSet -
			font.StringWidth (ENCODING_TEXT) - 2 * SEPARATOR_MARGIN,
			y - 2, windowRect.Width() - SEPARATOR_MARGIN, y + TO_FIELD_HEIGHT + 2);
		field = new BMenuField (r, "encoding", ENCODING_TEXT, fEncodingMenu,
			true /* fixedSize */,
			B_FOLLOW_TOP | B_FOLLOW_LEFT,
			B_WILL_DRAW | B_NAVIGABLE | B_NAVIGABLE_JUMP);
		field->SetFont (&font);
		field->SetDivider (font.StringWidth(ENCODING_TEXT) + 5);
		AddChild(field);

		// And now the "from account" pop-up menu, on the left side, taking the
		// remaining space.

		fAccountMenu = new BPopUpMenu(B_EMPTY_STRING);

		BList chains;
		if (GetOutboundMailChains(&chains) >= B_OK)
		{
			bool marked = false;
			for (int32 i = 0;i < chains.CountItems();i++)
			{
				BMailChain *chain = (BMailChain *)chains.ItemAt(i);
				BString name = chain->Name();
				if ((msg = chain->MetaData()) != NULL)
				{
					name << ":   " << msg->FindString("real_name")
						 << "  <" << msg->FindString("reply_to") << ">";
				}
				BMenuItem *item = new BMenuItem(name.String(),msg = new BMessage(kMsgFrom));

				msg->AddInt32("id",chain->ID());

				if (gDefaultChain == chain->ID())
				{
					item->SetMarked(true);
					marked = true;
				}
				fAccountMenu->AddItem(item);
				delete chain;
			}
			if (!marked)
			{
				BMenuItem *item = fAccountMenu->ItemAt(0);
				if (item != NULL)
				{
					item->SetMarked(true);
					fChain = item->Message()->FindInt32("id");
				}
				else
				{
					fAccountMenu->AddItem(item = new BMenuItem("<none>",NULL));
					item->SetEnabled(false);
					fChain = ~0UL;
				}
				// default chain is invalid, set to marked
				gDefaultChain = fChain;
			}
		}
		r.Set(x - font.StringWidth(FROM_TEXT) - 11, y - 2,
			  field->Frame().left - SEPARATOR_MARGIN, y + TO_FIELD_HEIGHT + 2);
		field = new BMenuField(r, "account", FROM_TEXT, fAccountMenu,
			true /* fixedSize */,
			B_FOLLOW_TOP | B_FOLLOW_LEFT,
			B_WILL_DRAW | B_NAVIGABLE | B_NAVIGABLE_JUMP);
		field->SetFont(&font);
		field->SetDivider(font.StringWidth(FROM_TEXT) + 11);
		AddChild(field);

		y += FIELD_HEIGHT;
	}
	else	// To: account
	{
		bool account = count_pop_accounts() > 0;

		r.Set(x - font.StringWidth(TO_TEXT) - 11, y,
			  windowRect.Width() - SEPARATOR_MARGIN, y + TO_FIELD_HEIGHT);
		if (account)
			r.right -= SEPARATOR_MARGIN + ACCOUNT_FIELD_WIDTH;
		fAccountTo = new TTextControl(r, TO_TEXT, NULL, fIncoming, false, B_FOLLOW_LEFT_RIGHT);
		fAccountTo->SetEnabled(false);
		AddChild(fAccountTo);

		if (account)
		{
			r.left = r.right + 6;  r.right = windowRect.Width() - SEPARATOR_MARGIN;
			fAccount = new TTextControl(r, ACCOUNT_TEXT, NULL, fIncoming, false, B_FOLLOW_RIGHT | B_FOLLOW_TOP);
			fAccount->SetEnabled(false);
			AddChild(fAccount);
		}
		y += FIELD_HEIGHT;
	}

	--y;
	r.Set(x - font.StringWidth(SUBJECT_TEXT) - 11, y,
		  windowRect.Width() - SEPARATOR_MARGIN, y + TO_FIELD_HEIGHT);
	y += FIELD_HEIGHT;
	fSubject = new TTextControl(r, SUBJECT_TEXT, new BMessage(SUBJECT_FIELD),
				fIncoming, false, B_FOLLOW_LEFT_RIGHT);
	AddChild(fSubject);
	(msg = new BMessage(FIELD_CHANGED))->AddInt32("bitmask", FIELD_SUBJECT);
	fSubject->SetModificationMessage(msg);
	
	if (fResending)
		fSubject->SetEnabled(false);
	
	--y;
	if (!fIncoming)
	{
		r.Set(x - 11, y, CC_FIELD_H + CC_FIELD_WIDTH, y + CC_FIELD_HEIGHT);
		fCc = new TTextControl(r, "", new BMessage(CC_FIELD), fIncoming, false);
		fCc->SetChoiceList(&fEmailList);
		fCc->SetAutoComplete(true);
		AddChild(fCc);
		(msg = new BMessage(FIELD_CHANGED))->AddInt32("bitmask", FIELD_CC);
		fCc->SetModificationMessage(msg);
	
		r.right = r.left + 9;
		r.left = r.right - be_plain_font->StringWidth(CC_TEXT) - 30;
		r.top -= 1;
		fCcMenu = new QPopupMenu(CC_TEXT);
		field = new BMenuField(r, "", "", fCcMenu, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);

		field->SetDivider(0.0);
		field->SetEnabled(true);
		AddChild(field);

		r.Set(BCC_FIELD_H + be_plain_font->StringWidth(BCC_TEXT), y,
			  windowRect.Width() - SEPARATOR_MARGIN, y + BCC_FIELD_HEIGHT);
		y += FIELD_HEIGHT;
		fBcc = new TTextControl(r, "", new BMessage(BCC_FIELD),
						fIncoming, false, B_FOLLOW_LEFT_RIGHT);
		fBcc->SetChoiceList(&fEmailList);
		fBcc->SetAutoComplete(true);
		AddChild(fBcc);
		(msg = new BMessage(FIELD_CHANGED))->AddInt32("bitmask", FIELD_BCC);
		fBcc->SetModificationMessage(msg);
		
		r.right = r.left + 9;
		r.left = r.right - be_plain_font->StringWidth(BCC_TEXT) - 30;
		r.top -= 1;
		fBccMenu = new QPopupMenu(BCC_TEXT);
		field = new BMenuField(r, "", "", fBccMenu, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
		field->SetDivider(0.0);
		field->SetEnabled(true);
		AddChild(field);
	}
	else
	{
		r.Set(x - font.StringWidth(kDateLabel) - 10, y + 4,
			  windowRect.Width(), y + TO_FIELD_HEIGHT + 1);
		y += TO_FIELD_HEIGHT + 5;
		fDate = new BStringView(r, "", "");
		AddChild(fDate);	
		fDate->SetFont(&font);	
		fDate->SetHighColor(0, 0, 0);

		LoadMessage(mail);
	}
	ResizeTo(Bounds().Width(),y);
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

	while (query.GetNextRef (&ref) == B_OK)
	{
		BNode file;
		if (file.SetTo(&ref) == B_OK)
		{
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
			for (int16 i = 2;i < 6;i++)
			{
				char attr[16];
				sprintf(attr,"META:email%d",i);
				if (file.ReadAttrString(attr,&email) >= B_OK)
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

	map<BString *, BString *, CompareBStrings> group_map;
	entry_ref ref;
	BNode file;
	while (query.GetNextRef(&ref) == B_OK)
	{
		if (file.SetTo(&ref) != B_OK)
			continue;

		BString groups;
		if (ReadAttrString(&file, "META:group", &groups) < B_OK || groups.Length() == 0)
			continue;

		BString address;
		ReadAttrString(&file, "META:email", &address);

		// avoid adding an empty address
		if (address.Length() == 0)
			continue;

		char *grp = groups.LockBuffer(groups.Length());
		char *next = strchr(grp, ',');

		for (;;)
		{
			if (next) *next = 0;
			while (*grp && *grp == ' ') grp++;

			BString *group = new BString(grp);
			BString *addressListString = NULL;

			// nobody is in this group yet, start it off
			if (group_map[group] == NULL)
			{
				addressListString = new BString(*group);
				addressListString->Append(" ");
				group_map[group] = addressListString;
			}
			else
			{
				addressListString = group_map[group];
				addressListString->Append(", ");
				delete group;
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
	
			grp = next+1;
			next = strchr(grp, ',');
		}
	}
	
	map<BString *, BString *, CompareBStrings>::iterator iter;
	for (iter = group_map.begin(); iter != group_map.end();)
	{
		BString *grp = iter->first;
		BString *addr = iter->second;
		fEmailList.AddChoice(addr->String());
		++iter;
		group_map.erase(grp);
		delete grp;
		delete addr;
	}
}


void
THeaderView::MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case B_SIMPLE_DATA:
		{
			BTextView *textView = dynamic_cast<BTextView *>(Window()->CurrentFocus());
			if (dynamic_cast<TTextControl *>(textView->Parent()) != NULL)
				textView->Parent()->MessageReceived(msg);
			else
			{
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

			uint32 chain;
			if (msg->FindInt32("id",(int32 *)&chain) >= B_OK)
				fChain = chain;
			break;
		}

		case kMsgEncoding:
		{
			BMessage message(*msg);
			int32 tempInt;

			if (msg->FindInt32("charset", &tempInt) == B_OK)
				fCharacterSetUserSees = tempInt;

			message.what = CHARSET_CHOICE_MADE;
			message.AddInt32 ("charset", fCharacterSetUserSees);
			Window()->PostMessage (&message, Window());
			break;
		}
	}
}


void
THeaderView::AttachedToWindow(void)
{
	if (fToMenu)
	{
		fToMenu->SetTargetForItems(fTo);
		fToMenu->SetPredicate("META:email=**");
	}
	if (fCcMenu)
	{
		fCcMenu->SetTargetForItems(fCc);
		fCcMenu->SetPredicate("META:email=**");
	}
	if (fBccMenu)
	{
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
	//
	//	Set the date on this message
	//
	const char *dateField = mail->Date();
	char string[256];
	sprintf(string, "%s   %s", kDateLabel, dateField != NULL ? dateField : "Unknown");
	fDate->SetText(string);

	//	
	//	Set contents of header fields
	//
	if (fIncoming && !fResending)
	{
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

	if (fAccount != NULL && mail->GetAccountName(string,sizeof(string)) == B_OK)
		fAccount->SetText(string);

	return B_OK;
}


//====================================================================
//	#pragma mark -


TTextControl::TTextControl(BRect rect, char *label, BMessage *msg, 
	bool incoming, bool resending, int32 resizingMode)
	:	BComboBox(rect, "happy", label, msg, resizingMode),
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
	BFont font = *be_plain_font;
	BTextView *text;

	SetHighColor(0, 0, 0);
	// BTextControl::AttachedToWindow();
	BComboBox::AttachedToWindow();
	font.SetSize(FONT_SIZE);
	SetFont(&font);

	SetDivider(StringWidth(fLabel) + 6);
	text = (BTextView *)ChildAt(0);
	text->SetFont(&font);
}


void
TTextControl::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case B_SIMPLE_DATA: {
			if ((fIncoming == true) && (fResending == false)) return;
			
			int32 buttons = -1;
			BPoint point;

			if (msg->FindInt32("buttons", &buttons) != B_OK) {
				buttons = B_PRIMARY_MOUSE_BUTTON;
			};
			if (buttons != B_PRIMARY_MOUSE_BUTTON) {
				if (msg->FindPoint("_drop_point_", &point) != B_OK) return;
			};
		
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

					if (fCommand != SUBJECT_FIELD &&
						!strcmp(type,"application/x-person")) {
						// add person's E-mail address to the To: field

						BString attr = "";
						if (buttons == B_PRIMARY_MOUSE_BUTTON) {
						
							if (msg->FindString("attr", &attr) < B_OK)
								attr = "META:email"; // If not META:email3 etc.
						} else {
							BNode node(&ref);
							node.RewindAttrs();

							char buffer[B_ATTR_NAME_LENGTH];

							if (fRefDropMenu) delete fRefDropMenu;
							fRefDropMenu = new BPopUpMenu("RecipientMenu");
							
							while (node.GetNextAttrName(buffer) == B_OK) {
								if (strstr(buffer, "email") <= 0) continue;
								attr = buffer;

								BString address;
								ReadAttrString(&node, buffer, &address);
								if (address.Length() <= 0) continue;

								BMessage *itemMsg = new BMessage(kMsgAddressChosen);
								itemMsg->AddString("address", address.String());
								itemMsg->AddRef("ref", &ref);

								BMenuItem *item = new BMenuItem(address.String(),
									itemMsg);
								fRefDropMenu->AddItem(item);
							};
							
							if (fRefDropMenu->CountItems() > 1) {
								fRefDropMenu->SetTargetForItems(this);
								fRefDropMenu->Go(point, true, true, true);
								return;
							} else {
								delete fRefDropMenu;
								fRefDropMenu = NULL;
							};
						};

						BString email;
						ReadAttrString(&file,attr.String(),&email);

						/* we got something... */
						if (email.Length() > 0) {
							/* see if we can get a username as well */
							BString name;
							ReadAttrString(&file,"META:name",&name);

							BString	address;
							/* if we have no Name, just use the email address */
							if (name.Length() == 0)
								address = email;
							else {
								/* otherwise, pretty-format it */
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

			if (enclosure) Window()->PostMessage(&message, Window());

			break;
		};

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
			
			if (msg->FindString("address", &address) != B_OK) return;
			if (msg->FindRef("ref", &ref) != B_OK) return;
			
			if (address.Length() > 0) {
				BString name;
				BNode node(&ref);

				display = address;

				ReadAttrString(&node, "META:name", &name);
				if (name.Length() > 0) {
					display = "";
					display << "\"" << name << "\" <" << address << ">";
				};

				BTextView *textView = TextView();
				int end = textView->TextLength();
				if (end != 0) {
					textView->Select(end, end);
					textView->Insert(", ");
				};
				textView->Insert(display.String());

			};
		} break;

		default:
			// BTextControl::MessageReceived(msg);
			BComboBox::MessageReceived(msg);
	}
}


bool
TTextControl::HasFocus()
{
	BTextView *textView = TextView();

	return textView->IsFocus();
}


//====================================================================
//	QPopupMenu (definition at the beginning of this file)
//	#pragma mark -


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
	ReadAttrString(&file, "META:group", &groups);
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
		ReadAttrString(&file, "META:name", &name);

		BString email;
		ReadAttrString(&file, "META:email", &email);

		if (email.Length() != 0 || name.Length() != 0)
			AddPersonItem(&ref, node, name, email, NULL, groupMenu, superItem);

		// support for 3rd-party People apps
		for (int16 i = 2; i < 6; i++) {
			char attr[16];
			sprintf(attr, "META:email%d", i);
			if (ReadAttrString(&file, attr, &email) >= B_OK && email.Length() > 0)
				AddPersonItem(&ref, node, name, email, attr, groupMenu, superItem);
		}
	} while (groups.Length() > 0);
}


void
QPopupMenu::EntryRemoved(ino_t /*node*/)
{
}
