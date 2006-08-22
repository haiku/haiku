/*
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files or portions
 * thereof (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice
 *    in the  binary, as well as this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided with
 *    the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <Window.h>
#include <Rect.h>
#include <TextControl.h>
#include <Box.h>
#include <CheckBox.h>
#include <Button.h>
#include <RadioButton.h>
#include <Message.h>
#include <stdio.h>
#include <File.h>
#include <String.h>

#include "TermWindow.h"
#include "FindDlg.h"
#include "TermApp.h"
#include "MenuUtil.h"
#include "PrefHandler.h"

// message define

const uint32 MSG_FIND_HIDE = 'Fhid';

//////////////////////////////////////////////////////////////////////////////
// FindDlg
// 	Constructer
//////////////////////////////////////////////////////////////////////////////
FindDlg::FindDlg (BRect frame, TermWindow *win)
	: BWindow(frame, "Find",
		B_FLOATING_WINDOW, B_NOT_RESIZABLE|B_NOT_ZOOMABLE)
{
 	fWindow = win;
	SetTitle("Find");

	AddShortcut ((ulong)'W', (ulong)B_COMMAND_KEY, new BMessage (MSG_FIND_HIDE));

	//Build up view
	fFindView = new BView(Bounds(), "FindView", B_FOLLOW_ALL, B_WILL_DRAW);
	fFindView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(fFindView);

	font_height height;
	fFindView->GetFontHeight(&height);
	float lineHeight = height.ascent + height.descent + height.leading;

	//These labels are from the bottom up
	float buttonsTop = frame.Height() - 19 - lineHeight;
	float matchwordBottom = buttonsTop - 4;
	float matchwordTop = matchwordBottom - lineHeight - 8;
	float matchcaseBottom = matchwordTop - 4;
	float matchcaseTop = matchcaseBottom - lineHeight - 8;
	float forwardsearchBottom = matchcaseTop - 4;
	float forwardsearchTop = forwardsearchBottom - lineHeight - 8;

	//These things are calculated from the top
	float textRadioTop = 12;
	float textRadioBottom = textRadioTop + 2 + lineHeight + 2 + 1;
	float textRadioRight = fFindView->StringWidth("Use Text: ") + 30;
	float selectionRadioTop = textRadioBottom + 4;
	float selectionRadioBottom = selectionRadioTop + lineHeight + 8;

	//Divider
	float dividerHeight = (selectionRadioBottom + forwardsearchTop) / 2;
	
	//Button Coordinates
	float searchbuttonLeft = (frame.Width() - fFindView->StringWidth("Find") - 60) / 2;
	float searchbuttonRight = searchbuttonLeft + fFindView->StringWidth("Find") + 60; 
	
	//Build the Views
	fTextRadio = new BRadioButton(BRect(14, textRadioTop, textRadioRight, textRadioBottom),
		"fTextRadio", "Use Text: ", NULL);
	fFindView->AddChild(fTextRadio);
	fTextRadio->SetValue(1);	//enable first option
	
	fFindLabel = new BTextControl(BRect(textRadioRight + 4, textRadioTop, frame.Width() - 14, textRadioBottom),
		"fFindLabel", "", "", NULL);
	fFindLabel->SetDivider(0);
	fFindView->AddChild(fFindLabel);
	fFindLabel->MakeFocus(true);

	fSelectionRadio = new BRadioButton(BRect(14, selectionRadioTop, frame.Width() - 14, selectionRadioBottom),
		"fSelectionRadio", "Use Selection", NULL);
	fFindView->AddChild(fSelectionRadio);

	fSeparator = new BBox(BRect(6, dividerHeight, frame.Width() - 6, dividerHeight + 1));
	fFindView->AddChild(fSeparator);
	
	fForwardSearchBox = new BCheckBox(BRect(14, forwardsearchTop, frame.Width() - 14, forwardsearchBottom),
		"fForwardSearchBox", "Search Forward", NULL);
	fFindView->AddChild(fForwardSearchBox);
	
	fMatchCaseBox = new BCheckBox(BRect(14, matchcaseTop, frame.Width() - 14, matchcaseBottom),
		"fMatchCaseBox", "Match Case", NULL);
	fFindView->AddChild(fMatchCaseBox);
	
	fMatchWordBox = new BCheckBox(BRect(14, matchwordTop, frame.Width() - 14, matchwordBottom),
		"fMatchWordBox", "Match Word", NULL);
	fFindView->AddChild(fMatchWordBox);
	
	fFindButton = new BButton(BRect(searchbuttonLeft, buttonsTop, searchbuttonRight, frame.Height() - 14),
		"fFindButton", "Find", new BMessage(MSG_FIND));
	fFindButton->MakeDefault(true);
	fFindView->AddChild(fFindButton);
	
	Show();
}

FindDlg::~FindDlg (void)
{
  
}

void
FindDlg::MessageReceived (BMessage *msg)
{
	switch (msg->what) {
		case B_QUIT_REQUESTED:
			Quit();
			break;
		case MSG_FIND:
			SendFindMessage();
			break;
		case MSG_FIND_HIDE:
			Quit();
			break;
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}

void
FindDlg::Quit (void)
{
	fWindow->PostMessage(MSG_FIND_CLOSED);
	BWindow::Quit ();
}

void
FindDlg::SendFindMessage (void)
{
	BMessage message(MSG_FIND);

	if (fTextRadio->Value() == B_CONTROL_ON) {
		message.AddString("findstring", fFindLabel->Text());
		message.AddBool("findselection", false);
	} else
		message.AddBool("findselection", true);
	
	//Add the other parameters
	message.AddBool("usetext", fTextRadio->Value() == B_CONTROL_ON);
	message.AddBool("forwardsearch", fForwardSearchBox->Value() == B_CONTROL_ON);
	message.AddBool("matchcase", fMatchCaseBox->Value() == B_CONTROL_ON);
	message.AddBool("matchword", fMatchWordBox->Value() == B_CONTROL_ON);
	
	fWindow->PostMessage(&message);
}
