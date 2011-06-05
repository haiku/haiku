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
#include <GridView.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <Message.h>


static const uint32 kCancelMsg = 'cncl';
static const uint32 kProgressMsg = 'prgs'; 


StatusWindow::StatusWindow(int32 passes, int32 pages, PrinterDriver *pd) 
	:
	HWindow(BRect(100, 100, 200, 200), "PDF Writer",
	B_TITLED_WINDOW, 
	B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_CLOSABLE | B_FRAME_EVENTS
	| B_AUTO_UPDATE_SIZE_LIMITS,
	B_CURRENT_WORKSPACE, kCancelMsg) 
{
	fPass = 0;
	fPages = pages;
	fPrinterDriver = pd;
	fPageCount = 0;
	fReportIndex = 0;
	fCloseSem = -1;
	int32 closeOption;

	if (pd->JobMsg()->FindInt32("close_option", &closeOption) != B_OK)
		closeOption = kNever;
	fCloseOption = (CloseOption)closeOption;
	

	fPageLabel = new BStringView("page_text", "Page");

	fPageStatus = new BStatusBar("pageStatus");
	fPageStatus->SetMaxValue(pages * passes);
	fPageStatus->SetBarHeight(12);

	fCancel	= new BButton("cancel", "Cancel", new BMessage(kCancelMsg));

	fReport = new BTextView("report");
	fReport->SetStylable(true);
	fReport->MakeEditable(false);
	fReportScrollView = new BScrollView("report_scroll_viewer",
		fReport, 0, false, true);
	fReportScrollView->Hide();

	SetLayout(new BGroupLayout(B_VERTICAL));
	AddChild(BGroupLayoutBuilder(B_VERTICAL, 0)
		.AddGroup(B_HORIZONTAL)
			.Add(fPageLabel)
			.AddGlue()
		.End()
		.Add(fPageStatus)
		.Add(fReportScrollView)
		.AddStrut(5)
		.AddGroup(B_HORIZONTAL, 10, 1.0f)
			.Add(fCancel)
		.End()
		.SetInsets(10, 10, 10, 10)
	);

	Show();
}


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


void
StatusWindow::UpdateReport()
{
	Report* r = Report::Instance();
	const int32 n = r->CountItems();
	const bool update = fReportIndex < n;
	if (update && fReportIndex == 0) {
		fReportScrollView->SetExplicitMinSize(BSize(600, 400));
		fReportScrollView->Show();
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
StatusWindow::NextPage()
{
	PostMessage(kProgressMsg);
}


void 
StatusWindow::WaitForClose()
{
	fCloseSem = create_sem(0, "close_sem");
	
	Lock();
		Report* r = Report::Instance();
		char b[80];
		sprintf(b, "%d Infos, %d Warnings, %d Errors", r->Count(kInfo),
			r->Count(kWarning), r->Count(kError)); 
		fPageLabel->SetText(b);
		fCancel->SetLabel("Close");
		fCancel->SetEnabled(true);
		UpdateReport();
		
		bool hasErrors = r->Count(kError);
		bool hasErrorsOrWarnings = hasErrors || r->Count(kWarning);
		bool hasErrorsWarningsOrInfo = hasErrorsOrWarnings || r->Count(kInfo);
		
		if (fCloseOption == kAlways
			|| (fCloseOption == kNoErrors && !hasErrors)
			|| (fCloseOption == kNoErrorsOrWarnings && !hasErrorsOrWarnings)
			|| (fCloseOption == kNoErrorsWarningsOrInfo 
			&& !hasErrorsWarningsOrInfo)) {
			PostMessage(kCancelMsg);
		}
	Unlock();
	
	acquire_sem(fCloseSem);
	delete_sem(fCloseSem);
}
