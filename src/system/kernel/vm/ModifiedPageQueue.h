/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef MODIFIED_PAGE_QUEUE_H
#define MODIFIED_PAGE_QUEUE_H


#include <util/BinarySemaphore.h>

#include "VMPageQueue.h"


struct ModifiedPageQueue : public VMPageQueue {
public:
			status_t			StartWriter();
			void				NotifyWriter() { fPageWriterCondition.WakeUp(); }

			bool				IsOverQuota(page_num_t additionalPages = 0);
			status_t			WaitIfOverQuota(page_num_t additionalPages,
									bigtime_t timeout, uint32 flags);

private:
	static	status_t			_WriterThreadEntry(void* _this);
			status_t			_PageWriter();

private:
			thread_id			fWriterThread;
			BinarySemaphore		fPageWriterCondition;
			ConditionVariable	fUnderQuotaCondition;

			bigtime_t			fLastAveragePageWriteDuration;
};


ModifiedPageQueue* vm_page_get_modified_queue();


#endif	// MODIFIED_PAGE_QUEUE_H
