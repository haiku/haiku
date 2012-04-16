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


#include "Prefs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Application.h>
#include <CharacterSet.h>
#include <CharacterSetRoster.h>
#include <E-mail.h>
#include <GridView.h>
#include <GroupLayoutBuilder.h>
#include <InterfaceKit.h>
#include <Locale.h>
#include <MailSettings.h>
#include <mail_encoding.h>
#include <StorageKit.h>
#include <String.h>

using namespace BPrivate;

#include "MailApp.h"
#include "MailSupport.h"
#include "MailWindow.h"
#include "Messages.h"
#include "Signature.h"


#define B_TRANSLATION_CONTEXT "Mail"

#define BUTTON_WIDTH		70
#define BUTTON_HEIGHT		20
#define ITEM_SPACE			6

#define OK_BUTTON_X1		(PREF_WIDTH - BUTTON_WIDTH - 6)
#define OK_BUTTON_X2		(OK_BUTTON_X1 + BUTTON_WIDTH)

#define REVERT_BUTTON_X1	8
#define REVERT_BUTTON_X2	(REVERT_BUTTON_X1 + BUTTON_WIDTH)

enum	P_MESSAGES			{P_OK = 128, P_CANCEL, P_REVERT, P_FONT,
							 P_SIZE, P_LEVEL, P_WRAP, P_ATTACH_ATTRIBUTES,
							 P_SIG, P_ENC, P_WARN_UNENCODABLE,
							 P_SPELL_CHECK_START_ON, P_BUTTON_BAR,
							 P_ACCOUNT, P_REPLYTO, P_REPLY_PREAMBLE,
							 P_COLORED_QUOTES, P_MARK_READ};

extern BPoint	prefs_window;

//#pragma mark -


static inline void
add_menu_to_layout(BMenuField* menu, BGridLayout* layout, int32& row)
{
	menu->SetAlignment(B_ALIGN_RIGHT);
	layout->AddItem(menu->CreateLabelLayoutItem(), 0, row);
	layout->AddItem(menu->CreateMenuBarLayoutItem(), 1, row, 2);
	row++;
}


