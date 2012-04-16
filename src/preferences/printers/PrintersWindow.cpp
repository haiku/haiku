/*
 * Copyright 2001-2011, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 *		Philippe Houdoin
 */


#include "PrintersWindow.h"

#include <stdio.h>

#include <Application.h>
#include <Button.h>
#include <Catalog.h>
#include <FindDirectory.h>
#include <GroupLayout.h>
#include <Layout.h>
#include <ListView.h>
#include <Locale.h>
#include <PrintJob.h>
#include <ScrollView.h>

#include "pr_server.h"
#include "AddPrinterDialog.h"
#include "Globals.h"
#include "JobListView.h"
#include "Messages.h"
#include "PrinterListView.h"
#include "TestPageView.h"
#include "SpoolFolder.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PrintersWindow"


class TestPageWindow : public BWindow {
public:
						TestPageWindow(BPrintJob* job, PrinterItem* printer);
	virtual				~TestPageWindow();

			void 		MessageReceived(BMessage* message);
private:
		BPrintJob*		fJob;
		TestPageView*	fTestPage;
};


TestPageWindow::TestPageWindow(BPrintJob* job, PrinterItem* printer)
	: BWindow(job->PaperRect().OffsetByCopy(-20000, -20000), B_TRANSLATE("Test Page"),
		B_TITLED_WINDOW, 0), fJob(job)
{
	fTestPage = new TestPageView(job->PrintableRect(), printer);

	// SetLayout(new BGroupLayout(B_VERTICAL));
	AddChild(fTestPage);
}


TestPageWindow::~TestPageWindow()
{
	delete fJob;
}


void
TestPageWindow::MessageReceived(BMessage* message)
{
	if (message->what != kMsgPrintTestPage) {
		BWindow::MessageReceived(message);
		return;
	}

	fJob->BeginJob();

	fJob->DrawView(fTestPage, fTestPage->Bounds(), B_ORIGIN);
	fJob->SpoolPage();

	if (!fJob->CanContinue())
		return;

	fJob->CommitJob();

	Quit();
}


// #pragma mark PrintersWindow main class


PrintersWindow::PrintersWindow(BRect frame)
	:
	BWindow(BRect(78, 71, 761, 509), B_TRANSLATE_SYSTEM_NAME("Printers"),
		B_TITLED_WINDOW, 0),
	fSelectedPrinter(NULL),
	fAddingPrinter(false)
{
	_BuildGUI();
}


bool
PrintersWindow::QuitRequested()
{
	bool result = Inherited::QuitRequested();
	if (result)
		be_app->PostMessage(B_QUIT_REQUESTED);

	return result;
}


void
PrintersWindow::MessageReceived(BMessage* msg)
{
	switch(msg->what) {
		case kMsgPrinterSelected:
		{
			fSelectedPrinter = fPrinterListView->SelectedItem();
			if (fSelectedPrinter) {
				fJobsBox->SetLabel((BString(B_TRANSLATE("Print jobs for "))
					<< fSelectedPrinter->Name()).String());
				fMakeDefault->SetEnabled(true);
				fRemove->SetEnabled(true);
				fJobListView->SetSpoolFolder(fSelectedPrinter->Folder());
			} else {
				fJobsBox->SetLabel(
					B_TRANSLATE("Print jobs: No printer selected"));
				fMakeDefault->SetEnabled(false);
				fRemove->SetEnabled(false);
				fSelectedPrinter = NULL;
				fJobListView->SetSpoolFolder(NULL);
			}
			_UpdateJobButtons();
			_UpdatePrinterButtons();
			break;
		}

		case kMsgAddPrinter:
			if (!fAddingPrinter) {
				fAddingPrinter = true;
				new AddPrinterDialog(this);
			}
			break;

		case kMsgAddPrinterClosed:
			fAddingPrinter = false;
			break;

		case kMsgRemovePrinter:
		{
			fSelectedPrinter = fPrinterListView->SelectedItem();
			if (fSelectedPrinter)
				fSelectedPrinter->Remove(fPrinterListView);
			break;
		}

		case kMsgMakeDefaultPrinter:
		{
			PrinterItem* printer = fPrinterListView->SelectedItem();
			if (printer && printer == fPrinterListView->ActivePrinter())
				break;
			BMessenger msgr;
			if (printer && GetPrinterServerMessenger(msgr) == B_OK) {
				BMessage setActivePrinter(B_SET_PROPERTY);
				setActivePrinter.AddSpecifier("ActivePrinter");
				setActivePrinter.AddString("data", printer->Name());
				msgr.SendMessage(&setActivePrinter);
				_UpdatePrinterButtons();
			}
			break;
		}

		case kMsgPrintTestPage:
		{
			fSelectedPrinter = fPrinterListView->SelectedItem();
			if (fSelectedPrinter)
				PrintTestPage(fSelectedPrinter);
			break;
		}

		case kMsgCancelJob:
			fJobListView->CancelJob();
			break;

		case kMsgRestartJob:
			fJobListView->RestartJob();
			break;

		case kMsgJobSelected:
			_UpdateJobButtons();
			break;

		case B_PRINTER_CHANGED:
		{
			// active printer could have been changed, even outside of prefs
			BString activePrinterName(ActivePrinterName());
			PrinterItem* item = fPrinterListView->ActivePrinter();
			if (item && item->Name() != activePrinterName)
				fPrinterListView->UpdateItem(item);

			for (int32 i = 0; i < fPrinterListView->CountItems(); ++i) {
				item = dynamic_cast<PrinterItem*>(fPrinterListView->ItemAt(i));
				if (item && item->Name() == activePrinterName) {
					fPrinterListView->UpdateItem(item);
					fPrinterListView->SetActivePrinter(item);
					break;
				}
			}
		}	break;

		default:
			Inherited::MessageReceived(msg);
	}
}


