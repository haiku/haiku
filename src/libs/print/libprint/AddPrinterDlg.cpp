#include "AddPrinterDlg.h"

#include <GroupLayoutBuilder.h>

#include "PrinterCap.h"
#include "PrinterData.h"


enum MSGS {
	kMsgCancel = 1,
	kMsgOK,
	kMsgProtocolClassChanged,
};


ProtocolClassItem::ProtocolClassItem(const ProtocolClassCap* cap)
	: BStringItem(cap->fLabel.c_str())
	, fProtocolClassCap(cap)
{
}


int 
ProtocolClassItem::GetProtocolClass() const
{
	return fProtocolClassCap->fProtocolClass;
}

const char *
ProtocolClassItem::GetDescription() const
{
	return fProtocolClassCap->fDescription.c_str();
}


AddPrinterView::AddPrinterView(PrinterData* printerData,
	const PrinterCap* printerCap)
	:
	BView("addPrinter", B_WILL_DRAW),
	fPrinterData(printerData),
	fPrinterCap(printerCap)
{
}


AddPrinterView::~AddPrinterView()
{
}


void
AddPrinterView::AttachedToWindow()
{
	// protocol class box
	BBox* protocolClassBox = new BBox("protocolClass");
	protocolClassBox->SetLabel("Protocol Classes:");
	
	// protocol class
	fProtocolClassList = new BListView("protocolClassList");
	fProtocolClassList->SetExplicitMinSize(BSize(500, 200));
	BScrollView* protocolClassScroller = new BScrollView(
		"protocolClassListScroller", 
		fProtocolClassList, 
		0, 
		false, 
		true,
		B_NO_BORDER);
	fProtocolClassList->SetSelectionMessage(
		new BMessage(kMsgProtocolClassChanged));
	fProtocolClassList->SetTarget(this);
	
	protocolClassBox->AddChild(protocolClassScroller);

	int count = fPrinterCap->CountCap(PrinterCap::kProtocolClass);
	ProtocolClassCap **protocolClasses =
		(ProtocolClassCap **)fPrinterCap->GetCaps(PrinterCap::kProtocolClass);
	while (count--) {
		const ProtocolClassCap *protocolClass = *protocolClasses;
		
		BStringItem* item = new ProtocolClassItem(protocolClass);
		fProtocolClassList->AddItem(item);
		if (protocolClass->fIsDefault) {
			int index = fProtocolClassList->IndexOf(item);
			fProtocolClassList->Select(index);
		}
		protocolClasses ++;
	}
	
	// description of protocol class box
	BBox* descriptionBox = new BBox("descriptionBox");
	descriptionBox->SetLabel("Description:");

	// description of protocol class
	fDescription = new BTextView("description");
	fDescription->SetExplicitMinSize(BSize(200, 200));
	fDescription->SetViewColor(descriptionBox->ViewColor());
	BScrollView* descriptionScroller = new BScrollView("descriptionScroller",
			fDescription,
			0,
			false,
			true,
			B_NO_BORDER);
	fDescription->MakeEditable(false);

	descriptionBox->AddChild(descriptionScroller);

	// separator line
	BBox *separator = new BBox("separator");
	separator->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 1));

	// buttons
	BButton* cancel = new BButton("cancel", "Cancel",
		new BMessage(kMsgCancel));
	BButton* ok = new BButton("ok", "OK", new BMessage(kMsgOK));
	ok->MakeDefault(true);

	SetLayout(new BGroupLayout(B_VERTICAL));
	AddChild(BGroupLayoutBuilder(B_VERTICAL, 0)
		.Add(protocolClassBox)
		.Add(descriptionBox)
		.AddGlue()
		.Add(separator)
		.AddGroup(B_HORIZONTAL, 10, 1.0f)
			.AddGlue()
			.Add(cancel)
			.Add(ok)
		.End()
		.SetInsets(0, 0, 0, 0)
	);

	
	// update description
	BMessage updateDescription(kMsgProtocolClassChanged);
	MessageReceived(&updateDescription);
}


ProtocolClassItem*
AddPrinterView::CurrentSelection()
{
	int selected = fProtocolClassList->CurrentSelection();
	if (selected >= 0) {
		return (ProtocolClassItem*)fProtocolClassList->ItemAt(selected);
	}
	return NULL;
}


void
AddPrinterView::MessageReceived(BMessage* msg)
{
	if (msg->what == kMsgProtocolClassChanged) {
		ProtocolClassItem *item = CurrentSelection();
		if (item != NULL) {
			fDescription->SetText(item->GetDescription());
		}		
	} else {
		BView::MessageReceived(msg);
	}
}


void
AddPrinterView::Save()
{
	ProtocolClassItem* item = CurrentSelection();
	if (item != NULL) {
		fPrinterData->SetProtocolClass(item->GetProtocolClass());
		fPrinterData->Save();
	}
}


AddPrinterDlg::AddPrinterDlg(PrinterData* printerData,
	const PrinterCap *printerCap)
	:
	DialogWindow(BRect(100, 100, 120, 120),
		"Add Printer", B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS
			| B_AUTO_UPDATE_SIZE_LIMITS)
{
	SetResult(B_ERROR);

	fAddPrinterView = new AddPrinterView(printerData, printerCap);
	SetLayout(new BGroupLayout(B_VERTICAL));
	AddChild(BGroupLayoutBuilder(B_VERTICAL, 0)
		.Add(fAddPrinterView)
		.SetInsets(10, 10, 10, 10)
	);
}


void 
AddPrinterDlg::MessageReceived(BMessage* msg)
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
