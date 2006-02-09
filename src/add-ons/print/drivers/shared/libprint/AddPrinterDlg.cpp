#include "AddPrinterDlg.h"

#include "PrinterCap.h"
#include "PrinterData.h"

#define DIALOG_WIDTH 400

#define BUTTON_WIDTH		70
#define BUTTON_HEIGHT		20
#define TEXT_HEIGHT         16

#define BOX_TOP_INSET       18
#define BOX_BOTTOM_INSET    10
#define BOX_H_INSET         10

#define PROTOCOL_CLASS_BOX_H      10
#define PROTOCOL_CLASS_BOX_V      10
#define PROTOCOL_CLASS_BOX_WIDTH  DIALOG_WIDTH - 20
#define PROTOCOL_CLASS_BOX_HEIGHT 125

#define PROTOCOL_CLASS_H          BOX_H_INSET
#define PROTOCOL_CLASS_V          BOX_TOP_INSET
#define PROTOCOL_CLASS_WIDTH      PROTOCOL_CLASS_BOX_WIDTH - 2 * BOX_H_INSET 
#define PROTOCOL_CLASS_HEIGHT     PROTOCOL_CLASS_BOX_HEIGHT - BOX_TOP_INSET - BOX_BOTTOM_INSET

#define DESCRIPTION_BOX_H         PROTOCOL_CLASS_BOX_H
#define DESCRIPTION_BOX_V         PROTOCOL_CLASS_BOX_V + PROTOCOL_CLASS_BOX_HEIGHT + 10
#define DESCRIPTION_BOX_WIDTH     PROTOCOL_CLASS_BOX_WIDTH
#define DESCRIPTION_BOX_HEIGHT    125

#define DESCRIPTION_H         BOX_H_INSET
#define DESCRIPTION_V         BOX_TOP_INSET
#define DESCRIPTION_WIDTH     DESCRIPTION_BOX_WIDTH - 2 * BOX_H_INSET 
#define DESCRIPTION_HEIGHT    DESCRIPTION_BOX_HEIGHT - BOX_TOP_INSET - BOX_BOTTOM_INSET

#define DIALOG_HEIGHT         DESCRIPTION_BOX_V + DESCRIPTION_BOX_HEIGHT + BUTTON_HEIGHT + 20

#define OK_H				(DIALOG_WIDTH  - BUTTON_WIDTH  - 11)
#define OK_V				(DIALOG_HEIGHT - BUTTON_HEIGHT - 11)
#define OK_TEXT				"OK"

#define CANCEL_H			(OK_H - BUTTON_WIDTH - 12)
#define CANCEL_V			OK_V
#define CANCEL_TEXT			"Cancel"

const BRect PROTOCOL_CLASS_BOX_RECT(
	PROTOCOL_CLASS_BOX_H,
	PROTOCOL_CLASS_BOX_V,
	PROTOCOL_CLASS_BOX_H + PROTOCOL_CLASS_BOX_WIDTH ,
	PROTOCOL_CLASS_BOX_V + PROTOCOL_CLASS_BOX_HEIGHT);

const BRect PROTOCOL_CLASS_RECT(
	PROTOCOL_CLASS_H,
	PROTOCOL_CLASS_V,
	PROTOCOL_CLASS_H + PROTOCOL_CLASS_WIDTH - B_V_SCROLL_BAR_WIDTH,
	PROTOCOL_CLASS_V + PROTOCOL_CLASS_HEIGHT);

const BRect DESCRIPTION_BOX_RECT(
	DESCRIPTION_BOX_H,
	DESCRIPTION_BOX_V,
	DESCRIPTION_BOX_H + DESCRIPTION_BOX_WIDTH,
	DESCRIPTION_BOX_V + DESCRIPTION_BOX_HEIGHT);
	
const BRect DESCRIPTION_RECT(
	DESCRIPTION_H,
	DESCRIPTION_V,
	DESCRIPTION_H + DESCRIPTION_WIDTH - B_V_SCROLL_BAR_WIDTH,
	DESCRIPTION_V + DESCRIPTION_HEIGHT);

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
	kMsgProtocolClassChanged,
};

ProtocolClassItem::ProtocolClassItem(const ProtocolClassCap* cap)
	: BStringItem(cap->label.c_str())
	, fProtocolClassCap(cap)
{
}

int 
ProtocolClassItem::getProtocolClass()
{
	return fProtocolClassCap->protocolClass;
}

const char *
ProtocolClassItem::getDescription()
{
	return fProtocolClassCap->description.c_str();
}


