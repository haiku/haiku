//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#ifndef _K_PPP_REPORT_MANAGER__H
#define _K_PPP_REPORT_MANAGER__H

#include <OS.h>

#include <KPPPDefs.h>
#include <PPPReportDefs.h>

#include <TemplateList.h>


#define PPP_REPLY(sender, value) send_data_with_timeout((sender), (value), NULL, 0, PPP_REPORT_TIMEOUT)

class PPPReportManager {
	public:
		PPPReportManager(BLocker& lock);
		~PPPReportManager();
		
		void EnableReports(ppp_report_type type, thread_id thread,
				int32 flags = PPP_NO_FLAGS);
		void DisableReports(ppp_report_type type, thread_id thread);
		bool DoesReport(ppp_report_type type, thread_id thread);
		bool Report(ppp_report_type type, int32 code, void *data, int32 length);
			// returns false if reply was bad (or an error occured)

	private:
		BLocker& fLock;
		TemplateList<ppp_report_request*> fReportRequests;
};


#endif
