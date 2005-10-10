/*
 * Copyright 2003-2005, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */

#ifndef _K_PPP_REPORT_MANAGER__H
#define _K_PPP_REPORT_MANAGER__H

#include <OS.h>

#include <KPPPDefs.h>
#include <PPPReportDefs.h>

#include <TemplateList.h>


class KPPPReportManager {
	public:
		KPPPReportManager(BLocker& lock);
		~KPPPReportManager();
		
		static bool SendReport(thread_id thread, const ppp_report_packet *report);
			// returns false if reply was bad (or an error occured)
		
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