TPrefsWindow::TPrefsWindow(BRect rect, BFont* font, int32* level, bool* wrap,
	bool* attachAttributes, bool* cquotes, int32* account, int32* replyTo,
	char** preamble, char** sig, uint32* encoding, bool* warnUnencodable,
	bool* spellCheckStartOn, bool* autoMarkRead, uint8* buttonBar)
	:
	BWindow(rect, B_TRANSLATE("Mail preferences"),
		B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE
			| B_AUTO_UPDATE_SIZE_LIMITS),

	fNewWrap(wrap),
	fWrap(*fNewWrap),

	fNewAttachAttributes(attachAttributes),
	fAttachAttributes(*fNewAttachAttributes),

	fNewButtonBar(buttonBar),
	fButtonBar(*fNewButtonBar),

	fNewColoredQuotes(cquotes),
	fColoredQuotes(*fNewColoredQuotes),

	fNewAccount(account),
	fAccount(*fNewAccount),

	fNewReplyTo(replyTo),
	fReplyTo(*fNewReplyTo),

	fNewPreamble(preamble),

	fNewSignature(sig),
	fSignature((char*)malloc(strlen(*fNewSignature) + 1)),

	fNewFont(font),
	fFont(*fNewFont),

	fNewEncoding(encoding),
	fEncoding(*fNewEncoding),

	fNewWarnUnencodable(warnUnencodable),
	fWarnUnencodable(*fNewWarnUnencodable),

	fNewSpellCheckStartOn(spellCheckStartOn),
	fSpellCheckStartOn(*fNewSpellCheckStartOn),

	fNewAutoMarkRead(autoMarkRead),
	fAutoMarkRead(*autoMarkRead)
{
	strcpy(fSignature, *fNewSignature);

	BMenuField* menu;

	// group boxes

	const float kSpacing = 8;

	BGridView* interfaceView = new BGridView(kSpacing, kSpacing);
	BGridLayout* interfaceLayout = interfaceView->GridLayout();
	interfaceLayout->SetInsets(kSpacing, kSpacing, kSpacing, kSpacing);
	BGridView* mailView = new BGridView(kSpacing, kSpacing);
	BGridLayout* mailLayout = mailView->GridLayout();
	mailLayout->SetInsets(kSpacing, kSpacing, kSpacing, kSpacing);

	interfaceLayout->AlignLayoutWith(mailLayout, B_HORIZONTAL);

	BBox* interfaceBox = new BBox(B_FANCY_BORDER, interfaceView);
	interfaceBox->SetLabel(B_TRANSLATE("User interface"));
	BBox* mailBox = new BBox(B_FANCY_BORDER, mailView);
	mailBox->SetLabel(B_TRANSLATE("Mailing"));

	// revert, ok & cancel

	BButton* okButton = new BButton("ok", B_TRANSLATE("OK"),
		new BMessage(P_OK));
	okButton->MakeDefault(true);

	BButton* cancelButton = new BButton("cancel", B_TRANSLATE("Cancel"),
		new BMessage(P_CANCEL));

	fRevert = new BButton("revert", B_TRANSLATE("Revert"),
		new BMessage(P_REVERT));
	fRevert->SetEnabled(false);

	// User Interface
	int32 layoutRow = 0;

	fButtonBarMenu = _BuildButtonBarMenu(*buttonBar);
	menu = new BMenuField("bar", B_TRANSLATE("Button bar:"), fButtonBarMenu);
	add_menu_to_layout(menu, interfaceLayout, layoutRow);

	fFontMenu = _BuildFontMenu(font);
	menu = new BMenuField("font", B_TRANSLATE("Font:"), fFontMenu);
	add_menu_to_layout(menu, interfaceLayout, layoutRow);

	fSizeMenu = _BuildSizeMenu(font);
	menu = new BMenuField("size", B_TRANSLATE("Size:"), fSizeMenu);
	add_menu_to_layout(menu, interfaceLayout, layoutRow);

	fColoredQuotesMenu = _BuildColoredQuotesMenu(fColoredQuotes);
	menu = new BMenuField("cquotes", B_TRANSLATE("Colored quotes:"),
		fColoredQuotesMenu);
	add_menu_to_layout(menu, interfaceLayout, layoutRow);

	fSpellCheckStartOnMenu = _BuildSpellCheckStartOnMenu(fSpellCheckStartOn);
	menu = new BMenuField("spellCheckStartOn",
		B_TRANSLATE("Initial spell check mode:"),
		fSpellCheckStartOnMenu);
	add_menu_to_layout(menu, interfaceLayout, layoutRow);

	fAutoMarkReadMenu = _BuildAutoMarkReadMenu(fAutoMarkRead);
	menu = new BMenuField("autoMarkRead",
		B_TRANSLATE("Automatically mark mail as read:"),
		fAutoMarkReadMenu);
	add_menu_to_layout(menu, interfaceLayout, layoutRow);
	// Mail Accounts

	layoutRow = 0;

	fAccountMenu = _BuildAccountMenu(fAccount);
	menu = new BMenuField("account", B_TRANSLATE("Default account:"),
		fAccountMenu);
	add_menu_to_layout(menu, mailLayout, layoutRow);

	fReplyToMenu = _BuildReplyToMenu(fReplyTo);
	menu = new BMenuField("replyTo", B_TRANSLATE("Reply account:"),
		fReplyToMenu);
	add_menu_to_layout(menu, mailLayout, layoutRow);

	// Mail Contents

	fReplyPreamble = new BTextControl("replytext",
		B_TRANSLATE("Reply preamble:"),
		*preamble, new BMessage(P_REPLY_PREAMBLE));
	fReplyPreamble->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);

	fReplyPreambleMenu = _BuildReplyPreambleMenu();
	menu = new BMenuField("replyPreamble", NULL, fReplyPreambleMenu);
	menu->SetExplicitMaxSize(BSize(27, B_SIZE_UNSET));

	mailLayout->AddItem(fReplyPreamble->CreateLabelLayoutItem(), 0, layoutRow);
	mailLayout->AddItem(fReplyPreamble->CreateTextViewLayoutItem(), 1,
		layoutRow);
	mailLayout->AddView(menu, 2, layoutRow);
	layoutRow++;

	fSignatureMenu = _BuildSignatureMenu(*sig);
	menu = new BMenuField("sig", B_TRANSLATE("Auto signature:"),
		fSignatureMenu);
	add_menu_to_layout(menu, mailLayout, layoutRow);

	fEncodingMenu = _BuildEncodingMenu(fEncoding);
	menu = new BMenuField("enc", B_TRANSLATE("Encoding:"), fEncodingMenu);
	add_menu_to_layout(menu, mailLayout, layoutRow);

	fWarnUnencodableMenu = _BuildWarnUnencodableMenu(fWarnUnencodable);
	menu = new BMenuField("warnUnencodable", B_TRANSLATE("Warn unencodable:"),
		fWarnUnencodableMenu);
	add_menu_to_layout(menu, mailLayout, layoutRow);

	fWrapMenu = _BuildWrapMenu(*wrap);
	menu = new BMenuField("wrap", B_TRANSLATE("Text wrapping:"), fWrapMenu);
	add_menu_to_layout(menu, mailLayout, layoutRow);

	fAttachAttributesMenu = _BuildAttachAttributesMenu(*attachAttributes);
	menu = new BMenuField("attachAttributes", B_TRANSLATE("Attach attributes:"),
		fAttachAttributesMenu);
	add_menu_to_layout(menu, mailLayout, layoutRow);

	SetLayout(new BGroupLayout(B_HORIZONTAL));

	AddChild(BGroupLayoutBuilder(B_VERTICAL, kSpacing)
		.Add(interfaceBox)
		.Add(mailBox)
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, kSpacing)
			.Add(fRevert)
			.AddGlue()
			.Add(cancelButton)
			.Add(okButton)
		)
		.SetInsets(kSpacing, kSpacing, kSpacing, kSpacing)
	);

	Show();
}


