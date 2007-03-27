/*
 * Copyright 2003-2007, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

/*!	\class PPPInterfaceListener
	\brief This class simplifies the process of monitoring PPP interfaces.
	
	PPPInterfaceListener converts all kernel report messages from the PPP stack
	into BMessage objects and forwards them to the target BHandler.\n
	The following values are added to each BMessage as int32 values:
		- "interface" [\c int32] (optional): the interface ID of the affected interface
		- "type" [\c int32]: the report type
		- "code" [\c int32]: the report code
*/

#include "PPPInterfaceListener.h"

#include "PPPInterface.h"

#include <Messenger.h>
#include <Handler.h>
#include <LockerHelper.h>
#include <KPPPUtils.h>


static const int32 kCodeQuitReportThread = 'QUIT';


// Creates a BMessage for each report and send it to the target BHandler.
static
status_t
report_thread(void *data)
{
	PPPInterfaceListener *listener = static_cast<PPPInterfaceListener*>(data);
	
	ppp_report_packet report;
	ppp_interface_id *interfaceID;
	int32 code;
	thread_id sender;
	BMessage message;
	
	while (true) {
		code = receive_data(&sender, &report, sizeof(report));
		
		if (code == kCodeQuitReportThread)
			break;
		else if (code != PPP_REPORT_CODE)
			continue;
		
		BMessenger messenger(listener->Target());
		if (messenger.IsValid()) {
			message.MakeEmpty();
			message.what = PPP_REPORT_MESSAGE;
			message.AddInt32("type", report.type);
			message.AddInt32("code", report.code);
			
			if (report.length >= sizeof(ppp_interface_id)
					&& ((report.type == PPP_MANAGER_REPORT
							&& report.code == PPP_REPORT_INTERFACE_CREATED)
						|| report.type >= PPP_INTERFACE_REPORT_TYPE_MIN)) {
				interfaceID = reinterpret_cast<ppp_interface_id*>(report.data);
				message.AddInt32("interface", static_cast<int32>(*interfaceID));
			}
			
			// We might cause a dead-lock. Thus, abort if we cannot send.
			messenger.SendMessage(&message, (BHandler*) NULL, 100000);
		}
	}
	
	return B_OK;
}


/*!	\brief Constructs a new listener that sends report messages to \a target.
	
	\param target The target BHandler which should receive report messages.
*/
PPPInterfaceListener::PPPInterfaceListener(BHandler *target)
	: fTarget(target),
	fIsWatching(false),
	fInterface(PPP_UNDEFINED_INTERFACE_ID)
{
	Construct();
}


//!	Copy constructor.
PPPInterfaceListener::PPPInterfaceListener(const PPPInterfaceListener& copy)
	: fTarget(copy.Target()),
	fIsWatching(false),
	fInterface(PPP_UNDEFINED_INTERFACE_ID)
{
	Construct();
}


//!	Removes all report message requests.
PPPInterfaceListener::~PPPInterfaceListener()
{
	// disable all report messages
	StopWatchingInterface();
	StopWatchingManager();
	
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
	if (fReportThread < 0)
		return B_ERROR;
	
	return Manager().InitCheck();
}


//!	Changes the target BHandler for the report messages (may be \c NULL).
void
PPPInterfaceListener::SetTarget(BHandler *target)
{
	fTarget = target;
}


/*!	\brief Changes the interface being monitored.
	
	This unregisters the old interface from the watch-list.
	
	\param ID The ID of the interface you want to watch.
	
	\return \c true on sucess, \c false on failure.
*/
bool
PPPInterfaceListener::WatchInterface(ppp_interface_id ID)
{
	if (ID == fInterface)
		return true;
	
	StopWatchingInterface();
	
	if (ID == PPP_UNDEFINED_INTERFACE_ID)
		return true;
	
	// enable reports
	PPPInterface interface(ID);
	if (interface.InitCheck() != B_OK)
		return false;
	
	if (!interface.EnableReports(PPP_CONNECTION_REPORT, fReportThread, PPP_NO_FLAGS))
		return false;
	
	fIsWatching = true;
	fInterface = ID;
	
	return true;
}


//!	Enables interface creation messages from the PPP manager.
void
PPPInterfaceListener::WatchManager()
{
	Manager().EnableReports(PPP_MANAGER_REPORT, fReportThread, PPP_NO_FLAGS);
}


/*!	\brief Stops watching the interface.
	
	Beware that this does not disable the PPP manager's report messages.
*/
void
PPPInterfaceListener::StopWatchingInterface()
{
	if (!fIsWatching)
		return;
	
	PPPInterface interface(fInterface);
	interface.DisableReports(PPP_ALL_REPORTS, fReportThread);
	
	fIsWatching = false;
	fInterface = PPP_UNDEFINED_INTERFACE_ID;
}


//!	Disables interface creation messages from the PPP manager.
void
PPPInterfaceListener::StopWatchingManager()
{
	Manager().DisableReports(PPP_ALL_REPORTS, fReportThread);
}


void
PPPInterfaceListener::Construct()
{
	fReportThread = spawn_thread(report_thread, "report_thread", B_NORMAL_PRIORITY,
		this);
	resume_thread(fReportThread);
}
