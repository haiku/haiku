/*
 * JobSetupDlg.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "_sstream"
#include <string>
#include <fcntl.h>  
#include <unistd.h>
#include <sys/stat.h>

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
#include <TextControl.h>
#include <View.h>

#include "JobSetupDlg.h"
#include "JobData.h"
#include "PrinterData.h"
#include "PrinterCap.h"
#include "DbgMsg.h"

#if (!__MWERKS__ || defined(MSIPL_USING_NAMESPACE))
using namespace std;
#else 
#define std
#endif

//#define	PRINT_COPIES		100

#define	PRINT_WIDTH			355
#define PRINT_HEIGHT		210

#define QUALITY_H			10
#define QUALITY_V			10
#define QUALITY_WIDTH		180
#define QUALITY_HEIGHT		70

	#define BPP_H			10
	#define BPP_V			15
	#define BPP_WIDTH		140
	#define BPP_HEIGHT		16

	#define GAMMA_H			BPP_H
	#define GAMMA_V			BPP_V + BPP_HEIGHT + 10
	#define GAMMA_WIDTH		120
	#define GAMMA_HEIGHT	BPP_HEIGHT

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

const BRect bpp_rect(
	BPP_H,
	BPP_V,
	BPP_H + BPP_WIDTH,
	BPP_V + BPP_HEIGHT);

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

const BRect nup_rect(
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

const SurfaceCap rgb32("RGB32", false, B_RGB32);
const SurfaceCap cmap8("CMAP8", true,  B_CMAP8);
const SurfaceCap gray8("GRAY8", false, B_GRAY8);
const SurfaceCap gray1("GRAY1", false, B_GRAY1);

const NupCap nup1("Normal", true,  1);
const NupCap nup2("2-up",   false, 2);
const NupCap nup4("4-up",   false, 4);
const NupCap nup8("8-up",   false, 8);
const NupCap nup9("9-up",   false, 9);
const NupCap nup16("16-up", false, 16);
const NupCap nup25("25-up", false, 25);
const NupCap nup32("32-up", false, 32);
const NupCap nup36("36-up", false, 36);

const SurfaceCap *surfaces[] = {
	&rgb32,
	&cmap8,
	&gray8,
	&gray1
};

const NupCap *nups[] = {
	&nup1,
	&nup2,
	&nup4,
	&nup8,
	&nup9,
	&nup16,
	&nup25,
	&nup32,
	&nup36
};

enum {
	M_RANGE_ALL = 1,
	M_RANGE_SELECTION,
	M_CANCEL,
	M_OK
};

JobSetupView::JobSetupView(BRect frame, JobData *job_data, PrinterData *printer_data, const PrinterCap *printer_cap)
	: BView(frame, "", B_FOLLOW_ALL, B_WILL_DRAW), __job_data(job_data), __printer_data(printer_data), __printer_cap(printer_cap)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

void JobSetupView::AttachedToWindow()
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

	marked = false;
	__surface_type = new BPopUpMenu("");
	__surface_type->SetRadioMode(true);
	count = sizeof(surfaces) / sizeof(surfaces[0]);
	const SurfaceCap **surface_cap = surfaces;
	uint32 support_flags;
	while (count--) {
		if (bitmaps_support_space((*surface_cap)->surface_type, &support_flags)) {
			item = new BMenuItem((*surface_cap)->label.c_str(), NULL);
			__surface_type->AddItem(item);
			if ((*surface_cap)->surface_type == __job_data->getSurfaceType()) {
				item->SetMarked(true);
				marked = true;
			}
		}
		surface_cap++;
	}
	menufield = new BMenuField(bpp_rect, "", "Surface Type", __surface_type);
	box->AddChild(menufield);
	width = StringWidth("Surface Type") + 10;
	menufield->SetDivider(width);

	__gamma = new BTextControl(gamma_rect, "", "Gamma", "", NULL);
	box->AddChild(__gamma);
	__gamma->SetDivider(width);
	ostringstream oss3;
	oss3 << __job_data->getGamma();
	__gamma->SetText(oss3.str().c_str());

	/* page range */

	box = new BBox(pagerange_rect);
	AddChild(box);
	box->SetLabel("Page Range");

	__all = new BRadioButton(all_button_rect, "", "All", new BMessage(M_RANGE_ALL));
	box->AddChild(__all);

	BRadioButton *from = new BRadioButton(selection_rect, "", "", new BMessage(M_RANGE_SELECTION));
	box->AddChild(from);

	from_page = new BTextControl(from_rect, "", "From", "", NULL);
	box->AddChild(from_page);
	from_page->SetAlignment(B_ALIGN_LEFT, B_ALIGN_RIGHT);
	from_page->SetDivider(StringWidth("From") + 7);

	to_page = new BTextControl(to_rect, "", "To", "", NULL);
	box->AddChild(to_page);
	to_page->SetAlignment(B_ALIGN_LEFT, B_ALIGN_RIGHT);
	to_page->SetDivider(StringWidth("To") + 7);

	int first_page = __job_data->getFirstPage();
	int last_page  = __job_data->getLastPage();

	if (first_page <= 1 && last_page <= 0) {
		__all->SetValue(B_CONTROL_ON);
		from_page->SetEnabled(false);
		to_page->SetEnabled(false);
	} else {
		from->SetValue(B_CONTROL_ON);
		if (first_page < 1)
			first_page = 1;
		if (first_page > last_page)
			last_page = -1;

		ostringstream oss1;
		oss1 << first_page;
		from_page->SetText(oss1.str().c_str());

		ostringstream oss2;
		oss2 << last_page;
		to_page->SetText(oss2.str().c_str());
	}

	__all->SetTarget(this);
	from->SetTarget(this);

	/* paper source */

	marked = false;
	__paper_feed = new BPopUpMenu("");
	__paper_feed->SetRadioMode(true);
	count = __printer_cap->countCap(PrinterCap::PAPERSOURCE);
	const PaperSourceCap **paper_source_cap = (const PaperSourceCap **)__printer_cap->enumCap(PrinterCap::PAPERSOURCE);
	while (count--) {
		item = new BMenuItem((*paper_source_cap)->label.c_str(), NULL);
		__paper_feed->AddItem(item);
		if ((*paper_source_cap)->paper_source == __job_data->getPaperSource()) {
			item->SetMarked(true);
			marked = true;
		}
		paper_source_cap++;
	}
	if (!marked)
		item->SetMarked(true);
	menufield = new BMenuField(paperfeed_rect, "", "Paper Source", __paper_feed);
	AddChild(menufield);
	width = StringWidth("Number of Copies") + 7;
	menufield->SetDivider(width);

	/* Page Per Sheet */

	marked = false;
	__nup = new BPopUpMenu("");
	__nup->SetRadioMode(true);
	count = sizeof(nups) / sizeof(nups[0]);
	const NupCap **nup_cap = nups;
	while (count--) {
		item = new BMenuItem((*nup_cap)->label.c_str(), NULL);
		__nup->AddItem(item);
		if ((*nup_cap)->nup == __job_data->getNup()) {
			item->SetMarked(true);
			marked = true;
		}
		nup_cap++;
	}
	if (!marked)
		item->SetMarked(true);
	menufield = new BMenuField(nup_rect, "", "Page Per Sheet", __nup);
	menufield->SetDivider(width);
	AddChild(menufield);

	/* duplex */

	if (__printer_cap->isSupport(PrinterCap::PRINTSTYLE)) {
		__duplex = new BCheckBox(duplex_rect, "Duplex", "Duplex", NULL);
		AddChild(__duplex);
		if (__job_data->getPrintStyle() != JobData::SIMPLEX) {
			__duplex->SetValue(B_CONTROL_ON);
		}
	}

	/* copies */

	copies = new BTextControl(copies_rect, "", "Number of Copies", "", NULL);
	AddChild(copies);
	copies->SetDivider(width);

	ostringstream oss4;
	oss4 << __job_data->getCopies();
	copies->SetText(oss4.str().c_str());

	/* collate */

	__collate = new BCheckBox(collate_rect, "Collate", "Collate", NULL);
	AddChild(__collate);
	if (__job_data->getCollate()) {
		__collate->SetValue(B_CONTROL_ON);
	}

	/* reverse */

	__reverse = new BCheckBox(reverse_rect, "Reverse", "Reverse", NULL);
	AddChild(__reverse);
	if (__job_data->getReverse()) {
		__reverse->SetValue(B_CONTROL_ON);
	}

	/* cancel */

	button = new BButton(cancel_rect, "", "Cancel", new BMessage(M_CANCEL));
	AddChild(button);

	/* ok */

	button = new BButton(ok_rect, "", "OK", new BMessage(M_OK));
	AddChild(button);
	button->MakeDefault(true);
}