TPrefsWindow::~TPrefsWindow()
{
	BMessage msg(WINDOW_CLOSED);
	msg.AddInt32("kind", PREFS_WINDOW);
	msg.AddPoint("window pos", Frame().LeftTop());
	be_app->PostMessage(&msg);
}


void
TPrefsWindow::MessageReceived(BMessage* msg)
{
	bool revert = true;
	const char* family;
	const char* signature;
	const char* style;
	char label[256];
	int32 new_size;
	int32 old_size;
	font_family new_family;
	font_family old_family;
	font_style new_style;
	font_style old_style;
	BMenuItem* item;
	BMessage message;

	switch (msg->what) {
		case P_OK:
			if (strcmp(fReplyPreamble->Text(), *fNewPreamble)) {
				free(*fNewPreamble);
				*fNewPreamble
					= (char *)malloc(strlen(fReplyPreamble->Text()) + 1);
				strcpy(*fNewPreamble, fReplyPreamble->Text());
			}
			be_app->PostMessage(PREFS_CHANGED);
			Quit();
			break;

		case P_CANCEL:
			revert = false;
			// supposed to fall through
		case P_REVERT:
			fFont.GetFamilyAndStyle(&old_family, &old_style);
			fNewFont->GetFamilyAndStyle(&new_family, &new_style);
			old_size = (int32)fFont.Size();
			new_size = (int32)fNewFont->Size();
			if (strcmp(old_family, new_family) || strcmp(old_style, new_style)
				|| old_size != new_size) {
				fNewFont->SetFamilyAndStyle(old_family, old_style);
				if (revert) {
					sprintf(label, "%s %s", old_family, old_style);
					item = fFontMenu->FindItem(label);
					if (item != NULL)
						item->SetMarked(true);
				}

				fNewFont->SetSize(old_size);
				if (revert) {
					sprintf(label, "%ld", old_size);
					item = fSizeMenu->FindItem(label);
					if (item != NULL)
						item->SetMarked(true);
				}
				message.what = M_FONT;
				be_app->PostMessage(&message);
			}
			*fNewWrap = fWrap;
			*fNewAttachAttributes = fAttachAttributes;

			if (strcmp(fSignature, *fNewSignature)) {
				free(*fNewSignature);
				*fNewSignature = (char*)malloc(strlen(fSignature) + 1);
				strcpy(*fNewSignature, fSignature);
			}

			*fNewEncoding = fEncoding;
			*fNewWarnUnencodable = fWarnUnencodable;
			*fNewSpellCheckStartOn = fSpellCheckStartOn;
			*fNewAutoMarkRead = fAutoMarkRead;
			*fNewButtonBar = fButtonBar;

			be_app->PostMessage(PREFS_CHANGED);

			if (revert) {
				for (int i = fAccountMenu->CountItems(); --i > 0;) {
					BMenuItem *item = fAccountMenu->ItemAt(i);
					BMessage* itemMessage = item->Message();
					if (itemMessage != NULL
						&& itemMessage->FindInt32("id") == *(int32 *)&fAccount) {
						item->SetMarked(true);
						break;
					}
				}

				strcpy(label,fReplyTo == ACCOUNT_USE_DEFAULT
					? B_TRANSLATE("Use default account")
					: B_TRANSLATE("Account from mail"));
				if ((item = fReplyToMenu->FindItem(label)) != NULL)
					item->SetMarked(true);

				strcpy(label, fWrap ? B_TRANSLATE("On") : B_TRANSLATE("Off"));
				if ((item = fWrapMenu->FindItem(label)) != NULL)
					item->SetMarked(true);

				strcpy(label, fAttachAttributes
					? B_TRANSLATE("Include file attributes in attachments")
					: B_TRANSLATE("No file attributes, just plain data"));
				if ((item = fAttachAttributesMenu->FindItem(label)) != NULL)
					item->SetMarked(true);

				strcpy(label, fColoredQuotes ? B_TRANSLATE("On") : B_TRANSLATE("Off"));
				if ((item = fColoredQuotesMenu->FindItem(label)) != NULL)
					item->SetMarked(true);

				if (strcmp(fReplyPreamble->Text(), *fNewPreamble))
					fReplyPreamble->SetText(*fNewPreamble);

				item = fSignatureMenu->FindItem(fSignature);
				if (item)
					item->SetMarked(true);

				uint32 index = 0;
				while ((item = fEncodingMenu->ItemAt(index++)) != NULL) {
					BMessage* itemMessage = item->Message();
					if (itemMessage == NULL)
						continue;

					int32 encoding;
					if (itemMessage->FindInt32("encoding", &encoding) == B_OK
						&& (uint32)encoding == *fNewEncoding) {
						item->SetMarked(true);
						break;
					}
				}

				strcpy(label, fWarnUnencodable ? B_TRANSLATE("On") : B_TRANSLATE("Off"));
				if ((item = fWarnUnencodableMenu->FindItem(label)) != NULL)
					item->SetMarked(true);

				strcpy(label, fSpellCheckStartOn ? B_TRANSLATE("On") : B_TRANSLATE("Off"));
				if ((item = fSpellCheckStartOnMenu->FindItem(label)) != NULL)
					item->SetMarked(true);
			} else
				Quit();
			break;

		case P_FONT:
			family = NULL;
			style = NULL;
			int32 family_menu_index;
			if (msg->FindString("font", &family) == B_OK) {
				msg->FindString("style", &style);
				fNewFont->SetFamilyAndStyle(family, style);
				message.what = M_FONT;
				be_app->PostMessage(&message);
			}

			/* grab this little tidbit so we can set the correct Family */
			if (msg->FindInt32("parent_index", &family_menu_index) == B_OK)
				fFontMenu->ItemAt(family_menu_index)->SetMarked(true);
			break;

		case P_SIZE:
			old_size = (int32) fNewFont->Size();
			msg->FindInt32("size", &new_size);
			if (old_size != new_size) {
				fNewFont->SetSize(new_size);
				message.what = M_FONT;
				be_app->PostMessage(&message);
			}
			break;

		case P_WRAP:
			msg->FindBool("wrap", fNewWrap);
			break;
		case P_ATTACH_ATTRIBUTES:
			msg->FindBool("attachAttributes", fNewAttachAttributes);
			break;
		case P_COLORED_QUOTES:
			msg->FindBool("cquotes", fNewColoredQuotes);
			break;
		case P_ACCOUNT:
			msg->FindInt32("id",(int32*)fNewAccount);
			break;
		case P_REPLYTO:
			msg->FindInt32("replyTo", fNewReplyTo);
			break;
		case P_REPLY_PREAMBLE:
		{
			int32 index = -1;
			if (msg->FindInt32("index", &index) < B_OK)
				break;
			BMenuItem *item = fReplyPreambleMenu->ItemAt(index);
			if (item == NULL) {
				msg->PrintToStream();
				break;
			}

			BTextView *text = fReplyPreamble->TextView();
			int32 selectionStart;
			int32 selectionEnd;
			text->GetSelection(&selectionStart, &selectionEnd);
			if (selectionStart != selectionEnd)
				text->Delete(selectionStart, selectionEnd);
			text->Insert(item->Label(), 2);
		}
		case P_SIG:
			free(*fNewSignature);
			if (msg->FindString("signature", &signature) == B_NO_ERROR) {
				*fNewSignature = (char*)malloc(strlen(signature) + 1);
				strcpy(*fNewSignature, signature);
			} else {
				*fNewSignature = (char*)malloc(
					strlen(B_TRANSLATE("None")) + 1);
				strcpy(*fNewSignature, B_TRANSLATE("None"));
			}
			break;
		case P_ENC:
			msg->FindInt32("encoding", (int32*)fNewEncoding);
			break;
		case P_WARN_UNENCODABLE:
			msg->FindBool("warnUnencodable", fNewWarnUnencodable);
			break;
		case P_SPELL_CHECK_START_ON:
			msg->FindBool("spellCheckStartOn", fNewSpellCheckStartOn);
			break;
		case P_MARK_READ:
			msg->FindBool("autoMarkRead", fNewAutoMarkRead);
			be_app->PostMessage(PREFS_CHANGED);
			break;
		case P_BUTTON_BAR:
			msg->FindInt8("bar", (int8*)fNewButtonBar);
			be_app->PostMessage(PREFS_CHANGED);
			break;

		default:
			BWindow::MessageReceived(msg);
	}

	fFont.GetFamilyAndStyle(&old_family, &old_style);
	fNewFont->GetFamilyAndStyle(&new_family, &new_style);
	old_size = (int32) fFont.Size();
	new_size = (int32) fNewFont->Size();
	bool changed = old_size != new_size
		|| fWrap != *fNewWrap
		|| fAttachAttributes != *fNewAttachAttributes
		|| fColoredQuotes != *fNewColoredQuotes
		|| fAccount != *fNewAccount
		|| fReplyTo != *fNewReplyTo
		|| strcmp(old_family, new_family)
		|| strcmp(old_style, new_style)
		|| strcmp(fReplyPreamble->Text(), *fNewPreamble)
		|| strcmp(fSignature, *fNewSignature)
		|| fEncoding != *fNewEncoding
		|| fWarnUnencodable != *fNewWarnUnencodable
		|| fSpellCheckStartOn != *fNewSpellCheckStartOn
		|| fAutoMarkRead != *fNewAutoMarkRead
		|| fButtonBar != *fNewButtonBar;
	fRevert->SetEnabled(changed);
}


