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
#include <GridView.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <MenuField.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <Screen.h>
#include <TextControl.h>


// static global variables
static struct
{
	const char *label;
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
	const char *label;
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
	: HWindow(BRect(0, 0, 200, 100), "Page setup", B_TITLED_WINDOW_LOOK,
 		B_MODAL_APP_WINDOW_FEEL,
 		B_NOT_RESIZABLE | B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE
			| B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	 fResult(B_ERROR),
	 fSetupMsg(msg),
	 fAdvancedSettings(*msg),
	 fPrinterDirName(printerName)
{
	fExitSem = create_sem(0, "PageSetup");

	if (printerName)
		SetTitle(BString(printerName).Append(" page setup").String());

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

	fMarginView = new MarginView(int32(width), int32(height), margin,
		MarginUnit(units));

	BPopUpMenu* pageSize = new BPopUpMenu("Page size");
	pageSize->SetRadioMode(true);

	fPageSizeMenu = new BMenuField("page_size", "Page size:", pageSize);
	fPageSizeMenu->Menu()->SetLabelFromMarked(true);

	for (int32 i = 0; pageFormat[i].label != NULL; i++) {
		BMessage* message = new BMessage(PAGE_SIZE_CHANGED);
		message->AddFloat("width", pageFormat[i].width);
		message->AddFloat("height", pageFormat[i].height);
		BMenuItem* item = new BMenuItem(pageFormat[i].label, message);
		pageSize->AddItem(item);

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

	BPopUpMenu* compatibility = new BPopUpMenu("PDF compatibility");
	compatibility->SetRadioMode(true);

	fPDFCompatibilityMenu = new BMenuField("pdf_compatibility",
		"PDF compatibility:", compatibility);
	fPDFCompatibilityMenu->Menu()->SetLabelFromMarked(true);

	for (int32 i = 0; pdf_compatibility[i] != NULL; i++) {
		BMenuItem* item = new BMenuItem(pdf_compatibility[i], NULL);
		compatibility->AddItem(item);
		if (setting_value == pdf_compatibility[i])
			item->SetMarked(true);
	}

	fPDFCompressionSlider = new BSlider("pdf_compression",
		"Compression:", NULL, 0, 9, B_HORIZONTAL);
	fPDFCompressionSlider->SetLimitLabels("None", "Best");
	fPDFCompressionSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fPDFCompressionSlider->SetValue(compression);

	BBox *separator = new BBox("separator");
	separator->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 1));

	BButton *cancel = new BButton("cancel", "Cancel", new BMessage(CANCEL_MSG));

	BButton *ok = new BButton("ok", "OK", new BMessage(OK_MSG));
	ok->MakeDefault(true);

	BButton *fontsButton = new BButton("fonts", "Fonts" B_UTF8_ELLIPSIS,
		new BMessage(FONTS_MSG));

	BButton* advancedButton = new BButton("advanced",
		"Advanced" B_UTF8_ELLIPSIS,
		new BMessage(ADVANCED_MSG));

	BGridView* settings = new BGridView();
	BGridLayout* settingsLayout = settings->GridLayout();
	settingsLayout->AddItem(fPageSizeMenu->CreateLabelLayoutItem(), 0, 0);
	settingsLayout->AddItem(fPageSizeMenu->CreateMenuBarLayoutItem(), 1, 0);
	settingsLayout->AddItem(fOrientationMenu->CreateLabelLayoutItem(), 0, 1);
	settingsLayout->AddItem(fOrientationMenu->CreateMenuBarLayoutItem(), 1, 1);
	settingsLayout->AddItem(fPDFCompatibilityMenu->CreateLabelLayoutItem(), 0, 2);
	settingsLayout->AddItem(fPDFCompatibilityMenu->CreateMenuBarLayoutItem(), 1, 2);
	settingsLayout->AddView(fPDFCompressionSlider, 0, 3, 2);
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
			.Add(fontsButton)
			.Add(advancedButton)
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

	while (acquire_sem(fExitSem) == B_INTERRUPTED) {
	}

	// Have to cache the value since we delete ourself on Quit()
	status_t result = fResult;
	if (Lock())
		Quit();

	return result;
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
