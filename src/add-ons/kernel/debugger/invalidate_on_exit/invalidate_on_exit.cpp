/*
 * Copyright 2008, Michael Lotz, mmlr@mlotz.ch
 * Distributed under the terms of the MIT License.
 */


#include <debug.h>

#include <signal.h>

#include <kscheduler.h>
#include <smp.h>


static sem_id sRequestSem = -1;


static int32
invalidate_loop(void *data)
{
	while (true) {
		if (acquire_sem(sRequestSem) != B_OK)
			break;

		uint32 message[3];
		message[0] = sizeof(message);	// size
		message[1] = 'KDLE';			// message code
		message[2] = 0;					// flags

		// where "d:0:baron' stands for desktop x of user y which both
		// currently are hardcoded and where '_PTL' is the port link code
		write_port(find_port("d:0:baron"), '_PTL', &message, sizeof(message));
	}

	return 0;
}


static void
exit_debugger()
{
	// If someone holds the scheduler lock at this point, release_sem_etc()
	// will block forever. So avoid that.
	if (!try_acquire_spinlock(&gSchedulerLock))
		return;
	release_spinlock(&gSchedulerLock);

	release_sem_etc(sRequestSem, 1, B_DO_NOT_RESCHEDULE);
}


static status_t
std_ops(int32 op, ...)
{
	if (op == B_MODULE_INIT) {
		sRequestSem = create_sem(0, "invalidate_loop_request");
		if (sRequestSem < B_OK)
			return sRequestSem;

		thread_id thread = spawn_kernel_thread(&invalidate_loop,
			"invalidate_loop", B_NORMAL_PRIORITY, NULL);
		if (thread < B_OK)
			return thread;

		resume_thread(thread);
		return B_OK;
	} else if (op == B_MODULE_UNINIT) {
		// deleting the sem will also cause the thread to exit
		delete_sem(sRequestSem);
		sRequestSem = -1;
		return B_OK;
	}

	return B_BAD_VALUE;
}


static struct debugger_module_info sModuleInfo = {
	{
		"debugger/invalidate_on_exit/v1",
		B_KEEP_LOADED,
		&std_ops
	},

	NULL,
	exit_debugger,
	NULL,
	NULL
};

module_info *modules[] = {
	(module_info *)&sModuleInfo,
	NULL
};
