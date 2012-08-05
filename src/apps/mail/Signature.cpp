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
#include <string.h>

#include <Clipboard.h>
#include <InterfaceKit.h>
#include <Locale.h>
#include <StorageKit.h>

#include "MailApp.h"
#include "MailPopUpMenu.h"
#include "MailSupport.h"
#include "MailWindow.h"
#include "Messages.h"


#define B_TRANSLATION_CONTEXT "Mail"


extern BRect		signature_window;
extern const char	*kUndoStrings[];
extern const char	*kRedoStrings[];


TSignatureWindow::TSignatureWindow(BRect rect)
	:
	BWindow (rect, B_TRANSLATE("Signatures"), B_TITLED_WINDOW, 0),
	fFile(NULL)
{
	BMenu		*menu;
	BMenuBar	*menu_bar;
	BMenuItem	*item;

	BRect r = Bounds();
	/*** Set up the menus ****/
	menu_bar = new BMenuBar(r, "MenuBar");
	menu = new BMenu(B_TRANSLATE("Signature"));
	menu->AddItem(fNew = new BMenuItem(B_TRANSLATE("New"),
		new BMessage(M_NEW), 'N'));
	fSignature = new TMenu(B_TRANSLATE("Open"), INDEX_SIGNATURE, M_SIGNATURE);
	menu->AddItem(new BMenuItem(fSignature));
	menu->AddSeparatorItem();
	menu->AddItem(fSave = new BMenuItem(B_TRANSLATE("Save"),
		new BMessage(M_SAVE), 'S'));
	menu->AddItem(fDelete = new BMenuItem(B_TRANSLATE("Delete"),
		new BMessage(M_DELETE), 'T'));
	menu_bar->AddItem(menu);

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
	menu_bar->AddItem(menu);

	AddChild(menu_bar);
	/**** Done with the menu set up *****/

	/**** Add on the panel, giving it the width and at least one vertical pixel *****/
	fSigView = new TSignatureView(BRect(0, menu_bar->Frame().bottom+1,
										rect.Width(), menu_bar->Frame().bottom+2));
	AddChild(fSigView);

	/* resize the window to the correct height */
	fSigView->SetResizingMode(B_FOLLOW_NONE);
	ResizeTo(rect.Width()-2, fSigView->Frame().bottom-2);
	fSigView->SetResizingMode(B_FOLLOW_ALL);

	SetSizeLimits(kSigWidth, RIGHT_BOUNDARY, r.top + 100, RIGHT_BOUNDARY);
}


TSignatureWindow::~TSignatureWindow()
{
}


void
TSignatureWindow::MenusBeginning()
{
	int32		finish = 0;
	int32		start = 0;
	BTextView	*text_view;

	fDelete->SetEnabled(fFile);
	fSave->SetEnabled(IsDirty());
	fUndo->SetEnabled(false);		// ***TODO***

	text_view = (BTextView *)fSigView->fName->ChildAt(0);
	if (text_view->IsFocus())
		text_view->GetSelection(&start, &finish);
	else
		fSigView->fTextView->GetSelection(&start, &finish);

	fCut->SetEnabled(start != finish);
	fCopy->SetEnabled(start != finish);

	fNew->SetEnabled(text_view->TextLength() | fSigView->fTextView->TextLength());
	be_clipboard->Lock();
	fPaste->SetEnabled(be_clipboard->Data()->HasData("text/plain", B_MIME_TYPE));
	be_clipboard->Unlock();

	// Undo stuff
	bool		isRedo = false;
	undo_state	undoState = B_UNDO_UNAVAILABLE;

	BTextView *focusTextView = dynamic_cast<BTextView *>(CurrentFocus());
	if (focusTextView != NULL)
		undoState = focusTextView->UndoState(&isRedo);

	fUndo->SetLabel((isRedo) ? kRedoStrings[undoState] : kUndoStrings[undoState]);
	fUndo->SetEnabled(undoState != B_UNDO_UNAVAILABLE);
}


