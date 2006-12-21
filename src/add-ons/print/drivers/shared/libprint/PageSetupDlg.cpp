/*
 * PageSetupDlg.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */
 
#include <string>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>  
#include <unistd.h>
#include <sys/stat.h>

#include <Alert.h>
#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <Font.h>
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
#include <String.h>
#include <SupportDefs.h>
#include <TextControl.h>
#include <View.h>

#include "BeUtils.h"
#include "DbgMsg.h"
#include "JobData.h"
#include "MarginView.h"
#include "PageSetupDlg.h"
#include "PrinterData.h"
#include "PrinterCap.h"

#if (!__MWERKS__ || defined(MSIPL_USING_NAMESPACE))
using namespace std;
#else 
#define std
#endif

#define	MENU_HEIGHT			16
#define MENU_WIDTH          200
#define BUTTON_WIDTH		70
#define BUTTON_HEIGHT		20
#define TEXT_HEIGHT         16

#define MARGIN_H            10
#define MARGIN_V            10
#define MARGIN_WIDTH        180
#define MARGIN_HEIGHT       140

#define	PAGESETUP_WIDTH		MARGIN_H + MARGIN_WIDTH + 15 + MENU_WIDTH + 10
#define PAGESETUP_HEIGHT	MARGIN_V + MARGIN_HEIGHT + BUTTON_HEIGHT + 20

#define PAPER_H				MARGIN_H + MARGIN_WIDTH + 15
#define PAPER_V				10
#define PAPER_WIDTH			MENU_WIDTH
#define PAPER_HEIGHT		MENU_HEIGHT
#define PAPER_TEXT			"Paper Size:"

#define ORIENT_H			PAPER_H
#define ORIENT_V			PAPER_V + 24
#define ORIENT_WIDTH		MENU_WIDTH
#define ORIENT_HEIGHT		MENU_HEIGHT
#define ORIENTATION_TEXT    "Orientation:"
#define PORTRAIT_TEXT		"Portrait"
#define LANDSCAPE_TEXT		"Landscape"

#define RES_H				PAPER_H
#define RES_V				ORIENT_V + 24
#define RES_WIDTH			MENU_WIDTH
#define RES_HEIGHT			MENU_HEIGHT
#define RES_TEXT			"Resolution:"

#define SCALE_H				PAPER_H
#define SCALE_V				RES_V + 24
#define SCALE_WIDTH			100
#define SCALE_HEIGHT		TEXT_HEIGHT
#define SCALE_TEXT			"Scale [%]:"

#define OK_H				(PAGESETUP_WIDTH  - BUTTON_WIDTH  - 11)
#define OK_V				(PAGESETUP_HEIGHT - BUTTON_HEIGHT - 11)
#define OK_TEXT				"OK"

#define CANCEL_H			(OK_H - BUTTON_WIDTH - 12)
#define CANCEL_V			OK_V
#define CANCEL_TEXT			"Cancel"

#define PRINT_LINE_V		(PAGESETUP_HEIGHT - BUTTON_HEIGHT - 23)

const BRect MARGIN_RECT(
	MARGIN_H,
	MARGIN_V,
	MARGIN_H + MARGIN_WIDTH,
	MARGIN_V + MARGIN_HEIGHT);

const BRect ORIENTATION_RECT(
	ORIENT_H,
	ORIENT_V,
	ORIENT_H + ORIENT_WIDTH,
	ORIENT_V + ORIENT_HEIGHT);

const BRect PAPER_RECT(
	PAPER_H,
	PAPER_V,
	PAPER_H + PAPER_WIDTH,
	PAPER_V + PAPER_HEIGHT);

const BRect RESOLUTION_RECT(
	RES_H,
	RES_V,
	RES_H + RES_WIDTH,
	RES_V + RES_HEIGHT);

const BRect SCALE_RECT(
	SCALE_H,
	SCALE_V,
	SCALE_H + SCALE_WIDTH,
	SCALE_V + SCALE_HEIGHT);

const BRect OK_RECT(
	OK_H,
	OK_V,
	OK_H + BUTTON_WIDTH,
	OK_V + BUTTON_HEIGHT);

const BRect CANCEL_RECT(
	CANCEL_H,
	CANCEL_V,
	CANCEL_H + BUTTON_WIDTH,
	CANCEL_V + BUTTON_HEIGHT);

enum MSGS {
	kMsgCancel = 1,
	kMsgOK,
	kMsgOrientationChanged,
	kMsgPaperChanged,
};

PageSetupView::PageSetupView(BRect frame, JobData *job_data, PrinterData *printer_data, const PrinterCap *printer_cap)
	: BView(frame, "", B_FOLLOW_ALL, B_WILL_DRAW), fJobData(job_data), fPrinterData(printer_data), fPrinterCap(printer_cap)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

PageSetupView::~PageSetupView()
{
}

