//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#ifndef _K_PPP_REPORT_MANAGER__H
#define _K_PPP_REPORT_MANAGER__H

#include <OS.h>

#include <KPPPReportDefs.h>

#include <List.h>


#define PPP_REPLY(sender, value) send_data_with_timeout((sender), (value), NULL, 0, PPP_REPORT_TIMEOUT)

class PPPReportManager {
	public:
		PPPReportManager(BLocker& lock);
		~PPPReportManager();
		
		void EnableReports(PPP_REPORT_TYPE type, thread_id thread,
				int32 flags = PPP_NO_REPORT_FLAGS);
		void DisableReports(PPP_REPORT_TYPE type, thread_id thread);
		bool DoesReport(PPP_REPORT_TYPE type, thread_id thread);
		bool Report(PPP_REPORT_TYPE type, int32 code, void *data, int32 length);
			// returns false if reply was bad (or an error occured)

	private:
		BLocker& fLock;
		List<ppp_report_request*> fReportRequests;
};


#endif
