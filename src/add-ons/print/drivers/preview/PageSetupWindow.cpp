/*
 * Copyright 2003-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 *		Simon Gauvin	
 *		Michael Pfeiffer
 *		Hartmut Reh
 */

#include <stdlib.h>

#include <InterfaceKit.h>
#include <SupportKit.h>
#include "PrinterDriver.h"
#include "PageSetupWindow.h"

#include "BeUtils.h"
#include "MarginView.h"

// static global variables
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


/**
 * Constuctor
 *
 * @param 
 * @return
 */
PageSetupWindow::PageSetupWindow(BMessage *msg, const char *printerName)
	:	BlockingWindow(BRect(0,0,400,220), "Page Setup", B_TITLED_WINDOW_LOOK,
 			B_MODAL_APP_WINDOW_FEEL, B_NOT_RESIZABLE | B_NOT_MINIMIZABLE |
 			B_NOT_ZOOMABLE)
{
	MoveTo(300, 300);

	fSetupMsg	= msg;
	
	if ( printerName ) {
		BString	title;
		
		title << printerName << " Page Setup";
		SetTitle( title.String() );

		// save the printer name
		fPrinterDirName = printerName;
	}

	// ---- Ok, build a default page setup user interface
	BRect		r(0, 0, letter_width, letter_height);
	BBox		*panel;
	BButton		*button;
	float		x, y, w, h;
	int         i;
	BMenuItem	*item;
	float       width, height;
	int32       orient;
	BRect		page;
	BRect		margin(0, 0, 0, 0);
	MarginUnit	units = kUnitInch;
	BString 	setting_value;

	// load orientation
	fSetupMsg->FindInt32("orientation", &orient);

	// load page rect
	if (fSetupMsg->FindRect("preview:paper_rect", &r) == B_OK) {
		width = r.Width();
		height = r.Height();
		page = r;
	} else {
		width = letter_width;
		height = letter_height;
		page.Set(0, 0, width, height);
	}
	
	// Load units
	int32 unitsValue;
	if (fSetupMsg->FindInt32("units", &unitsValue) == B_OK) {
		units = (MarginUnit)unitsValue;
	}
	
	// add a *dialog* background
	r = Bounds();
	panel = new BBox(r, "top_panel", B_FOLLOW_ALL, 
					B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP,
					B_PLAIN_BORDER);

	////////////// Create the margin view //////////////////////
	
	// re-calculate the margin from the printable rect in points
	margin = page;
	if (fSetupMsg->FindRect("preview:printable_rect", &margin) == B_OK) {
		margin.top -= page.top;
		margin.left -= page.left;
		margin.right = page.right - margin.right;
		margin.bottom = page.bottom - margin.bottom;
	} else {
		margin.Set(28.34, 28.34, 28.34, 28.34);		// 28.34 dots = 1cm
	}

	fMarginView = new MarginView(BRect(20,20,200,160), (int32)width, (int32)height,
			margin, units);
	panel->AddChild(fMarginView);
	
	// add page format menu
	// Simon Changed to OFFSET popups
	x = r.left + kMargin * 2 + kOffset; y = r.top + kMargin * 2;

	BPopUpMenu* m = new BPopUpMenu("page_size");
	m->SetRadioMode(true);

	// Simon changed width 200->140
	BMenuField *mf = new BMenuField(BRect(x, y, x + 140, y + 20), "page_size", 
		"Page Size:", m);
	fPageSizeMenu = mf;
	mf->ResizeToPreferred();
	mf->GetPreferredSize(&w, &h);

	// Simon added: SetDivider
	mf->SetDivider(be_plain_font->StringWidth("Page Size#"));

	panel->AddChild(mf);

	item = NULL;
	for (i = 0; pageFormat[i].label != NULL; i++) 
	{
		BMessage* msg = new BMessage('pgsz');
		msg->AddFloat("width", pageFormat[i].width);
		msg->AddFloat("height", pageFormat[i].height);
		BMenuItem* mi = new BMenuItem(pageFormat[i].label, msg);
		m->AddItem(mi);
	
		if (width == pageFormat[i].width && height == pageFormat[i].height) {
			item = mi; 
		}
		if (height == pageFormat[i].width && width == pageFormat[i].height) {
			item = mi; 
		}
	}
	mf->Menu()->SetLabelFromMarked(true);
	if (!item) {
		item = m->ItemAt(0);
	}
	item->SetMarked(true);
	mf->MenuItem()->SetLabel(item->Label());

	// add orientation menu
	y += h + kMargin;	 
	m = new BPopUpMenu("orientation");
	m->SetRadioMode(true);
	
	// Simon changed 200->140
	mf = new BMenuField(BRect(x, y, x + 140, y + 20), "orientation", "Orientation:", m);
	
	// Simon added: SetDivider
	mf->SetDivider(be_plain_font->StringWidth("Orientation#"));
		
	fOrientationMenu = mf;
	mf->ResizeToPreferred();
	panel->AddChild(mf);
	r.top += h;
	item = NULL;
	for (int i = 0; orientation[i].label != NULL; i++) 
	{
	 	BMessage* msg = new BMessage('ornt');		
		msg->AddInt32("orientation", orientation[i].orientation);
		BMenuItem* mi = new BMenuItem(orientation[i].label, msg);
		m->AddItem(mi);
		
		if (orient == orientation[i].orientation) {
			item = mi;
		}
	}
	mf->Menu()->SetLabelFromMarked(true);
// SHOULD BE REMOVED
	if (!item) {
		item = m->ItemAt(0);
	}
///////////////////
	item->SetMarked(true);
	mf->MenuItem()->SetLabel(item->Label());
	
	// add scale text control
	y += h + kMargin;	 
	BString scale;
	float scale0;
	if (fSetupMsg->FindFloat("scale", &scale0) == B_OK) {
		scale << (int)scale0;
	} else {
		scale = "100";
	}
	fScaleControl = new BTextControl(BRect(x, y, x + 100, y + 20), "scale", "Scale [%]:", 
									scale.String(),
	                                NULL, B_FOLLOW_RIGHT);
	int num;
	for (num = 0; num <= 255; num++) {
		fScaleControl->TextView()->DisallowChar(num);
	}
	for (num = 0; num <= 9; num++) {
		fScaleControl->TextView()->AllowChar('0' + num);
	}
	fScaleControl->TextView()->SetMaxBytes(3);

	panel->AddChild(fScaleControl);	

	// add a "OK" button, and make it default
	button 	= new BButton(r, NULL, "OK", new BMessage(OK_MSG), 
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	button->ResizeToPreferred();
	button->GetPreferredSize(&w, &h);
	x = r.right - w - 8;
	y = r.bottom - h - 8;
	button->MoveTo(x, y);
	panel->AddChild(button);
	button->MakeDefault(true);

	// add a "Cancel button	
	button 	= new BButton(r, NULL, "Cancel", new BMessage(CANCEL_MSG), 
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	button->GetPreferredSize(&w, &h);
	button->ResizeToPreferred();
	button->MoveTo(x - w - 8, y);
	panel->AddChild(button);

	// add a separator line...
	BBox * line = new BBox(BRect(r.left, y - 9, r.right, y - 8), NULL,
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM );
	panel->AddChild(line);

	// Finally, add our panel to window
	AddChild(panel);
}


// --------------------------------------------------
void 
PageSetupWindow::UpdateSetupMessage() 
{
	BMenuItem *item;
	int32 orientation = 0;

	item = fOrientationMenu->Menu()->FindMarked();
	if (item) {
		BMessage *msg = item->Message();
		msg->FindInt32("orientation", &orientation);
		SetInt32(fSetupMsg, "orientation", orientation);
	}

	// Save scaling factor
	float scale = atoi(fScaleControl->Text());
	if (scale <= 0.0) { // sanity check
		scale = 100.0;
	}
	if (scale > 1000.0) {
		scale = 1000.0;
	}
	SetFloat(fSetupMsg, "scale", scale);

	float scaleR = 100.0 / scale;

	item = fPageSizeMenu->Menu()->FindMarked();
	if (item) {
		float w, h;
		BMessage *msg = item->Message();
		msg->FindFloat("width", &w);
		msg->FindFloat("height", &h);
		BRect r;
		if (orientation == 0) {
			r.Set(0, 0, w, h);
		} else {
			r.Set(0, 0, h, w);
		}
		SetRect(fSetupMsg, "preview:paper_rect", r);
		SetRect(fSetupMsg, "paper_rect", ScaleRect(r, scaleR));
		
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

		// save the units used
		int32 units = fMarginView->GetMarginUnit();
		SetInt32(fSetupMsg, "units", units);
	}	
}


// --------------------------------------------------
void 
PageSetupWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what){
		case OK_MSG:
			UpdateSetupMessage();
			Quit(B_OK);
			break;
		
		case CANCEL_MSG:
			Quit(B_ERROR);
			break;

		// Simon added
		case 'pgsz':
			{
				float w, h;
				msg->FindFloat("width", &w);
				msg->FindFloat("height", &h);
				BMenuItem *item = fOrientationMenu->Menu()->FindMarked();
				if (item) {
					int32 orientation = 0;
					BMessage *m = item->Message();
					m->FindInt32("orientation", &orientation);
					if (orientation == PrinterDriver::PORTRAIT_ORIENTATION) {
						fMarginView->SetPageSize(w, h);
					} else {
						fMarginView->SetPageSize(h, w);
					}
					fMarginView->UpdateView(MARGIN_CHANGED);
				}
			}
			break;

		// Simon added
		case 'ornt':
			{	
				BPoint p = fMarginView->GetPageSize();
				int32 orientation;
				msg->FindInt32("orientation", &orientation);
				if (orientation == PrinterDriver::LANDSCAPE_ORIENTATION
					&& p.y > p.x) { 
					fMarginView->SetPageSize(p.y, p.x);
					fMarginView->UpdateView(MARGIN_CHANGED);
				}
				if (orientation == PrinterDriver::PORTRAIT_ORIENTATION
					&& p.x > p.y) {
					fMarginView->SetPageSize(p.y, p.x);
					fMarginView->UpdateView(MARGIN_CHANGED);
				}
			}
			break;
	
		default:
			inherited::MessageReceived(msg);
			break;
	}
}
			


