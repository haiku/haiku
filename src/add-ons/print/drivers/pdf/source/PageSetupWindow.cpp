/*

PDF Writer printer driver.

Version: 12.19.2000

Copyright (c) 2001 OpenBeOS.

Authors:
	Philippe Houdoin
	Simon Gauvin
	Michael Pfeiffer

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#include "PageSetupWindow.h"

#include "AdvancedSettingsWindow.h"
#include "Fonts.h"
#include "FontsWindow.h"
#include "BlockingWindow.h"
#include "MarginView.h"
#include "PrinterDriver.h"
#include "pdflib.h"				// for pageFormat constants
#include "PrintUtils.h"


#include <Box.h>
#include <Button.h>
#include <MenuField.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <Screen.h>
#include <TextControl.h>


// static global variables
static struct
{
	char *label;
	float width;
	float height;
} pageFormat[] =
{
	{ "Letter", letter_width, letter_height },
	{ "Legal",  legal_width,  legal_height  },
	{ "Ledger", ledger_width, ledger_height },
	{ "p11x17", p11x17_width, p11x17_height },
	{ "A0",     a0_width,     a0_height     },
	{ "A1",     a1_width,     a1_height     },
	{ "A2",     a2_width,     a2_height     },
	{ "A3",     a3_width,     a3_height     },
	{ "A4",     a4_width,     a4_height     },
	{ "A5",     a5_width,     a5_height     },
	{ "A6",     a6_width,     a6_height     },
	{ "B5",     b5_width,     b5_height     },
	{ NULL,     0.0,          0.0           }
};


static struct
{
	char *label;
	int32 orientation;
} orientation[] =
{
	{ "Portrait", PrinterDriver::PORTRAIT_ORIENTATION },
	{ "Landscape", PrinterDriver::LANDSCAPE_ORIENTATION },
	{ NULL, 0 }
};

// PDFLib 5.x does not support PDF 1.2 anymore!
static const char *pdf_compatibility[] = { "1.3", "1.4", NULL };


PageSetupWindow::PageSetupWindow(BMessage *msg, const char *printerName)
	: HWindow(BRect(0,0,400,220), "Page Setup", B_TITLED_WINDOW_LOOK,
 		B_MODAL_APP_WINDOW_FEEL, B_NOT_RESIZABLE | B_NOT_MINIMIZABLE |
 		B_NOT_ZOOMABLE),
	 fResult(B_ERROR),
	 fSetupMsg(msg),
	 fAdvancedSettings(*msg),
	 fPrinterDirName(printerName)
{
	fExitSem 	= create_sem(0, "PageSetup");

	if (printerName)
		SetTitle(BString(printerName).Append(" Page Setup").String());

	if (fSetupMsg->FindInt32("orientation", &fCurrentOrientation) != B_OK)
		fCurrentOrientation = PrinterDriver::PORTRAIT_ORIENTATION;

	BRect page;
	float width = letter_width;
	float height = letter_height;
	if (fSetupMsg->FindRect("paper_rect", &page) == B_OK) {
		width = page.Width();
		height = page.Height();
	} else {
		page.Set(0, 0, width, height);
	}

	BString label;
	if (fSetupMsg->FindString("pdf_paper_size", &label) != B_OK)
		label = "Letter";

	int32 compression;
	fSetupMsg->FindInt32("pdf_compression", &compression);

	int32 units;
	if (fSetupMsg->FindInt32("units", &units) != B_OK)
		units = kUnitInch;

	// re-calculate the margin from the printable rect in points
	BRect margin = page;
	if (fSetupMsg->FindRect("printable_rect", &margin) == B_OK) {
		margin.top -= page.top;
		margin.left -= page.left;
		margin.right = page.right - margin.right;
		margin.bottom = page.bottom - margin.bottom;
	} else {
		margin.Set(28.34, 28.34, 28.34, 28.34);		// 28.34 dots = 1cm
	}

	BString setting_value;
	if (fSetupMsg->FindString("pdf_compatibility", &setting_value) != B_OK)
		setting_value = "1.3";

	// Load font settings
	fFonts = new Fonts();
	fFonts->CollectFonts();
	BMessage fonts;
	if (fSetupMsg->FindMessage("fonts", &fonts) == B_OK)
		fFonts->SetTo(&fonts);

	// add a *dialog* background
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
	float divider = be_plain_font->StringWidth("PDF Compatibility: ");
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

		if (fCurrentOrientation == orientation[i].orientation)
			item->SetMarked(true);
	}

	m = new BPopUpMenu("PDF Compatibility");
	m->SetRadioMode(true);

	bounds.OffsetBy(0.0, fOrientationMenu->Bounds().Height() + 10.0);
	fPDFCompatibilityMenu = new BMenuField(bounds, "pdf_compatibility",
		"PDF Compatibility:", m);
	panel->AddChild(fPDFCompatibilityMenu);
	fPDFCompatibilityMenu->ResizeToPreferred();
	fPDFCompatibilityMenu->SetDivider(divider);
	fPDFCompatibilityMenu->Menu()->SetLabelFromMarked(true);

	for (int32 i = 0; pdf_compatibility[i] != NULL; i++) {
		BMenuItem* item = new BMenuItem(pdf_compatibility[i], NULL);
		m->AddItem(item);
		if (setting_value == pdf_compatibility[i])
			item->SetMarked(true);
	}

	bounds.OffsetBy(0.0, fPDFCompatibilityMenu->Bounds().Height() + 10.0);
	fPDFCompressionSlider = new BSlider(bounds, "pdf_compression",
		"Compression:", NULL, 0, 9);
	panel->AddChild(fPDFCompressionSlider);
	fPDFCompressionSlider->SetLimitLabels("None", "Best");
	fPDFCompressionSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fPDFCompressionSlider->SetValue(compression);
	fPDFCompressionSlider->ResizeToPreferred();

	bounds = Bounds();
	bounds.InsetBy(5.0, 0.0);
	bounds.top = MAX(fPDFCompressionSlider->Frame().bottom,
		fMarginView->Frame().bottom) + 10.0;
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

	bounds.right = fPDFCompressionSlider->Frame().right;
	ok->MoveTo(bounds.right - ok->Bounds().Width(), ok->Frame().top);

	bounds = ok->Frame();
	cancel->MoveTo(bounds.left - cancel->Bounds().Width() - 10.0, bounds.top);

	ok->MakeDefault(true);
	ResizeTo(bounds.right + 10.0, bounds.bottom + 10.0);

	BButton *button = new BButton(bounds, NULL, "Fonts" B_UTF8_ELLIPSIS,
		new BMessage(FONTS_MSG));
	panel->AddChild(button);
	button->ResizeToPreferred();
	button->MoveTo(fMarginView->Frame().left, bounds.top);

	bounds = button->Frame();
	button = new BButton(bounds, NULL, "Advanced" B_UTF8_ELLIPSIS,
		new BMessage(ADVANCED_MSG));
	panel->AddChild(button);
	button->ResizeToPreferred();
	button->MoveTo(bounds.right + 10, bounds.top);

	BRect winFrame(Frame());
	BRect screenFrame(BScreen().Frame());
	MoveTo((screenFrame.right - winFrame.right) / 2,
		(screenFrame.bottom - winFrame.bottom) / 2);
}


PageSetupWindow::~PageSetupWindow()
{
	delete_sem(fExitSem);
	delete fFonts;
}


void
PageSetupWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case OK_MSG: {
			_UpdateSetupMessage();
			fResult = B_OK;
			release_sem(fExitSem);
		}	break;

		case CANCEL_MSG: {
			fResult = B_ERROR;
			release_sem(fExitSem);
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

		case FONTS_MSG: {
			(new FontsWindow(fFonts))->Show();
		}	break;

		case ADVANCED_MSG: {
			(new AdvancedSettingsWindow(&fAdvancedSettings))->Show();
		}	break;

		default:
			inherited::MessageReceived(msg);
			break;
	}
}


bool
PageSetupWindow::QuitRequested()
{
	release_sem(fExitSem);
	return true;
}


status_t
PageSetupWindow::Go()
{
	MoveTo(300,300);
	Show();
	acquire_sem(fExitSem);
	Lock();
	Quit();

	return fResult;
}


void
PageSetupWindow::_UpdateSetupMessage()
{
	SetInt32(fSetupMsg, "orientation", fCurrentOrientation);

	BMenuItem *item = fPDFCompatibilityMenu->Menu()->FindMarked();
	if (item)
		SetString(fSetupMsg, "pdf_compatibility", item->Label());

	SetInt32(fSetupMsg, "pdf_compression", fPDFCompressionSlider->Value());

	item = fPageSizeMenu->Menu()->FindMarked();
	if (item) {
		float w, h;
		BMessage *msg = item->Message();
		msg->FindFloat("width", &w);
		msg->FindFloat("height", &h);
		BRect r(0, 0, w, h);
		if (fCurrentOrientation == PrinterDriver::LANDSCAPE_ORIENTATION)
			r.Set(0, 0, h, w);

		// Save the printable_rect
		BRect margin = fMarginView->Margin();
		if (fCurrentOrientation == PrinterDriver::PORTRAIT_ORIENTATION) {
			margin.right = w - margin.right;
			margin.bottom = h - margin.bottom;
		} else {
			margin.right = h - margin.right;
			margin.bottom = w - margin.bottom;
		}

		SetRect(fSetupMsg, "paper_rect", r);
		SetRect(fSetupMsg, "printable_rect", margin);
		SetInt32(fSetupMsg, "units", fMarginView->Unit());
		SetString(fSetupMsg, "pdf_paper_size", item->Label());
	}

	BMessage fonts;
	if (fFonts->Archive(&fonts) == B_OK) {
		fSetupMsg->RemoveName("fonts");
		fSetupMsg->AddMessage("fonts", &fonts);
	}

	// advanced settings
	BString value;
	if (fAdvancedSettings.FindString("pdflib_license_key", &value) == B_OK)
		SetString(fSetupMsg, "pdflib_license_key", value);

	bool webLinks;
	if (fAdvancedSettings.FindBool("create_web_links", &webLinks) == B_OK)
		SetBool(fSetupMsg, "create_web_links", webLinks);

	float linkBorder;
	if (fAdvancedSettings.FindFloat("link_border_width", &linkBorder) == B_OK)
		SetFloat(fSetupMsg, "link_border_width", linkBorder);

	bool createBookmarks;
	if (fAdvancedSettings.FindBool("create_bookmarks", &createBookmarks) == B_OK)
		SetBool(fSetupMsg, "create_bookmarks", createBookmarks);

	if (fAdvancedSettings.FindString("bookmark_definition_file", &value) == B_OK)
		SetString(fSetupMsg, "bookmark_definition_file", value);

	bool createXrefs;
	if (fAdvancedSettings.FindBool("create_xrefs", &createXrefs) == B_OK)
		SetBool(fSetupMsg, "create_xrefs", createXrefs);

	if (fAdvancedSettings.FindString("xrefs_file", &value) == B_OK)
		SetString(fSetupMsg, "xrefs_file", value.String());

	int32 closeOption;
	if (fAdvancedSettings.FindInt32("close_option", &closeOption) == B_OK)
		SetInt32(fSetupMsg, "close_option", closeOption);
}
