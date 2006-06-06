/*
 * Copyright 2006 Haiku, Inc. All Rights Reserved.
 * Copyright 1997, 1998 R3 Software Ltd. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Timothy Wayper <timmy@wunderbear.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "CalcOptionsWindow.h"

#include <stdlib.h>
#include <stdio.h>

#include <Box.h>
#include <Button.h>
#include <CheckBox.h>


CalcOptions::CalcOptions()
	: auto_num_lock(true),
	  audio_feedback(false)
{
}


CalcOptionsWindow::CalcOptionsWindow(BRect frame, CalcOptions *options,
									 BMessage* quitMessage,
									 BHandler* target)
	: BWindow(frame, "Options", B_TITLED_WINDOW,
			  B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE),
	  fOptions(options),
	  fQuitMessage(quitMessage),
	  fTarget(target)
{
	// TODO: revisit when Ingo has created his layout framework...
	frame.OffsetTo(B_ORIGIN);
	BBox* bg = new BBox(frame, "bg", B_FOLLOW_ALL);
	bg->SetBorder(B_PLAIN_BORDER);
	AddChild(bg);

	frame.InsetBy(5.0f, 5.0f);

	// create interface components
	float y = 16.0f, vw, vh;
	BRect viewframe(4.0f, y, frame.right, y + 16.0f);
	fAutoNumLockCheckBox = new BCheckBox(viewframe,
		"autoNumLockCheckBox", "Auto Num Lock", NULL);
	if (fOptions->auto_num_lock) {
		fAutoNumLockCheckBox->SetValue(B_CONTROL_ON);
	}
	bg->AddChild(fAutoNumLockCheckBox);
	fAutoNumLockCheckBox->ResizeToPreferred();
	y += fAutoNumLockCheckBox->Frame().Height();
	
	viewframe.Set(4.0f, y, frame.right, y + 16.0f);
	fAudioFeedbackCheckBox = new BCheckBox(viewframe,
		"audioFeedbackCheckBox", "Audio Feedback", NULL);
	if (fOptions->audio_feedback) {
		fAudioFeedbackCheckBox->SetValue(B_CONTROL_ON);
	}
	bg->AddChild(fAudioFeedbackCheckBox);
	fAudioFeedbackCheckBox->ResizeToPreferred();
	y += fAudioFeedbackCheckBox->Frame().Height();
	
	// create buttons
	viewframe.Set(0.0f, 0.0f, 40.0f, 40.0f);
	fOkButton = new BButton(viewframe, "okButton", "OK",
		new BMessage(B_QUIT_REQUESTED));
	fOkButton->MakeDefault(true);
	fOkButton->GetPreferredSize(&vw, &vh);
	fOkButton->ResizeTo(vw, vh);
	fOkButton->MoveTo(frame.right - vw - 8.0f,
		frame.bottom - vh - 8.0f);
	bg->AddChild(fOkButton);
	
	float cw, ch;
	fCancelButton = new BButton(viewframe, "cancelButton", "Cancel",
		new BMessage(B_QUIT_REQUESTED));
	fCancelButton->GetPreferredSize(&cw, &ch);
	fCancelButton->ResizeTo(cw, ch);
	fCancelButton->MoveTo(frame.right - vw - 16.0f - cw,
		frame.bottom - ch - 8.0f);
	bg->AddChild(fCancelButton);
}

 
CalcOptionsWindow::~CalcOptionsWindow()
{
	delete fQuitMessage;
}


bool
CalcOptionsWindow::QuitRequested()
{
	// auto num-lock
	fOptions->auto_num_lock = fAutoNumLockCheckBox->Value() == B_CONTROL_ON;

	// audio feedback
	fOptions->audio_feedback = fAudioFeedbackCheckBox->Value() == B_CONTROL_ON;

	// notify target of our demise
	if (fQuitMessage && fTarget && fTarget->Looper()) {
		fQuitMessage->AddRect("window frame", Frame());
		fTarget->Looper()->PostMessage(fQuitMessage, fTarget);
	}

	return true;
}


