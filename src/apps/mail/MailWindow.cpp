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


#include "MailWindow.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <syslog.h>
#include <unistd.h>

#include <AppFileInfo.h>
#include <Autolock.h>
#include <Bitmap.h>
#include <Button.h>
#include <CharacterSet.h>
#include <CharacterSetRoster.h>
#include <Clipboard.h>
#include <Debug.h>
#include <E-mail.h>
#include <File.h>
#include <IconUtils.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Node.h>
#include <PathMonitor.h>
#include <PrintJob.h>
#include <Query.h>
#include <Resources.h>
#include <Roster.h>
#include <Screen.h>
#include <String.h>
#include <StringView.h>
#include <TextView.h>
#include <UTF8.h>
#include <VolumeRoster.h>

#include <fs_index.h>
#include <fs_info.h>

#include <MailMessage.h>
#include <MailSettings.h>
#include <MailDaemon.h>
#include <mail_util.h>

#include <CharacterSetRoster.h>

#include "AttributeUtilities.h"
#include "Content.h"
#include "Enclosures.h"
#include "FieldMsg.h"
#include "FindWindow.h"
#include "Header.h"
#include "Messages.h"
#include "MailApp.h"
#include "MailPopUpMenu.h"
#include "MailSupport.h"
#include "Prefs.h"
#include "QueryMenu.h"
#include "Signature.h"
#include "Settings.h"
#include "Status.h"
#include "String.h"
#include "Utilities.h"


#define B_TRANSLATION_CONTEXT "Mail"


using namespace BPrivate;


const char* kUndoStrings[] = {
	"Undo",
	"Undo typing",
	"Undo cut",
	"Undo paste",
	"Undo clear",
	"Undo drop"
};

const char* kRedoStrings[] = {
	"Redo",
	"Redo typing",
	"Redo cut",
	"Redo paste",
	"Redo clear",
	"Redo drop"
};


// Text for both the main menu and the pop-up menu.
static const char* kSpamMenuItemTextArray[] = {
	"Mark as spam and move to trash",		// M_TRAIN_SPAM_AND_DELETE
	"Mark as spam",							// M_TRAIN_SPAM
	"Unmark this message",					// M_UNTRAIN
	"Mark as genuine"						// M_TRAIN_GENUINE
};

static const uint32 kMsgQuitAndKeepAllStatus = 'Casm';

static const char* kQueriesDirectory = "mail/queries";
static const char* kAttrQueryInitialMode = "_trk/qryinitmode";
	// taken from src/kits/tracker/Attributes.h
static const char* kAttrQueryInitialString = "_trk/qryinitstr";
static const char* kAttrQueryInitialNumAttrs = "_trk/qryinitnumattrs";
static const char* kAttrQueryInitialAttrs = "_trk/qryinitattrs";
static const uint32 kAttributeItemMain = 'Fatr';
	// taken from src/kits/tracker/FindPanel.h
static const uint32 kByNameItem = 'Fbyn';
	// taken from src/kits/tracker/FindPanel.h
static const uint32 kByAttributeItem = 'Fbya';
	// taken from src/kits/tracker/FindPanel.h
static const uint32 kByForumlaItem = 'Fbyq';
	// taken from src/kits/tracker/FindPanel.h
static const int kCopyBufferSize = 64 * 1024;	// 64 KB

static const char* kSameRecipientItem = B_TRANSLATE_MARK("Same recipient");
static const char* kSameSenderItem = B_TRANSLATE_MARK("Same sender");
static const char* kSameSubjectItem = B_TRANSLATE_MARK("Same subject");


// static bitmap cache
BObjectList<TMailWindow::BitmapItem> TMailWindow::sBitmapCache;
BLocker TMailWindow::sBitmapCacheLock;

// static list for tracking of Windows
BList TMailWindow::sWindowList;
BLocker TMailWindow::sWindowListLock;


class HorizontalLine : public BView {
public:
	HorizontalLine(BRect rect)
		:
		BView (rect, NULL, B_FOLLOW_ALL, B_WILL_DRAW)
	{
	}

	virtual void Draw(BRect rect)
	{
		FillRect(rect, B_SOLID_HIGH);
	}
};


//	#pragma mark -


TMailWindow::TMailWindow(BRect rect, const char* title, TMailApp* app,
	const entry_ref* ref, const char* to, const BFont* font, bool resending,
	BMessenger* messenger)
	:
	BWindow(rect, title, B_DOCUMENT_WINDOW, B_AUTO_UPDATE_SIZE_LIMITS),

	fApp(app),
	fMail(NULL),
	fRef(NULL),
	fFieldState(0),
	fPanel(NULL),
	fSaveAddrMenu(NULL),
	fLeaveStatusMenu(NULL),
	fEncodingMenu(NULL),
	fZoom(rect),
	fEnclosuresView(NULL),
	fPrevTrackerPositionSaved(false),
	fNextTrackerPositionSaved(false),
	fSigAdded(false),
	fReplying(false),
	fResending(resending),
	fSent(false),
	fDraft(false),
	fChanged(false),
	fOriginatingWindow(NULL),

	fDownloading(false)
{
	fKeepStatusOnQuit = false;

	if (messenger != NULL)
		fTrackerMessenger = *messenger;

	BFile file(ref, B_READ_ONLY);
	if (ref) {
		fRef = new entry_ref(*ref);
		fIncoming = true;
	} else
		fIncoming = false;

	fAutoMarkRead = fApp->AutoMarkRead();
	fMenuBar = new BMenuBar("menuBar");

	// File Menu

	BMenu* menu = new BMenu(B_TRANSLATE("File"));

	BMessage* msg = new BMessage(M_NEW);
	msg->AddInt32("type", M_NEW);
	BMenuItem* item = new BMenuItem(B_TRANSLATE("New mail message"), msg, 'N');
	menu->AddItem(item);
	item->SetTarget(be_app);

	// Cheap hack - only show the drafts menu when composing messages.  Insert
	// a "true || " in the following IF statement if you want the old BeMail
	// behaviour.  The difference is that without live draft menu updating you
	// can open around 100 e-mails (the BeOS maximum number of open files)
	// rather than merely around 20, since each open draft-monitoring query
	// sucks up one file handle per mounted BFS disk volume.  Plus mail file
	// opening speed is noticably improved!  ToDo: change this to populate the
	// Draft menu with the file names on demand - when the user clicks on it;
	// don't need a live query since the menu isn't staying up for more than a
	// few seconds.

	if (!fIncoming) {
		QueryMenu* queryMenu = new QueryMenu(B_TRANSLATE("Open draft"), false);
		queryMenu->SetTargetForItems(be_app);

		queryMenu->SetPredicate("MAIL:draft==1");
		menu->AddItem(queryMenu);
	}

	if (!fIncoming || resending) {
		menu->AddItem(fSendLater = new BMenuItem(B_TRANSLATE("Save as draft"),
			new BMessage(M_SAVE_AS_DRAFT), 'S'));
	}

	if (!resending && fIncoming) {
		menu->AddSeparatorItem();

		BMenu* subMenu = new BMenu(B_TRANSLATE("Close and "));

		read_flags flag;
		read_read_attr(file, flag);

		if (flag == B_UNREAD) {
			subMenu->AddItem(item = new BMenuItem(
				B_TRANSLATE_COMMENT("Leave as 'New'",
				"Do not translate New - this is non-localizable e-mail status"),
				new BMessage(kMsgQuitAndKeepAllStatus), 'W', B_SHIFT_KEY));
		} else {
			BString status;
			file.ReadAttrString(B_MAIL_ATTR_STATUS, &status);

			BString label;
			if (status.Length() > 0)
				label.SetToFormat(B_TRANSLATE("Leave as '%s'"),
					status.String());
			else
				label = B_TRANSLATE("Leave same");

			subMenu->AddItem(item = new BMenuItem(label.String(),
							new BMessage(B_QUIT_REQUESTED), 'W'));
			AddShortcut('W', B_COMMAND_KEY | B_SHIFT_KEY,
				new BMessage(kMsgQuitAndKeepAllStatus));
		}

		subMenu->AddItem(new BMenuItem(B_TRANSLATE("Move to trash"),
			new BMessage(M_DELETE), 'T', B_CONTROL_KEY));
		AddShortcut('T', B_SHIFT_KEY | B_COMMAND_KEY,
			new BMessage(M_DELETE_NEXT));

		subMenu->AddSeparatorItem();

		subMenu->AddItem(new BMenuItem(B_TRANSLATE("Set to Saved"),
			new BMessage(M_CLOSE_SAVED), 'W', B_CONTROL_KEY));

		if (add_query_menu_items(subMenu, INDEX_STATUS, M_STATUS,
			B_TRANSLATE("Set to %s")) > 0)
			subMenu->AddSeparatorItem();

		subMenu->AddItem(new BMenuItem(B_TRANSLATE("Set to" B_UTF8_ELLIPSIS),
			new BMessage(M_CLOSE_CUSTOM)));

#if 0
		subMenu->AddItem(new BMenuItem(new TMenu(
			B_TRANSLATE("Set to" B_UTF8_ELLIPSIS), INDEX_STATUS, M_STATUS,
				false, false),
			new BMessage(M_CLOSE_CUSTOM)));
#endif
		menu->AddItem(subMenu);

		fLeaveStatusMenu = subMenu;
	} else {
		menu->AddSeparatorItem();
		menu->AddItem(new BMenuItem(B_TRANSLATE("Close"),
			new BMessage(B_CLOSE_REQUESTED), 'W'));
	}

	menu->AddSeparatorItem();
	menu->AddItem(fPrint = new BMenuItem(
		B_TRANSLATE("Page setup" B_UTF8_ELLIPSIS),
		new BMessage(M_PRINT_SETUP)));
	menu->AddItem(fPrint = new BMenuItem(
		B_TRANSLATE("Print" B_UTF8_ELLIPSIS),
		new BMessage(M_PRINT), 'P'));
	fMenuBar->AddItem(menu);

	menu->AddSeparatorItem();
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Quit"),
		new BMessage(B_QUIT_REQUESTED), 'Q'));
	item->SetTarget(be_app);

	// Edit Menu

	menu = new BMenu(B_TRANSLATE("Edit"));
	menu->AddItem(fUndo = new BMenuItem(B_TRANSLATE("Undo"),
		new BMessage(B_UNDO), 'Z', 0));
	fUndo->SetTarget(NULL, this);
	menu->AddItem(fRedo = new BMenuItem(B_TRANSLATE("Redo"),
		new BMessage(M_REDO), 'Z', B_SHIFT_KEY));
	fRedo->SetTarget(NULL, this);
	menu->AddSeparatorItem();
	menu->AddItem(fCut = new BMenuItem(B_TRANSLATE("Cut"),
		new BMessage(B_CUT), 'X'));
	fCut->SetTarget(NULL, this);
	menu->AddItem(fCopy = new BMenuItem(B_TRANSLATE("Copy"),
		new BMessage(B_COPY), 'C'));
	fCopy->SetTarget(NULL, this);
	menu->AddItem(fPaste = new BMenuItem(B_TRANSLATE("Paste"),
		new BMessage(B_PASTE),
		'V'));
	fPaste->SetTarget(NULL, this);
	menu->AddSeparatorItem();
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Select all"),
		new BMessage(M_SELECT), 'A'));
	menu->AddSeparatorItem();
	item->SetTarget(NULL, this);
	menu->AddItem(new BMenuItem(B_TRANSLATE("Find" B_UTF8_ELLIPSIS),
		new BMessage(M_FIND), 'F'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Find again"),
		new BMessage(M_FIND_AGAIN), 'G'));
	if (!fIncoming) {
		menu->AddSeparatorItem();
		fQuote = new BMenuItem(B_TRANSLATE("Quote"),
			new BMessage(M_QUOTE), '\'');
		menu->AddItem(fQuote);
		fRemoveQuote = new BMenuItem(B_TRANSLATE("Remove quote"),
			new BMessage(M_REMOVE_QUOTE), '\'', B_SHIFT_KEY);
		menu->AddItem(fRemoveQuote);

		menu->AddSeparatorItem();
		fSpelling = new BMenuItem(B_TRANSLATE("Check spelling"),
			new BMessage(M_CHECK_SPELLING), ';');
		menu->AddItem(fSpelling);
		if (fApp->StartWithSpellCheckOn())
			PostMessage(M_CHECK_SPELLING);
	}
	menu->AddSeparatorItem();
	menu->AddItem(item = new BMenuItem(
		B_TRANSLATE("Settings" B_UTF8_ELLIPSIS),
		new BMessage(M_PREFS),','));
	item->SetTarget(be_app);
	fMenuBar->AddItem(menu);
	menu->AddItem(item = new BMenuItem(
		B_TRANSLATE("Accounts" B_UTF8_ELLIPSIS),
		new BMessage(M_ACCOUNTS),'-'));
	item->SetTarget(be_app);

	// View Menu

	if (!resending && fIncoming) {
		menu = new BMenu(B_TRANSLATE("View"));
		menu->AddItem(fHeader = new BMenuItem(B_TRANSLATE("Show header"),
			new BMessage(M_HEADER), 'H'));
		menu->AddItem(fRaw = new BMenuItem(B_TRANSLATE("Show raw message"),
			new BMessage(M_RAW)));
		fMenuBar->AddItem(menu);
	}

	// Message Menu

	menu = new BMenu(B_TRANSLATE("Message"));

	if (!resending && fIncoming) {
		BMenuItem* menuItem;
		menu->AddItem(new BMenuItem(B_TRANSLATE("Reply"),
			new BMessage(M_REPLY),'R'));
		menu->AddItem(new BMenuItem(B_TRANSLATE("Reply to sender"),
			new BMessage(M_REPLY_TO_SENDER),'R',B_OPTION_KEY));
		menu->AddItem(new BMenuItem(B_TRANSLATE("Reply to all"),
			new BMessage(M_REPLY_ALL), 'R', B_SHIFT_KEY));

		menu->AddSeparatorItem();

		menu->AddItem(new BMenuItem(B_TRANSLATE("Forward"),
			new BMessage(M_FORWARD), 'J'));
		menu->AddItem(new BMenuItem(B_TRANSLATE("Forward without attachments"),
			new BMessage(M_FORWARD_WITHOUT_ATTACHMENTS)));
		menu->AddItem(menuItem = new BMenuItem(B_TRANSLATE("Resend"),
			new BMessage(M_RESEND)));
		menu->AddItem(menuItem = new BMenuItem(B_TRANSLATE("Copy to new"),
			new BMessage(M_COPY_TO_NEW), 'D'));

		menu->AddSeparatorItem();
		fDeleteNext = new BMenuItem(B_TRANSLATE("Move to trash"),
			new BMessage(M_DELETE_NEXT), 'T');
		menu->AddItem(fDeleteNext);
		menu->AddSeparatorItem();

		fPrevMsg = new BMenuItem(B_TRANSLATE("Previous message"),
			new BMessage(M_PREVMSG), B_UP_ARROW);
		menu->AddItem(fPrevMsg);
		fNextMsg = new BMenuItem(B_TRANSLATE("Next message"),
			new BMessage(M_NEXTMSG), B_DOWN_ARROW);
		menu->AddItem(fNextMsg);
	} else {
		menu->AddItem(fSendNow = new BMenuItem(B_TRANSLATE("Send message"),
			new BMessage(M_SEND_NOW), 'M'));

		if (!fIncoming) {
			menu->AddSeparatorItem();
			fSignature = new TMenu(B_TRANSLATE("Add signature"),
				INDEX_SIGNATURE, M_SIGNATURE);
			menu->AddItem(new BMenuItem(fSignature));
			menu->AddItem(item = new BMenuItem(
				B_TRANSLATE("Edit signatures" B_UTF8_ELLIPSIS),
				new BMessage(M_EDIT_SIGNATURE)));
			item->SetTarget(be_app);
			menu->AddSeparatorItem();
			menu->AddItem(fAdd = new BMenuItem(
				B_TRANSLATE("Add enclosure" B_UTF8_ELLIPSIS),
				new BMessage(M_ADD), 'E'));
			menu->AddItem(fRemove = new BMenuItem(
				B_TRANSLATE("Remove enclosure"),
				new BMessage(M_REMOVE), 'T'));
		}
	}
	if (fIncoming) {
		menu->AddSeparatorItem();
		fSaveAddrMenu = new BMenu(B_TRANSLATE("Save address"));
		menu->AddItem(fSaveAddrMenu);
	}

	// Encoding menu

	fEncodingMenu = new BMenu(B_TRANSLATE("Encoding"));

	BMenuItem* automaticItem = NULL;
	if (!resending && fIncoming) {
		// Reading a message, display the Automatic item
		msg = new BMessage(CHARSET_CHOICE_MADE);
		msg->AddInt32("charset", B_MAIL_NULL_CONVERSION);
		automaticItem = new BMenuItem(B_TRANSLATE("Automatic"), msg);
		fEncodingMenu->AddItem(automaticItem);
		fEncodingMenu->AddSeparatorItem();
	}

	uint32 defaultCharSet = resending || !fIncoming
		? fApp->MailCharacterSet() : B_MAIL_NULL_CONVERSION;
	bool markedCharSet = false;

	BCharacterSetRoster roster;
	BCharacterSet charSet;
	while (roster.GetNextCharacterSet(&charSet) == B_OK) {
		BString name(charSet.GetPrintName());
		const char* mime = charSet.GetMIMEName();
		if (mime != NULL)
			name << " (" << mime << ")";

		uint32 convertID;
		if (mime == NULL || strcasecmp(mime, "UTF-8") != 0)
			convertID = charSet.GetConversionID();
		else
			convertID = B_MAIL_UTF8_CONVERSION;

		msg = new BMessage(CHARSET_CHOICE_MADE);
		msg->AddInt32("charset", convertID);
		fEncodingMenu->AddItem(item = new BMenuItem(name.String(), msg));
		if (convertID == defaultCharSet && !markedCharSet) {
			item->SetMarked(true);
			markedCharSet = true;
		}
	}

	msg = new BMessage(CHARSET_CHOICE_MADE);
	msg->AddInt32("charset", B_MAIL_US_ASCII_CONVERSION);
	fEncodingMenu->AddItem(item = new BMenuItem("US-ASCII", msg));
	if (defaultCharSet == B_MAIL_US_ASCII_CONVERSION && !markedCharSet) {
		item->SetMarked(true);
		markedCharSet = true;
	}

	if (automaticItem != NULL && !markedCharSet)
		automaticItem->SetMarked(true);

	menu->AddSeparatorItem();
	menu->AddItem(fEncodingMenu);
	fMenuBar->AddItem(menu);
	fEncodingMenu->SetRadioMode(true);
	fEncodingMenu->SetTargetForItems(this);

	// Spam Menu

	if (!resending && fIncoming && fApp->ShowSpamGUI()) {
		menu = new BMenu("Spam filtering");
		menu->AddItem(new BMenuItem("Mark as spam and move to trash",
			new BMessage(M_TRAIN_SPAM_AND_DELETE), 'K'));
		menu->AddItem(new BMenuItem("Mark as spam",
			new BMessage(M_TRAIN_SPAM), 'K', B_OPTION_KEY));
		menu->AddSeparatorItem();
		menu->AddItem(new BMenuItem("Unmark this message",
			new BMessage(M_UNTRAIN)));
		menu->AddSeparatorItem();
		menu->AddItem(new BMenuItem("Mark as genuine",
			new BMessage(M_TRAIN_GENUINE), 'K', B_SHIFT_KEY));
		fMenuBar->AddItem(menu);
	}

	// Queries Menu

	fQueryMenu = new BMenu(B_TRANSLATE("Queries"));
	fMenuBar->AddItem(fQueryMenu);

	_RebuildQueryMenu(true);

	// Button Bar

	BuildToolBar();

	if (!fApp->ShowToolBar())
		fToolBar->Hide();

	fHeaderView = new THeaderView(fIncoming, resending,
		fApp->DefaultAccount());

	fContentView = new TContentView(fIncoming, const_cast<BFont*>(font),
		false, fApp->ColoredQuotes());
		// TContentView needs to be properly const, for now cast away constness

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(fMenuBar)
		.Add(fToolBar)
		.AddGroup(B_VERTICAL, 0)
			.Add(fHeaderView)
			.SetInsets(B_USE_WINDOW_SPACING, B_USE_DEFAULT_SPACING)
		.End()
		.Add(fContentView);

	if (to != NULL)
		fHeaderView->SetTo(to);

	AddShortcut('n', B_COMMAND_KEY, new BMessage(M_NEW));

	// If auto-signature, add signature to the text here.

	BString signature = fApp->Signature();

	if (!fIncoming && strcmp(signature.String(), B_TRANSLATE("None")) != 0) {
		if (strcmp(signature.String(), B_TRANSLATE("Random")) == 0)
			PostMessage(M_RANDOM_SIG);
		else {
			// Create a query to find this signature
			BVolume volume;
			BVolumeRoster().GetBootVolume(&volume);

			BQuery query;
			query.SetVolume(&volume);
			query.PushAttr(INDEX_SIGNATURE);
			query.PushString(signature.String());
			query.PushOp(B_EQ);
			query.Fetch();

			// If we find the named query, add it to the text.
			BEntry entry;
			if (query.GetNextEntry(&entry) == B_NO_ERROR) {
				BFile file;
				file.SetTo(&entry, O_RDWR);
				if (file.InitCheck() == B_NO_ERROR) {
					entry_ref ref;
					entry.GetRef(&ref);

					BMessage msg(M_SIGNATURE);
					msg.AddRef("ref", &ref);
					PostMessage(&msg);
				}
			} else {
				char tempString [2048];
				query.GetPredicate (tempString, sizeof (tempString));
				printf ("Query failed, was looking for: %s\n", tempString);
			}
		}
	}

	OpenMessage(ref, _CurrentCharacterSet());

	AddShortcut('q', B_SHIFT_KEY, new BMessage(kMsgQuitAndKeepAllStatus));
}