AddPrinterView::AddPrinterView(BRect frame, PrinterData *printer_data, const PrinterCap *printer_cap)
	: BView(frame, "", B_FOLLOW_ALL, B_WILL_DRAW), fPrinterData(printer_data), fPrinterCap(printer_cap)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

AddPrinterView::~AddPrinterView()
{
}

void
AddPrinterView::AttachedToWindow()
{
	/* protocol class box */
	BBox *box;
	box = new BBox(PROTOCOL_CLASS_BOX_RECT, NULL, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	box->SetLabel("Protocol Classes:");
	AddChild(box);
	
	/* protocol class */
	fProtocolClassList = new BListView(
		PROTOCOL_CLASS_RECT, 
		"protocolClassList", 
		B_SINGLE_SELECTION_LIST, 
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	box->AddChild(new BScrollView(
		"protocolClassListScroller", 
		fProtocolClassList, 
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, 
		0, 
		false, 
		true));
	fProtocolClassList->SetSelectionMessage(new BMessage(kMsgProtocolClassChanged));
	fProtocolClassList->SetTarget(this);
	
	int count = fPrinterCap->countCap(PrinterCap::kProtocolClass);
	ProtocolClassCap **protocolClasses = (ProtocolClassCap **)fPrinterCap->enumCap(PrinterCap::kProtocolClass);
	while (count--) {
		const ProtocolClassCap *protocolClass = *protocolClasses;
		
		BStringItem* item = new ProtocolClassItem(protocolClass);
		fProtocolClassList->AddItem(item);
		if (protocolClass->is_default) {
			int index = fProtocolClassList->IndexOf(item);
			fProtocolClassList->Select(index);
		}
		protocolClasses ++;
	}
	
	/* description of protocol class box */
	box = new BBox(DESCRIPTION_BOX_RECT, NULL, B_FOLLOW_ALL_SIDES);
	box->SetLabel("Description:");
	AddChild(box);

	/* description of protocol class */
	BRect textRect(DESCRIPTION_RECT);
	textRect.OffsetTo(0, 0);
	fDescription = new BTextView(DESCRIPTION_RECT, "description", textRect, B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	fDescription->SetViewColor(box->ViewColor());
	box->AddChild(new BScrollView("descriptionScroller", fDescription, 
		B_FOLLOW_ALL_SIDES, 0, false, true, B_NO_BORDER));
	fDescription->MakeEditable(false);

	/* cancel */
	BButton *button;
	button = new BButton(CANCEL_RECT, "", CANCEL_TEXT, new BMessage(kMsgCancel), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	AddChild(button);

	/* ok */

	button = new BButton(OK_RECT, "", OK_TEXT, new BMessage(kMsgOK), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	AddChild(button);
	button->MakeDefault(true);
	
	// update description
	BMessage updateDescription(kMsgProtocolClassChanged);
	MessageReceived(&updateDescription);
}

ProtocolClassItem *
AddPrinterView::CurrentSelection()
{
	int selected = fProtocolClassList->CurrentSelection();
	if (selected >= 0) {
		return (ProtocolClassItem*)fProtocolClassList->ItemAt(selected);
	}
	return NULL;
}

void
AddPrinterView::MessageReceived(BMessage *msg)
{
	if (msg->what == kMsgProtocolClassChanged) {
		ProtocolClassItem *item = CurrentSelection();
		if (item != NULL) {
			fDescription->SetText(item->getDescription());
		}		
	} else {
		BView::MessageReceived(msg);
	}
}

void
AddPrinterView::Save()
{
	ProtocolClassItem *item = CurrentSelection();
	if (item != NULL) {
		fPrinterData->setProtocolClass(item->getProtocolClass());
		fPrinterData->save();
	}
}

AddPrinterDlg::AddPrinterDlg(PrinterData *printerData, const PrinterCap *printerCap)
	: DialogWindow(BRect(100, 100, 100 + DIALOG_WIDTH, 100 + DIALOG_HEIGHT),
		"Add Printer", B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)
{
	SetResult(B_ERROR);

	// increase min. window size	
	float minWidth, maxWidth, minHeight, maxHeight;
	GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);
	minHeight = DIALOG_HEIGHT;
	minWidth = DIALOG_WIDTH;
	SetSizeLimits(minWidth, maxWidth, minHeight, maxHeight);
	
	fAddPrinterView = new AddPrinterView(Bounds(), printerData, printerCap);
	AddChild(fAddPrinterView);
}

void 
AddPrinterDlg::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
	case kMsgOK:
		fAddPrinterView->Save();
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
