/*
 * Copyright 2003-2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 *		Simon Gauvin
 *		Michael Pfeiffer
 *		Dr. Hartmut Reh
 *		julun <host.haiku@gmx.de>
 */

#include "BeUtils.h"
#include "MarginView.h"
#include "PrinterDriver.h"
#include "PageSetupWindow.h"


#include <stdlib.h>


#include <Box.h>
#include <Button.h>
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
	char  *label;
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
	char  *label;
	int32 orientation;
} orientation[] =
{
	{"Portrait",  PrinterDriver::PORTRAIT_ORIENTATION},
	{"Landscape", PrinterDriver::LANDSCAPE_ORIENTATION},
	{NULL, 0}
};


PageSetupWindow::PageSetupWindow(BMessage *msg, const char *printerName)
	:	BlockingWindow(BRect(0,0,400,220), "Page Setup", B_TITLED_WINDOW_LOOK,
 			B_MODAL_APP_WINDOW_FEEL, B_NOT_RESIZABLE | B_NOT_MINIMIZABLE |
 			B_NOT_ZOOMABLE),
	fSetupMsg(msg),
	fPrinterDirName(printerName)
{
	if (printerName)
		SetTitle(BString(printerName).Append(" Page Setup").String());

	// load orientation
	int32 orient = 0;
	fSetupMsg->FindInt32("orientation", &orient);

	// load page rect
	float width = letter_width;
	float height = letter_height;
	BRect page(0, 0, width, height);
	if (fSetupMsg->FindRect("preview:paper_rect", &page) == B_OK) {
		width = page.Width();
		height = page.Height();
	}

	BString label("Letter");
	fSetupMsg->FindString("preview:paper_size", &label);

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

	BRect bounds(Bounds());
	BBox *panel = new BBox(bounds, "background", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP, B_PLAIN_BORDER);
	AddChild(panel);

	bounds.InsetBy(10.0, 10.0);
	bounds.right = 230.0;
	bounds.bottom = 160.0;
	fMarginView = new MarginView(bounds, int32(width), int32(height), margin,
		MarginUnit(units));
	panel->AddChild(fMarginView);
	fMarginView->SetResizingMode(B_FOLLOW_NONE);

	BPopUpMenu* m = new BPopUpMenu("Page Size");
	m->SetRadioMode(true);

	bounds.OffsetBy(bounds.Width() + 10.0, 5.0);
	float divider = be_plain_font->StringWidth("Orientation: ");
	fPageSizeMenu = new BMenuField(bounds, "page_size", "Page Size:", m);
	panel->AddChild(fPageSizeMenu);
	fPageSizeMenu->ResizeToPreferred();
	fPageSizeMenu->SetDivider(divider);
	fPageSizeMenu->Menu()->SetLabelFromMarked(true);

	for (int32 i = 0; pageFormat[i].label != NULL; i++) {
		BMessage* message = new BMessage(PAGE_SIZE_CHANGED);
		message->AddFloat("width", pageFormat[i].width);
		message->AddFloat("height", pageFormat[i].height);
		BMenuItem* item = new BMenuItem(pageFormat[i].label, message);
		m->AddItem(item);

		if (label.Compare(pageFormat[i].label) == 0)
			item->SetMarked(true);
	}

	m = new BPopUpMenu("Orientation");
	m->SetRadioMode(true);

	bounds.OffsetBy(0.0, fPageSizeMenu->Bounds().Height() + 10.0);
	fOrientationMenu = new BMenuField(bounds, "orientation", "Orientation:", m);
	panel->AddChild(fOrientationMenu);
	fOrientationMenu->ResizeToPreferred();
	fOrientationMenu->SetDivider(divider);
	fOrientationMenu->Menu()->SetLabelFromMarked(true);

	for (int32 i = 0; orientation[i].label != NULL; i++) {
	 	BMessage* message = new BMessage(ORIENTATION_CHANGED);
		message->AddInt32("orientation", orientation[i].orientation);
		BMenuItem* item = new BMenuItem(orientation[i].label, message);
		m->AddItem(item);

		if (orient == orientation[i].orientation)
			item->SetMarked(true);
	}

	float scale0;
	BString scale;
	if (fSetupMsg->FindFloat("scale", &scale0) == B_OK)
		scale << (int)scale0;
	else
		scale = "100";

	bounds.OffsetBy(0.0, fOrientationMenu->Bounds().Height() + 10.0);
	bounds.right -= 30.0;
	fScaleControl = new BTextControl(bounds, "scale", "Scale [%]:",
		scale.String(), NULL);
	panel->AddChild(fScaleControl);
	fScaleControl->ResizeToPreferred();
	fScaleControl->SetDivider(divider);

	for (uint32 i = 0; i < '0'; i++)
		fScaleControl->TextView()->DisallowChar(i);

	for (uint32 i = '9' + 1; i < 255; i++)
		fScaleControl->TextView()->DisallowChar(i);

	fScaleControl->TextView()->SetMaxBytes(3);

	bounds = Bounds();
	bounds.InsetBy(5.0, 0.0);
	bounds.top =
		MAX(fScaleControl->Frame().bottom, fMarginView->Frame().bottom) + 10.0;
	BBox *line = new BBox(BRect(bounds.left, bounds.top, bounds.right,
		bounds.top + 1.0), NULL, B_FOLLOW_LEFT_RIGHT);
	panel->AddChild(line);

	bounds.InsetBy(5.0, 0.0);
	bounds.OffsetBy(0.0, 11.0);
	BButton *cancel = new BButton(bounds, NULL, "Cancel", new BMessage(CANCEL_MSG));
	panel->AddChild(cancel);
	cancel->ResizeToPreferred();

	BButton *ok = new BButton(bounds, NULL, "OK", new BMessage(OK_MSG));
	panel->AddChild(ok, cancel);
	ok->ResizeToPreferred();

	bounds.right = fScaleControl->Frame().right;
	ok->MoveTo(bounds.right - ok->Bounds().Width(), ok->Frame().top);

	bounds = ok->Frame();
	cancel->MoveTo(bounds.left - cancel->Bounds().Width() - 10.0, bounds.top);

	ok->MakeDefault(true);
	ResizeTo(bounds.right + 10.0, bounds.bottom + 10.0);

	BRect winFrame(Frame());
	BRect screenFrame(BScreen().Frame());
	MoveTo((screenFrame.right - winFrame.right) / 2,
		(screenFrame.bottom - winFrame.bottom) / 2);
}


