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
#include <ControlLook.h>
#include <FindDirectory.h>
#include <GroupLayout.h>
#include <Layout.h>
#include <LayoutBuilder.h>
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
	: BWindow(job->PaperRect().OffsetByCopy(-20000, -20000), B_TRANSLATE("Test page"),
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
		B_TITLED_WINDOW, B_AUTO_UPDATE_SIZE_LIMITS),
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
				BString text = B_TRANSLATE("Print jobs for %printer_name%");
				text.ReplaceFirst("%printer_name%", fSelectedPrinter->Name());

				fJobsBox->SetLabel(text);
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
	BPrintJob* job = new BPrintJob(B_TRANSLATE("Test page"));
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
// ------------------------ Next, build the printers overview box
	BBox* printersBox = new BBox("printersBox");
	printersBox->SetFont(be_bold_font);
	printersBox->SetLabel(B_TRANSLATE("Printers"));

		// Add Button
	BButton* addButton = new BButton("add",
		B_TRANSLATE("Add" B_UTF8_ELLIPSIS), new BMessage(kMsgAddPrinter));
	addButton->SetExplicitMaxSize(
		BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

		// Remove button
	fRemove = new BButton("remove",
		B_TRANSLATE("Remove"), new BMessage(kMsgRemovePrinter));
	fRemove->SetExplicitMaxSize(
		BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

		// Make Default button
	fMakeDefault = new BButton("default",
		B_TRANSLATE("Make default"), new BMessage(kMsgMakeDefaultPrinter));
	fMakeDefault->SetExplicitMaxSize(
		BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

		// Print Test Page button
	fPrintTestPage = new BButton("print_test_page",
		B_TRANSLATE("Print test page"), new BMessage(kMsgPrintTestPage));
	fPrintTestPage->SetExplicitMaxSize(
		BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

		// Disable all selection-based buttons
	fRemove->SetEnabled(false);
	fMakeDefault->SetEnabled(false);
	fPrintTestPage->SetEnabled(false);

		// Create listview with scroller
	fPrinterListView = new PrinterListView(BRect());
	BScrollView* printerScrollView = new BScrollView("printer_scroller",
		fPrinterListView, B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS,
		false, true, B_FANCY_BORDER);

	printerScrollView->SetExplicitMinSize(
		BSize(be_plain_font->Size() * 30, B_SIZE_UNSET));

	float padding = be_control_look->DefaultItemSpacing();

	BLayoutBuilder::Group<>(printersBox, B_HORIZONTAL, padding)
		.SetInsets(padding, padding * 2, padding, padding)
		.Add(printerScrollView)
		.AddGroup(B_VERTICAL, padding / 2, 0.0f)
			.SetInsets(0)
			.Add(addButton)
			.Add(fRemove)
			.Add(fMakeDefault)
			.Add(fPrintTestPage)
			.AddGlue();

// ------------------------ Lastly, build the jobs overview box
	fJobsBox = new BBox("jobsBox");
	fJobsBox->SetFont(be_bold_font);
	fJobsBox->SetLabel(B_TRANSLATE("Print jobs: No printer selected"));

		// Cancel Job Button
	fCancel = new BButton("cancel",
		B_TRANSLATE("Cancel job"), new BMessage(kMsgCancelJob));
	fCancel->SetExplicitMaxSize(
		BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

		// Restart Job button
	fRestart = new BButton("restart",
		B_TRANSLATE("Restart job"), new BMessage(kMsgRestartJob));
	fRestart->SetExplicitMaxSize(
		BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

		// Disable all selection-based buttons
	fCancel->SetEnabled(false);
	fRestart->SetEnabled(false);

		// Create listview with scroller
	fJobListView = new JobListView(BRect());
	BScrollView* jobScrollView = new BScrollView("jobs_scroller",
		fJobListView, B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS,
		false, true, B_FANCY_BORDER);

	BLayoutBuilder::Group<>(fJobsBox, B_HORIZONTAL, padding)
		.SetInsets(padding, padding * 2, padding, padding)
		.Add(jobScrollView)
		.AddGroup(B_VERTICAL, padding / 2, 0.0f)
			.SetInsets(0)
			.Add(fCancel)
			.Add(fRestart)
			.AddGlue();

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(B_USE_WINDOW_SPACING)
		.Add(printersBox)
		.AddStrut(B_USE_DEFAULT_SPACING)
		.Add(fJobsBox);

		// There is a better solution?
	Layout(true);
	if (fPrintTestPage->Bounds().Width() > fRestart->Bounds().Width())
		fRestart->SetExplicitMinSize(
			BSize(fPrintTestPage->Bounds().Width(), B_SIZE_UNSET));
	else
		fPrintTestPage->SetExplicitMinSize(
			BSize(fRestart->Bounds().Width(), B_SIZE_UNSET));
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