void
PageSetupView::AddOrientationItem(const char* name, JobData::Orientation orientation)
{
	BMessage *msg = new BMessage(kMsgOrientationChanged);
	msg->AddInt32("orientation", orientation);
	BMenuItem *item = new BMenuItem(name, msg);
	
	fOrientation->AddItem(item);
	item->SetTarget(this);
	if (fJobData->getOrientation() == orientation) {
		item->SetMarked(true);
	} else if (fOrientation->CountItems() == 1) {
		item->SetMarked(true);
	}
}

void 
PageSetupView::AttachedToWindow()
{
	BMenuItem  *item = NULL;
	BMenuField *menuField;
	BButton    *button;
	bool       marked;
	int        count;

	/* margin */
	MarginUnit units = fJobData->getMarginUnit();
	BRect paper = fJobData->getPaperRect();
	BRect margin = fJobData->getPrintableRect();

	// re-calculate the margin from the printable rect in points
	margin.top -= paper.top;
	margin.left -= paper.left;
	margin.right = paper.right - margin.right;
	margin.bottom = paper.bottom - margin.bottom;

	fMarginView = new MarginView(MARGIN_RECT, 
		paper.IntegerWidth(), 
		paper.IntegerHeight(),
			margin, units);
	AddChild(fMarginView);

	/* paper selection */

	marked = false;
	fPaper = new BPopUpMenu("");
	fPaper->SetRadioMode(true);
	count = fPrinterCap->countCap(PrinterCap::kPaper);
	PaperCap **paper_cap = (PaperCap **)fPrinterCap->enumCap(PrinterCap::kPaper);
	while (count--) {
		BMessage *msg = new BMessage(kMsgPaperChanged);
		msg->AddPointer("paperCap", *paper_cap);
		item = new BMenuItem((*paper_cap)->label.c_str(), msg);
		fPaper->AddItem(item);
		item->SetTarget(this);
		if ((*paper_cap)->paper == fJobData->getPaper()) {
			item->SetMarked(true);
			marked = true;
		}
		paper_cap++;
	}
	if (!marked)
		item->SetMarked(true);
	menuField = new BMenuField(PAPER_RECT, "", PAPER_TEXT, fPaper);
	AddChild(menuField);
	float width = StringWidth(PAPER_TEXT) + 7;
	menuField->SetDivider(width);

	/* orientaion */
	fOrientation = new BPopUpMenu("orientation");
	fOrientation->SetRadioMode(true);
	
	menuField = new BMenuField(ORIENTATION_RECT, "orientation", ORIENTATION_TEXT, fOrientation);
	menuField->SetDivider(width);
	
	count = fPrinterCap->countCap(PrinterCap::kOrientation);
	if (count == 0) {
		AddOrientationItem(PORTRAIT_TEXT, JobData::kPortrait);
		AddOrientationItem(LANDSCAPE_TEXT, JobData::kLandscape);
	} else {
		OrientationCap **orientation_cap = (OrientationCap **)fPrinterCap->enumCap(PrinterCap::kOrientation);
		while (count--) {
			AddOrientationItem((*orientation_cap)->label.c_str(),
				(*orientation_cap)->orientation);
			orientation_cap++;
		}
	}
	
	AddChild(menuField);

	/* resolution */

	marked = false;
	fResolution = new BPopUpMenu("");
	fResolution->SetRadioMode(true);
	count = fPrinterCap->countCap(PrinterCap::kResolution);
	ResolutionCap **resolution_cap = (ResolutionCap **)fPrinterCap->enumCap(PrinterCap::kResolution);
	while (count--) {
		item = new BMenuItem((*resolution_cap)->label.c_str(), NULL);
		fResolution->AddItem(item);
		item->SetTarget(this);
		if (((*resolution_cap)->xres == fJobData->getXres()) && ((*resolution_cap)->yres == fJobData->getYres())) {
			item->SetMarked(true);
			marked = true;
		}
		resolution_cap++;
	}
	if (!marked)
		item->SetMarked(true);
	menuField = new BMenuField(RESOLUTION_RECT, "", RES_TEXT, fResolution);
	AddChild(menuField);
	menuField->SetDivider(width);

	/* scale */
	BString scale;
	scale << (int)fJobData->getScaling();
	fScaling = new BTextControl(SCALE_RECT, "scale", "Scale [%]:", 
									scale.String(),
	                                NULL, B_FOLLOW_RIGHT);
	int num;
	for (num = 0; num <= 255; num++) {
		fScaling->TextView()->DisallowChar(num);
	}
	for (num = 0; num <= 9; num++) {
		fScaling->TextView()->AllowChar('0' + num);
	}
	fScaling->TextView()->SetMaxBytes(3);
	fScaling->SetDivider(width);

	AddChild(fScaling);		

	/* cancel */

	button = new BButton(CANCEL_RECT, "", CANCEL_TEXT, new BMessage(kMsgCancel));
	AddChild(button);

	/* ok */

	button = new BButton(OK_RECT, "", OK_TEXT, new BMessage(kMsgOK));
	AddChild(button);
	button->MakeDefault(true);
}

inline void 
swap(float *e1, float *e2)
{
	float e = *e1;
	*e1 = *e2;
	*e2 = e;
}

