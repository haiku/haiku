/*
 * JobSetupDlg.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <string>
#include <fcntl.h>  
#include <unistd.h>
#include <sys/stat.h>
#include <math.h>

#include <Alert.h>
#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <Looper.h>
#include <MessageFilter.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <Point.h>
#include <PopUpMenu.h>
#include <PrintJob.h>
#include <RadioButton.h>
#include <Rect.h>
#include <Slider.h>
#include <TextControl.h>
#include <TextView.h>
#include <View.h>

#include "HalftoneView.h"
#include "JobSetupDlg.h"
#include "JobData.h"
#include "JSDSlider.h"
#include "PagesView.h"
#include "PrinterData.h"
#include "PrinterCap.h"
#include "DbgMsg.h"

#if (!__MWERKS__ || defined(MSIPL_USING_NAMESPACE))
using namespace std;
#else 
#define std
#endif

//#define	PRINT_COPIES		100

#define QUALITY_H			10
#define QUALITY_V			10
#define QUALITY_WIDTH		180

	#define BPP_H			10
	#define BPP_V			15
	#define BPP_WIDTH		120
	#define BPP_HEIGHT		16

	#define DITHER_H        BPP_H
	#define DITHER_V        BPP_V + BPP_HEIGHT + 10
	#define DITHER_WIDTH    BPP_WIDTH
	#define DITHER_HEIGHT   BPP_HEIGHT

	#define GAMMA_H			BPP_H
	#define GAMMA_V			DITHER_V + DITHER_HEIGHT + 10
	#define GAMMA_WIDTH		QUALITY_WIDTH - 20
	#define GAMMA_HEIGHT	55 // BPP_HEIGHT
	
	#define INK_DENSITY_H           BPP_H
	#define INK_DENSITY_V           GAMMA_V + GAMMA_HEIGHT + 5
	#define INK_DENSITY_WIDTH       GAMMA_WIDTH
	#define INK_DENSITY_HEIGHT      55 // BPP_HEIGHT
	
	#define HALFTONE_H      INK_DENSITY_H
	#define HALFTONE_V      INK_DENSITY_V + INK_DENSITY_HEIGHT + 5
	#define HALFTONE_WIDTH  160
	#define HALFTONE_HEIGHT 4*11 
	
#define QUALITY_HEIGHT		HALFTONE_V + HALFTONE_HEIGHT + 5

#define PAGERABGE_H			QUALITY_H
#define PAGERABGE_V			QUALITY_V + QUALITY_HEIGHT + 5
#define PAGERABGE_WIDTH		QUALITY_WIDTH
#define PAGERABGE_HEIGHT	70

	#define ALL_H				 10
	#define ALL_V				 20
	#define ALL_WIDTH			 36
	#define ALL_HEIGHT			 16

	#define SELECTION_H			ALL_H
	#define SELECTION_V			ALL_V + ALL_HEIGHT + 4
	#define SELECTION_WIDTH		16
	#define SELECTION_HEIGHT	16

	#define FROM_H				(SELECTION_H + SELECTION_WIDTH + 1)
	#define FROM_V				ALL_V + 19
	#define FROM_WIDTH			73
	#define FROM_HEIGHT			16

	#define TO_H				(FROM_H + FROM_WIDTH + 7)
	#define TO_V				FROM_V
	#define TO_WIDTH			59
	#define TO_HEIGHT			FROM_HEIGHT

#define	PRINT_WIDTH			365
#define PRINT_HEIGHT		QUALITY_HEIGHT + PAGERABGE_HEIGHT + 20 // 210

#define PAPERFEED_H			QUALITY_H + QUALITY_WIDTH + 10
#define PAPERFEED_V			QUALITY_V + 5
#define PAPERFEED_WIDTH		160
#define PAPERFEED_HEIGHT	16

#define NUP_H			 	PAPERFEED_H
#define NUP_V			 	PAPERFEED_V + PAPERFEED_HEIGHT + 7
#define NUP_WIDTH		 	PAPERFEED_WIDTH
#define NUP_HEIGHT		 	16
#define	MENU_HEIGHT			16

#define COPIES_H			PAPERFEED_H
#define COPIES_V			NUP_V + NUP_HEIGHT + 10
#define COPIES_WIDTH		140
#define COPIES_HEIGHT		16

#define DUPLEX_H			PAPERFEED_H
#define DUPLEX_V			COPIES_V + COPIES_HEIGHT + 7
#define DUPLEX_WIDTH		PAPERFEED_WIDTH
#define DUPLEX_HEIGHT		16

#define COLLATE_H			PAPERFEED_H
#define COLLATE_V			DUPLEX_V + DUPLEX_HEIGHT + 5
#define COLLATE_WIDTH		PAPERFEED_WIDTH
#define COLLATE_HEIGHT		16

#define REVERSE_H			PAPERFEED_H
#define REVERSE_V			COLLATE_V + COLLATE_HEIGHT + 5
#define REVERSE_WIDTH		PAPERFEED_WIDTH
#define REVERSE_HEIGHT		16

#define PAGES_H				PAPERFEED_H
#define PAGES_V				REVERSE_V + REVERSE_HEIGHT + 5
#define PAGES_WIDTH			PAPERFEED_WIDTH
#define PAGES_HEIGHT		40

#define PAGE_SELECTION_H      PAPERFEED_H
#define PAGE_SELECTION_V      PAGES_V + PAGES_HEIGHT + 5
#define PAGE_SELECTION_WIDTH  PAPERFEED_WIDTH
#define PAGE_SELECTION_HEIGHT 3 * 16 + 30

	#define PS_ALL_PAGES_H       10
	#define PS_ALL_PAGES_V       20
	#define PS_ALL_PAGES_WIDTH   PAGE_SELECTION_WIDTH - 20
	#define PS_ALL_PAGES_HEIGHT  16

	#define PS_ODD_PAGES_H       PS_ALL_PAGES_H
	#define PS_ODD_PAGES_V       PS_ALL_PAGES_V + PS_ALL_PAGES_HEIGHT
	#define PS_ODD_PAGES_WIDTH   PS_ALL_PAGES_WIDTH
	#define PS_ODD_PAGES_HEIGHT  PS_ALL_PAGES_HEIGHT

	#define PS_EVEN_PAGES_H       PS_ALL_PAGES_H
	#define PS_EVEN_PAGES_V       PS_ODD_PAGES_V + PS_ODD_PAGES_HEIGHT
	#define PS_EVEN_PAGES_WIDTH   PS_ALL_PAGES_WIDTH
	#define PS_EVEN_PAGES_HEIGHT  PS_ALL_PAGES_HEIGHT

#define PRINT_BUTTON_WIDTH	70
#define PRINT_BUTTON_HEIGHT	20

#define PRINT_LINE_V			(PRINT_HEIGHT - PRINT_BUTTON_HEIGHT - 23)

#define PRINT_OK_BUTTON_H		(PRINT_WIDTH - PRINT_BUTTON_WIDTH - 10)
#define PRINT_OK_BUTTON_V		(PRINT_HEIGHT - PRINT_BUTTON_HEIGHT - 11)

#define PRINT_CANCEL_BUTTON_H	(PRINT_OK_BUTTON_H - PRINT_BUTTON_WIDTH - 12)
#define PRINT_CANCEL_BUTTON_V	PRINT_OK_BUTTON_V

const BRect quality_rect(
	QUALITY_H,
	QUALITY_V,
	QUALITY_H + QUALITY_WIDTH,
	QUALITY_V + QUALITY_HEIGHT);

BRect bpp_rect(
	BPP_H,
	BPP_V,
	BPP_H + BPP_WIDTH,
	BPP_V + BPP_HEIGHT);

BRect dither_rect(
	DITHER_H,
	DITHER_V,
	DITHER_H + DITHER_WIDTH,
	DITHER_V + DITHER_HEIGHT);

const BRect ink_density_rect(
	INK_DENSITY_H,
	INK_DENSITY_V,
	INK_DENSITY_H + INK_DENSITY_WIDTH,
	INK_DENSITY_V + INK_DENSITY_HEIGHT);

const BRect halftone_rect(
	HALFTONE_H,
	HALFTONE_V,
	HALFTONE_H + HALFTONE_WIDTH,
	HALFTONE_V + HALFTONE_HEIGHT);

const BRect gamma_rect(
	GAMMA_H,
	GAMMA_V,
	GAMMA_H + GAMMA_WIDTH,
	GAMMA_V + GAMMA_HEIGHT);

const BRect pagerange_rect(
	PAGERABGE_H,
	PAGERABGE_V,
	PAGERABGE_H + PAGERABGE_WIDTH,
	PAGERABGE_V + PAGERABGE_HEIGHT);

const BRect all_button_rect(
	ALL_H,
	ALL_V,
	ALL_H + ALL_WIDTH,
	ALL_V + ALL_HEIGHT);

const BRect selection_rect(
	SELECTION_H,
	SELECTION_V,
	SELECTION_H + SELECTION_WIDTH,
	SELECTION_V + SELECTION_HEIGHT);

const BRect from_rect(
	FROM_H,
	FROM_V,
	FROM_H + FROM_WIDTH,
	FROM_V + FROM_HEIGHT);

const BRect to_rect(
	TO_H,
	TO_V,
	TO_H + TO_WIDTH,
	TO_V + TO_HEIGHT);

const BRect paperfeed_rect(
	PAPERFEED_H,
	PAPERFEED_V,
	PAPERFEED_H + PAPERFEED_WIDTH,
	PAPERFEED_V + PAPERFEED_HEIGHT);

BRect nup_rect(
	NUP_H,
	NUP_V,
	NUP_H + NUP_WIDTH,
	NUP_V + NUP_HEIGHT);

const BRect copies_rect(
	COPIES_H,
	COPIES_V,
	COPIES_H + COPIES_WIDTH,
	COPIES_V + COPIES_HEIGHT);

const BRect duplex_rect(
	DUPLEX_H,
	DUPLEX_V,
	DUPLEX_H + DUPLEX_WIDTH,
	DUPLEX_V + DUPLEX_HEIGHT);

const BRect collate_rect(
	COLLATE_H,
	COLLATE_V,
	COLLATE_H + COLLATE_WIDTH,
	COLLATE_V + COLLATE_HEIGHT);

const BRect reverse_rect(
	REVERSE_H,
	REVERSE_V,
	REVERSE_H + REVERSE_WIDTH,
	REVERSE_V + REVERSE_HEIGHT);

const BRect pages_rect(
	PAGES_H,
	PAGES_V,
	PAGES_H + PAGES_WIDTH,
	PAGES_V + PAGES_HEIGHT);

const BRect page_selection_rect(
	PAGE_SELECTION_H,
	PAGE_SELECTION_V,
	PAGE_SELECTION_H + PAGE_SELECTION_WIDTH,
	PAGE_SELECTION_V + PAGE_SELECTION_HEIGHT);

const BRect page_selection_all_pages_rect(
	PS_ALL_PAGES_H,
	PS_ALL_PAGES_V,
	PS_ALL_PAGES_H + PS_ALL_PAGES_WIDTH,
	PS_ALL_PAGES_V + PS_ALL_PAGES_HEIGHT);

const BRect page_selection_odd_pages_rect(
	PS_ODD_PAGES_H,
	PS_ODD_PAGES_V,
	PS_ODD_PAGES_H + PS_ODD_PAGES_WIDTH,
	PS_ODD_PAGES_V + PS_ODD_PAGES_HEIGHT);

const BRect page_selection_even_pages_rect(
	PS_EVEN_PAGES_H,
	PS_EVEN_PAGES_V,
	PS_EVEN_PAGES_H + PS_EVEN_PAGES_WIDTH,
	PS_EVEN_PAGES_V + PS_EVEN_PAGES_HEIGHT);

const BRect ok_rect(
	PRINT_OK_BUTTON_H,
	PRINT_OK_BUTTON_V,
	PRINT_OK_BUTTON_H + PRINT_BUTTON_WIDTH,
	PRINT_OK_BUTTON_V + PRINT_BUTTON_HEIGHT);

const BRect cancel_rect(
	PRINT_CANCEL_BUTTON_H,
	PRINT_CANCEL_BUTTON_V,
	PRINT_CANCEL_BUTTON_H + PRINT_BUTTON_WIDTH,
	PRINT_CANCEL_BUTTON_V + PRINT_BUTTON_HEIGHT);

struct SurfaceCap : public BaseCap {
	color_space surface_type;
	SurfaceCap(const string &s, bool d, color_space cs) : BaseCap(s, d), surface_type(cs) {}
};

struct NupCap : public BaseCap {
	int nup;
	NupCap(const string &s, bool d, int n) : BaseCap(s, d), nup(n) {}
};

struct DitherCap : public BaseCap {
	Halftone::DitherType dither_type;
	DitherCap(const string &s, bool d, Halftone::DitherType type) : BaseCap(s, d), dither_type(type) {}
};

static const SurfaceCap gRGB32("RGB32", false, B_RGB32);
static const SurfaceCap gCMAP8("CMAP8", true,  B_CMAP8);
static const SurfaceCap gGray8("GRAY8", false, B_GRAY8);
static const SurfaceCap gGray1("GRAY1", false, B_GRAY1);

static const NupCap gNup1("1", true,  1);
static const NupCap gNup2("2",   false, 2);
static const NupCap gNup4("4",   false, 4);
static const NupCap gNup8("8",   false, 8);
static const NupCap gNup9("9",   false, 9);
static const NupCap gNup16("16", false, 16);
static const NupCap gNup25("25", false, 25);
static const NupCap gNup32("32", false, 32);
static const NupCap gNup36("36", false, 36);

static const DitherCap gDitherType1("Crosshatch", false, Halftone::kType1);
static const DitherCap gDitherType2("Grid", false, Halftone::kType2);
static const DitherCap gDitherType3("Stipple", false, Halftone::kType3);
static const DitherCap gDitherFloydSteinberg("Floyd-Steinberg", false, Halftone::kTypeFloydSteinberg);

const SurfaceCap *gSurfaces[] = {
	&gRGB32,
	&gCMAP8,
	&gGray8,
	&gGray1
};

const NupCap *gNups[] = {
	&gNup1,
	&gNup2,
	&gNup4,
	&gNup8,
	&gNup9,
	&gNup16,
	&gNup25,
	&gNup32,
	&gNup36
};

const DitherCap *gDitherTypes[] = {
	&gDitherType1,
	&gDitherType2,
	&gDitherType3,
	&gDitherFloydSteinberg
};

enum {
	kMsgRangeAll = 'JSdl',
	kMsgRangeSelection,
	kMsgCancel,
	kMsgOK,
	kMsgQuality,
	kMsgCollateChanged,
	kMsgReverseChanged,
	kMsgDuplexChanged,
};

JobSetupView::JobSetupView(BRect frame, JobData *job_data, PrinterData *printer_data, const PrinterCap *printer_cap)
	: BView(frame, "", B_FOLLOW_ALL, B_WILL_DRAW), fJobData(job_data), fPrinterData(printer_data), fPrinterCap(printer_cap)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

BRadioButton* 
JobSetupView::AddPageSelectionItem(BView* parent, BRect rect, const char* name, const char* label, 
	JobData::PageSelection pageSelection)
{
	BRadioButton* button = new BRadioButton(rect, name, label, NULL);
	if (fJobData->getPageSelection() == pageSelection) {
		button->SetValue(B_CONTROL_ON);
	}
	parent->AddChild(button);
	return button;
}

void
JobSetupView::AllowOnlyDigits(BTextView* textView, int maxDigits)
{
	int num;
	for (num = 0; num <= 255; num++) {
		textView->DisallowChar(num);
	}
	for (num = 0; num <= 9; num++) {
		textView->AllowChar('0' + num);
	}
	textView->SetMaxBytes(maxDigits);
}

void 
JobSetupView::AttachedToWindow()
{
	BBox *box;
	BMenuItem  *item = NULL;
	BMenuField *menufield;
	BButton *button;
	float width;
	bool marked;
	int  count;

	/* quality */

	box = new BBox(quality_rect);
	AddChild(box);
	box->SetLabel("Quality");

