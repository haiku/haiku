/*
 * Copyright 2005-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


/*! Threading routines */


#include <thread.h>

#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>

#include <OS.h>

#include <util/AutoLock.h>
#include <util/khash.h>

#include <arch/debug.h>
#include <boot/kernel_args.h>
#include <condition_variable.h>
#include <cpu.h>
#include <int.h>
#include <kimage.h>
#include <kscheduler.h>
#include <ksignal.h>
#include <Notifications.h>
#include <real_time_clock.h>
#include <slab/Slab.h>
#include <smp.h>
#include <syscalls.h>
#include <syscall_restart.h>
#include <team.h>
#include <tls.h>
#include <user_runtime.h>
#include <user_thread.h>
#include <vfs.h>
#include <vm/vm.h>
#include <vm/VMAddressSpace.h>
#include <wait_for_objects.h>


//#define TRACE_THREAD
#ifdef TRACE_THREAD
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#define THREAD_MAX_MESSAGE_SIZE		65536


struct thread_key {
	thread_id id;
};

// global
spinlock gThreadSpinlock = B_SPINLOCK_INITIALIZER;

// thread list
static Thread sIdleThreads[B_MAX_CPU_COUNT];
static hash_table *sThreadHash = NULL;
static thread_id sNextThreadID = 1;

// some arbitrary chosen limits - should probably depend on the available
// memory (the limit is not yet enforced)
static int32 sMaxThreads = 4096;
static int32 sUsedThreads = 0;

struct UndertakerEntry : DoublyLinkedListLinkImpl<UndertakerEntry> {
	Thread*	thread;
	team_id	teamID;

	UndertakerEntry(Thread* thread, team_id teamID)
		:
		thread(thread),
		teamID(teamID)
	{
	}
};


class ThreadNotificationService : public DefaultNotificationService {
public:
	ThreadNotificationService()
		: DefaultNotificationService("threads")
	{
	}

	void Notify(uint32 eventCode, team_id teamID, thread_id threadID,
		Thread* thread = NULL)
	{
		char eventBuffer[128];
		KMessage event;
		event.SetTo(eventBuffer, sizeof(eventBuffer), THREAD_MONITOR);
		event.AddInt32("event", eventCode);
		event.AddInt32("team", teamID);
		event.AddInt32("thread", threadID);
		if (thread != NULL)
			event.AddPointer("threadStruct", thread);

		DefaultNotificationService::Notify(event, eventCode);
	}

	void Notify(uint32 eventCode, Thread* thread)
	{
		return Notify(eventCode, thread->id, thread->team->id, thread);
	}
};


static DoublyLinkedList<UndertakerEntry> sUndertakerEntries;
static ConditionVariable sUndertakerCondition;
static ThreadNotificationService sNotificationService;


// object cache to allocate thread structures from
static object_cache* sThreadCache;

static void thread_kthread_entry(void);
static void thread_kthread_exit(void);


/*!	Inserts a thread into a team.
	You must hold the team lock when you call this function.
*/
static void
insert_thread_into_team(Team *team, Thread *thread)
{
	thread->team_next = team->thread_list;
	team->thread_list = thread;
	team->num_threads++;

	if (team->num_threads == 1) {
		// this was the first thread
		team->main_thread = thread;
	}
	thread->team = team;
}


/*!	Removes a thread from a team.
	You must hold the team lock when you call this function.
*/
static void
remove_thread_from_team(Team *team, Thread *thread)
{
	Thread *temp, *last = NULL;

	for (temp = team->thread_list; temp != NULL; temp = temp->team_next) {
		if (temp == thread) {
			if (last == NULL)
				team->thread_list = temp->team_next;
			else
				last->team_next = temp->team_next;

			team->num_threads--;
			break;
		}
		last = temp;
	}
}


static int
thread_struct_compare(void *_t, const void *_key)
{
	Thread *thread = (Thread*)_t;
	const struct thread_key *key = (const struct thread_key*)_key;

	if (thread->id == key->id)
		return 0;

	return 1;
}


static uint32
thread_struct_hash(void *_t, const void *_key, uint32 range)
{
	Thread *thread = (Thread*)_t;
	const struct thread_key *key = (const struct thread_key*)_key;

	if (thread != NULL)
		return thread->id % range;

	return (uint32)key->id % range;
}


static void
reset_signals(Thread *thread)
{
	thread->sig_pending = 0;
	thread->sig_block_mask = 0;
	thread->sig_temp_enabled = 0;
	memset(thread->sig_action, 0, 32 * sizeof(struct sigaction));
	thread->signal_stack_base = 0;
	thread->signal_stack_size = 0;
	thread->signal_stack_enabled = false;
}


/*!	Allocates and fills in thread structure.

	\param threadID The ID to be assigned to the new thread. If
		  \code < 0 \endcode a fresh one is allocated.
	\param thread initialize this thread struct if nonnull
*/
static Thread *
create_thread_struct(Thread *inthread, const char *name,
	thread_id threadID, struct cpu_ent *cpu)
{
	Thread *thread;
	char temp[64];

	if (inthread == NULL) {
		thread = (Thread*)object_cache_alloc(sThreadCache, 0);
		if (thread == NULL)
			return NULL;

		scheduler_on_thread_create(thread);
			// TODO: We could use the object cache object
			// constructor/destructor!
	} else
		thread = inthread;

	if (name != NULL)
		strlcpy(thread->name, name, B_OS_NAME_LENGTH);
	else
		strcpy(thread->name, "unnamed thread");

	thread->flags = 0;
	thread->id = threadID >= 0 ? threadID : allocate_thread_id();
	thread->team = NULL;
	thread->cpu = cpu;
	thread->previous_cpu = NULL;
	thread->pinned_to_cpu = 0;
	thread->fault_handler = 0;
	thread->page_faults_allowed = 1;
	thread->kernel_stack_area = -1;
	thread->kernel_stack_base = 0;
	thread->user_stack_area = -1;
	thread->user_stack_base = 0;
	thread->user_local_storage = 0;
	thread->kernel_errno = 0;
	thread->team_next = NULL;
	thread->queue_next = NULL;
	thread->priority = thread->next_priority = -1;
	thread->io_priority = -1;
	thread->args1 = NULL;  thread->args2 = NULL;
	thread->alarm.period = 0;
	reset_signals(thread);
	thread->in_kernel = true;
	thread->was_yielded = false;
	thread->user_time = 0;
	thread->kernel_time = 0;
	thread->last_time = 0;
	thread->exit.status = 0;
	thread->exit.reason = 0;
	thread->exit.signal = 0;
	list_init(&thread->exit.waiters);
	thread->select_infos = NULL;
	thread->post_interrupt_callback = NULL;
	thread->post_interrupt_data = NULL;
	thread->user_thread = NULL;

	sprintf(temp, "thread_%ld_retcode_sem", thread->id);
	thread->exit.sem = create_sem(0, temp);
	if (thread->exit.sem < B_OK)
		goto err1;

	sprintf(temp, "%s send", thread->name);
	thread->msg.write_sem = create_sem(1, temp);
	if (thread->msg.write_sem < B_OK)
		goto err2;

	sprintf(temp, "%s receive", thread->name);
	thread->msg.read_sem = create_sem(0, temp);
	if (thread->msg.read_sem < B_OK)
		goto err3;

	if (arch_thread_init_thread_struct(thread) < B_OK)
		goto err4;

	return thread;

err4:
	delete_sem(thread->msg.read_sem);
err3:
	delete_sem(thread->msg.write_sem);
err2:
	delete_sem(thread->exit.sem);
err1:
	// ToDo: put them in the dead queue instead?
	if (inthread == NULL) {
		scheduler_on_thread_destroy(thread);
		object_cache_free(sThreadCache, thread, 0);
	}

	return NULL;
}


static void
delete_thread_struct(Thread *thread)
{
	delete_sem(thread->exit.sem);
	delete_sem(thread->msg.write_sem);
	delete_sem(thread->msg.read_sem);

	scheduler_on_thread_destroy(thread);
	object_cache_free(sThreadCache, thread, 0);
}


/*! This function gets run by a new thread before anything else */
static void
thread_kthread_entry(void)
{
	Thread *thread = thread_get_current_thread();

	// The thread is new and has been scheduled the first time. Notify the user
	// debugger code.
	if ((thread->flags & THREAD_FLAGS_DEBUGGER_INSTALLED) != 0)
		user_debug_thread_scheduled(thread);

	// simulates the thread spinlock release that would occur if the thread had been
	// rescheded from. The resched didn't happen because the thread is new.
	RELEASE_THREAD_LOCK();

	// start tracking time
	thread->last_time = system_time();

	enable_interrupts(); // this essentially simulates a return-from-interrupt
}


static void
thread_kthread_exit(void)
{
	Thread *thread = thread_get_current_thread();

	thread->exit.reason = THREAD_RETURN_EXIT;
	thread_exit();
}


/*!	Initializes the thread and jumps to its userspace entry point.
	This function is called at creation time of every user thread,
	but not for a team's main thread.
*/
static int
_create_user_thread_kentry(void)
{
	Thread *thread = thread_get_current_thread();

	// jump to the entry point in user space
	arch_thread_enter_userspace(thread, (addr_t)thread->entry,
		thread->args1, thread->args2);

	// only get here if the above call fails
	return 0;
}


/*! Initializes the thread and calls it kernel space entry point. */
static int
_create_kernel_thread_kentry(void)
{
	Thread *thread = thread_get_current_thread();
	int (*func)(void *args) = (int (*)(void *))thread->entry;

	// call the entry function with the appropriate args
	return func(thread->args1);
}


