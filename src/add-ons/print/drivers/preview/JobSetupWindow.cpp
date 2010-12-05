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
#include <GridView.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <RadioButton.h>
#include <Screen.h>
#include <TextControl.h>


JobSetupWindow::JobSetupWindow(BMessage *msg, const char * printerName)
	: BlockingWindow(BRect(0, 0, 100, 100), "Job Setup",
		B_TITLED_WINDOW_LOOK,
 		B_MODAL_APP_WINDOW_FEEL, B_NOT_RESIZABLE | B_NOT_MINIMIZABLE |
 			B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
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

	fAll = new BRadioButton("allPages", "Print all pages",
		new BMessage(ALL_PAGES_MGS));
	fAll->SetValue(allPages);

	fRange = new BRadioButton("pagesRange", "Print selected pages:",
		new BMessage(RANGE_SELECTION_MSG));
	fRange->SetValue(!allPages);

	fFrom = new BTextControl("from", "From:", "SomeSpaceHere", NULL);
	fFrom->SetAlignment(B_ALIGN_LEFT, B_ALIGN_RIGHT);
	fFrom->SetEnabled(!allPages);

	fTo = new BTextControl("to", "To:", "", NULL);
	fTo->SetAlignment(B_ALIGN_LEFT, B_ALIGN_RIGHT);
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

	BBox *separator = new BBox("separator");
	separator->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 1));

	BButton *cancel = new BButton("cancel", "Cancel", new BMessage(CANCEL_MSG));

	BButton *ok = new BButton("ok", "OK", new BMessage(OK_MSG));
	ok->MakeDefault(true);

	BGridView* settings = new BGridView();
	BGridLayout* settingsLayout = settings->GridLayout();
	settingsLayout->AddItem(fFrom->CreateLabelLayoutItem(), 0, 0);
	settingsLayout->AddItem(fFrom->CreateTextViewLayoutItem(), 1, 0);
	settingsLayout->AddItem(fTo->CreateLabelLayoutItem(), 0, 1);
	settingsLayout->AddItem(fTo->CreateTextViewLayoutItem(), 1, 1);
	settingsLayout->SetSpacing(0, 0);

	SetLayout(new BGroupLayout(B_VERTICAL));
	AddChild(BGroupLayoutBuilder(B_VERTICAL, 0)
		.Add(fAll)
		.Add(fRange)
		.Add(settings)
		.AddGlue()
		.Add(separator)
		.AddGroup(B_HORIZONTAL, 10, 1.0f)
			.AddGlue()
			.Add(cancel)
			.Add(ok)
		.End()
		.SetInsets(10, 10, 10, 10)
	);


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
		case OK_MSG:
			UpdateJobMessage();
			Quit(B_OK);
			break;

		case CANCEL_MSG:
			Quit(B_ERROR);
			break;

		case ALL_PAGES_MGS:
			fTo->SetEnabled(false);
			fFrom->SetEnabled(false);
			break;

		case RANGE_SELECTION_MSG:
			fTo->SetEnabled(true);
			fFrom->SetEnabled(true);
			break;

		default:
			BlockingWindow::MessageReceived(msg);
			break;
	}
}
