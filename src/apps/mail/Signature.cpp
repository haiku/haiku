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


#include "Signature.h"

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include <Clipboard.h>
#include <Directory.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <ScrollView.h>
#include <StringView.h>

#include "MailApp.h"
#include "MailPopUpMenu.h"
#include "MailSupport.h"
#include "MailWindow.h"
#include "Messages.h"


#define B_TRANSLATION_CONTEXT "Mail"


const float kSigHeight = 250;
const float kSigWidth = 300;

extern const char* kUndoStrings[];
extern const char* kRedoStrings[];


TSignatureWindow::TSignatureWindow(BRect rect)
	:
	BWindow(rect, B_TRANSLATE("Signatures"), B_TITLED_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS),
	fFile(NULL)
{
	BMenuItem* item;

	// Set up the menu
	BMenuBar* menuBar = new BMenuBar("MenuBar");
	BMenu* menu = new BMenu(B_TRANSLATE("Signature"));
	menu->AddItem(fNew = new BMenuItem(B_TRANSLATE("New"),
		new BMessage(M_NEW), 'N'));
	fSignature = new TMenu(B_TRANSLATE("Open"), INDEX_SIGNATURE, M_SIGNATURE);
	menu->AddItem(new BMenuItem(fSignature));
	menu->AddSeparatorItem();
	menu->AddItem(fSave = new BMenuItem(B_TRANSLATE("Save"),
		new BMessage(M_SAVE), 'S'));
	menu->AddItem(fDelete = new BMenuItem(B_TRANSLATE("Delete"),
		new BMessage(M_DELETE), 'T'));
	menuBar->AddItem(menu);

	menu = new BMenu(B_TRANSLATE("Edit"));
	menu->AddItem(fUndo = new BMenuItem(B_TRANSLATE("Undo"),
		new BMessage(B_UNDO), 'Z'));
	fUndo->SetTarget(NULL, this);
	menu->AddSeparatorItem();
	menu->AddItem(fCut = new BMenuItem(B_TRANSLATE("Cut"),
		new BMessage(B_CUT), 'X'));
	fCut->SetTarget(NULL, this);
	menu->AddItem(fCopy = new BMenuItem(B_TRANSLATE("Copy"),
		new BMessage(B_COPY), 'C'));
	fCopy->SetTarget(NULL, this);
	menu->AddItem(fPaste = new BMenuItem(B_TRANSLATE("Paste"),
		new BMessage(B_PASTE), 'V'));
	fPaste->SetTarget(NULL, this);
	menu->AddSeparatorItem();
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Select all"),
		new BMessage(M_SELECT), 'A'));
	item->SetTarget(NULL, this);
	menuBar->AddItem(menu);

	fSigView = new TSignatureView();

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(menuBar)
		.Add(fSigView);

	if (!rect.IsValid()) {
		float fontFactor = be_plain_font->Size() / 12.0f;
		ResizeTo(kSigWidth * fontFactor, kSigHeight * fontFactor);
		// TODO: this should work, too, but doesn't
		//ResizeToPreferred();
	}
}


TSignatureWindow::~TSignatureWindow()
{
}


void
TSignatureWindow::MenusBeginning()
{
	fDelete->SetEnabled(fFile);
	fSave->SetEnabled(IsDirty());
	fUndo->SetEnabled(false);		// ***TODO***

	BTextView* textView = fSigView->fName->TextView();
	int32 finish = 0;
	int32 start = 0;
	if (textView->IsFocus())
		textView->GetSelection(&start, &finish);
	else
		fSigView->fTextView->GetSelection(&start, &finish);

	fCut->SetEnabled(start != finish);
	fCopy->SetEnabled(start != finish);

	fNew->SetEnabled(textView->TextLength()
		| fSigView->fTextView->TextLength());
	be_clipboard->Lock();
	fPaste->SetEnabled(be_clipboard->Data()->HasData("text/plain",
		B_MIME_TYPE));
	be_clipboard->Unlock();

	// Undo stuff
	bool isRedo = false;
	undo_state undoState = B_UNDO_UNAVAILABLE;

	BTextView *focusTextView = dynamic_cast<BTextView *>(CurrentFocus());
	if (focusTextView != NULL)
		undoState = focusTextView->UndoState(&isRedo);

	fUndo->SetLabel(isRedo ? kRedoStrings[undoState] : kUndoStrings[undoState]);
	fUndo->SetEnabled(undoState != B_UNDO_UNAVAILABLE);
}


