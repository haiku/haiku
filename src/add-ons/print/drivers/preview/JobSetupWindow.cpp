/*
 * Copyright 2003-2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 *		Simon Gauvin
 *		Michael Pfeiffer
 *		julun <host.haiku@gmx.de>
 */

#include "JobSetupWindow.h"
#include "PrinterDriver.h"


#include <stdlib.h>


#include <Box.h>
#include <Button.h>
#include <RadioButton.h>
#include <Screen.h>
#include <TextControl.h>


JobSetupWindow::JobSetupWindow(BMessage *msg, const char * printerName)
	:	BlockingWindow(BRect(0, 0, 300, 200), "Job Setup", B_TITLED_WINDOW_LOOK,
 			B_MODAL_APP_WINDOW_FEEL, B_NOT_RESIZABLE | B_NOT_MINIMIZABLE |
 			B_NOT_ZOOMABLE),
	fPrinterName(printerName),
	fSetupMsg(msg)
{
	if (printerName)
		SetTitle(BString(printerName).Append(" Job Setup").String());

	int32 firstPage;
	fSetupMsg->FindInt32("first_page", &firstPage);

	int32 lastPage;
	fSetupMsg->FindInt32("last_page", &lastPage);
	bool allPages = firstPage == 1 && lastPage == LONG_MAX;

	BRect bounds(Bounds());
	BBox *panel = new BBox(bounds, "background", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP, B_PLAIN_BORDER);
	AddChild(panel);

	bounds.InsetBy(10.0, 10.0);

	fAll = new BRadioButton(bounds, "allPages", "Print all pages",
		new BMessage(ALL_PAGES_MGS));
	panel->AddChild(fAll);
	fAll->ResizeToPreferred();
	fAll->SetValue(allPages);

	bounds.OffsetBy(0.0, fAll->Bounds().Height() + 10.0);
	fRange = new BRadioButton(bounds, "pagesRange", "Print pages:",
		new BMessage(RANGE_SELECTION_MSG));
	panel->AddChild(fRange);
	fRange->ResizeToPreferred();
	fRange->SetValue(!allPages);

	bounds.OffsetBy(0.0, fRange->Bounds().Height() + 5.0);
	BRect rect(bounds);
	rect.right = be_plain_font->StringWidth("From: SomeSpaceHere");
	fFrom = new BTextControl(rect, "from", "From:", "SomeSpaceHere", NULL);
	panel->AddChild(fFrom);
	fFrom->SetAlignment(B_ALIGN_LEFT, B_ALIGN_RIGHT);
	fFrom->ResizeToPreferred();
	fFrom->SetDivider(be_plain_font->StringWidth("From: "));
	fFrom->SetEnabled(!allPages);

	fTo = new BTextControl(rect, "to", "To:", "SomeSpaceHere", NULL);
	panel->AddChild(fTo);
	fTo->SetAlignment(B_ALIGN_LEFT, B_ALIGN_RIGHT);
	fTo->ResizeToPreferred();
	fTo->SetDivider(be_plain_font->StringWidth("To: "));
	fTo->MoveTo(fFrom->Frame().right + 10.0, fTo->Frame().top);
	fTo->SetEnabled(!allPages);

	BString buffer;
	buffer << firstPage;
	fFrom->SetText(buffer.String());

	buffer = "";
	buffer << lastPage;
	fTo->SetText(buffer.String());

	for (uint32 i = 0; i < '0'; i++) {
		fTo->TextView()->DisallowChar(i);
		fFrom->TextView()->DisallowChar(i);
	}

	for (uint32 i = '9' + 1; i < 255; i++) {
		fTo->TextView()->DisallowChar(i);
		fFrom->TextView()->DisallowChar(i);
	}

	bounds.OffsetBy(0.0, fTo->Bounds().Height() + 10.0);
	BBox *line = new BBox(BRect(bounds.left - 5.0, bounds.top, bounds.right + 5.0,
		bounds.top + 1.0), NULL, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP );
	panel->AddChild(line);

	bounds.OffsetBy(0.0, 11.0);
	BButton *cancel = new BButton(bounds, NULL, "Cancel", new BMessage(CANCEL_MSG));
	panel->AddChild(cancel);
	cancel->ResizeToPreferred();

	BButton *ok = new BButton(bounds, NULL, "OK", new BMessage(OK_MSG));
	panel->AddChild(ok, cancel);
	ok->ResizeToPreferred();

	bounds.right = fTo->Frame().right;
	ok->MoveTo(bounds.right - ok->Bounds().Width(), ok->Frame().top);

	bounds = ok->Frame();
	cancel->MoveTo(bounds.left - cancel->Bounds().Width() - 10.0, bounds.top);

	ok->MakeDefault(true);
	ResizeTo(bounds.right + 10.0, bounds.bottom + 10.0);

	BRect winFrame(Frame());
	BRect screenFrame(BScreen().Frame());
	MoveTo((screenFrame.right - winFrame.right) / 2,
		(screenFrame.bottom - winFrame.bottom) / 2);
}


void
JobSetupWindow::UpdateJobMessage()
{
	int32 from = 1;
	int32 to = LONG_MAX;
	if (fAll->Value() == B_CONTROL_OFF) {
		from = atoi(fFrom->Text());
		to = atoi(fTo->Text());
		if (from <= 0) from = 1;
		if (to < from) to = from;
	}

	int32 copies = 1;
	fSetupMsg->RemoveName("copies");
	fSetupMsg->AddInt32("copies", copies);

	fSetupMsg->RemoveName("first_page");
	fSetupMsg->AddInt32("first_page", from);

	fSetupMsg->RemoveName("last_page");
	fSetupMsg->AddInt32("last_page", to);
}


void
JobSetupWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case OK_MSG: {
			UpdateJobMessage();
			Quit(B_OK);
		}	break;

		case CANCEL_MSG: {
			Quit(B_ERROR);
		}	break;

		case ALL_PAGES_MGS : {
			fTo->SetEnabled(false);
			fFrom->SetEnabled(false);
		}	break;

		case RANGE_SELECTION_MSG : {
			fTo->SetEnabled(true);
			fFrom->SetEnabled(true);
		}	break;

		default: {
			BlockingWindow::MessageReceived(msg);
		}	break;
	}
}
