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
#include <TextControl.h>
#include <View.h>

#include "PageSetupDlg.h"
#include "JobData.h"
#include "PrinterData.h"
#include "PrinterCap.h"
#include "DbgMsg.h"

#if (!__MWERKS__ || defined(MSIPL_USING_NAMESPACE))
using namespace std;
#else 
#define std
#endif

#define	PAGESETUP_WIDTH		270
#define PAGESETUP_HEIGHT	125

#define	MENU_HEIGHT			16
#define BUTTON_WIDTH		70
#define BUTTON_HEIGHT		20

#define ORIENT_H			10
#define ORIENT_V			10
#define ORIENT_WIDTH		100
#define ORIENT_HEIGHT		60
#define PORT_TEXT			"Portrait"
#define RAND_TEXT			"Landscape"

#define PAPER_H				ORIENT_H + ORIENT_WIDTH + 15
#define PAPER_V				20
#define PAPER_WIDTH			200
#define PAPER_HEIGHT		MENU_HEIGHT
#define PAPER_TEXT			"Paper Size"

#define RES_H				PAPER_H
#define RES_V				PAPER_V + 24
#define RES_WIDTH			150
#define RES_HEIGHT			MENU_HEIGHT
#define RES_TEXT			"Resolution"

#define OK_H				(PAGESETUP_WIDTH  - BUTTON_WIDTH  - 11)
#define OK_V				(PAGESETUP_HEIGHT - BUTTON_HEIGHT - 11)
#define OK_TEXT				"OK"

#define CANCEL_H			(OK_H - BUTTON_WIDTH - 12)
#define CANCEL_V			OK_V
#define CANCEL_TEXT			"Cancel"

#define PRINT_LINE_V		(PAGESETUP_HEIGHT - BUTTON_HEIGHT - 23)

const BRect ORIENTAION_RECT(
	ORIENT_H,
	ORIENT_V,
	ORIENT_H + ORIENT_WIDTH,
	ORIENT_V + ORIENT_HEIGHT);

const BRect PORT_RECT(10, 15, 80, 14);
const BRect LAND_RECT(10, 35, 80, 14);

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
	kMsgOK
};

PageSetupView::PageSetupView(BRect frame, JobData *job_data, PrinterData *printer_data, const PrinterCap *printer_cap)
	: BView(frame, "", B_FOLLOW_ALL, B_WILL_DRAW), fJobData(job_data), fPrinterData(printer_data), fPrinterCap(printer_cap)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

PageSetupView::~PageSetupView()
{
}

void PageSetupView::AttachedToWindow()
{
	BMenuItem  *item = NULL;
	BMenuField *menufield;
	BButton    *button;
	bool       marked;
	int        count;

	/* orientaion */

	BBox *box = new BBox(ORIENTAION_RECT);
	AddChild(box);
	box->SetLabel("Orientation");

	fPortrait  = new BRadioButton(PORT_RECT, "", PORT_TEXT, NULL);
	box->AddChild(fPortrait);

	BRadioButton *landscape = new BRadioButton(LAND_RECT, "", RAND_TEXT, NULL);
	box->AddChild(landscape);

	if (JobData::kPortrait == fJobData->getOrientation())
		fPortrait->SetValue(B_CONTROL_ON);
	else
		landscape->SetValue(B_CONTROL_ON);

	/* paper selection */

	marked = false;
	fPaper = new BPopUpMenu("");
	fPaper->SetRadioMode(true);
	count = fPrinterCap->countCap(PrinterCap::kPaper);
	PaperCap **paper_cap = (PaperCap **)fPrinterCap->enumCap(PrinterCap::kPaper);
	while (count--) {
		item = new BMenuItem((*paper_cap)->label.c_str(), NULL);
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
	menufield = new BMenuField(PAPER_RECT, "", PAPER_TEXT, fPaper);
	AddChild(menufield);
	float width = StringWidth(PAPER_TEXT) + 7;
	menufield->SetDivider(width);

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
	menufield = new BMenuField(RESOLUTION_RECT, "", RES_TEXT, fResolution);
	AddChild(menufield);
	menufield->SetDivider(width);

	/* cancel */

	button = new BButton(CANCEL_RECT, "", CANCEL_TEXT, new BMessage(kMsgCancel));
	AddChild(button);

	/* ok */

	button = new BButton(OK_RECT, "", OK_TEXT, new BMessage(kMsgOK));
	AddChild(button);
	button->MakeDefault(true);
}

inline void swap(float *e1, float *e2)
{
	float e = *e1;
	*e1 = *e2;
	*e2 = e;
}

bool PageSetupView::UpdateJobData()
{
	if (B_CONTROL_ON == fPortrait->Value()) {
		fJobData->setOrientation(JobData::kPortrait);
	} else {
		fJobData->setOrientation(JobData::kLandscape);
	}

	BRect paper_rect;
	BRect printable_rect;

	int count;

	count = fPrinterCap->countCap(PrinterCap::kPaper);
	PaperCap **paper_cap = (PaperCap **)fPrinterCap->enumCap(PrinterCap::kPaper);
	const char *paper_label = fPaper->FindMarked()->Label();
	while (count--) {
		if (!strcmp((*paper_cap)->label.c_str(), paper_label)) {
			fJobData->setPaper((*paper_cap)->paper);
			paper_rect = (*paper_cap)->paper_rect;
			printable_rect = (*paper_cap)->printable_rect;
			break;
		}
		paper_cap++;
	}

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

	if (JobData::kLandscape == fJobData->getOrientation()) {
		swap(&paper_rect.left, &paper_rect.top);
		swap(&paper_rect.right, &paper_rect.bottom);
		swap(&printable_rect.left, &printable_rect.top);
		swap(&printable_rect.right, &printable_rect.bottom);
	}

	fJobData->setScaling(100.0);
	fJobData->setPaperRect(paper_rect);
	fJobData->setPrintableRect(printable_rect);

	fJobData->save();
	return true;
}

//====================================================================

PageSetupDlg::PageSetupDlg(JobData *job_data, PrinterData *printer_data, const PrinterCap *printer_cap)
	: BWindow(BRect(100, 100, 100 + PAGESETUP_WIDTH, 100 + PAGESETUP_HEIGHT),
		"Page Setup", B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE)
{
	fResult = 0;
/*
	ostringstream oss;
	oss << printer_data->get_printer_name() << " Setup";
	SetTitle(title.str().c_str());
*/

	Lock();
	PageSetupView *view = new PageSetupView(Bounds(), job_data, printer_data, printer_cap);
	AddChild(view);
	Unlock();

	fSemaphore = create_sem(0, "PageSetupSem");
}

PageSetupDlg::~PageSetupDlg()
{
}

bool PageSetupDlg::QuitRequested()
{
	fResult = B_ERROR;
	release_sem(fSemaphore);
	return true;
}

void PageSetupDlg::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
	case kMsgOK:
		Lock();
		((PageSetupView *)ChildAt(0))->UpdateJobData();
		Unlock();
		fResult = B_NO_ERROR;
		release_sem(fSemaphore);
		break;

	case kMsgCancel:
		fResult = B_ERROR;
		release_sem(fSemaphore);
		break;

	default:
		BWindow::MessageReceived(msg);
		break;
	}
}

int PageSetupDlg::Go()
{
	Show();
	acquire_sem(fSemaphore);
	delete_sem(fSemaphore);
	int value = fResult;
	Lock();
	Quit();
	return value;
}
