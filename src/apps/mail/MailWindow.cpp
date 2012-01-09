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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/utsname.h>

#include <Autolock.h>
#include <Clipboard.h>
#include <Debug.h>
#include <E-mail.h>
#include <File.h>
#include <InterfaceKit.h>
#include <Locale.h>
#include <Node.h>
#include <PathMonitor.h>
#include <Roster.h>
#include <Screen.h>
#include <StorageKit.h>
#include <String.h>
#include <TextView.h>
#include <UTF8.h>

#include <fs_index.h>
#include <fs_info.h>

#include <MailMessage.h>
#include <MailSettings.h>
#include <MailDaemon.h>
#include <mail_util.h>

#include <CharacterSetRoster.h>

#include "ButtonBar.h"
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
#include "Status.h"
#include "String.h"
#include "Utilities.h"


#define B_TRANSLATE_CONTEXT "Mail"


using namespace BPrivate;


const char *kUndoStrings[] = {
	"Undo",
	"Undo typing",
	"Undo cut",
	"Undo paste",
	"Undo clear",
	"Undo drop"
};

const char *kRedoStrings[] = {
	"Redo",
	"Redo typing",
	"Redo cut",
	"Redo paste",
	"Redo clear",
	"Redo drop"
};


// Text for both the main menu and the pop-up menu.
static const char *kSpamMenuItemTextArray[] = {
	"Mark as spam and move to trash",		// M_TRAIN_SPAM_AND_DELETE
	"Mark as spam",							// M_TRAIN_SPAM
	"Unmark this message",					// M_UNTRAIN
	"Mark as genuine"						// M_TRAIN_GENUINE
};

static const uint32 kMsgQuitAndKeepAllStatus = 'Casm';

static const char *kQueriesDirectory = "mail/queries";
static const char *kAttrQueryInitialMode = "_trk/qryinitmode";
	// taken from src/kits/tracker/Attributes.h
static const char *kAttrQueryInitialString = "_trk/qryinitstr";
static const char *kAttrQueryInitialNumAttrs = "_trk/qryinitnumattrs";
static const char *kAttrQueryInitialAttrs = "_trk/qryinitattrs";
static const uint32 kAttributeItemMain = 'Fatr';
	// taken from src/kits/tracker/FindPanel.h
static const uint32 kByNameItem = 'Fbyn';
	// taken from src/kits/tracker/FindPanel.h
static const uint32 kByAttributeItem = 'Fbya';
	// taken from src/kits/tracker/FindPanel.h
static const uint32 kByForumlaItem = 'Fbyq';
	// taken from src/kits/tracker/FindPanel.h


// static list for tracking of Windows
BList TMailWindow::sWindowList;
BLocker TMailWindow::sWindowListLock;


//	#pragma mark -


TMailWindow::TMailWindow(BRect rect, const char* title, TMailApp* app,
	const entry_ref* ref, const char* to, const BFont* font, bool resending,
	BMessenger* messenger)
	:
	BWindow(rect, title, B_DOCUMENT_WINDOW, 0),

	fApp(app),
	fMail(NULL),
	fRef(NULL),
	fFieldState(0),
	fPanel(NULL),
	fLeaveStatusMenu(NULL),
	fSendButton(NULL),
	fSaveButton(NULL),
	fPrintButton(NULL),
	fSigButton(NULL),
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
	fStartingText(NULL),
	fOriginatingWindow(NULL),
	fReadButton(NULL),
	fNextButton(NULL),

	fDownloading(false)
{
	fKeepStatusOnQuit = false;

	if (messenger != NULL)
		fTrackerMessenger = *messenger;

	float height;
	BMenu* menu;
	BMenu* subMenu;
	BMenuItem* item;
	BMessage* msg;
	BFile file(ref, B_READ_ONLY);

	if (ref) {
		fRef = new entry_ref(*ref);
		fIncoming = true;
	} else
		fIncoming = false;

	fAutoMarkRead = fApp->AutoMarkRead();
	BRect r(0, 0, RIGHT_BOUNDARY, 15);
	fMenuBar = new BMenuBar(r, "");

	// File Menu

	menu = new BMenu(B_TRANSLATE("File"));

	msg = new BMessage(M_NEW);
	msg->AddInt32("type", M_NEW);
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("New mail message"),
		msg, 'N'));
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
		QueryMenu *queryMenu;
		queryMenu = new QueryMenu(B_TRANSLATE("Open draft"), false);
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

		subMenu = new BMenu(B_TRANSLATE("Close and "));

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
		menu->AddItem(fQuote =new BMenuItem(B_TRANSLATE("Quote"),
			new BMessage(M_QUOTE), B_RIGHT_ARROW));
		menu->AddItem(fRemoveQuote = new BMenuItem(B_TRANSLATE("Remove quote"),
			new BMessage(M_REMOVE_QUOTE), B_LEFT_ARROW));
		menu->AddSeparatorItem();
		fSpelling = new BMenuItem(B_TRANSLATE("Check spelling"),
			new BMessage(M_CHECK_SPELLING), ';');
		menu->AddItem(fSpelling);
		if (fApp->StartWithSpellCheckOn())
			PostMessage (M_CHECK_SPELLING);
	}
	menu->AddSeparatorItem();
	menu->AddItem(item = new BMenuItem(
		B_TRANSLATE("Preferences" B_UTF8_ELLIPSIS),
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
		BMenuItem *menuItem;
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
		menu->AddSeparatorItem();
		fSaveAddrMenu = subMenu = new BMenu(B_TRANSLATE("Save address"));
		menu->AddItem(subMenu);
		fMenuBar->AddItem(menu);

		// Spam Menu

		if (fApp->ShowSpamGUI()) {
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
		fMenuBar->AddItem(menu);
	}

	// Queries Menu

	fQueryMenu = new BMenu(B_TRANSLATE("Queries"));
	fMenuBar->AddItem(fQueryMenu);

	_RebuildQueryMenu(true);

	// Menu Bar

	AddChild(fMenuBar);
	height = fMenuBar->Bounds().bottom + 1;

	// Button Bar

	float bbwidth = 0, bbheight = 0;

	bool showButtonBar = fApp->ShowButtonBar();

	if (showButtonBar) {
		BuildButtonBar();
		fButtonBar->ShowLabels(showButtonBar);
		fButtonBar->Arrange(true);
		fButtonBar->GetPreferredSize(&bbwidth, &bbheight);
		fButtonBar->ResizeTo(Bounds().right, bbheight);
		fButtonBar->MoveTo(0, height);
		fButtonBar->Show();
	} else
		fButtonBar = NULL;

	r.top = r.bottom = height + bbheight + 1;
	fHeaderView = new THeaderView (r, rect, fIncoming, resending,
		(resending || !fIncoming)
		? fApp->MailCharacterSet()
			// Use preferences setting for composing mail.
		: B_MAIL_NULL_CONVERSION,
			// Default is automatic selection for reading mail.
		fApp->DefaultAccount());

	r = Frame();
	r.OffsetTo(0, 0);
	r.top = fHeaderView->Frame().bottom - 1;
	fContentView = new TContentView(r, fIncoming, const_cast<BFont *>(font),
		false, fApp->ColoredQuotes());
		// TContentView needs to be properly const, for now cast away constness

	AddChild(fHeaderView);
	if (fEnclosuresView)
		AddChild(fEnclosuresView);
	AddChild(fContentView);

	if (to)
		fHeaderView->fTo->SetText(to);

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
				off_t size;
				BFile file;
				file.SetTo(&entry, O_RDWR);
				if (file.InitCheck() == B_NO_ERROR) {
					file.GetSize(&size);
					char *str = (char *)malloc(size);
					size = file.Read(str, size);

					fContentView->fTextView->Insert(str, size);
					fContentView->fTextView->GoToLine(0);
					fContentView->fTextView->ScrollToSelection();

					fStartingText = (char *)malloc(size
						= strlen(fContentView->fTextView->Text()) + 1);
					if (fStartingText != NULL)
						strcpy(fStartingText, fContentView->fTextView->Text());
				}
			} else {
				char tempString [2048];
				query.GetPredicate (tempString, sizeof (tempString));
				printf ("Query failed, was looking for: %s\n", tempString);
			}
		}
	}

	OpenMessage(ref, fHeaderView->fCharacterSetUserSees);

	_UpdateSizeLimits();

	AddShortcut('q', B_SHIFT_KEY, new BMessage(kMsgQuitAndKeepAllStatus));
}


