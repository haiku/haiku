//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

/*!	\class PPPInterfaceListener
	\brief This class simplifies the process of monitoring PPP interfaces.
	
	PPPInterfaceListener converts all kernel report messages from the PPP stack
	into BMessage objects and forwards them to the target BHandler.\n
	In case the BLooper's message queue is full and does not respond after a timeout
	period it sends the reply message to the PPP stack for you. Otherwise you must
	do it yourself.\n
	Of course you can use this class to watch for all interfaces. This includes
	automatically adding newly created interfaces to the watch-list.\n
	The following values are added to each BMessage as int32 values:
		- "sender": the thread_id of the report sender (your reply target)
		- "interface" (optional): the interface ID of the affected interface
		- "type": the report type
		- "code": the report code
*/

#include "PPPInterfaceListener.h"

#include "PPPInterface.h"

#include <Messenger.h>
#include <Handler.h>
#include <LockerHelper.h>


static const uint32 kReportFlags = PPP_WAIT_FOR_REPLY | PPP_NO_REPLY_TIMEOUT
							| PPP_ALLOW_ANY_REPLY_THREAD;

static const int32 kCodeQuitReportThread = 'QUIT';


//!	Private class.
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
		
		if(code == kCodeQuitReportThread)
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
		} else
			send_data(sender, B_OK, NULL, 0);
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


/*!	\brief Constructs a new listener that sends report messages to \a target.
	
	\param target The target BHandler which should receive report messages.
*/
PPPInterfaceListener::PPPInterfaceListener(BHandler *target)
	: fTarget(target),
	fDoesWatch(false),
	fWatchingInterface(PPP_UNDEFINED_INTERFACE_ID)
{
	Construct();
}


//!	Copy constructor.
PPPInterfaceListener::PPPInterfaceListener(const PPPInterfaceListener& copy)
	: fTarget(copy.Target()),
	fDoesWatch(false),
	fWatchingInterface(PPP_UNDEFINED_INTERFACE_ID)
{
	Construct();
}


//!	Removes all report message requests.
PPPInterfaceListener::~PPPInterfaceListener()
{
	// disable all report messages
	StopWatchingInterfaces();
	Manager().DisableReports(PPP_ALL_REPORTS, fReportThread);
	
	// tell thread to quit
	send_data(fReportThread, kCodeQuitReportThread, NULL, 0);
	int32 tmp;
	wait_for_thread(fReportThread, &tmp);
}


/*!	\brief Returns whether the listener was constructed correctly.
	
	\return
		- \c B_OK: No errors.
		- \c B_ERROR: Some not defined error occured (like missing PPP stack).
		- any other value: see \c PPPManager::InitCheck()
	
	\sa PPPManager::InitCheck()
*/
status_t
PPPInterfaceListener::InitCheck() const
{
	if(fReportThread < 0)
		return B_ERROR;
	
	return Manager().InitCheck();
}


//!	Changes the target BHandler for the report messages (may be \c NULL).
void
PPPInterfaceListener::SetTarget(BHandler *target)
{
	LockerHelper locker(fLock);
	
	target = fTarget;
}


/*!	\brief Changes the interface being monitored.
	
	This unregisters the old interface from the watch-list.
	
	\param ID The ID of the interface you want to watch.
	
	\return \c true on sucess, \c false on failure.
*/
bool
PPPInterfaceListener::WatchInterface(ppp_interface_id ID)
{
	StopWatchingInterfaces();
	
	// enable reports
	PPPInterface interface(ID);
	if(interface.InitCheck() != B_OK)
		return false;
	
	if(!interface.EnableReports(PPP_CONNECTION_REPORT, fReportThread, kReportFlags))
		return false;
	
	fDoesWatch = true;
	fWatchingInterface = ID;
	
	return true;
}


//!	Enables mode for watching all interfaces. New interfaces are added automatically.
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
		interface.EnableReports(PPP_CONNECTION_REPORT, fReportThread, kReportFlags);
	}
	delete interfaceList;
	
	fDoesWatch = true;
	fWatchingInterface = PPP_UNDEFINED_INTERFACE_ID;
		// this means watching all
}


/*!	\brief Stops watching interfaces.
	
	Beware that this does not disable the PPP stack's own report messages (e.g.: new
	interfaces that being created).
*/
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
	Manager().EnableReports(PPP_MANAGER_REPORT, fReportThread, kReportFlags);
}
