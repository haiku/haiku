/*
 * Copyright 2009-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IO_SCHEDULER_ROSTER_H
#define IO_SCHEDULER_ROSTER_H


#include <Notifications.h>

#include "IOScheduler.h"


// I/O scheduler notifications
#define IO_SCHEDULER_MONITOR			'_io_'
#define IO_SCHEDULER_ADDED				0x01
#define IO_SCHEDULER_REMOVED			0x02
#define IO_SCHEDULER_REQUEST_SCHEDULED	0x04
#define IO_SCHEDULER_REQUEST_FINISHED	0x08
#define IO_SCHEDULER_OPERATION_STARTED	0x10
#define IO_SCHEDULER_OPERATION_FINISHED	0x20



typedef DoublyLinkedList<IOScheduler> IOSchedulerList;


class IOSchedulerRoster {
public:
	static	void				Init();
	static	IOSchedulerRoster*	Default()	{ return &sDefaultInstance; }

			bool				Lock()	{ return mutex_lock(&fLock) == B_OK; }
			void				Unlock()	{ mutex_unlock(&fLock); }

			const IOSchedulerList& SchedulerList() const
									{ return fSchedulers; }
									// caller must keep the roster locked,
									// while accessing the list

			void				AddScheduler(IOScheduler* scheduler);
			void				RemoveScheduler(IOScheduler* scheduler);

			void				Notify(uint32 eventCode,
									const IOScheduler* scheduler,
									IORequest* request = NULL,
									IOOperation* operation = NULL);

			int32				NextID();

private:
								IOSchedulerRoster();
								~IOSchedulerRoster();

private:
			mutex				fLock;
			int32				fNextID;
			IOSchedulerList		fSchedulers;
			DefaultNotificationService fNotificationService;
			char				fEventBuffer[256];

	static	IOSchedulerRoster	sDefaultInstance;
};


#endif	// IO_SCHEDULER_ROSTER_H