BPopUpMenu*
TPrefsWindow::_BuildFontMenu(BFont* font)
{
	font_family	def_family;
	font_style	def_style;
	font_family	f_family;
	font_style	f_style;

	BPopUpMenu *menu = new BPopUpMenu("");
	font->GetFamilyAndStyle(&def_family, &def_style);

	int32 family_menu_index = 0;
	int family_count = count_font_families();
	for (int family_loop = 0; family_loop < family_count; family_loop++) {
		get_font_family(family_loop, &f_family);
		BMenu *family_menu = new BMenu(f_family);

		int style_count = count_font_styles(f_family);
		for (int style_loop = 0; style_loop < style_count; style_loop++) {
			get_font_style(f_family, style_loop, &f_style);

			BMessage *msg = new BMessage(P_FONT);
			msg->AddString("font", f_family);
			msg->AddString("style", f_style);
			// we send this to make setting the Family easier when things
			// change
			msg->AddInt32("parent_index", family_menu_index);

			BMenuItem *item = new BMenuItem(f_style, msg);
			family_menu->AddItem(item);
			if ((strcmp(def_family, f_family) == 0)
				&& (strcmp(def_style, f_style) == 0)) {
				item->SetMarked(true);
			}

			item->SetTarget(this);
		}

		menu->AddItem(family_menu);
		BMenuItem *item = menu->ItemAt(family_menu_index);
		BMessage *msg = new BMessage(P_FONT);
		msg->AddString("font", f_family);

		item->SetMessage(msg);
		item->SetTarget(this);
		if (strcmp(def_family, f_family) == 0)
			item->SetMarked(true);

		family_menu_index++;
	}
	return menu;
}


