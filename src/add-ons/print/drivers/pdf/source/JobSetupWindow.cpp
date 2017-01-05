/*

PDF Writer printer driver.

Copyright (c) 2001-2003 OpenBeOS.
Copyright (c) 2010 Haiku, Inc.

Authors:
	Philippe Houdoin
	Simon Gauvin
	Michael Pfeiffer

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#include <GridView.h>
#include <InterfaceKit.h>
#include <LayoutBuilder.h>
#include <SeparatorView.h>
#include <SupportKit.h>
#include <stdlib.h>

#include "PrintUtils.h"

#include "PrinterDriver.h"
#include "JobSetupWindow.h"
#include "DocInfoWindow.h"

static const char* includeKeys[] = {
	"doc_info",
#if HAVE_FULLVERSION_PDF_LIB
	"master_password", "user_password", "permissions",
#endif
	NULL
};

// --------------------------------------------------
JobSetupWindow::JobSetupWindow(BMessage *msg, const char * printerName)
	:	HWindow(BRect(0, 0, 100, 100), "Job setup",
			B_TITLED_WINDOW_LOOK,
			B_MODAL_APP_WINDOW_FEEL,
			B_NOT_RESIZABLE | B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE
				| B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE)
{
	fSetupMsg = msg;
	fExitSem = create_sem(0, "JobSetup");
	fResult	= B_ERROR;

	if (printerName) {
		BString	title;
		title << printerName << " job setup";
		SetTitle(title.String());
		fPrinterName = printerName;
	}

	// PrinterDriver ensures that property exists
	int32 firstPage;
	fSetupMsg->FindInt32("first_page", &firstPage);
	int32 lastPage;
	fSetupMsg->FindInt32("last_page",  &lastPage);

	BMessage doc_info;
	if (fSetupMsg->FindMessage("doc_info", &doc_info) != B_OK) {
		// default fields
		doc_info.AddString("Author", "");
		doc_info.AddString("Subject", "");
		doc_info.AddString("Keywords", "");
		fSetupMsg->AddMessage("doc_info", &doc_info);
	}
	AddFields(&fDocInfo, fSetupMsg, NULL, includeKeys);

	bool allPages = firstPage == 1 && lastPage == MAX_INT32;

	fAll = new BRadioButton("allPages", "Print all pages",
		new BMessage(ALL_PAGES_MGS));
	fAll->SetValue(allPages);

	fRange = new BRadioButton("pagesRange", "Print pages:",
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

	BButton* documentInfo = new BButton("documentInfo",
		"Document"  B_UTF8_ELLIPSIS,
		new BMessage(DOC_INFO_MSG));

	BButton* cancel = new BButton("cancel", "Cancel", new BMessage(CANCEL_MSG));

	BButton* ok = new BButton("ok", "OK", new BMessage(OK_MSG));
	ok->MakeDefault(true);

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(fAll)
		.Add(fRange)
		.AddGrid(B_USE_SMALL_SPACING, B_USE_SMALL_SPACING)
			.SetInsets(B_USE_SMALL_SPACING, 0, B_USE_SMALL_SPACING, 0)
			.Add(fFrom->CreateLabelLayoutItem(), 0, 0)
			.Add(fFrom->CreateTextViewLayoutItem(), 1, 0)
			.Add(fTo->CreateLabelLayoutItem(), 0, 1)
			.Add(fTo->CreateTextViewLayoutItem(), 1, 1)
		.End()
		.AddGlue()
		.Add(new BSeparatorView(B_HORIZONTAL, B_FANCY_BORDER))
		.AddGroup(B_HORIZONTAL)
			.Add(documentInfo)
			.AddGlue()
			.Add(cancel)
			.Add(ok)
		.End();
}


// --------------------------------------------------
void
JobSetupWindow::UpdateJobMessage()
{
	int32 copies = 1;

	int32 from;
	int32 to;
	if (fAll->Value() == B_CONTROL_ON) {
		from = 1; to = MAX_INT32;
	} else {
		from = atoi(fFrom->Text());
		to   = atoi(fTo->Text());
		if (from <= 0) from = 1;
		if (to < from) to = from;
	}

	fSetupMsg->ReplaceInt32("copies", copies);
	fSetupMsg->ReplaceInt32("first_page", from);
	fSetupMsg->ReplaceInt32("last_page", to);
	AddFields(fSetupMsg, &fDocInfo);
}


// --------------------------------------------------
JobSetupWindow::~JobSetupWindow()
{
	delete_sem(fExitSem);
}


// --------------------------------------------------
bool
JobSetupWindow::QuitRequested()
{
	release_sem(fExitSem);
	return true;
}


// --------------------------------------------------
void
JobSetupWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case OK_MSG:
			UpdateJobMessage();
			fResult = B_OK;
			release_sem(fExitSem);
			break;

		case CANCEL_MSG:
			release_sem(fExitSem);
			break;

		case ALL_PAGES_MGS:
			fTo->SetEnabled(false);
			fFrom->SetEnabled(false);
			break;

		case RANGE_SELECTION_MSG:
			fTo->SetEnabled(true);
			fFrom->SetEnabled(true);
			break;

		case DOC_INFO_MSG:
			(new DocInfoWindow(&fDocInfo))->Show();
			break;

		default:
			inherited::MessageReceived(msg);
			break;
	}
}


// --------------------------------------------------
status_t
JobSetupWindow::Go()
{
	MoveTo(300,300);
	Show();

	while (acquire_sem(fExitSem) == B_INTERRUPTED) {
	}

	// Cache the value as after Quit() this object is deleted.
	status_t result = fResult;
	if (Lock())
		Quit();

	return result;
}


