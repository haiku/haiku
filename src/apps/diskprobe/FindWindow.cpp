/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "FindWindow.h"
#include "DiskProbe.h"

#include <Application.h>
#include <TextView.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <Button.h>
#include <ScrollView.h>
#include <CheckBox.h>
#include <Beep.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


enum find_mode {
	kAsciiMode,
	kHexMode
};

static const uint32 kMsgFindMode = 'FMde';
static const uint32 kMsgStartFind = 'SFnd';


class FindTextView : public BTextView {
	public:
		FindTextView(BRect frame, const char *name, BRect textRect, uint32 resizeMask);

		virtual void MakeFocus(bool state);
		virtual void TargetedByScrollView(BScrollView *view);

		status_t SetMode(find_mode mode);
		void GetData(BMessage &message);

		virtual void KeyDown(const char *bytes, int32 numBytes);

	protected:
		virtual	void InsertText(const char *text, int32 length, 
						int32 offset, const text_run_array *runs);

	private:
		BScrollView	*fScrollView;
		find_mode	fMode;
};


//---------------


FindTextView::FindTextView(BRect frame, const char *name, BRect textRect,
	uint32 resizeMask)
	: BTextView(frame, name, textRect, resizeMask),
	fScrollView(NULL),
	fMode(kAsciiMode)
{
}


void
FindTextView::MakeFocus(bool state)
{
	BTextView::MakeFocus(state);

	if (fScrollView != NULL)
		fScrollView->SetBorderHighlighted(state);
}


void 
FindTextView::TargetedByScrollView(BScrollView *view)
{
	BTextView::TargetedByScrollView(view);
	fScrollView = view;
}


void 
FindTextView::KeyDown(const char *bytes, int32 numBytes)
{
	if (fMode == kHexMode) {
		// filter out invalid characters
		if (numBytes > 1)
			return;
		
		if (bytes[0] == B_DELETE || bytes[0] == B_BACKSPACE) {
			// ToDo
			return;
		}

		if (!strchr("0123456789abcdefABCDEF", bytes[0]))
			return;

		// the original KeyDown() has severe cursor setting
		// problems with our InsertText().

		int32 start, end;
		GetSelection(&start, &end);
		InsertText(bytes, 1, start, NULL);
		Invalidate();
		return;
	}
	BTextView::KeyDown(bytes, numBytes);
}


void
FindTextView::InsertText(const char *text, int32 length, int32 offset,
	const text_run_array *runs)
{
	if (fMode == kHexMode) {
		if (offset > TextLength())
			offset = TextLength();

		BTextView::InsertText(text, length, offset, runs);
			// lets add anything, and then start to filter out
			// (since we have to reformat the whole text)

		const char *text = Text();
		int32 textLength = TextLength();
		char *insert = (char *)malloc(textLength * 2);
		if (insert == NULL)
			return;

		int32 cursor = TextLength();
		int32 out = 0;
		for (int32 i = 0; i < textLength; i++) {
			char c = text[i];
			if (c >= 'A' && c <= 'F')
				c += 'A' - 'a';
			if ((c >= 'a' && c <= 'f') || (c >= '0' && c <= '9'))
				insert[out++] = c;

			if (i == offset && length == 1) {
				// this is the end of the inserted text
				cursor = out;
			}

			if ((out % 3) == 2)
				insert[out++] = ' ';
		}
		insert[out] = '\0';

		DeleteText(0, textLength);
		BTextView::InsertText(insert, out, 0, NULL);

		int32 start, end;
		GetSelection(&start, &end);
		if (start == offset)
			Select(cursor, cursor);

		free(insert);
	} else
		BTextView::InsertText(text, length, offset, runs);
}


status_t
FindTextView::SetMode(find_mode mode)
{
	if (fMode == mode)
		return B_OK;

	if (mode == kHexMode) {
		// convert text to hex mode

		int32 length = TextLength();
		char *hex = (char *)malloc(length * 3 + 1);
		if (hex == NULL)
			return B_NO_MEMORY;

		fMode = mode;

		const char *text = Text();
		char *out = hex;
		for (int32 i = 0; i < length; i++) {
			out += sprintf(out, "%02x", *(unsigned char *)(text + i));
		}
		out[0] = '\0';
		SetText(hex);
		free(hex);
	} else {
		// ToDo: convert to ascii
		return B_ERROR;
	}

	return B_OK;	
}