/*!	Creates a new thread in the team with the specified team ID.

	\param threadID The ID to be assigned to the new thread. If
		  \code < 0 \endcode a fresh one is allocated.
*/
static thread_id
create_thread(thread_creation_attributes& attributes, bool kernel)
{
	Thread *thread, *currentThread;
	Team *team;
	cpu_status state;
	char stack_name[B_OS_NAME_LENGTH];
	status_t status;
	bool abort = false;
	bool debugNewThread = false;

	TRACE(("create_thread(%s, id = %ld, %s)\n", attributes.name,
		attributes.thread, kernel ? "kernel" : "user"));

	thread = create_thread_struct(NULL, attributes.name, attributes.thread,
		NULL);
	if (thread == NULL)
		return B_NO_MEMORY;

	thread->priority = attributes.priority == -1
		? B_NORMAL_PRIORITY : attributes.priority;
	thread->next_priority = thread->priority;
	// ToDo: this could be dangerous in case someone calls resume_thread() on us
	thread->state = B_THREAD_SUSPENDED;
	thread->next_state = B_THREAD_SUSPENDED;

	// init debug structure
	init_thread_debug_info(&thread->debug_info);

	snprintf(stack_name, B_OS_NAME_LENGTH, "%s_%ld_kstack", attributes.name,
		thread->id);
	thread->kernel_stack_area = create_area(stack_name,
		(void **)&thread->kernel_stack_base, B_ANY_KERNEL_ADDRESS,
		KERNEL_STACK_SIZE + KERNEL_STACK_GUARD_PAGES  * B_PAGE_SIZE,
		B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_KERNEL_STACK_AREA);

	if (thread->kernel_stack_area < 0) {
		// we're not yet part of a team, so we can just bail out
		status = thread->kernel_stack_area;

		dprintf("create_thread: error creating kernel stack: %s!\n",
			strerror(status));

		delete_thread_struct(thread);
		return status;
	}

	thread->kernel_stack_top = thread->kernel_stack_base + KERNEL_STACK_SIZE
		+ KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE;

	state = disable_interrupts();
	GRAB_THREAD_LOCK();

	// If the new thread belongs to the same team as the current thread,
	// it may inherit some of the thread debug flags.
	currentThread = thread_get_current_thread();
	if (currentThread && currentThread->team->id == attributes.team) {
		// inherit all user flags...
		int32 debugFlags = currentThread->debug_info.flags
			& B_THREAD_DEBUG_USER_FLAG_MASK;

		// ... save the syscall tracing flags, unless explicitely specified
		if (!(debugFlags & B_THREAD_DEBUG_SYSCALL_TRACE_CHILD_THREADS)) {
			debugFlags &= ~(B_THREAD_DEBUG_PRE_SYSCALL
				| B_THREAD_DEBUG_POST_SYSCALL);
		}

		thread->debug_info.flags = debugFlags;

		// stop the new thread, if desired
		debugNewThread = debugFlags & B_THREAD_DEBUG_STOP_CHILD_THREADS;

		// copy signal handlers
		memcpy(thread->sig_action, currentThread->sig_action,
			sizeof(thread->sig_action));
	}

	// insert into global list
	hash_insert(sThreadHash, thread);
	sUsedThreads++;
	scheduler_on_thread_init(thread);
	RELEASE_THREAD_LOCK();

	GRAB_TEAM_LOCK();
	// look at the team, make sure it's not being deleted
	team = team_get_team_struct_locked(attributes.team);

	if (team == NULL || team->state == TEAM_STATE_DEATH
		|| team->death_entry != NULL) {
		abort = true;
	}

	if (!abort && !kernel) {
		thread->user_thread = team_allocate_user_thread(team);
		abort = thread->user_thread == NULL;
	}

	if (!abort) {
		// Debug the new thread, if the parent thread required that (see above),
		// or the respective global team debug flag is set. But only, if a
		// debugger is installed for the team.
		debugNewThread |= (atomic_get(&team->debug_info.flags)
			& B_TEAM_DEBUG_STOP_NEW_THREADS);
		if (debugNewThread
			&& (atomic_get(&team->debug_info.flags)
				& B_TEAM_DEBUG_DEBUGGER_INSTALLED)) {
			thread->debug_info.flags |= B_THREAD_DEBUG_STOP;
		}

		insert_thread_into_team(team, thread);
	}

	RELEASE_TEAM_LOCK();
	if (abort) {
		GRAB_THREAD_LOCK();
		hash_remove(sThreadHash, thread);
		RELEASE_THREAD_LOCK();
	}
	restore_interrupts(state);
	if (abort) {
		delete_area(thread->kernel_stack_area);
		delete_thread_struct(thread);
		return B_BAD_TEAM_ID;
	}

	thread->args1 = attributes.args1;
	thread->args2 = attributes.args2;
	thread->entry = attributes.entry;
	status = thread->id;

	// notify listeners
	sNotificationService.Notify(THREAD_ADDED, thread);

	if (kernel) {
		// this sets up an initial kthread stack that runs the entry

		// Note: whatever function wants to set up a user stack later for this
		// thread must initialize the TLS for it
		arch_thread_init_kthread_stack(thread, &_create_kernel_thread_kentry,
			&thread_kthread_entry, &thread_kthread_exit);
	} else {
		// create user stack

		// the stack will be between USER_STACK_REGION and the main thread stack
		// area (the user stack of the main thread is created in
		// team_create_team())
		if (attributes.stack_address == NULL) {
			thread->user_stack_base = USER_STACK_REGION;
			if (attributes.stack_size <= 0)
				thread->user_stack_size = USER_STACK_SIZE;
			else
				thread->user_stack_size = PAGE_ALIGN(attributes.stack_size);
			thread->user_stack_size += USER_STACK_GUARD_PAGES * B_PAGE_SIZE;

			snprintf(stack_name, B_OS_NAME_LENGTH, "%s_%ld_stack",
				attributes.name, thread->id);
			virtual_address_restrictions virtualRestrictions = {};
			virtualRestrictions.address = (void*)thread->user_stack_base;
			virtualRestrictions.address_specification = B_BASE_ADDRESS;
			physical_address_restrictions physicalRestrictions = {};
			thread->user_stack_area = create_area_etc(team->id, stack_name,
				thread->user_stack_size + TLS_SIZE, B_NO_LOCK,
				B_READ_AREA | B_WRITE_AREA | B_STACK_AREA, 0,
				&virtualRestrictions, &physicalRestrictions,
				(void**)&thread->user_stack_base);
			if (thread->user_stack_area < B_OK
				|| arch_thread_init_tls(thread) < B_OK) {
				// great, we have a fully running thread without a (usable)
				// stack
				dprintf("create_thread: unable to create proper user stack!\n");
				status = thread->user_stack_area;
				kill_thread(thread->id);
			}
		} else {
			thread->user_stack_base = (addr_t)attributes.stack_address;
			thread->user_stack_size = attributes.stack_size;
		}

		user_debug_update_new_thread_flags(thread->id);

		// copy the user entry over to the args field in the thread struct
		// the function this will call will immediately switch the thread into
		// user space.
		arch_thread_init_kthread_stack(thread, &_create_user_thread_kentry,
			&thread_kthread_entry, &thread_kthread_exit);
	}

	return status;
}


static status_t
undertaker(void* /*args*/)
{
	while (true) {
		// wait for a thread to bury
		InterruptsSpinLocker locker(gThreadSpinlock);

		while (sUndertakerEntries.IsEmpty()) {
			ConditionVariableEntry conditionEntry;
			sUndertakerCondition.Add(&conditionEntry);
			locker.Unlock();

			conditionEntry.Wait();

			locker.Lock();
		}

		UndertakerEntry* _entry = sUndertakerEntries.RemoveHead();
		locker.Unlock();

		UndertakerEntry entry = *_entry;
			// we need a copy, since the original entry is on the thread's stack

		// we've got an entry
		Thread* thread = entry.thread;

		// delete the old kernel stack area
		delete_area(thread->kernel_stack_area);

		// remove this thread from all of the global lists
		disable_interrupts();
		GRAB_TEAM_LOCK();

		remove_thread_from_team(team_get_kernel_team(), thread);

		RELEASE_TEAM_LOCK();
		enable_interrupts();
			// needed for the debugger notification below

		// free the thread structure
		delete_thread_struct(thread);
	}

	// never can get here
	return B_OK;
}


static sem_id
get_thread_wait_sem(Thread* thread)
{
	if (thread->state == B_THREAD_WAITING
		&& thread->wait.type == THREAD_BLOCK_TYPE_SEMAPHORE) {
		return (sem_id)(addr_t)thread->wait.object;
	}
	return -1;
}


/*!	Fills the thread_info structure with information from the specified
	thread.
	The thread lock must be held when called.
*/
static void
fill_thread_info(Thread *thread, thread_info *info, size_t size)
{
	info->thread = thread->id;
	info->team = thread->team->id;

	strlcpy(info->name, thread->name, B_OS_NAME_LENGTH);

	if (thread->state == B_THREAD_WAITING) {
		info->state = B_THREAD_WAITING;

		switch (thread->wait.type) {
			case THREAD_BLOCK_TYPE_SNOOZE:
				info->state = B_THREAD_ASLEEP;
				break;

			case THREAD_BLOCK_TYPE_SEMAPHORE:
			{
				sem_id sem = (sem_id)(addr_t)thread->wait.object;
				if (sem == thread->msg.read_sem)
					info->state = B_THREAD_RECEIVING;
				break;
			}

			case THREAD_BLOCK_TYPE_CONDITION_VARIABLE:
			default:
				break;
		}
	} else
		info->state = (thread_state)thread->state;

	info->priority = thread->priority;
	info->user_time = thread->user_time;
	info->kernel_time = thread->kernel_time;
	info->stack_base = (void *)thread->user_stack_base;
	info->stack_end = (void *)(thread->user_stack_base
		+ thread->user_stack_size);
	info->sem = get_thread_wait_sem(thread);
}


static status_t
send_data_etc(thread_id id, int32 code, const void *buffer, size_t bufferSize,
	int32 flags)
{
	Thread *target;
	sem_id cachedSem;
	cpu_status state;
	status_t status;

	state = disable_interrupts();
	GRAB_THREAD_LOCK();
	target = thread_get_thread_struct_locked(id);
	if (!target) {
		RELEASE_THREAD_LOCK();
		restore_interrupts(state);
		return B_BAD_THREAD_ID;
	}
	cachedSem = target->msg.write_sem;
	RELEASE_THREAD_LOCK();
	restore_interrupts(state);

	if (bufferSize > THREAD_MAX_MESSAGE_SIZE)
		return B_NO_MEMORY;

	status = acquire_sem_etc(cachedSem, 1, flags, 0);
	if (status == B_INTERRUPTED) {
		// We got interrupted by a signal
		return status;
	}
	if (status != B_OK) {
		// Any other acquisition problems may be due to thread deletion
		return B_BAD_THREAD_ID;
	}

	void* data;
	if (bufferSize > 0) {
		data = malloc(bufferSize);
		if (data == NULL)
			return B_NO_MEMORY;
		if (user_memcpy(data, buffer, bufferSize) != B_OK) {
			free(data);
			return B_BAD_DATA;
		}
	} else
		data = NULL;

	state = disable_interrupts();
	GRAB_THREAD_LOCK();

	// The target thread could have been deleted at this point
	target = thread_get_thread_struct_locked(id);
	if (target == NULL) {
		RELEASE_THREAD_LOCK();
		restore_interrupts(state);
		free(data);
		return B_BAD_THREAD_ID;
	}

	// Save message informations
	target->msg.sender = thread_get_current_thread()->id;
	target->msg.code = code;
	target->msg.size = bufferSize;
	target->msg.buffer = data;
	cachedSem = target->msg.read_sem;

	RELEASE_THREAD_LOCK();
	restore_interrupts(state);

	release_sem(cachedSem);
	return B_OK;
}


