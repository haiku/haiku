/*
 * Copyright 2008-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Julun, <host.haiku@gmx.de>
 */

#include <JobSetupPanel.h>

#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <GroupView.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Printer.h>
#include <PrinterRoster.h>
#include <RadioButton.h>
#include <StringView.h>
#include <TextControl.h>


#include <stdlib.h>


namespace BPrivate {
	namespace Print {


BJobSetupPanel::BJobSetupPanel(BPrinter* printer)
	: BPrintPanel("Print document")
	, fPrinter(printer)
	, fPrinterRoster(NULL)
	, fPrintRange(B_ALL_PAGES)
	, fJobPanelFlags(B_NO_OPTIONS)
{
	_InitObject();
	_SetupInterface();
}


BJobSetupPanel::BJobSetupPanel(BPrinter* printer, uint32 flags)
	: BPrintPanel("Print document")
	, fPrinter(printer)
	, fPrinterRoster(NULL)
	, fPrintRange(B_ALL_PAGES)
	, fJobPanelFlags(flags)
{
	_InitObject();
	_SetupInterface();
}


BJobSetupPanel::~BJobSetupPanel()
{
	delete fPrinterRoster;
}


BJobSetupPanel::BJobSetupPanel(BMessage* data)
	: BPrintPanel(data)
{
	// TODO: implement
}


BArchivable*
BJobSetupPanel::Instantiate(BMessage* data)
{
	// TODO: implement
	return NULL;
}


status_t
BJobSetupPanel::Archive(BMessage* data, bool deep) const
{
	// TODO: implement
	return B_ERROR;
}


void
BJobSetupPanel::MessageReceived(BMessage* message)
{

	BPrintPanel::MessageReceived(message);
}


status_t
BJobSetupPanel::Go()
{
	status_t status = ShowPanel();

	if (status == B_OK) {
		// TODO: check if we did work on an real printer
		// TODO: set all selected values on printer object
	}

	if (Lock())
		Quit();

	return status;
}


BPrinter*
BJobSetupPanel::Printer() const
{
	return fPrinter;
}


void
BJobSetupPanel::SetPrinter(BPrinter* printer, bool keepSettings)
{
	// TODO: implement
}


print_range
BJobSetupPanel::PrintRange() const
{
	return fPrintRange;
}


void
BJobSetupPanel::SetPrintRange(print_range range)
{
	switch (range) {
		case B_ALL_PAGES: {
			fPrintRange = range;
			fPrintAll->SetValue(B_CONTROL_ON);
		}	break;

		case B_SELECTION: {
			fPrintRange = range;
			SetOptionFlags(OptionFlags() | B_PRINT_SELECTION);
			fSelection->SetValue(B_CONTROL_ON);
		}	break;

		case B_PAGE_RANGE: {
			fPrintRange = range;
			SetOptionFlags(OptionFlags() | B_PRINT_PAGE_RANGE);
			fPagesFrom->SetValue(B_CONTROL_ON);
		}	break;
	}
}


int32
BJobSetupPanel::FirstPage() const
{
	BString text(fFirstPage->Text());
	if (text.Length() <= 0)
		return 0;

	return atoi(text.String());
}


int32
BJobSetupPanel::LastPage() const
{
	BString text(fLastPage->Text());
	if (text.Length() <= 0)
		return LONG_MAX;

	return atoi(text.String());
}


void
BJobSetupPanel::SetPageRange(int32 firstPage, int32 lastPage)
{
	BString text;
	SetPrintRange(B_PAGE_RANGE);

	text << firstPage;
	fFirstPage->SetText(text.String());

	text << lastPage;
	fLastPage->SetText(text.String());
}


uint32
BJobSetupPanel::OptionFlags() const
{
	return fJobPanelFlags;
}


void
BJobSetupPanel::SetOptionFlags(uint32 flags)
{
	bool value = false;
	if (flags & B_PRINT_TO_FILE)
		value = true;
	fPrintToFile->SetEnabled(value);

	value = false;
	if (flags & B_PRINT_PAGE_RANGE)
		value = true;
	fPagesFrom->SetEnabled(value);
	fFirstPage->SetEnabled(value);
	fLastPage->SetEnabled(value);

	value = false;
	if (flags & B_PRINT_SELECTION)
		value = true;
	fSelection->SetEnabled(value);

	value = false;
	if (flags & B_PRINT_COLLATE_COPIES)
		value = true;
	fCollate->SetEnabled(value);

	fJobPanelFlags = flags;
}


void
BJobSetupPanel::_InitObject()
{
	fPrinterRoster = new BPrinterRoster();
	fPrinterRoster->StartWatching(this);

	if (!fPrinter->IsValid()) {
		BPrinter defaultPrinter;
		fPrinterRoster->GetDefaultPrinter(&defaultPrinter);
		*fPrinter = defaultPrinter;
	}
}


void
BJobSetupPanel::_SetupInterface()
{
	BGroupView* groupView = new BGroupView(B_VERTICAL, 10.0);

	// printers
	fPrinterPopUp = new BPopUpMenu("");
	fPrinterPopUp->SetRadioMode(true);
	fPrinterMenuField = new BMenuField("", fPrinterPopUp);
	fPrinterMenuField->Menu()->SetLabelFromMarked(true);

	BPrinter printer;
	while (fPrinterRoster->GetNextPrinter(&printer) == B_OK) {
		BMenuItem* item = new BMenuItem(printer.Name().String(), NULL);
		fPrinterPopUp->AddItem(item);
		if (printer == *fPrinter)
			item->SetMarked(true);
	}

	if (fPrinterRoster->CountPrinters() > 0)
		fPrinterPopUp->AddItem(new BSeparatorItem);

	BMenuItem* pdf = new BMenuItem("Save as PDF file" , NULL);
	fPrinterPopUp->AddItem(pdf);
	if (fPrinterPopUp->FindMarked() == NULL)
		pdf->SetMarked(true);

	fProperties = new BButton("Properties"B_UTF8_ELLIPSIS , new BMessage('prop'));
	fPrinterInfo = new BStringView("label", "");
	fPrinterInfo->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	BBox* divider = new BBox(B_FANCY_BORDER, NULL);
	divider->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 1));
	fPrintToFile = new BCheckBox("Print to file");

