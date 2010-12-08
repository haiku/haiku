/*
* Copyright 2010, Haiku. All rights reserved.
* Distributed under the terms of the MIT License.
*
* Authors:
*		Ithamar R. Adema <ithamar.adema@team-embedded.nl>
*		Michael Pfeiffer
*/


#include "SelectPrinterDialog.h"

#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>

#include <Button.h>
#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <ListView.h>
#include <Message.h>
#include <Path.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <String.h>
#include <StringItem.h>

#include "GPBinding.h"


enum {
	kMsgCancel = 'stop',
	kMsgOK = 'okok',

	kMsgManufacturerSelected = 'msel',
	kMsgPrinterSelected = 'psel',
};


class StringValueItem : public BStringItem {
public:
			StringValueItem(const BString& text, const BString& value)
				:
				BStringItem(text.String()),
				fValue(value)
				{
				}

	BString	fValue;
};


SelectPrinterDialog::SelectPrinterDialog(GPData* data)
	:
	DialogWindow(BRect(100, 100, 500, 500),
		"Select Printer", B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS
			| B_CLOSE_ON_ESCAPE),
	fData(data)
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
	fManufacturersListView->SetSelectionMessage(
		new BMessage(kMsgManufacturerSelected));
	PopulateManufacturers();

	// Build the layout
	SetLayout(new BGroupLayout(B_VERTICAL));

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 10)
		.AddGroup(B_HORIZONTAL, 5, 1.0f)
			.Add(manuScroller)
			.Add(printerScroller)
		.End()
		.AddGroup(B_HORIZONTAL, 5, 2.0f)
			.AddGlue()
			.Add(cancel)
			.Add(ok)
		.End()
		.SetInsets(10, 10, 10, 10)
	);
}


void
SelectPrinterDialog::PopulateManufacturers()
{
	BMessage manufacturers;
	GPBinding binding;
	binding.GetPrinterManufacturers(manufacturers);

	BString id;
	BString displayName;
	for (int32 index = 0; binding.ExtractManufacturer(manufacturers, index, id,
		displayName); index ++) {
		fManufacturersListView->AddItem(new StringValueItem(id, displayName));
	}
}


void
SelectPrinterDialog::PopulatePrinters()
{
	BString manufacturer = GetSelectedItemValue(fManufacturersListView).String();
	if (manufacturer.Length() == 0)
		return;

	BMessage models;
	GPBinding binding;
	binding.GetPrinterModels(manufacturer, models);

	BString displayName;
	BString driver;
	for (int32 index = 0; binding.ExtractModel(models, index, displayName, driver);
		index ++) {
		StringValueItem* item = new StringValueItem(displayName, driver);
		fPrintersListView->AddItem(item);
	}
}


BString
SelectPrinterDialog::GetSelectedItemValue(BListView* listView)
{
	int32 index = listView->CurrentSelection();

	// Bail out if no manufacturer is selected
	if (index < 0)
		return "";
	BListItem* item = listView->ItemAt(index);
	StringValueItem* valueItem = dynamic_cast<StringValueItem*>(item);
	ASSERT(valueItem != NULL);

	return valueItem->fValue;
}

void
SelectPrinterDialog::PrinterSelected()
{
	int32 index = fPrintersListView->CurrentSelection();
	fOKButton->SetEnabled(index >= 0);
}


void
SelectPrinterDialog::Save()
{
	BString driverName = GetSelectedItemValue(fPrintersListView);
	fData->fGutenprintDriverName = driverName;
	fData->Save();
}


void 
SelectPrinterDialog::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case kMsgManufacturerSelected:
			fPrintersListView->MakeEmpty();
			PopulatePrinters();
			PrinterSelected();
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
