/*****************************************************************************/
// Printers Preference Application.
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2001-2003 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#include "PrintersWindow.h"

#include "AddPrinterDialog.h"
#include "PrinterListView.h"
#include "pr_server.h"
#include "Globals.h"
#include "Messages.h"
#include "JobListView.h"
#include "SpoolFolder.h"

// BeOS API
#include <Box.h>
#include <Button.h>
#include <ListView.h>
#include <ScrollView.h>
#include <Application.h>
#include <FindDirectory.h>

PrintersWindow::PrintersWindow(BRect frame)
	: Inherited(BRect(78.0, 71.0, 561.0, 409.0), "Printers", B_TITLED_WINDOW, B_NOT_H_RESIZABLE)
	, fSelectedPrinter(NULL)
{
	BuildGUI();
}

bool PrintersWindow::QuitRequested()
{
	bool result = Inherited::QuitRequested();
	if (result) {
		be_app->PostMessage(B_QUIT_REQUESTED);
	}

	return result;
}

void PrintersWindow::MessageReceived(BMessage* msg)
{
	switch(msg->what)
	{
		case MSG_PRINTER_SELECTED:
			{
				fSelectedPrinter = fPrinterListView->SelectedItem();
				if (fSelectedPrinter)
				{
					fJobsBox->SetLabel((BString("Print Jobs for ") << fSelectedPrinter->Name()).String());
					fMakeDefault->SetEnabled(true);
					fRemove->SetEnabled(true);
					fJobListView->SetSpoolFolder(fSelectedPrinter->Folder());
				}
				else
				{
					fJobsBox->SetLabel("Print Jobs: No printer selected");
					fMakeDefault->SetEnabled(false);
					fRemove->SetEnabled(false);
					fSelectedPrinter = NULL;
					fJobListView->SetSpoolFolder(NULL);
				}
				UpdateJobButtons();
				UpdatePrinterButtons();
			}
			break;

		case MSG_ADD_PRINTER:
			AddPrinterDialog::Start();
			break;

		case MSG_REMOVE_PRINTER:
			{
				fSelectedPrinter = fPrinterListView->SelectedItem();
				if (fSelectedPrinter) fSelectedPrinter->Remove(fPrinterListView);
			}
			break;

		case MSG_MKDEF_PRINTER:
			{
				int32 prIndex = fPrinterListView->CurrentSelection();
				if (prIndex >= 0)
				{
					PrinterItem* printer = dynamic_cast<PrinterItem*>(fPrinterListView->ItemAt(prIndex));
					BMessenger msgr;
					if (printer != NULL && ::GetPrinterServerMessenger(msgr) == B_OK)
					{
						BMessage setActivePrinter(B_SET_PROPERTY);
						setActivePrinter.AddSpecifier("ActivePrinter");
						setActivePrinter.AddString("data", printer->Name());
						msgr.SendMessage(&setActivePrinter);
						
						fPrinterListView->Invalidate();
						UpdatePrinterButtons();
					}
				}
			}
			break;


		case MSG_CANCEL_JOB: fJobListView->CancelJob();
			break;

		case MSG_RESTART_JOB: fJobListView->RestartJob();
			break;

		case MSG_JOB_SELECTED: UpdateJobButtons();
			break;

		case B_PRINTER_CHANGED:
				// active printer could have been changed
			fPrinterListView->Invalidate();
			break;
			
		default:
			Inherited::MessageReceived(msg);
	}
}