BBitmap*
TMailWindow::_RetrieveVectorIcon(int32 id)
{
	// Lock access to the list
	BAutolock lock(sBitmapCacheLock);
	if (!lock.IsLocked())
		return NULL;

	// Check for the bitmap in the cache first
	BitmapItem* item;
	for (int32 i = 0; (item = sBitmapCache.ItemAt(i)) != NULL; i++) {
		if (item->id == id)
			return item->bm;
	}

	// If it's not in the cache, try to load it
	BResources* res = BApplication::AppResources();
	if (res == NULL)
		return NULL;
	size_t size;
	const void* data = res->LoadResource(B_VECTOR_ICON_TYPE, id, &size);

	if (!data)
		return NULL;

	BBitmap* bitmap = new BBitmap(BRect(0, 0, 21, 21), B_RGBA32);
	status_t status = BIconUtils::GetVectorIcon((uint8*)data, size, bitmap);
	if (status == B_OK) {
		item = (BitmapItem*)malloc(sizeof(BitmapItem));
		item->bm = bitmap;
		item->id = id;
		sBitmapCache.AddItem(item);
		return bitmap;
	}

	return NULL;
}


void
TMailWindow::BuildToolBar()
{
	fToolBar = new BToolBar();
	fToolBar->AddAction(M_NEW, this, _RetrieveVectorIcon(11), NULL,
		B_TRANSLATE("New"));
	fToolBar->AddSeparator();

	if (fResending) {
		fToolBar->AddAction(M_SEND_NOW, this, _RetrieveVectorIcon(1), NULL,
			B_TRANSLATE("Send"));
	} else if (!fIncoming) {
		fToolBar->AddAction(M_SEND_NOW, this, _RetrieveVectorIcon(1), NULL,
			B_TRANSLATE("Send"));
		fToolBar->SetActionEnabled(M_SEND_NOW, false);
		fToolBar->AddAction(M_SIG_MENU, this, _RetrieveVectorIcon(2), NULL,
			B_TRANSLATE("Signature"));
		fToolBar->AddAction(M_SAVE_AS_DRAFT, this, _RetrieveVectorIcon(3), NULL,
			B_TRANSLATE("Save"));
		fToolBar->SetActionEnabled(M_SAVE_AS_DRAFT, false);
		fToolBar->AddAction(M_PRINT, this, _RetrieveVectorIcon(5), NULL,
			B_TRANSLATE("Print"));
		fToolBar->SetActionEnabled(M_PRINT, false);
		fToolBar->AddAction(M_DELETE, this, _RetrieveVectorIcon(4), NULL,
			B_TRANSLATE("Trash"));
	} else {
		fToolBar->AddAction(M_REPLY, this, _RetrieveVectorIcon(8), NULL,
			B_TRANSLATE("Reply"));
		fToolBar->AddAction(M_FORWARD, this, _RetrieveVectorIcon(9), NULL,
			B_TRANSLATE("Forward"));
		fToolBar->AddAction(M_PRINT, this, _RetrieveVectorIcon(5), NULL,
			B_TRANSLATE("Print"));
		fToolBar->AddAction(M_DELETE_NEXT, this, _RetrieveVectorIcon(4), NULL,
			B_TRANSLATE("Trash"));
		if (fApp->ShowSpamGUI()) {
			fToolBar->AddAction(M_SPAM_BUTTON, this, _RetrieveVectorIcon(10),
				NULL, B_TRANSLATE("Spam"));
		}
		fToolBar->AddSeparator();
		fToolBar->AddAction(M_NEXTMSG, this, _RetrieveVectorIcon(6), NULL,
			B_TRANSLATE("Next"));
		fToolBar->AddAction(M_UNREAD, this, _RetrieveVectorIcon(12), NULL,
			B_TRANSLATE("Unread"));
		fToolBar->SetActionVisible(M_UNREAD, false);
		fToolBar->AddAction(M_READ, this, _RetrieveVectorIcon(13), NULL,
			B_TRANSLATE(" Read "));
		fToolBar->SetActionVisible(M_READ, false);
		fToolBar->AddAction(M_PREVMSG, this, _RetrieveVectorIcon(7), NULL,
			B_TRANSLATE("Previous"));

		if (!fTrackerMessenger.IsValid()) {
			fToolBar->SetActionEnabled(M_NEXTMSG, false);
			fToolBar->SetActionEnabled(M_PREVMSG, false);
		}

		if (!fAutoMarkRead)
			_AddReadButton();
	}
	fToolBar->AddGlue();
}