/*
	// always B_RGB32
	fSurfaceType = new BPopUpMenu("");
	fSurfaceType->SetRadioMode(true);

	count = sizeof(gSurfaces) / sizeof(gSurfaces[0]);
	const SurfaceCap **surface_cap = gSurfaces;
	uint32 support_flags;
	while (count--) {
		if (bitmaps_support_space((*surface_cap)->surface_type, &support_flags)) {
			item = new BMenuItem((*surface_cap)->label.c_str(), NULL);
			fSurfaceType->AddItem(item);
			if ((*surface_cap)->surface_type == fJobData->getSurfaceType()) {
				item->SetMarked(true);
				marked = true;
			}
		}
		surface_cap++;
	}
	menufield = new BMenuField(bpp_rect, "", "Surface Type", fSurfaceType);
	box->AddChild(menufield);
	width = StringWidth("Color") + 10;
	menufield->SetDivider(width);
*/

	/* color */
	marked = false;
	fColorType = new BPopUpMenu("");
	fColorType->SetRadioMode(true);

	count = fPrinterCap->countCap(PrinterCap::kColor);
	const ColorCap **color_cap = (const ColorCap **)fPrinterCap->enumCap(PrinterCap::kColor);
	while (count--) {
		item = new BMenuItem((*color_cap)->label.c_str(), new BMessage(kMsgQuality));
		fColorType->AddItem(item);
		if ((*color_cap)->color == fJobData->getColor()) {
			item->SetMarked(true);
			marked = true;
		}
		color_cap++;
	}
	if (!marked && item)
		item->SetMarked(true);
	bpp_rect.right = bpp_rect.left + StringWidth("Color:") + fColorType->MaxContentWidth() + 10;
	menufield = new BMenuField(bpp_rect, "color", "Color:", fColorType);

	box->AddChild(menufield);
	width = StringWidth("Color:") + 10;
	menufield->SetDivider(width);
	fColorType->SetTargetForItems(this);
	
	/* dither type */
	marked = false;
	fDitherType = new BPopUpMenu("");
	fDitherType->SetRadioMode(true);

	count = sizeof(gDitherTypes) / sizeof(gDitherTypes[0]);
	const DitherCap **dither_cap = gDitherTypes;
	while (count--) {
		item = new BMenuItem((*dither_cap)->label.c_str(), new BMessage(kMsgQuality));
		fDitherType->AddItem(item);
		if ((*dither_cap)->dither_type == fJobData->getDitherType()) {
			item->SetMarked(true);
			marked = true;
		}
		dither_cap++;
	}
	if (!marked && item)
		item->SetMarked(true);
	dither_rect.right = dither_rect.left + StringWidth("Dot Pattern:") + fDitherType->MaxContentWidth() + 20;
	menufield = new BMenuField(dither_rect, "dithering", "Dot Pattern:", fDitherType);

	box->AddChild(menufield);
	width = StringWidth("Dot Pattern:") + 10;
	menufield->SetDivider(width);
	fDitherType->SetTargetForItems(this);
	
	/* halftone preview view */
	BRect rect(halftone_rect);
	BBox* halftoneBorder = new BBox(rect.InsetByCopy(-1, -1));
	box->AddChild(halftoneBorder);
	halftoneBorder->SetBorder(B_PLAIN_BORDER);
	
	fHalftone = new HalftoneView(rect.OffsetToCopy(1, 1), "halftone", B_FOLLOW_ALL, B_WILL_DRAW); 
	halftoneBorder->AddChild(fHalftone);
	fHalftone->preview(fJobData->getGamma(), fJobData->getInkDensity(), fJobData->getDitherType(), fJobData->getColor() == JobData::kColor);
	
	/* gamma */
	fGamma = new JSDSlider(gamma_rect, "gamma", "Gamma", new BMessage(kMsgQuality), -300, 300, B_BLOCK_THUMB);
	
	fGamma->SetLimitLabels("Lighter", "Darker");
	fGamma->SetValue((int32)(100 * log(fJobData->getGamma()) / log(2.0)));
	fGamma->SetHashMarks(B_HASH_MARKS_BOTH);
	fGamma->SetHashMarkCount(7);
	box->AddChild(fGamma);
	fGamma->SetModificationMessage(new BMessage(kMsgQuality));
	fGamma->SetTarget(this);

	/* ink density */
	fInkDensity = new JSDSlider(ink_density_rect, "inkDensity", "Ink Usage", new BMessage(kMsgQuality), 0, 127, B_BLOCK_THUMB);
	
	fInkDensity->SetLimitLabels("Min", "Max");
	fInkDensity->SetValue((int32)fJobData->getInkDensity());
	fInkDensity->SetHashMarks(B_HASH_MARKS_BOTH);
	fInkDensity->SetHashMarkCount(10);
	box->AddChild(fInkDensity);
	fInkDensity->SetModificationMessage(new BMessage(kMsgQuality));
	fInkDensity->SetTarget(this);

	/* page range */

	box = new BBox(pagerange_rect);
	AddChild(box);
	box->SetLabel("Page Range");

	fAll = new BRadioButton(all_button_rect, "all", "All", new BMessage(kMsgRangeAll));
	box->AddChild(fAll);

	BRadioButton *from = new BRadioButton(selection_rect, "selection", "", new BMessage(kMsgRangeSelection));
	box->AddChild(from);

	fFromPage = new BTextControl(from_rect, "from", "From", "", NULL);
	box->AddChild(fFromPage);
	fFromPage->SetAlignment(B_ALIGN_LEFT, B_ALIGN_RIGHT);
	fFromPage->SetDivider(StringWidth("From") + 7);
	AllowOnlyDigits(fFromPage->TextView(), 6);

	fToPage = new BTextControl(to_rect, "to", "To", "", NULL);
	box->AddChild(fToPage);
	fToPage->SetAlignment(B_ALIGN_LEFT, B_ALIGN_RIGHT);
	fToPage->SetDivider(StringWidth("To") + 7);
	AllowOnlyDigits(fToPage->TextView(), 6);

	int first_page = fJobData->getFirstPage();
	int last_page  = fJobData->getLastPage();

	if (first_page <= 1 && last_page <= 0) {
		fAll->SetValue(B_CONTROL_ON);
	} else {
		from->SetValue(B_CONTROL_ON);
		if (first_page < 1)
			first_page = 1;
		if (first_page > last_page)
			last_page = -1;

		ostringstream oss1;
		oss1 << first_page;
		fFromPage->SetText(oss1.str().c_str());

		ostringstream oss2;
		oss2 << last_page;
		fToPage->SetText(oss2.str().c_str());
	}

	fAll->SetTarget(this);
	from->SetTarget(this);

	/* paper source */

	marked = false;
	fPaperFeed = new BPopUpMenu("");
	fPaperFeed->SetRadioMode(true);
	count = fPrinterCap->countCap(PrinterCap::kPaperSource);
	const PaperSourceCap **paper_source_cap = (const PaperSourceCap **)fPrinterCap->enumCap(PrinterCap::kPaperSource);
	while (count--) {
		item = new BMenuItem((*paper_source_cap)->label.c_str(), NULL);
		fPaperFeed->AddItem(item);
		if ((*paper_source_cap)->paper_source == fJobData->getPaperSource()) {
			item->SetMarked(true);
			marked = true;
		}
		paper_source_cap++;
	}
	if (!marked)
		item->SetMarked(true);
	menufield = new BMenuField(paperfeed_rect, "paperSource", "Paper Source:", fPaperFeed);
	AddChild(menufield);
	width = StringWidth("Number of Copies:") + 7;
	menufield->SetDivider(width);

	/* Page Per Sheet */

	marked = false;
	fNup = new BPopUpMenu("");
	fNup->SetRadioMode(true);
	count = sizeof(gNups) / sizeof(gNups[0]);
	const NupCap **nup_cap = gNups;
	while (count--) {
		item = new BMenuItem((*nup_cap)->label.c_str(), NULL);
		fNup->AddItem(item);
		if ((*nup_cap)->nup == fJobData->getNup()) {
			item->SetMarked(true);
			marked = true;
		}
		nup_cap++;
	}
	if (!marked)
		item->SetMarked(true);
	menufield = new BMenuField(nup_rect, "pagePerSheet", "Pages Per Sheet:", fNup);
	menufield->SetDivider(StringWidth("Number of Copies:") + 7);
	AddChild(menufield);

	/* duplex */

	if (fPrinterCap->isSupport(PrinterCap::kPrintStyle)) {
		fDuplex = new BCheckBox(duplex_rect, "duplex", "Duplex", new BMessage(kMsgDuplexChanged));
		AddChild(fDuplex);
		if (fJobData->getPrintStyle() != JobData::kSimplex) {
			fDuplex->SetValue(B_CONTROL_ON);
		}
		fDuplex->SetTarget(this);
	} else {
		fDuplex = NULL;
	}

	/* copies */

	fCopies = new BTextControl(copies_rect, "copies", "Number of Copies:", "", NULL);
	AddChild(fCopies);
	fCopies->SetDivider(width);
	AllowOnlyDigits(fCopies->TextView(), 3);

	ostringstream oss4;
	oss4 << fJobData->getCopies();
	fCopies->SetText(oss4.str().c_str());

	/* collate */

	fCollate = new BCheckBox(collate_rect, "collate", "Collate", new BMessage(kMsgCollateChanged));
	fCollate->ResizeToPreferred();
	AddChild(fCollate);
	if (fJobData->getCollate()) {
		fCollate->SetValue(B_CONTROL_ON);
	}
	fCollate->SetTarget(this);

	/* reverse */

	fReverse = new BCheckBox(reverse_rect, "reverse", "Reverse Order", new BMessage(kMsgReverseChanged));
	fReverse->ResizeToPreferred();
	AddChild(fReverse);
	if (fJobData->getReverse()) {
		fReverse->SetValue(B_CONTROL_ON);
	}
	fReverse->SetTarget(this);

	/* pages view */

	fPages = new PagesView(pages_rect, "pages", B_FOLLOW_ALL, B_WILL_DRAW);
	AddChild(fPages);
	fPages->setCollate(fJobData->getCollate());
	fPages->setReverse(fJobData->getReverse());
	
	/* page selection */
	BBox* pageSelectionBox = new BBox(page_selection_rect);
	AddChild(pageSelectionBox);
	pageSelectionBox->SetLabel("Page Selection");
	
	fAllPages = AddPageSelectionItem(pageSelectionBox, page_selection_all_pages_rect, "allPages", "All Pages", JobData::kAllPages);
	fOddNumberedPages = AddPageSelectionItem(pageSelectionBox, page_selection_odd_pages_rect, "oddPages", "Odd-Numbered Pages", JobData::kOddNumberedPages);
	fEvenNumberedPages = AddPageSelectionItem(pageSelectionBox, page_selection_even_pages_rect, "evenPages", "Even-Numbered Pages", JobData::kEvenNumberedPages);

	/* cancel */

	button = new BButton(cancel_rect, "cancel", "Cancel", new BMessage(kMsgCancel));
	AddChild(button);

	/* ok */

	button = new BButton(ok_rect, "ok", "OK", new BMessage(kMsgOK));
	AddChild(button);
	button->MakeDefault(true);
	
	UpdateButtonEnabledState();
}

