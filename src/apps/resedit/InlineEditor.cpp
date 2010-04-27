/*
 * Copyright (c) 2005-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@gmail.com>
 */
#include "InlineEditor.h"

#include <Handler.h>
#include <MessageFilter.h>

class EditFilter : public BMessageFilter
{
public:
	EditFilter(BTextControl *textbox)
		: BMessageFilter(B_PROGRAMMED_DELIVERY, B_ANY_SOURCE, B_KEY_DOWN)
	{
		fTextBox = textbox;
	}
		
	~EditFilter(void)
	{
	}
	
	filter_result Filter(BMessage *msg, BHandler **target)
	{
		int32 rawchar;
		msg->FindInt32("raw_char", &rawchar);
		
		if (rawchar == B_ESCAPE) {
			BLooper *loop = (*target)->Looper();
			if (loop) {
				BMessenger msgr(loop);
				msgr.SendMessage(B_QUIT_REQUESTED);
				return B_SKIP_MESSAGE;
			}
		} else if (rawchar == B_ENTER) {
			fTextBox->Invoke();
			return B_SKIP_MESSAGE;
		}
		return B_DISPATCH_MESSAGE;
	}

private:
	BTextControl 	*fTextBox;

};

InlineEditor::InlineEditor(BMessenger target, const BRect &frame, 
							const char *text)
 :	BWindow(frame, "InlineEditor", B_NO_BORDER_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
 			B_ASYNCHRONOUS_CONTROLS),
 	fMessenger(target),
 	fCommand(M_INLINE_TEXT)
{
	fTextBox = new BTextControl(BRect(0, 0, 1, 1), "inlinebox", NULL, text,
								new BMessage(fCommand), B_FOLLOW_ALL, B_WILL_DRAW);
	AddChild(fTextBox);
	fTextBox->SetDivider(0);
	fTextBox->MakeFocus(true);
	
	fTextBox->ResizeToPreferred();
	fTextBox->ResizeTo(Bounds().Width(), fTextBox->Bounds().Height());
	ResizeTo(Bounds().Width(), fTextBox->Bounds().Height());
	
	AddCommonFilter(new EditFilter(fTextBox));
}


bool
InlineEditor::QuitRequested(void)
{
	return true;
}


void
InlineEditor::SetMessage(BMessage *msg)
{
	fCommand = msg ? msg->what : 0;
	fTextBox->SetMessage(msg);
}


void
InlineEditor::MessageReceived(BMessage *msg)
{
	if (msg->what == fCommand) {
		fMessenger.SendMessage(msg);
		PostMessage(B_QUIT_REQUESTED);
	}
}

void
InlineEditor::WindowActivated(bool active)
{
	if (!active)
		PostMessage(B_QUIT_REQUESTED);
}