void
TMailWindow::BuildButtonBar()
{
	ButtonBar *bbar;

	bbar = new ButtonBar(BRect(0, 0, 100, 100), "ButtonBar", 2, 3, 0, 1, 10,
		2);
	bbar->AddButton(B_TRANSLATE("New"), 28, new BMessage(M_NEW));
	bbar->AddDivider(5);
	fButtonBar = bbar;

	if (fResending) {
		fSendButton = bbar->AddButton(B_TRANSLATE("Send"), 8,
			new BMessage(M_SEND_NOW));
		bbar->AddDivider(5);
	} else if (!fIncoming) {
		fSendButton = bbar->AddButton(B_TRANSLATE("Send"), 8,
			new BMessage(M_SEND_NOW));
		fSendButton->SetEnabled(false);
		fSigButton = bbar->AddButton(B_TRANSLATE("Signature"), 4,
			new BMessage(M_SIG_MENU));
		fSigButton->InvokeOnButton(B_SECONDARY_MOUSE_BUTTON);
		fSaveButton = bbar->AddButton(B_TRANSLATE("Save"), 44,
			new BMessage(M_SAVE_AS_DRAFT));
		fSaveButton->SetEnabled(false);
		fPrintButton = bbar->AddButton(B_TRANSLATE("Print"), 16,
			new BMessage(M_PRINT));
		fPrintButton->SetEnabled(false);
		bbar->AddButton(B_TRANSLATE("Trash"), 0, new BMessage(M_DELETE));
		bbar->AddDivider(5);
	} else {
		BmapButton *button = bbar->AddButton(B_TRANSLATE("Reply"), 12,
			new BMessage(M_REPLY));
		button->InvokeOnButton(B_SECONDARY_MOUSE_BUTTON);
		button = bbar->AddButton(B_TRANSLATE("Forward"), 40,
			new BMessage(M_FORWARD));
		button->InvokeOnButton(B_SECONDARY_MOUSE_BUTTON);
		fPrintButton = bbar->AddButton(B_TRANSLATE("Print"), 16,
			new BMessage(M_PRINT));
		bbar->AddButton(B_TRANSLATE("Trash"), 0, new BMessage(M_DELETE_NEXT));
		if (fApp->ShowSpamGUI()) {
			button = bbar->AddButton("Spam", 48, new BMessage(M_SPAM_BUTTON));
			button->InvokeOnButton(B_SECONDARY_MOUSE_BUTTON);
		}
		bbar->AddDivider(5);
		fNextButton = bbar->AddButton(B_TRANSLATE("Next"), 24,
			new BMessage(M_NEXTMSG));
		bbar->AddButton(B_TRANSLATE("Previous"), 20, new BMessage(M_PREVMSG));
		if (!fAutoMarkRead)
			_AddReadButton();
	}

	bbar->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	bbar->Hide();
	AddChild(bbar);
}


void
TMailWindow::UpdateViews()
{
	float bbwidth = 0, bbheight = 0;
	float nextY = fMenuBar->Frame().bottom + 1;

	uint8 showButtonBar = fApp->ShowButtonBar();

	// Show/Hide Button Bar
	if (showButtonBar) {
		// Create the Button Bar if needed
		if (!fButtonBar)
			BuildButtonBar();

		fButtonBar->ShowLabels(showButtonBar == 1);
		fButtonBar->Arrange(true);
			// True for all buttons same size, false to just fit
		fButtonBar->GetPreferredSize(&bbwidth, &bbheight);
		fButtonBar->ResizeTo(Bounds().right, bbheight);
		fButtonBar->MoveTo(0, nextY);
		nextY += bbheight + 1;
		if (fButtonBar->IsHidden())
			fButtonBar->Show();
		else
			fButtonBar->Invalidate();
	} else if (fButtonBar && !fButtonBar->IsHidden())
		fButtonBar->Hide();

	// Arange other views to match
	fHeaderView->MoveTo(0, nextY);
	nextY = fHeaderView->Frame().bottom;
	if (fEnclosuresView) {
		fEnclosuresView->MoveTo(0, nextY);
		nextY = fEnclosuresView->Frame().bottom + 1;
	}
	BRect bounds(Bounds());
	fContentView->MoveTo(0, nextY - 1);
	fContentView->ResizeTo(bounds.right - bounds.left,
		bounds.bottom - nextY + 1);

	_UpdateSizeLimits();
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
TMailWindow::GetMailNodeRef(node_ref &nodeRef) const
{
	if (fRef == NULL)
		return B_ERROR;

	BNode node(fRef);
	return node.GetNodeRef(&nodeRef);
}


bool
TMailWindow::GetTrackerWindowFile(entry_ref *ref, bool next) const
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
TMailWindow::SaveTrackerPosition(entry_ref *ref)
{
	// if only one of them is saved, we're not going to do it again
	if (fNextTrackerPositionSaved || fPrevTrackerPositionSaved)
		return;

	fNextRef = fPrevRef = *ref;

	fNextTrackerPositionSaved = GetTrackerWindowFile(&fNextRef, true);
	fPrevTrackerPositionSaved = GetTrackerWindowFile(&fPrevRef, false);
}


void
TMailWindow::SetOriginatingWindow(BWindow *window)
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

	BMailDaemon::MarkAsRead(account, *message, flag);
}