void
TMailWindow::UpdateViews()
{
	uint8 showToolBar = fApp->ShowToolBar();

	// Show/Hide Button Bar
	if (showToolBar) {
		if (fToolBar->IsHidden())
			fToolBar->Show();

		bool showLabel = showToolBar == kShowToolBar;
		_UpdateLabel(M_NEW, B_TRANSLATE("New"), showLabel);
		_UpdateLabel(M_SEND_NOW, B_TRANSLATE("Send"), showLabel);
		_UpdateLabel(M_SIG_MENU, B_TRANSLATE("Signature"), showLabel);
		_UpdateLabel(M_SAVE_AS_DRAFT, B_TRANSLATE("Save"), showLabel);
		_UpdateLabel(M_PRINT, B_TRANSLATE("Print"), showLabel);
		_UpdateLabel(M_DELETE, B_TRANSLATE("Trash"), showLabel);
		_UpdateLabel(M_REPLY, B_TRANSLATE("Reply"), showLabel);
		_UpdateLabel(M_FORWARD, B_TRANSLATE("Forward"), showLabel);
		_UpdateLabel(M_DELETE_NEXT, B_TRANSLATE("Trash"), showLabel);
		_UpdateLabel(M_SPAM_BUTTON, B_TRANSLATE("Spam"), showLabel);
		_UpdateLabel(M_NEXTMSG, B_TRANSLATE("Next"), showLabel);
		_UpdateLabel(M_UNREAD, B_TRANSLATE("Unread"), showLabel);
		_UpdateLabel(M_READ, B_TRANSLATE(" Read "), showLabel);
		_UpdateLabel(M_PREVMSG, B_TRANSLATE("Previous"), showLabel);
	} else if (!fToolBar->IsHidden())
		fToolBar->Hide();
}


void
TMailWindow::UpdatePreferences()
{
	fAutoMarkRead = fApp->AutoMarkRead();

	_UpdateReadButton();
}


TMailWindow::~TMailWindow()
{
	fApp->SetLastWindowFrame(Frame());

	delete fMail;
	delete fPanel;
	delete fOriginatingWindow;
	delete fRef;

	BAutolock locker(sWindowListLock);
	sWindowList.RemoveItem(this);
}


status_t
TMailWindow::GetMailNodeRef(node_ref& nodeRef) const
{
	if (fRef == NULL)
		return B_ERROR;

	BNode node(fRef);
	return node.GetNodeRef(&nodeRef);
}


bool
TMailWindow::GetTrackerWindowFile(entry_ref* ref, bool next) const
{
	// Position was already saved
	if (next && fNextTrackerPositionSaved) {
		*ref = fNextRef;
		return true;
	}
	if (!next && fPrevTrackerPositionSaved) {
		*ref = fPrevRef;
		return true;
	}

	if (!fTrackerMessenger.IsValid())
		return false;

	// Ask the tracker what the next/prev file in the window is.
	// Continue asking for the next reference until a valid
	// email file is found (ignoring other types).
	entry_ref nextRef = *ref;
	bool foundRef = false;
	while (!foundRef) {
		BMessage request(B_GET_PROPERTY);
		BMessage spc;
		if (next)
			spc.what = 'snxt';
		else
			spc.what = 'sprv';

		spc.AddString("property", "Entry");
		spc.AddRef("data", &nextRef);

		request.AddSpecifier(&spc);
		BMessage reply;
		if (fTrackerMessenger.SendMessage(&request, &reply) != B_OK)
			return false;

		if (reply.FindRef("result", &nextRef) != B_OK)
			return false;

		char fileType[256];
		BNode node(&nextRef);
		if (node.InitCheck() != B_OK)
			return false;

		if (BNodeInfo(&node).GetType(fileType) != B_OK)
			return false;

		if (strcasecmp(fileType, B_MAIL_TYPE) == 0
			|| strcasecmp(fileType, B_PARTIAL_MAIL_TYPE) == 0)
			foundRef = true;
	}

	*ref = nextRef;
	return foundRef;
}


void
TMailWindow::SaveTrackerPosition(entry_ref* ref)
{
	// if only one of them is saved, we're not going to do it again
	if (fNextTrackerPositionSaved || fPrevTrackerPositionSaved)
		return;

	fNextRef = fPrevRef = *ref;

	fNextTrackerPositionSaved = GetTrackerWindowFile(&fNextRef, true);
	fPrevTrackerPositionSaved = GetTrackerWindowFile(&fPrevRef, false);
}


void
TMailWindow::SetOriginatingWindow(BWindow* window)
{
	delete fOriginatingWindow;
	fOriginatingWindow = new BMessenger(window);
}


void
TMailWindow::SetTrackerSelectionToCurrent()
{
	BMessage setSelection(B_SET_PROPERTY);
	setSelection.AddSpecifier("Selection");
	setSelection.AddRef("data", fRef);

	fTrackerMessenger.SendMessage(&setSelection);
}


void
TMailWindow::PreserveReadingPos(bool save)
{
	BScrollBar* scroll = fContentView->TextView()->ScrollBar(B_VERTICAL);
	if (scroll == NULL || fRef == NULL)
		return;

	BNode node(fRef);
	float pos = scroll->Value();

	const char* name = "MAIL:read_pos";
	if (save) {
		node.WriteAttr(name, B_FLOAT_TYPE, 0, &pos, sizeof(pos));
		return;
	}

	if (node.ReadAttr(name, B_FLOAT_TYPE, 0, &pos, sizeof(pos)) == sizeof(pos)) {
		Lock();
		scroll->SetValue(pos);
		Unlock();
	}
}


void
TMailWindow::MarkMessageRead(entry_ref* message, read_flags flag)
{
	BNode node(message);
	status_t status = node.InitCheck();
	if (status != B_OK)
		return;

	int32 account;
	if (node.ReadAttr(B_MAIL_ATTR_ACCOUNT_ID, B_INT32_TYPE, 0, &account,
		sizeof(account)) < 0)
		account = -1;

	// don't wait for the server write the attribute directly
	write_read_attr(node, flag);

	// preserve the read position in the node attribute
	PreserveReadingPos(true);

	BMailDaemon().MarkAsRead(account, *message, flag);
}


void
TMailWindow::FrameResized(float width, float height)
{
	fContentView->FrameResized(width, height);
}


void
TMailWindow::MenusBeginning()
{
	int32 finish = 0;
	int32 start = 0;

	if (!fIncoming) {
		bool gotToField = !fHeaderView->IsToEmpty();
		bool gotCcField = !fHeaderView->IsCcEmpty();
		bool gotBccField = !fHeaderView->IsBccEmpty();
		bool gotSubjectField = !fHeaderView->IsSubjectEmpty();
		bool gotText = fContentView->TextView()->Text()[0] != 0;
		fSendNow->SetEnabled(gotToField || gotBccField);
		fSendLater->SetEnabled(fChanged && (gotToField || gotCcField
			|| gotBccField || gotSubjectField || gotText));

		be_clipboard->Lock();
		fPaste->SetEnabled(be_clipboard->Data()->HasData("text/plain",
				B_MIME_TYPE)
			&& (fEnclosuresView == NULL || !fEnclosuresView->fList->IsFocus()));
		be_clipboard->Unlock();

		fQuote->SetEnabled(false);
		fRemoveQuote->SetEnabled(false);

		fAdd->SetEnabled(true);
		fRemove->SetEnabled(fEnclosuresView != NULL
			&& fEnclosuresView->fList->CurrentSelection() >= 0);
	} else {
		if (fResending) {
			bool enable = !fHeaderView->IsToEmpty();
			fSendNow->SetEnabled(enable);
			//fSendLater->SetEnabled(enable);

			if (fHeaderView->ToControl()->HasFocus()) {
				fHeaderView->ToControl()->GetSelection(&start, &finish);

				fCut->SetEnabled(start != finish);
				be_clipboard->Lock();
				fPaste->SetEnabled(be_clipboard->Data()->HasData(
					"text/plain", B_MIME_TYPE));
				be_clipboard->Unlock();
			} else {
				fCut->SetEnabled(false);
				fPaste->SetEnabled(false);
			}
		} else {
			fCut->SetEnabled(false);
			fPaste->SetEnabled(false);
		}
	}

	fPrint->SetEnabled(fContentView->TextView()->TextLength());

	BTextView* textView = dynamic_cast<BTextView*>(CurrentFocus());
	if (textView != NULL
		&& (dynamic_cast<AddressTextControl*>(textView->Parent()) != NULL
			|| dynamic_cast<BTextControl*>(textView->Parent()) != NULL)) {
		// one of To:, Subject:, Account:, Cc:, Bcc:
		textView->GetSelection(&start, &finish);
	} else if (fContentView->TextView()->IsFocus()) {
		fContentView->TextView()->GetSelection(&start, &finish);
		if (!fIncoming) {
			fQuote->SetEnabled(true);
			fRemoveQuote->SetEnabled(true);
		}
	}

	fCopy->SetEnabled(start != finish);
	if (!fIncoming)
		fCut->SetEnabled(start != finish);

	// Undo stuff
	bool isRedo = false;
	undo_state undoState = B_UNDO_UNAVAILABLE;

	BTextView* focusTextView = dynamic_cast<BTextView*>(CurrentFocus());
	if (focusTextView != NULL)
		undoState = focusTextView->UndoState(&isRedo);

//	fUndo->SetLabel((isRedo)
//	? kRedoStrings[undoState] : kUndoStrings[undoState]);
	fUndo->SetEnabled(undoState != B_UNDO_UNAVAILABLE);

	if (fLeaveStatusMenu != NULL && fRef != NULL) {
		BFile file(fRef, B_READ_ONLY);
		BString status;
		file.ReadAttrString(B_MAIL_ATTR_STATUS, &status);

		BMenuItem* LeaveStatus = fLeaveStatusMenu->FindItem(B_QUIT_REQUESTED);
		if (LeaveStatus == NULL)
			LeaveStatus = fLeaveStatusMenu->FindItem(kMsgQuitAndKeepAllStatus);

		if (LeaveStatus != NULL && status.Length() > 0) {
			BString label;
			label.SetToFormat(B_TRANSLATE("Leave as '%s'"), status.String());
			LeaveStatus->SetLabel(label.String());
		}
	}
}


