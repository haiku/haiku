/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/
#ifndef IO_SCHEDULER_H
#define IO_SCHEDULER_H


#include <OS.h>
#include <Drivers.h>
#include <pnp_devfs.h>

#include <util/DoublyLinkedList.h>
#include <lock.h>


class IORequest {
	public:
		IORequest(void *cookie, off_t offset, void *buffer, size_t size, bool write = false);
		IORequest(void *cookie, off_t offset, const void *buffer, size_t size, bool write = true);
			// ToDo: iovecs version?

		size_t Size() const { return size; }

		DoublyLinked::Link link;

		void		*cookie;
		addr_t		physical_address;
		addr_t		virtual_address;
		off_t		offset;
		size_t		size;
		bool		write;
		thread_id	thread;
};


class IOScheduler {
	public:
		IOScheduler(const char *name, pnp_devfs_driver_info *hooks);
		~IOScheduler();

		status_t InitCheck() const;
		status_t Process(IORequest &request);

#if 0
		static IOScheduler *GetScheduler();
#endif

	private:
		int32 Scheduler();
		static int32 scheduler(void *);

	private:
		pnp_devfs_driver_info	*fDeviceHooks;
		mutex					fLock;
		thread_id				fThread;
		DoublyLinked::List<IORequest, &IORequest::link> fRequests;
};

#endif	/* IO_SCHEDULER_H */
