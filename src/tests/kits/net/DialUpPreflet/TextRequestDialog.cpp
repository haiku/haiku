//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#include "TextRequestDialog.h"
#include "InterfaceUtils.h"

#include <Button.h>
#include <TextControl.h>


// GUI constants
static const uint32 kWindowWidth = 250;
static const uint32 kWindowHeight = 5 + 20 + 10 + 25 + 5;
static const BRect kWindowRect(0, 0, kWindowWidth, kWindowHeight);
static const uint32 kDefaultButtonWidth = 80;

// message constants
static const int32 kMsgButton = 'MBTN';

// labels
static const char *kLabelOK = "OK";
static const char *kLabelCancel = "Cancel";


TextRequestDialog::TextRequestDialog(const char *title, const char *request,
		const char *text = NULL)
	: BWindow(kWindowRect, title, B_MODAL_WINDOW, B_NOT_RESIZABLE | B_NOT_CLOSABLE, 0),
	fInvoker(NULL)
{
	BRect rect = Bounds();
	BView *backgroundView = new BView(rect, "background", B_FOLLOW_ALL_SIDES, 0);
	backgroundView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	rect.InsetBy(5, 5);
	rect.bottom = rect.top + 20;
	fTextControl = new BTextControl(rect, "request", request, text, NULL);
	fTextControl->SetDivider(fTextControl->StringWidth(fTextControl->Label()) + 5);
	if(text && strlen(text) > 0)
		fTextControl->TextView()->SelectAll();
	
	rect.top = rect.bottom + 10;
	rect.bottom = rect.top + 25;
	rect.left = rect.right - kDefaultButtonWidth;
	BMessage message(kMsgButton);
	message.AddInt32("which", 1);
	BButton *okButton = new BButton(rect, "okButton", kLabelOK, new BMessage(message));
	rect.right = rect.left - 10;
	rect.left = rect.right - kDefaultButtonWidth;
	message.ReplaceInt32("which", 0);
	BButton *cancelButton = new BButton(rect, "cancelButton", kLabelCancel,
		new BMessage(message));
	backgroundView->AddChild(okButton);
	backgroundView->AddChild(cancelButton);
	backgroundView->AddChild(fTextControl);
	AddChild(backgroundView);
	
	fTextControl->MakeFocus(true);
	SetDefaultButton(okButton);
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
