/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IO_SCHEDULER_H
#define IO_SCHEDULER_H


#include <KernelExport.h>

#include <util/DoublyLinkedList.h>

#include "IOCallback.h"
#include "IORequest.h"


struct IORequestOwner : DoublyLinkedListLinkImpl<IORequestOwner> {
	team_id			team;
	thread_id		thread;
	int32			priority;
	IORequestList	requests;
	IORequestList	completed_requests;
	IOOperationList	operations;
	IORequestOwner*	hash_link;

			bool				IsActive() const
									{ return !requests.IsEmpty()
										|| !completed_requests.IsEmpty()
										|| !operations.IsEmpty(); }

			void				Dump() const;
};


class IOScheduler : public DoublyLinkedListLinkImpl<IOScheduler> {
public:
								IOScheduler(DMAResource* resource);
	virtual						~IOScheduler();

	virtual	status_t			Init(const char* name);

			const char*			Name() const	{ return fName; }
			int32				ID() const		{ return fID; }

	virtual	void				SetCallback(IOCallback& callback);
	virtual	void				SetCallback(io_callback callback, void* data);

	virtual	void				SetDeviceCapacity(off_t deviceCapacity);
	virtual void				MediaChanged();

	virtual	status_t			ScheduleRequest(IORequest* request) = 0;

	virtual	void				AbortRequest(IORequest* request,
									status_t status = B_CANCELED) = 0;
	virtual	void				OperationCompleted(IOOperation* operation,
									status_t status,
									generic_size_t transferredBytes) = 0;
									// called by the driver when the operation
									// has been completed successfully or failed
									// for some reason

	virtual	void				Dump() const = 0;

protected:
			DMAResource*		fDMAResource;
			char*				fName;
			int32				fID;
			io_callback			fIOCallback;
			void*				fIOCallbackData;
			bool				fSchedulerRegistered;
};


#endif	// IO_SCHEDULER_H