BPopUpMenu*
TPrefsWindow::_BuildLevelMenu(int32 level)
{
	BMenuItem* item;
	BMessage* msg;
	BPopUpMenu* menu;

	menu = new BPopUpMenu("");
	msg = new BMessage(P_LEVEL);
	msg->AddInt32("level", L_BEGINNER);
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Beginner"), msg));
	if (level == L_BEGINNER)
		item->SetMarked(true);

	msg = new BMessage(P_LEVEL);
	msg->AddInt32("level", L_EXPERT);
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Expert"), msg));
	if (level == L_EXPERT)
		item->SetMarked(true);

	return menu;
}


BPopUpMenu*
TPrefsWindow::_BuildAccountMenu(int32 account)
{
	BPopUpMenu* menu = new BPopUpMenu("");
	BMenuItem* item;

	//menu->SetRadioMode(true);
	BMailAccounts accounts;
	if (accounts.CountAccounts() == 0) {
		menu->AddItem(item = new BMenuItem("<no account found>", NULL));
		item->SetEnabled(false);
		return menu;
	}

	BMessage* msg;
	for (int32 i = 0; i < accounts.CountAccounts(); i++) {
		BMailAccountSettings* settings = accounts.AccountAt(i);
		item = new BMenuItem(settings->Name(), msg = new BMessage(P_ACCOUNT));

		msg->AddInt32("id", settings->AccountID());

		if (account == settings->AccountID())
			item->SetMarked(true);

		menu->AddItem(item);
	}
	return menu;
}