	BView* view = BGroupLayoutBuilder(B_VERTICAL, 5.0)
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, 10.0)
			.Add(fPrinterMenuField->CreateMenuBarLayoutItem())
			.Add(fProperties))
		.Add(BGroupLayoutBuilder(B_HORIZONTAL,5.0)
			.Add(new BStringView("label", "Printer info:"))
			.Add(fPrinterInfo))
		.Add(divider)
		.Add(fPrintToFile)
		.SetInsets(10.0, 5.0, 10.0, 5.0);

	BBox *box = new BBox(B_FANCY_BORDER, view);
	box->SetLabel(BGroupLayoutBuilder()
		.Add(new BStringView("", "Printer"))
		.SetInsets(2.0, 0.0, 2.0, 0.0));
	groupView->AddChild(box);

	// page range
	fPrintAll = new BRadioButton("Print all", new BMessage('prrg'));
	fPrintAll->SetValue(B_CONTROL_ON);
	fPagesFrom = new BRadioButton("Pages from:", new BMessage('prrg'));
	fFirstPage = new BTextControl("", "", NULL);
	_DisallowChar(fFirstPage->TextView());
	fLastPage = new BTextControl("to:", "", NULL);
	_DisallowChar(fLastPage->TextView());
	fSelection = new BRadioButton("Print selection", new BMessage('prrg'));

	fFirstPage->CreateLabelLayoutItem();
	view = BGroupLayoutBuilder(B_VERTICAL, 5.0)
		.Add(fPrintAll)
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, 5.0)
			.Add(fPagesFrom)
			.Add(fFirstPage->CreateTextViewLayoutItem())
			.Add(fLastPage->CreateLabelLayoutItem())
			.Add(fLastPage->CreateTextViewLayoutItem()))
		.Add(fSelection)
		.SetInsets(10.0, 5.0, 10.0, 5.0);

	box = new BBox(B_FANCY_BORDER, view);
	box->SetLabel(BGroupLayoutBuilder()
		.Add(new BStringView("", "Page range"))
		.SetInsets(2.0, 0.0, 2.0, 0.0));

	// copies
	fNumberOfCopies = new BTextControl("Number of copies:", "1", NULL);
	_DisallowChar(fNumberOfCopies->TextView());
	fCollate = new BCheckBox("Collate");
	fReverse = new BCheckBox("Reverse");

	BView* view2 = BGroupLayoutBuilder(B_VERTICAL, 5.0)
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, 5.0)
			.Add(fNumberOfCopies->CreateLabelLayoutItem())
			.Add(fNumberOfCopies->CreateTextViewLayoutItem()))
		.Add(fCollate)
		.Add(fReverse)
		.SetInsets(10.0, 5.0, 10.0, 5.0);

	BBox* box2 = new BBox(B_FANCY_BORDER, view2);
	box2->SetLabel(BGroupLayoutBuilder()
		.Add(new BStringView("", "Copies"))
		.SetInsets(2.0, 0.0, 2.0, 0.0));

	groupView->AddChild(BGroupLayoutBuilder(B_HORIZONTAL, 10.0)
		.Add(box)
		.Add(box2));

	// other
	fColor = new BCheckBox("Print in color");
	fDuplex = new BCheckBox("Double side printing");

	view = BGroupLayoutBuilder(B_VERTICAL, 5.0)
		.Add(fColor)
		.Add(fDuplex)
		.SetInsets(10.0, 5.0, 10.0, 5.0);

	box = new BBox(B_FANCY_BORDER, view);
	box->SetLabel(BGroupLayoutBuilder()
		.Add(new BStringView("", "Other"))
		.SetInsets(2.0, 0.0, 2.0, 0.0));
	groupView->AddChild(box);

	AddPanel(groupView);
	SetOptionFlags(fJobPanelFlags);
}


void
BJobSetupPanel::_DisallowChar(BTextView* textView)
{
	for (uint32 i = 0; i < '0'; ++i)
		textView->DisallowChar(i);

	for (uint32 i = '9' + 1; i < 255; ++i)
		textView->DisallowChar(i);
}


	}	// namespace Print
}	// namespace BPrivate
