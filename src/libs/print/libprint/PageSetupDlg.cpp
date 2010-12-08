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
#include <Debug.h>
#include <Font.h>
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
#include <String.h>
#include <SupportDefs.h>
#include <TextControl.h>
#include <View.h>


#include "DbgMsg.h"
#include "JobData.h"
#include "MarginView.h"
#include "PageSetupDlg.h"
#include "PrinterData.h"
#include "PrinterCap.h"
#include "PrintUtils.h"


using namespace std;


enum MSGS {
	kMsgCancel = 1,
	kMsgOK,
	kMsgOrientationChanged,
	kMsgPaperChanged,
};

PageSetupView::PageSetupView(JobData *job_data, PrinterData *printer_data,
	const PrinterCap *printer_cap)
	: BView("pageSetupView", B_WILL_DRAW)
	, fJobData(job_data)
	, fPrinterData(printer_data)
	, fPrinterCap(printer_cap)
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
	if (fJobData->GetOrientation() == orientation) {
		item->SetMarked(true);
	} else if (fOrientation->CountItems() == 1) {
		item->SetMarked(true);
	}
}

void
PageSetupView::AttachedToWindow()
{
	BMenuItem  *item = NULL;
	bool       marked;
	int        count;

	// margin
	MarginUnit units = fJobData->GetMarginUnit();
	BRect paper = fJobData->GetPaperRect();
	BRect margin = fJobData->GetPrintableRect();

	// re-calculate the margin from the printable rect in points
	margin.top -= paper.top;
	margin.left -= paper.left;
	margin.right = paper.right - margin.right;
	margin.bottom = paper.bottom - margin.bottom;

	fMarginView = new MarginView(
		paper.IntegerWidth(),
		paper.IntegerHeight(),
			margin, units);

	// paper selection
	marked = false;
	fPaper = new BPopUpMenu("paperSize");
	fPaper->SetRadioMode(true);
	count = fPrinterCap->CountCap(PrinterCap::kPaper);
	PaperCap **paper_cap = (PaperCap **)fPrinterCap->GetCaps(PrinterCap::kPaper);
	while (count--) {
		BMessage *msg = new BMessage(kMsgPaperChanged);
		msg->AddPointer("paperCap", *paper_cap);
		item = new BMenuItem((*paper_cap)->fLabel.c_str(), msg);
		fPaper->AddItem(item);
		item->SetTarget(this);
		if ((*paper_cap)->fPaper == fJobData->GetPaper()) {
			item->SetMarked(true);
			marked = true;
		}
		paper_cap++;
	}
	if (!marked)
		item->SetMarked(true);
	BMenuField* paperSize = new BMenuField("paperSize", "Paper Size:", fPaper);

	// orientation
	fOrientation = new BPopUpMenu("orientation");
	fOrientation->SetRadioMode(true);

	BMenuField* orientation = new BMenuField("orientation", "Orientation:", fOrientation);

	count = fPrinterCap->CountCap(PrinterCap::kOrientation);
	if (count == 0) {
		AddOrientationItem("Portrait", JobData::kPortrait);
		AddOrientationItem("Landscape", JobData::kLandscape);
	} else {
		OrientationCap **orientation_cap = (OrientationCap **)fPrinterCap->GetCaps(PrinterCap::kOrientation);
		while (count--) {
			AddOrientationItem((*orientation_cap)->fLabel.c_str(),
				(*orientation_cap)->fOrientation);
			orientation_cap++;
		}
	}

	// resolution
	marked = false;
	fResolution = new BPopUpMenu("resolution");
	fResolution->SetRadioMode(true);
	count = fPrinterCap->CountCap(PrinterCap::kResolution);
	ResolutionCap **resolution_cap = (ResolutionCap **)fPrinterCap->GetCaps(PrinterCap::kResolution);
	while (count--) {
		item = new BMenuItem((*resolution_cap)->fLabel.c_str(), NULL);
		fResolution->AddItem(item);
		item->SetTarget(this);
		if (((*resolution_cap)->fXResolution == fJobData->GetXres()) &&
			((*resolution_cap)->fYResolution == fJobData->GetYres())) {
			item->SetMarked(true);
			marked = true;
		}
		resolution_cap++;
	}
	if (!marked)
		item->SetMarked(true);
	BMenuField* resolution = new BMenuField("resolution", "Resolution:", fResolution);

	// scale
	BString scale;
	scale << (int)fJobData->GetScaling();
	fScaling = new BTextControl("scale", "Scale [%]:",
									scale.String(),
	                                NULL);
	int num;
	for (num = 0; num <= 255; num++) {
		fScaling->TextView()->DisallowChar(num);
	}
	for (num = 0; num <= 9; num++) {
		fScaling->TextView()->AllowChar('0' + num);
	}
	fScaling->TextView()->SetMaxBytes(3);

	// cancel and ok
	BButton* cancel = new BButton("cancel", "Cancel", new BMessage(kMsgCancel));
	BButton* ok = new BButton("ok", "OK", new BMessage(kMsgOK));

	ok->MakeDefault(true);

	BGridView* settings = new BGridView();
	BGridLayout* settingsLayout = settings->GridLayout();
	settingsLayout->AddItem(paperSize->CreateLabelLayoutItem(), 0, 0);
	settingsLayout->AddItem(paperSize->CreateMenuBarLayoutItem(), 1, 0);
	settingsLayout->AddItem(orientation->CreateLabelLayoutItem(), 0, 1);
	settingsLayout->AddItem(orientation->CreateMenuBarLayoutItem(), 1, 1);
	settingsLayout->AddItem(resolution->CreateLabelLayoutItem(), 0, 2);
	settingsLayout->AddItem(resolution->CreateMenuBarLayoutItem(), 1, 2);
	settingsLayout->AddItem(fScaling->CreateLabelLayoutItem(), 0, 3);
	settingsLayout->AddItem(fScaling->CreateTextViewLayoutItem(), 1, 3);
	settingsLayout->SetSpacing(0, 0);

	SetLayout(new BGroupLayout(B_VERTICAL));
	AddChild(BGroupLayoutBuilder(B_VERTICAL, 0)
		.AddGroup(B_HORIZONTAL, 5, 1.0f)
			.AddGroup(B_VERTICAL, 0, 1.0f)
				.Add(fMarginView)
				.AddGlue()
			.End()
			.AddGroup(B_VERTICAL, 0, 1.0f)
				.Add(settings)
				.AddGlue()
			.End()
		.End()
		.AddGroup(B_HORIZONTAL, 10, 1.0f)
			.AddGlue()
			.Add(cancel)
			.Add(ok)
		.End()
		.SetInsets(10, 10, 10, 10)
	);
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
		return (PaperCap*)fPrinterCap->GetDefaultCap(PrinterCap::kPaper);
	}
}