BPopUpMenu*
TPrefsWindow::_BuildReplyToMenu(int32 account)
{
	BPopUpMenu* menu = new BPopUpMenu(B_EMPTY_STRING);

	BMenuItem* item;
	BMessage* msg;
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Use default account"),
		msg = new BMessage(P_REPLYTO)));
	msg->AddInt32("replyTo", ACCOUNT_USE_DEFAULT);
	if (account == ACCOUNT_USE_DEFAULT)
		item->SetMarked(true);

	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Account from mail"),
		msg = new BMessage(P_REPLYTO)));
	msg->AddInt32("replyTo", ACCOUNT_FROM_MAIL);
	if (account == ACCOUNT_FROM_MAIL)
		item->SetMarked(true);

	return menu;
}


BMenu*
TPrefsWindow::_BuildReplyPreambleMenu()
{
	BMenu *menu = new BMenu(B_EMPTY_STRING);

	menu->AddItem(new BMenuItem(B_TRANSLATE("%n - Full name"),
		new BMessage(P_REPLY_PREAMBLE)));

	menu->AddItem(new BMenuItem(B_TRANSLATE("%e - E-mail address"),
		new BMessage(P_REPLY_PREAMBLE)));

	menu->AddItem(new BMenuItem(B_TRANSLATE("%d - Date"),
		new BMessage(P_REPLY_PREAMBLE)));

	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem(B_TRANSLATE("\\n - Newline"),
		new BMessage(P_REPLY_PREAMBLE)));

	return menu;
}


