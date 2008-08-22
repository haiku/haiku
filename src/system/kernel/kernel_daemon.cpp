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


class KernelDaemon {
public:
			status_t			Init(const char* name);

			status_t			Register(daemon_hook function, void* arg,
									int frequency);
			status_t			Unregister(daemon_hook function, void* arg);

private:
	static	status_t			_DaemonThreadEntry(void* data);
			status_t			_DaemonThread();

private:
			mutex				fLock;
			DaemonList			fDaemons;
			thread_id			fThread;
};


static KernelDaemon sKernelDaemon;
static KernelDaemon sResourceResizer;


status_t
KernelDaemon::Init(const char* name)
{
	new(&fDaemons) DaemonList;
	mutex_init(&fLock, name);

	fThread = spawn_kernel_thread(&_DaemonThreadEntry, name, B_LOW_PRIORITY,
		this);
	if (fThread < 0)
		return fThread;

	send_signal_etc(fThread, SIGCONT, B_DO_NOT_RESCHEDULE);

	return B_OK;
}


status_t
KernelDaemon::Register(daemon_hook function, void* arg, int frequency)
{
	if (function == NULL || frequency < 1)
		return B_BAD_VALUE;

	struct daemon* daemon = new(std::nothrow) struct ::daemon;
	if (daemon == NULL)
		return B_NO_MEMORY;

	daemon->function = function;
	daemon->arg = arg;
	daemon->frequency = frequency;

	MutexLocker _(fLock);

	if (frequency > 1) {
		// we try to balance the work-load for each daemon run
		// (beware, it's a very simple algorithm, yet effective)

		DaemonList::Iterator iterator = fDaemons.GetIterator();
		int32 num = 0;

		while (iterator.HasNext()) {
			if (iterator.Next()->frequency == frequency)
				num++;
		}

		daemon->offset = num % frequency;
	} else
		daemon->offset = 0;

	fDaemons.Add(daemon);
	return B_OK;
}


status_t
KernelDaemon::Unregister(daemon_hook function, void* arg)
{
	MutexLocker _(fLock);

	DaemonList::Iterator iterator = fDaemons.GetIterator();

	// search for the daemon and remove it from the list
	while (iterator.HasNext()) {
		struct daemon* daemon = iterator.Next();

		if (daemon->function == function && daemon->arg == arg) {
			// found it!
			iterator.Remove();
			delete daemon;
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


/*static*/ status_t
KernelDaemon::_DaemonThreadEntry(void* data)
{
	return ((KernelDaemon*)data)->_DaemonThread();
}


status_t
KernelDaemon::_DaemonThread()
{
	int32 iteration = 0;

	while (true) {
		mutex_lock(&fLock);

		DaemonList::Iterator iterator = fDaemons.GetIterator();

		// iterate through the list and execute each daemon if needed
		while (iterator.HasNext()) {
			struct daemon* daemon = iterator.Next();

			if (((iteration + daemon->offset) % daemon->frequency) == 0)
				daemon->function(daemon->arg, iteration);
		}
		mutex_unlock(&fLock);

		iteration++;
		snooze(100000);	// 0.1 seconds
	}

	return B_OK;
}


//	#pragma mark -


extern "C" status_t
register_kernel_daemon(daemon_hook function, void* arg, int frequency)
{
	return sKernelDaemon.Register(function, arg, frequency);
}


extern "C" status_t
unregister_kernel_daemon(daemon_hook function, void* arg)
{
	return sKernelDaemon.Unregister(function, arg);
}


extern "C" status_t
register_resource_resizer(daemon_hook function, void* arg, int frequency)
{
	return sResourceResizer.Register(function, arg, frequency);
}


extern "C" status_t
unregister_resource_resizer(daemon_hook function, void* arg)
{
	return sResourceResizer.Unregister(function, arg);
}


extern "C" status_t
kernel_daemon_init(void)
{
	new(&sKernelDaemon) KernelDaemon;
	if (sKernelDaemon.Init("kernel daemon") != B_OK)
		panic("kernel_daemon_init(): failed to init kernel daemon");

	new(&sResourceResizer) KernelDaemon;
	if (sResourceResizer.Init("resource resizer") != B_OK)
		panic("kernel_daemon_init(): failed to init resource resizer");

	return B_OK;
}