static int32
receive_data_etc(thread_id *_sender, void *buffer, size_t bufferSize,
	int32 flags)
{
	Thread *thread = thread_get_current_thread();
	status_t status;
	size_t size;
	int32 code;

	status = acquire_sem_etc(thread->msg.read_sem, 1, flags, 0);
	if (status < B_OK) {
		// Actually, we're not supposed to return error codes
		// but since the only reason this can fail is that we
		// were killed, it's probably okay to do so (but also
		// meaningless).
		return status;
	}

	if (buffer != NULL && bufferSize != 0 && thread->msg.buffer != NULL) {
		size = min_c(bufferSize, thread->msg.size);
		status = user_memcpy(buffer, thread->msg.buffer, size);
		if (status != B_OK) {
			free(thread->msg.buffer);
			release_sem(thread->msg.write_sem);
			return status;
		}
	}

	*_sender = thread->msg.sender;
	code = thread->msg.code;

	free(thread->msg.buffer);
	release_sem(thread->msg.write_sem);

	return code;
}


static status_t
common_getrlimit(int resource, struct rlimit * rlp)
{
	if (!rlp)
		return B_BAD_ADDRESS;

	switch (resource) {
		case RLIMIT_NOFILE:
		case RLIMIT_NOVMON:
			return vfs_getrlimit(resource, rlp);

		case RLIMIT_CORE:
			rlp->rlim_cur = 0;
			rlp->rlim_max = 0;
			return B_OK;

		case RLIMIT_STACK:
		{
			Thread *thread = thread_get_current_thread();
			if (!thread)
				return B_ERROR;
			rlp->rlim_cur = thread->user_stack_size;
			rlp->rlim_max = thread->user_stack_size;
			return B_OK;
		}

		default:
			return EINVAL;
	}

	return B_OK;
}


static status_t
common_setrlimit(int resource, const struct rlimit * rlp)
{
	if (!rlp)
		return B_BAD_ADDRESS;

	switch (resource) {
		case RLIMIT_NOFILE:
		case RLIMIT_NOVMON:
			return vfs_setrlimit(resource, rlp);

		case RLIMIT_CORE:
			// We don't support core file, so allow settings to 0/0 only.
			if (rlp->rlim_cur != 0 || rlp->rlim_max != 0)
				return EINVAL;
			return B_OK;

		default:
			return EINVAL;
	}

	return B_OK;
}


//	#pragma mark - debugger calls