void
JobSetupView::UpdateButtonEnabledState()
{
	bool pageRangeEnabled = fAll->Value() != B_CONTROL_ON;
	fFromPage->SetEnabled(pageRangeEnabled);
	fToPage->SetEnabled(pageRangeEnabled);
	
	bool pageSelectionEnabled = fDuplex == NULL || 
		fDuplex->Value() != B_CONTROL_ON;
	fAllPages->SetEnabled(pageSelectionEnabled);
	fOddNumberedPages->SetEnabled(pageSelectionEnabled);
	fEvenNumberedPages->SetEnabled(pageSelectionEnabled);
}

void 
JobSetupView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
	case kMsgRangeAll:
	case kMsgRangeSelection:
	case kMsgDuplexChanged:
		UpdateButtonEnabledState();
		break;

	case kMsgQuality:
		fHalftone->preview(getGamma(), getInkDensity(), getDitherType(), getColor() == JobData::kColor); 
		break;

	case kMsgCollateChanged:
		fPages->setCollate(fCollate->Value() == B_CONTROL_ON);
		break;
	
	case kMsgReverseChanged:
		fPages->setReverse(fReverse->Value() == B_CONTROL_ON);
		break;	
	}
}

JobData::Color 
JobSetupView::getColor()
{
	int count = fPrinterCap->countCap(PrinterCap::kColor);
	const ColorCap **color_cap = (const ColorCap**)fPrinterCap->enumCap(PrinterCap::kColor);
	const char *color_label = fColorType->FindMarked()->Label();
	while (count--) {
		if (!strcmp((*color_cap)->label.c_str(), color_label)) {
			return (*color_cap)->color;
		}
		color_cap++;
	}
	return JobData::kMonochrome;
}

