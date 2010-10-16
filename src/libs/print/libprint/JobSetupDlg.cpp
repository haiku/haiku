/*
 * JobSetupDlg.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#include <cstdio>
#include <cstring>
#include <cstdlib>
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
#include <GridView.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
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
#include <String.h>
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
	kMsgPreview,
	kMsgCancel,
	kMsgOK,
	kMsgQuality,
	kMsgCollateChanged,
	kMsgReverseChanged,
	kMsgDuplexChanged,
};

JobSetupView::JobSetupView(JobData *job_data, PrinterData *printer_data,
	const PrinterCap *printer_cap)
	: BView("jobSetup", B_WILL_DRAW)
	, fJobData(job_data)
	, fPrinterData(printer_data)
	, fPrinterCap(printer_cap)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

BRadioButton* 
JobSetupView::CreatePageSelectionItem(const char* name, const char* label,
	JobData::PageSelection pageSelection)
{
	BRadioButton* button = new BRadioButton(name, label, NULL);
	if (fJobData->getPageSelection() == pageSelection) {
		button->SetValue(B_CONTROL_ON);
	}
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
	// quality
	BBox* qualityBox = new BBox("quality");
	qualityBox->SetLabel("Quality");

	// color
	fColorType = new BPopUpMenu("color");
	fColorType->SetRadioMode(true);

	int count = fPrinterCap->countCap(PrinterCap::kColor);
	const ColorCap **color_cap = (const ColorCap **)fPrinterCap->enumCap(
		PrinterCap::kColor);
	bool marked = false;
	BMenuItem* item = NULL;
	while (count--) {
		item = new BMenuItem((*color_cap)->label.c_str(),
			new BMessage(kMsgQuality));
		fColorType->AddItem(item);
		if ((*color_cap)->color == fJobData->getColor()) {
			item->SetMarked(true);
			marked = true;
		}
		color_cap++;
	}
	if (!marked && item)
		item->SetMarked(true);

	BMenuField* colorMenuField = new BMenuField("color", "Color:", fColorType);
	fColorType->SetTargetForItems(this);
	
	// dither type
	fDitherType = new BPopUpMenu("");
	fDitherType->SetRadioMode(true);

	count = sizeof(gDitherTypes) / sizeof(gDitherTypes[0]);
	const DitherCap **dither_cap = gDitherTypes;
	marked = false;
	item = NULL;
	while (count--) {
		item = new BMenuItem((*dither_cap)->label.c_str(),
			new BMessage(kMsgQuality));
		fDitherType->AddItem(item);
		if ((*dither_cap)->dither_type == fJobData->getDitherType()) {
			item->SetMarked(true);
			marked = true;
		}
		dither_cap++;
	}
	if (!marked && item)
		item->SetMarked(true);
	BMenuField* ditherMenuField = new BMenuField("dithering", "Dot Pattern:",
		fDitherType);
	fDitherType->SetTargetForItems(this);
	
	// halftone preview view
	BBox* halftoneBox = new BBox("halftoneBox");
	halftoneBox->SetBorder(B_PLAIN_BORDER);

	// TODO make layout compatible
	BSize size(240, 14 * 4);
	BRect rect(0, 0, size.width, size.height);
	fHalftone = new HalftoneView(rect, "halftone",
		B_FOLLOW_ALL, B_WILL_DRAW);
	fHalftone->SetExplicitMinSize(size);
	fHalftone->SetExplicitMaxSize(size);
	
	// gamma
	fGamma = new JSDSlider("gamma", "Gamma", new BMessage(kMsgQuality),
		-300, 300);
	
	fGamma->SetLimitLabels("Lighter", "Darker");
	fGamma->SetValue((int32)(100 * log(fJobData->getGamma()) / log(2.0)));
	fGamma->SetHashMarks(B_HASH_MARKS_BOTH);
	fGamma->SetHashMarkCount(7);
	fGamma->SetModificationMessage(new BMessage(kMsgQuality));
	fGamma->SetTarget(this);

	// ink density
	fInkDensity = new JSDSlider("inkDensity", "Ink Usage",
		new BMessage(kMsgQuality), 0, 127);
	
	fInkDensity->SetLimitLabels("Min", "Max");
	fInkDensity->SetValue((int32)fJobData->getInkDensity());
	fInkDensity->SetHashMarks(B_HASH_MARKS_BOTH);
	fInkDensity->SetHashMarkCount(10);
	fInkDensity->SetModificationMessage(new BMessage(kMsgQuality));
	fInkDensity->SetTarget(this);

	// page range

	BBox* pageRangeBox = new BBox("pageRange");
	pageRangeBox->SetLabel("Page Range");

	fAll = new BRadioButton("all", "Print all Pages", new BMessage(kMsgRangeAll));

	BRadioButton *range = new BRadioButton("selection", "Print selected Pages:",
		new BMessage(kMsgRangeSelection));

	fFromPage = new BTextControl("from", "From:", "", NULL);
	fFromPage->SetAlignment(B_ALIGN_LEFT, B_ALIGN_RIGHT);
	AllowOnlyDigits(fFromPage->TextView(), 6);

	fToPage = new BTextControl("to", "To:", "", NULL);
	fToPage->SetAlignment(B_ALIGN_LEFT, B_ALIGN_RIGHT);
	AllowOnlyDigits(fToPage->TextView(), 6);

	int first_page = fJobData->getFirstPage();
	int last_page  = fJobData->getLastPage();

	if (first_page <= 1 && last_page <= 0) {
		fAll->SetValue(B_CONTROL_ON);
	} else {
		range->SetValue(B_CONTROL_ON);
		if (first_page < 1)
			first_page = 1;
		if (first_page > last_page)
			last_page = -1;

		BString oss1;
		oss1 << first_page;
		fFromPage->SetText(oss1.String());

		BString oss2;
		oss2 << last_page;
		fToPage->SetText(oss2.String());
	}

	fAll->SetTarget(this);
	range->SetTarget(this);

	// paper source
	fPaperFeed = new BPopUpMenu("");
	fPaperFeed->SetRadioMode(true);
	count = fPrinterCap->countCap(PrinterCap::kPaperSource);
	const PaperSourceCap **paper_source_cap =
		(const PaperSourceCap **)fPrinterCap->enumCap(PrinterCap::kPaperSource);
	marked = false;
	item = NULL;
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
	BMenuField* paperSourceMenufield = new BMenuField("paperSource",
		"Paper Source:", fPaperFeed);

	// Pages per sheet
	fNup = new BPopUpMenu("");
	fNup->SetRadioMode(true);
	count = sizeof(gNups) / sizeof(gNups[0]);
	const NupCap **nup_cap = gNups;
	marked = false;
	item = NULL;
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
	BMenuField* pagesPerSheet = new BMenuField("pagesPerSheet",
		"Pages Per Sheet:", fNup);

	// duplex
	if (fPrinterCap->isSupport(PrinterCap::kPrintStyle)) {
		fDuplex = new BCheckBox("duplex", "Duplex",
			new BMessage(kMsgDuplexChanged));
		if (fJobData->getPrintStyle() != JobData::kSimplex) {
			fDuplex->SetValue(B_CONTROL_ON);
		}
		fDuplex->SetTarget(this);
	} else {
		fDuplex = NULL;
	}

	// copies
	fCopies = new BTextControl("copies", "Number of Copies:", "", NULL);
	AllowOnlyDigits(fCopies->TextView(), 3);

	BString copies;
	copies << fJobData->getCopies();
	fCopies->SetText(copies.String());

	// collate
	fCollate = new BCheckBox("collate", "Collate",
		new BMessage(kMsgCollateChanged));
	if (fJobData->getCollate()) {
		fCollate->SetValue(B_CONTROL_ON);
	}
	fCollate->SetTarget(this);

	// reverse
	fReverse = new BCheckBox("reverse", "Reverse Order",
		new BMessage(kMsgReverseChanged));
	if (fJobData->getReverse()) {
		fReverse->SetValue(B_CONTROL_ON);
	}
	fReverse->SetTarget(this);

	// pages view
	// TODO make layout API compatible
	fPages = new PagesView(BRect(0, 0, 150, 40), "pages", B_FOLLOW_ALL,
		B_WILL_DRAW);
	fPages->setCollate(fJobData->getCollate());
	fPages->setReverse(fJobData->getReverse());
	fPages->SetExplicitMinSize(BSize(150, 40));
	fPages->SetExplicitMaxSize(BSize(150, 40));
	
	// page selection
	BBox* pageSelectionBox = new BBox("pageSelection");
	pageSelectionBox->SetLabel("Page Selection");
	
	fAllPages = CreatePageSelectionItem("allPages", "All Pages",
		JobData::kAllPages);
	fOddNumberedPages = CreatePageSelectionItem("oddPages",
		"Odd-Numbered Pages", JobData::kOddNumberedPages);
	fEvenNumberedPages = CreatePageSelectionItem("evenPages",
		"Even-Numbered Pages", JobData::kEvenNumberedPages);

	// separator line
	BBox *separator = new BBox("separator");
	separator->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 1));

	// buttons
	BButton* preview = new BButton("preview", "Preview" B_UTF8_ELLIPSIS,
		new BMessage(kMsgPreview));
	BButton* cancel = new BButton("cancel", "Cancel",
		new BMessage(kMsgCancel));
	BButton* ok = new BButton("ok", "OK", new BMessage(kMsgOK));
	ok->MakeDefault(true);
	
	BGroupView* halftoneGroup = new BGroupView(B_VERTICAL, 0);
	BGroupLayout* halftoneLayout = halftoneGroup->GroupLayout();
	halftoneLayout->AddView(fHalftone);
	halftoneBox->AddChild(halftoneGroup);

	BGridView* qualityGrid = new BGridView();
	BGridLayout* qualityGridLayout = qualityGrid->GridLayout();
	qualityGridLayout->AddItem(colorMenuField->CreateLabelLayoutItem(), 0, 0);
	qualityGridLayout->AddItem(colorMenuField->CreateMenuBarLayoutItem(), 1, 0);
	qualityGridLayout->AddItem(ditherMenuField->CreateLabelLayoutItem(), 0, 1);
	qualityGridLayout->AddItem(ditherMenuField->CreateMenuBarLayoutItem(), 1,
		1);
	qualityGridLayout->AddView(fGamma, 0, 2, 2);
	qualityGridLayout->AddView(fInkDensity, 0, 3, 2);
	qualityGridLayout->AddView(halftoneBox, 0, 4, 2);
	qualityGridLayout->SetSpacing(0, 0);
	qualityGridLayout->SetInsets(5, 5, 5, 5);
	qualityBox->AddChild(qualityGrid);

	BGridView* pageRangeGrid = new BGridView();
	BGridLayout* pageRangeLayout = pageRangeGrid->GridLayout();
	pageRangeLayout->AddItem(fFromPage->CreateLabelLayoutItem(), 0, 0);
	pageRangeLayout->AddItem(fFromPage->CreateTextViewLayoutItem(), 1, 0);
	pageRangeLayout->AddItem(fToPage->CreateLabelLayoutItem(), 0, 1);
	pageRangeLayout->AddItem(fToPage->CreateTextViewLayoutItem(), 1, 1);
	pageRangeLayout->SetInsets(0, 0, 0, 0);
	pageRangeLayout->SetSpacing(0, 0);

	BGroupView* pageRangeGroup = new BGroupView(B_VERTICAL, 0);
	BGroupLayout* pageRangeGroupLayout = pageRangeGroup->GroupLayout();
	pageRangeGroupLayout->AddView(fAll);
	pageRangeGroupLayout->AddView(range);
	pageRangeGroupLayout->AddView(pageRangeGrid);
	pageRangeGroupLayout->SetInsets(5, 5, 5, 5);
	pageRangeBox->AddChild(pageRangeGroup);

	BGridView* settings = new BGridView();
	BGridLayout* settingsLayout = settings->GridLayout();
	settingsLayout->AddItem(paperSourceMenufield->CreateLabelLayoutItem(), 0,
		0);
	settingsLayout->AddItem(paperSourceMenufield->CreateMenuBarLayoutItem(), 1,
		0);
	settingsLayout->AddItem(pagesPerSheet->CreateLabelLayoutItem(), 0, 1);
	settingsLayout->AddItem(pagesPerSheet->CreateMenuBarLayoutItem(), 1, 1);
	int row = 2;
	if (fDuplex != NULL) {
		settingsLayout->AddView(fDuplex, 0, row, 2);
		row ++;
	}
	settingsLayout->AddItem(fCopies->CreateLabelLayoutItem(), 0, row);
	settingsLayout->AddItem(fCopies->CreateTextViewLayoutItem(), 1, row);
	settingsLayout->SetSpacing(0, 0);


	BGroupView* pageSelectionGroup = new BGroupView(B_VERTICAL, 0);
	BGroupLayout* groupLayout = pageSelectionGroup->GroupLayout();
	groupLayout->AddView(fAllPages);
	groupLayout->AddView(fOddNumberedPages);
	groupLayout->AddView(fEvenNumberedPages);
	groupLayout->SetInsets(5, 5, 5, 5);
	pageSelectionBox->AddChild(pageSelectionGroup);

	SetLayout(new BGroupLayout(B_VERTICAL));
	AddChild(BGroupLayoutBuilder(B_VERTICAL, 0)
		.AddGroup(B_HORIZONTAL, 10, 1.0f)
			.AddGroup(B_VERTICAL, 10, 1.0f)
				.Add(qualityBox)
				.Add(pageRangeBox)
				.AddGlue()
			.End()
			.AddGroup(B_VERTICAL, 0, 1.0f)
				.Add(settings)
				.AddStrut(5)
				.Add(fCollate)
				.Add(fReverse)
				.Add(fPages)
				.AddStrut(5)
				.Add(pageSelectionBox)
				.AddGlue()
			.End()
		.End()
		.AddGlue()
		.Add(separator)
		.AddGroup(B_HORIZONTAL, 10, 1.0f)
			.AddGlue()
			.Add(cancel)
			.Add(preview)
			.Add(ok)
		.End()
		.SetInsets(0, 0, 0, 0)
	);

	fHalftone->preview(fJobData->getGamma(), fJobData->getInkDensity(),
		fJobData->getDitherType(), fJobData->getColor() != JobData::kMonochrome);

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
		fHalftone->preview(getGamma(), getInkDensity(), getDitherType(), getColor() != JobData::kMonochrome); 
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
JobSetupView::UpdateJobData(bool showPreview)
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
	fJobData->setShowPreview(showPreview);
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

JobSetupDlg::JobSetupDlg(JobData *job_data, PrinterData *printer_data,
	const PrinterCap *printer_cap)
	: DialogWindow(BRect(100, 100, 200, 200),
		"PrintJob Setup", B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE
			| B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS)
{
	SetResult(B_ERROR);
	AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));
	
	fJobSetup = new JobSetupView(job_data, printer_data, printer_cap);
	SetLayout(new BGroupLayout(B_VERTICAL));
	AddChild(BGroupLayoutBuilder(B_VERTICAL, 0)
		.Add(fJobSetup)
		.SetInsets(10, 10, 10, 10)
	);
}

void 
JobSetupDlg::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
	case kMsgOK:
	case kMsgPreview:
		fJobSetup->UpdateJobData(msg->what == kMsgPreview);
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
