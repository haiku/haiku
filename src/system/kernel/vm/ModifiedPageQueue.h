/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef MODIFIED_PAGE_QUEUE_H
#define MODIFIED_PAGE_QUEUE_H


#include <Referenceable.h>
#include <util/BinarySemaphore.h>

#include "VMPageQueue.h"


struct ModifiedPageQueue : public BReferenceable, public VMPageQueue {
public:
	static	int64				GlobalModifiedCount()
									{ return atomic_get64(&sGlobalModifiedCount); }

	virtual						~ModifiedPageQueue();

			status_t			StartWriter(const char* name);
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

private:
	static	int64				sGlobalModifiedCount;
			int64				fLastReportedModifiedCount = 0;

	static	bigtime_t			sGlobalEstimatedWriteDuration;
			bigtime_t			fLastReportedEstimatedWriteDuration = 0;
};


ModifiedPageQueue* vm_page_default_modified_queue();


#endif	// MODIFIED_PAGE_QUEUE_H