void
PageSetupWindow::UpdateSetupMessage()
{
	int32 orientation = 0;
	BMenuItem *item = fOrientationMenu->Menu()->FindMarked();
	if (item) {
		BMessage *msg = item->Message();
		msg->FindInt32("orientation", &orientation);
		SetInt32(fSetupMsg, "orientation", orientation);
	}

	// Save scaling factor
	float scale = atoi(fScaleControl->Text());
	if (scale <= 0.0) scale = 100.0;
	if (scale > 1000.0) scale = 1000.0;
	SetFloat(fSetupMsg, "scale", scale);

	float scaleR = 100.0 / scale;
	item = fPageSizeMenu->Menu()->FindMarked();
	if (item) {
		float w, h;
		BMessage *msg = item->Message();
		msg->FindFloat("width", &w);
		msg->FindFloat("height", &h);
		BRect r(0, 0, w, h);
		if (orientation != 0)
			r.Set(0, 0, h, w);

		SetRect(fSetupMsg, "preview:paper_rect", r);
		SetRect(fSetupMsg, "paper_rect", ScaleRect(r, scaleR));
		AddString(fSetupMsg, "preview:paper_size", item->Label());

		// Save the printable_rect
		BRect margin = fMarginView->GetMargin();
		if (orientation == 0) {
			margin.right = w - margin.right;
			margin.bottom = h - margin.bottom;
		} else {
			margin.right = h - margin.right;
			margin.bottom = w - margin.bottom;
		}
		SetRect(fSetupMsg, "preview:printable_rect", margin);
		SetRect(fSetupMsg, "printable_rect", ScaleRect(margin, scaleR));

		SetInt32(fSetupMsg, "units", fMarginView->GetMarginUnit());
	}

	SetInt32(fSetupMsg, "xres", 300);
	SetInt32(fSetupMsg, "yres", 300);
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
			BMenuItem *item = fOrientationMenu->Menu()->FindMarked();
			if (item) {
				int32 orientation = 0;
				item->Message()->FindInt32("orientation", &orientation);
				if (orientation == PrinterDriver::PORTRAIT_ORIENTATION) {
					fMarginView->SetPageSize(w, h);
				} else {
					fMarginView->SetPageSize(h, w);
				}
				fMarginView->UpdateView(MARGIN_CHANGED);
			}
		}	break;

		case ORIENTATION_CHANGED: {
			int32 orientation;
			msg->FindInt32("orientation", &orientation);

			BPoint p = fMarginView->GetPageSize();
			if (orientation == PrinterDriver::LANDSCAPE_ORIENTATION) {
				fMarginView->SetPageSize(p.y, p.x);
				fMarginView->UpdateView(MARGIN_CHANGED);
			}

			if (orientation == PrinterDriver::PORTRAIT_ORIENTATION) {
				fMarginView->SetPageSize(p.y, p.x);
				fMarginView->UpdateView(MARGIN_CHANGED);
			}
		}	break;

		default:
			BlockingWindow::MessageReceived(msg);
			break;
	}
}
