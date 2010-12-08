/*
* Copyright 2010, Haiku. All rights reserved.
* Distributed under the terms of the MIT License.
*
* Authors:
*		Ithamar R. Adema <ithamar.adema@team-embedded.nl>
*/


#include "SelectPPDDlg.h"

#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>

#include <Button.h>
#include <Directory.h>
#include <Entry.h>
#include <ListView.h>
#include <Message.h>
#include <Path.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <String.h>
#include <StringItem.h>

#include "PPDParser.h"

enum {
	kMsgCancel = 'stop',
	kMsgOK = 'okok',

	kMsgManuSelected = 'msel',
	kMsgPrinterSelected = 'psel',
};


class PPDStringItem : public BStringItem {
public:
			PPDStringItem(const BString& text, const BString& path)
			:
			BStringItem(text.String()),
			fPPDPath(path)
			{
			}

	BString	fPPDPath;
};


SelectPPDDlg::SelectPPDDlg(PSData* data)
	: DialogWindow(BRect(10, 10, 400, 400),
		"Select PPD", B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS
			| B_CLOSE_ON_ESCAPE),
		fPSData(data)
{
	SetResult(B_ERROR);

	BButton* ok;
	BButton* cancel;

	ok = new BButton("btn:ok", "OK", new BMessage(kMsgOK));
	ok->MakeDefault(true);
	ok->SetEnabled(false);
	fOKButton = ok;

	cancel = new BButton("btn:cancel", "Cancel", new BMessage(kMsgCancel));

	BScrollView* manuScroller, *printerScroller;
	fManufacturersListView = new BListView("olv:manufacturers");
	manuScroller = new BScrollView("scr:manufacturers", fManufacturersListView,
		0, false, true);
	fPrintersListView = new BListView("olv:printers");
	printerScroller = new BScrollView("scr:printers", fPrintersListView, 0,
		false, true);

	fPrintersListView->SetSelectionMessage(new BMessage(kMsgPrinterSelected));
	fManufacturersListView->SetSelectionMessage(new BMessage(kMsgManuSelected));
	PopulateManufacturers(B_SYSTEM_DATA_DIRECTORY);

	// Build the layout
	SetLayout(new BGroupLayout(B_VERTICAL));

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 10)
		.AddGroup(B_HORIZONTAL, 5)
			.Add(manuScroller)
			.Add(printerScroller)
		.End()
		.AddGroup(B_HORIZONTAL, 5)
			.AddGlue()
			.Add(cancel)
			.Add(ok)
		.End()
		.SetInsets(10, 10, 10, 10)
	);
}


void
SelectPPDDlg::PopulateManufacturers(directory_which data_dir)
{
	char name[1024];
	BDirectory dir;
	BEntry entry;
	BPath path;

	if (find_directory(data_dir, &path) == B_OK
		&& path.Append("ppd") == B_OK
		&& dir.SetTo(path.Path()) == B_OK) {
		// Got the directory, now scan it
		while (dir.GetNextEntry(&entry) == B_OK)
			if (entry.IsDirectory() 
				&& entry.GetName(name) == B_OK)
				fManufacturersListView->AddItem(new BStringItem(name));
	}
}


void
SelectPPDDlg::PopulatePrinters(directory_which data_dir)
{
	int32 idx = fManufacturersListView->CurrentSelection();
	char name[1024];
	BDirectory dir;
	BString manu;
	BEntry entry;
	BPath path;

	// Bail out if no manufacturer is selected
	if (idx < 0)
		return;

	manu = ((BStringItem*)fManufacturersListView->ItemAt(idx))->Text();

	if (find_directory(data_dir, &path) == B_OK
		&& path.Append("ppd") == B_OK
		&& path.Append(manu) == B_OK
		&& dir.SetTo(path.Path()) == B_OK) {
		// Found manufacturer PPD directory, now fill our printer list
		while (dir.GetNextEntry(&entry) == B_OK)
			if (entry.GetName(name) == B_OK) {
				PPDParser parser(dir, name);
				if (parser.InitCheck() == B_OK) {
					BString modelName = parser.GetParameter("ModelName");
					BPath ppdPath = path;
					ppdPath.Append(name);
					fPrintersListView->AddItem(new PPDStringItem(modelName,
						ppdPath.Path()));
				}
			}
	}
}


void
SelectPPDDlg::PrinterSelected()
{
	int32 idx = fPrintersListView->CurrentSelection();
	fOKButton->SetEnabled(idx >= 0);
}


void
SelectPPDDlg::Save()
{
	BString ppdPath;
	int32 idx;

	idx = fPrintersListView->CurrentSelection();
	if (idx >= 0)
		ppdPath = dynamic_cast<PPDStringItem*>
			(fPrintersListView->ItemAt(idx))->fPPDPath;

	fPSData->fPPD = ppdPath;
	fPSData->Save();
}


void 
SelectPPDDlg::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case kMsgManuSelected:
			fPrintersListView->MakeEmpty();
			PopulatePrinters(B_SYSTEM_DATA_DIRECTORY);
			break;
		case kMsgPrinterSelected:
			PrinterSelected();
			break;
		case kMsgOK:
			Save();
			SetResult(B_NO_ERROR);
			PostMessage(B_QUIT_REQUESTED);
			break;
		case kMsgCancel:
			PostMessage(B_QUIT_REQUESTED);
			break;
		default:
			DialogWindow::MessageReceived(msg);
			break;
	}
}
