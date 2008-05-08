/*
 * Copyright 2003-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <kernel_daemon.h>

#include <new>
#include <signal.h>
#include <stdlib.h>

#include <KernelExport.h>

#include <lock.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>


// The use of snooze() in the kernel_daemon() function is very inaccurate, of
// course - the time the daemons need to execute add up in each iteration.
// But since the kernel daemon is documented to be very inaccurate, this
// actually might be okay (and that's why it's implemented this way now :-).
// BeOS R5 seems to do it in the same way, anyway.

struct daemon : DoublyLinkedListLinkImpl<struct daemon> {
	daemon_hook	function;
	void*		arg;
	int32		frequency;
	int32		offset;
};


typedef DoublyLinkedList<struct daemon> DaemonList;

static mutex sDaemonMutex;
static DaemonList sDaemons;


static status_t
kernel_daemon(void* data)
{
	int32 iteration = 0;

	while (true) {
		mutex_lock(&sDaemonMutex);

		DaemonList::Iterator iterator = sDaemons.GetIterator();

		// iterate through the list and execute each daemon if needed
		while (iterator.HasNext()) {
			struct daemon* daemon = iterator.Next();

			if (((iteration + daemon->offset) % daemon->frequency) == 0)
				daemon->function(daemon->arg, iteration);
		}
		mutex_unlock(&sDaemonMutex);

		iteration++;
		snooze(100000);	// 0.1 seconds
	}

	return B_OK;
}


//	#pragma mark -


extern "C" status_t
unregister_kernel_daemon(daemon_hook function, void* arg)
{
	MutexLocker _(sDaemonMutex);

	DaemonList::Iterator iterator = sDaemons.GetIterator();

	// search for the daemon and remove it from the list
	while (iterator.HasNext()) {
		struct daemon* daemon = iterator.Next();

		if (daemon->function == function && daemon->arg == arg) {
			// found it!
			iterator.Remove();
			free(daemon);
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


extern "C" status_t
register_kernel_daemon(daemon_hook function, void* arg, int frequency)
{
	if (function == NULL || frequency < 1)
		return B_BAD_VALUE;

	struct daemon* daemon = new(std::nothrow) struct daemon();
	if (daemon == NULL)
		return B_NO_MEMORY;

	daemon->function = function;
	daemon->arg = arg;
	daemon->frequency = frequency;

	MutexLocker _(sDaemonMutex);
	
	if (frequency > 1) {
		// we try to balance the work-load for each daemon run
		// (beware, it's a very simple algorithm, yet effective)

		DaemonList::Iterator iterator = sDaemons.GetIterator();
		int32 num = 0;

		while (iterator.HasNext()) {
			if (iterator.Next()->frequency == frequency)
				num++;
		}

		daemon->offset = num % frequency;
	} else
		daemon->offset = 0;

	sDaemons.Add(daemon);
	return B_OK;
}


extern "C" status_t
kernel_daemon_init(void)
{
	thread_id thread;

	mutex_init(&sDaemonMutex, "kernel daemon");
	new(&sDaemons) DaemonList;

	thread = spawn_kernel_thread(&kernel_daemon, "kernel daemon",
		B_LOW_PRIORITY, NULL);
	send_signal_etc(thread, SIGCONT, B_DO_NOT_RESCHEDULE);

	return B_OK;
}
