/*

PDF Writer printer driver.

Copyright (c) 2001-2003 OpenBeOS. 

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

#include <InterfaceKit.h>
#include <SupportKit.h>
#include <stdlib.h>

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
	:	HWindow(BRect(0, 0, 320, 160), "Job Setup", B_TITLED_WINDOW_LOOK,
 			B_MODAL_APP_WINDOW_FEEL, B_NOT_RESIZABLE | B_NOT_MINIMIZABLE |
 			B_NOT_ZOOMABLE)
{
	fSetupMsg	= msg;
	fExitSem 	= create_sem(0, "JobSetup");
	fResult 	= B_ERROR;
	
	if (printerName) {
		BString	title;
		title << printerName << " Job Setup";
		SetTitle(title.String());
		fPrinterName = printerName;
	}
	
	// ---- Ok, build a default job setup user interface
	BRect			r;
	BBox			*panel;
	BBox			*line;
	BButton	 		*ok;
	BButton			*cancel;
	BStringView		*sv;
	float			x, y, w, h;
	float			indent;
	int32           copies;
	int32           firstPage;
	int32           lastPage;
	bool            allPages;
	char            buffer[80];
	
	// PrinterDriver ensures that property exists
	fSetupMsg->FindInt32("copies",     &copies);
	fSetupMsg->FindInt32("first_page", &firstPage);
	fSetupMsg->FindInt32("last_page",  &lastPage);
	BMessage doc_info;
	if (B_OK != fSetupMsg->FindMessage("doc_info", &doc_info)) {
		// default fields
		doc_info.AddString("Author", "");
		doc_info.AddString("Subject", "");
		doc_info.AddString("Keywords", "");
		fSetupMsg->AddMessage("doc_info", &doc_info);
	}
	AddFields(&fDocInfo, fSetupMsg, true, NULL, includeKeys);
	
	allPages = firstPage == 1 && lastPage == MAX_INT32;

	r = Bounds();

	// add a *dialog* background
	panel = new BBox(r, "top_panel", B_FOLLOW_ALL, 
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP,
		B_PLAIN_BORDER);

	const int kMargin = 6;

	//const char *kCopiesLabel				= "Copies:";
	const char *kCopiesLabelExtraSpace		= "Copies:##";
	const char *kPagesRangeLabel			= "Pages:";
	const char *kAllPagesLabel				= "All";
	const char *kPagesRangeSelectionLabel	= "";
	const char *kFromLabel					= "From:";
	const char *kFromLabelExtraSpace		= "From:##";
	const char *kToLabel					= "To:";
	const char *kToLabelExtraSpace			= "To:##";

	r = panel->Bounds();

	x = r.left + kMargin;
	y = r.top + kMargin;
	
	
	// add a "copies" input field

/* Simon: temporarily removed this code
	sprintf(buffer, "%d", (int)copies);
	fCopies = new BTextControl(BRect(x, y, x+100, y+20), "copies", kCopiesLabel,
								buffer, new BMessage(NB_COPIES_MSG));
	fCopies->SetAlignment(B_ALIGN_LEFT, B_ALIGN_RIGHT);
	fCopies->ResizeToPreferred();
	fCopies->GetPreferredSize(&w, &h);
	panel->AddChild(fCopies);
	
	y += h + kMargin;	// "new line"
*/
	// add a "pages" label
	sv = new BStringView(BRect(x, y, x+100, y+20), "pages_range", kPagesRangeLabel);
	panel->AddChild(sv);
	sv->ResizeToPreferred();
	sv->GetPreferredSize(&w, &h);

	// align "copies" textcontrol field on the "allPages" radiobutton bellow...
	indent = be_plain_font->StringWidth(kCopiesLabelExtraSpace);
	w += kMargin;
	if ( w > indent )
		indent = w;
	// fCopies->SetDivider(indent);

	x += indent;

	// add a "all" radiobutton
	fAll = new BRadioButton(BRect(x, y, x+100, y+20), "all_pages", kAllPagesLabel,
						new BMessage(ALL_PAGES_MGS));
	fAll->ResizeToPreferred();
	fAll->GetPreferredSize(&w, &h);
	fAll->SetValue(allPages);
	panel->AddChild(fAll);

	y += h + kMargin;	// "new line"

	// add a range selection raddiobutton
	fRange = new BRadioButton(BRect(x, y, x+100, y+20), "pages_range_selection", kPagesRangeSelectionLabel,
						new BMessage(RANGE_SELECTION_MSG));
	fRange->ResizeToPreferred();
	fRange->GetPreferredSize(&w, &h);
	fRange->SetValue(!allPages);
	panel->AddChild(fRange);

	x += w + kMargin;
	
	// add a "from" field
	if (allPages) { 
		buffer[0] = 0;
	} else {
		sprintf(buffer, "%d", (int)firstPage);
	}
	fFrom = new BTextControl(BRect(x, y, x+100, y+20), "from_field", kFromLabel, buffer,
							new BMessage(RANGE_FROM_MSG));
	fFrom->SetAlignment(B_ALIGN_LEFT, B_ALIGN_RIGHT);
	fFrom->SetDivider(be_plain_font->StringWidth(kFromLabelExtraSpace));
	fFrom->ResizeToPreferred();
	fFrom->GetPreferredSize(&w, &h);
	panel->AddChild(fFrom);

	x += w + kMargin;
	
	// add a "to" field
	if (allPages) {
		buffer[0] = 0;
	} else {
		sprintf(buffer, "%d", (int)lastPage);
	} 
	fTo = new BTextControl(BRect(x, y, x+100, y+20), "to_field", kToLabel, buffer,
							new BMessage(RANGE_TO_MSG));
	fTo->SetAlignment(B_ALIGN_LEFT, B_ALIGN_RIGHT);
	fTo->SetDivider(be_plain_font->StringWidth(kToLabelExtraSpace));
	fTo->ResizeToPreferred();
	fTo->GetPreferredSize(&w, &h);
	panel->AddChild(fTo);

	y += h + kMargin + kMargin;	// "new line"
	x = r.left + kMargin;

	// add a separator line...
	line = new BBox(BRect(r.left, y - 1, r.right, y), NULL,
						 B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP );
	panel->AddChild(line);

	y += 2 + kMargin + kMargin;	// "new line"

	// add a "OK" button, and make it default
	ok 	= new BButton(BRect(x, y, x+100, y+20), NULL, "OK", new BMessage(OK_MSG), B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	ok->MakeDefault(true);
	ok->ResizeToPreferred();
	ok->GetPreferredSize(&w, &h);
	x = r.right - w - kMargin;
	ok->MoveTo(x, ok->Frame().top);	// put the ok bottom at bottom right corner
	panel->AddChild(ok);

	// add a "Cancel" button	
	cancel 	= new BButton(BRect(x, y, x + 100, y + 20), NULL, "Cancel", new BMessage(CANCEL_MSG), B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	cancel->ResizeToPreferred();
	cancel->GetPreferredSize(&w, &h);
	cancel->MoveTo(x - w - kMargin, y);	// put cancel button left next the ok button
	panel->AddChild(cancel);

	// add a "DocInfo" button	
	BButton *button = new BButton(r, NULL, "Doc Info", new BMessage(DOC_INFO_MSG), 
		B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	button->GetPreferredSize(&w, &h);
	button->ResizeToPreferred();
	button->MoveTo(8, y);
	panel->AddChild(button);

	// Finally, add our panel to window
	AddChild(panel);
	
	// Auto resize window
	ResizeTo(ok->Frame().right + kMargin, ok->Frame().bottom + kMargin);
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

		case RANGE_FROM_MSG:
		case RANGE_TO_MSG:
			fRange->SetValue(B_CONTROL_ON);
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
	acquire_sem(fExitSem);
	if (Lock()) Quit();
	return fResult;
}