void
PrintersWindow::PrintTestPage(PrinterItem* printer)
{
	BPrintJob* job = new BPrintJob(B_TRANSLATE("Test Page"));
	job->ConfigPage();

	// job->ConfigJob();

	BMessage* settings = job->Settings();
	if (settings == NULL) {
		delete job;	
		return;
	}

	// enforce job config properties
	settings->AddInt32("copies", 1);
	settings->AddInt32("first_page", 1);
	settings->AddInt32("last_page", -1);

	BWindow* win = new TestPageWindow(job, printer);
	win->Show();
	win->PostMessage(kMsgPrintTestPage);
}


void
PrintersWindow::AddJob(SpoolFolder* folder, Job* job)
{
	if (_IsSelected(folder->Item()))
		fJobListView->AddJob(job);
	fPrinterListView->UpdateItem(folder->Item());
	_UpdatePrinterButtons();
}


void
PrintersWindow::RemoveJob(SpoolFolder* folder, Job* job)
{
	if (_IsSelected(folder->Item()))
		fJobListView->RemoveJob(job);
	fPrinterListView->UpdateItem(folder->Item());
	_UpdatePrinterButtons();
}


void
PrintersWindow::UpdateJob(SpoolFolder* folder, Job* job)
{
	if (_IsSelected(folder->Item())) {
		fJobListView->UpdateJob(job);
		_UpdateJobButtons();
	}
	fPrinterListView->UpdateItem(folder->Item());
	_UpdatePrinterButtons();
}


// #pragma mark -