void
TMailWindow::MessageReceived(BMessage* msg)
{
	bool wasReadMsg = false;
	switch (msg->what) {
		case B_MAIL_BODY_FETCHED:
		{
			status_t status = msg->FindInt32("status");
			if (status != B_OK) {
				fprintf(stderr, "Body could not be fetched: %s\n", strerror(status));
				PostMessage(B_QUIT_REQUESTED);
				break;
			}

			entry_ref ref;
			if (msg->FindRef("ref", &ref) != B_OK)
				break;
			if (ref != *fRef)
				break;

			// reload the current message
			OpenMessage(&ref, _CurrentCharacterSet());
			break;
		}

		case FIELD_CHANGED:
		{
			int32 prevState = fFieldState;
			int32 fieldMask = msg->FindInt32("bitmask");
			void* source;

			if (msg->FindPointer("source", &source) == B_OK) {
				int32 length;

				if (fieldMask == FIELD_BODY)
					length = ((TTextView*)source)->TextLength();
				else
					length = ((AddressTextControl*)source)->TextLength();

				if (length)
					fFieldState |= fieldMask;
				else
					fFieldState &= ~fieldMask;
			}

			// Has anything changed?
			if (prevState != fFieldState || !fChanged) {
				// Change Buttons to reflect this
				fToolBar->SetActionEnabled(M_SAVE_AS_DRAFT, fFieldState);
				fToolBar->SetActionEnabled(M_PRINT, fFieldState);
				fToolBar->SetActionEnabled(M_SEND_NOW, (fFieldState & FIELD_TO)
					|| (fFieldState & FIELD_BCC));
			}
			fChanged = true;

			// Update title bar if "subject" has changed
			if (!fIncoming && (fieldMask & FIELD_SUBJECT) != 0) {
				// If no subject, set to "Mail"
				if (fHeaderView->IsSubjectEmpty())
					SetTitle(B_TRANSLATE_SYSTEM_NAME("Mail"));
				else
					SetTitle(fHeaderView->Subject());
			}
			break;
		}
		case LIST_INVOKED:
			PostMessage(msg, fEnclosuresView);
			break;

		case CHANGE_FONT:
			PostMessage(msg, fContentView);
			break;

		case M_NEW:
		{
			BMessage message(M_NEW);
			message.AddInt32("type", msg->what);
			be_app->PostMessage(&message);
			break;
		}

		case M_SPAM_BUTTON:
		{
			/*
				A popup from a button is good only when the behavior has some
				consistency and there is some visual indication that a menu
				will be shown when clicked. A workable implementation would
				have an extra button attached to the main one which has a
				downward-pointing arrow. Mozilla Thunderbird's 'Get Mail'
				button is a good example of this.

				TODO: Replace this code with a split toolbar button
			*/
			uint32 buttons;
			if (msg->FindInt32("buttons", (int32*)&buttons) == B_OK
				&& buttons == B_SECONDARY_MOUSE_BUTTON) {
				BPopUpMenu menu("Spam Actions", false, false);
				for (int i = 0; i < 4; i++)
					menu.AddItem(new BMenuItem(kSpamMenuItemTextArray[i],
						new BMessage(M_TRAIN_SPAM_AND_DELETE + i)));

				BPoint where;
				msg->FindPoint("where", &where);
				BMenuItem* item;
				if ((item = menu.Go(where, false, false)) != NULL)
					PostMessage(item->Message());
				break;
			} else {
				// Default action for left clicking on the spam button.
				PostMessage(new BMessage(M_TRAIN_SPAM_AND_DELETE));
			}
			break;
		}

		case M_TRAIN_SPAM_AND_DELETE:
			PostMessage(M_DELETE_NEXT);
		case M_TRAIN_SPAM:
			TrainMessageAs("Spam");
			break;

		case M_UNTRAIN:
			TrainMessageAs("Uncertain");
			break;

		case M_TRAIN_GENUINE:
			TrainMessageAs("Genuine");
			break;

		case M_REPLY:
		{
			// TODO: This needs removed in favor of a split toolbar button.
			// See comments for Spam button
			uint32 buttons;
			if (msg->FindInt32("buttons", (int32*)&buttons) == B_OK
				&& buttons == B_SECONDARY_MOUSE_BUTTON) {
				BPopUpMenu menu("Reply To", false, false);
				menu.AddItem(new BMenuItem(B_TRANSLATE("Reply"),
					new BMessage(M_REPLY)));
				menu.AddItem(new BMenuItem(B_TRANSLATE("Reply to sender"),
					new BMessage(M_REPLY_TO_SENDER)));
				menu.AddItem(new BMenuItem(B_TRANSLATE("Reply to all"),
					new BMessage(M_REPLY_ALL)));

				BPoint where;
				msg->FindPoint("where", &where);

				BMenuItem* item;
				if ((item = menu.Go(where, false, false)) != NULL) {
					item->SetTarget(this);
					PostMessage(item->Message());
				}
				break;
			}
			// Fall through
		}
		case M_FORWARD:
		{
			// TODO: This needs removed in favor of a split toolbar button.
			// See comments for Spam button
			uint32 buttons;
			if (msg->FindInt32("buttons", (int32*)&buttons) == B_OK
				&& buttons == B_SECONDARY_MOUSE_BUTTON) {
				BPopUpMenu menu("Forward", false, false);
				menu.AddItem(new BMenuItem(B_TRANSLATE("Forward"),
					new BMessage(M_FORWARD)));
				menu.AddItem(new BMenuItem(
					B_TRANSLATE("Forward without attachments"),
					new BMessage(M_FORWARD_WITHOUT_ATTACHMENTS)));

				BPoint where;
				msg->FindPoint("where", &where);

				BMenuItem* item;
				if ((item = menu.Go(where, false, false)) != NULL) {
					item->SetTarget(this);
					PostMessage(item->Message());
				}
				break;
			}
		}

		// Fall Through
		case M_REPLY_ALL:
		case M_REPLY_TO_SENDER:
		case M_FORWARD_WITHOUT_ATTACHMENTS:
		case M_RESEND:
		case M_COPY_TO_NEW:
		{
			BMessage message(M_NEW);
			message.AddRef("ref", fRef);
			message.AddPointer("window", this);
			message.AddInt32("type", msg->what);
			be_app->PostMessage(&message);
			break;
		}
		case M_DELETE:
		case M_DELETE_PREV:
		case M_DELETE_NEXT:
		{
			if (msg->what == M_DELETE_NEXT && (modifiers() & B_SHIFT_KEY) != 0)
				msg->what = M_DELETE_PREV;

			bool foundRef = false;
			entry_ref nextRef;
			if ((msg->what == M_DELETE_PREV || msg->what == M_DELETE_NEXT)
				&& fRef != NULL) {
				// Find the next message that should be displayed
				nextRef = *fRef;
				foundRef = GetTrackerWindowFile(&nextRef,
					msg->what == M_DELETE_NEXT);
			}
			if (fIncoming) {
				read_flags flag = (fAutoMarkRead == true) ? B_READ : B_SEEN;
				MarkMessageRead(fRef, flag);
			}

			if (!fTrackerMessenger.IsValid() || !fIncoming) {
				// Not associated with a tracker window.  Create a new
				// messenger and ask the tracker to delete this entry
				if (fDraft || fIncoming) {
					BMessenger tracker("application/x-vnd.Be-TRAK");
					if (tracker.IsValid()) {
						BMessage msg('Ttrs');
						msg.AddRef("refs", fRef);
						tracker.SendMessage(&msg);
					} else {
						BAlert* alert = new BAlert("",
							B_TRANSLATE("Need Tracker to move items to trash"),
							B_TRANSLATE("Sorry"));
						alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
						alert->Go();
					}
				}
			} else {
				// This is associated with a tracker window.  Ask the
				// window to delete this entry.  Do it this way if we
				// can instead of the above way because it doesn't reset
				// the selection (even though we set selection below, this
				// still causes problems).
				BMessage delmsg(B_DELETE_PROPERTY);
				BMessage entryspec('sref');
				entryspec.AddRef("refs", fRef);
				entryspec.AddString("property", "Entry");
				delmsg.AddSpecifier(&entryspec);
				fTrackerMessenger.SendMessage(&delmsg);
			}

			// 	If the next file was found, open it.  If it was not,
			//	we have no choice but to close this window.
			if (foundRef) {
				TMailWindow* window
					= static_cast<TMailApp*>(be_app)->FindWindow(nextRef);
				if (window == NULL)
					OpenMessage(&nextRef, _CurrentCharacterSet());
				else
					window->Activate();

				SetTrackerSelectionToCurrent();

				if (window == NULL)
					break;
			}

			fSent = true;
			BMessage msg(B_CLOSE_REQUESTED);
			PostMessage(&msg);
			break;
		}

		case M_CLOSE_READ:
		{
			BMessage message(B_CLOSE_REQUESTED);
			message.AddString("status", "Read");
			PostMessage(&message);
			break;
		}
		case M_CLOSE_SAVED:
		{
			BMessage message(B_QUIT_REQUESTED);
			message.AddString("status", "Saved");
			PostMessage(&message);
			break;
		}
		case kMsgQuitAndKeepAllStatus:
			fKeepStatusOnQuit = true;
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;
		case M_CLOSE_CUSTOM:
			if (msg->HasString("status")) {
				BMessage message(B_CLOSE_REQUESTED);
				message.AddString("status", msg->GetString("status"));
				PostMessage(&message);
			} else {
				BRect r = Frame();
				r.left += ((r.Width() - STATUS_WIDTH) / 2);
				r.right = r.left + STATUS_WIDTH;
				r.top += 40;
				r.bottom = r.top + STATUS_HEIGHT;

				BString string = "could not read";
				BNode node(fRef);
				if (node.InitCheck() == B_OK)
					node.ReadAttrString(B_MAIL_ATTR_STATUS, &string);

				new TStatusWindow(r, this, string.String());
			}
			break;

		case M_STATUS:
		{
			const char* attribute;
			if (msg->FindString("attribute", &attribute) != B_OK)
				break;

			BMessage message(B_CLOSE_REQUESTED);
			message.AddString("status", attribute);
			PostMessage(&message);
			break;
		}
		case M_HEADER:
		{
			bool showHeader = !fHeader->IsMarked();
			fHeader->SetMarked(showHeader);

			BMessage message(M_HEADER);
			message.AddBool("header", showHeader);
			PostMessage(&message, fContentView->TextView());
			break;
		}
		case M_RAW:
		{
			bool raw = !(fRaw->IsMarked());
			fRaw->SetMarked(raw);
			BMessage message(M_RAW);
			message.AddBool("raw", raw);
			PostMessage(&message, fContentView->TextView());
			break;
		}
		case M_SEND_NOW:
		case M_SAVE_AS_DRAFT:
			Send(msg->what == M_SEND_NOW);
			break;

		case M_SAVE:
		{
			const char* address;
			if (msg->FindString("address", (const char**)&address) != B_OK)
				break;

			BVolumeRoster volumeRoster;
			BVolume volume;
			BQuery query;
			BEntry entry;
			bool foundEntry = false;

			char* arg = (char*)malloc(strlen("META:email=")
				+ strlen(address) + 1);
			sprintf(arg, "META:email=%s", address);

			// Search a Person file with this email address
			while (volumeRoster.GetNextVolume(&volume) == B_NO_ERROR) {
				if (!volume.KnowsQuery())
					continue;

				query.SetVolume(&volume);
				query.SetPredicate(arg);
				query.Fetch();

				if (query.GetNextEntry(&entry) == B_NO_ERROR) {
					BMessenger tracker("application/x-vnd.Be-TRAK");
					if (tracker.IsValid()) {
						entry_ref ref;
						entry.GetRef(&ref);

						BMessage open(B_REFS_RECEIVED);
						open.AddRef("refs", &ref);
						tracker.SendMessage(&open);
						foundEntry = true;
						break;
					}
				}
				// Try next volume, if any
				query.Clear();
			}

			if (!foundEntry) {
				// None found.
				// Ask to open a new Person file with this address pre-filled

				status_t result = be_roster->Launch("application/x-person",
					1, &arg);

				if (result != B_NO_ERROR) {
					BAlert* alert = new BAlert("", B_TRANSLATE(
						"Sorry, could not find an application that "
						"supports the 'Person' data type."),
						B_TRANSLATE("OK"));
					alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
					alert->Go();
				}
			}
			free(arg);
			break;
		}

		case M_READ_POS:
			PreserveReadingPos(false);
			break;

		case M_PRINT_SETUP:
			PrintSetup();
			break;

		case M_PRINT:
			Print();
			break;

		case M_SELECT:
			break;

		case M_FIND:
			FindWindow::Find(this);
			break;

		case M_FIND_AGAIN:
			FindWindow::FindAgain(this);
			break;

		case M_QUOTE:
		case M_REMOVE_QUOTE:
			PostMessage(msg->what, fContentView);
			break;

		case M_RANDOM_SIG:
		{
			BList		sigList;
			BMessage	*message;

			BVolume volume;
			BVolumeRoster().GetBootVolume(&volume);

			BQuery query;
			query.SetVolume(&volume);

			char predicate[128];
			sprintf(predicate, "%s = *", INDEX_SIGNATURE);
			query.SetPredicate(predicate);
			query.Fetch();

			BEntry entry;
			while (query.GetNextEntry(&entry) == B_NO_ERROR) {
				BFile file(&entry, O_RDONLY);
				if (file.InitCheck() == B_NO_ERROR) {
					entry_ref ref;
					entry.GetRef(&ref);

					message = new BMessage(M_SIGNATURE);
					message->AddRef("ref", &ref);
					sigList.AddItem(message);
				}
			}
			if (sigList.CountItems() > 0) {
				srand(time(0));
				PostMessage((BMessage*)sigList.ItemAt(rand()
					% sigList.CountItems()));

				for (int32 i = 0; (message = (BMessage*)sigList.ItemAt(i))
					!= NULL; i++)
					delete message;
			}
			break;
		}
		case M_SIGNATURE:
		{
			BMessage message(*msg);
			PostMessage(&message, fContentView);
			fSigAdded = true;
			break;
		}
		case M_SIG_MENU:
		{
			TMenu* menu;
			BMenuItem* item;
			menu = new TMenu("Add Signature", INDEX_SIGNATURE, M_SIGNATURE,
				true);

			BPoint where;
			if (msg->FindPoint("where", &where) != B_OK) {
				BRect bounds = fToolBar->Bounds();
				where = fToolBar->ConvertToScreen(BPoint(
					(bounds.right - bounds.left) / 2,
					(bounds.bottom - bounds.top) / 2));
			}

			if ((item = menu->Go(where, false, true)) != NULL) {
				item->SetTarget(this);
				(dynamic_cast<BInvoker*>(item))->Invoke();
			}
			delete menu;
			break;
		}

		case M_ADD:
			if (!fPanel) {
				BMessenger me(this);
				BMessage msg(REFS_RECEIVED);
				fPanel = new BFilePanel(B_OPEN_PANEL, &me, &fOpenFolder, false,
					true, &msg);
			} else if (!fPanel->Window()->IsHidden()) {
				fPanel->Window()->Activate();
			}

			if (fPanel->Window()->IsHidden())
				fPanel->Window()->Show();
			break;

		case M_REMOVE:
			PostMessage(msg->what, fEnclosuresView);
			break;

		case CHARSET_CHOICE_MADE:
		{
			int32 charSet;
			if (msg->FindInt32("charset", &charSet) != B_OK)
				break;

			BMessage update(FIELD_CHANGED);
			update.AddInt32("bitmask", 0);
				// just enable the save button
			PostMessage(&update);

			if (fIncoming && !fResending) {
				// The user wants to see the message they are reading (not
				// composing) displayed with a different kind of character set
				// for decoding.  Reload the whole message and redisplay.  For
				// messages which are being composed, the character set is
				// retrieved from the header view when it is needed.

				entry_ref fileRef = *fRef;
				OpenMessage(&fileRef, charSet);
			}
			break;
		}

		case REFS_RECEIVED:
			AddEnclosure(msg);
			break;

		//
		//	Navigation Messages
		//
		case M_UNREAD:
			MarkMessageRead(fRef, B_SEEN);
			_UpdateReadButton();
			PostMessage(M_NEXTMSG);
			break;
		case M_READ:
			wasReadMsg = true;
			_UpdateReadButton();
			msg->what = M_NEXTMSG;
		case M_PREVMSG:
		case M_NEXTMSG:
		{
			if (fRef == NULL)
				break;
			entry_ref orgRef = *fRef;
			entry_ref nextRef = *fRef;
			if (GetTrackerWindowFile(&nextRef, (msg->what == M_NEXTMSG))) {
				TMailWindow* window = static_cast<TMailApp*>(be_app)
					->FindWindow(nextRef);
				if (window == NULL) {
					BNode node(fRef);
					read_flags currentFlag;
					if (read_read_attr(node, currentFlag) != B_OK)
						currentFlag = B_UNREAD;
					if (fAutoMarkRead == true)
						MarkMessageRead(fRef, B_READ);
					else if (currentFlag != B_READ && !wasReadMsg)
						MarkMessageRead(fRef, B_SEEN);

					OpenMessage(&nextRef, _CurrentCharacterSet());
				} else {
					window->Activate();
					//fSent = true;
					PostMessage(B_CLOSE_REQUESTED);
				}

				SetTrackerSelectionToCurrent();
			} else {
				if (wasReadMsg)
					PostMessage(B_CLOSE_REQUESTED);

				beep();
			}
			if (wasReadMsg)
				MarkMessageRead(&orgRef, B_READ);
			break;
		}

		case M_SAVE_POSITION:
			if (fRef != NULL)
				SaveTrackerPosition(fRef);
			break;

		case RESET_BUTTONS:
			fChanged = false;
			fFieldState = 0;
			if (!fHeaderView->IsToEmpty())
				fFieldState |= FIELD_TO;
			if (!fHeaderView->IsSubjectEmpty())
				fFieldState |= FIELD_SUBJECT;
			if (!fHeaderView->IsCcEmpty())
				fFieldState |= FIELD_CC;
			if (!fHeaderView->IsBccEmpty())
				fFieldState |= FIELD_BCC;
			if (fContentView->TextView()->TextLength() != 0)
				fFieldState |= FIELD_BODY;

			fToolBar->SetActionEnabled(M_SAVE_AS_DRAFT, false);
			fToolBar->SetActionEnabled(M_PRINT, fFieldState);
			fToolBar->SetActionEnabled(M_SEND_NOW, (fFieldState & FIELD_TO)
				|| (fFieldState & FIELD_BCC));
			break;

		case M_CHECK_SPELLING:
			if (gDictCount == 0)
				// Give the application time to init and load dictionaries.
				snooze (1500000);
			if (!gDictCount) {
				beep();
				BAlert* alert = new BAlert("",
					B_TRANSLATE("Mail couldn't find its dictionary."),
					B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_AS_USUAL,
					B_OFFSET_SPACING, B_STOP_ALERT);
				alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
				alert->Go();
			} else {
				fSpelling->SetMarked(!fSpelling->IsMarked());
				fContentView->TextView()->EnableSpellCheck(
					fSpelling->IsMarked());
			}
			break;

		case M_QUERY_RECIPIENT:
		{
			BString searchText(fHeaderView->To());
			if (searchText != "") {
				_LaunchQuery(kSameRecipientItem, B_MAIL_ATTR_TO,
					searchText);
			}
			break;
		}

		case M_QUERY_SENDER:
		{
			BString searchText(fHeaderView->From());
			if (searchText != "") {
				_LaunchQuery(kSameSenderItem, B_MAIL_ATTR_FROM,
					searchText);
			}
			break;
		}

		case M_QUERY_SUBJECT:
		{
			// If there's no thread attribute (e.g. new mail) use subject
			BString searchText(fHeaderView->Subject());
			BNode node(fRef);
			if (node.InitCheck() == B_OK)
				node.ReadAttrString(B_MAIL_ATTR_THREAD, &searchText);

			if (searchText != "") {
				// query for subject as sent mails have no thread attribute
				_LaunchQuery(kSameSubjectItem, B_MAIL_ATTR_SUBJECT,
					searchText);
			}
			break;
		}
		case M_EDIT_QUERIES:
		{
			BPath path;
			if (_GetQueryPath(&path) < B_OK)
				break;

			// the user used this command, make sure the folder actually
			// exists - if it didn't inform the user what to do with it
			BEntry entry(path.Path());
			bool showAlert = false;
			if (!entry.Exists()) {
				showAlert = true;
				create_directory(path.Path(), 0777);
			}

			BEntry folderEntry;
			if (folderEntry.SetTo(path.Path()) == B_OK
				&& folderEntry.Exists()) {
				BMessage openFolderCommand(B_REFS_RECEIVED);
				BMessenger tracker("application/x-vnd.Be-TRAK");

				entry_ref ref;
				folderEntry.GetRef(&ref);
				openFolderCommand.AddRef("refs", &ref);
				tracker.SendMessage(&openFolderCommand);
			}

			if (showAlert) {
				// just some patience before Tracker pops up the folder
				snooze(250000);
				BAlert* alert = new BAlert(B_TRANSLATE("helpful message"),
					B_TRANSLATE("Put your favorite e-mail queries and query "
					"templates in this folder."), B_TRANSLATE("OK"), NULL, NULL,
					B_WIDTH_AS_USUAL, B_IDEA_ALERT);
				alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
				alert->Go(NULL);
			}

			break;
		}

		case B_PATH_MONITOR:
			_RebuildQueryMenu();
			break;

		default:
			BWindow::MessageReceived(msg);
	}
}