BPopUpMenu*
TPrefsWindow::_BuildSignatureMenu(char* sig)
{
	char name[B_FILE_NAME_LENGTH];
	BEntry entry;
	BFile file;
	BMenuItem* item;
	BMessage* msg;
	BQuery query;
	BVolume vol;
	BVolumeRoster volume;

	BPopUpMenu* menu = new BPopUpMenu("");

	msg = new BMessage(P_SIG);
	msg->AddString("signature", B_TRANSLATE("None"));
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("None"), msg));
	if (!strcmp(sig, B_TRANSLATE("None")))
		item->SetMarked(true);

	msg = new BMessage(P_SIG);
	msg->AddString("signature", B_TRANSLATE("Random"));
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Random"), msg));
	if (!strcmp(sig, B_TRANSLATE("Random")))
		item->SetMarked(true);
	menu->AddSeparatorItem();

	volume.GetBootVolume(&vol);
	query.SetVolume(&vol);
	query.SetPredicate("_signature = *");
	query.Fetch();

	while (query.GetNextEntry(&entry) == B_NO_ERROR) {
		file.SetTo(&entry, O_RDONLY);
		if (file.InitCheck() == B_NO_ERROR) {
			msg = new BMessage(P_SIG);
			file.ReadAttr("_signature", B_STRING_TYPE, 0, name, sizeof(name));
			msg->AddString("signature", name);
			menu->AddItem(item = new BMenuItem(name, msg));
			if (!strcmp(sig, name))
				item->SetMarked(true);
		}
	}
	return menu;
}


BPopUpMenu*
TPrefsWindow::_BuildSizeMenu(BFont* font)
{
	char label[16];
	uint32 loop;
	int32 sizes[] = {9, 10, 11, 12, 14, 18, 24};
	float size;
	BMenuItem* item;
	BMessage* msg;
	BPopUpMenu* menu;

	menu = new BPopUpMenu("");
	size = font->Size();
	for (loop = 0; loop < sizeof(sizes) / sizeof(int32); loop++) {
		msg = new BMessage(P_SIZE);
		msg->AddInt32("size", sizes[loop]);
		sprintf(label, "%ld", sizes[loop]);
		menu->AddItem(item = new BMenuItem(label, msg));
		if (sizes[loop] == (int32)size)
			item->SetMarked(true);
	}
	return menu;
}


BPopUpMenu*
TPrefsWindow::_BuildBoolMenu(uint32 what, const char* boolItem, bool isTrue)
{
	BMenuItem* item;
	BMessage* msg;
	BPopUpMenu* menu;

	menu = new BPopUpMenu("");
	msg = new BMessage(what);
	msg->AddBool(boolItem, true);
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("On"), msg));
	if (isTrue)
		item->SetMarked(true);

	msg = new BMessage(what);
	msg->AddInt32(boolItem, false);
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Off"), msg));
	if (!isTrue)
		item->SetMarked(true);

	return menu;
}


BPopUpMenu*
TPrefsWindow::_BuildWrapMenu(bool wrap)
{
	return _BuildBoolMenu(P_WRAP, "wrap", wrap);
}


