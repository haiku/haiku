#ifndef _K_PPP_REPORT_MANAGER__H
#define _K_PPP_REPORT_MANAGER__H

#include "KPPPReportDefs.h"

#include "List.h"
#include "LockerHelper.h"


class PPPReportManager {
	public:
		PPPReportManager(BLocker& lock);
		
		void EnableReports(PPP_REPORT_TYPE type, thread_id thread,
				int32 flags = PPP_NO_REPORT_FLAGS);
		void DisableReports(PPP_REPORT_TYPE type, thread_id thread);
		bool DoesReport(PPP_REPORT_TYPE type, thread_id thread);
		bool Report(PPP_REPORT_TYPE type, int32 code, void *data, int32 length);
			// returns false if reply was bad (or an error occured)

	private:
		BLocker& fLock;
		List<ppp_report_request> fReportRequests;
};


#endif
