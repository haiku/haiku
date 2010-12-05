/*
 * Copyright 2003-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 *		Simon Gauvin
 *		Michael Pfeiffer
 *		Dr. Hartmut Reh
 *		julun <host.haiku@gmx.de>
 */

#include "MarginView.h"
#include "PrinterDriver.h"
#include "PageSetupWindow.h"
#include "PrintUtils.h"

#include <stdlib.h>


#include <Box.h>
#include <Button.h>
#include <GridView.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <MenuField.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <Screen.h>
#include <TextControl.h>


/* copied from PDFlib.h: */
#define a0_width	 (float) 2380.0
#define a0_height	 (float) 3368.0
#define a1_width	 (float) 1684.0
#define a1_height	 (float) 2380.0
#define a2_width	 (float) 1190.0
#define a2_height	 (float) 1684.0
#define a3_width	 (float) 842.0
#define a3_height	 (float) 1190.0
#define a4_width	 (float) 595.0
#define a4_height	 (float) 842.0
#define a5_width	 (float) 421.0
#define a5_height	 (float) 595.0
#define a6_width	 (float) 297.0
#define a6_height	 (float) 421.0
#define b5_width	 (float) 501.0
#define b5_height	 (float) 709.0
#define letter_width	 (float) 612.0
#define letter_height	 (float) 792.0
#define legal_width 	 (float) 612.0
#define legal_height 	 (float) 1008.0
#define ledger_width	 (float) 1224.0
#define ledger_height	 (float) 792.0
#define p11x17_width	 (float) 792.0
#define p11x17_height	 (float) 1224.0


static struct
{
	const char * label;
	float width;
	float height;
} pageFormat[] =
{
	{"Letter", letter_width, letter_height },
	{"Legal",  legal_width,  legal_height  },
	{"Ledger", ledger_width, ledger_height  },
	{"p11x17", p11x17_width, p11x17_height  },
	{"A0",     a0_width,     a0_height     },
	{"A1",     a1_width,     a1_height     },
	{"A2",     a2_width,     a2_height     },
	{"A3",     a3_width,     a3_height     },
	{"A4",     a4_width,     a4_height     },
	{"A5",     a5_width,     a5_height     },
	{"A6",     a6_width,     a6_height     },
	{"B5",     b5_width,     b5_height     },
	{NULL,     0.0,          0.0           }
};


static struct
{
	const char * label;
	int32 orientation;
} orientation[] =
{
	{ "Portrait", PrinterDriver::PORTRAIT_ORIENTATION },
	{ "Landscape", PrinterDriver::LANDSCAPE_ORIENTATION },
	{ NULL, 0 }
};


PageSetupWindow::PageSetupWindow(BMessage *msg, const char *printerName)
	: BlockingWindow(BRect(0, 0, 100, 100), "Page setup",
			B_TITLED_WINDOW_LOOK,
 			B_MODAL_APP_WINDOW_FEEL,
 			B_NOT_RESIZABLE | B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE
				| B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	fSetupMsg(msg),
	fPrinterDirName(printerName)
{
	if (printerName)
		SetTitle(BString(printerName).Append(" Page setup").String());

	// load orientation
	if (fSetupMsg->FindInt32("orientation", &fCurrentOrientation) != B_OK)
		fCurrentOrientation = PrinterDriver::PORTRAIT_ORIENTATION;

	// load page rect
	BRect page;
	float width = letter_width;
	float height = letter_height;
	if (fSetupMsg->FindRect("preview:paper_rect", &page) == B_OK) {
		width = page.Width();
		height = page.Height();
	} else {
		page.Set(0, 0, width, height);
	}

	BString label;
	if (fSetupMsg->FindString("preview:paper_size", &label) != B_OK)
		label = "Letter";

	// Load units
	int32 units;
	if (fSetupMsg->FindInt32("units", &units) != B_OK)
		units = kUnitInch;

	// re-calculate the margin from the printable rect in points
	BRect margin = page;
	if (fSetupMsg->FindRect("preview:printable_rect", &margin) == B_OK) {
		margin.top -= page.top;
		margin.left -= page.left;
		margin.right = page.right - margin.right;
		margin.bottom = page.bottom - margin.bottom;
	} else {
		margin.Set(28.34, 28.34, 28.34, 28.34);		// 28.34 dots = 1cm
	}


	fMarginView = new MarginView(int32(width), int32(height), margin,
		MarginUnit(units));

	BPopUpMenu* pageSizePopUpMenu = new BPopUpMenu("Page size");
	pageSizePopUpMenu->SetRadioMode(true);

	fPageSizeMenu = new BMenuField("page_size", "Page size:", pageSizePopUpMenu);
	fPageSizeMenu->Menu()->SetLabelFromMarked(true);

	for (int32 i = 0; pageFormat[i].label != NULL; i++) {
		BMessage* message = new BMessage(PAGE_SIZE_CHANGED);
		message->AddFloat("width", pageFormat[i].width);
		message->AddFloat("height", pageFormat[i].height);
		BMenuItem* item = new BMenuItem(pageFormat[i].label, message);
		pageSizePopUpMenu->AddItem(item);

		if (label.Compare(pageFormat[i].label) == 0)
			item->SetMarked(true);
	}

	BPopUpMenu* orientationPopUpMenu = new BPopUpMenu("Orientation");
	orientationPopUpMenu->SetRadioMode(true);

	fOrientationMenu = new BMenuField("orientation", "Orientation:",
		orientationPopUpMenu);
	fOrientationMenu->Menu()->SetLabelFromMarked(true);

	for (int32 i = 0; orientation[i].label != NULL; i++) {
	 	BMessage* message = new BMessage(ORIENTATION_CHANGED);
		message->AddInt32("orientation", orientation[i].orientation);
		BMenuItem* item = new BMenuItem(orientation[i].label, message);
		orientationPopUpMenu->AddItem(item);

		if (fCurrentOrientation == orientation[i].orientation)
			item->SetMarked(true);
	}

	float scale0;
	BString scale;
	if (fSetupMsg->FindFloat("scale", &scale0) == B_OK)
		scale << (int)scale0;
	else
		scale = "100";

	fScaleControl = new BTextControl("scale", "Scale [%]:",
		scale.String(), NULL);

	for (uint32 i = 0; i < '0'; i++)
		fScaleControl->TextView()->DisallowChar(i);

	for (uint32 i = '9' + 1; i < 255; i++)
		fScaleControl->TextView()->DisallowChar(i);

	fScaleControl->TextView()->SetMaxBytes(3);

	BBox *separator = new BBox("separator");
	separator->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 1));

	BButton *cancel = new BButton("cancel", "Cancel", new BMessage(CANCEL_MSG));

	BButton *ok = new BButton("ok", "OK", new BMessage(OK_MSG));
	ok->MakeDefault(true);

	BGridView* settings = new BGridView();
	BGridLayout* settingsLayout = settings->GridLayout();
	settingsLayout->AddItem(fPageSizeMenu->CreateLabelLayoutItem(), 0, 0);
	settingsLayout->AddItem(fPageSizeMenu->CreateMenuBarLayoutItem(), 1, 0);
	settingsLayout->AddItem(fOrientationMenu->CreateLabelLayoutItem(), 0, 1);
	settingsLayout->AddItem(fOrientationMenu->CreateMenuBarLayoutItem(), 1, 1);
	settingsLayout->AddItem(fScaleControl->CreateLabelLayoutItem(), 0, 2);
	settingsLayout->AddItem(fScaleControl->CreateTextViewLayoutItem(), 1, 2);
	settingsLayout->SetSpacing(0, 0);

	SetLayout(new BGroupLayout(B_VERTICAL));
	AddChild(BGroupLayoutBuilder(B_VERTICAL, 0)
		.AddGroup(B_HORIZONTAL, 5, 1)
			.AddGroup(B_VERTICAL, 0, 1.0f)
				.Add(fMarginView)
				.AddGlue()
			.End()
			.AddGroup(B_VERTICAL, 0, 1.0f)
				.Add(settings)
				.AddGlue()
			.End()
		.End()
		.Add(separator)
		.AddGroup(B_HORIZONTAL, 10, 1.0f)
			.AddGlue()
			.Add(cancel)
			.Add(ok)
		.End()
		.SetInsets(10, 10, 10, 10)
	);

	BRect winFrame(Frame());
	BRect screenFrame(BScreen().Frame());
	MoveTo((screenFrame.right - winFrame.right) / 2,
		(screenFrame.bottom - winFrame.bottom) / 2);
}


