/*
 * Copyright 2004-2005, Waldemar Kornewald <Waldemar.Kornewald@web.de>
 * Distributed under the terms of the MIT License.
 */

#include "PPPUpApplication.h"

#include <Window.h>

#include "DialUpView.h"
#include "ConnectionWindow.h"
#include <KPPPUtils.h>
#include <PPPReportDefs.h>

#include <Deskbar.h>
#include "PPPDeskbarReplicant.h"


static
status_t
report_thread(void *data)
{
	PPPUpApplication *app = static_cast<PPPUpApplication*>(data);
	
	// Send reply. From now on we will receive report messages.
	send_data(app->ReplyThread(), 0, NULL, 0);
	
	// get messages and open connection window when we receive a GOING_UP report
	thread_id sender, me = find_thread(NULL);
	int32 reportCode;
	ppp_interface_id id;
	ppp_report_packet report;
	ConnectionWindow *window = NULL;
	while(true) {
		reportCode = receive_data(&sender, &report, sizeof(report));
		if(reportCode == PPP_RESPONSE_TEST_CODE) {
			if(!window || window->ResponseTest())
				PPP_REPLY(sender, B_OK);
			else
				PPP_REPLY(sender, B_ERROR);
			continue;
		} else if(reportCode != PPP_REPORT_CODE)
			continue;
		
		if(report.type == PPP_DESTRUCTION_REPORT) {
			id = PPP_UNDEFINED_INTERFACE_ID;
			PPP_REPLY(sender, B_OK);
		} else if(report.type != PPP_CONNECTION_REPORT) {
			PPP_REPLY(sender, B_OK);
			continue;
		} else if(report.length == sizeof(ppp_interface_id))
			memcpy(&id, report.data, sizeof(ppp_interface_id));
		else
			id = PPP_UNDEFINED_INTERFACE_ID;
		
		// notify window
		if(window && (report.type == PPP_DESTRUCTION_REPORT
				|| report.type == PPP_CONNECTION_REPORT)) {
			BMessage message;
			message.AddInt32("code", report.type == PPP_DESTRUCTION_REPORT
				? PPP_REPORT_DOWN_SUCCESSFUL : report.code);
			message.AddInt32("interface", id);
			window->UpdateStatus(message);
		}
		
		if(report.code == PPP_REPORT_GOING_UP) {
			// create connection window (it will send the reply for us) (DONE?)
			BRect rect(150, 50, 450, 435);
			if(!window) {
				window = new ConnectionWindow(rect, app->Name(), app->ID(), me);
				window->Show();
			}
			
			window->RequestReply();
			// wait for reply from window and forward it to the kernel
			thread_id tmp;
			PPP_REPLY(sender, receive_data(&tmp, NULL, 0));
		} else {
			if(report.code == PPP_REPORT_UP_SUCCESSFUL) {
				// add deskbar replicant (DONE?)
				PPPDeskbarReplicant *replicant = new PPPDeskbarReplicant(id);
				BDeskbar().AddItem(replicant);
				delete replicant;
			}
			
			PPP_REPLY(sender, PPP_OK_DISABLE_REPORTS);
		}
	}
	
	return B_OK;
}


int
main(int argc, const char *argv[])
{
	if(argc != 2)
		return -1;
	
	const char *name = argv[1];
	
	thread_id replyThread;
	ppp_interface_id id;
	receive_data(&replyThread, &id, sizeof(id));
	
	new PPPUpApplication(name, id, replyThread);
	be_app->Run();
	delete be_app;
	
	return 0;
}


PPPUpApplication::PPPUpApplication(const char *name, ppp_interface_id id,
		thread_id replyThread)
	: BApplication(APP_SIGNATURE),
	fName(name),
	fID(id),
	fReplyThread(replyThread)
{
}


void
PPPUpApplication::ReadyToRun()
{
	// Create report message thread (which in turn sends the reply and creates
	// the connection window). (DONE?)
	thread_id reportThread = spawn_thread(report_thread, "ppp_up: report_thread",
		B_NORMAL_PRIORITY, this);
	resume_thread(reportThread);
}