Halftone::DitherType 
JobSetupView::getDitherType()
{
	int count = sizeof(gDitherTypes) / sizeof(gDitherTypes[0]);
	const DitherCap **dither_cap = gDitherTypes;
	const char *dithering_label = fDitherType->FindMarked()->Label();
	while (count --) {
		if (strcmp((*dither_cap)->label.c_str(), dithering_label) == 0) {
			return (*dither_cap)->dither_type;
		}
		dither_cap ++;
	}
	return Halftone::kTypeFloydSteinberg;
}

float 
JobSetupView::getGamma()
{
	const float value = (float)fGamma->Value();
	return pow(2.0, value / 100.0);
}

float 
JobSetupView::getInkDensity()
{
	const float value = (float)(127 - fInkDensity->Value());
	return value;
}

bool 
JobSetupView::UpdateJobData()
{
	int count;

/*
	count = sizeof(gSurfaces) / sizeof(gSurfaces[0]);
	const SurfaceCap **surface_cap = gSurfaces;
	const char *surface_label = fSurfaceType->FindMarked()->Label();
	while (count--) {
		if (!strcmp((*surface_cap)->label.c_str(), surface_label)) {
			fJobData->setSurfaceType((*surface_cap)->surface_type);
			break;
		}
		surface_cap++;
	}
*/
	fJobData->setColor(getColor());	
	fJobData->setGamma(getGamma());
	fJobData->setInkDensity(getInkDensity());
	fJobData->setDitherType(getDitherType());

	int first_page;
	int last_page;

	if (B_CONTROL_ON == fAll->Value()) {
		first_page = 1;
		last_page  = -1;
	} else {
		first_page = atoi(fFromPage->Text());
		last_page  = atoi(fToPage->Text());
	}

	fJobData->setFirstPage(first_page);
	fJobData->setLastPage(last_page);

	count = fPrinterCap->countCap(PrinterCap::kPaperSource);
	const PaperSourceCap **paper_source_cap = (const PaperSourceCap **)fPrinterCap->enumCap(PrinterCap::kPaperSource);
	const char *paper_source_label = fPaperFeed->FindMarked()->Label();
	while (count--) {
		if (!strcmp((*paper_source_cap)->label.c_str(), paper_source_label)) {
			fJobData->setPaperSource((*paper_source_cap)->paper_source);
			break;
		}
		paper_source_cap++;
	}

	count = sizeof(gNups) / sizeof(gNups[0]);
	const NupCap **nup_cap = gNups;
	const char *nup_label = fNup->FindMarked()->Label();
	while (count--) {
		if (!strcmp((*nup_cap)->label.c_str(), nup_label)) {
			fJobData->setNup((*nup_cap)->nup);
			break;
		}
		nup_cap++;
	}

	if (fPrinterCap->isSupport(PrinterCap::kPrintStyle)) {
		fJobData->setPrintStyle((B_CONTROL_ON == fDuplex->Value()) ? JobData::kDuplex : JobData::kSimplex);
	}

	fJobData->setCopies(atoi(fCopies->Text()));

	fJobData->setCollate((B_CONTROL_ON == fCollate->Value()) ? true : false);
	fJobData->setReverse((B_CONTROL_ON == fReverse->Value()) ? true : false);

	JobData::PageSelection pageSelection = JobData::kAllPages;
	if (fOddNumberedPages->Value() == B_CONTROL_ON)
		pageSelection = JobData::kOddNumberedPages;
	if (fEvenNumberedPages->Value() == B_CONTROL_ON)
		pageSelection = JobData::kEvenNumberedPages;
	fJobData->setPageSelection(pageSelection);
	
	fJobData->save();
	return true;
}

//====================================================================

JobSetupDlg::JobSetupDlg(JobData *job_data, PrinterData *printer_data, const PrinterCap *printer_cap)
	: DialogWindow(BRect(100, 100, 100 + PRINT_WIDTH, 100 + PRINT_HEIGHT),
		"PrintJob Setup", B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)
{
	SetResult(B_ERROR);
	AddShortcut('W',B_COMMAND_KEY,new BMessage(B_QUIT_REQUESTED));
	
	fJobSetup = new JobSetupView(Bounds(), job_data, printer_data, printer_cap);
	AddChild(fJobSetup);
}

void 
JobSetupDlg::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
	case kMsgOK:
		fJobSetup->UpdateJobData();
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
