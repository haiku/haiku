/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#include "PrinterSelection.h"
#include "PPDParser.h"
#include "StatementListVisitor.h"
#include "UIUtils.h"

#include <Directory.h>
#include <Entry.h>
#include <Path.h>

#include <ScrollView.h>
#include <StringView.h>

// margin
const float kLeftMargin = 3.0;
const float kRightMargin = 3.0;
const float kTopMargin = 3.0;
const float kBottomMargin = 3.0;

// space between views
const float kHorizontalSpace = 8.0;
const float kVerticalSpace = 8.0;

#include <stdio.h>

PrinterSelectionView::PrinterSelectionView(BRect bounds, const char *name, uint32 resizeMask, uint32 flags) 
	: BView(bounds, name, resizeMask, flags) 
{	
	// add vendor list view	
	bounds.OffsetTo(0, 0);
	BRect listBounds(bounds.left + kLeftMargin, bounds.top + kTopMargin, 
		bounds.right / 3.0 - kHorizontalSpace / 2, bounds.bottom - kBottomMargin);
	listBounds.right -= B_V_SCROLL_BAR_WIDTH;
	listBounds.bottom -= B_H_SCROLL_BAR_HEIGHT;

	BStringView* label = new BStringView(listBounds, "vendors-label", "Vendors:");
	AddChild(label);
	label->ResizeToPreferred();
	
	listBounds.top += label->Bounds().bottom + 5;

	fVendors = new BListView(listBounds, "vendors", B_SINGLE_SELECTION_LIST,
		B_FOLLOW_ALL);
	FillVendors();

	BScrollView* scrollView = new BScrollView("vendors-scroll-view", 
		fVendors, B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM, 0, true, true);

	AddChild(scrollView);

	// add details view
	BRect printerBounds(listBounds);
	printerBounds.left = B_V_SCROLL_BAR_WIDTH + printerBounds.right + kHorizontalSpace;
	printerBounds.right = bounds.right - kRightMargin - B_V_SCROLL_BAR_WIDTH;
	printerBounds.top = bounds.top + kTopMargin;
	label = new BStringView(printerBounds, "printers-label", "Printers:");
	AddChild(label);
	label->ResizeToPreferred();

	BRect detailBounds(listBounds);
	detailBounds.left = B_V_SCROLL_BAR_WIDTH + detailBounds.right + kHorizontalSpace;
	detailBounds.right = bounds.right - kRightMargin - B_V_SCROLL_BAR_WIDTH;
	fPrinters = new BListView(detailBounds, "printers", B_SINGLE_SELECTION_LIST,
		B_FOLLOW_ALL);

	scrollView = new BScrollView("printers-scroll-view", 
		fPrinters, B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM, 0, true, true);

	AddChild(scrollView);
}

void PrinterSelectionView::AttachedToWindow()
{
	fVendors->SetSelectionMessage(new BMessage('sel'));
	fVendors->SetTarget(this);

	fPrinters->SetSelectionMessage(new BMessage('prnt'));
	fPrinters->SetTarget(this);
}


void PrinterSelectionView::FillVendors()
{
	BDirectory directory("/boot/beos/etc/ppd");
	BEntry entry;
	while (directory.GetNextEntry(&entry) == B_OK) {
		char name[B_FILE_NAME_LENGTH];
		entry.GetName(name);
		BPath path;
		entry.GetPath(&path);
		fVendors->AddItem(new FileItem(name, path.Path()));
	}
}

void PrinterSelectionView::FillPrinters(const char* vendor)
{
	MakeEmpty(fPrinters);
	
	BList printers;
	BDirectory directory(vendor);
	BEntry entry;
	while (directory.GetNextEntry(&entry) == B_OK) {
		char name[B_FILE_NAME_LENGTH];
		entry.GetName(name);
		BPath path;
		entry.GetPath(&path);
		
		PPDParser parser(path.Path());
		PPD* ppd = parser.ParseHeader();
		if (parser.HasWarning()) {
			fprintf(stderr, "Warning(s): %s", parser.GetWarningMessage());
		}

		if (ppd != NULL) {
		
			BString label;
			const char* s;
			s = ppd->GetValue("ModelName");
			if (s != NULL) {
				label << s;
			}
			s = ppd->GetValue("PCFileName");
			if (s != NULL) {
				label << " [" << s << "]";
			}
			s = ppd->GetValue("Manufacturer");
			if (s != NULL) {
				label << " (" << s << ")";	
			}
			printers.AddItem(new FileItem(label.String(), path.Path()));
			delete ppd;
		} else {
			fprintf(stderr, "Parsing error (%s)\n%s\n", path.Path(),
				parser.GetErrorMessage());
		}
	}
	
	fPrinters->AddList(&printers);
}

void PrinterSelectionView::MessageReceived(BMessage* msg)
{
	int32 index;
	switch (msg->what) {
		case 'sel': 
			if (msg->FindInt32("index", &index) == B_OK) {
				FileItem* file = (FileItem*)fVendors->ItemAt(index);
				if (file != NULL) {
					FillPrinters(file->GetFile());
				}
			}
			break;
		case 'prnt': 
			if (msg->FindInt32("index", &index) == B_OK) {
				FileItem* file = (FileItem*)fPrinters->ItemAt(index);
				if (file != NULL) {
					BMessage copy(*Message());
					copy.AddString("file", file->GetFile());
					InvokeNotify(&copy);
				}
			}
			break;
	}
	BView::MessageReceived(msg);
}