void
TMailWindow::AddEnclosure(BMessage* msg)
{
	if (fEnclosuresView == NULL && !fIncoming) {
		BRect r;
		r.left = 0;
		r.top = fHeaderView->Frame().bottom - 1;
		r.right = Frame().Width() + 2;
		r.bottom = r.top + ENCLOSURES_HEIGHT;

		fEnclosuresView = new TEnclosuresView(r, Frame());
		AddChild(fEnclosuresView, fContentView);
		fContentView->ResizeBy(0, -ENCLOSURES_HEIGHT);
		fContentView->MoveBy(0, ENCLOSURES_HEIGHT);
	}

	if (fEnclosuresView == NULL)
		return;

	if (msg && msg->HasRef("refs")) {
		// Add enclosure to view
		PostMessage(msg, fEnclosuresView);

		fChanged = true;
		BEntry entry;
		entry_ref ref;
		msg->FindRef("refs", &ref);
		entry.SetTo(&ref);
		entry.GetParent(&entry);
		entry.GetRef(&fOpenFolder);
	}
}


bool
TMailWindow::QuitRequested()
{
	int32 result;

	if ((!fIncoming || (fIncoming && fResending)) && fChanged && !fSent
		&& (!fHeaderView->IsToEmpty()
			|| !fHeaderView->IsSubjectEmpty()
			|| !fHeaderView->IsCcEmpty()
			|| !fHeaderView->IsBccEmpty()
			|| (fContentView->TextView() != NULL
				&& strlen(fContentView->TextView()->Text()))
			|| (fEnclosuresView != NULL
				&& fEnclosuresView->fList->CountItems()))) {
		if (fResending) {
			BAlert* alert = new BAlert("", B_TRANSLATE(
					"Send this message before closing?"),
				B_TRANSLATE("Cancel"),
				B_TRANSLATE("Don't send"),
				B_TRANSLATE("Send"),
				B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
			alert->SetShortcut(0, B_ESCAPE);
			alert->SetShortcut(1, 'd');
			alert->SetShortcut(2, 's');
			result = alert->Go();

			switch (result) {
				case 0:	// Cancel
					return false;
				case 1:	// Don't send
					break;
				case 2:	// Send
					Send(true);
					break;
			}
		} else {
			BAlert* alert = new BAlert("",
				B_TRANSLATE("Save this message as a draft before closing?"),
				B_TRANSLATE("Cancel"),
				B_TRANSLATE("Don't save"),
				B_TRANSLATE("Save"),
				B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
			alert->SetShortcut(0, B_ESCAPE);
			alert->SetShortcut(1, 'd');
			alert->SetShortcut(2, 's');
			result = alert->Go();
			switch (result) {
				case 0:	// Cancel
					return false;
				case 1:	// Don't Save
					break;
				case 2:	// Save
					Send(false);
					break;
			}
		}
	}

	BMessage message(WINDOW_CLOSED);
	message.AddInt32("kind", MAIL_WINDOW);
	message.AddPointer("window", this);
	be_app->PostMessage(&message);

	if (CurrentMessage() && CurrentMessage()->HasString("status")) {
		// User explicitly requests a status to set this message to.
		if (!CurrentMessage()->HasString("same")) {
			const char* status = CurrentMessage()->FindString("status");
			if (status != NULL) {
				BNode node(fRef);
				if (node.InitCheck() == B_NO_ERROR) {
					node.RemoveAttr(B_MAIL_ATTR_STATUS);
					WriteAttrString(&node, B_MAIL_ATTR_STATUS, status);
				}
			}
		}
	} else if (fRef != NULL && !fKeepStatusOnQuit) {
		// ...Otherwise just set the message read
		if (fAutoMarkRead == true)
			MarkMessageRead(fRef, B_READ);
		else {
			BNode node(fRef);
			read_flags currentFlag;
			if (read_read_attr(node, currentFlag) != B_OK)
				currentFlag = B_UNREAD;
			if (currentFlag == B_UNREAD)
				MarkMessageRead(fRef, B_SEEN);
		}
	}

	BPrivate::BPathMonitor::StopWatching(BMessenger(this, this));

	return true;
}


void
TMailWindow::Show()
{
	if (Lock()) {
		if (!fResending && (fIncoming || fReplying)) {
			fContentView->TextView()->MakeFocus(true);
		} else {
			fHeaderView->ToControl()->MakeFocus(true);
			fHeaderView->ToControl()->SelectAll();
		}
		Unlock();
	}
	BWindow::Show();
}


void
TMailWindow::Zoom(BPoint /*pos*/, float /*x*/, float /*y*/)
{
	float		height;
	float		width;

	BRect rect = Frame();
	width = 80 * fApp->ContentFont().StringWidth("M")
		+ (rect.Width() - fContentView->TextView()->Bounds().Width() + 6);

	BScreen screen(this);
	BRect screenFrame = screen.Frame();
	if (width > (screenFrame.Width() - 8))
		width = screenFrame.Width() - 8;

	height = max_c(fContentView->TextView()->CountLines(), 20)
		* fContentView->TextView()->LineHeight(0)
		+ (rect.Height() - fContentView->TextView()->Bounds().Height());
	if (height > (screenFrame.Height() - 29))
		height = screenFrame.Height() - 29;

	rect.right = rect.left + width;
	rect.bottom = rect.top + height;

	if (abs((int)(Frame().Width() - rect.Width())) < 5
		&& abs((int)(Frame().Height() - rect.Height())) < 5) {
		rect = fZoom;
	} else {
		fZoom = Frame();
		screenFrame.InsetBy(6, 6);

		if (rect.Width() > screenFrame.Width())
			rect.right = rect.left + screenFrame.Width();
		if (rect.Height() > screenFrame.Height())
			rect.bottom = rect.top + screenFrame.Height();

		if (rect.right > screenFrame.right) {
			rect.left -= rect.right - screenFrame.right;
			rect.right = screenFrame.right;
		}
		if (rect.bottom > screenFrame.bottom) {
			rect.top -= rect.bottom - screenFrame.bottom;
			rect.bottom = screenFrame.bottom;
		}
		if (rect.left < screenFrame.left) {
			rect.right += screenFrame.left - rect.left;
			rect.left = screenFrame.left;
		}
		if (rect.top < screenFrame.top) {
			rect.bottom += screenFrame.top - rect.top;
			rect.top = screenFrame.top;
		}
	}

	ResizeTo(rect.Width(), rect.Height());
	MoveTo(rect.LeftTop());
}


void
TMailWindow::WindowActivated(bool status)
{
	if (status) {
		BAutolock locker(sWindowListLock);
		sWindowList.RemoveItem(this);
		sWindowList.AddItem(this, 0);
	}
}


void
TMailWindow::Forward(entry_ref* ref, TMailWindow* window,
	bool includeAttachments)
{
	BEmailMessage* mail = window->Mail();
	if (mail == NULL)
		return;

	uint32 useAccountFrom = fApp->UseAccountFrom();

	fMail = mail->ForwardMessage(useAccountFrom == ACCOUNT_FROM_MAIL,
		includeAttachments);

	BFile file(ref, O_RDONLY);
	if (file.InitCheck() < B_NO_ERROR)
		return;

	fHeaderView->SetSubject(fMail->Subject());

	// set mail account

	if (useAccountFrom == ACCOUNT_FROM_MAIL)
		fHeaderView->SetAccount(fMail->Account());

	if (fMail->CountComponents() > 1) {
		// if there are any enclosures to be added, first add the enclosures
		// view to the window
		AddEnclosure(NULL);
		if (fEnclosuresView)
			fEnclosuresView->AddEnclosuresFromMail(fMail);
	}

	fContentView->TextView()->LoadMessage(fMail, false, NULL);
	fChanged = false;
	fFieldState = 0;
}


void
TMailWindow::Print()
{
	BPrintJob print(Title());

	if (!fApp->HasPrintSettings()) {
		if (print.Settings()) {
			fApp->SetPrintSettings(print.Settings());
		} else {
			PrintSetup();
			if (!fApp->HasPrintSettings())
				return;
		}
	}

	print.SetSettings(new BMessage(fApp->PrintSettings()));

	if (print.ConfigJob() == B_OK) {
		int32 curPage = 1;
		int32 lastLine = 0;
		BTextView header_view(print.PrintableRect(), "header",
			print.PrintableRect().OffsetByCopy(BPoint(
				-print.PrintableRect().left, -print.PrintableRect().top)),
			B_FOLLOW_ALL_SIDES);

		//---------Init the header fields
		#define add_header_field(label, field) { \
			/*header_view.SetFontAndColor(be_bold_font);*/ \
			header_view.Insert(label); \
			header_view.Insert(" "); \
			/*header_view.SetFontAndColor(be_plain_font);*/ \
			header_view.Insert(field); \
			header_view.Insert("\n"); \
		}

		add_header_field("Subject:", fHeaderView->Subject());
		add_header_field("To:", fHeaderView->To());
		if (!fHeaderView->IsCcEmpty())
			add_header_field(B_TRANSLATE("Cc:"), fHeaderView->Cc());

		if (!fHeaderView->IsDateEmpty())
			header_view.Insert(fHeaderView->Date());

		int32 maxLine = fContentView->TextView()->CountLines();
		BRect pageRect = print.PrintableRect();
		BRect curPageRect = pageRect;

		print.BeginJob();
		float header_height = header_view.TextHeight(0,
			header_view.CountLines());

		BRect rect(0, 0, pageRect.Width(), header_height);
		BBitmap bmap(rect, B_BITMAP_ACCEPTS_VIEWS, B_RGBA32);
		bmap.Lock();
		bmap.AddChild(&header_view);
		print.DrawView(&header_view, rect, BPoint(0.0, 0.0));
		HorizontalLine line(BRect(0, 0, pageRect.right, 0));
		bmap.AddChild(&line);
		print.DrawView(&line, line.Bounds(), BPoint(0, header_height + 1));
		bmap.Unlock();
		header_height += 5;

		do {
			int32 lineOffset = fContentView->TextView()->OffsetAt(lastLine);
			curPageRect.OffsetTo(0,
				fContentView->TextView()->PointAt(lineOffset).y);

			int32 fromLine = lastLine;
			lastLine = fContentView->TextView()->LineAt(
				BPoint(0.0, curPageRect.bottom - ((curPage == 1)
					? header_height : 0)));

			float curPageHeight = fContentView->TextView()->TextHeight(
				fromLine, lastLine) + (curPage == 1 ? header_height : 0);

			if (curPageHeight > pageRect.Height()) {
				curPageHeight = fContentView->TextView()->TextHeight(
					fromLine, --lastLine) + (curPage == 1 ? header_height : 0);
			}
			curPageRect.bottom = curPageRect.top + curPageHeight - 1.0;

			if (curPage >= print.FirstPage() && curPage <= print.LastPage()) {
				print.DrawView(fContentView->TextView(), curPageRect,
					BPoint(0.0, curPage == 1 ? header_height : 0.0));
				print.SpoolPage();
			}

			curPageRect = pageRect;
			lastLine++;
			curPage++;

		} while (print.CanContinue() && lastLine < maxLine);

		print.CommitJob();
		bmap.RemoveChild(&header_view);
		bmap.RemoveChild(&line);
	}
}


void
TMailWindow::PrintSetup()
{
	BPrintJob printJob("mail_print");

	if (fApp->HasPrintSettings()) {
		BMessage printSettings = fApp->PrintSettings();
		printJob.SetSettings(new BMessage(printSettings));
	}

	if (printJob.ConfigPage() == B_OK)
		fApp->SetPrintSettings(printJob.Settings());
}


void
TMailWindow::SetTo(const char* mailTo, const char* subject, const char* ccTo,
	const char* bccTo, const BString* body, BMessage* enclosures)
{
	Lock();

	if (mailTo != NULL && mailTo[0])
		fHeaderView->SetTo(mailTo);
	if (subject != NULL && subject[0])
		fHeaderView->SetSubject(subject);
	if (ccTo != NULL && ccTo[0])
		fHeaderView->SetCc(ccTo);
	if (bccTo != NULL && bccTo[0])
		fHeaderView->SetBcc(bccTo);

	if (body != NULL && body->Length()) {
		fContentView->TextView()->SetText(body->String(), body->Length());
		fContentView->TextView()->GoToLine(0);
	}

	if (enclosures && enclosures->HasRef("refs"))
		AddEnclosure(enclosures);

	Unlock();
}


void
TMailWindow::CopyMessage(entry_ref* ref, TMailWindow* src)
{
	BNode file(ref);
	if (file.InitCheck() == B_OK) {
		BString string;
		if (file.ReadAttrString(B_MAIL_ATTR_TO, &string) == B_OK)
			fHeaderView->SetTo(string);

		if (file.ReadAttrString(B_MAIL_ATTR_SUBJECT, &string) == B_OK)
			fHeaderView->SetSubject(string);

		if (file.ReadAttrString(B_MAIL_ATTR_CC, &string) == B_OK)
			fHeaderView->SetCc(string);
	}

	TTextView* text = src->fContentView->TextView();
	text_run_array* style = text->RunArray(0, text->TextLength());

	fContentView->TextView()->SetText(text->Text(), text->TextLength(), style);

	free(style);
}


void
TMailWindow::Reply(entry_ref* ref, TMailWindow* window, uint32 type)
{
	fRepliedMail = *ref;
	SetOriginatingWindow(window);

	BEmailMessage* mail = window->Mail();
	if (mail == NULL)
		return;

	if (type == M_REPLY_ALL)
		type = B_MAIL_REPLY_TO_ALL;
	else if (type == M_REPLY_TO_SENDER)
		type = B_MAIL_REPLY_TO_SENDER;
	else
		type = B_MAIL_REPLY_TO;

	uint32 useAccountFrom = fApp->UseAccountFrom();

	fMail = mail->ReplyMessage(mail_reply_to_mode(type),
		useAccountFrom == ACCOUNT_FROM_MAIL, QUOTE);

	// set header fields
	fHeaderView->SetTo(fMail->To());
	fHeaderView->SetCc(fMail->CC());
	fHeaderView->SetSubject(fMail->Subject());

	int32 accountID;
	BFile file(window->fRef, B_READ_ONLY);
	if (file.ReadAttr("MAIL:reply_with", B_INT32_TYPE, 0, &accountID,
		sizeof(int32)) != B_OK)
		accountID = -1;

	// set mail account

	if ((useAccountFrom == ACCOUNT_FROM_MAIL) || (accountID > -1)) {
		if (useAccountFrom == ACCOUNT_FROM_MAIL)
			fHeaderView->SetAccount(fMail->Account());
		else
			fHeaderView->SetAccount(accountID);
	}

	// create preamble string

	BString preamble = fApp->ReplyPreamble();

	BString name;
	mail->GetName(&name);
	if (name.Length() <= 0)
		name = B_TRANSLATE("(Name unavailable)");

	BString address(mail->From());
	if (address.Length() <= 0)
		address = B_TRANSLATE("(Address unavailable)");

	BString date(mail->HeaderField("Date"));
	if (date.Length() <= 0)
		date = B_TRANSLATE("(Date unavailable)");

	preamble.ReplaceAll("%n", name);
	preamble.ReplaceAll("%e", address);
	preamble.ReplaceAll("%d", date);
	preamble.ReplaceAll("\\n", "\n");

	// insert (if selection) or load (if whole mail) message text into text view

	int32 finish, start;
	window->fContentView->TextView()->GetSelection(&start, &finish);
	if (start != finish) {
		char* text = (char*)malloc(finish - start + 1);
		if (text == NULL)
			return;

		window->fContentView->TextView()->GetText(start, finish - start, text);
		if (text[strlen(text) - 1] != '\n') {
			text[strlen(text)] = '\n';
			finish++;
		}
		fContentView->TextView()->SetText(text, finish - start);
		free(text);

		finish = fContentView->TextView()->CountLines();
		for (int32 loop = 0; loop < finish; loop++) {
			fContentView->TextView()->GoToLine(loop);
			fContentView->TextView()->Insert((const char*)QUOTE);
		}

		if (fApp->ColoredQuotes()) {
			const BFont* font = fContentView->TextView()->Font();
			int32 length = fContentView->TextView()->TextLength();

			TextRunArray style(length / 8 + 8);

			FillInQuoteTextRuns(fContentView->TextView(), NULL,
				fContentView->TextView()->Text(), length, font, &style.Array(),
				style.MaxEntries());

			fContentView->TextView()->SetRunArray(0, length, &style.Array());
		}

		fContentView->TextView()->GoToLine(0);
		if (preamble.Length() > 0)
			fContentView->TextView()->Insert(preamble);
	} else {
		fContentView->TextView()->LoadMessage(mail, true, preamble);
	}

	fReplying = true;
}


status_t
TMailWindow::Send(bool now)
{
	if (!now) {
		status_t status = SaveAsDraft();
		if (status != B_OK) {
			beep();
			BAlert* alert = new BAlert("", B_TRANSLATE("E-mail draft could "
				"not be saved!"), B_TRANSLATE("OK"));
			alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
			alert->Go();
		}
		return status;
	}

	uint32 characterSetToUse = _CurrentCharacterSet();
	mail_encoding encodingForBody = quoted_printable;
	mail_encoding encodingForHeaders = quoted_printable;

	// Set up the encoding to use for converting binary to printable ASCII.
	// Normally this will be quoted printable, but for some old software,
	// particularly Japanese stuff, they only understand base64.  They also
	// prefer it for the smaller size.  Later on this will be reduced to 7bit
	// if the encoded text is just 7bit characters.
	if (characterSetToUse == B_SJIS_CONVERSION
		|| characterSetToUse == B_EUC_CONVERSION)
		encodingForBody = base64;
	else if (characterSetToUse == B_JIS_CONVERSION
		|| characterSetToUse == B_MAIL_US_ASCII_CONVERSION
		|| characterSetToUse == B_ISO1_CONVERSION
		|| characterSetToUse == B_EUC_KR_CONVERSION)
		encodingForBody = eight_bit;

	// Using quoted printable headers on almost completely non-ASCII Japanese
	// is a waste of time.  Besides, some stupid cell phone services need
	// base64 in the headers.
	if (characterSetToUse == B_SJIS_CONVERSION
		|| characterSetToUse == B_EUC_CONVERSION
		|| characterSetToUse == B_JIS_CONVERSION
		|| characterSetToUse == B_EUC_KR_CONVERSION)
		encodingForHeaders = base64;

	// Count the number of characters in the message body which aren't in the
	// currently selected character set.  Also see if the resulting encoded
	// text can safely use 7 bit characters.
	if (fContentView->TextView()->TextLength() > 0) {
		// First do a trial encoding with the user's character set.
		int32 converterState = 0;
		int32 originalLength;
		BString tempString;
		int32 tempStringLength;
		char* tempStringPntr;
		originalLength = fContentView->TextView()->TextLength();
		tempStringLength = originalLength * 6;
			// Some character sets bloat up on escape codes
		tempStringPntr = tempString.LockBuffer (tempStringLength);
		if (tempStringPntr != NULL && mail_convert_from_utf8(characterSetToUse,
				fContentView->TextView()->Text(), &originalLength,
				tempStringPntr, &tempStringLength, &converterState,
				0x1A /* used for unknown characters */) == B_OK) {
			// Check for any characters which don't fit in a 7 bit encoding.
			int i;
			bool has8Bit = false;
			for (i = 0; i < tempStringLength; i++) {
				if (tempString[i] == 0 || (tempString[i] & 0x80)) {
					has8Bit = true;
					break;
				}
			}
			if (!has8Bit)
				encodingForBody = seven_bit;
			tempString.UnlockBuffer (tempStringLength);

			// Count up the number of unencoded characters and warn the user
			if (fApp->WarnAboutUnencodableCharacters()) {
				// TODO: ideally, the encoding should be silently changed to
				// one that can express this character
				int32 offset = 0;
				int count = 0;
				while (offset >= 0) {
					offset = tempString.FindFirst (0x1A, offset);
					if (offset >= 0) {
						count++;
						offset++;
							// Don't get stuck finding the same character again.
					}
				}
				if (count > 0) {
					int32 userAnswer;
					BString	messageString;
					BString countString;
					countString << count;
					messageString << B_TRANSLATE("Your main text contains %ld"
						" unencodable characters. Perhaps a different "
						"character set would work better? Hit Send to send it "
						"anyway "
						"(a substitute character will be used in place of "
						"the unencodable ones), or choose Cancel to go back "
						"and try fixing it up.");
					messageString.ReplaceFirst("%ld", countString);
					BAlert* alert = new BAlert("Question", messageString.String(),
						B_TRANSLATE("Send"),
						B_TRANSLATE("Cancel"),
						NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING,
						B_WARNING_ALERT);
					alert->SetShortcut(1, B_ESCAPE);
					userAnswer = alert->Go();

					if (userAnswer == 1) {
						// Cancel was picked.
						return -1;
					}
				}
			}
		}
	}

	Hide();
		// depending on the system (and I/O) load, this could take a while
		// but the user shouldn't be left waiting

	status_t result;

	if (fResending) {
		BFile file(fRef, O_RDONLY);
		result = file.InitCheck();
		if (result == B_OK) {
			BEmailMessage mail(&file);
			mail.SetTo(fHeaderView->To(), characterSetToUse,
				encodingForHeaders);

			if (fHeaderView->AccountID() != ~0L)
				mail.SendViaAccount(fHeaderView->AccountID());

			result = mail.Send(now);
		}
	} else {
		if (fMail == NULL)
			// the mail will be deleted when the window is closed
			fMail = new BEmailMessage;

		// Had an embarrassing bug where replying to a message and clearing the
		// CC field meant that it got sent out anyway, so pass in empty strings
		// when changing the header to force it to remove the header.

		fMail->SetTo(fHeaderView->To(), characterSetToUse, encodingForHeaders);
		fMail->SetSubject(fHeaderView->Subject(), characterSetToUse,
			encodingForHeaders);
		fMail->SetCC(fHeaderView->Cc(), characterSetToUse, encodingForHeaders);
		fMail->SetBCC(fHeaderView->Bcc());

		//--- Add X-Mailer field
		{
			// get app version
			version_info info;
			memset(&info, 0, sizeof(version_info));

			app_info appInfo;
			if (be_app->GetAppInfo(&appInfo) == B_OK) {
				BFile file(&appInfo.ref, B_READ_ONLY);
				if (file.InitCheck() == B_OK) {
					BAppFileInfo appFileInfo(&file);
					if (appFileInfo.InitCheck() == B_OK)
						appFileInfo.GetVersionInfo(&info, B_APP_VERSION_KIND);
				}
			}

			char versionString[255];
			sprintf(versionString,
				"Mail/Haiku %" B_PRIu32 ".%" B_PRIu32 ".%" B_PRIu32,
				info.major, info.middle, info.minor);
			fMail->SetHeaderField("X-Mailer", versionString);
		}

		/****/

		// the content text is always added to make sure there is a mail body
		fMail->SetBodyTextTo("");
		fContentView->TextView()->AddAsContent(fMail, fApp->WrapMode(),
			characterSetToUse, encodingForBody);

		if (fEnclosuresView != NULL) {
			TListItem* item;
			int32 index = 0;
			while ((item = (TListItem*)fEnclosuresView->fList->ItemAt(index++))
				!= NULL) {
				if (item->Component())
					continue;

				// leave out missing enclosures
				BEntry entry(item->Ref());
				if (!entry.Exists())
					continue;

				fMail->Attach(item->Ref(), fApp->AttachAttributes());
			}
		}
		if (fHeaderView->AccountID() != ~0L)
			fMail->SendViaAccount(fHeaderView->AccountID());

		result = fMail->Send(now);

		if (fReplying) {
			// Set status of the replied mail

			BNode node(&fRepliedMail);
			if (node.InitCheck() >= B_OK) {
				if (fOriginatingWindow) {
					BMessage msg(M_SAVE_POSITION), reply;
					fOriginatingWindow->SendMessage(&msg, &reply);
				}
				WriteAttrString(&node, B_MAIL_ATTR_STATUS, "Replied");
			}
		}
	}

	bool close = false;
	BString errorMessage;

	switch (result) {
		case B_OK:
			close = true;
			fSent = true;

			// If it's a draft, remove the draft file
			if (fDraft) {
				BEntry entry(fRef);
				entry.Remove();
			}
			break;

		case B_MAIL_NO_DAEMON:
		{
			close = true;
			fSent = true;

			BAlert* alert = new BAlert("no daemon",
				B_TRANSLATE("The mail_daemon is not running. The message is "
					"queued and will be sent when the mail_daemon is started."),
				B_TRANSLATE("Start now"), B_TRANSLATE("OK"));
			alert->SetShortcut(1, B_ESCAPE);
			int32 start = alert->Go();

			if (start == 0) {
				BMailDaemon daemon;
				result = daemon.Launch();
				if (result == B_OK) {
					daemon.SendQueuedMail();
				} else {
					errorMessage
						<< B_TRANSLATE("The mail_daemon could not be "
							"started:\n\t")
						<< strerror(result);
				}
			}
			break;
		}

//		case B_MAIL_UNKNOWN_HOST:
//		case B_MAIL_ACCESS_ERROR:
//			sprintf(errorMessage,
//				"An error occurred trying to connect with the SMTP "
//				"host.  Check your SMTP host name.");
//			break;
//
//		case B_MAIL_NO_RECIPIENT:
//			sprintf(errorMessage,
//				"You must have either a \"To\" or \"Bcc\" recipient.");
//			break;

		default:
			errorMessage << "An error occurred trying to send mail:\n\t"
				<< strerror(result);
			break;
	}

	if (result != B_NO_ERROR && result != B_MAIL_NO_DAEMON) {
		beep();
		BAlert* alert = new BAlert("", errorMessage.String(),
			B_TRANSLATE("OK"));
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
	}
	if (close) {
		PostMessage(B_QUIT_REQUESTED);
	} else {
		// The window was hidden earlier
		Show();
	}

	return result;
}


status_t
TMailWindow::SaveAsDraft()
{
	BPath draftPath;
	BDirectory dir;
	BFile draft;
	uint32 flags = 0;

	if (fDraft) {
		status_t status = draft.SetTo(fRef,
				B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
		if (status != B_OK)
			return status;
	} else {
		// Get the user home directory
		status_t status = find_directory(B_USER_DIRECTORY, &draftPath);
		if (status != B_OK)
			return status;

		// Append the relative path of the draft directory
		draftPath.Append(kDraftPath);

		// Create the file
		status = dir.SetTo(draftPath.Path());
		switch (status) {
			// Create the directory if it does not exist
			case B_ENTRY_NOT_FOUND:
				if ((status = dir.CreateDirectory(draftPath.Path(), &dir))
					!= B_OK)
					return status;
			case B_OK:
			{
				char fileName[B_FILE_NAME_LENGTH];
				// save as some version of the message's subject
				if (fHeaderView->IsSubjectEmpty()) {
					strlcpy(fileName, B_TRANSLATE("Untitled"),
						sizeof(fileName));
				} else {
					strlcpy(fileName, fHeaderView->Subject(), sizeof(fileName));
				}

				uint32 originalLength = strlen(fileName);

				// convert /, \ and : to -
				for (char* bad = fileName; (bad = strchr(bad, '/')) != NULL;
						++bad) {
					*bad = '-';
				}
				for (char* bad = fileName; (bad = strchr(bad, '\\')) != NULL;
						++bad) {
					*bad = '-';
				}
				for (char* bad = fileName; (bad = strchr(bad, ':')) != NULL;
						++bad) {
					*bad = '-';
				}

				// Create the file; if the name exists, find a unique name
				flags = B_WRITE_ONLY | B_CREATE_FILE | B_FAIL_IF_EXISTS;
				int32 i = 1;
				do {
					status = draft.SetTo(&dir, fileName, flags);
					if (status == B_OK)
						break;
					char appendix[B_FILE_NAME_LENGTH];
					sprintf(appendix, " %" B_PRId32, i++);
					int32 pos = min_c(sizeof(fileName) - strlen(appendix),
						originalLength);
					sprintf(fileName + pos, "%s", appendix);
				} while (status == B_FILE_EXISTS);
				if (status != B_OK)
					return status;

				// Cache the ref
				if (fRef == NULL)
					fRef = new entry_ref;
				BEntry entry(&dir, fileName);
				entry.GetRef(fRef);
				break;
			}
			default:
				return status;
		}
	}

	// Write the content of the message
	draft.Write(fContentView->TextView()->Text(),
		fContentView->TextView()->TextLength());

	// Add the header stuff as attributes
	WriteAttrString(&draft, B_MAIL_ATTR_NAME, fHeaderView->To());
	WriteAttrString(&draft, B_MAIL_ATTR_TO, fHeaderView->To());
	WriteAttrString(&draft, B_MAIL_ATTR_SUBJECT, fHeaderView->Subject());
	if (!fHeaderView->IsCcEmpty())
		WriteAttrString(&draft, B_MAIL_ATTR_CC, fHeaderView->Cc());
	if (!fHeaderView->IsBccEmpty())
		WriteAttrString(&draft, B_MAIL_ATTR_BCC, fHeaderView->Bcc());

	// Add account
	if (fHeaderView->AccountName() != NULL) {
		WriteAttrString(&draft, B_MAIL_ATTR_ACCOUNT,
			fHeaderView->AccountName());
	}

	// Add encoding
	BMenuItem* menuItem = fEncodingMenu->FindMarked();
	if (menuItem != NULL)
		WriteAttrString(&draft, "MAIL:encoding", menuItem->Label());

	// Add the draft attribute for indexing
	uint32 draftAttr = true;
	draft.WriteAttr("MAIL:draft", B_INT32_TYPE, 0, &draftAttr, sizeof(uint32));

	// Add Attachment paths in attribute
	if (fEnclosuresView != NULL) {
		TListItem* item;
		BString pathStr;

		for (int32 i = 0; (item = (TListItem*)fEnclosuresView->fList->ItemAt(i))
				!= NULL; i++) {
			if (i > 0)
				pathStr.Append(":");

			BEntry entry(item->Ref(), true);
			if (!entry.Exists())
				continue;

			BPath path;
			entry.GetPath(&path);
			pathStr.Append(path.Path());
		}
		if (pathStr.Length())
			draft.WriteAttrString("MAIL:attachments", &pathStr);
	}

	// Set the MIME Type of the file
	BNodeInfo info(&draft);
	info.SetType(kDraftType);

	fDraft = true;
	fChanged = false;

	fToolBar->SetActionEnabled(M_SAVE_AS_DRAFT, false);

	return B_OK;
}


status_t
TMailWindow::TrainMessageAs(const char* commandWord)
{
	status_t	errorCode = -1;
	BEntry		fileEntry;
	BPath		filePath;
	BMessage	replyMessage;
	BMessage	scriptingMessage;
	team_id		serverTeam;

	if (fRef == NULL)
		goto ErrorExit; // Need to have a real file and name.
	errorCode = fileEntry.SetTo(fRef, true);
	if (errorCode != B_OK)
		goto ErrorExit;
	errorCode = fileEntry.GetPath(&filePath);
	if (errorCode != B_OK)
		goto ErrorExit;
	fileEntry.Unset();

	// Get a connection to the spam database server.  Launch if needed.

	if (!fMessengerToSpamServer.IsValid()) {
		// Make sure the server is running.
		if (!be_roster->IsRunning (kSpamServerSignature)) {
			errorCode = be_roster->Launch (kSpamServerSignature);
			if (errorCode != B_OK) {
				BPath path;
				entry_ref ref;
				directory_which places[] = {B_SYSTEM_NONPACKAGED_BIN_DIRECTORY,
					B_SYSTEM_BIN_DIRECTORY};
				for (int32 i = 0; i < 2; i++) {
					find_directory(places[i],&path);
					path.Append("spamdbm");
					if (!BEntry(path.Path()).Exists())
						continue;
					get_ref_for_path(path.Path(),&ref);

					errorCode = be_roster->Launch(&ref);
					if (errorCode == B_OK)
						break;
				}
				if (errorCode != B_OK)
					goto ErrorExit;
			}
		}

		// Set up the messenger to the database server.
		errorCode = B_SERVER_NOT_FOUND;
		serverTeam = be_roster->TeamFor(kSpamServerSignature);
		if (serverTeam < 0)
			goto ErrorExit;

		fMessengerToSpamServer = BMessenger (kSpamServerSignature, serverTeam,
			&errorCode);

		if (!fMessengerToSpamServer.IsValid())
			goto ErrorExit;
	}

	// Ask the server to train on the message.  Give it the command word and
	// the absolute path name to use.

	scriptingMessage.MakeEmpty();
	scriptingMessage.what = B_SET_PROPERTY;
	scriptingMessage.AddSpecifier(commandWord);
	errorCode = scriptingMessage.AddData("data", B_STRING_TYPE,
		filePath.Path(), strlen(filePath.Path()) + 1, false);
	if (errorCode != B_OK)
		goto ErrorExit;
	replyMessage.MakeEmpty();
	errorCode = fMessengerToSpamServer.SendMessage(&scriptingMessage,
		&replyMessage);
	if (errorCode != B_OK
		|| replyMessage.FindInt32("error", &errorCode) != B_OK
		|| errorCode != B_OK)
		goto ErrorExit; // Classification failed in one of many ways.

	SetTitleForMessage();
		// Update window title to show new spam classification.
	return B_OK;

ErrorExit:
	beep();
	char errorString[1500];
	snprintf(errorString, sizeof(errorString), "Unable to train the message "
		"file \"%s\" as %s.  Possibly useful error code: %s (%" B_PRId32 ").",
		filePath.Path(), commandWord, strerror(errorCode), errorCode);
	BAlert* alert = new BAlert("", errorString,	B_TRANSLATE("OK"));
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go();

	return errorCode;
}


void
TMailWindow::SetTitleForMessage()
{
	// Figure out the title of this message and set the title bar
	BString title = B_TRANSLATE_SYSTEM_NAME("Mail");

	if (fIncoming) {
		if (fMail->GetName(&title) == B_OK)
			title << ": \"" << fMail->Subject() << "\"";
		else
			title = fMail->Subject();

		if (fDownloading)
			title.Prepend("Downloading: ");

		if (fApp->ShowSpamGUI() && fRef != NULL) {
			BString	classification;
			BNode node(fRef);
			char numberString[30];
			BString oldTitle(title);
			float spamRatio;
			if (node.InitCheck() != B_OK || node.ReadAttrString(
					"MAIL:classification", &classification) != B_OK)
				classification = "Unrated";
			if (classification != "Spam" && classification != "Genuine") {
				// Uncertain, Unrated and other unknown classes, show the ratio.
				if (node.InitCheck() == B_OK && node.ReadAttr("MAIL:ratio_spam",
						B_FLOAT_TYPE, 0, &spamRatio, sizeof(spamRatio))
							== sizeof(spamRatio)) {
					sprintf(numberString, "%.4f", spamRatio);
					classification << " " << numberString;
				}
			}
			title = "";
			title << "[" << classification << "] " << oldTitle;
		}
	}
	SetTitle(title);
}


/*!	Open *another* message in the existing mail window.  Some code here is
	duplicated from various constructors.
	TODO: The duplicated code should be moved to a private initializer method
*/
status_t
TMailWindow::OpenMessage(const entry_ref* ref, uint32 characterSetForDecoding)
{
	if (ref == NULL)
		return B_ERROR;

	// Set some references to the email file
	delete fRef;
	fRef = new entry_ref(*ref);

	fPrevTrackerPositionSaved = false;
	fNextTrackerPositionSaved = false;

	fContentView->TextView()->StopLoad();
	delete fMail;
	fMail = NULL;

	BFile file(fRef, B_READ_ONLY);
	status_t err = file.InitCheck();
	if (err != B_OK)
		return err;

	char mimeType[256];
	BNodeInfo fileInfo(&file);
	fileInfo.GetType(mimeType);

	if (strcmp(mimeType, B_PARTIAL_MAIL_TYPE) == 0) {
		BMessenger listener(this);
		status_t status = BMailDaemon().FetchBody(*ref, &listener);
		if (status != B_OK)
			fprintf(stderr, "Could not fetch body: %s\n", strerror(status));
		fileInfo.GetType(mimeType);
		_SetDownloading(true);
	} else
		_SetDownloading(false);

	// Check if it's a draft file, which contains only the text, and has the
	// from, to, bcc, attachments listed as attributes.
	if (strcmp(kDraftType, mimeType) == 0) {
		BNode node(fRef);
		off_t size;
		BString string;

		fMail = new BEmailMessage; // Not really used much, but still needed.

		// Load the raw UTF-8 text from the file.
		file.GetSize(&size);
		fContentView->TextView()->SetText(&file, 0, size);

		// Restore Fields from attributes
		if (node.ReadAttrString(B_MAIL_ATTR_TO, &string) == B_OK)
			fHeaderView->SetTo(string);
		if (node.ReadAttrString(B_MAIL_ATTR_SUBJECT, &string) == B_OK)
			fHeaderView->SetSubject(string);
		if (node.ReadAttrString(B_MAIL_ATTR_CC, &string) == B_OK)
			fHeaderView->SetCc(string);
		if (node.ReadAttrString(B_MAIL_ATTR_BCC, &string) == B_OK)
			fHeaderView->SetBcc(string);

		// Restore account
		if (node.ReadAttrString(B_MAIL_ATTR_ACCOUNT, &string) == B_OK)
			fHeaderView->SetAccount(string);

		// Restore encoding
		if (node.ReadAttrString("MAIL:encoding", &string) == B_OK) {
			BMenuItem* encodingItem = fEncodingMenu->FindItem(string.String());
			if (encodingItem != NULL)
				encodingItem->SetMarked(true);
		}

		// Restore attachments
		if (node.ReadAttrString("MAIL:attachments", &string) == B_OK) {
			BMessage msg(REFS_RECEIVED);
			entry_ref enc_ref;

			char* s = strtok((char*)string.String(), ":");
			while (s != NULL) {
				BEntry entry(s, true);
				if (entry.Exists()) {
					entry.GetRef(&enc_ref);
					msg.AddRef("refs", &enc_ref);
				}
				s = strtok(NULL, ":");
			}
			AddEnclosure(&msg);
		}

		// restore the reading position if available
		PostMessage(M_READ_POS);

		PostMessage(RESET_BUTTONS);
		fIncoming = false;
		fDraft = true;
	} else {
		// A real mail message, parse its headers to get from, to, etc.
		fMail = new BEmailMessage(fRef, characterSetForDecoding);
		fIncoming = true;
		fHeaderView->SetFromMessage(fMail);
	}

	err = fMail->InitCheck();
	if (err < B_OK) {
		delete fMail;
		fMail = NULL;
		return err;
	}

	SetTitleForMessage();

	if (fIncoming) {
		//	Put the addresses in the 'Save Address' Menu
		BMenuItem* item;
		while ((item = fSaveAddrMenu->RemoveItem((int32)0)) != NULL)
			delete item;

		// create the list of addresses

		BList addressList;
		get_address_list(addressList, fMail->To(), extract_address);
		get_address_list(addressList, fMail->CC(), extract_address);
		get_address_list(addressList, fMail->From(), extract_address);
		get_address_list(addressList, fMail->ReplyTo(), extract_address);

		BMessage* msg;

		for (int32 i = addressList.CountItems(); i-- > 0;) {
			char* address = (char*)addressList.RemoveItem((int32)0);

			// insert the new address in alphabetical order
			int32 index = 0;
			while ((item = fSaveAddrMenu->ItemAt(index)) != NULL) {
				if (!strcmp(address, item->Label())) {
					// item already in list
					goto skip;
				}

				if (strcmp(address, item->Label()) < 0)
					break;

				index++;
			}

			msg = new BMessage(M_SAVE);
			msg->AddString("address", address);
			fSaveAddrMenu->AddItem(new BMenuItem(address, msg), index);

		skip:
			free(address);
		}

		// Clear out existing contents of text view.
		fContentView->TextView()->SetText("", (int32)0);

		fContentView->TextView()->LoadMessage(fMail, false, NULL);

		if (fApp->ShowToolBar())
			_UpdateReadButton();
	}

	return B_OK;
}


TMailWindow*
TMailWindow::FrontmostWindow()
{
	BAutolock locker(sWindowListLock);
	if (sWindowList.CountItems() > 0)
		return (TMailWindow*)sWindowList.ItemAt(0);

	return NULL;
}


// #pragma mark -


status_t
TMailWindow::_GetQueryPath(BPath* queryPath) const
{
	// get the user home directory and from there the query folder
	status_t ret = find_directory(B_USER_DIRECTORY, queryPath);
	if (ret == B_OK)
		ret = queryPath->Append(kQueriesDirectory);

	return ret;
}


void
TMailWindow::_RebuildQueryMenu(bool firstTime)
{
	while (fQueryMenu->ItemAt(0)) {
		BMenuItem* item = fQueryMenu->RemoveItem((int32)0);
		delete item;
	}

	fQueryMenu->AddItem(new BMenuItem(kSameRecipientItem,
			new BMessage(M_QUERY_RECIPIENT)));
	fQueryMenu->AddItem(new BMenuItem(kSameSenderItem,
			new BMessage(M_QUERY_SENDER)));
	fQueryMenu->AddItem(new BMenuItem(kSameSubjectItem,
			new BMessage(M_QUERY_SUBJECT)));

	bool queryItemsAdded = false;

	BPath queryPath;
	if (_GetQueryPath(&queryPath) < B_OK)
		return;

	BDirectory queryDir(queryPath.Path());

	if (firstTime) {
		BPrivate::BPathMonitor::StartWatching(queryPath.Path(),
			B_WATCH_RECURSIVELY, BMessenger(this, this));
	}

	// If we find the named query, add it to the menu.
	BEntry entry;
	while (queryDir.GetNextEntry(&entry) == B_OK) {
		char name[B_FILE_NAME_LENGTH + 1];
		entry.GetName(name);

		char* queryString = _BuildQueryString(&entry);
		if (queryString == NULL)
			continue;

		queryItemsAdded = true;

		QueryMenu* queryMenu = new QueryMenu(name, false);
		queryMenu->SetTargetForItems(be_app);
		queryMenu->SetPredicate(queryString);
		fQueryMenu->AddItem(queryMenu);

		free(queryString);
	}

	fQueryMenu->AddItem(new BSeparatorItem());

	fQueryMenu->AddItem(new BMenuItem(B_TRANSLATE("Edit queries"
			B_UTF8_ELLIPSIS),
		new BMessage(M_EDIT_QUERIES), 'E', B_SHIFT_KEY));
}


char*
TMailWindow::_BuildQueryString(BEntry* entry) const
{
	BNode node(entry);
	if (node.InitCheck() != B_OK)
		return NULL;

	uint32 mode;
	if (node.ReadAttr(kAttrQueryInitialMode, B_INT32_TYPE, 0, (int32*)&mode,
		sizeof(int32)) <= 0) {
		mode = kByNameItem;
	}

	BString queryString;
	switch (mode) {
		case kByForumlaItem:
		{
			BString buffer;
			if (node.ReadAttrString(kAttrQueryInitialString, &buffer) == B_OK)
				queryString << buffer;
			break;
		}

		case kByNameItem:
		{
			BString buffer;
			if (node.ReadAttrString(kAttrQueryInitialString, &buffer) == B_OK)
				queryString << "(name==*" << buffer << "*)";
			break;
		}

		case kByAttributeItem:
		{
			int32 count = 1;
			if (node.ReadAttr(kAttrQueryInitialNumAttrs, B_INT32_TYPE, 0,
					(int32*)&count, sizeof(int32)) <= 0) {
				count = 1;
			}

			attr_info info;
			if (node.GetAttrInfo(kAttrQueryInitialAttrs, &info) != B_OK)
				break;

			if (count > 1)
				queryString << "(";

			char* buffer = new char[info.size];
			if (node.ReadAttr(kAttrQueryInitialAttrs, B_MESSAGE_TYPE, 0,
					buffer, (size_t)info.size) == info.size) {
				BMessage message;
				if (message.Unflatten(buffer) == B_OK) {
					for (int32 index = 0; /*index < count*/; index++) {
						const char* field;
						const char* value;
						if (message.FindString("menuSelection", index, &field)
								!= B_OK
							|| message.FindString("attrViewText", index, &value)
								!= B_OK) {
							break;
						}

						// ignore the mime type, we'll force it to be email
						// later
						if (strcmp(field, "BEOS:TYPE") != 0) {
							// TODO: check if subMenu contains the type of
							// comparison we are suppose to make here
							queryString << "(" << field << "==\""
								<< value << "\")";

							int32 logicMenuSelectedIndex;
							if (message.FindInt32("logicalRelation", index,
								&logicMenuSelectedIndex) == B_OK) {
								if (logicMenuSelectedIndex == 0)
									queryString << "&&";
								else if (logicMenuSelectedIndex == 1)
									queryString << "||";
							} else
								break;
						}
					}
				}
			}

			if (count > 1)
				queryString << ")";

			delete [] buffer;
			break;
		}

		default:
			break;
	}

	if (queryString.Length() == 0)
		return NULL;

	// force it to check for email only
	if (queryString.FindFirst("text/x-email") < 0) {
		BString temp;
		temp << "(" << queryString << "&&(BEOS:TYPE==\"text/x-email\"))";
		queryString = temp;
	}

	return strdup(queryString.String());
}


void
TMailWindow::_LaunchQuery(const char* title, const char* attribute,
	BString text)
{
/*	ToDo:
	If the search attribute is To or From, it'd be nice to parse the
	search text to separate the email address and user name.
	Then search for 'name || address' to get all mails of people,
	never mind the account or mail config they sent from.
*/
	text.ReplaceAll(" ", "*"); // query on MAIL:track demands * for space
	text.ReplaceAll("\"", "\\\"");

	BString* term = new BString("((");
	term->Append(attribute);
	term->Append("==\"*");
	term->Append(text);
	term->Append("*\")&&(BEOS:TYPE==\"text/x-email\"))");

	BPath queryPath;
	if (find_directory(B_USER_CACHE_DIRECTORY, &queryPath) != B_OK)
		return;
	queryPath.Append("Mail");
	if ((create_directory(queryPath.Path(), 0777)) != B_OK)
		return;
	queryPath.Append(title);
	BFile query(queryPath.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (query.InitCheck() != B_OK)
		return;

	BNode queryNode(queryPath.Path());
	if (queryNode.InitCheck() != B_OK)
		return;

	// Copy layout from DefaultQueryTemplates
	BPath templatePath;
	find_directory(B_USER_SETTINGS_DIRECTORY, &templatePath);
	templatePath.Append("Tracker/DefaultQueryTemplates/text_x-email");
	BNode templateNode(templatePath.Path());

	if (templateNode.InitCheck() == B_OK) {
		if (CopyAttributes(templateNode, queryNode) != B_OK) {
			syslog(LOG_INFO, "Mail: copying x-email DefaultQueryTemplate "
				"attributes failed");
		}
	}

	queryNode.WriteAttrString("_trk/qrystr", term);
	BNodeInfo nodeInfo(&queryNode);
	nodeInfo.SetType("application/x-vnd.Be-query");

	// Launch query
	BEntry entry(queryPath.Path());
	entry_ref ref;
	if (entry.GetRef(&ref) == B_OK)
		be_roster->Launch(&ref);
}


void
TMailWindow::_AddReadButton()
{
	BNode node(fRef);

	read_flags flag = B_UNREAD;
	read_read_attr(node, flag);

	if (flag == B_READ) {
		fToolBar->SetActionVisible(M_UNREAD, true);
		fToolBar->SetActionVisible(M_READ, false);
	} else {
		fToolBar->SetActionVisible(M_UNREAD, false);
		fToolBar->SetActionVisible(M_READ, true);
	}
}


void
TMailWindow::_UpdateReadButton()
{
	if (fApp->ShowToolBar()) {
		if (!fAutoMarkRead && fIncoming)
			_AddReadButton();
	}
	UpdateViews();
}


void
TMailWindow::_UpdateLabel(uint32 command, const char* label, bool show)
{
	BButton* button = fToolBar->FindButton(command);
	if (button != NULL) {
		button->SetLabel(show ? label : NULL);
		button->SetToolTip(show ? NULL : label);
	}
}


void
TMailWindow::_SetDownloading(bool downloading)
{
	fDownloading = downloading;
}


uint32
TMailWindow::_CurrentCharacterSet() const
{
	uint32 defaultCharSet = fResending || !fIncoming
		? fApp->MailCharacterSet() : B_MAIL_NULL_CONVERSION;

	BMenuItem* marked = fEncodingMenu->FindMarked();
	if (marked == NULL)
		return defaultCharSet;

	return marked->Message()->GetInt32("charset", defaultCharSet);
}