JobData::Orientation 
PageSetupView::GetOrientation()
{
	BMenuItem *item = fOrientation->FindMarked();
	int32 orientation;
	if (item != NULL &&
		item->Message()->FindInt32("orientation", &orientation) == B_OK) {
		return (JobData::Orientation)orientation;
	} else {
		return JobData::kPortrait;
	}
}

PaperCap *
PageSetupView::GetPaperCap()
{
	BMenuItem *item = fPaper->FindMarked();
	void *pointer;
	if (item != NULL &&
		item->Message()->FindPointer("paperCap", &pointer) == B_OK) {
		return (PaperCap*)pointer;
	} else {
		return (PaperCap*)fPrinterCap->getDefaultCap(PrinterCap::kPaper);
	}
}

bool 
PageSetupView::UpdateJobData()
{
	fJobData->setOrientation(GetOrientation());

	PaperCap *paperCap = GetPaperCap();	
	BRect paper_rect = paperCap->paper_rect;
	BRect physical_rect = paperCap->physical_rect;
	fJobData->setPaper(paperCap->paper);

	int count;

	count = fPrinterCap->countCap(PrinterCap::kResolution);
	ResolutionCap **resolution_cap = (ResolutionCap **)fPrinterCap->enumCap(PrinterCap::kResolution);
	const char *resolution_label = fResolution->FindMarked()->Label();
	while (count--) {
		if (!strcmp((*resolution_cap)->label.c_str(), resolution_label)) {
			fJobData->setXres((*resolution_cap)->xres);
			fJobData->setYres((*resolution_cap)->yres);
			break;
		}
		resolution_cap++;
	}

	// adjust printable rect by margin
	fJobData->setMarginUnit(fMarginView->GetMarginUnit());
	BRect margin = fMarginView->GetMargin();
	BRect printable_rect = margin;
	printable_rect.right = paper_rect.right - margin.right;
	printable_rect.bottom = paper_rect.bottom - margin.bottom;

	printable_rect.left = max_c(printable_rect.left, physical_rect.left);
	printable_rect.top = max_c(printable_rect.top, physical_rect.top);
	printable_rect.right = min_c(printable_rect.right, physical_rect.right);
	printable_rect.bottom = min_c(printable_rect.bottom, physical_rect.bottom);


	if (JobData::kLandscape == fJobData->getOrientation()) {
		swap(&paper_rect.left, &paper_rect.top);
		swap(&paper_rect.right, &paper_rect.bottom);
		swap(&printable_rect.left, &printable_rect.top);
		swap(&printable_rect.right, &printable_rect.bottom);
		swap(&physical_rect.left, &physical_rect.top);
		swap(&physical_rect.right, &physical_rect.bottom);
	}

	float scaling = atoi(fScaling->Text());
	if (scaling <= 0.0) { // sanity check
		scaling = 100.0;
	}
	if (scaling > 1000.0) {
		scaling = 1000.0;
	}

	float scalingR = 100.0 / scaling;

	fJobData->setScaling(scaling);
	fJobData->setPaperRect(paper_rect);
	fJobData->setScaledPaperRect(ScaleRect(paper_rect, scalingR));
	fJobData->setPrintableRect(printable_rect);
	fJobData->setScaledPrintableRect(ScaleRect(printable_rect, scalingR));
	fJobData->setPhysicalRect(physical_rect);
	fJobData->setScaledPhysicalRect(ScaleRect(physical_rect, scalingR));	

	fJobData->save();
	return true;
}

void 
PageSetupView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case kMsgPaperChanged:
		case kMsgOrientationChanged:
			{
				JobData::Orientation orientation = GetOrientation();
				PaperCap *paperCap = GetPaperCap();
				float width = paperCap->paper_rect.Width();
				float height = paperCap->paper_rect.Height();
				if (orientation != JobData::kPortrait) {
					swap(&width, &height);
				}
				fMarginView->SetPageSize(width, height);
				fMarginView->UpdateView(MARGIN_CHANGED);
				
			}
			break;
	}
}

//====================================================================

PageSetupDlg::PageSetupDlg(JobData *job_data, PrinterData *printer_data, const PrinterCap *printer_cap)
	: DialogWindow(BRect(100, 100, 100 + PAGESETUP_WIDTH, 100 + PAGESETUP_HEIGHT),
		"Page Setup", B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE)
{
/*
	ostringstream oss;
	oss << printer_data->get_printer_name() << " Setup";
	SetTitle(title.str().c_str());
*/
	AddShortcut('W',B_COMMAND_KEY,new BMessage(B_QUIT_REQUESTED));

	PageSetupView *view = new PageSetupView(Bounds(), job_data, printer_data, printer_cap);
	AddChild(view);
	
	SetResult(B_ERROR);
}

void 
PageSetupDlg::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
	case kMsgOK:
		Lock();
		((PageSetupView *)ChildAt(0))->UpdateJobData();
		Unlock();
		SetResult(B_NO_ERROR);
		PostMessage(B_QUIT_REQUESTED);
		break;

	case kMsgCancel:
		PostMessage(B_QUIT_REQUESTED);
		break;

	default:
		DialogWindow::MessageReceived(msg);
	}
}