static int
make_thread_unreal(int argc, char **argv)
{
	Thread *thread;
	struct hash_iterator i;
	int32 id = -1;

	if (argc > 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	if (argc > 1)
		id = strtoul(argv[1], NULL, 0);

	hash_open(sThreadHash, &i);

	while ((thread = (Thread*)hash_next(sThreadHash, &i)) != NULL) {
		if (id != -1 && thread->id != id)
			continue;

		if (thread->priority > B_DISPLAY_PRIORITY) {
			thread->priority = thread->next_priority = B_NORMAL_PRIORITY;
			kprintf("thread %ld made unreal\n", thread->id);
		}
	}

	hash_close(sThreadHash, &i, false);
	return 0;
}


static int
set_thread_prio(int argc, char **argv)
{
	Thread *thread;
	struct hash_iterator i;
	int32 id;
	int32 prio;

	if (argc > 3 || argc < 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	prio = strtoul(argv[1], NULL, 0);
	if (prio > THREAD_MAX_SET_PRIORITY)
		prio = THREAD_MAX_SET_PRIORITY;
	if (prio < THREAD_MIN_SET_PRIORITY)
		prio = THREAD_MIN_SET_PRIORITY;

	if (argc > 2)
		id = strtoul(argv[2], NULL, 0);
	else
		id = thread_get_current_thread()->id;

	hash_open(sThreadHash, &i);

	while ((thread = (Thread*)hash_next(sThreadHash, &i)) != NULL) {
		if (thread->id != id)
			continue;
		thread->priority = thread->next_priority = prio;
		kprintf("thread %ld set to priority %ld\n", id, prio);
		break;
	}
	if (!thread)
		kprintf("thread %ld (%#lx) not found\n", id, id);

	hash_close(sThreadHash, &i, false);
	return 0;
}


static int
make_thread_suspended(int argc, char **argv)
{
	Thread *thread;
	struct hash_iterator i;
	int32 id;

	if (argc > 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	if (argc == 1)
		id = thread_get_current_thread()->id;
	else
		id = strtoul(argv[1], NULL, 0);

	hash_open(sThreadHash, &i);

	while ((thread = (Thread*)hash_next(sThreadHash, &i)) != NULL) {
		if (thread->id != id)
			continue;

		thread->next_state = B_THREAD_SUSPENDED;
		kprintf("thread %ld suspended\n", id);
		break;
	}
	if (!thread)
		kprintf("thread %ld (%#lx) not found\n", id, id);

	hash_close(sThreadHash, &i, false);
	return 0;
}


static int
make_thread_resumed(int argc, char **argv)
{
	Thread *thread;
	struct hash_iterator i;
	int32 id;

	if (argc != 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	// force user to enter a thread id, as using
	// the current thread is usually not intended
	id = strtoul(argv[1], NULL, 0);

	hash_open(sThreadHash, &i);

	while ((thread = (Thread*)hash_next(sThreadHash, &i)) != NULL) {
		if (thread->id != id)
			continue;

		if (thread->state == B_THREAD_SUSPENDED) {
			scheduler_enqueue_in_run_queue(thread);
			kprintf("thread %ld resumed\n", thread->id);
		}
		break;
	}
	if (!thread)
		kprintf("thread %ld (%#lx) not found\n", id, id);

	hash_close(sThreadHash, &i, false);
	return 0;
}


static int
drop_into_debugger(int argc, char **argv)
{
	status_t err;
	int32 id;

	if (argc > 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	if (argc == 1)
		id = thread_get_current_thread()->id;
	else
		id = strtoul(argv[1], NULL, 0);

	err = _user_debug_thread(id);
	if (err)
		kprintf("drop failed\n");
	else
		kprintf("thread %ld dropped into user debugger\n", id);

	return 0;
}


static const char *
state_to_text(Thread *thread, int32 state)
{
	switch (state) {
		case B_THREAD_READY:
			return "ready";

		case B_THREAD_RUNNING:
			return "running";

		case B_THREAD_WAITING:
		{
			if (thread != NULL) {
				switch (thread->wait.type) {
					case THREAD_BLOCK_TYPE_SNOOZE:
						return "zzz";

					case THREAD_BLOCK_TYPE_SEMAPHORE:
					{
						sem_id sem = (sem_id)(addr_t)thread->wait.object;
						if (sem == thread->msg.read_sem)
							return "receive";
						break;
					}
				}
			}

			return "waiting";
		}

		case B_THREAD_SUSPENDED:
			return "suspended";

		case THREAD_STATE_FREE_ON_RESCHED:
			return "death";

		default:
			return "UNKNOWN";
	}
}


static void
print_thread_list_table_head()
{
	kprintf("thread         id  state     wait for   object  cpu pri  stack    "
		"  team  name\n");
}


static void
_dump_thread_info(Thread *thread, bool shortInfo)
{
	if (shortInfo) {
		kprintf("%p %6ld  %-10s", thread, thread->id, state_to_text(thread,
			thread->state));

		// does it block on a semaphore or a condition variable?
		if (thread->state == B_THREAD_WAITING) {
			switch (thread->wait.type) {
				case THREAD_BLOCK_TYPE_SEMAPHORE:
				{
					sem_id sem = (sem_id)(addr_t)thread->wait.object;
					if (sem == thread->msg.read_sem)
						kprintf("                    ");
					else
						kprintf("sem  %12ld   ", sem);
					break;
				}

				case THREAD_BLOCK_TYPE_CONDITION_VARIABLE:
					kprintf("cvar   %p   ", thread->wait.object);
					break;

				case THREAD_BLOCK_TYPE_SNOOZE:
					kprintf("                    ");
					break;

				case THREAD_BLOCK_TYPE_SIGNAL:
					kprintf("signal              ");
					break;

				case THREAD_BLOCK_TYPE_MUTEX:
					kprintf("mutex  %p   ", thread->wait.object);
					break;

				case THREAD_BLOCK_TYPE_RW_LOCK:
					kprintf("rwlock %p   ", thread->wait.object);
					break;

				case THREAD_BLOCK_TYPE_OTHER:
					kprintf("other               ");
					break;

				default:
					kprintf("???    %p   ", thread->wait.object);
					break;
			}
		} else
			kprintf("        -           ");

		// on which CPU does it run?
		if (thread->cpu)
			kprintf("%2d", thread->cpu->cpu_num);
		else
			kprintf(" -");

		kprintf("%4ld  %p%5ld  %s\n", thread->priority,
			(void *)thread->kernel_stack_base, thread->team->id,
			thread->name != NULL ? thread->name : "<NULL>");

		return;
	}

	// print the long info

	struct death_entry *death = NULL;

	kprintf("THREAD: %p\n", thread);
	kprintf("id:                 %ld (%#lx)\n", thread->id, thread->id);
	kprintf("name:               \"%s\"\n", thread->name);
	kprintf("all_next:           %p\nteam_next:          %p\nq_next:             %p\n",
		thread->all_next, thread->team_next, thread->queue_next);
	kprintf("priority:           %ld (next %ld, I/O: %ld)\n", thread->priority,
		thread->next_priority, thread->io_priority);
	kprintf("state:              %s\n", state_to_text(thread, thread->state));
	kprintf("next_state:         %s\n", state_to_text(thread, thread->next_state));
	kprintf("cpu:                %p ", thread->cpu);
	if (thread->cpu)
		kprintf("(%d)\n", thread->cpu->cpu_num);
	else
		kprintf("\n");
	kprintf("sig_pending:        %#" B_PRIx32 " (blocked: %#" B_PRIx32
		", temp enabled: %#" B_PRIx32 ")\n", thread->sig_pending,
		thread->sig_block_mask, thread->sig_temp_enabled);
	kprintf("in_kernel:          %d\n", thread->in_kernel);

	if (thread->state == B_THREAD_WAITING) {
		kprintf("waiting for:        ");

		switch (thread->wait.type) {
			case THREAD_BLOCK_TYPE_SEMAPHORE:
			{
				sem_id sem = (sem_id)(addr_t)thread->wait.object;
				if (sem == thread->msg.read_sem)
					kprintf("data\n");
				else
					kprintf("semaphore %ld\n", sem);
				break;
			}

			case THREAD_BLOCK_TYPE_CONDITION_VARIABLE:
				kprintf("condition variable %p\n", thread->wait.object);
				break;

			case THREAD_BLOCK_TYPE_SNOOZE:
				kprintf("snooze()\n");
				break;

			case THREAD_BLOCK_TYPE_SIGNAL:
				kprintf("signal\n");
				break;

			case THREAD_BLOCK_TYPE_MUTEX:
				kprintf("mutex %p\n", thread->wait.object);
				break;

			case THREAD_BLOCK_TYPE_RW_LOCK:
				kprintf("rwlock %p\n", thread->wait.object);
				break;

			case THREAD_BLOCK_TYPE_OTHER:
				kprintf("other (%s)\n", (char*)thread->wait.object);
				break;

			default:
				kprintf("unknown (%p)\n", thread->wait.object);
				break;
		}
	}

	kprintf("fault_handler:      %p\n", (void *)thread->fault_handler);
	kprintf("args:               %p %p\n", thread->args1, thread->args2);
	kprintf("entry:              %p\n", (void *)thread->entry);
	kprintf("team:               %p, \"%s\"\n", thread->team, thread->team->name);
	kprintf("  exit.sem:         %ld\n", thread->exit.sem);
	kprintf("  exit.status:      %#lx (%s)\n", thread->exit.status, strerror(thread->exit.status));
	kprintf("  exit.reason:      %#x\n", thread->exit.reason);
	kprintf("  exit.signal:      %#x\n", thread->exit.signal);
	kprintf("  exit.waiters:\n");
	while ((death = (struct death_entry*)list_get_next_item(
			&thread->exit.waiters, death)) != NULL) {
		kprintf("\t%p (group %ld, thread %ld)\n", death, death->group_id, death->thread);
	}

	kprintf("kernel_stack_area:  %ld\n", thread->kernel_stack_area);
	kprintf("kernel_stack_base:  %p\n", (void *)thread->kernel_stack_base);
	kprintf("user_stack_area:    %ld\n", thread->user_stack_area);
	kprintf("user_stack_base:    %p\n", (void *)thread->user_stack_base);
	kprintf("user_local_storage: %p\n", (void *)thread->user_local_storage);
	kprintf("user_thread:        %p\n", (void *)thread->user_thread);
	kprintf("kernel_errno:       %#x (%s)\n", thread->kernel_errno,
		strerror(thread->kernel_errno));
	kprintf("kernel_time:        %Ld\n", thread->kernel_time);
	kprintf("user_time:          %Ld\n", thread->user_time);
	kprintf("flags:              0x%lx\n", thread->flags);
	kprintf("architecture dependant section:\n");
	arch_thread_dump_info(&thread->arch_info);
}


static int
dump_thread_info(int argc, char **argv)
{
	bool shortInfo = false;
	int argi = 1;
	if (argi < argc && strcmp(argv[argi], "-s") == 0) {
		shortInfo = true;
		print_thread_list_table_head();
		argi++;
	}

	if (argi == argc) {
		_dump_thread_info(thread_get_current_thread(), shortInfo);
		return 0;
	}

	for (; argi < argc; argi++) {
		const char *name = argv[argi];
		int32 id = strtoul(name, NULL, 0);

		if (IS_KERNEL_ADDRESS(id)) {
			// semi-hack
			_dump_thread_info((Thread *)id, shortInfo);
			continue;
		}

		// walk through the thread list, trying to match name or id
		bool found = false;
		struct hash_iterator i;
		hash_open(sThreadHash, &i);
		Thread *thread;
		while ((thread = (Thread*)hash_next(sThreadHash, &i)) != NULL) {
			if (!strcmp(name, thread->name) || thread->id == id) {
				_dump_thread_info(thread, shortInfo);
				found = true;
				break;
			}
		}
		hash_close(sThreadHash, &i, false);

		if (!found)
			kprintf("thread \"%s\" (%ld) doesn't exist!\n", name, id);
	}

	return 0;
}


static int
dump_thread_list(int argc, char **argv)
{
	Thread *thread;
	struct hash_iterator i;
	bool realTimeOnly = false;
	bool calling = false;
	const char *callSymbol = NULL;
	addr_t callStart = 0;
	addr_t callEnd = 0;
	int32 requiredState = 0;
	team_id team = -1;
	sem_id sem = -1;

	if (!strcmp(argv[0], "realtime"))
		realTimeOnly = true;
	else if (!strcmp(argv[0], "ready"))
		requiredState = B_THREAD_READY;
	else if (!strcmp(argv[0], "running"))
		requiredState = B_THREAD_RUNNING;
	else if (!strcmp(argv[0], "waiting")) {
		requiredState = B_THREAD_WAITING;

		if (argc > 1) {
			sem = strtoul(argv[1], NULL, 0);
			if (sem == 0)
				kprintf("ignoring invalid semaphore argument.\n");
		}
	} else if (!strcmp(argv[0], "calling")) {
		if (argc < 2) {
			kprintf("Need to give a symbol name or start and end arguments.\n");
			return 0;
		} else if (argc == 3) {
			callStart = parse_expression(argv[1]);
			callEnd = parse_expression(argv[2]);
		} else
			callSymbol = argv[1];

		calling = true;
	} else if (argc > 1) {
		team = strtoul(argv[1], NULL, 0);
		if (team == 0)
			kprintf("ignoring invalid team argument.\n");
	}

	print_thread_list_table_head();

	hash_open(sThreadHash, &i);
	while ((thread = (Thread*)hash_next(sThreadHash, &i)) != NULL) {
		// filter out threads not matching the search criteria
		if ((requiredState && thread->state != requiredState)
			|| (calling && !arch_debug_contains_call(thread, callSymbol,
					callStart, callEnd))
			|| (sem > 0 && get_thread_wait_sem(thread) != sem)
			|| (team > 0 && thread->team->id != team)
			|| (realTimeOnly && thread->priority < B_REAL_TIME_DISPLAY_PRIORITY))
			continue;

		_dump_thread_info(thread, true);
	}
	hash_close(sThreadHash, &i, false);
	return 0;
}


//	#pragma mark - private kernel API


void
thread_exit(void)
{
	cpu_status state;
	Thread *thread = thread_get_current_thread();
	Team *team = thread->team;
	thread_id parentID = -1;
	status_t status;
	struct thread_debug_info debugInfo;
	team_id teamID = team->id;

	TRACE(("thread %ld exiting %s w/return code %#lx\n", thread->id,
		thread->exit.reason == THREAD_RETURN_INTERRUPTED
			? "due to signal" : "normally", thread->exit.status));

	if (!are_interrupts_enabled())
		panic("thread_exit() called with interrupts disabled!\n");

	// boost our priority to get this over with
	thread->priority = thread->next_priority = B_URGENT_DISPLAY_PRIORITY;

	// Cancel previously installed alarm timer, if any
	cancel_timer(&thread->alarm);

	// remember the user stack area -- we will delete it below
	area_id userStackArea = -1;
	if (team->address_space != NULL && thread->user_stack_area >= 0) {
		userStackArea = thread->user_stack_area;
		thread->user_stack_area = -1;
	}

	struct job_control_entry *death = NULL;
	struct death_entry* threadDeathEntry = NULL;
	bool deleteTeam = false;
	port_id debuggerPort = -1;

	if (team != team_get_kernel_team()) {
		user_debug_thread_exiting(thread);

		if (team->main_thread == thread) {
			// The main thread is exiting. Shut down the whole team.
			deleteTeam = true;
		} else {
			threadDeathEntry = (death_entry*)malloc(sizeof(death_entry));
			team_free_user_thread(thread);
		}

		// remove this thread from the current team and add it to the kernel
		// put the thread into the kernel team until it dies
		state = disable_interrupts();
		GRAB_TEAM_LOCK();

		if (deleteTeam)
			debuggerPort = team_shutdown_team(team, state);

		GRAB_THREAD_LOCK();
			// removing the thread and putting its death entry to the parent
			// team needs to be an atomic operation

		// remember how long this thread lasted
		team->dead_threads_kernel_time += thread->kernel_time;
		team->dead_threads_user_time += thread->user_time;

		remove_thread_from_team(team, thread);
		insert_thread_into_team(team_get_kernel_team(), thread);

		if (team->death_entry != NULL) {
			if (--team->death_entry->remaining_threads == 0)
				team->death_entry->condition.NotifyOne(true, B_OK);
		}

		if (deleteTeam) {
			Team *parent = team->parent;

			// remember who our parent was so we can send a signal
			parentID = parent->id;

			// Set the team job control state to "dead" and detach the job
			// control entry from our team struct.
			team_set_job_control_state(team, JOB_CONTROL_STATE_DEAD, 0, true);
			death = team->job_control_entry;
			team->job_control_entry = NULL;

			if (death != NULL) {
				death->InitDeadState();

				// team_set_job_control_state() already moved our entry
				// into the parent's list. We just check the soft limit of
				// death entries.
				if (parent->dead_children.count > MAX_DEAD_CHILDREN) {
					death = parent->dead_children.entries.RemoveHead();
					parent->dead_children.count--;
				} else
					death = NULL;

				RELEASE_THREAD_LOCK();
			} else
				RELEASE_THREAD_LOCK();

			team_remove_team(team);

			send_signal_etc(parentID, SIGCHLD,
				SIGNAL_FLAG_TEAMS_LOCKED | B_DO_NOT_RESCHEDULE);
		} else {
			// The thread is not the main thread. We store a thread death
			// entry for it, unless someone is already waiting it.
			if (threadDeathEntry != NULL
				&& list_is_empty(&thread->exit.waiters)) {
				threadDeathEntry->thread = thread->id;
				threadDeathEntry->status = thread->exit.status;
				threadDeathEntry->reason = thread->exit.reason;
				threadDeathEntry->signal = thread->exit.signal;

				// add entry -- remove and old one, if we hit the limit
				list_add_item(&team->dead_threads, threadDeathEntry);
				team->dead_threads_count++;
				threadDeathEntry = NULL;

				if (team->dead_threads_count > MAX_DEAD_THREADS) {
					threadDeathEntry = (death_entry*)list_remove_head_item(
						&team->dead_threads);
					team->dead_threads_count--;
				}
			}

			RELEASE_THREAD_LOCK();
		}

		RELEASE_TEAM_LOCK();

		// swap address spaces, to make sure we're running on the kernel's pgdir
		vm_swap_address_space(team->address_space, VMAddressSpace::Kernel());
		restore_interrupts(state);

		TRACE(("thread_exit: thread %ld now a kernel thread!\n", thread->id));
	}

	free(threadDeathEntry);

	// delete the team if we're its main thread
	if (deleteTeam) {
		team_delete_team(team, debuggerPort);

		// we need to delete any death entry that made it to here
		delete death;
	}

	state = disable_interrupts();
	GRAB_THREAD_LOCK();

	// remove thread from hash, so it's no longer accessible
	hash_remove(sThreadHash, thread);
	sUsedThreads--;

	// Stop debugging for this thread
	debugInfo = thread->debug_info;
	clear_thread_debug_info(&thread->debug_info, true);

	// Remove the select infos. We notify them a little later.
	select_info* selectInfos = thread->select_infos;
	thread->select_infos = NULL;

	RELEASE_THREAD_LOCK();
	restore_interrupts(state);

	destroy_thread_debug_info(&debugInfo);

	// notify select infos
	select_info* info = selectInfos;
	while (info != NULL) {
		select_sync* sync = info->sync;

		notify_select_events(info, B_EVENT_INVALID);
		info = info->next;
		put_select_sync(sync);
	}

	// notify listeners
	sNotificationService.Notify(THREAD_REMOVED, thread);

	// shutdown the thread messaging

	status = acquire_sem_etc(thread->msg.write_sem, 1, B_RELATIVE_TIMEOUT, 0);
	if (status == B_WOULD_BLOCK) {
		// there is data waiting for us, so let us eat it
		thread_id sender;

		delete_sem(thread->msg.write_sem);
			// first, let's remove all possibly waiting writers
		receive_data_etc(&sender, NULL, 0, B_RELATIVE_TIMEOUT);
	} else {
		// we probably own the semaphore here, and we're the last to do so
		delete_sem(thread->msg.write_sem);
	}
	// now we can safely remove the msg.read_sem
	delete_sem(thread->msg.read_sem);

	// fill all death entries and delete the sem that others will use to wait on us
	{
		sem_id cachedExitSem = thread->exit.sem;
		cpu_status state;

		state = disable_interrupts();
		GRAB_THREAD_LOCK();

		// make sure no one will grab this semaphore again
		thread->exit.sem = -1;

		// fill all death entries
		death_entry* entry = NULL;
		while ((entry = (struct death_entry*)list_get_next_item(
				&thread->exit.waiters, entry)) != NULL) {
			entry->status = thread->exit.status;
			entry->reason = thread->exit.reason;
			entry->signal = thread->exit.signal;
		}

		RELEASE_THREAD_LOCK();
		restore_interrupts(state);

		delete_sem(cachedExitSem);
	}

	// delete the user stack, if this was a user thread
	if (!deleteTeam && userStackArea >= 0) {
		// We postponed deleting the user stack until now, since this way all
		// notifications for the thread's death are out already and all other
		// threads waiting for this thread's death and some object on its stack
		// will wake up before we (try to) delete the stack area. Of most
		// relevance is probably the case where this is the main thread and
		// other threads use objects on its stack -- so we want them terminated
		// first.
		// When the team is deleted, all areas are deleted anyway, so we don't
		// need to do that explicitly in that case.
		vm_delete_area(teamID, userStackArea, true);
	}

	// notify the debugger
	if (teamID != team_get_kernel_team_id())
		user_debug_thread_deleted(teamID, thread->id);

	// enqueue in the undertaker list and reschedule for the last time
	UndertakerEntry undertakerEntry(thread, teamID);

	disable_interrupts();
	GRAB_THREAD_LOCK();

	sUndertakerEntries.Add(&undertakerEntry);
	sUndertakerCondition.NotifyOne(true);

	thread->next_state = THREAD_STATE_FREE_ON_RESCHED;
	scheduler_reschedule();

	panic("never can get here\n");
}


Thread *
thread_get_thread_struct(thread_id id)
{
	Thread *thread;
	cpu_status state;

	state = disable_interrupts();
	GRAB_THREAD_LOCK();

	thread = thread_get_thread_struct_locked(id);

	RELEASE_THREAD_LOCK();
	restore_interrupts(state);

	return thread;
}


Thread *
thread_get_thread_struct_locked(thread_id id)
{
	struct thread_key key;

	key.id = id;

	return (Thread*)hash_lookup(sThreadHash, &key);
}


/*!	Called in the interrupt handler code when a thread enters
	the kernel for any reason.
	Only tracks time for now.
	Interrupts are disabled.
*/
void
thread_at_kernel_entry(bigtime_t now)
{
	Thread *thread = thread_get_current_thread();

	TRACE(("thread_at_kernel_entry: entry thread %ld\n", thread->id));

	// track user time
	thread->user_time += now - thread->last_time;
	thread->last_time = now;

	thread->in_kernel = true;
}


/*!	Called whenever a thread exits kernel space to user space.
	Tracks time, handles signals, ...
	Interrupts must be enabled. When the function returns, interrupts will be
	disabled.
*/
void
thread_at_kernel_exit(void)
{
	Thread *thread = thread_get_current_thread();

	TRACE(("thread_at_kernel_exit: exit thread %ld\n", thread->id));

	while (handle_signals(thread)) {
		InterruptsSpinLocker _(gThreadSpinlock);
		scheduler_reschedule();
	}

	disable_interrupts();

	thread->in_kernel = false;

	// track kernel time
	bigtime_t now = system_time();
	thread->kernel_time += now - thread->last_time;
	thread->last_time = now;
}


/*!	The quick version of thread_kernel_exit(), in case no signals are pending
	and no debugging shall be done.
	Interrupts must be disabled.
*/
void
thread_at_kernel_exit_no_signals(void)
{
	Thread *thread = thread_get_current_thread();

	TRACE(("thread_at_kernel_exit_no_signals: exit thread %ld\n", thread->id));

	thread->in_kernel = false;

	// track kernel time
	bigtime_t now = system_time();
	thread->kernel_time += now - thread->last_time;
	thread->last_time = now;
}


void
thread_reset_for_exec(void)
{
	Thread *thread = thread_get_current_thread();

	reset_signals(thread);

	// Note: We don't cancel an alarm. It is supposed to survive exec*().
}


/*! Insert a thread to the tail of a queue */
void
thread_enqueue(Thread *thread, struct thread_queue *queue)
{
	thread->queue_next = NULL;
	if (queue->head == NULL) {
		queue->head = thread;
		queue->tail = thread;
	} else {
		queue->tail->queue_next = thread;
		queue->tail = thread;
	}
}


Thread *
thread_lookat_queue(struct thread_queue *queue)
{
	return queue->head;
}


Thread *
thread_dequeue(struct thread_queue *queue)
{
	Thread *thread = queue->head;

	if (thread != NULL) {
		queue->head = thread->queue_next;
		if (queue->tail == thread)
			queue->tail = NULL;
	}
	return thread;
}


Thread *
thread_dequeue_id(struct thread_queue *q, thread_id id)
{
	Thread *thread;
	Thread *last = NULL;

	thread = q->head;
	while (thread != NULL) {
		if (thread->id == id) {
			if (last == NULL)
				q->head = thread->queue_next;
			else
				last->queue_next = thread->queue_next;

			if (q->tail == thread)
				q->tail = last;
			break;
		}
		last = thread;
		thread = thread->queue_next;
	}
	return thread;
}


Thread*
thread_iterate_through_threads(thread_iterator_callback callback, void* cookie)
{
	struct hash_iterator iterator;
	hash_open(sThreadHash, &iterator);

	Thread* thread;
	while ((thread = (Thread*)hash_next(sThreadHash, &iterator))
			!= NULL) {
		if (callback(thread, cookie))
			break;
	}

	hash_close(sThreadHash, &iterator, false);

	return thread;
}


thread_id
allocate_thread_id(void)
{
	return atomic_add(&sNextThreadID, 1);
}


thread_id
peek_next_thread_id(void)
{
	return atomic_get(&sNextThreadID);
}


/*!	Yield the CPU to other threads.
	If \a force is \c true, the thread will almost guaranteedly be unscheduled.
	If \c false, it will continue to run, if there's no other thread in ready
	state, and if it has a higher priority than the other ready threads, it
	still has a good chance to continue.
*/
void
thread_yield(bool force)
{
	if (force) {
		// snooze for roughly 3 thread quantums
		snooze_etc(9000, B_SYSTEM_TIMEBASE, B_RELATIVE_TIMEOUT | B_CAN_INTERRUPT);
#if 0
		cpu_status state;

		Thread *thread = thread_get_current_thread();
		if (thread == NULL)
			return;

		state = disable_interrupts();
		GRAB_THREAD_LOCK();

		// mark the thread as yielded, so it will not be scheduled next
		//thread->was_yielded = true;
		thread->next_priority = B_LOWEST_ACTIVE_PRIORITY;
		scheduler_reschedule();

		RELEASE_THREAD_LOCK();
		restore_interrupts(state);
#endif
	} else {
		Thread *thread = thread_get_current_thread();
		if (thread == NULL)
			return;

		// Don't force the thread off the CPU, just reschedule.
		InterruptsSpinLocker _(gThreadSpinlock);
		scheduler_reschedule();
	}
}


/*!	Kernel private thread creation function.

	\param threadID The ID to be assigned to the new thread. If
		  \code < 0 \endcode a fresh one is allocated.
*/
thread_id
spawn_kernel_thread_etc(thread_func function, const char *name, int32 priority,
	void *arg, team_id team, thread_id threadID)
{
	thread_creation_attributes attributes;
	attributes.entry = (thread_entry_func)function;
	attributes.name = name;
	attributes.priority = priority;
	attributes.args1 = arg;
	attributes.args2 = NULL;
	attributes.stack_address = NULL;
	attributes.stack_size = 0;
	attributes.team = team;
	attributes.thread = threadID;

	return create_thread(attributes, true);
}


status_t
wait_for_thread_etc(thread_id id, uint32 flags, bigtime_t timeout,
	status_t *_returnCode)
{
	sem_id exitSem = B_BAD_THREAD_ID;
	struct death_entry death;
	job_control_entry* freeDeath = NULL;
	Thread *thread;
	cpu_status state;
	status_t status = B_OK;

	if (id < B_OK)
		return B_BAD_THREAD_ID;

	// we need to resume the thread we're waiting for first

	state = disable_interrupts();
	GRAB_THREAD_LOCK();

	thread = thread_get_thread_struct_locked(id);
	if (thread != NULL) {
		// remember the semaphore we have to wait on and place our death entry
		exitSem = thread->exit.sem;
		list_add_link_to_head(&thread->exit.waiters, &death);
	}

	death_entry* threadDeathEntry = NULL;

	RELEASE_THREAD_LOCK();

	if (thread == NULL) {
		// we couldn't find this thread - maybe it's already gone, and we'll
		// find its death entry in our team
		GRAB_TEAM_LOCK();

		Team* team = thread_get_current_thread()->team;

		// check the child death entries first (i.e. main threads of child
		// teams)
		bool deleteEntry;
		freeDeath = team_get_death_entry(team, id, &deleteEntry);
		if (freeDeath != NULL) {
			death.status = freeDeath->status;
			if (!deleteEntry)
				freeDeath = NULL;
		} else {
			// check the thread death entries of the team (non-main threads)
			while ((threadDeathEntry = (death_entry*)list_get_next_item(
					&team->dead_threads, threadDeathEntry)) != NULL) {
				if (threadDeathEntry->thread == id) {
					list_remove_item(&team->dead_threads, threadDeathEntry);
					team->dead_threads_count--;
					death.status = threadDeathEntry->status;
					break;
				}
			}

			if (threadDeathEntry == NULL)
				status = B_BAD_THREAD_ID;
		}

		RELEASE_TEAM_LOCK();
	}

	restore_interrupts(state);

	if (thread == NULL && status == B_OK) {
		// we found the thread's death entry in our team
		if (_returnCode)
			*_returnCode = death.status;

		delete freeDeath;
		free(threadDeathEntry);
		return B_OK;
	}

	// we need to wait for the death of the thread

	if (exitSem < B_OK)
		return B_BAD_THREAD_ID;

	resume_thread(id);
		// make sure we don't wait forever on a suspended thread

	status = acquire_sem_etc(exitSem, 1, flags, timeout);

	if (status == B_OK) {
		// this should never happen as the thread deletes the semaphore on exit
		panic("could acquire exit_sem for thread %ld\n", id);
	} else if (status == B_BAD_SEM_ID) {
		// this is the way the thread normally exits
		status = B_OK;

		if (_returnCode)
			*_returnCode = death.status;
	} else {
		// We were probably interrupted; we need to remove our death entry now.
		state = disable_interrupts();
		GRAB_THREAD_LOCK();

		thread = thread_get_thread_struct_locked(id);
		if (thread != NULL)
			list_remove_link(&death);

		RELEASE_THREAD_LOCK();
		restore_interrupts(state);

		// If the thread is already gone, we need to wait for its exit semaphore
		// to make sure our death entry stays valid - it won't take long
		if (thread == NULL)
			acquire_sem(exitSem);
	}

	return status;
}


status_t
select_thread(int32 id, struct select_info* info, bool kernel)
{
	InterruptsSpinLocker locker(gThreadSpinlock);

	// get thread
	Thread* thread = thread_get_thread_struct_locked(id);
	if (thread == NULL)
		return B_BAD_THREAD_ID;

	// We support only B_EVENT_INVALID at the moment.
	info->selected_events &= B_EVENT_INVALID;

	// add info to list
	if (info->selected_events != 0) {
		info->next = thread->select_infos;
		thread->select_infos = info;

		// we need a sync reference
		atomic_add(&info->sync->ref_count, 1);
	}

	return B_OK;
}


status_t
deselect_thread(int32 id, struct select_info* info, bool kernel)
{
	InterruptsSpinLocker locker(gThreadSpinlock);

	// get thread
	Thread* thread = thread_get_thread_struct_locked(id);
	if (thread == NULL)
		return B_BAD_THREAD_ID;

	// remove info from list
	select_info** infoLocation = &thread->select_infos;
	while (*infoLocation != NULL && *infoLocation != info)
		infoLocation = &(*infoLocation)->next;

	if (*infoLocation != info)
		return B_OK;

	*infoLocation = info->next;

	locker.Unlock();

	// surrender sync reference
	put_select_sync(info->sync);

	return B_OK;
}


int32
thread_max_threads(void)
{
	return sMaxThreads;
}


int32
thread_used_threads(void)
{
	return sUsedThreads;
}


const char*
thread_state_to_text(Thread* thread, int32 state)
{
	return state_to_text(thread, state);
}


int32
thread_get_io_priority(thread_id id)
{
	// take a shortcut, if it is the current thread
	Thread* thread = thread_get_current_thread();
	int32 priority;
	if (id == thread->id) {
		int32 priority = thread->io_priority;
		return priority < 0 ? thread->priority : priority;
	}

	// not the current thread -- get it
	InterruptsSpinLocker locker(gThreadSpinlock);

	thread = thread_get_thread_struct_locked(id);
	if (thread == NULL)
		return B_BAD_THREAD_ID;

	priority = thread->io_priority;
	return priority < 0 ? thread->priority : priority;
}


void
thread_set_io_priority(int32 priority)
{
	Thread* thread = thread_get_current_thread();
	thread->io_priority = priority;
}


status_t
thread_init(kernel_args *args)
{
	uint32 i;

	TRACE(("thread_init: entry\n"));

	// create the thread hash table
	sThreadHash = hash_init(15, offsetof(Thread, all_next),
		&thread_struct_compare, &thread_struct_hash);

	// create the thread structure object cache
	sThreadCache = create_object_cache("threads", sizeof(Thread), 16, NULL,
		NULL, NULL);
		// Note: The x86 port requires 16 byte alignment of thread structures.
	if (sThreadCache == NULL)
		panic("thread_init(): failed to allocate thread object cache!");

	if (arch_thread_init(args) < B_OK)
		panic("arch_thread_init() failed!\n");

	// skip all thread IDs including B_SYSTEM_TEAM, which is reserved
	sNextThreadID = B_SYSTEM_TEAM + 1;

	// create an idle thread for each cpu

	for (i = 0; i < args->num_cpus; i++) {
		Thread *thread;
		area_info info;
		char name[64];

		sprintf(name, "idle thread %lu", i + 1);
		thread = create_thread_struct(&sIdleThreads[i], name,
			i == 0 ? team_get_kernel_team_id() : -1, &gCPU[i]);
		if (thread == NULL) {
			panic("error creating idle thread struct\n");
			return B_NO_MEMORY;
		}

		gCPU[i].running_thread = thread;

		thread->team = team_get_kernel_team();
		thread->priority = thread->next_priority = B_IDLE_PRIORITY;
		thread->state = B_THREAD_RUNNING;
		thread->next_state = B_THREAD_READY;
		sprintf(name, "idle thread %lu kstack", i + 1);
		thread->kernel_stack_area = find_area(name);
		thread->entry = NULL;

		if (get_area_info(thread->kernel_stack_area, &info) != B_OK)
			panic("error finding idle kstack area\n");

		thread->kernel_stack_base = (addr_t)info.address;
		thread->kernel_stack_top = thread->kernel_stack_base + info.size;

		hash_insert(sThreadHash, thread);
		insert_thread_into_team(thread->team, thread);
	}
	sUsedThreads = args->num_cpus;

	// init the notification service
	new(&sNotificationService) ThreadNotificationService();

	// start the undertaker thread
	new(&sUndertakerEntries) DoublyLinkedList<UndertakerEntry>();
	sUndertakerCondition.Init(&sUndertakerEntries, "undertaker entries");

	thread_id undertakerThread = spawn_kernel_thread(&undertaker, "undertaker",
		B_DISPLAY_PRIORITY, NULL);
	if (undertakerThread < 0)
		panic("Failed to create undertaker thread!");
	resume_thread(undertakerThread);

	// set up some debugger commands
	add_debugger_command_etc("threads", &dump_thread_list, "List all threads",
		"[ <team> ]\n"
		"Prints a list of all existing threads, or, if a team ID is given,\n"
		"all threads of the specified team.\n"
		"  <team>  - The ID of the team whose threads shall be listed.\n", 0);
	add_debugger_command_etc("ready", &dump_thread_list,
		"List all ready threads",
		"\n"
		"Prints a list of all threads in ready state.\n", 0);
	add_debugger_command_etc("running", &dump_thread_list,
		"List all running threads",
		"\n"
		"Prints a list of all threads in running state.\n", 0);
	add_debugger_command_etc("waiting", &dump_thread_list,
		"List all waiting threads (optionally for a specific semaphore)",
		"[ <sem> ]\n"
		"Prints a list of all threads in waiting state. If a semaphore is\n"
		"specified, only the threads waiting on that semaphore are listed.\n"
		"  <sem>  - ID of the semaphore.\n", 0);
	add_debugger_command_etc("realtime", &dump_thread_list,
		"List all realtime threads",
		"\n"
		"Prints a list of all threads with realtime priority.\n", 0);
	add_debugger_command_etc("thread", &dump_thread_info,
		"Dump info about a particular thread",
		"[ -s ] ( <id> | <address> | <name> )*\n"
		"Prints information about the specified thread. If no argument is\n"
		"given the current thread is selected.\n"
		"  -s         - Print info in compact table form (like \"threads\").\n"
		"  <id>       - The ID of the thread.\n"
		"  <address>  - The address of the thread structure.\n"
		"  <name>     - The thread's name.\n", 0);
	add_debugger_command_etc("calling", &dump_thread_list,
		"Show all threads that have a specific address in their call chain",
		"{ <symbol-pattern> | <start> <end> }\n", 0);
	add_debugger_command_etc("unreal", &make_thread_unreal,
		"Set realtime priority threads to normal priority",
		"[ <id> ]\n"
		"Sets the priority of all realtime threads or, if given, the one\n"
		"with the specified ID to \"normal\" priority.\n"
		"  <id>  - The ID of the thread.\n", 0);
	add_debugger_command_etc("suspend", &make_thread_suspended,
		"Suspend a thread",
		"[ <id> ]\n"
		"Suspends the thread with the given ID. If no ID argument is given\n"
		"the current thread is selected.\n"
		"  <id>  - The ID of the thread.\n", 0);
	add_debugger_command_etc("resume", &make_thread_resumed, "Resume a thread",
		"<id>\n"
		"Resumes the specified thread, if it is currently suspended.\n"
		"  <id>  - The ID of the thread.\n", 0);
	add_debugger_command_etc("drop", &drop_into_debugger,
		"Drop a thread into the userland debugger",
		"<id>\n"
		"Drops the specified (userland) thread into the userland debugger\n"
		"after leaving the kernel debugger.\n"
		"  <id>  - The ID of the thread.\n", 0);
	add_debugger_command_etc("priority", &set_thread_prio,
		"Set a thread's priority",
		"<priority> [ <id> ]\n"
		"Sets the priority of the thread with the specified ID to the given\n"
		"priority. If no thread ID is given, the current thread is selected.\n"
		"  <priority>  - The thread's new priority (0 - 120)\n"
		"  <id>        - The ID of the thread.\n", 0);

	return B_OK;
}


status_t
thread_preboot_init_percpu(struct kernel_args *args, int32 cpuNum)
{
	// set up the cpu pointer in the not yet initialized per-cpu idle thread
	// so that get_current_cpu and friends will work, which is crucial for
	// a lot of low level routines
	sIdleThreads[cpuNum].cpu = &gCPU[cpuNum];
	arch_thread_set_current_thread(&sIdleThreads[cpuNum]);
	return B_OK;
}


//	#pragma mark - thread blocking API


static status_t
thread_block_timeout(timer* timer)
{
	// The timer has been installed with B_TIMER_ACQUIRE_THREAD_LOCK, so
	// we're holding the thread lock already. This makes things comfortably
	// easy.

	Thread* thread = (Thread*)timer->user_data;
	thread_unblock_locked(thread, B_TIMED_OUT);

	return B_HANDLED_INTERRUPT;
}


status_t
thread_block()
{
	InterruptsSpinLocker _(gThreadSpinlock);
	return thread_block_locked(thread_get_current_thread());
}


void
thread_unblock(status_t threadID, status_t status)
{
	InterruptsSpinLocker _(gThreadSpinlock);

	Thread* thread = thread_get_thread_struct_locked(threadID);
	if (thread != NULL)
		thread_unblock_locked(thread, status);
}


status_t
thread_block_with_timeout(uint32 timeoutFlags, bigtime_t timeout)
{
	InterruptsSpinLocker _(gThreadSpinlock);
	return thread_block_with_timeout_locked(timeoutFlags, timeout);
}


status_t
thread_block_with_timeout_locked(uint32 timeoutFlags, bigtime_t timeout)
{
	Thread* thread = thread_get_current_thread();

	if (thread->wait.status != 1)
		return thread->wait.status;

	bool useTimer = (timeoutFlags & (B_RELATIVE_TIMEOUT | B_ABSOLUTE_TIMEOUT))
		&& timeout != B_INFINITE_TIMEOUT;

	if (useTimer) {
		// Timer flags: absolute/relative + "acquire thread lock". The latter
		// avoids nasty race conditions and deadlock problems that could
		// otherwise occur between our cancel_timer() and a concurrently
		// executing thread_block_timeout().
		uint32 timerFlags;
		if ((timeoutFlags & B_RELATIVE_TIMEOUT) != 0) {
			timerFlags = B_ONE_SHOT_RELATIVE_TIMER;
		} else {
			timerFlags = B_ONE_SHOT_ABSOLUTE_TIMER;
			if ((timeoutFlags & B_TIMEOUT_REAL_TIME_BASE) != 0)
				timeout -= rtc_boot_time();
		}
		timerFlags |= B_TIMER_ACQUIRE_THREAD_LOCK;

		// install the timer
		thread->wait.unblock_timer.user_data = thread;
		add_timer(&thread->wait.unblock_timer, &thread_block_timeout, timeout,
			timerFlags);
	}

	// block
	status_t error = thread_block_locked(thread);

	// cancel timer, if it didn't fire
	if (error != B_TIMED_OUT && useTimer)
		cancel_timer(&thread->wait.unblock_timer);

	return error;
}


/*!	Thread spinlock must be held.
*/
static status_t
user_unblock_thread(thread_id threadID, status_t status)
{
	Thread* thread = thread_get_thread_struct_locked(threadID);
	if (thread == NULL)
		return B_BAD_THREAD_ID;
	if (thread->user_thread == NULL)
		return B_NOT_ALLOWED;

	if (thread->user_thread->wait_status > 0) {
		thread->user_thread->wait_status = status;
		thread_unblock_locked(thread, status);
	}

	return B_OK;
}


//	#pragma mark - public kernel API


void
exit_thread(status_t returnValue)
{
	Thread *thread = thread_get_current_thread();

	thread->exit.status = returnValue;
	thread->exit.reason = THREAD_RETURN_EXIT;

	// if called from a kernel thread, we don't deliver the signal,
	// we just exit directly to keep the user space behaviour of
	// this function
	if (thread->team != team_get_kernel_team())
		send_signal_etc(thread->id, SIGKILLTHR, B_DO_NOT_RESCHEDULE);
	else
		thread_exit();
}


status_t
kill_thread(thread_id id)
{
	if (id <= 0)
		return B_BAD_VALUE;

	return send_signal(id, SIGKILLTHR);
}


status_t
send_data(thread_id thread, int32 code, const void *buffer, size_t bufferSize)
{
	return send_data_etc(thread, code, buffer, bufferSize, 0);
}


int32
receive_data(thread_id *sender, void *buffer, size_t bufferSize)
{
	return receive_data_etc(sender, buffer, bufferSize, 0);
}


bool
has_data(thread_id thread)
{
	int32 count;

	if (get_sem_count(thread_get_current_thread()->msg.read_sem,
			&count) != B_OK)
		return false;

	return count == 0 ? false : true;
}


status_t
_get_thread_info(thread_id id, thread_info *info, size_t size)
{
	status_t status = B_OK;
	Thread *thread;
	cpu_status state;

	if (info == NULL || size != sizeof(thread_info) || id < B_OK)
		return B_BAD_VALUE;

	state = disable_interrupts();
	GRAB_THREAD_LOCK();

	thread = thread_get_thread_struct_locked(id);
	if (thread == NULL) {
		status = B_BAD_VALUE;
		goto err;
	}

	fill_thread_info(thread, info, size);

err:
	RELEASE_THREAD_LOCK();
	restore_interrupts(state);

	return status;
}


status_t
_get_next_thread_info(team_id teamID, int32 *_cookie, thread_info *info,
	size_t size)
{
	if (info == NULL || size != sizeof(thread_info) || teamID < 0)
		return B_BAD_VALUE;

	int32 lastID = *_cookie;

	InterruptsSpinLocker teamLocker(gTeamSpinlock);

	Team* team;
	if (teamID == B_CURRENT_TEAM)
		team = thread_get_current_thread()->team;
	else
		team = team_get_team_struct_locked(teamID);

	if (team == NULL)
		return B_BAD_VALUE;

	Thread* thread = NULL;

	if (lastID == 0) {
		// We start with the main thread
		thread = team->main_thread;
	} else {
		// Find the one thread with an ID higher than ours
		// (as long as the IDs don't overlap they are always sorted from
		// highest to lowest).
		for (Thread* next = team->thread_list; next != NULL;
				next = next->team_next) {
			if (next->id <= lastID)
				break;

			thread = next;
		}
	}

	if (thread == NULL)
		return B_BAD_VALUE;

	lastID = thread->id;
	*_cookie = lastID;

	SpinLocker threadLocker(gThreadSpinlock);
	fill_thread_info(thread, info, size);

	return B_OK;
}


thread_id
find_thread(const char *name)
{
	struct hash_iterator iterator;
	Thread *thread;
	cpu_status state;

	if (name == NULL)
		return thread_get_current_thread_id();

	state = disable_interrupts();
	GRAB_THREAD_LOCK();

	// ToDo: this might not be in the same order as find_thread() in BeOS
	//		which could be theoretically problematic.
	// ToDo: scanning the whole list with the thread lock held isn't exactly
	//		cheap either - although this function is probably used very rarely.

	hash_open(sThreadHash, &iterator);
	while ((thread = (Thread*)hash_next(sThreadHash, &iterator))
			!= NULL) {
		// Search through hash
		if (!strcmp(thread->name, name)) {
			thread_id id = thread->id;

			RELEASE_THREAD_LOCK();
			restore_interrupts(state);
			return id;
		}
	}

	RELEASE_THREAD_LOCK();
	restore_interrupts(state);

	return B_NAME_NOT_FOUND;
}


status_t
rename_thread(thread_id id, const char* name)
{
	Thread* thread = thread_get_current_thread();

	if (name == NULL)
		return B_BAD_VALUE;

	InterruptsSpinLocker locker(gThreadSpinlock);

	if (thread->id != id) {
		thread = thread_get_thread_struct_locked(id);
		if (thread == NULL)
			return B_BAD_THREAD_ID;
		if (thread->team != thread_get_current_thread()->team)
			return B_NOT_ALLOWED;
	}

	strlcpy(thread->name, name, B_OS_NAME_LENGTH);

	team_id teamID = thread->team->id;

	locker.Unlock();

	// notify listeners
	sNotificationService.Notify(THREAD_NAME_CHANGED, teamID, id);
		// don't pass the thread structure, as it's unsafe, if it isn't ours

	return B_OK;
}


status_t
set_thread_priority(thread_id id, int32 priority)
{
	Thread *thread;
	int32 oldPriority;

	// make sure the passed in priority is within bounds
	if (priority > THREAD_MAX_SET_PRIORITY)
		priority = THREAD_MAX_SET_PRIORITY;
	if (priority < THREAD_MIN_SET_PRIORITY)
		priority = THREAD_MIN_SET_PRIORITY;

	thread = thread_get_current_thread();
	if (thread->id == id) {
		if (thread_is_idle_thread(thread))
			return B_NOT_ALLOWED;

		// It's ourself, so we know we aren't in the run queue, and we can
		// manipulate our structure directly
		oldPriority = thread->priority;
			// Note that this might not return the correct value if we are
			// preempted here, and another thread changes our priority before
			// the next line is executed.
		thread->priority = thread->next_priority = priority;
	} else {
		InterruptsSpinLocker _(gThreadSpinlock);

		thread = thread_get_thread_struct_locked(id);
		if (thread == NULL)
			return B_BAD_THREAD_ID;

		if (thread_is_idle_thread(thread))
			return B_NOT_ALLOWED;

		oldPriority = thread->priority;
		scheduler_set_thread_priority(thread, priority);
	}

	return oldPriority;
}


status_t
snooze_etc(bigtime_t timeout, int timebase, uint32 flags)
{
	status_t status;

	if (timebase != B_SYSTEM_TIMEBASE)
		return B_BAD_VALUE;

	InterruptsSpinLocker _(gThreadSpinlock);
	Thread* thread = thread_get_current_thread();

	thread_prepare_to_block(thread, flags, THREAD_BLOCK_TYPE_SNOOZE, NULL);
	status = thread_block_with_timeout_locked(flags, timeout);

	if (status == B_TIMED_OUT || status == B_WOULD_BLOCK)
		return B_OK;

	return status;
}


/*!	snooze() for internal kernel use only; doesn't interrupt on signals. */
status_t
snooze(bigtime_t timeout)
{
	return snooze_etc(timeout, B_SYSTEM_TIMEBASE, B_RELATIVE_TIMEOUT);
}


/*!	snooze_until() for internal kernel use only; doesn't interrupt on
	signals.
*/
status_t
snooze_until(bigtime_t timeout, int timebase)
{
	return snooze_etc(timeout, timebase, B_ABSOLUTE_TIMEOUT);
}


status_t
wait_for_thread(thread_id thread, status_t *_returnCode)
{
	return wait_for_thread_etc(thread, 0, 0, _returnCode);
}


status_t
suspend_thread(thread_id id)
{
	if (id <= 0)
		return B_BAD_VALUE;

	return send_signal(id, SIGSTOP);
}


status_t
resume_thread(thread_id id)
{
	if (id <= 0)
		return B_BAD_VALUE;

	return send_signal_etc(id, SIGCONT, SIGNAL_FLAG_DONT_RESTART_SYSCALL);
		// This retains compatibility to BeOS which documents the
		// combination of suspend_thread() and resume_thread() to
		// interrupt threads waiting on semaphores.
}


thread_id
spawn_kernel_thread(thread_func function, const char *name, int32 priority,
	void *arg)
{
	thread_creation_attributes attributes;
	attributes.entry = (thread_entry_func)function;
	attributes.name = name;
	attributes.priority = priority;
	attributes.args1 = arg;
	attributes.args2 = NULL;
	attributes.stack_address = NULL;
	attributes.stack_size = 0;
	attributes.team = team_get_kernel_team()->id;
	attributes.thread = -1;

	return create_thread(attributes, true);
}


int
getrlimit(int resource, struct rlimit * rlp)
{
	status_t error = common_getrlimit(resource, rlp);
	if (error != B_OK) {
		errno = error;
		return -1;
	}

	return 0;
}


int
setrlimit(int resource, const struct rlimit * rlp)
{
	status_t error = common_setrlimit(resource, rlp);
	if (error != B_OK) {
		errno = error;
		return -1;
	}

	return 0;
}


//	#pragma mark - syscalls


void
_user_exit_thread(status_t returnValue)
{
	exit_thread(returnValue);
}


status_t
_user_kill_thread(thread_id thread)
{
	return kill_thread(thread);
}


status_t
_user_resume_thread(thread_id thread)
{
	return resume_thread(thread);
}


status_t
_user_suspend_thread(thread_id thread)
{
	return suspend_thread(thread);
}


status_t
_user_rename_thread(thread_id thread, const char *userName)
{
	char name[B_OS_NAME_LENGTH];

	if (!IS_USER_ADDRESS(userName)
		|| userName == NULL
		|| user_strlcpy(name, userName, B_OS_NAME_LENGTH) < B_OK)
		return B_BAD_ADDRESS;

	return rename_thread(thread, name);
}


int32
_user_set_thread_priority(thread_id thread, int32 newPriority)
{
	return set_thread_priority(thread, newPriority);
}


thread_id
_user_spawn_thread(thread_creation_attributes* userAttributes)
{
	thread_creation_attributes attributes;
	if (userAttributes == NULL || !IS_USER_ADDRESS(userAttributes)
		|| user_memcpy(&attributes, userAttributes,
				sizeof(attributes)) != B_OK) {
		return B_BAD_ADDRESS;
	}

	if (attributes.stack_size != 0
		&& (attributes.stack_size < MIN_USER_STACK_SIZE
			|| attributes.stack_size > MAX_USER_STACK_SIZE)) {
		return B_BAD_VALUE;
	}

	char name[B_OS_NAME_LENGTH];
	thread_id threadID;

	if (!IS_USER_ADDRESS(attributes.entry) || attributes.entry == NULL
		|| (attributes.stack_address != NULL
			&& !IS_USER_ADDRESS(attributes.stack_address))
		|| (attributes.name != NULL && (!IS_USER_ADDRESS(attributes.name)
			|| user_strlcpy(name, attributes.name, B_OS_NAME_LENGTH) < 0)))
		return B_BAD_ADDRESS;

	attributes.name = attributes.name != NULL ? name : "user thread";
	attributes.team = thread_get_current_thread()->team->id;
	attributes.thread = -1;

	threadID = create_thread(attributes, false);

	if (threadID >= 0)
		user_debug_thread_created(threadID);

	return threadID;
}


status_t
_user_snooze_etc(bigtime_t timeout, int timebase, uint32 flags)
{
	// NOTE: We only know the system timebase at the moment.
	syscall_restart_handle_timeout_pre(flags, timeout);

	status_t error = snooze_etc(timeout, timebase, flags | B_CAN_INTERRUPT);

	return syscall_restart_handle_timeout_post(error, timeout);
}


void
_user_thread_yield(void)
{
	thread_yield(true);
}


status_t
_user_get_thread_info(thread_id id, thread_info *userInfo)
{
	thread_info info;
	status_t status;

	if (!IS_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;

	status = _get_thread_info(id, &info, sizeof(thread_info));

	if (status >= B_OK
		&& user_memcpy(userInfo, &info, sizeof(thread_info)) < B_OK)
		return B_BAD_ADDRESS;

	return status;
}


status_t
_user_get_next_thread_info(team_id team, int32 *userCookie,
	thread_info *userInfo)
{
	status_t status;
	thread_info info;
	int32 cookie;

	if (!IS_USER_ADDRESS(userCookie) || !IS_USER_ADDRESS(userInfo)
		|| user_memcpy(&cookie, userCookie, sizeof(int32)) < B_OK)
		return B_BAD_ADDRESS;

	status = _get_next_thread_info(team, &cookie, &info, sizeof(thread_info));
	if (status < B_OK)
		return status;

	if (user_memcpy(userCookie, &cookie, sizeof(int32)) < B_OK
		|| user_memcpy(userInfo, &info, sizeof(thread_info)) < B_OK)
		return B_BAD_ADDRESS;

	return status;
}


thread_id
_user_find_thread(const char *userName)
{
	char name[B_OS_NAME_LENGTH];

	if (userName == NULL)
		return find_thread(NULL);

	if (!IS_USER_ADDRESS(userName)
		|| user_strlcpy(name, userName, sizeof(name)) < B_OK)
		return B_BAD_ADDRESS;

	return find_thread(name);
}


status_t
_user_wait_for_thread(thread_id id, status_t *userReturnCode)
{
	status_t returnCode;
	status_t status;

	if (userReturnCode != NULL && !IS_USER_ADDRESS(userReturnCode))
		return B_BAD_ADDRESS;

	status = wait_for_thread_etc(id, B_CAN_INTERRUPT, 0, &returnCode);

	if (status == B_OK && userReturnCode != NULL
		&& user_memcpy(userReturnCode, &returnCode, sizeof(status_t)) < B_OK) {
		return B_BAD_ADDRESS;
	}

	return syscall_restart_handle_post(status);
}


bool
_user_has_data(thread_id thread)
{
	return has_data(thread);
}


status_t
_user_send_data(thread_id thread, int32 code, const void *buffer,
	size_t bufferSize)
{
	if (!IS_USER_ADDRESS(buffer))
		return B_BAD_ADDRESS;

	return send_data_etc(thread, code, buffer, bufferSize,
		B_KILL_CAN_INTERRUPT);
		// supports userland buffers
}


status_t
_user_receive_data(thread_id *_userSender, void *buffer, size_t bufferSize)
{
	thread_id sender;
	status_t code;

	if ((!IS_USER_ADDRESS(_userSender) && _userSender != NULL)
		|| !IS_USER_ADDRESS(buffer))
		return B_BAD_ADDRESS;

	code = receive_data_etc(&sender, buffer, bufferSize, B_KILL_CAN_INTERRUPT);
		// supports userland buffers

	if (_userSender != NULL)
		if (user_memcpy(_userSender, &sender, sizeof(thread_id)) < B_OK)
			return B_BAD_ADDRESS;

	return code;
}


status_t
_user_block_thread(uint32 flags, bigtime_t timeout)
{
	syscall_restart_handle_timeout_pre(flags, timeout);
	flags |= B_CAN_INTERRUPT;

	Thread* thread = thread_get_current_thread();

	InterruptsSpinLocker locker(gThreadSpinlock);

	// check, if already done
	if (thread->user_thread->wait_status <= 0)
		return thread->user_thread->wait_status;

	// nope, so wait
	thread_prepare_to_block(thread, flags, THREAD_BLOCK_TYPE_OTHER, "user");
	status_t status = thread_block_with_timeout_locked(flags, timeout);
	thread->user_thread->wait_status = status;

	return syscall_restart_handle_timeout_post(status, timeout);
}


status_t
_user_unblock_thread(thread_id threadID, status_t status)
{
	InterruptsSpinLocker locker(gThreadSpinlock);
	status_t error = user_unblock_thread(threadID, status);
	scheduler_reschedule_if_necessary_locked();
	return error;
}


status_t
_user_unblock_threads(thread_id* userThreads, uint32 count, status_t status)
{
	enum {
		MAX_USER_THREADS_TO_UNBLOCK	= 128
	};

	if (userThreads == NULL || !IS_USER_ADDRESS(userThreads))
		return B_BAD_ADDRESS;
	if (count > MAX_USER_THREADS_TO_UNBLOCK)
		return B_BAD_VALUE;

	thread_id threads[MAX_USER_THREADS_TO_UNBLOCK];
	if (user_memcpy(threads, userThreads, count * sizeof(thread_id)) != B_OK)
		return B_BAD_ADDRESS;

	InterruptsSpinLocker locker(gThreadSpinlock);
	for (uint32 i = 0; i < count; i++)
		user_unblock_thread(threads[i], status);

	scheduler_reschedule_if_necessary_locked();

	return B_OK;
}


// TODO: the following two functions don't belong here


int
_user_getrlimit(int resource, struct rlimit *urlp)
{
	struct rlimit rl;
	int ret;

	if (urlp == NULL)
		return EINVAL;

	if (!IS_USER_ADDRESS(urlp))
		return B_BAD_ADDRESS;

	ret = common_getrlimit(resource, &rl);

	if (ret == 0) {
		ret = user_memcpy(urlp, &rl, sizeof(struct rlimit));
		if (ret < 0)
			return ret;

		return 0;
	}

	return ret;
}


int
_user_setrlimit(int resource, const struct rlimit *userResourceLimit)
{
	struct rlimit resourceLimit;

	if (userResourceLimit == NULL)
		return EINVAL;

	if (!IS_USER_ADDRESS(userResourceLimit)
		|| user_memcpy(&resourceLimit, userResourceLimit,
			sizeof(struct rlimit)) < B_OK)
		return B_BAD_ADDRESS;

	return common_setrlimit(resource, &resourceLimit);
}