void
TSignatureWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case CHANGE_FONT:
		{
			BFont* font;
			msg->FindPointer("font", (void **)&font);
			fSigView->fTextView->SetFontAndColor(font);
			fSigView->fTextView->Invalidate(fSigView->fTextView->Bounds());
			break;
		}

		case M_NEW:
			if (Clear()) {
				fSigView->fName->SetText("");
				fSigView->fTextView->SetText("");
				fSigView->fName->MakeFocus(true);
			}
			break;

		case M_SAVE:
			Save();
			break;

		case M_DELETE: {
			BAlert* alert = new BAlert("",
					B_TRANSLATE("Really delete this signature? This cannot "
						"be undone."),
					B_TRANSLATE("Cancel"),
					B_TRANSLATE("Delete"),
					NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
			alert->SetShortcut(0, B_ESCAPE);
			int32 choice = alert->Go();

			if (choice == 0)
				break;

			if (fFile) {
				delete fFile;
				fFile = NULL;
				fEntry.Remove();
				fSigView->fName->SetText("");
				fSigView->fTextView->SetText(NULL, (int32)0);
				fSigView->fName->MakeFocus(true);
			}
			break;
		}
		case M_SIGNATURE:
			if (Clear()) {
				entry_ref ref;
				msg->FindRef("ref", &ref);
				fEntry.SetTo(&ref);
				fFile = new BFile(&ref, O_RDWR);
				if (fFile->InitCheck() == B_OK) {
					char name[B_FILE_NAME_LENGTH];
					fFile->ReadAttr(INDEX_SIGNATURE, B_STRING_TYPE, 0, name,
						sizeof(name));
					fSigView->fName->SetText(name);

					off_t size;
					fFile->GetSize(&size);
					char* sig = (char*)malloc(size);
					if (sig == NULL)
						break;

					size = fFile->Read(sig, size);
					fSigView->fTextView->SetText(sig, size);
					fSigView->fName->MakeFocus(true);
					BTextView* textView = fSigView->fName->TextView();
					textView->Select(0, textView->TextLength());
					fSigView->fTextView->fDirty = false;
				} else {
					fFile = NULL;
					beep();
					BAlert* alert = new BAlert("",
						B_TRANSLATE("Couldn't open this signature. Sorry."),
						B_TRANSLATE("OK"));
					alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
					alert->Go();
				}
			}
			break;

		default:
			BWindow::MessageReceived(msg);
			break;
	}
}


bool
TSignatureWindow::QuitRequested()
{
	if (Clear()) {
		BMessage msg(WINDOW_CLOSED);
		msg.AddInt32("kind", SIG_WINDOW);
		msg.AddRect("window frame", Frame());

		be_app->PostMessage(&msg);
		return true;
	}
	return false;
}


void
TSignatureWindow::FrameResized(float width, float height)
{
	fSigView->FrameResized(width, height);
}


void
TSignatureWindow::Show()
{
	Lock();
	BTextView* textView = (BTextView *)fSigView->fName->TextView();
	fSigView->fName->MakeFocus(true);
	textView->Select(0, textView->TextLength());
	Unlock();

	BWindow::Show();
}


bool
TSignatureWindow::Clear()
{
	if (IsDirty()) {
		beep();
		BAlert *alert = new BAlert("",
			B_TRANSLATE("Save changes to this signature?"),
			B_TRANSLATE("Cancel"),
			B_TRANSLATE("Don't save"),
			B_TRANSLATE("Save"),
			B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
		alert->SetShortcut(0, B_ESCAPE);
		alert->SetShortcut(1, 'd');
		alert->SetShortcut(2, 's');
		int32 result = alert->Go();
		if (result == 0)
			return false;
		if (result == 2)
			Save();
	}

	delete fFile;
	fFile = NULL;
	fSigView->fTextView->fDirty = false;
	return true;
}


bool
TSignatureWindow::IsDirty()
{
	if (fFile != NULL) {
		char name[B_FILE_NAME_LENGTH];
		fFile->ReadAttr(INDEX_SIGNATURE, B_STRING_TYPE, 0, name, sizeof(name));
		if (strcmp(name, fSigView->fName->Text()) != 0
			|| fSigView->fTextView->fDirty) {
			return true;
		}
	} else if (fSigView->fName->Text()[0] != '\0'
		|| fSigView->fTextView->TextLength() != 0) {
		return true;
	}
	return false;
}


void
TSignatureWindow::Save()
{
	char			name[B_FILE_NAME_LENGTH];
	int32			index = 0;
	status_t		result;
	BDirectory		dir;
	BEntry			entry;
	BNodeInfo		*node;
	BPath			path;

	if (!fFile) {
		find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);
		dir.SetTo(path.Path());

		if (dir.FindEntry("Mail", &entry) == B_NO_ERROR)
			dir.SetTo(&entry);
		else
			dir.CreateDirectory("Mail", &dir);

		if (dir.InitCheck() != B_NO_ERROR)
			goto err_exit;

		if (dir.FindEntry("signatures", &entry) == B_NO_ERROR)
			dir.SetTo(&entry);
		else
			dir.CreateDirectory("signatures", &dir);

		if (dir.InitCheck() != B_NO_ERROR)
			goto err_exit;

		fFile = new BFile();
		while(true) {
			sprintf(name, "signature_%" B_PRId32, index++);
			if ((result = dir.CreateFile(name, fFile, true)) == B_NO_ERROR)
				break;
			if (result != EEXIST)
				goto err_exit;
		}
		dir.FindEntry(name, &fEntry);
		node = new BNodeInfo(fFile);
		node->SetType("text/plain");
		delete node;
	}

	fSigView->fTextView->fDirty = false;
	fFile->Seek(0, 0);
	fFile->Write(fSigView->fTextView->Text(),
				 fSigView->fTextView->TextLength());
	fFile->SetSize(fFile->Position());
	fFile->WriteAttr(INDEX_SIGNATURE, B_STRING_TYPE, 0, fSigView->fName->Text(),
					 strlen(fSigView->fName->Text()) + 1);
	return;

err_exit:
	beep();
	BAlert* alert = new BAlert("",
		B_TRANSLATE("An error occurred trying to save this signature."),
		B_TRANSLATE("Sorry"));
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go();
}


