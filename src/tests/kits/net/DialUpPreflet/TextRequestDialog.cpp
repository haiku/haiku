/* -----------------------------------------------------------------------
 * Copyright (c) 2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 * ----------------------------------------------------------------------- */

#include "TextRequestDialog.h"
#include "InterfaceUtils.h"

#include <Button.h>
#include <TextControl.h>
#include <TextView.h>


// GUI constants
static const uint32 kWindowWidth = 250;
static const uint32 kWindowHeight = 10 + 20 + 10 + 25 + 5;
static const BRect kWindowRect(0, 0, kWindowWidth, kWindowHeight);
static const uint32 kDefaultButtonWidth = 80;

// message constants
static const uint32 kMsgButton = 'MBTN';
static const uint32 kMsgUpdateControls = 'UCTL';

// labels
static const char *kLabelOK = "OK";
static const char *kLabelCancel = "Cancel";


TextRequestDialog::TextRequestDialog(const char *title, const char *information,
		const char *request, const char *text)
	: BWindow(kWindowRect, title, B_MODAL_WINDOW, B_NOT_RESIZABLE | B_NOT_CLOSABLE, 0),
	fInvoker(NULL)
{
	BRect rect = Bounds();
	BView *backgroundView = new BView(rect, "background", B_FOLLOW_ALL_SIDES, 0);
	backgroundView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	rect.InsetBy(5, 5);
	rect.bottom = rect.top;
		// init
	
	if(information) {
		BRect textRect(rect);
		textRect.OffsetTo(0, 0);
		fTextView = new BTextView(rect, "TextView", textRect, B_FOLLOW_NONE,
			B_WILL_DRAW);
		fTextView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		fTextView->MakeSelectable(false);
		fTextView->MakeEditable(false);
		fTextView->SetText(information);
		float textHeight = fTextView->TextHeight(0, fTextView->CountLines());
		backgroundView->ResizeBy(0, textHeight + 5);
		ResizeBy(0, textHeight + 5);
		fTextView->ResizeBy(0, textHeight - textRect.Height());
		rect.bottom += textHeight + 5;
		backgroundView->AddChild(fTextView);
	} else
		fTextView = NULL;
	
	rect.top = rect.bottom + 5;
	rect.bottom = rect.top + 20;
	fTextControl = new BTextControl(rect, "request", request, text, NULL);
	fTextControl->SetModificationMessage(new BMessage(kMsgUpdateControls));
	fTextControl->SetDivider(fTextControl->StringWidth(fTextControl->Label()) + 5);
	if(text && strlen(text) > 0)
		fTextControl->TextView()->SelectAll();
	
	rect.top = rect.bottom + 10;
	rect.bottom = rect.top + 25;
	rect.left = rect.right - kDefaultButtonWidth;
	BMessage message(kMsgButton);
	message.AddInt32("which", 1);
	fOKButton = new BButton(rect, "okButton", kLabelOK, new BMessage(message));
	rect.right = rect.left - 10;
	rect.left = rect.right - kDefaultButtonWidth;
	message.ReplaceInt32("which", 0);
	BButton *cancelButton = new BButton(rect, "cancelButton", kLabelCancel,
		new BMessage(message));
	backgroundView->AddChild(cancelButton);
	backgroundView->AddChild(fOKButton);
	backgroundView->AddChild(fTextControl);
	AddChild(backgroundView);
	
	fTextControl->MakeFocus(true);
	SetDefaultButton(fOKButton);
	
	UpdateControls();
}


TextRequestDialog::~TextRequestDialog()
{
	delete fInvoker;
}


void
TextRequestDialog::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case kMsgButton: {
			if(!fInvoker || !fInvoker->Message())
				return;
			
			int32 which;
			message->FindInt32("which", &which);
			BMessage toSend(*fInvoker->Message());
			toSend.AddInt32("which", which);
			if(which == 1)
				toSend.AddString("text", fTextControl->Text());
			
			fInvoker->Invoke(&toSend);
			PostMessage(B_QUIT_REQUESTED);
		} break;
		
		case kMsgUpdateControls:
			UpdateControls();
		break;
		
		default:
			BWindow::MessageReceived(message);
	}
}


bool
TextRequestDialog::QuitRequested()
{
	return true;
}


status_t
TextRequestDialog::Go(BInvoker *invoker)
{
	fInvoker = invoker;
	MoveTo(center_on_screen(Bounds()));
	Show();
	
	return B_OK;
}


void
TextRequestDialog::UpdateControls()
{
	fOKButton->SetEnabled(fTextControl->TextView()->TextLength() > 0);
}
