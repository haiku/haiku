/*
 *  Printers Preference Application.
 *  Copyright (C) 2001 OpenBeOS. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "PrintersWindow.h"

#include "AddPrinterDialog.h"
#include "PrinterListView.h"
#include "pr_server.h"
#include "Globals.h"
#include "Messages.h"

// BeOS API
#include <Box.h>
#include <Button.h>
#include <ListView.h>
#include <ScrollView.h>
#include <Application.h>

PrintersWindow::PrintersWindow(BRect frame)
	: Inherited(BRect(78.0, 71.0, 561.0, 409.0), "Printers", B_TITLED_WINDOW, B_NOT_H_RESIZABLE)
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
				int32 prIndex = fPrinterListView->CurrentSelection();
				if (prIndex >= 0)
				{
					BMessenger msgr;
					if (::GetPrinterServerMessenger(msgr) == B_OK)
					{
						BMessage script(B_GET_PROPERTY);
						BMessage reply;
						BString name;
						
						script.AddSpecifier("Name");
						script.AddSpecifier("Printer", fPrinterListView->CurrentSelection());
						msgr.SendMessage(&script,&reply);
						if (reply.FindString("result", &name) == B_OK)
						{
							fJobsBox->SetLabel((BString("Print Jobs for ") << name).String());
							fMakeDefault->SetEnabled(true);
							fRemove->SetEnabled(true);
						}
					}
				}
				else
				{
					fJobsBox->SetLabel("Print Jobs: No printer selected");
					fMakeDefault->SetEnabled(false);
					fRemove->SetEnabled(false);
				}
			}
			break;

		case MSG_ADD_PRINTER:
			//if (AddPrinterDialog::Start() == B_OK)	
			break;

		case MSG_REMOVE_PRINTER:
			{
					// Get selection
				int32 idx = fPrinterListView->CurrentSelection();
				
					// If a printer is selected
				if (idx >= 0)
				{
					PrinterItem* item = (PrinterItem*)fPrinterListView->ItemAt(idx);
					if (item != NULL)
						item->Remove(fPrinterListView);
				}
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
					}
				}
			}
			break;


		case MSG_CANCEL_JOB:
			break;

		case MSG_RESTART_JOB:
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
	BButton* addButton = new BButton(BRect(5,5,5,5), "add", "Add...", new BMessage(MSG_ADD_PRINTER), B_FOLLOW_RIGHT);
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

	maxWidth = cancelButton->Bounds().Width();

		// Restart Job button
	BButton* restartButton = new BButton(BRect(5,30,5,30), "restart", "Restart Job", new BMessage(MSG_RESTART_JOB), B_FOLLOW_RIGHT+B_FOLLOW_TOP);
	fJobsBox->AddChild(restartButton);
	restartButton->ResizeToPreferred();

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
	fJobListView = new BListView(listBounds, "jobs_list", B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL);
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