void JobSetupView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
	case M_RANGE_ALL:
		Window()->Lock();
		from_page->SetEnabled(false);
		to_page->SetEnabled(false);
		Window()->Unlock();
		break;

	case M_RANGE_SELECTION:
		Window()->Lock();
		from_page->SetEnabled(true);
		to_page->SetEnabled(true);
		Window()->Unlock();
		break;
	}
}

bool JobSetupView::UpdateJobData()
{
	int count;

	count = sizeof(surfaces) / sizeof(surfaces[0]);
	const SurfaceCap **surface_cap = surfaces;
	const char *surface_label = __surface_type->FindMarked()->Label();
	while (count--) {
		if (!strcmp((*surface_cap)->label.c_str(), surface_label)) {
			__job_data->setSurfaceType((*surface_cap)->surface_type);
			break;
		}
		surface_cap++;
	}

	__job_data->setGamma(atof(__gamma->Text()));

	int first_page;
	int last_page;

	if (B_CONTROL_ON == __all->Value()) {
		first_page = 1;
		last_page  = -1;
	} else {
		first_page = atoi(from_page->Text());
		last_page  = atoi(to_page->Text());
	}

	__job_data->setFirstPage(first_page);
	__job_data->setLastPage(last_page);

	count = __printer_cap->countCap(PrinterCap::PAPERSOURCE);
	const PaperSourceCap **paper_source_cap = (const PaperSourceCap **)__printer_cap->enumCap(PrinterCap::PAPERSOURCE);
	const char *paper_source_label = __paper_feed->FindMarked()->Label();
	while (count--) {
		if (!strcmp((*paper_source_cap)->label.c_str(), paper_source_label)) {
			__job_data->setPaperSource((*paper_source_cap)->paper_source);
			break;
		}
		paper_source_cap++;
	}

	count = sizeof(nups) / sizeof(nups[0]);
	const NupCap **nup_cap = nups;
	const char *nup_label = __nup->FindMarked()->Label();
	while (count--) {
		if (!strcmp((*nup_cap)->label.c_str(), nup_label)) {
			__job_data->setNup((*nup_cap)->nup);
			break;
		}
		nup_cap++;
	}

	if (__printer_cap->isSupport(PrinterCap::PRINTSTYLE)) {
		__job_data->setPrintStyle((B_CONTROL_ON == __duplex->Value()) ? JobData::DUPLEX : JobData::SIMPLEX);
	}

	__job_data->setCopies(atoi(copies->Text()));

	__job_data->setCollate((B_CONTROL_ON == __collate->Value()) ? true : false);
	__job_data->setReverse((B_CONTROL_ON == __reverse->Value()) ? true : false);

	__job_data->save();
	return true;
}