// #pragma mark -


TSignatureView::TSignatureView()
	:
	BGridView("SigView")
{
	GridLayout()->SetInsets(B_USE_DEFAULT_SPACING);

	BStringView* nameLabel = new BStringView("NameLabel",
		B_TRANSLATE("Title:"));
	nameLabel->SetAlignment(B_ALIGN_RIGHT);
	GridLayout()->AddView(nameLabel, 0, 0);

	fName = new TNameControl("", new BMessage(NAME_FIELD));
	GridLayout()->AddItem(fName->CreateTextViewLayoutItem(), 1, 0);

	BStringView* signatureLabel = new BStringView("SigLabel",
		B_TRANSLATE("Signature:"));
	signatureLabel->SetAlignment(B_ALIGN_RIGHT);
	GridLayout()->AddView(signatureLabel, 0, 1);

	fTextView = new TSigTextView();

	font_height fontHeight;
	fTextView->GetFontHeight(&fontHeight);
	float lineHeight = ceilf(fontHeight.ascent) + ceilf(fontHeight.descent);

	BScrollView* scroller = new BScrollView("SigScroller", fTextView, 0,
		false, true);
	scroller->SetExplicitPreferredSize(
		BSize(fTextView->StringWidth("W") * 30, lineHeight * 6));

	GridLayout()->AddView(scroller, 1, 1, 1, 2);

	GridLayout()->AddItem(BSpaceLayoutItem::CreateGlue(), 0, 2);
}


void
TSignatureView::AttachedToWindow()
{
}


// #pragma mark -


TNameControl::TNameControl(const char* label, BMessage* invocationMessage)
	:
	BTextControl("", label, "", invocationMessage)
{
	strcpy(fLabel, label);
}


void
TNameControl::AttachedToWindow()
{
	BTextControl::AttachedToWindow();

	TextView()->SetMaxBytes(B_FILE_NAME_LENGTH - 1);
}


void
TNameControl::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case M_SELECT:
			TextView()->Select(0, TextView()->TextLength());
			break;

		default:
			BTextControl::MessageReceived(msg);
	}
}


// #pragma mark -


TSigTextView::TSigTextView()
	:
	BTextView("SignatureView", B_NAVIGABLE | B_WILL_DRAW)
{
	fDirty = false;
	SetDoesUndo(true);
}


void
TSigTextView::DeleteText(int32 offset, int32 len)
{
	fDirty = true;
	BTextView::DeleteText(offset, len);
}


void
TSigTextView::InsertText(const char *text, int32 len, int32 offset,
	const text_run_array *runs)
{
	fDirty = true;
	BTextView::InsertText(text, len, offset, runs);
}


void
TSigTextView::KeyDown(const char *key, int32 count)
{
	bool	up = false;
	int32	height;
	BRect	r;

	switch (key[0]) {
		case B_HOME:
			Select(0, 0);
			ScrollToSelection();
			break;

		case B_END:
			Select(TextLength(), TextLength());
			ScrollToSelection();
			break;

		case B_PAGE_UP:
			up = true;
		case B_PAGE_DOWN:
			r = Bounds();
			height = (int32)((up ? r.top - r.bottom : r.bottom - r.top) - 25);
			if ((up) && (!r.top))
				break;
			ScrollBy(0, height);
			break;

		default:
			BTextView::KeyDown(key, count);
			break;
	}
}


void
TSigTextView::MessageReceived(BMessage *msg)
{
	char		type[B_FILE_NAME_LENGTH];
	char		*text;
	int32		end;
	int32		start;
	BFile		file;
	BNodeInfo	*node;
	entry_ref	ref;
	off_t		size;

	switch (msg->what) {
		case B_SIMPLE_DATA:
			if (msg->HasRef("refs")) {
				msg->FindRef("refs", &ref);
				file.SetTo(&ref, O_RDONLY);
				if (file.InitCheck() == B_NO_ERROR) {
					node = new BNodeInfo(&file);
					node->GetType(type);
					delete node;
					file.GetSize(&size);
					if ((!strncasecmp(type, "text/", 5)) && (size)) {
						text = (char *)malloc(size);
						file.Read(text, size);
						Delete();
						GetSelection(&start, &end);
						Insert(text, size);
						Select(start, start + size);
						free(text);
					}
				}
			}
			else
				BTextView::MessageReceived(msg);
			break;

		case M_SELECT:
			if (IsSelectable())
				Select(0, TextLength());
			break;

		default:
			BTextView::MessageReceived(msg);
			break;
	}
}