void
PageSetupWindow::UpdateSetupMessage()
{
	SetInt32(fSetupMsg, "xres", 300);
	SetInt32(fSetupMsg, "yres", 300);
	SetInt32(fSetupMsg, "orientation", fCurrentOrientation);

	// Save scaling factor
	float scale = atoi(fScaleControl->Text());
	if (scale <= 0.0) scale = 100.0;
	if (scale > 1000.0) scale = 1000.0;
	SetFloat(fSetupMsg, "scale", scale);

	float scaleR = 100.0 / scale;
	BMenuItem *item = fPageSizeMenu->Menu()->FindMarked();
	if (item) {
		float w, h;
		BMessage *msg = item->Message();
		msg->FindFloat("width", &w);
		msg->FindFloat("height", &h);
		BRect r(0, 0, w, h);
		if (fCurrentOrientation == PrinterDriver::LANDSCAPE_ORIENTATION)
			r.Set(0, 0, h, w);

		SetRect(fSetupMsg, "preview:paper_rect", r);
		SetRect(fSetupMsg, "paper_rect", ScaleRect(r, scaleR));
		SetString(fSetupMsg, "preview:paper_size", item->Label());

		// Save the printable_rect
		BRect margin = fMarginView->Margin();
		if (fCurrentOrientation == PrinterDriver::PORTRAIT_ORIENTATION) {
			margin.right = w - margin.right;
			margin.bottom = h - margin.bottom;
		} else {
			margin.right = h - margin.right;
			margin.bottom = w - margin.bottom;
		}
		SetRect(fSetupMsg, "preview:printable_rect", margin);
		SetRect(fSetupMsg, "printable_rect", ScaleRect(margin, scaleR));

		SetInt32(fSetupMsg, "units", fMarginView->Unit());
	}
}


void
PageSetupWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case OK_MSG: {
			UpdateSetupMessage();
			Quit(B_OK);
		}	break;

		case CANCEL_MSG: {
			Quit(B_ERROR);
		}	break;

		case PAGE_SIZE_CHANGED:	{
			float w, h;
			msg->FindFloat("width", &w);
			msg->FindFloat("height", &h);
			if (fCurrentOrientation == PrinterDriver::PORTRAIT_ORIENTATION) {
				fMarginView->SetPageSize(w, h);
			} else {
				fMarginView->SetPageSize(h, w);
			}
			fMarginView->UpdateView(MARGIN_CHANGED);
		}	break;

		case ORIENTATION_CHANGED: {
			int32 orientation;
			msg->FindInt32("orientation", &orientation);

			if (fCurrentOrientation != orientation) {
				fCurrentOrientation = orientation;
				BPoint p = fMarginView->PageSize();
				fMarginView->SetPageSize(p.y, p.x);
				fMarginView->UpdateView(MARGIN_CHANGED);
			}
		}	break;

		default:
			BlockingWindow::MessageReceived(msg);
			break;
	}
}