//====================================================================

filter_result PrintKeyFilter(BMessage *msg, BHandler **target, BMessageFilter *filter)
{

	BWindow *window = (BWindow *)filter->Looper();
	JobSetupView *view = (JobSetupView *)window->ChildAt(0);

	if ((*target != view->copies->ChildAt(0))    &&
		(*target != view->from_page->ChildAt(0)) &&
		(*target != view->to_page->ChildAt(0)))
	{
		return B_DISPATCH_MESSAGE;
	}

	ulong mods = msg->FindInt32("modifiers");
	if (mods & B_COMMAND_KEY)
		return B_DISPATCH_MESSAGE;

	const uchar *bytes = NULL;
	if (msg->FindString("bytes", (const char **)&bytes) != B_NO_ERROR)
		return B_DISPATCH_MESSAGE;

	long key =  bytes[0];
	if (key < '0') {
		if ((key != B_TAB) && (key != B_BACKSPACE) && (key != B_ENTER) &&
			(key != B_LEFT_ARROW) && (key != B_RIGHT_ARROW) &&
			(key != B_UP_ARROW) && (key != B_DOWN_ARROW))
			return B_SKIP_MESSAGE;
	}
	if (key > '9')
		return B_SKIP_MESSAGE;
	return B_DISPATCH_MESSAGE;
}

//====================================================================

JobSetupDlg::JobSetupDlg(JobData *job_data, PrinterData *printer_data, const PrinterCap *printer_cap)
	: BWindow(BRect(100, 100, 100 + PRINT_WIDTH, 100 + PRINT_HEIGHT),
		"PrintJob Setup", B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE)
{
	__result = 0;
/*
	ostringstream oss;
	oss << printer_data->get_printer_name() << " Print";
	SetTitle(oss.str().c_str());
*/
	Lock();
	JobSetupView *view = new JobSetupView(Bounds(), job_data, printer_data, printer_cap);
	AddChild(view);
	__filter = new BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE, B_KEY_DOWN, &PrintKeyFilter);
	AddCommonFilter(__filter);
	Unlock();

	__semaphore = create_sem(0, "JobSetupSem");
}

JobSetupDlg::~JobSetupDlg()
{
	Lock();
	RemoveCommonFilter(__filter);
	Unlock();
	delete __filter;
}

bool JobSetupDlg::QuitRequested()
{
	__result = B_ERROR;
	release_sem(__semaphore);
	return true;
}

void JobSetupDlg::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
	case M_OK:
		Lock();
		((JobSetupView *)ChildAt(0))->UpdateJobData();
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

int JobSetupDlg::Go()
{
	Show();
	acquire_sem(__semaphore);
	delete_sem(__semaphore);
	int value = __result;
	Lock();
	Quit();
	return value;
}
