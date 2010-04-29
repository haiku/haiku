/*
 * Copyright 2003-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <kernel_daemon.h>

#include <new>
#include <stdlib.h>

#include <KernelExport.h>

#include <elf.h>
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
	bool		executing;
};


typedef DoublyLinkedList<struct daemon> DaemonList;


class KernelDaemon {
public:
			status_t			Init(const char* name);

			status_t			Register(daemon_hook function, void* arg,
									int frequency);
			status_t			Unregister(daemon_hook function, void* arg);

			void				Dump();

private:
	static	status_t			_DaemonThreadEntry(void* data);
			struct daemon*		_NextDaemon(struct daemon& marker);
			status_t			_DaemonThread();
			bool				_IsDaemon() const;

private:
			recursive_lock		fLock;
			DaemonList			fDaemons;
			thread_id			fThread;
			ConditionVariable	fUnregisterCondition;
			int32				fUnregisterWaiters;
};


static KernelDaemon sKernelDaemon;
static KernelDaemon sResourceResizer;


status_t
KernelDaemon::Init(const char* name)
{
	recursive_lock_init(&fLock, name);

	fThread = spawn_kernel_thread(&_DaemonThreadEntry, name, B_LOW_PRIORITY,
		this);
	if (fThread < 0)
		return fThread;

	resume_thread(fThread);
	fUnregisterCondition.Init(this, name);

	return B_OK;
}


status_t
KernelDaemon::Register(daemon_hook function, void* arg, int frequency)
{
	if (function == NULL || frequency < 1)
		return B_BAD_VALUE;

	struct ::daemon* daemon = new(std::nothrow) (struct ::daemon);
	if (daemon == NULL)
		return B_NO_MEMORY;

	daemon->function = function;
	daemon->arg = arg;
	daemon->frequency = frequency;
	daemon->executing = false;

	RecursiveLocker _(fLock);

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
	RecursiveLocker locker(fLock);

	DaemonList::Iterator iterator = fDaemons.GetIterator();

	// search for the daemon and remove it from the list
	while (iterator.HasNext()) {
		struct daemon* daemon = iterator.Next();

		if (daemon->function == function && daemon->arg == arg) {
			// found it!
			if (!_IsDaemon()) {
				// wait if it's busy
				while (daemon->executing) {
					fUnregisterWaiters++;

					ConditionVariableEntry entry;
					fUnregisterCondition.Add(&entry);

					locker.Unlock();

					entry.Wait();

					locker.Lock();
				}
			}

			iterator.Remove();
			delete daemon;
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


void
KernelDaemon::Dump()
{
	DaemonList::Iterator iterator = fDaemons.GetIterator();

	while (iterator.HasNext()) {
		struct daemon* daemon = iterator.Next();
		const char* imageName;
		const char* symbol;
		bool exactMatch;

		status_t status = elf_debug_lookup_symbol_address(
			(addr_t)daemon->function, NULL, &symbol, &imageName, &exactMatch);
		if (status == B_OK && exactMatch) {
			if (strchr(imageName, '/') != NULL)
				imageName = strrchr(imageName, '/') + 1;

			kprintf("\t%s:%s (%p)", imageName, symbol, daemon->function);
		} else
			kprintf("\t%p", daemon->function);

		kprintf(", arg %p%s\n", daemon->arg,
			daemon->executing ? " (running) " : "");
	}
}


/*static*/ status_t
KernelDaemon::_DaemonThreadEntry(void* data)
{
	return ((KernelDaemon*)data)->_DaemonThread();
}


struct daemon*
KernelDaemon::_NextDaemon(struct daemon& marker)
{
	struct daemon* daemon;

	if (marker.arg == NULL) {
		// The marker is not part of the list yet, just return the first entry
		daemon = fDaemons.Head();
	} else {
		daemon = fDaemons.GetNext(&marker);
		fDaemons.Remove(&marker);
	}

	marker.arg = daemon;

	if (daemon != NULL)
		fDaemons.InsertAfter(daemon, &marker);

	return daemon;
}


status_t
KernelDaemon::_DaemonThread()
{
	struct daemon marker;
	int32 iteration = 0;

	marker.arg = NULL;

	while (true) {
		RecursiveLocker locker(fLock);

		// iterate through the list and execute each daemon if needed
		while (struct daemon* daemon = _NextDaemon(marker)) {
			daemon->executing = true;
			locker.Unlock();

			if (((iteration + daemon->offset) % daemon->frequency) == 0)
				daemon->function(daemon->arg, iteration);

			locker.Lock();
			daemon->executing = false;
		}

		if (fUnregisterWaiters != 0) {
			fUnregisterCondition.NotifyAll();
			fUnregisterWaiters = 0;
		}

		locker.Unlock();

		iteration++;
		snooze(100000);	// 0.1 seconds
	}

	return B_OK;
}


bool
KernelDaemon::_IsDaemon() const
{
	return find_thread(NULL) == fThread;
}


// #pragma mark -


static int
dump_daemons(int argc, char** argv)
{
	kprintf("kernel daemons:\n");
	sKernelDaemon.Dump();

	kprintf("\nresource resizers:\n");
	sResourceResizer.Dump();

	return 0;
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


//	#pragma mark -


extern "C" status_t
kernel_daemon_init(void)
{
	new(&sKernelDaemon) KernelDaemon;
	if (sKernelDaemon.Init("kernel daemon") != B_OK)
		panic("kernel_daemon_init(): failed to init kernel daemon");

	new(&sResourceResizer) KernelDaemon;
	if (sResourceResizer.Init("resource resizer") != B_OK)
		panic("kernel_daemon_init(): failed to init resource resizer");

	add_debugger_command("daemons", dump_daemons,
		"Shows registered kernel daemons.");
	return B_OK;
}