BPopUpMenu*
TPrefsWindow::_BuildAttachAttributesMenu(bool attachAttributes)
{
	BMenuItem* item;
	BMessage* msg;
	BPopUpMenu* menu;

	menu = new BPopUpMenu("");
	msg = new BMessage(P_ATTACH_ATTRIBUTES);
	msg->AddBool("attachAttributes", true);
	menu->AddItem(item = new BMenuItem(
		B_TRANSLATE("Include file attributes in attachments"), msg));
	if (attachAttributes)
		item->SetMarked(true);

	msg = new BMessage(P_ATTACH_ATTRIBUTES);
	msg->AddInt32("attachAttributes", false);
	menu->AddItem(item = new BMenuItem(
		B_TRANSLATE("No file attributes, just plain data"), msg));
	if (!attachAttributes)
		item->SetMarked(true);

	return menu;
}


BPopUpMenu*
TPrefsWindow::_BuildColoredQuotesMenu(bool quote)
{
	return _BuildBoolMenu(P_COLORED_QUOTES, "cquotes", quote);
}


BPopUpMenu*
TPrefsWindow::_BuildEncodingMenu(uint32 encoding)
{
	BMenuItem* item;
	BMessage* msg;
	BPopUpMenu* menu;

	menu = new BPopUpMenu("");

	BCharacterSetRoster roster;
	BCharacterSet charset;
	while (roster.GetNextCharacterSet(&charset) == B_NO_ERROR) {
		BString name(charset.GetPrintName());
		const char* mime = charset.GetMIMEName();
		if (mime)
			name << " (" << mime << ")";
		msg = new BMessage(P_ENC);
		uint32 convert_id;
		if ((mime == 0) || (strcasecmp(mime, "UTF-8") != 0))
			convert_id = charset.GetConversionID();
		else
			convert_id = B_MAIL_UTF8_CONVERSION;
		msg->AddInt32("encoding", convert_id);
		menu->AddItem(item = new BMenuItem(name.String(), msg));
		if (convert_id == encoding)
			item->SetMarked(true);
	}

	msg = new BMessage(P_ENC);
	msg->AddInt32("encoding", B_MAIL_US_ASCII_CONVERSION);
	menu->AddItem(item = new BMenuItem("US-ASCII", msg));
	if (encoding == B_MAIL_US_ASCII_CONVERSION)
		item->SetMarked(true);

	return menu;
}


BPopUpMenu*
TPrefsWindow::_BuildWarnUnencodableMenu(bool warnUnencodable)
{
	return _BuildBoolMenu(P_WARN_UNENCODABLE, "warnUnencodable",
		warnUnencodable);
}


BPopUpMenu*
TPrefsWindow::_BuildSpellCheckStartOnMenu(bool spellCheckStartOn)
{
	return _BuildBoolMenu(P_SPELL_CHECK_START_ON, "spellCheckStartOn",
		spellCheckStartOn);
}


BPopUpMenu*
TPrefsWindow::_BuildAutoMarkReadMenu(bool autoMarkRead)
{
	return _BuildBoolMenu(P_MARK_READ, "autoMarkRead",
		autoMarkRead);
}


BPopUpMenu*
TPrefsWindow::_BuildButtonBarMenu(uint8 show)
{
	BMenuItem* item;
	BMessage* msg;
	BPopUpMenu* menu = new BPopUpMenu("");

	msg = new BMessage(P_BUTTON_BAR);
	msg->AddInt8("bar", 1);
	menu->AddItem(item = new BMenuItem(
		B_TRANSLATE("Show icons & labels"), msg));
	if (show & 1)
		item->SetMarked(true);

	msg = new BMessage(P_BUTTON_BAR);
	msg->AddInt8("bar", 2);
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Show icons only"), msg));
	if (show & 2)
		item->SetMarked(true);

	msg = new BMessage(P_BUTTON_BAR);
	msg->AddInt8("bar", 0);
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Hide"), msg));
	if (!show)
		item->SetMarked(true);

	return menu;
}