void
PrintersWindow::_BuildGUI()
{
	const float boxInset = 10.0;
	BRect r(Bounds());

// ------------------------ First of all, create a nice grey backdrop
	BBox * backdrop = new BBox(Bounds(), "backdrop", B_FOLLOW_ALL_SIDES,
						B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP,
						B_PLAIN_BORDER);
	AddChild(backdrop);

// ------------------------ Next, build the printers overview box
	BBox* printersBox = new BBox(BRect(boxInset, boxInset,
		r.Width() - boxInset, (r.Height()/2) - (boxInset/2)),
		"printersBox", B_FOLLOW_ALL);
	printersBox->SetFont(be_bold_font);
	printersBox->SetLabel(B_TRANSLATE("Printers"));
	backdrop->AddChild(printersBox);

		// Width of largest button
	float maxWidth = 0;

		// Add Button
	BButton* addButton = new BButton(BRect(5,5,5,5), "add",
		B_TRANSLATE("Add …"), new BMessage(kMsgAddPrinter), B_FOLLOW_RIGHT);
	printersBox->AddChild(addButton);
	addButton->ResizeToPreferred();

	maxWidth = addButton->Bounds().Width();

		// Remove button
	fRemove = new BButton(BRect(5,30,5,30), "remove",
		B_TRANSLATE("Remove"), new BMessage(kMsgRemovePrinter),
		B_FOLLOW_RIGHT);
	printersBox->AddChild(fRemove);
	fRemove->ResizeToPreferred();

	if (fRemove->Bounds().Width() > maxWidth)
		maxWidth = fRemove->Bounds().Width();

		// Make Default button
	fMakeDefault = new BButton(BRect(5,60,5,60), "default",
		B_TRANSLATE("Make default"), new BMessage(kMsgMakeDefaultPrinter),
		B_FOLLOW_RIGHT);
	printersBox->AddChild(fMakeDefault);
	fMakeDefault->ResizeToPreferred();

	if (fMakeDefault->Bounds().Width() > maxWidth)
		maxWidth = fMakeDefault->Bounds().Width();

		// Print Test Page button
	fPrintTestPage = new BButton(BRect(5,60,5,60), "print_test_page",
		B_TRANSLATE("Print test page"), new BMessage(kMsgPrintTestPage),
		B_FOLLOW_RIGHT);
	printersBox->AddChild(fPrintTestPage);
	fPrintTestPage->ResizeToPreferred();

	if (fPrintTestPage->Bounds().Width() > maxWidth)
		maxWidth = fPrintTestPage->Bounds().Width();


		// Resize all buttons to maximum width and align them to the right
	float xPos = printersBox->Bounds().Width() - boxInset - maxWidth;
	addButton->MoveTo(xPos, boxInset + 8);
	addButton->ResizeTo(maxWidth, addButton->Bounds().Height());

	fRemove->MoveTo(xPos,
		boxInset + addButton->Bounds().Height() + boxInset + 8);
	fRemove->ResizeTo(maxWidth, fRemove->Bounds().Height());

	fMakeDefault->MoveTo(xPos, boxInset + addButton->Bounds().Height() +
		boxInset + fRemove->Bounds().Height() + boxInset + 8);
	fMakeDefault->ResizeTo(maxWidth, fMakeDefault->Bounds().Height());

	fPrintTestPage->MoveTo(xPos, boxInset + addButton->Bounds().Height() +
		boxInset + fRemove->Bounds().Height() +
		boxInset + fMakeDefault->Bounds().Height() + boxInset + 8);
	fPrintTestPage->ResizeTo(maxWidth, fPrintTestPage->Bounds().Height());


		// Disable all selection-based buttons
	fRemove->SetEnabled(false);
	fMakeDefault->SetEnabled(false);
	fPrintTestPage->SetEnabled(false);

		// Create listview with scroller
	BRect listBounds(boxInset, boxInset + 8,
		fMakeDefault->Frame().left - boxInset - B_V_SCROLL_BAR_WIDTH,
		printersBox->Bounds().Height()- boxInset - 3);
	fPrinterListView = new PrinterListView(listBounds);
	BScrollView* pscroller = new BScrollView("printer_scroller", fPrinterListView,
		B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS, false, true, B_FANCY_BORDER);
	printersBox->AddChild(pscroller);

// ------------------------ Lastly, build the jobs overview box
	fJobsBox = new BBox(BRect(boxInset, r.Height() / 2 + boxInset / 2,
		Bounds().Width() - 10, Bounds().Height() - boxInset), "jobsBox",
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
	fJobsBox->SetFont(be_bold_font);
	fJobsBox->SetLabel(B_TRANSLATE("Print jobs: No printer selected"));
	backdrop->AddChild(fJobsBox);

		// Cancel Job Button
	BButton* cancelButton = new BButton(BRect(5, 5, 5, 5), "cancel",
		B_TRANSLATE("Cancel job"), new BMessage(kMsgCancelJob),
		B_FOLLOW_RIGHT+B_FOLLOW_TOP);
	fJobsBox->AddChild(cancelButton);
	cancelButton->ResizeToPreferred();
	fCancel = cancelButton;

	maxWidth = cancelButton->Bounds().Width();

		// Restart Job button
	BButton* restartButton = new BButton(BRect(5, 30, 5, 30), "restart",
		B_TRANSLATE("Restart job"), new BMessage(kMsgRestartJob),
		B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	fJobsBox->AddChild(restartButton);
	restartButton->ResizeToPreferred();
	fRestart = restartButton;

	if (restartButton->Bounds().Width() > maxWidth)
		maxWidth = restartButton->Bounds().Width();

		// Resize all buttons to maximum width and align them to the right
	xPos = fJobsBox->Bounds().Width() - boxInset - maxWidth;
	cancelButton->MoveTo(xPos, boxInset + 8);
	cancelButton->ResizeTo(maxWidth, cancelButton->Bounds().Height());

	restartButton->MoveTo(xPos, boxInset + cancelButton->Bounds().Height()
		+ boxInset + 8);
	restartButton->ResizeTo(maxWidth, restartButton->Bounds().Height());

		// Disable all selection-based buttons
	cancelButton->SetEnabled(false);
	restartButton->SetEnabled(false);

		// Create listview with scroller
	listBounds = BRect(boxInset, boxInset + 8,
		cancelButton->Frame().left - boxInset - B_V_SCROLL_BAR_WIDTH,
		fJobsBox->Bounds().Height() - boxInset - 3);
	fJobListView = new JobListView(listBounds);
	BScrollView* jscroller = new BScrollView("jobs_scroller", fJobListView,
		B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS, false, true, B_FANCY_BORDER);
	fJobsBox->AddChild(jscroller);

		// Determine min width
	float width = (jscroller->Bounds().Width() < pscroller->Bounds().Width())
		? jscroller->Bounds().Width() : pscroller->Bounds().Width();

		// Resize boxes to the same size
	jscroller->ResizeTo(width, jscroller->Bounds().Height());
	pscroller->ResizeTo(width, pscroller->Bounds().Height());
}


bool
PrintersWindow::_IsSelected(PrinterItem* printer)
{
	return fSelectedPrinter && fSelectedPrinter == printer;
}


void
PrintersWindow::_UpdatePrinterButtons()
{
	PrinterItem* item = fPrinterListView->SelectedItem();
	fRemove->SetEnabled(item && !item->HasPendingJobs());
	fMakeDefault->SetEnabled(item && !item->IsActivePrinter());
	fPrintTestPage->SetEnabled(item);
}


void
PrintersWindow::_UpdateJobButtons()
{
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


