/*

PDF Writer printer driver.

Copyright (c) 2001 OpenBeOS. 

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

#include "StatusWindow.h"
#include "Report.h"
#include <Message.h>
#include <Box.h>

static const uint32 kCancelMsg = 'cncl';
static const uint32 kProgressMsg = 'prgs'; 

// --------------------------------------------------
StatusWindow::StatusWindow(int32 passes, int32 pages, PrinterDriver *pd) 
	:	HWindow(BRect(100, 100, 700, 600), "PDF Writer", 
			B_TITLED_WINDOW, 
			B_NOT_RESIZABLE|B_NOT_ZOOMABLE|B_NOT_CLOSABLE|B_FRAME_EVENTS,
			B_CURRENT_WORKSPACE, kCancelMsg) 
{
	fPass = 0;
	fPages = pages;
	fPrinterDriver = pd;
	fPageCount = 0;
	fReportIndex = 0;
	fCloseSem = -1;
	int32 closeOption;

	if (pd->JobMsg()->FindInt32("close_option", &closeOption) != B_OK) closeOption = kNever;
	fCloseOption = (CloseOption)closeOption;
	
	BRect r(0, 0, Frame().Width(), Frame().Height());

	// view for the background color
	BView *fPanel = new BBox(r, "top_panel", B_FOLLOW_ALL, 
					B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP,
					B_PLAIN_BORDER);
	AddChild(fPanel);

	r.Set(10, 12, Frame().Width()-5, 22);
	fPageLabel = new BStringView(r, "page_text", "Page", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	fPanel->AddChild(fPageLabel);

	r.Set(10, 15, 300-10, 10);
	fPageStatus = new BStatusBar(r, "pageStatus");
	fPageStatus->SetMaxValue(pages * passes);
	fPageStatus->SetBarHeight(12);
	fPanel->AddChild(fPageStatus);

	// add a "Cancel" button
	int32 x = 110;
	int32 y = 55;
	fCancel 	= new BButton(BRect(x, y, x + 100, y + 20), NULL, "Cancel", 
				new BMessage(kCancelMsg), B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS);
	fCancel->ResizeToPreferred();
	fPanel->AddChild(fCancel);

	BRect b(0, 90, Bounds().right-B_V_SCROLL_BAR_WIDTH, Bounds().bottom);
	BRect t(b);
	t.OffsetTo(0, 0);
	fReport = new BTextView(b, "", t, B_FOLLOW_TOP_BOTTOM | B_FOLLOW_LEFT_RIGHT);
	fReport->SetStylable(true);
	fReport->MakeEditable(false);
	fPanel->AddChild(new BScrollView("", fReport, B_FOLLOW_ALL, 0, false, true));

	ResizeTo(300, 85);

	Show();
}

// --------------------------------------------------
void 
StatusWindow::MessageReceived(BMessage *msg) 
{
	switch (msg->what) {
		case kCancelMsg:
			if (fCloseSem == -1) {
				fPrinterDriver->StopPrinting();
				fCancel->SetEnabled(false);
			} else {
				fCancel->SetEnabled(false);
				release_sem(fCloseSem);
			}
			break;
			
		case kProgressMsg:
			fPage = "";
			if (fPass == 0) 
				fPage << "Collecting Patterns Page: " << fPageCount;
			else
				fPage << "Generating Page: " << fPageCount;
			fPageCount ++;
			if (fPageCount == fPages) {
				fPass ++;
				fPageCount = 0;
			}
			fPageLabel->SetText(fPage.String());
			fPageStatus->Update(1);
			UpdateReport();
			break;
		default:
			inherited::MessageReceived(msg);
	}
}


void StatusWindow::UpdateReport() {
	Report* r = Report::Instance();
	const int32 n = r->CountItems();
	const bool update = fReportIndex < n;
	if (update && fReportIndex == 0) {
		SetFlags(Flags() & ~(B_NOT_RESIZABLE));
		ResizeTo(600, 500);
	}
	while (fReportIndex < n) {
		ReportRecord* rr = r->ItemAt(fReportIndex ++);
		const char* label = NULL;
		rgb_color color = {0, 0, 0, 255};
		switch (rr->Kind()) {
			case kInfo:    label = "Info";    color.green = 255; break;
			case kWarning: label = "Warning"; color.red = 255; color.green=255; break;
			case kError:   label = "Error";   color.red = 255; break;
			case kDebug:   label = "Debug";   color.blue = 255; break;
		}

		int32 cur = fReport->TextLength()-1;
		fReport->Insert(fReport->TextLength(), label, strlen(label));
		int32 end = fReport->TextLength();
		fReport->SetFontAndColor(cur, end, NULL, 0, &color);

		char page[80];
		if (rr->Page() > 0) {
			sprintf(page, " (Page %d)", (int)rr->Page());
			int32 len = strlen(page);
			fReport->Insert(fReport->TextLength(), page, len);
		}

		fReport->Insert(fReport->TextLength(), ": ", 2);

		fReport->Insert(fReport->TextLength(), rr->Desc(), strlen(rr->Desc()));		
		fReport->Insert(fReport->TextLength(), "\n", 1);
	}
	
	if (update) {
		fReport->ScrollToOffset(fReport->TextLength());
		fReport->Invalidate();
	}
}

void
StatusWindow::NextPage() {
	PostMessage(kProgressMsg);
}

void 
StatusWindow::WaitForClose() {
	fCloseSem = create_sem(0, "close_sem");
	
	Lock();
		Report* r = Report::Instance();
		char b[80];
		sprintf(b, "%d Infos, %d Warnings, %d Errors", r->Count(kInfo), r->Count(kWarning), r->Count(kError)); 
		fPageLabel->SetText(b);
		fCancel->SetLabel("Close");
		fCancel->SetEnabled(true);
		UpdateReport();
		
		bool hasErrors = r->Count(kError);
		bool hasErrorsOrWarnings = hasErrors || r->Count(kWarning);
		bool hasErrorsWarningsOrInfo = hasErrorsOrWarnings || r->Count(kInfo);
		
		if (fCloseOption == kAlways || 
			fCloseOption == kNoErrors && !hasErrors ||
			fCloseOption == kNoErrorsOrWarnings && !hasErrorsOrWarnings ||
			fCloseOption == kNoErrorsWarningsOrInfo && !hasErrorsWarningsOrInfo) {
			PostMessage(kCancelMsg);
		}
	Unlock();
	
	acquire_sem(fCloseSem);
	delete_sem(fCloseSem);
}