void
TSignatureWindow::MessageReceived(BMessage* msg)
{
	char		*sig;
	char		name[B_FILE_NAME_LENGTH];
	BFont		*font;
	BTextView	*text_view;
	entry_ref	ref;
	off_t		size;

	switch(msg->what) {
		case CHANGE_FONT:
			msg->FindPointer("font", (void **)&font);
			fSigView->fTextView->SetFontAndColor(font);
			fSigView->fTextView->Invalidate(fSigView->fTextView->Bounds());
			break;

		case M_NEW:
			if (Clear()) {
				fSigView->fName->SetText("");
//				fSigView->fTextView->SetText(NULL, (int32)0);
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
				msg->FindRef("ref", &ref);
				fEntry.SetTo(&ref);
				fFile = new BFile(&ref, O_RDWR);
				if (fFile->InitCheck() == B_NO_ERROR) {
					fFile->ReadAttr(INDEX_SIGNATURE, B_STRING_TYPE, 0, name, sizeof(name));
					fSigView->fName->SetText(name);
					fFile->GetSize(&size);
					sig = (char *)malloc(size);
					size = fFile->Read(sig, size);
					fSigView->fTextView->SetText(sig, size);
					fSigView->fName->MakeFocus(true);
					text_view = (BTextView *)fSigView->fName->ChildAt(0);
					text_view->Select(0, text_view->TextLength());
					fSigView->fTextView->fDirty = false;
				}
				else {
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
	BTextView	*text_view;

	Lock();
	text_view = (BTextView *)fSigView->fName->TextView();
	fSigView->fName->MakeFocus(true);
	text_view->Select(0, text_view->TextLength());
	Unlock();

	BWindow::Show();
}


bool
TSignatureWindow::Clear()
{
	int32		result;

	if (IsDirty()) {
		beep();
		BAlert *alert = new BAlert("",
			B_TRANSLATE("Save changes to this signature?"),
			B_TRANSLATE("Don't save"),
			B_TRANSLATE("Cancel"),
			B_TRANSLATE("Save"),
			B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->SetShortcut(0, 'd');
		alert->SetShortcut(1, B_ESCAPE);
		result = alert->Go();
		if (result == 1)
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
	char		name[B_FILE_NAME_LENGTH];

	if (fFile) {
		fFile->ReadAttr(INDEX_SIGNATURE, B_STRING_TYPE, 0, name, sizeof(name));
		if ((strcmp(name, fSigView->fName->Text())) || (fSigView->fTextView->fDirty))
			return true;
	}
	else {
		if ((strlen(fSigView->fName->Text())) ||
			(fSigView->fTextView->TextLength()))
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
			sprintf(name, "signature_%ld", index++);
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


//====================================================================
//	#pragma mark -


TSignatureView::TSignatureView(BRect rect)
	: BBox(rect, "SigView", B_FOLLOW_ALL, B_WILL_DRAW)
{
}


void
TSignatureView::AttachedToWindow()
{
	BRect	rect = Bounds();
	float	name_text_length = StringWidth(B_TRANSLATE("Title:"));
	float	sig_text_length = StringWidth(B_TRANSLATE("Signature:"));
	float	divide_length;

	if (name_text_length > sig_text_length)
		divide_length = name_text_length;
	else
		divide_length = sig_text_length;

	rect.InsetBy(8,0);
	rect.top+= 8;

	fName = new TNameControl(rect, B_TRANSLATE("Title:"),
		new BMessage(NAME_FIELD));
	AddChild(fName);

	fName->SetDivider(divide_length + 10);
	fName->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);

	rect.OffsetBy(0,fName->Bounds().Height()+5);
	rect.bottom = rect.top + kSigHeight;
	rect.left = fName->TextView()->Frame().left;

	BRect text = rect;
	text.OffsetTo(10,0);
	fTextView = new TSigTextView(rect, text);
	BScrollView *scroller = new BScrollView("SigScroller", fTextView, B_FOLLOW_ALL, 0, false, true);
	AddChild(scroller);
	scroller->ResizeBy(-1 * scroller->ScrollBar(B_VERTICAL)->Frame().Width() - 9, 0);
	scroller->MoveBy(7,0);

	/* back up a bit to make room for the label */

	rect = scroller->Frame();
	BStringView *stringView = new BStringView(rect, "SigLabel",
		B_TRANSLATE("Signature:"));
	AddChild(stringView);

	float tWidth, tHeight;
	stringView->GetPreferredSize(&tWidth, &tHeight);

	/* the 5 is for the spacer in the TextView */

	rect.OffsetBy(-1 *(tWidth) - 5, 0);
	rect.right = rect.left + tWidth;
	rect.bottom = rect.top + tHeight;

	stringView->MoveTo(rect.LeftTop());
	stringView->ResizeTo(rect.Width(), rect.Height());

	/* Resize the View to the correct height */
	scroller->SetResizingMode(B_FOLLOW_NONE);
	ResizeTo(Frame().Width(), scroller->Frame().bottom + 8);
	scroller->SetResizingMode(B_FOLLOW_ALL);
}


//====================================================================
//	#pragma mark -


TNameControl::TNameControl(BRect rect, const char *label, BMessage *msg)
			 :BTextControl(rect, "", label, "", msg, B_FOLLOW_LEFT_RIGHT)
{
	strcpy(fLabel, label);
}


void
TNameControl::AttachedToWindow()
{
	BTextControl::AttachedToWindow();

	SetDivider(StringWidth(fLabel) + 6);
	TextView()->SetMaxBytes(B_FILE_NAME_LENGTH - 1);
}


void
TNameControl::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case M_SELECT:
			TextView()->Select(0, TextView()->TextLength());
			break;

		default:
			BTextControl::MessageReceived(msg);
	}
}


//====================================================================
//	#pragma mark -


TSigTextView::TSigTextView(BRect frame, BRect text)
			 :BTextView(frame, "SignatureView", text, B_FOLLOW_ALL, B_NAVIGABLE | B_WILL_DRAW)
{
	fDirty = false;
	SetDoesUndo(true);
}


void
TSigTextView::FrameResized(float /*width*/, float /*height*/)
{
	BRect r(Bounds());
	r.InsetBy(3, 3);
	SetTextRect(r);
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
	}
}

