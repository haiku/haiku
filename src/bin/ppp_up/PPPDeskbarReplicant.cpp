/*
 * Copyright 2004-2005, Waldemar Kornewald <Waldemar.Kornewald@web.de>
 * Distributed under the terms of the MIT License.
 */

#include "PPPDeskbarReplicant.h"

#include "PPPUpApplication.h"
#include "StatusWindow.h"
#include <PPPInterface.h>
#include <InterfaceUtils.h>

#include <Message.h>
#include <Deskbar.h>


// TODO: remove this
/*extern "C" _EXPORT BView *instantiate_deskbar_item();

BView*
instantiate_deskbar_item()
{
	return new PPPDeskbarReplicant();
}*/


static
status_t
destruction_thread(void *data)
{
	thread_id sender;
	ppp_report_packet report;
	int32 reportCode;
	while(true) {
		reportCode = receive_data(&sender, &report, sizeof(report));
		if(reportCode != PPP_REPORT_CODE)
			continue;
		
		if(report.type != PPP_DESTRUCTION_REPORT)
			continue;
		
		// our interface has been destroyed, remove the corresponding replicant
		// XXX: We do not know which ID the replicant has and there might exist
		// multiple connections (replicants)!
		BDeskbar().RemoveItem("PPPDeskbarReplicant");
	}
	
	return B_OK;
}


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
PPPDeskbarReplicant::Archive(BMessage *data, bool deep = true) const
{
	BView::Archive(data, deep);
	
	data->AddString("add_on", APP_SIGNATURE);
	data->AddInt32("interface", fID);
	return B_NO_ERROR;
}


void
PPPDeskbarReplicant::MouseDown(BPoint point)
{
	Looper()->CurrentMessage()->FindInt32("buttons", &fLastButtons);
	
	// TODO: on secondary mouse button we want to show a pop-up menu
}


void
PPPDeskbarReplicant::MouseUp(BPoint point)
{
	if(fLastButtons & B_PRIMARY_MOUSE_BUTTON) {
		fWindow->MoveTo(center_on_screen(fWindow->Frame(), fWindow));
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
	BRect rect(50,50,380,150);
	fWindow = new StatusWindow(rect, fID);
	
	// watch interface destruction
	PPPInterface interface(fID);
	if(interface.InitCheck() != B_OK)
		return;
	
	thread_id destructionThread = spawn_thread(destruction_thread,
		"destruction_thread", B_NORMAL_PRIORITY, NULL);
	interface.EnableReports(PPP_DESTRUCTION_REPORT, destructionThread);
}
