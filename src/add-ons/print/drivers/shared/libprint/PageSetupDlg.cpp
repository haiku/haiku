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
	M_CANCEL = 1,
	M_OK
};

PageSetupView::PageSetupView(BRect frame, JobData *job_data, PrinterData *printer_data, const PrinterCap *printer_cap)
	: BView(frame, "", B_FOLLOW_ALL, B_WILL_DRAW), __job_data(job_data), __printer_data(printer_data), __printer_cap(printer_cap)
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

	__portrait  = new BRadioButton(PORT_RECT, "", PORT_TEXT, NULL);
	box->AddChild(__portrait);

	BRadioButton *landscape = new BRadioButton(LAND_RECT, "", RAND_TEXT, NULL);
	box->AddChild(landscape);

	if (JobData::PORTRAIT == __job_data->getOrientation())
		__portrait->SetValue(B_CONTROL_ON);
	else
		landscape->SetValue(B_CONTROL_ON);

	/* paper selection */

	marked = false;
	__paper = new BPopUpMenu("");
	__paper->SetRadioMode(true);
	count = __printer_cap->countCap(PrinterCap::PAPER);
	PaperCap **paper_cap = (PaperCap **)__printer_cap->enumCap(PrinterCap::PAPER);
	while (count--) {
		item = new BMenuItem((*paper_cap)->label.c_str(), NULL);
		__paper->AddItem(item);
		item->SetTarget(this);
		if ((*paper_cap)->paper == __job_data->getPaper()) {
			item->SetMarked(true);
			marked = true;
		}
		paper_cap++;
	}
	if (!marked)
		item->SetMarked(true);
	menufield = new BMenuField(PAPER_RECT, "", PAPER_TEXT, __paper);
	AddChild(menufield);
	float width = StringWidth(PAPER_TEXT) + 7;
	menufield->SetDivider(width);

	/* resolution */

	marked = false;
	__resolution = new BPopUpMenu("");
	__resolution->SetRadioMode(true);
	count = __printer_cap->countCap(PrinterCap::RESOLUTION);
	ResolutionCap **resolution_cap = (ResolutionCap **)__printer_cap->enumCap(PrinterCap::RESOLUTION);
	while (count--) {
		item = new BMenuItem((*resolution_cap)->label.c_str(), NULL);
		__resolution->AddItem(item);
		item->SetTarget(this);
		if (((*resolution_cap)->xres == __job_data->getXres()) && ((*resolution_cap)->yres == __job_data->getYres())) {
			item->SetMarked(true);
			marked = true;
		}
		resolution_cap++;
	}
	if (!marked)
		item->SetMarked(true);
	menufield = new BMenuField(RESOLUTION_RECT, "", RES_TEXT, __resolution);
	AddChild(menufield);
	menufield->SetDivider(width);

	/* cancel */

	button = new BButton(CANCEL_RECT, "", CANCEL_TEXT, new BMessage(M_CANCEL));
	AddChild(button);

	/* ok */

	button = new BButton(OK_RECT, "", OK_TEXT, new BMessage(M_OK));
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
	if (B_CONTROL_ON == __portrait->Value()) {
		__job_data->setOrientation(JobData::PORTRAIT);
	} else {
		__job_data->setOrientation(JobData::LANDSCAPE);
	}

	BRect paper_rect;
	BRect printable_rect;

	int count;

	count = __printer_cap->countCap(PrinterCap::PAPER);
	PaperCap **paper_cap = (PaperCap **)__printer_cap->enumCap(PrinterCap::PAPER);
	const char *paper_label = __paper->FindMarked()->Label();
	while (count--) {
		if (!strcmp((*paper_cap)->label.c_str(), paper_label)) {
			__job_data->setPaper((*paper_cap)->paper);
			paper_rect = (*paper_cap)->paper_rect;
			printable_rect = (*paper_cap)->printable_rect;
			break;
		}
		paper_cap++;
	}

	count = __printer_cap->countCap(PrinterCap::RESOLUTION);
	ResolutionCap **resolution_cap = (ResolutionCap **)__printer_cap->enumCap(PrinterCap::RESOLUTION);
	const char *resolution_label = __resolution->FindMarked()->Label();
	while (count--) {
		if (!strcmp((*resolution_cap)->label.c_str(), resolution_label)) {
			__job_data->setXres((*resolution_cap)->xres);
			__job_data->setYres((*resolution_cap)->yres);
			break;
		}
		resolution_cap++;
	}

	if (JobData::LANDSCAPE == __job_data->getOrientation()) {
		swap(&paper_rect.left, &paper_rect.top);
		swap(&paper_rect.right, &paper_rect.bottom);
		swap(&printable_rect.left, &printable_rect.top);
		swap(&printable_rect.right, &printable_rect.bottom);
	}

	__job_data->setScaling(100.0);
	__job_data->setPaperRect(paper_rect);
	__job_data->setPrintableRect(printable_rect);

	__job_data->save();
	return true;
}

//====================================================================

PageSetupDlg::PageSetupDlg(JobData *job_data, PrinterData *printer_data, const PrinterCap *printer_cap)
	: BWindow(BRect(100, 100, 100 + PAGESETUP_WIDTH, 100 + PAGESETUP_HEIGHT),
		"Page Setup", B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE)
{
	__result = 0;
/*
	ostringstream oss;
	oss << printer_data->get_printer_name() << " Setup";
	SetTitle(title.str().c_str());
*/

	Lock();
	PageSetupView *view = new PageSetupView(Bounds(), job_data, printer_data, printer_cap);
	AddChild(view);
	Unlock();

	__semaphore = create_sem(0, "PageSetupSem");
}

PageSetupDlg::~PageSetupDlg()
{
}

bool PageSetupDlg::QuitRequested()
{
	__result = B_ERROR;
	release_sem(__semaphore);
	return true;
}

void PageSetupDlg::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
	case M_OK:
		Lock();
		((PageSetupView *)ChildAt(0))->UpdateJobData();
		Unlock();
		__result = B_NO_ERROR;
		release_sem(__semaphore);
		break;

	case M_CANCEL:
		__result = B_ERROR;
		release_sem(__semaphore);
		break;

	default:
		BWindow::MessageReceived(msg);
		break;
	}
}

int PageSetupDlg::Go()
{
	Show();
	acquire_sem(__semaphore);
	delete_sem(__semaphore);
	int value = __result;
	Lock();
	Quit();
	return value;
}
