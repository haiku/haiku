//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#include <KPPPReportManager.h>
#include <LockerHelper.h>

#include <KPPPUtils.h>


KPPPReportManager::KPPPReportManager(BLocker& lock)
	: fLock(lock)
{
}


KPPPReportManager::~KPPPReportManager()
{
	for(int32 index = 0; index < fReportRequests.CountItems(); index++)
		delete fReportRequests.ItemAt(index);
}


void
KPPPReportManager::EnableReports(ppp_report_type type, thread_id thread,
	int32 flags = PPP_NO_FLAGS)
{
	if(thread < 0 || type == PPP_ALL_REPORTS)
		return;
	
	LockerHelper locker(fLock);
	
	ppp_report_request *request = new ppp_report_request;
	request->type = type;
	request->thread = thread;
	request->flags = flags;
	
	fReportRequests.AddItem(request);
}


void
KPPPReportManager::DisableReports(ppp_report_type type, thread_id thread)
{
	if(thread < 0)
		return;
	
	LockerHelper locker(fLock);
	
	ppp_report_request *request;
	
	for(int32 i = 0; i < fReportRequests.CountItems(); i++) {
		request = fReportRequests.ItemAt(i);
		
		if(request->thread != thread)
			continue;
		
		if(request->type == type || type == PPP_ALL_REPORTS)
			fReportRequests.RemoveItem(request);
	}
}


bool
KPPPReportManager::DoesReport(ppp_report_type type, thread_id thread)
{
	if(thread < 0)
		return false;
	
	LockerHelper locker(fLock);
	
	ppp_report_request *request;
	
	for(int32 i = 0; i < fReportRequests.CountItems(); i++) {
		request = fReportRequests.ItemAt(i);
		
		if(request->thread == thread && request->type == type)
			return true;
	}
	
	return false;
}


bool
KPPPReportManager::Report(ppp_report_type type, int32 code, void *data, int32 length)
{
#if DEBUG
	dprintf("KPPPReportManager: Report(type=%d code=%ld length=%ld) to %ld receivers\n",
		type, code, length, fReportRequests.CountItems());
#endif
	
	if(length > PPP_REPORT_DATA_LIMIT)
		return false;
	
	if(fReportRequests.CountItems() == 0)
		return true;
	
	if(!data)
		length = 0;
	
	LockerHelper locker(fLock);
	
	status_t result;
	thread_id sender, me = find_thread(NULL);
	bool acceptable = true;
	
	ppp_report_packet report;
	report.type = type;
	report.code = code;
	report.length = length;
	memcpy(report.data, data, length);
	
	ppp_report_request *request;
	
	for(int32 index = 0; index < fReportRequests.CountItems(); index++) {
		request = fReportRequests.ItemAt(index);
		
		// do not send to yourself
		if(request->thread == me)
			continue;
		
		result = send_data_with_timeout(request->thread, PPP_REPORT_CODE, &report,
			sizeof(report), PPP_REPORT_TIMEOUT);
		
#if DEBUG
		if(result == B_TIMED_OUT)
			dprintf("KPPPReportManager::Report(): timed out sending\n");
#endif
		
		thread_info info;
		
		if(result == B_BAD_THREAD_ID || result == B_NO_MEMORY) {
			fReportRequests.RemoveItem(request);
			--index;
			continue;
		} else if(result == B_OK && request->flags & PPP_WAIT_FOR_REPLY) {
			if(request->flags & PPP_NO_REPLY_TIMEOUT) {
				sender = -1;
				result = B_ERROR;
				// always check if the thread still exists
				while(sender != request->thread
						&& get_thread_info(request->thread, &info) == B_OK) {
					result = receive_data_with_timeout(&sender, &code, NULL, 0,
						PPP_REPORT_TIMEOUT);
					
					if(request->flags & PPP_ALLOW_ANY_REPLY_THREAD
							&& result == B_OK)
						sender = request->thread;
				}
			} else {
				sender = -1;
				result = B_OK;
				while(sender != request->thread && result == B_OK
						&& get_thread_info(request->thread, &info) == B_OK) {
					result = receive_data_with_timeout(&sender, &code, NULL, 0,
						PPP_REPORT_TIMEOUT);
					
					if(request->flags & PPP_ALLOW_ANY_REPLY_THREAD)
						sender = request->thread;
				}
			}
			
			if(sender != request->thread) {
#if DEBUG
				dprintf("KPPPReportManager::Report(): sender != requested\n");
#endif
				continue;
			}
			
			if(result == B_OK && code != B_OK)
				acceptable = false;
#if DEBUG
			if(result == B_TIMED_OUT)
				dprintf("KPPPReportManager::Report(): reply timed out\n");
#endif
		}
		
		// remove thread if it is not existant or if remove-flag is set
		if(request->flags & PPP_REMOVE_AFTER_REPORT
				|| get_thread_info(request->thread, &info) != B_OK) {
			fReportRequests.RemoveItem(request);
			--index;
		}
	}
	
#if DEBUG
	dprintf("KPPPReportManager::Report(): returning: %s\n", acceptable?"true":"false");
#endif
	
	return acceptable;
}