void
TMailWindow::FrameResized(float width, float height)
{
	fContentView->FrameResized(width, height);
}


void
TMailWindow::MenusBeginning()
{
	bool enable;
	int32 finish = 0;
	int32 start = 0;
	BTextView *textView;

	if (!fIncoming) {
		bool gotToField = fHeaderView->fTo->Text()[0] != 0;
		bool gotCcField = fHeaderView->fCc->Text()[0] != 0;
		bool gotBccField = fHeaderView->fBcc->Text()[0] != 0;
		bool gotSubjectField = fHeaderView->fSubject->Text()[0] != 0;
		bool gotText = fContentView->fTextView->Text()[0] != 0;
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
			enable = strlen(fHeaderView->fTo->Text());
			fSendNow->SetEnabled(enable);
			// fSendLater->SetEnabled(enable);

			if (fHeaderView->fTo->HasFocus()) {
				textView = fHeaderView->fTo->TextView();
				textView->GetSelection(&start, &finish);

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

			if (!fTrackerMessenger.IsValid()) {
				fNextMsg->SetEnabled(false);
				fPrevMsg->SetEnabled(false);
			}
		}
	}

	fPrint->SetEnabled(fContentView->fTextView->TextLength());

	textView = dynamic_cast<BTextView *>(CurrentFocus());
	if (textView != NULL
		&& dynamic_cast<TTextControl *>(textView->Parent()) != NULL) {
		// one of To:, Subject:, Account:, Cc:, Bcc:
		textView->GetSelection(&start, &finish);
	} else if (fContentView->fTextView->IsFocus()) {
		fContentView->fTextView->GetSelection(&start, &finish);
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

	BTextView *focusTextView = dynamic_cast<BTextView *>(CurrentFocus());
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
TMailWindow::MessageReceived(BMessage *msg)
{
	bool wasReadMsg = false;
	switch (msg->what) {
		case kMsgBodyFetched:
		{
			status_t status = msg->FindInt32("status");
			if (status != B_OK) {
				PostMessage(B_QUIT_REQUESTED);
				break;
			}

			entry_ref ref;
			if (msg->FindRef("ref", &ref) != B_OK)
				break;
			if (ref != *fRef)
				break;

			// reload the current message
			OpenMessage(&ref, fHeaderView->fCharacterSetUserSees);
			break;
		}

		case FIELD_CHANGED:
		{
			int32 prevState = fFieldState;
			int32 fieldMask = msg->FindInt32("bitmask");
			void *source;

			if (msg->FindPointer("source", &source) == B_OK) {
				int32 length;

				if (fieldMask == FIELD_BODY)
					length = ((TTextView *)source)->TextLength();
				else
					length = ((BComboBox *)source)->TextView()->TextLength();

				if (length)
					fFieldState |= fieldMask;
				else
					fFieldState &= ~fieldMask;
			}

			// Has anything changed?
			if (prevState != fFieldState || !fChanged) {
				// Change Buttons to reflect this
				if (fSaveButton)
					fSaveButton->SetEnabled(fFieldState);
				if (fPrintButton)
					fPrintButton->SetEnabled(fFieldState);
				if (fSendButton)
					fSendButton->SetEnabled((fFieldState & FIELD_TO)
						|| (fFieldState & FIELD_BCC));
			}
			fChanged = true;

			// Update title bar if "subject" has changed
			if (!fIncoming && fieldMask & FIELD_SUBJECT) {
				// If no subject, set to "Mail"
				if (!fHeaderView->fSubject->TextView()->TextLength())
					SetTitle(B_TRANSLATE_SYSTEM_NAME("Mail"));
				else
					SetTitle(fHeaderView->fSubject->Text());
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
			if (msg->FindInt32("buttons", (int32 *)&buttons) == B_OK
				&& buttons == B_SECONDARY_MOUSE_BUTTON) {
				BPopUpMenu menu("Spam Actions", false, false);
				for (int i = 0; i < 4; i++)
					menu.AddItem(new BMenuItem(kSpamMenuItemTextArray[i],
						new BMessage(M_TRAIN_SPAM_AND_DELETE + i)));

				BPoint where;
				msg->FindPoint("where", &where);
				BMenuItem *item;
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
			if (msg->FindInt32("buttons", (int32 *)&buttons) == B_OK
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

				BMenuItem *item;
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
			if (msg->FindInt32("buttons", (int32 *)&buttons) == B_OK
				&& buttons == B_SECONDARY_MOUSE_BUTTON) {
				BPopUpMenu menu("Forward", false, false);
				menu.AddItem(new BMenuItem(B_TRANSLATE("Forward"),
					new BMessage(M_FORWARD)));
				menu.AddItem(new BMenuItem(
					B_TRANSLATE("Forward without attachments"),
					new BMessage(M_FORWARD_WITHOUT_ATTACHMENTS)));

				BPoint where;
				msg->FindPoint("where", &where);

				BMenuItem *item;
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
			if (msg->what == M_DELETE_NEXT && (modifiers() & B_SHIFT_KEY))
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
						(new BAlert("",
							B_TRANSLATE("Need Tracker to move items to trash"),
							B_TRANSLATE("Sorry")))->Go();
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
				TMailWindow *window
					= static_cast<TMailApp *>(be_app)->FindWindow(nextRef);
				if (window == NULL)
					OpenMessage(&nextRef, fHeaderView->fCharacterSetUserSees);
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
				const char *str;
				msg->FindString("status", (const char**) &str);
				BMessage message(B_CLOSE_REQUESTED);
				message.AddString("status", str);
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
			PostMessage(&message, fContentView->fTextView);
			break;
		}
		case M_RAW:
		{
			bool raw = !(fRaw->IsMarked());
			fRaw->SetMarked(raw);
			BMessage message(M_RAW);
			message.AddBool("raw", raw);
			PostMessage(&message, fContentView->fTextView);
			break;
		}
		case M_SEND_NOW:
		case M_SAVE_AS_DRAFT:
			Send(msg->what == M_SEND_NOW);
			break;

		case M_SAVE:
		{
			const char* address;
			if (msg->FindString("address", (const char**)&address) != B_NO_ERROR)
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
					(new BAlert("",	B_TRANSLATE(
						"Sorry, could not find an application that "
						"supports the 'Person' data type."),
						B_TRANSLATE("OK")))->Go();
				}

			}

			free(arg);
			break;
		}

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
				PostMessage((BMessage *)sigList.ItemAt(rand()
					% sigList.CountItems()));

				for (int32 i = 0; (message = (BMessage *)sigList.ItemAt(i))
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
			TMenu *menu;
			BMenuItem *item;
			menu = new TMenu("Add Signature", INDEX_SIGNATURE, M_SIGNATURE,
				true);

			BPoint	where;
			bool open_anyway = true;

			if (msg->FindPoint("where", &where) != B_OK) {
				BRect	bounds;
				bounds = fSigButton->Bounds();
				where = fSigButton->ConvertToScreen(BPoint(
					(bounds.right - bounds.left) / 2,
					(bounds.bottom - bounds.top) / 2));
			} else if (msg->FindInt32("buttons") == B_SECONDARY_MOUSE_BUTTON) {
				open_anyway = false;
			}

			if ((item = menu->Go(where, false, open_anyway)) != NULL) {
				item->SetTarget(this);
				(dynamic_cast<BInvoker *>(item))->Invoke();
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
			if (fIncoming && !fResending) {
				// The user wants to see the message they are reading (not
				// composing) displayed with a different kind of character set
				// for decoding.  Reload the whole message and redisplay.  For
				// messages which are being composed, the character set is
				// retrieved from the header view when it is needed.

				entry_ref fileRef = *fRef;
				int32 characterSet;
				msg->FindInt32("charset", &characterSet);
				OpenMessage(&fileRef, characterSet);
			}
			break;

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
				TMailWindow *window = static_cast<TMailApp *>(be_app)
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

					OpenMessage(&nextRef,
						fHeaderView->fCharacterSetUserSees);
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
			if (fHeaderView->fTo->TextView()->TextLength())
				fFieldState |= FIELD_TO;
			if (fHeaderView->fSubject->TextView()->TextLength())
				fFieldState |= FIELD_SUBJECT;
			if (fHeaderView->fCc->TextView()->TextLength())
				fFieldState |= FIELD_CC;
			if (fHeaderView->fBcc->TextView()->TextLength())
				fFieldState |= FIELD_BCC;
			if (fContentView->fTextView->TextLength())
				fFieldState |= FIELD_BODY;

			if (fSaveButton)
				fSaveButton->SetEnabled(false);
			if (fPrintButton)
				fPrintButton->SetEnabled(fFieldState);
			if (fSendButton)
				fSendButton->SetEnabled((fFieldState & FIELD_TO)
					|| (fFieldState & FIELD_BCC));
			break;

		case M_CHECK_SPELLING:
			if (gDictCount == 0)
				// Give the application time to init and load dictionaries.
				snooze (1500000);
			if (!gDictCount) {
				beep();
				(new BAlert("",
					B_TRANSLATE("Mail couldn't find its dictionary."),
					B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_AS_USUAL,
					B_OFFSET_SPACING, B_STOP_ALERT))->Go();
			} else {
				fSpelling->SetMarked(!fSpelling->IsMarked());
				fContentView->fTextView->EnableSpellCheck(
					fSpelling->IsMarked());
			}
			break;

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
TMailWindow::AddEnclosure(BMessage *msg)
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
		&& (strlen(fHeaderView->fTo->Text())
			|| strlen(fHeaderView->fSubject->Text())
			|| (fHeaderView->fCc && strlen(fHeaderView->fCc->Text()))
			|| (fHeaderView->fBcc && strlen(fHeaderView->fBcc->Text()))
			|| (strlen(fContentView->fTextView->Text()) && (!fStartingText
				|| (fStartingText
					&& strcmp(fContentView->fTextView->Text(), fStartingText))))
			|| (fEnclosuresView != NULL
				&& fEnclosuresView->fList->CountItems()))) {
		if (fResending) {
			BAlert *alert = new BAlert("", B_TRANSLATE(
					"Do you wish to send this message before closing?"),
				B_TRANSLATE("Discard"),
				B_TRANSLATE("Cancel"),
				B_TRANSLATE("Send"),
				B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
			alert->SetShortcut(0, 'd');
			alert->SetShortcut(1, B_ESCAPE);
			result = alert->Go();

			switch (result) {
				case 0:	// Discard
					break;
				case 1:	// Cancel
					return false;
				case 2:	// Send
					Send(true);
					break;
			}
		} else {
			BAlert *alert = new BAlert("",
				B_TRANSLATE("Do you wish to save this message as a draft "
					"before closing?"),
				B_TRANSLATE("Don't save"),
				B_TRANSLATE("Cancel"),
				B_TRANSLATE("Save"),
				B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
			alert->SetShortcut(0, 'd');
			alert->SetShortcut(1, B_ESCAPE);
			result = alert->Go();
			switch (result) {
				case 0:	// Don't Save
					break;
				case 1:	// Cancel
					return false;
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
			const char *status = CurrentMessage()->FindString("status");
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
			fContentView->fTextView->MakeFocus(true);
		} else {
			BTextView *textView = fHeaderView->fTo->TextView();
			fHeaderView->fTo->MakeFocus(true);
			textView->Select(0, textView->TextLength());
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
	BScreen		screen(this);
	BRect		r;
	BRect		s_frame = screen.Frame();

	r = Frame();
	width = 80 * fApp->ContentFont().StringWidth("M")
		+ (r.Width() - fContentView->fTextView->Bounds().Width() + 6);
	if (width > (s_frame.Width() - 8))
		width = s_frame.Width() - 8;

	height = max_c(fContentView->fTextView->CountLines(), 20)
		* fContentView->fTextView->LineHeight(0)
		+ (r.Height() - fContentView->fTextView->Bounds().Height());
	if (height > (s_frame.Height() - 29))
		height = s_frame.Height() - 29;

	r.right = r.left + width;
	r.bottom = r.top + height;

	if (abs((int)(Frame().Width() - r.Width())) < 5
		&& abs((int)(Frame().Height() - r.Height())) < 5) {
		r = fZoom;
	} else {
		fZoom = Frame();
		s_frame.InsetBy(6, 6);

		if (r.Width() > s_frame.Width())
			r.right = r.left + s_frame.Width();
		if (r.Height() > s_frame.Height())
			r.bottom = r.top + s_frame.Height();

		if (r.right > s_frame.right)
		{
			r.left -= r.right - s_frame.right;
			r.right = s_frame.right;
		}
		if (r.bottom > s_frame.bottom)
		{
			r.top -= r.bottom - s_frame.bottom;
			r.bottom = s_frame.bottom;
		}
		if (r.left < s_frame.left)
		{
			r.right += s_frame.left - r.left;
			r.left = s_frame.left;
		}
		if (r.top < s_frame.top)
		{
			r.bottom += s_frame.top - r.top;
			r.top = s_frame.top;
		}
	}

	ResizeTo(r.Width(), r.Height());
	MoveTo(r.LeftTop());
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
TMailWindow::Forward(entry_ref *ref, TMailWindow *window,
	bool includeAttachments)
{
	BEmailMessage *mail = window->Mail();
	if (mail == NULL)
		return;

	uint32 useAccountFrom = fApp->UseAccountFrom();

	fMail = mail->ForwardMessage(useAccountFrom == ACCOUNT_FROM_MAIL,
		includeAttachments);

	BFile file(ref, O_RDONLY);
	if (file.InitCheck() < B_NO_ERROR)
		return;

	fHeaderView->fSubject->SetText(fMail->Subject());

	// set mail account

	if (useAccountFrom == ACCOUNT_FROM_MAIL) {
		fHeaderView->fAccountID = fMail->Account();

		BMenu *menu = fHeaderView->fAccountMenu;
		for (int32 i = menu->CountItems(); i-- > 0;) {
			BMenuItem *item = menu->ItemAt(i);
			BMessage *msg;
			if (item && (msg = item->Message()) != NULL
				&& msg->FindInt32("id") == fHeaderView->fAccountID)
				item->SetMarked(true);
		}
	}

	if (fMail->CountComponents() > 1) {
		// if there are any enclosures to be added, first add the enclosures
		// view to the window
		AddEnclosure(NULL);
		if (fEnclosuresView)
			fEnclosuresView->AddEnclosuresFromMail(fMail);
	}

	fContentView->fTextView->LoadMessage(fMail, false, NULL);
	fChanged = false;
	fFieldState = 0;
}


class HorizontalLine : public BView {
	public:
		HorizontalLine(BRect rect)
			:
			BView (rect, NULL, B_FOLLOW_ALL, B_WILL_DRAW) {}

		virtual void Draw(BRect rect)
		{
			FillRect(rect, B_SOLID_HIGH);
		}
};


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
		#define add_header_field(field) { \
			/*header_view.SetFontAndColor(be_bold_font);*/ \
			header_view.Insert(fHeaderView->field->Label()); \
			header_view.Insert(" "); \
			/*header_view.SetFontAndColor(be_plain_font);*/ \
			header_view.Insert(fHeaderView->field->Text()); \
			header_view.Insert("\n"); \
		}

		add_header_field(fSubject);
		add_header_field(fTo);
		if ((fHeaderView->fCc != NULL)
			&& (strcmp(fHeaderView->fCc->Text(),"") != 0))
			add_header_field(fCc);

		if (fHeaderView->fDate != NULL)
			header_view.Insert(fHeaderView->fDate->Text());

		int32 maxLine = fContentView->fTextView->CountLines();
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
			int32 lineOffset = fContentView->fTextView->OffsetAt(lastLine);
			curPageRect.OffsetTo(0,
				fContentView->fTextView->PointAt(lineOffset).y);

			int32 fromLine = lastLine;
			lastLine = fContentView->fTextView->LineAt(
				BPoint(0.0, curPageRect.bottom - ((curPage == 1)
					? header_height : 0)));

			float curPageHeight = fContentView->fTextView->
				TextHeight(fromLine, lastLine) + ((curPage == 1)
					? header_height : 0);

			if (curPageHeight > pageRect.Height()) {
				curPageHeight = fContentView->fTextView->TextHeight(
					fromLine, --lastLine) + ((curPage == 1)
						? header_height : 0);
			}
			curPageRect.bottom = curPageRect.top + curPageHeight - 1.0;

			if ((curPage >= print.FirstPage())
				&& (curPage <= print.LastPage())) {
				print.DrawView(fContentView->fTextView, curPageRect,
					BPoint(0.0, (curPage == 1) ? header_height : 0.0));
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
TMailWindow::SetTo(const char *mailTo, const char *subject, const char *ccTo,
	const char *bccTo, const BString *body, BMessage *enclosures)
{
	Lock();

	if (mailTo && mailTo[0])
		fHeaderView->fTo->SetText(mailTo);
	if (subject && subject[0])
		fHeaderView->fSubject->SetText(subject);
	if (ccTo && ccTo[0])
		fHeaderView->fCc->SetText(ccTo);
	if (bccTo && bccTo[0])
		fHeaderView->fBcc->SetText(bccTo);

	if (body && body->Length()) {
		fContentView->fTextView->SetText(body->String(), body->Length());
		fContentView->fTextView->GoToLine(0);
	}

	if (enclosures && enclosures->HasRef("refs"))
		AddEnclosure(enclosures);

	Unlock();
}


void
TMailWindow::CopyMessage(entry_ref *ref, TMailWindow *src)
{
	BNode file(ref);
	if (file.InitCheck() == B_OK) {
		BString string;
		if (fHeaderView->fTo
			&& file.ReadAttrString(B_MAIL_ATTR_TO, &string) == B_OK)
			fHeaderView->fTo->SetText(string.String());

		if (fHeaderView->fSubject
			&& file.ReadAttrString(B_MAIL_ATTR_SUBJECT, &string) == B_OK)
			fHeaderView->fSubject->SetText(string.String());

		if (fHeaderView->fCc
			&& file.ReadAttrString(B_MAIL_ATTR_CC, &string) == B_OK)
			fHeaderView->fCc->SetText(string.String());
	}

	TTextView *text = src->fContentView->fTextView;
	text_run_array *style = text->RunArray(0, text->TextLength());

	fContentView->fTextView->SetText(text->Text(), text->TextLength(), style);

	free(style);
}


void
TMailWindow::Reply(entry_ref *ref, TMailWindow *window, uint32 type)
{
	fRepliedMail = *ref;
	SetOriginatingWindow(window);

	BEmailMessage *mail = window->Mail();
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
	fHeaderView->fTo->SetText(fMail->To());
	fHeaderView->fCc->SetText(fMail->CC());
	fHeaderView->fSubject->SetText(fMail->Subject());

	int32 accountID;
	BFile file(window->fRef, B_READ_ONLY);
	if (file.ReadAttr("MAIL:reply_with", B_INT32_TYPE, 0, &accountID,
		sizeof(int32)) != B_OK)
		accountID = -1;

	// set mail account

	if ((useAccountFrom == ACCOUNT_FROM_MAIL) || (accountID > -1)) {
		if (useAccountFrom == ACCOUNT_FROM_MAIL)
			fHeaderView->fAccountID = fMail->Account();
		else
			fHeaderView->fAccountID = accountID;

		BMenu *menu = fHeaderView->fAccountMenu;
		for (int32 i = menu->CountItems(); i-- > 0;) {
			BMenuItem *item = menu->ItemAt(i);
			BMessage *msg;
			if (item && (msg = item->Message()) != NULL
				&& msg->FindInt32("id") == fHeaderView->fAccountID)
				item->SetMarked(true);
		}
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

	BString date(mail->Date());
	if (date.Length() <= 0)
		date = B_TRANSLATE("(Date unavailable)");
			
	preamble.ReplaceAll("%n", name);
	preamble.ReplaceAll("%e", address);
	preamble.ReplaceAll("%d", date);
	preamble.ReplaceAll("\\n", "\n");

	// insert (if selection) or load (if whole mail) message text into text view

	int32 finish, start;
	window->fContentView->fTextView->GetSelection(&start, &finish);
	if (start != finish) {
		char *text = (char *)malloc(finish - start + 1);
		if (text == NULL)
			return;

		window->fContentView->fTextView->GetText(start, finish - start, text);
		if (text[strlen(text) - 1] != '\n') {
			text[strlen(text)] = '\n';
			finish++;
		}
		fContentView->fTextView->SetText(text, finish - start);
		free(text);

		finish = fContentView->fTextView->CountLines();
		for (int32 loop = 0; loop < finish; loop++) {
			fContentView->fTextView->GoToLine(loop);
			fContentView->fTextView->Insert((const char *)QUOTE);
		}

		if (fApp->ColoredQuotes()) {
			const BFont *font = fContentView->fTextView->Font();
			int32 length = fContentView->fTextView->TextLength();

			TextRunArray style(length / 8 + 8);

			FillInQuoteTextRuns(fContentView->fTextView, NULL,
				fContentView->fTextView->Text(), length, font, &style.Array(),
				style.MaxEntries());

			fContentView->fTextView->SetRunArray(0, length, &style.Array());
		}

		fContentView->fTextView->GoToLine(0);
		if (preamble.Length() > 0)
			fContentView->fTextView->Insert(preamble);
	} else {
		fContentView->fTextView->LoadMessage(mail, true, preamble);
	}

	fReplying = true;
}


status_t
TMailWindow::Send(bool now)
{
	uint32 characterSetToUse = fApp->MailCharacterSet();
	mail_encoding encodingForBody = quoted_printable;
	mail_encoding encodingForHeaders = quoted_printable;

	if (!now) {
		status_t status = SaveAsDraft();
		if (status != B_OK) {
			beep();
			(new BAlert("", B_TRANSLATE("E-mail draft could not be saved!"),
				B_TRANSLATE("OK")))->Go();
		}
		return status;
	}

	if (fHeaderView != NULL)
		characterSetToUse = fHeaderView->fCharacterSetUserSees;

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
	if (fContentView->fTextView->TextLength() > 0) {
		// First do a trial encoding with the user's character set.
		int32 converterState = 0;
		int32 originalLength;
		BString tempString;
		int32 tempStringLength;
		char* tempStringPntr;
		originalLength = fContentView->fTextView->TextLength();
		tempStringLength = originalLength * 6;
			// Some character sets bloat up on escape codes
		tempStringPntr = tempString.LockBuffer (tempStringLength);
		if (tempStringPntr != NULL && mail_convert_from_utf8(characterSetToUse,
				fContentView->fTextView->Text(), &originalLength,
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
					userAnswer = (new BAlert("Question", messageString.String(),
						B_TRANSLATE("Send"),
						B_TRANSLATE("Cancel"),
						NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING,
						B_WARNING_ALERT))->Go();
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
			mail.SetTo(fHeaderView->fTo->Text(), characterSetToUse,
				encodingForHeaders);

			if (fHeaderView->fAccountID != ~0L)
				mail.SendViaAccount(fHeaderView->fAccountID);

			result = mail.Send(now);
		}
	} else {
		if (fMail == NULL)
			// the mail will be deleted when the window is closed
			fMail = new BEmailMessage;

		// Had an embarrassing bug where replying to a message and clearing the
		// CC field meant that it got sent out anyway, so pass in empty strings
		// when changing the header to force it to remove the header.

		fMail->SetTo(fHeaderView->fTo->Text(), characterSetToUse,
			encodingForHeaders);
		fMail->SetSubject(fHeaderView->fSubject->Text(), characterSetToUse,
			encodingForHeaders);
		fMail->SetCC(fHeaderView->fCc->Text(), characterSetToUse,
			encodingForHeaders);
		fMail->SetBCC(fHeaderView->fBcc->Text());

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
				"Mail/Haiku %ld.%ld.%ld",
				info.major, info.middle, info.minor);
			fMail->SetHeaderField("X-Mailer", versionString);
		}

		/****/

		// the content text is always added to make sure there is a mail body
		fMail->SetBodyTextTo("");
		fContentView->fTextView->AddAsContent(fMail, fApp->WrapMode(),
			characterSetToUse, encodingForBody);

		if (fEnclosuresView != NULL) {
			TListItem *item;
			int32 index = 0;
			while ((item = (TListItem *)fEnclosuresView->fList->ItemAt(index++))
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
		if (fHeaderView->fAccountID != ~0L)
			fMail->SendViaAccount(fHeaderView->fAccountID);

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

			int32 start = (new BAlert("no daemon",
				B_TRANSLATE("The mail_daemon is not running. The message is "
				"queued and will be sent when the mail_daemon is started."),
				B_TRANSLATE("Start now"), B_TRANSLATE("OK")))->Go();

			if (start == 0) {
				result = be_roster->Launch("application/x-vnd.Be-POST");
				if (result == B_OK) {
					BMailDaemon::SendQueuedMail();
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
		(new BAlert("", errorMessage.String(), B_TRANSLATE("OK")))->Go();
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
	status_t	status;
	BPath		draftPath;
	BDirectory	dir;
	BFile		draft;
	uint32		flags = 0;

	if (fDraft) {
		if ((status = draft.SetTo(fRef,
			B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE)) != B_OK) {
			return status;
		}
	} else {
		// Get the user home directory
		if ((status = find_directory(B_USER_DIRECTORY, &draftPath)) != B_OK)
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
				char fileName[512], *eofn;
				int32 i;

				// save as some version of the message's subject
				strncpy(fileName, fHeaderView->fSubject->Text(),
					sizeof(fileName)-10);
				fileName[sizeof(fileName)-10]='\0';
					// terminate like strncpy doesn't
				eofn = fileName + strlen(fileName);

				// convert /, \ and : to -
				for (char *bad = fileName; (bad = strchr(bad, '/')) != NULL;
					++bad) *bad = '-';
				for (char *bad = fileName; (bad = strchr(bad, '\\')) != NULL;
					++bad) *bad = '-';
				for (char *bad = fileName; (bad = strchr(bad, ':')) != NULL;
					++bad) *bad = '-';

				// Create the file; if the name exists, find a unique name
				flags = B_WRITE_ONLY | B_CREATE_FILE | B_FAIL_IF_EXISTS;
				for (i = 1; (status = draft.SetTo(&dir, fileName, flags))
					!= B_OK; i++) {
					if (status != B_FILE_EXISTS)
						return status;
					sprintf(eofn, "%ld", i);
				}

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
	draft.Write(fContentView->fTextView->Text(),
		fContentView->fTextView->TextLength());

	//
	// Add the header stuff as attributes
	//
	WriteAttrString(&draft, B_MAIL_ATTR_NAME, fHeaderView->fTo->Text());
	WriteAttrString(&draft, B_MAIL_ATTR_TO, fHeaderView->fTo->Text());
	WriteAttrString(&draft, B_MAIL_ATTR_SUBJECT, fHeaderView->fSubject->Text());
	if (fHeaderView->fCc != NULL)
		WriteAttrString(&draft, B_MAIL_ATTR_CC, fHeaderView->fCc->Text());
	if (fHeaderView->fBcc != NULL)
		WriteAttrString(&draft, B_MAIL_ATTR_BCC, fHeaderView->fBcc->Text());
	
	// Add account
	BMenuItem* menuItem = fHeaderView->fAccountMenu->FindMarked();
	if (menuItem != NULL)
		WriteAttrString(&draft, B_MAIL_ATTR_ACCOUNT, menuItem->Label());
	
	// Add encoding
	menuItem = fHeaderView->fEncodingMenu->FindMarked();
	if (menuItem != NULL)
		WriteAttrString(&draft, "MAIL:encoding", menuItem->Label());

	// Add the draft attribute for indexing
	uint32 draftAttr = true;
	draft.WriteAttr("MAIL:draft", B_INT32_TYPE, 0, &draftAttr, sizeof(uint32));

	// Add Attachment paths in attribute
	if (fEnclosuresView != NULL) {
		TListItem *item;
		BPath path;
		BString pathStr;

		for (int32 i = 0; (item = (TListItem *)
			fEnclosuresView->fList->ItemAt(i)) != NULL; i++) {
			if (i > 0)
				pathStr.Append(":");

			BEntry entry(item->Ref(), true);
			if (!entry.Exists())
				continue;

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

	return B_OK;
}


status_t
TMailWindow::TrainMessageAs(const char *CommandWord)
{
	status_t	errorCode = -1;
	char		errorString[1500];
	BEntry		fileEntry;
	BPath		filePath;
	BMessage	replyMessage;
	BMessage	scriptingMessage;
	team_id		serverTeam;

	if (fRef == NULL)
		goto ErrorExit; // Need to have a real file and name.
	errorCode = fileEntry.SetTo(fRef, true /* traverse */);
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
				directory_which places[]
					= {B_COMMON_BIN_DIRECTORY, B_BEOS_BIN_DIRECTORY};
				for (int32 i = 0; i < 2; i++) {
					find_directory(places[i],&path);
					path.Append("spamdbm");
					if (!BEntry(path.Path()).Exists())
						continue;
					get_ref_for_path(path.Path(),&ref);
					if ((errorCode =  be_roster->Launch (&ref)) == B_OK)
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
	scriptingMessage.AddSpecifier(CommandWord);
	errorCode = scriptingMessage.AddData("data", B_STRING_TYPE,
		filePath.Path(), strlen(filePath.Path()) + 1, false /* fixed size */);
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
	sprintf(errorString, "Unable to train the message file \"%s\" as %s.  "
		"Possibly useful error code: %s (%ld).",
		filePath.Path(), CommandWord, strerror (errorCode), errorCode);
	(new BAlert("", errorString,
		B_TRANSLATE("OK")))->Go();
	return errorCode;
}


void
TMailWindow::SetTitleForMessage()
{
	//
	//	Figure out the title of this message and set the title bar
	//
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
			BNode	node (fRef);
			char	numberString [30];
			BString oldTitle (title);
			float	spamRatio;
			if (node.InitCheck() != B_OK || node.ReadAttrString
				("MAIL:classification", &classification) != B_OK)
				classification = "Unrated";
			if (classification != "Spam" && classification != "Genuine") {
				// Uncertain, Unrated and other unknown classes, show the ratio.
				if (node.InitCheck() == B_OK && sizeof (spamRatio) ==
					node.ReadAttr("MAIL:ratio_spam", B_FLOAT_TYPE, 0,
					&spamRatio, sizeof (spamRatio))) {
					sprintf (numberString, "%.4f", spamRatio);
					classification << " " << numberString;
				}
			}
			title = "";
			title << "[" << classification << "] " << oldTitle;
		}
	}
	SetTitle(title);
}


//
//	Open *another* message in the existing mail window.  Some code here is
//	duplicated from various constructors.
//	The duplicated code should be in a private initializer method -- axeld.
//

status_t
TMailWindow::OpenMessage(const entry_ref *ref, uint32 characterSetForDecoding)
{
	if (ref == NULL)
		return B_ERROR;
	//
	//	Set some references to the email file
	//
	delete fRef;
	fRef = new entry_ref(*ref);

	if (fStartingText) {
		free(fStartingText);
		fStartingText = NULL;
	}
	fPrevTrackerPositionSaved = false;
	fNextTrackerPositionSaved = false;

	fContentView->fTextView->StopLoad();
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
		BMailDaemon::FetchBody(*ref, &listener);
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
		fContentView->fTextView->SetText(&file, 0, size);

		// Restore Fields from attributes
		if (node.ReadAttrString(B_MAIL_ATTR_TO, &string) == B_OK)
			fHeaderView->fTo->SetText(string.String());
		if (node.ReadAttrString(B_MAIL_ATTR_SUBJECT, &string) == B_OK)
			fHeaderView->fSubject->SetText(string.String());
		if (node.ReadAttrString(B_MAIL_ATTR_CC, &string) == B_OK)
			fHeaderView->fCc->SetText(string.String());
		if (node.ReadAttrString(B_MAIL_ATTR_BCC, &string) == B_OK)
			fHeaderView->fBcc->SetText(string.String());
		
		// Restore account
		if (node.ReadAttrString(B_MAIL_ATTR_ACCOUNT, &string) == B_OK) {
			BMenuItem* accountItem = fHeaderView->fAccountMenu->FindItem(string.String());
			if (accountItem != NULL)
				accountItem->SetMarked(true);
		}
		
		// Restore encoding
		if (node.ReadAttrString("MAIL:encoding", &string) == B_OK) {
			BMenuItem* encodingItem = fHeaderView->fEncodingMenu->FindItem(string.String());
			if (encodingItem != NULL)
				encodingItem->SetMarked(true);
		}
		
		// Restore attachments
		if (node.ReadAttrString("MAIL:attachments", &string) == B_OK) {
			BMessage msg(REFS_RECEIVED);
			entry_ref enc_ref;

			char *s = strtok((char *)string.String(), ":");
			while (s) {
				BEntry entry(s, true);
				if (entry.Exists()) {
					entry.GetRef(&enc_ref);
					msg.AddRef("refs", &enc_ref);
				}
				s = strtok(NULL, ":");
			}
			AddEnclosure(&msg);
		}
		PostMessage(RESET_BUTTONS);
		fIncoming = false;
		fDraft = true;
	} else {
		// A real mail message, parse its headers to get from, to, etc.
		fMail = new BEmailMessage(fRef, characterSetForDecoding);
		fIncoming = true;
		fHeaderView->LoadMessage(fMail);
	}

	err = fMail->InitCheck();
	if (err < B_OK) {
		delete fMail;
		fMail = NULL;
		return err;
	}

	SetTitleForMessage();

	if (fIncoming) {
		//
		//	Put the addresses in the 'Save Address' Menu
		//
		BMenuItem *item;
		while ((item = fSaveAddrMenu->RemoveItem(0L)) != NULL)
			delete item;

		// create the list of addresses

		BList addressList;
		get_address_list(addressList, fMail->To(), extract_address);
		get_address_list(addressList, fMail->CC(), extract_address);
		get_address_list(addressList, fMail->From(), extract_address);
		get_address_list(addressList, fMail->ReplyTo(), extract_address);

		BMessage *msg;

		for (int32 i = addressList.CountItems(); i-- > 0;) {
			char *address = (char *)addressList.RemoveItem(0L);

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

		//
		// Clear out existing contents of text view.
		//
		fContentView->fTextView->SetText("", (int32)0);

		fContentView->fTextView->LoadMessage(fMail, false, NULL);

		if (fApp->ShowButtonBar())
			_UpdateReadButton();
	}

	return B_OK;
}


TMailWindow *
TMailWindow::FrontmostWindow()
{
	BAutolock locker(sWindowListLock);
	if (sWindowList.CountItems() > 0)
		return (TMailWindow *)sWindowList.ItemAt(0);

	return NULL;
}


/*
// Copied from src/kits/tracker/FindPanel.cpp.
uint32
TMailWindow::InitialMode(const BNode *node)
{
	if (!node || node->InitCheck() != B_OK)
		return kByNameItem;

	uint32 result;
	if (node->ReadAttr(kAttrQueryInitialMode, B_INT32_TYPE, 0,
		(int32 *)&result, sizeof(int32)) <= 0)
		return kByNameItem;

	return result;
}


// Copied from src/kits/tracker/FindPanel.cpp.
int32
TMailWindow::InitialAttrCount(const BNode *node)
{
	if (!node || node->InitCheck() != B_OK)
		return 1;

	int32 result;
	if (node->ReadAttr(kAttrQueryInitialNumAttrs, B_INT32_TYPE, 0,
		&result, sizeof(int32)) <= 0)
		return 1;

	return result;
}*/


// #pragma mark -


void
TMailWindow::_UpdateSizeLimits()
{
	float minWidth, maxWidth, minHeight, maxHeight;
	GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);

	float height;
	fMenuBar->GetPreferredSize(&minWidth, &height);

	minHeight = height;

	if (fButtonBar) {
		fButtonBar->GetPreferredSize(&minWidth, &height);
		minHeight += height;
	} else {
		minWidth = WIND_WIDTH;
	}

	minHeight += fHeaderView->Bounds().Height() + ENCLOSURES_HEIGHT + 60;

	SetSizeLimits(minWidth, RIGHT_BOUNDARY, minHeight, RIGHT_BOUNDARY);
}


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

	fQueryMenu->AddItem(new BMenuItem(B_TRANSLATE("Edit queries"
			B_UTF8_ELLIPSIS),
		new BMessage(M_EDIT_QUERIES), 'E', B_SHIFT_KEY));

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

	if (queryItemsAdded)
		fQueryMenu->AddItem(new BSeparatorItem(), 1);
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
					(int32 *)&count, sizeof(int32)) <= 0) {
				count = 1;
			}

			attr_info info;
			if (node.GetAttrInfo(kAttrQueryInitialAttrs, &info) != B_OK)
				break;

			if (count > 1)
				queryString << "(";

			char *buffer = new char[info.size];
			if (node.ReadAttr(kAttrQueryInitialAttrs, B_MESSAGE_TYPE, 0,
					buffer, (size_t)info.size) == info.size) {
				BMessage message;
				if (message.Unflatten(buffer) == B_OK) {
					for (int32 index = 0; /*index < count*/; index++) {
						const char *field;
						const char *value;
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
TMailWindow::_AddReadButton()
{
	BNode node(fRef);

	read_flags flag = B_UNREAD;
	read_read_attr(node, flag);

	int32 buttonIndex = fButtonBar->IndexOf(fNextButton);
	if (flag == B_READ) {
		fReadButton = fButtonBar->AddButton(B_TRANSLATE("Unread"), 28,
			new BMessage(M_UNREAD), buttonIndex);
	} else {
		fReadButton = fButtonBar->AddButton(B_TRANSLATE(" Read "), 24,
			new BMessage(M_READ), buttonIndex);
	}
}


void
TMailWindow::_UpdateReadButton()
{
	if (fApp->ShowButtonBar()) {
		fButtonBar->RemoveButton(fReadButton);
		fReadButton = NULL;
		if (!fAutoMarkRead && fIncoming)
			_AddReadButton();
	}
	UpdateViews();
}


void
TMailWindow::_SetDownloading(bool downloading)
{
	fDownloading = downloading;
}