bool
PageSetupView::UpdateJobData()
{
	fJobData->SetOrientation(GetOrientation());

	PaperCap *paperCap = GetPaperCap();
	BRect paper_rect = paperCap->fPaperRect;
	BRect physical_rect = paperCap->fPhysicalRect;
	fJobData->SetPaper(paperCap->fPaper);

	const char *resolutionLabel = fResolution->FindMarked()->Label();
	const ResolutionCap* resolution = static_cast<const ResolutionCap*>(
		fPrinterCap->FindCap(PrinterCap::kResolution, resolutionLabel));
	ASSERT(resolution != NULL);
	if (resolution != NULL) {
		fJobData->SetXres(resolution->fXResolution);
		fJobData->SetYres(resolution->fYResolution);
		fJobData->SetResolutionID(resolution->ID());
	}

	// rotate paper and physical rectangle if landscape orientation
	if (JobData::kLandscape == fJobData->GetOrientation()) {
		swap(&paper_rect.left, &paper_rect.top);
		swap(&paper_rect.right, &paper_rect.bottom);
		swap(&physical_rect.left, &physical_rect.top);
		swap(&physical_rect.right, &physical_rect.bottom);
	}

	// adjust printable rect by margin
	fJobData->SetMarginUnit(fMarginView->Unit());
	BRect margin = fMarginView->Margin();
	BRect printable_rect;
	printable_rect.left = paper_rect.left + margin.left;
	printable_rect.top = paper_rect.top + margin.top;
	printable_rect.right = paper_rect.right - margin.right;
	printable_rect.bottom = paper_rect.bottom - margin.bottom;

	printable_rect.left = max_c(printable_rect.left, physical_rect.left);
	printable_rect.top = max_c(printable_rect.top, physical_rect.top);
	printable_rect.right = min_c(printable_rect.right, physical_rect.right);
	printable_rect.bottom = min_c(printable_rect.bottom, physical_rect.bottom);

	float scaling = atoi(fScaling->Text());
	if (scaling <= 0.0) { // sanity check
		scaling = 100.0;
	}
	if (scaling > 1000.0) {
		scaling = 1000.0;
	}

	float scalingR = 100.0 / scaling;

	fJobData->SetScaling(scaling);
	fJobData->SetPaperRect(paper_rect);
	fJobData->SetScaledPaperRect(ScaleRect(paper_rect, scalingR));
	fJobData->SetPrintableRect(printable_rect);
	fJobData->SetScaledPrintableRect(ScaleRect(printable_rect, scalingR));
	fJobData->SetPhysicalRect(physical_rect);
	fJobData->SetScaledPhysicalRect(ScaleRect(physical_rect, scalingR));

	fJobData->Save();
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
				float width = paperCap->fPaperRect.Width();
				float height = paperCap->fPaperRect.Height();
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

// TODO center window on screen
PageSetupDlg::PageSetupDlg(JobData *job_data, PrinterData *printer_data, const PrinterCap *printer_cap)
	: DialogWindow(BRect(100, 100, 160, 160),
		"Page Setup", B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE
			| B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE)
{
	AddShortcut('W',B_COMMAND_KEY,new BMessage(B_QUIT_REQUESTED));

	fPageSetupView = new PageSetupView(job_data, printer_data,
		printer_cap);

	SetLayout(new BGroupLayout(B_HORIZONTAL));
	AddChild(BGroupLayoutBuilder(B_HORIZONTAL, 0)
		.Add(fPageSetupView)
	);

	SetResult(B_ERROR);
}

void
PageSetupDlg::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
	case kMsgOK:
		Lock();
		fPageSetupView->UpdateJobData();
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

