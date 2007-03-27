/*
 * Copyright 2003-2007, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

/*!	\class KPPPReportManager
	\brief Manager for PPP reports and report requests.
*/

#include <KPPPReportManager.h>

#include <LockerHelper.h>
#include <KPPPUtils.h>


typedef struct report_sender_info {
	thread_id thread;
	ppp_report_packet report;
} report_sender_info;


static status_t
report_sender_thread(void *data)
{
	report_sender_info *info = static_cast<report_sender_info*>(data);
	KPPPReportManager::SendReport(info->thread, &info->report);
	delete info;
	return B_OK;
}


/*!	\brief Constructor.
	
	\param lock The BLocker that should be used by this report manager.
*/
KPPPReportManager::KPPPReportManager(BLocker& lock)
	: fLock(lock)
{
}


//!	Deletes all report requests.
KPPPReportManager::~KPPPReportManager()
{
	for (int32 index = 0; index < fReportRequests.CountItems(); index++)
		delete fReportRequests.ItemAt(index);
}


/*!	\brief Send the given report message to the given thread.
	
	\param thread The report receiver.
	\param report The report message.
	
	\return \c false on error.
*/
bool
KPPPReportManager::SendReport(thread_id thread, const ppp_report_packet *report)
{
	if (!report)
		return false;
	
	if (thread == find_thread(NULL)) {
		report_sender_info *info = new report_sender_info;
		info->thread = thread;
		memcpy(&info->report, report, sizeof(ppp_report_packet));
		resume_thread(spawn_kernel_thread(report_sender_thread, "PPP: ReportSender",
			B_NORMAL_PRIORITY, info));
		return true;
	}
	
	send_data_with_timeout(thread, PPP_REPORT_CODE, &report, sizeof(report),
		PPP_REPORT_TIMEOUT);
	return true;
}


/*!	\brief Requests report messages of a given \a type.
	
	\param type The type of report.
	\param thread The receiver thread.
	\param flags Optional report flags. See \c ppp_report_flags for more information.
*/
void
KPPPReportManager::EnableReports(ppp_report_type type, thread_id thread,
	int32 flags)
{
	if (thread < 0 || type == PPP_ALL_REPORTS)
		return;
	
	LockerHelper locker(fLock);
	
	ppp_report_request *request = new ppp_report_request;
	request->type = type;
	request->thread = thread;
	request->flags = flags;
	
	fReportRequests.AddItem(request);
}


//!	Removes a report request.
void
KPPPReportManager::DisableReports(ppp_report_type type, thread_id thread)
{
	if (thread < 0)
		return;
	
	LockerHelper locker(fLock);
	
	ppp_report_request *request;
	
	for (int32 i = 0; i < fReportRequests.CountItems(); i++) {
		request = fReportRequests.ItemAt(i);
		
		if (request->thread != thread)
			continue;
		
		if (request->type == type || type == PPP_ALL_REPORTS)
			fReportRequests.RemoveItem(request);
	}
	
	// empty message queue
	while (has_data(thread)) {
		thread_id sender;
		receive_data(&sender, NULL, 0);
	}
}


//!	Returns if we report messages of a given \a type to the given \a thread.
bool
KPPPReportManager::DoesReport(ppp_report_type type, thread_id thread)
{
	if (thread < 0)
		return false;
	
	LockerHelper locker(fLock);
	
	ppp_report_request *request;
	
	for (int32 i = 0; i < fReportRequests.CountItems(); i++) {
		request = fReportRequests.ItemAt(i);
		
		if (request->thread == thread && request->type == type)
			return true;
	}
	
	return false;
}


/*!	\brief Send out report messages to all requestors.
	
	You may append additional data to the report messages. The data length may not be
	greater than \c PPP_REPORT_DATA_LIMIT.
	
	\param type The report type.
	\param code The report code belonging to the report type.
	\param data Additional data.
	\param length Length of the data.
	
	\return \c false on error.
*/
bool
KPPPReportManager::Report(ppp_report_type type, int32 code, void *data, int32 length)
{
	TRACE("KPPPReportManager: Report(type=%d code=%ld length=%ld) to %ld receivers\n",
		type, code, length, fReportRequests.CountItems());
	
	if (length > PPP_REPORT_DATA_LIMIT)
		return false;
	
	if (fReportRequests.CountItems() == 0)
		return true;
	
	if (!data)
		length = 0;
	
	LockerHelper locker(fLock);
	
	status_t result;
	thread_id me = find_thread(NULL);
	
	ppp_report_packet report;
	report.type = type;
	report.code = code;
	report.length = length;
	memcpy(report.data, data, length);
	
	ppp_report_request *request;
	
	for (int32 index = 0; index < fReportRequests.CountItems(); index++) {
		request = fReportRequests.ItemAt(index);
		
		// do not send to yourself
		if (request->thread == me)
			continue;
		
		result = send_data_with_timeout(request->thread, PPP_REPORT_CODE, &report,
			sizeof(report), PPP_REPORT_TIMEOUT);
		
#if DEBUG
		if (result == B_TIMED_OUT)
			TRACE("KPPPReportManager::Report(): timed out sending\n");
#endif
		
		if (result == B_BAD_THREAD_ID || result == B_NO_MEMORY
				|| request->flags & PPP_REMOVE_AFTER_REPORT) {
			fReportRequests.RemoveItem(request);
			--index;
			continue;
		}
	}
	
	return true;
}
