/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <KernelExport.h>
#include <malloc.h>

#include <kernel_daemon.h>
#include <lock.h>
#include <util/list.h>


// ToDo: the use of snooze() in the kernel_daemon() function is very inaccurate, of
//	course - the time the daemons need to execute add up in each iteration.
//	But since the kernel daemon is documented to be very inaccurate, this actually
//	might be okay (and that's why it's implemented this way now :-).
//	BeOS R5 seems to do it in the same way, anyway.


struct daemon {
	list_link	link;
	daemon_hook	function;
	void		*arg;
	int32		frequency;
	int32		offset;
};


mutex gDaemonMutex;
struct list gDaemons;


static int32
kernel_daemon(void *data)
{
	int32 iteration = 0;

	while (true) {
		struct daemon *daemon = NULL;

		mutex_lock(&gDaemonMutex);

		// iterate through the list and execute each daemon if needed
		while ((daemon = list_get_next_item(&gDaemons, daemon)) != NULL) {
			if (((iteration + daemon->offset) % daemon->frequency) == 0)
				daemon->function(daemon->arg, iteration);
		}
		mutex_unlock(&gDaemonMutex);

		iteration++;
		snooze(100000);	// 0.1 seconds
	}
}


status_t
unregister_kernel_daemon(daemon_hook function, void *arg)
{
	struct daemon *daemon = NULL;

	mutex_lock(&gDaemonMutex);

	// search for the daemon and remove it from the list
	while ((daemon = list_get_next_item(&gDaemons, daemon)) != NULL) {
		if (daemon->function == function && daemon->arg == arg) {
			// found it!
			list_remove_item(&gDaemons, daemon);
			free(daemon);
			break;
		}
	}
	mutex_unlock(&gDaemonMutex);

	// if we've iterated through the whole list, we didn't
	// find the daemon, and "daemon" is NULL
	return daemon != NULL ? B_OK : B_ENTRY_NOT_FOUND;
}


status_t
register_kernel_daemon(daemon_hook function, void *arg, int frequency)
{
	struct daemon *daemon;

	if (function == NULL || frequency < 1)
		return B_BAD_VALUE;

	daemon = malloc(sizeof(struct daemon));
	if (daemon == NULL)
		return B_NO_MEMORY;

	daemon->function = function;
	daemon->arg = arg;
	daemon->frequency = frequency;

	mutex_lock(&gDaemonMutex);
	
	if (frequency > 1) {
		// we try to balance the work-load for each daemon run
		// (beware, it's a very simple algorithm, yet effective)

		struct daemon *d = NULL;
		int32 num = 0;

		while ((d = list_get_next_item(&gDaemons, d)) != NULL) {
			if (d->frequency == frequency)
				num++;
		}

		daemon->offset = num % frequency;
	} else
		daemon->offset = 0;

	list_add_item(&gDaemons, daemon);
	mutex_unlock(&gDaemonMutex);

	return B_OK;
}


status_t
kernel_daemon_init(void)
{
	thread_id thread;

	if (mutex_init(&gDaemonMutex, "kernel daemon") < B_OK)
		return B_ERROR;

	list_init(&gDaemons);

	thread = spawn_kernel_thread(&kernel_daemon, "kernel daemon", B_LOW_PRIORITY, NULL);
	return resume_thread(thread);
}