void PrintersWindow::BuildGUI()
{
	const float boxInset = 10.0;
	BRect r(Bounds());

// ------------------------ First of all, create a nice grey backdrop
	BBox * backdrop = new BBox(Bounds(), "backdrop", B_FOLLOW_ALL_SIDES,
						B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP,
						B_PLAIN_BORDER);
	AddChild(backdrop);

// ------------------------ Next, build the printers overview box
	BBox* printersBox = new BBox(BRect(boxInset, boxInset, r.Width()-boxInset, (r.Height()/2) - (boxInset/2)),
		"printersBox", B_FOLLOW_ALL);
	printersBox->SetFont(be_bold_font);
	printersBox->SetLabel("Printers:");
	backdrop->AddChild(printersBox);

		// Width of largest button
	float maxWidth = 0;

		// Add Button
	BButton* addButton = new BButton(BRect(5,5,5,5), "add", "Add " B_UTF8_ELLIPSIS, new BMessage(MSG_ADD_PRINTER), B_FOLLOW_RIGHT);
	printersBox->AddChild(addButton);
	addButton->ResizeToPreferred();

	maxWidth = addButton->Bounds().Width();

		// Remove button
	fRemove = new BButton(BRect(5,30,5,30), "remove", "Remove", new BMessage(MSG_REMOVE_PRINTER), B_FOLLOW_RIGHT);
	printersBox->AddChild(fRemove);
	fRemove->ResizeToPreferred();

	if (fRemove->Bounds().Width() > maxWidth)
		maxWidth = fRemove->Bounds().Width();

		// Make Default button
	fMakeDefault = new BButton(BRect(5,60,5,60), "default", "Make Default", new BMessage(MSG_MKDEF_PRINTER), B_FOLLOW_RIGHT);
	printersBox->AddChild(fMakeDefault);
	fMakeDefault->ResizeToPreferred();

	if (fMakeDefault->Bounds().Width() > maxWidth)
		maxWidth = fMakeDefault->Bounds().Width();

		// Resize all buttons to maximum width and align them to the right
	float xPos = printersBox->Bounds().Width()-boxInset-maxWidth;
	addButton->MoveTo(xPos, boxInset +5);
	addButton->ResizeTo(maxWidth, addButton->Bounds().Height());

	fRemove->MoveTo(xPos, boxInset + addButton->Bounds().Height() + boxInset +5);
	fRemove->ResizeTo(maxWidth, fRemove->Bounds().Height());

	fMakeDefault->MoveTo(xPos, boxInset + addButton->Bounds().Height() +
									boxInset + fRemove->Bounds().Height() +
									boxInset +5);
	fMakeDefault->ResizeTo(maxWidth, fMakeDefault->Bounds().Height());

		// Disable all selection-based buttons
	fRemove->SetEnabled(false);
	fMakeDefault->SetEnabled(false);

		// Create listview with scroller
	BRect listBounds(boxInset, boxInset+5, fMakeDefault->Frame().left - boxInset - B_V_SCROLL_BAR_WIDTH,
					printersBox->Bounds().Height()-boxInset);
	fPrinterListView = new PrinterListView(listBounds);
	BScrollView* pscroller = new BScrollView("printer_scroller", fPrinterListView,
								B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS, false, true, B_FANCY_BORDER);
	printersBox->AddChild(pscroller);

// ------------------------ Lastly, build the jobs overview box
	fJobsBox = new BBox(BRect(boxInset, (r.Height()/2)+(boxInset/2), Bounds().Width()-10, Bounds().Height() - boxInset),
		"jobsBox", B_FOLLOW_LEFT_RIGHT+B_FOLLOW_BOTTOM);
	fJobsBox->SetFont(be_bold_font);
	fJobsBox->SetLabel("Print Jobs: No printer selected");
	backdrop->AddChild(fJobsBox);

		// Cancel Job Button
	BButton* cancelButton = new BButton(BRect(5,5,5,5), "cancel", "Cancel Job", new BMessage(MSG_CANCEL_JOB), B_FOLLOW_RIGHT+B_FOLLOW_TOP);
	fJobsBox->AddChild(cancelButton);
	cancelButton->ResizeToPreferred();
	fCancel = cancelButton;

	maxWidth = cancelButton->Bounds().Width();

		// Restart Job button
	BButton* restartButton = new BButton(BRect(5,30,5,30), "restart", "Restart Job", new BMessage(MSG_RESTART_JOB), B_FOLLOW_RIGHT+B_FOLLOW_TOP);
	fJobsBox->AddChild(restartButton);
	restartButton->ResizeToPreferred();
	fRestart = restartButton;

	if (restartButton->Bounds().Width() > maxWidth)
		maxWidth = restartButton->Bounds().Width();

		// Resize all buttons to maximum width and align them to the right
	xPos = fJobsBox->Bounds().Width()-boxInset-maxWidth;
	cancelButton->MoveTo(xPos, boxInset +5);
	cancelButton->ResizeTo(maxWidth, cancelButton->Bounds().Height());

	restartButton->MoveTo(xPos, boxInset + cancelButton->Bounds().Height() + boxInset +5);
	restartButton->ResizeTo(maxWidth, restartButton->Bounds().Height());

		// Disable all selection-based buttons
	cancelButton->SetEnabled(false);
	restartButton->SetEnabled(false);

		// Create listview with scroller
	listBounds = BRect(boxInset, boxInset+5, cancelButton->Frame().left - boxInset - B_V_SCROLL_BAR_WIDTH,
					fJobsBox->Bounds().Height()-boxInset);
	fJobListView = new JobListView(listBounds);
	BScrollView* jscroller = new BScrollView("jobs_scroller", fJobListView,
								B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS, false, true, B_FANCY_BORDER);
	fJobsBox->AddChild(jscroller);

		// Determine min width
	float width;
	width = (jscroller->Bounds().Width() < pscroller->Bounds().Width()) ?
				jscroller->Bounds().Width() : pscroller->Bounds().Width();

		// Resize boxes to the same size
	jscroller->ResizeTo(width, jscroller->Bounds().Height());
	pscroller->ResizeTo(width, pscroller->Bounds().Height());
	
	SetSizeLimits(Bounds().Width(), Bounds().Width(), Bounds().Height(), 20000);	
}

bool PrintersWindow::IsSelected(PrinterItem* printer) {
	return fSelectedPrinter && fSelectedPrinter == printer;
}

void PrintersWindow::AddJob(SpoolFolder* folder, Job* job) {
	if (IsSelected(folder->Item())) {
		fJobListView->AddJob(job);
	}
	fPrinterListView->UpdateItem(folder->Item());
	UpdatePrinterButtons();
}

void PrintersWindow::RemoveJob(SpoolFolder* folder, Job* job) {
	if (IsSelected(folder->Item())) {
		fJobListView->RemoveJob(job);
	}
	fPrinterListView->UpdateItem(folder->Item());
	UpdatePrinterButtons();
}

void PrintersWindow::UpdateJob(SpoolFolder* folder, Job* job) {
	if (IsSelected(folder->Item())) {
		fJobListView->UpdateJob(job);
		UpdateJobButtons();
	}
	fPrinterListView->UpdateItem(folder->Item());
	UpdatePrinterButtons();
}

void PrintersWindow::UpdatePrinterButtons() {
	PrinterItem* item = fPrinterListView->SelectedItem();
	fRemove->SetEnabled(item && !item->HasPendingJobs());
	fMakeDefault->SetEnabled(item && !item->IsActivePrinter());
}
	
void PrintersWindow::UpdateJobButtons() {
	JobItem* item = fJobListView->SelectedItem();
	if (item != NULL) {
		Job* job = item->GetJob();
		fCancel->SetEnabled(job->Status() != kProcessing);
		fRestart->SetEnabled(job->Status() == kFailed);
	} else {
		fCancel->SetEnabled(false);
		fRestart->SetEnabled(false);
	}
}