void
FindTextView::GetData(BMessage &message)
{
	if (fMode == kHexMode) {
		// ToDo: ...
	} else 
		message.AddData("data", B_RAW_TYPE, Text(), TextLength());
}


//	#pragma mark -


FindWindow::FindWindow(BRect rect, BMessenger &target)
	: BWindow(rect, "Find", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS),
	fTarget(target)
{
	BView *view = new BView(Bounds(), "main", B_FOLLOW_ALL, 0);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(view);

	// add the top widgets

	fMenu = new BPopUpMenu("mode");
	BMessage *message;
	BMenuItem *item;
	fMenu->AddItem(item = new BMenuItem("String", message = new BMessage(kMsgFindMode)));
	item->SetMarked(true);
	message->AddInt8("mode", kAsciiMode);
	fMenu->AddItem(item = new BMenuItem("Hexadecimal", message = new BMessage(kMsgFindMode)));
	message->AddInt8("mode", kHexMode);

	BRect rect = Bounds().InsetByCopy(5, 5);
	BMenuField *menuField = new BMenuField(rect, B_EMPTY_STRING,
						"Mode:", fMenu, B_FOLLOW_LEFT | B_FOLLOW_TOP);
	menuField->SetDivider(menuField->StringWidth(menuField->Label()) + 8);
	menuField->ResizeToPreferred();
	view->AddChild(menuField);

	// add the bottom widgets

	BButton *button = new BButton(rect, B_EMPTY_STRING, "Find", new BMessage(kMsgStartFind),
								B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	button->MakeDefault(true);
	button->ResizeToPreferred();
	button->MoveTo(rect.right - button->Bounds().Width(),
				rect.bottom - button->Bounds().Height());
	view->AddChild(button);

	fCaseCheckBox = new BCheckBox(rect, B_EMPTY_STRING, "Case sensitive",
							NULL, B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	fCaseCheckBox->ResizeToPreferred();
	fCaseCheckBox->MoveTo(5, button->Frame().top - 5 - fCaseCheckBox->Bounds().Height());
	view->AddChild(fCaseCheckBox);

	// and now those inbetween

	rect.top = menuField->Frame().bottom + 5;
	rect.bottom = fCaseCheckBox->Frame().top - 5;
	rect.InsetBy(2, 2);
	fTextView = new FindTextView(rect, B_EMPTY_STRING,
						rect.OffsetToCopy(B_ORIGIN).InsetByCopy(3, 3),
						B_FOLLOW_ALL);
	fTextView->SetWordWrap(true);

	BScrollView *scrollView = new BScrollView("scroller", fTextView, B_FOLLOW_ALL, B_WILL_DRAW, false, false);
	view->AddChild(scrollView);

	ResizeTo(290, button->Frame().Height() * 4 + 30);
}


FindWindow::~FindWindow()
{
}


void 
FindWindow::WindowActivated(bool active)
{
	fTextView->MakeFocus(active);
}


void 
FindWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgFindMode:
		{
			int8 mode;
			if (message->FindInt8("mode", &mode) != B_OK)
				break;

			if (fTextView->SetMode((find_mode)mode) == B_OK) {
				// the "case sensitive" check box is only for ASCII mode
				fCaseCheckBox->SetEnabled(mode == kAsciiMode);
			} else {
				// activate other item
				fMenu->ItemAt(mode == kAsciiMode ? 1 : 0)->SetMarked(true);
				beep();
			}
			fTextView->MakeFocus(true);
			break;
		}

		case kMsgStartFind:
		{
			BMessage find(kMsgFind);
			fTextView->GetData(find);
			find.AddBool("case_sensitive", fCaseCheckBox->Value() != 0);
			fTarget.SendMessage(&find);

			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		
		default:
			BWindow::MessageReceived(message);
	}
}


bool 
FindWindow::QuitRequested()
{
	be_app_messenger.SendMessage(kMsgFindWindowClosed);
	return true;
}


void 
FindWindow::SetTarget(BMessenger &target)
{
	fTarget = target;
}

