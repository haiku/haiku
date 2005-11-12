/*
 * Copyright 2005, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#include "PPPDeskbarReplicant.h"

#include "PPPUpApplication.h"
#include "PPPStatusWindow.h"
#include <PPPInterface.h>

#include <Deskbar.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>


// message constants
static const uint32 kMsgDisconnect = 'DISC';
static const uint32 kMsgStatus = 'STAT';

// labels
static const char *kLabelDisconnect = "Disconnect";
static const char *kLabelStatus = "Status";


PPPDeskbarReplicant::PPPDeskbarReplicant(ppp_interface_id id)
	: BView(BRect(0, 0, 15, 15), "PPPDeskbarReplicant", B_FOLLOW_NONE, 0),
	fID(id)
{
	Init();
}


PPPDeskbarReplicant::PPPDeskbarReplicant(BMessage *message)
	: BView(BRect(0, 0, 15, 15), "PPPDeskbarReplicant", B_FOLLOW_NONE, 0)
{
	message->FindInt32("interface", reinterpret_cast<int32*>(&fID));
	Init();
}


PPPDeskbarReplicant::~PPPDeskbarReplicant()
{
	delete fContextMenu;
	
	fWindow->LockLooper();
	fWindow->Quit();
}


PPPDeskbarReplicant*
PPPDeskbarReplicant::Instantiate(BMessage *data)
{
	if(!validate_instantiation(data, "PPPDeskbarReplicant"))
		return NULL;
	
	return new PPPDeskbarReplicant(data);
}


status_t
PPPDeskbarReplicant::Archive(BMessage *data, bool deep) const
{
	BView::Archive(data, deep);
	
	data->AddString("add_on", APP_SIGNATURE);
	data->AddInt32("interface", fID);
	return B_NO_ERROR;
}


void
PPPDeskbarReplicant::AttachedToWindow()
{
	BMenuItem *item = new BMenuItem(kLabelDisconnect, new BMessage(kMsgDisconnect));
	item->SetTarget(fWindow->StatusView());
	fContextMenu->AddItem(item);
	fContextMenu->AddSeparatorItem();
	item = new BMenuItem(kLabelStatus, new BMessage(kMsgStatus));
	item->SetTarget(this);
	fContextMenu->AddItem(item);
}


void
PPPDeskbarReplicant::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case PPP_REPORT_MESSAGE: {
			int32 type;
			message->FindInt32("type", &type);
			if(type == PPP_DESTRUCTION_REPORT)
				BDeskbar().RemoveItem(Name());
		} break;
		
		case kMsgStatus:
			fWindow->Show();
		break;
		
		default:
			BView::MessageReceived(message);
	}
}


void
PPPDeskbarReplicant::MouseDown(BPoint point)
{
	Looper()->CurrentMessage()->FindInt32("buttons", &fLastButtons);
	
	if(fLastButtons & B_SECONDARY_MOUSE_BUTTON) {
		ConvertToScreen(&point);
		fContextMenu->Go(point, true, true, true);
	}
}


void
PPPDeskbarReplicant::MouseUp(BPoint point)
{
	if(fLastButtons & B_PRIMARY_MOUSE_BUTTON) {
		// TODO: center on screen
//		fWindow->MoveTo(center_on_screen(fWindow->Frame(), fWindow));
		fWindow->Show();
	}
}


void
PPPDeskbarReplicant::Draw(BRect updateRect)
{
	// TODO: I want a nice blinking icon!
	MovePenTo(4, 12);
	DrawString("P");
}


void
PPPDeskbarReplicant::Init()
{
	BString name("PPPDeskbarReplicant");
	name << fID;
	SetName(name.String());
	
	BRect rect(50,50,380,150);
	fWindow = new PPPStatusWindow(rect, fID);
	
	fContextMenu = new BPopUpMenu("PPPContextMenu", false);
	
	// watch interface destruction
	PPPInterface interface(fID);
	if(interface.InitCheck() != B_OK)
		return;
}
