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

		find_mode Mode() const { return fMode; }
		status_t SetMode(find_mode mode);

		void SetData(BMessage &message);
		void GetData(BMessage &message);

		virtual void KeyDown(const char *bytes, int32 numBytes);

	protected:
		virtual	void InsertText(const char *text, int32 length, 
						int32 offset, const text_run_array *runs);

	private:
		void HexReformat(int32 oldCursor, int32 &newCursor);
		status_t GetHexFromData(const uint8 *in, size_t inSize, char **_hex, size_t *_hexSize);
		status_t GetDataFromHex(const char *text, size_t textLength, uint8 **_data, size_t *_dataSize);

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
FindTextView::HexReformat(int32 oldCursor, int32 &newCursor)
{
	const char *text = Text();
	int32 textLength = TextLength();
	char *insert = (char *)malloc(textLength * 2);
	if (insert == NULL)
		return;

	newCursor = TextLength();
	int32 out = 0;
	for (int32 i = 0; i < textLength; i++) {
		if (i == oldCursor) {
			// this is the end of the inserted text
			newCursor = out;
		}

		char c = text[i];
		if (c >= 'A' && c <= 'F')
			c += 'a' - 'A';
		if ((c >= 'a' && c <= 'f') || (c >= '0' && c <= '9'))
			insert[out++] = c;

		if ((out % 48) == 47)
			insert[out++] = '\n';
		else if ((out % 3) == 2)
			insert[out++] = ' ';
	}
	insert[out] = '\0';

	DeleteText(0, textLength);

	// InsertText() does not work here, as we need the text
	// to be reformatted as well (newlines, breaks, whatever).
	// IOW the BTextView class is not very nicely done.
	//	BTextView::InsertText(insert, out, 0, NULL);
	fMode = kAsciiMode;
	Insert(0, insert, out);
	fMode = kHexMode;

	free(insert);
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

		int32 start, end;
		GetSelection(&start, &end);

		int32 cursor;
		HexReformat(offset, cursor);

		if (length == 1 && start == offset)
			Select(cursor + 1, cursor + 1);
	} else
		BTextView::InsertText(text, length, offset, runs);
}


void 
FindTextView::KeyDown(const char *bytes, int32 numBytes)
{
	if (fMode == kHexMode) {
		// filter out invalid (for hex mode) characters
		if (numBytes > 1)
			return;

		switch (bytes[0]) {
			case B_RIGHT_ARROW:
			case B_LEFT_ARROW:
			case B_UP_ARROW:
			case B_DOWN_ARROW:
			case B_HOME:
			case B_END:
			case B_PAGE_UP:
			case B_PAGE_DOWN:
				break;

			case B_BACKSPACE:
			{
				int32 start, end;
				GetSelection(&start, &end);

				if (bytes[0] == B_BACKSPACE) {
					start--;
					if (start < 0)
						return;
				}

				if (Text()[start] == ' ')
					BTextView::KeyDown(bytes, numBytes);

				BTextView::KeyDown(bytes, numBytes);

				GetSelection(&start, &end);
				HexReformat(start, start);
				Select(start, start);
				return;
			}

			case B_DELETE:
			{
				int32 start, end;
				GetSelection(&start, &end);

				if (Text()[start] == ' ')
					BTextView::KeyDown(bytes, numBytes);

				BTextView::KeyDown(bytes, numBytes);

				HexReformat(start, start);
				Select(start, start);
				return;
			}

			default:
			{
				if (!strchr("0123456789abcdefABCDEF", bytes[0]))
					return;

				// the original KeyDown() has severe cursor setting
				// problems with our InsertText().

				int32 start, end;
				GetSelection(&start, &end);
				InsertText(bytes, 1, start, NULL);
				return;
			}
		}
	}
	BTextView::KeyDown(bytes, numBytes);
}


status_t
FindTextView::GetHexFromData(const uint8 *in, size_t inSize, char **_hex, size_t *_hexSize)
{
	char *hex = (char *)malloc(inSize * 3 + 1);
	if (hex == NULL)
		return B_NO_MEMORY;

	char *out = hex;
	for (uint32 i = 0; i < inSize; i++) {
		out += sprintf(out, "%02x", *(unsigned char *)(in + i));
	}
	out[0] = '\0';
	
	*_hex = hex;
	*_hexSize = out + 1 - hex;
	return B_OK;
}


