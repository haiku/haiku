//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#include "PPPInterfaceListener.h"

#include "PPPInterface.h"

#include <Messenger.h>
#include <Handler.h>
#include <LockerHelper.h>


#define REPORT_FLAGS		PPP_WAIT_FOR_REPLY | PPP_NO_REPLY_TIMEOUT \
							| PPP_ALLOW_ANY_REPLY_THREAD

#define QUIT_REPORT_THREAD	'QUIT'


class PPPInterfaceListenerThread {
	public:
		PPPInterfaceListenerThread(PPPInterfaceListener *listener)
			: fListener(listener) {}
		
		status_t Run();

	private:
		PPPInterfaceListener *fListener;
};


status_t
PPPInterfaceListenerThread::Run()
{
	ppp_report_packet packet;
	ppp_interface_id *interfaceID;
	int32 code;
	thread_id sender;
	
	BMessage message;
	bool sendMessage;
	
	while(true) {
		code = receive_data(&sender, &packet, sizeof(packet));
		
		if(code == QUIT_REPORT_THREAD)
			break;
		else if(code != PPP_REPORT_CODE)
			continue;
		
		BMessenger messenger(fListener->Target());
		sendMessage = messenger.IsValid();
		
		if(sendMessage) {
			message.MakeEmpty();
			
			message.what = PPP_REPORT_MESSAGE;
			message.AddInt32("sender", sender);
			message.AddInt32("type", packet.type);
			message.AddInt32("code", packet.code);
			
			if(packet.length >= sizeof(ppp_interface_id)
					&& ((packet.type == PPP_MANAGER_REPORT
							&& packet.code == PPP_REPORT_INTERFACE_CREATED)
						|| packet.type >= PPP_INTERFACE_REPORT_TYPE_MIN)) {
				interfaceID = reinterpret_cast<ppp_interface_id*>(packet.data);
				message.AddInt32("interface", static_cast<int32>(*interfaceID));
			}
			
			// We might cause a dead-lock. Thus, abort if we cannot get the lock.
			BHandler *noHandler = NULL;
				// needed to tell compiler which version of SendMessage we want
			if(messenger.SendMessage(&message, noHandler, 100000) != B_OK)
				send_data(sender, B_OK, NULL, 0);
		}
	}
	
	return B_OK;
}


static
status_t
report_thread(void *data)
{
	// Create BMessage for each report and send it to the target BHandler.
	// The target must send a reply to the interface!
	
	PPPInterfaceListenerThread *thread
		= static_cast<PPPInterfaceListenerThread*>(data);
	
	return thread->Run();
}


PPPInterfaceListener::PPPInterfaceListener(BHandler *target)
	: fTarget(target),
	fDoesWatch(false),
	fWatchingInterface(PPP_UNDEFINED_INTERFACE_ID)
{
	Construct();
}


PPPInterfaceListener::PPPInterfaceListener(const PPPInterfaceListener& copy)
	: fTarget(copy.Target()),
	fDoesWatch(false),
	fWatchingInterface(PPP_UNDEFINED_INTERFACE_ID)
{
	Construct();
}


PPPInterfaceListener::~PPPInterfaceListener()
{
	// disable all report messages
	StopWatchingInterfaces();
	Manager().DisableReports(PPP_ALL_REPORTS, fReportThread);
	
	// tell thread to quit
	send_data(fReportThread, QUIT_REPORT_THREAD, NULL, 0);
	int32 tmp;
	wait_for_thread(fReportThread, &tmp);
}


status_t
PPPInterfaceListener::InitCheck() const
{
	if(fReportThread < 0)
		return B_ERROR;
	
	return Manager().InitCheck();
}


void
PPPInterfaceListener::SetTarget(BHandler *target)
{
	LockerHelper locker(fLock);
	
	target = fTarget;
}


bool
PPPInterfaceListener::WatchInterface(ppp_interface_id ID)
{
	StopWatchingInterfaces();
	
	// enable reports
	PPPInterface interface(ID);
	if(interface.InitCheck() != B_OK)
		return false;
	
	if(!interface.EnableReports(PPP_CONNECTION_REPORT, fReportThread, REPORT_FLAGS))
		return false;
	
	fDoesWatch = true;
	fWatchingInterface = ID;
	
	return true;
}


void
PPPInterfaceListener::WatchAllInterfaces()
{
	StopWatchingInterfaces();
	
	// enable interface reports
	int32 count;
	PPPInterface interface;
	ppp_interface_id *interfaceList;
	
	interfaceList = Manager().Interfaces(&count);
	if(!interfaceList)
		return;
	
	for(int32 index = 0; index < count; index++) {
		interface.SetTo(interfaceList[index]);
		interface.EnableReports(PPP_CONNECTION_REPORT, fReportThread, REPORT_FLAGS);
	}
	delete interfaceList;
	
	fDoesWatch = true;
	fWatchingInterface = PPP_UNDEFINED_INTERFACE_ID;
		// this means watching all
}


void
PPPInterfaceListener::StopWatchingInterfaces()
{
	if(!fDoesWatch)
		return;
	
	if(fWatchingInterface == PPP_UNDEFINED_INTERFACE_ID) {
		// disable all reports
		int32 count;
		PPPInterface interface;
		ppp_interface_id *interfaceList;
		
		interfaceList = Manager().Interfaces(&count);
		if(!interfaceList)
			return;
		
		for(int32 index = 0; index < count; index++) {
			interface.SetTo(interfaceList[index]);
			interface.DisableReports(PPP_ALL_REPORTS, fReportThread);
		}
		delete interfaceList;
	} else {
		PPPInterface interface(fWatchingInterface);
		interface.DisableReports(PPP_ALL_REPORTS, fReportThread);
	}
	
	fDoesWatch = false;
	fWatchingInterface = PPP_UNDEFINED_INTERFACE_ID;
}


void
PPPInterfaceListener::Construct()
{
	fReportThread = spawn_thread(report_thread, "report_thread",
		B_NORMAL_PRIORITY, new PPPInterfaceListenerThread(this));
	resume_thread(fReportThread);
	
	// enable manager reports
	Manager().EnableReports(PPP_MANAGER_REPORT, fReportThread, REPORT_FLAGS);
}