status_t 
FindTextView::GetDataFromHex(const char *text, size_t textLength, uint8 **_data, size_t *_dataSize)
{
	uint8 *data = (uint8 *)malloc(textLength);
	if (data == NULL)
		return B_NO_MEMORY;

	size_t dataSize = 0;
	uint8 hiByte = 0;
	bool odd = false;
	for (uint32 i = 0; i < textLength; i++) {
		char c = text[i];
		int32 number;
		if (c >= 'A' && c <= 'F')
			number = c + 10 - 'A';
		else if (c >= 'a' && c <= 'f')
			number = c + 10 - 'a';
		else if (c >= '0' && c <= '9')
			number = c - '0';
		else
			continue;

		if (!odd)
			hiByte = (number << 4) & 0xf0;
		else
			data[dataSize++] = hiByte | (number & 0x0f);

		odd = !odd;
	}
	if (odd)
		data[dataSize++] = hiByte;

	*_data = data;
	*_dataSize = dataSize;
	return B_OK;
}


status_t
FindTextView::SetMode(find_mode mode)
{
	if (fMode == mode)
		return B_OK;

	if (mode == kHexMode) {
		// convert text to hex mode

		char *hex;
		size_t hexSize;
		if (GetHexFromData((const uint8 *)Text(), TextLength(), &hex, &hexSize) < B_OK)
			return B_NO_MEMORY;

		fMode = mode;

		SetText(hex, hexSize);
		free(hex);
	} else {
		// convert hex to ascii
		
		uint8 *data;
		size_t dataSize;
		if (GetDataFromHex(Text(), TextLength(), &data, &dataSize) < B_OK)
			return B_NO_MEMORY;

		fMode = mode;

		SetText((const char *)data, dataSize);
		free(data);
	}

	return B_OK;	
}


void
FindTextView::SetData(BMessage &message)
{
	const uint8 *data;
	ssize_t dataSize;
	if (message.FindData("data", B_RAW_TYPE, (const void **)&data, &dataSize) != B_OK)
		return;

	if (fMode == kHexMode) {
		char *hex;
		size_t hexSize;
		if (GetHexFromData(data, dataSize, &hex, &hexSize) < B_OK)
			return;

		SetText(hex, hexSize);
		free(hex);
	} else
		SetText((char *)data, dataSize);
}


void
FindTextView::GetData(BMessage &message)
{
	if (fMode == kHexMode) {
		// convert hex-text to real data
		uint8 *data;
		size_t dataSize;
		if (GetDataFromHex(Text(), TextLength(), &data, &dataSize) != B_OK)
			return;

		message.AddData("data", B_RAW_TYPE, data, dataSize);
		free(data);
	} else 
		message.AddData("data", B_RAW_TYPE, Text(), TextLength());
}


//	#pragma mark -


FindWindow::FindWindow(BRect rect, BMessage &previous, BMessenger &target)
	: BWindow(rect, "Find", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS),
	fTarget(target)
{
	BView *view = new BView(Bounds(), "main", B_FOLLOW_ALL, 0);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(view);

	int8 mode = kAsciiMode;
	previous.FindInt8("find_mode", &mode);

	// add the top widgets

	fMenu = new BPopUpMenu("mode");
	BMessage *message;
	BMenuItem *item;
	fMenu->AddItem(item = new BMenuItem("Text", message = new BMessage(kMsgFindMode)));
	message->AddInt8("mode", kAsciiMode);
	if (mode == kAsciiMode)
		item->SetMarked(true);
	fMenu->AddItem(item = new BMenuItem("Hexadecimal", message = new BMessage(kMsgFindMode)));
	message->AddInt8("mode", kHexMode);
	if (mode == kHexMode)
		item->SetMarked(true);

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
	bool caseSensitive;
	if (previous.FindBool("case_sensitive", &caseSensitive) != B_OK)
		caseSensitive = true;
	fCaseCheckBox->SetValue(caseSensitive);
	view->AddChild(fCaseCheckBox);

	// and now those inbetween

	rect.top = menuField->Frame().bottom + 5;
	rect.bottom = fCaseCheckBox->Frame().top - 5;
	rect.InsetBy(2, 2);
	fTextView = new FindTextView(rect, B_EMPTY_STRING,
						rect.OffsetToCopy(B_ORIGIN).InsetByCopy(3, 3),
						B_FOLLOW_ALL);
	fTextView->SetWordWrap(true);
	fTextView->SetMode((find_mode)mode);
	fTextView->SetData(previous);

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

			if (fTextView->SetMode((find_mode)mode) != B_OK) {
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
			find.AddInt8("find_mode", fTextView->Mode());
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

