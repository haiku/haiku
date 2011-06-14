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

#include <algorithm>

#include <OS.h>

#include <util/AutoLock.h>

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

#include "TeamThreadTables.h"


//#define TRACE_THREAD
#ifdef TRACE_THREAD
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#define THREAD_MAX_MESSAGE_SIZE		65536


// #pragma mark - ThreadHashTable


typedef BKernel::TeamThreadTable<Thread> ThreadHashTable;


// thread list
static Thread sIdleThreads[B_MAX_CPU_COUNT];
static ThreadHashTable sThreadHash;
static spinlock sThreadHashLock = B_SPINLOCK_INITIALIZER;
static thread_id sNextThreadID = 2;
	// ID 1 is allocated for the kernel by Team::Team() behind our back

// some arbitrarily chosen limits -- should probably depend on the available
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


struct ThreadEntryArguments {
	status_t	(*kernelFunction)(void* argument);
	void*		argument;
	bool		enterUserland;
};

struct UserThreadEntryArguments : ThreadEntryArguments {
	addr_t			userlandEntry;
	void*			userlandArgument1;
	void*			userlandArgument2;
	pthread_t		pthread;
	arch_fork_arg*	forkArgs;
	uint32			flags;
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


// #pragma mark - Thread


/*!	Constructs a thread.

	\param name The thread's name.
	\param threadID The ID to be assigned to the new thread. If
		  \code < 0 \endcode a fresh one is allocated.
	\param cpu The CPU the thread shall be assigned.
*/
Thread::Thread(const char* name, thread_id threadID, struct cpu_ent* cpu)
	:
	flags(0),
	serial_number(-1),
	hash_next(NULL),
	team_next(NULL),
	queue_next(NULL),
	priority(-1),
	next_priority(-1),
	io_priority(-1),
	cpu(cpu),
	previous_cpu(NULL),
	pinned_to_cpu(0),
	sig_block_mask(0),
	sigsuspend_original_unblocked_mask(0),
	user_signal_context(NULL),
	signal_stack_base(0),
	signal_stack_size(0),
	signal_stack_enabled(false),
	in_kernel(true),
	was_yielded(false),
	user_thread(NULL),
	fault_handler(0),
	page_faults_allowed(1),
	team(NULL),
	select_infos(NULL),
	kernel_stack_area(-1),
	kernel_stack_base(0),
	user_stack_area(-1),
	user_stack_base(0),
	user_local_storage(0),
	kernel_errno(0),
	user_time(0),
	kernel_time(0),
	last_time(0),
	cpu_clock_offset(0),
	post_interrupt_callback(NULL),
	post_interrupt_data(NULL)
{
	id = threadID >= 0 ? threadID : allocate_thread_id();
	visible = false;

	// init locks
	char lockName[32];
	snprintf(lockName, sizeof(lockName), "Thread:%" B_PRId32, id);
	mutex_init_etc(&fLock, lockName, MUTEX_FLAG_CLONE_NAME);

	B_INITIALIZE_SPINLOCK(&time_lock);

	// init name
	if (name != NULL)
		strlcpy(this->name, name, B_OS_NAME_LENGTH);
	else
		strcpy(this->name, "unnamed thread");

	alarm.period = 0;

	exit.status = 0;

	list_init(&exit.waiters);

	exit.sem = -1;
	msg.write_sem = -1;
	msg.read_sem = -1;

	// add to thread table -- yet invisible
	InterruptsSpinLocker threadHashLocker(sThreadHashLock);
	sThreadHash.Insert(this);
}


Thread::~Thread()
{
	// Delete resources that should actually be deleted by the thread itself,
	// when it exited, but that might still exist, if the thread was never run.

	if (user_stack_area >= 0)
		delete_area(user_stack_area);

	DeleteUserTimers(false);

	// delete the resources, that may remain in either case

	if (kernel_stack_area >= 0)
		delete_area(kernel_stack_area);

	fPendingSignals.Clear();

	if (exit.sem >= 0)
		delete_sem(exit.sem);
	if (msg.write_sem >= 0)
		delete_sem(msg.write_sem);
	if (msg.read_sem >= 0)
		delete_sem(msg.read_sem);

	scheduler_on_thread_destroy(this);

	mutex_destroy(&fLock);

	// remove from thread table
	InterruptsSpinLocker threadHashLocker(sThreadHashLock);
	sThreadHash.Remove(this);
}


/*static*/ status_t
Thread::Create(const char* name, Thread*& _thread)
{
	Thread* thread = new Thread(name, -1, NULL);
	if (thread == NULL)
		return B_NO_MEMORY;

	status_t error = thread->Init(false);
	if (error != B_OK) {
		delete thread;
		return error;
	}

	_thread = thread;
	return B_OK;
}


/*static*/ Thread*
Thread::Get(thread_id id)
{
	InterruptsSpinLocker threadHashLocker(sThreadHashLock);
	Thread* thread = sThreadHash.Lookup(id);
	if (thread != NULL)
		thread->AcquireReference();
	return thread;
}


/*static*/ Thread*
Thread::GetAndLock(thread_id id)
{
	// look it up and acquire a reference
	InterruptsSpinLocker threadHashLocker(sThreadHashLock);
	Thread* thread = sThreadHash.Lookup(id);
	if (thread == NULL)
		return NULL;

	thread->AcquireReference();
	threadHashLocker.Unlock();

	// lock and check, if it is still in the hash table
	thread->Lock();
	threadHashLocker.Lock();

	if (sThreadHash.Lookup(id) == thread)
		return thread;

	threadHashLocker.Unlock();

	// nope, the thread is no longer in the hash table
	thread->UnlockAndReleaseReference();

	return NULL;
}


/*static*/ Thread*
Thread::GetDebug(thread_id id)
{
	return sThreadHash.Lookup(id, false);
}


/*static*/ bool
Thread::IsAlive(thread_id id)
{
	InterruptsSpinLocker threadHashLocker(sThreadHashLock);
	return sThreadHash.Lookup(id) != NULL;
}


void*
Thread::operator new(size_t size)
{
	return object_cache_alloc(sThreadCache, 0);
}


void*
Thread::operator new(size_t, void* pointer)
{
	return pointer;
}


void
Thread::operator delete(void* pointer, size_t size)
{
	object_cache_free(sThreadCache, pointer, 0);
}


status_t
Thread::Init(bool idleThread)
{
	status_t error = scheduler_on_thread_create(this, idleThread);
	if (error != B_OK)
		return error;

	char temp[64];
	sprintf(temp, "thread_%ld_retcode_sem", id);
	exit.sem = create_sem(0, temp);
	if (exit.sem < 0)
		return exit.sem;

	sprintf(temp, "%s send", name);
	msg.write_sem = create_sem(1, temp);
	if (msg.write_sem < 0)
		return msg.write_sem;

	sprintf(temp, "%s receive", name);
	msg.read_sem = create_sem(0, temp);
	if (msg.read_sem < 0)
		return msg.read_sem;

	error = arch_thread_init_thread_struct(this);
	if (error != B_OK)
		return error;

	return B_OK;
}


/*!	Checks whether the thread is still in the thread hash table.
*/
bool
Thread::IsAlive() const
{
	InterruptsSpinLocker threadHashLocker(sThreadHashLock);

	return sThreadHash.Lookup(id) != NULL;
}


void
Thread::ResetSignalsOnExec()
{
	// We are supposed keep the pending signals and the signal mask. Only the
	// signal stack, if set, shall be unset.

	sigsuspend_original_unblocked_mask = 0;
	user_signal_context = NULL;
	signal_stack_base = 0;
	signal_stack_size = 0;
	signal_stack_enabled = false;
}


/*!	Adds the given user timer to the thread and, if user-defined, assigns it an
	ID.

	The caller must hold the thread's lock.

	\param timer The timer to be added. If it doesn't have an ID yet, it is
		considered user-defined and will be assigned an ID.
	\return \c B_OK, if the timer was added successfully, another error code
		otherwise.
*/
status_t
Thread::AddUserTimer(UserTimer* timer)
{
	// If the timer is user-defined, check timer limit and increment
	// user-defined count.
	if (timer->ID() < 0 && !team->CheckAddUserDefinedTimer())
		return EAGAIN;

	fUserTimers.AddTimer(timer);

	return B_OK;
}


/*!	Removes the given user timer from the thread.

	The caller must hold the thread's lock.

	\param timer The timer to be removed.

*/
void
Thread::RemoveUserTimer(UserTimer* timer)
{
	fUserTimers.RemoveTimer(timer);

	if (timer->ID() >= USER_TIMER_FIRST_USER_DEFINED_ID)
		team->UserDefinedTimersRemoved(1);
}


/*!	Deletes all (or all user-defined) user timers of the thread.

	The caller must hold the thread's lock.

	\param userDefinedOnly If \c true, only the user-defined timers are deleted,
		otherwise all timers are deleted.
*/
void
Thread::DeleteUserTimers(bool userDefinedOnly)
{
	int32 count = fUserTimers.DeleteTimers(userDefinedOnly);
	team->UserDefinedTimersRemoved(count);
}


void
Thread::DeactivateCPUTimeUserTimers()
{
	while (ThreadTimeUserTimer* timer = fCPUTimeUserTimers.Head())
		timer->Deactivate();
}


// #pragma mark - ThreadListIterator


ThreadListIterator::ThreadListIterator()
{
	// queue the entry
	InterruptsSpinLocker locker(sThreadHashLock);
	sThreadHash.InsertIteratorEntry(&fEntry);
}


ThreadListIterator::~ThreadListIterator()
{
	// remove the entry
	InterruptsSpinLocker locker(sThreadHashLock);
	sThreadHash.RemoveIteratorEntry(&fEntry);
}


Thread*
ThreadListIterator::Next()
{
	// get the next team -- if there is one, get reference for it
	InterruptsSpinLocker locker(sThreadHashLock);
	Thread* thread = sThreadHash.NextElement(&fEntry);
	if (thread != NULL)
		thread->AcquireReference();

	return thread;
}


// #pragma mark - ThreadCreationAttributes


ThreadCreationAttributes::ThreadCreationAttributes(thread_func function,
	const char* name, int32 priority, void* arg, team_id team,
	Thread* thread)
{
	this->entry = NULL;
	this->name = name;
	this->priority = priority;
	this->args1 = NULL;
	this->args2 = NULL;
	this->stack_address = NULL;
	this->stack_size = 0;
	this->pthread = NULL;
	this->flags = 0;
	this->team = team >= 0 ? team : team_get_kernel_team()->id;
	this->thread = thread;
	this->signal_mask = 0;
	this->additional_stack_size = 0;
	this->kernelEntry = function;
	this->kernelArgument = arg;
	this->forkArgs = NULL;
}


/*!	Initializes the structure from a userland structure.
	\param userAttributes The userland structure (must be a userland address).
	\param nameBuffer A character array of at least size B_OS_NAME_LENGTH,
		which will be used for the \c name field, if the userland structure has
		a name. The buffer must remain valid as long as this structure is in
		use afterwards (or until it is reinitialized).
	\return \c B_OK, if the initialization went fine, another error code
		otherwise.
*/
status_t
ThreadCreationAttributes::InitFromUserAttributes(
	const thread_creation_attributes* userAttributes, char* nameBuffer)
{
	if (userAttributes == NULL || !IS_USER_ADDRESS(userAttributes)
		|| user_memcpy((thread_creation_attributes*)this, userAttributes,
				sizeof(thread_creation_attributes)) != B_OK) {
		return B_BAD_ADDRESS;
	}

	if (stack_size != 0
		&& (stack_size < MIN_USER_STACK_SIZE
			|| stack_size > MAX_USER_STACK_SIZE)) {
		return B_BAD_VALUE;
	}

	if (entry == NULL || !IS_USER_ADDRESS(entry)
		|| (stack_address != NULL && !IS_USER_ADDRESS(stack_address))
		|| (name != NULL && (!IS_USER_ADDRESS(name)
			|| user_strlcpy(nameBuffer, name, B_OS_NAME_LENGTH) < 0))) {
		return B_BAD_ADDRESS;
	}

	name = name != NULL ? nameBuffer : "user thread";

	// kernel only attributes (not in thread_creation_attributes):
	Thread* currentThread = thread_get_current_thread();
	team = currentThread->team->id;
	thread = NULL;
	signal_mask = currentThread->sig_block_mask;
		// inherit the current thread's signal mask
	additional_stack_size = 0;
	kernelEntry = NULL;
	kernelArgument = NULL;
	forkArgs = NULL;

	return B_OK;
}


// #pragma mark - private functions


/*!	Inserts a thread into a team.
	The caller must hold the team's lock, the thread's lock, and the scheduler
	lock.
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
	The caller must hold the team's lock, the thread's lock, and the scheduler
	lock.
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


static status_t
enter_userspace(Thread* thread, UserThreadEntryArguments* args)
{
	status_t error = arch_thread_init_tls(thread);
	if (error != B_OK) {
		dprintf("Failed to init TLS for new userland thread \"%s\" (%" B_PRId32
			")\n", thread->name, thread->id);
		free(args->forkArgs);
		return error;
	}

	user_debug_update_new_thread_flags(thread);

	// init the thread's user_thread
	user_thread* userThread = thread->user_thread;
	userThread->pthread = args->pthread;
	userThread->flags = 0;
	userThread->wait_status = B_OK;
	userThread->defer_signals
		= (args->flags & THREAD_CREATION_FLAG_DEFER_SIGNALS) != 0 ? 1 : 0;
	userThread->pending_signals = 0;

	if (args->forkArgs != NULL) {
		// This is a fork()ed thread. Copy the fork args onto the stack and
		// free them.
		arch_fork_arg archArgs = *args->forkArgs;
		free(args->forkArgs);

		arch_restore_fork_frame(&archArgs);
			// this one won't return here
		return B_ERROR;
	}

	// Jump to the entry point in user space. Only returns, if something fails.
	return arch_thread_enter_userspace(thread, args->userlandEntry,
		args->userlandArgument1, args->userlandArgument2);
}


status_t
thread_enter_userspace_new_team(Thread* thread, addr_t entryFunction,
	void* argument1, void* argument2)
{
	UserThreadEntryArguments entryArgs;
	entryArgs.kernelFunction = NULL;
	entryArgs.argument = NULL;
	entryArgs.enterUserland = true;
	entryArgs.userlandEntry = (addr_t)entryFunction;
	entryArgs.userlandArgument1 = argument1;
	entryArgs.userlandArgument2 = argument2;
	entryArgs.pthread = NULL;
	entryArgs.forkArgs = NULL;
	entryArgs.flags = 0;

	return enter_userspace(thread, &entryArgs);
}


static void
common_thread_entry(void* _args)
{
	Thread* thread = thread_get_current_thread();

	// The thread is new and has been scheduled the first time.

	// start CPU time based user timers
	if (thread->HasActiveCPUTimeUserTimers()
		|| thread->team->HasActiveCPUTimeUserTimers()) {
		user_timer_continue_cpu_timers(thread, thread->cpu->previous_thread);
	}

	// notify the user debugger code
	if ((thread->flags & THREAD_FLAGS_DEBUGGER_INSTALLED) != 0)
		user_debug_thread_scheduled(thread);

	// start tracking time
	thread->last_time = system_time();

	// unlock the scheduler lock and enable interrupts
	release_spinlock(&gSchedulerLock);
	enable_interrupts();

	// call the kernel function, if any
	ThreadEntryArguments* args = (ThreadEntryArguments*)_args;
	if (args->kernelFunction != NULL)
		args->kernelFunction(args->argument);

	// If requested, enter userland, now.
	if (args->enterUserland) {
		enter_userspace(thread, (UserThreadEntryArguments*)args);
			// only returns or error

		// If that's the team's main thread, init the team exit info.
		if (thread == thread->team->main_thread)
			team_init_exit_info_on_error(thread->team);
	}

	// we're done
	thread_exit();
}


/*!	Prepares the given thread's kernel stack for executing its entry function.

	The data pointed to by \a data of size \a dataSize are copied to the
	thread's kernel stack. A pointer to the copy's data is passed to the entry
	function. The entry function is common_thread_entry().

	\param thread The thread.
	\param data Pointer to data to be copied to the thread's stack and passed
		to the entry function.
	\param dataSize The size of \a data.
 */
static void
init_thread_kernel_stack(Thread* thread, const void* data, size_t dataSize)
{
	uint8* stack = (uint8*)thread->kernel_stack_base;
	uint8* stackTop = (uint8*)thread->kernel_stack_top;

	// clear (or rather invalidate) the kernel stack contents, if compiled with
	// debugging
#if KDEBUG > 0
#	if defined(DEBUG_KERNEL_STACKS) && defined(STACK_GROWS_DOWNWARDS)
	memset((void*)(stack + KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE), 0xcc,
		KERNEL_STACK_SIZE);
#	else
	memset(stack, 0xcc, KERNEL_STACK_SIZE);
#	endif
#endif

	// copy the data onto the stack, with 16-byte alignment to be on the safe
	// side
	void* clonedData;
#ifdef STACK_GROWS_DOWNWARDS
	clonedData = (void*)ROUNDDOWN((addr_t)stackTop - dataSize, 16);
	stackTop = (uint8*)clonedData;
#else
	clonedData = (void*)ROUNDUP((addr_t)stack, 16);
	stack = (uint8*)clonedData + ROUNDUP(dataSize, 16);
#endif

	memcpy(clonedData, data, dataSize);

	arch_thread_init_kthread_stack(thread, stack, stackTop,
		&common_thread_entry, clonedData);
}


static status_t
create_thread_user_stack(Team* team, Thread* thread, void* _stackBase,
	size_t stackSize, size_t additionalSize, char* nameBuffer)
{
	area_id stackArea = -1;
	uint8* stackBase = (uint8*)_stackBase;

	if (stackBase != NULL) {
		// A stack has been specified. It must be large enough to hold the
		// TLS space at least.
		STATIC_ASSERT(TLS_SIZE < MIN_USER_STACK_SIZE);
		if (stackSize < MIN_USER_STACK_SIZE)
			return B_BAD_VALUE;

		stackBase -= TLS_SIZE;
	}

	if (stackBase == NULL) {
		// No user-defined stack -- allocate one. For non-main threads the stack
		// will be between USER_STACK_REGION and the main thread stack area. For
		// a main thread the position is fixed.

		if (stackSize == 0) {
			// Use the default size (a different one for a main thread).
			stackSize = thread->id == team->id
				? USER_MAIN_THREAD_STACK_SIZE : USER_STACK_SIZE;
		} else {
			// Verify that the given stack size is large enough.
			if (stackSize < MIN_USER_STACK_SIZE - TLS_SIZE)
				return B_BAD_VALUE;

			stackSize = PAGE_ALIGN(stackSize);
		}
		stackSize += USER_STACK_GUARD_PAGES * B_PAGE_SIZE;

		size_t areaSize = PAGE_ALIGN(stackSize + TLS_SIZE + additionalSize);

		snprintf(nameBuffer, B_OS_NAME_LENGTH, "%s_%ld_stack", thread->name,
			thread->id);

		virtual_address_restrictions virtualRestrictions = {};
		if (thread->id == team->id) {
			// The main thread gets a fixed position at the top of the stack
			// address range.
			stackBase = (uint8*)(USER_STACK_REGION + USER_STACK_REGION_SIZE
				- areaSize);
			virtualRestrictions.address_specification = B_EXACT_ADDRESS;

		} else {
			// not a main thread
			stackBase = (uint8*)(addr_t)USER_STACK_REGION;
			virtualRestrictions.address_specification = B_BASE_ADDRESS;
		}
		virtualRestrictions.address = (void*)stackBase;

		physical_address_restrictions physicalRestrictions = {};

		stackArea = create_area_etc(team->id, nameBuffer,
			areaSize, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA | B_STACK_AREA,
			0, &virtualRestrictions, &physicalRestrictions,
			(void**)&stackBase);
		if (stackArea < 0)
			return stackArea;
	}

	// set the stack
	ThreadLocker threadLocker(thread);
	thread->user_stack_base = (addr_t)stackBase;
	thread->user_stack_size = stackSize;
	thread->user_stack_area = stackArea;

	return B_OK;
}


status_t
thread_create_user_stack(Team* team, Thread* thread, void* stackBase,
	size_t stackSize, size_t additionalSize)
{
	char nameBuffer[B_OS_NAME_LENGTH];
	return create_thread_user_stack(team, thread, stackBase, stackSize,
		additionalSize, nameBuffer);
}


/*!	Creates a new thread.

	\param attributes The thread creation attributes, specifying the team in
		which to create the thread, as well as a whole bunch of other arguments.
	\param kernel \c true, if a kernel-only thread shall be created, \c false,
		if the thread shall also be able to run in userland.
	\return The ID of the newly created thread (>= 0) or an error code on
		failure.
*/
thread_id
thread_create_thread(const ThreadCreationAttributes& attributes, bool kernel)
{
	status_t status = B_OK;

	TRACE(("thread_create_thread(%s, thread = %p, %s)\n", attributes.name,
		attributes.thread, kernel ? "kernel" : "user"));

	// get the team
	Team* team = Team::Get(attributes.team);
	if (team == NULL)
		return B_BAD_TEAM_ID;
	BReference<Team> teamReference(team, true);

	// If a thread object is given, acquire a reference to it, otherwise create
	// a new thread object with the given attributes.
	Thread* thread = attributes.thread;
	if (thread != NULL) {
		thread->AcquireReference();
	} else {
		status = Thread::Create(attributes.name, thread);
		if (status != B_OK)
			return status;
	}
	BReference<Thread> threadReference(thread, true);

	thread->team = team;
		// set already, so, if something goes wrong, the team pointer is
		// available for deinitialization
	thread->priority = attributes.priority == -1
		? B_NORMAL_PRIORITY : attributes.priority;
	thread->next_priority = thread->priority;
	thread->state = B_THREAD_SUSPENDED;
	thread->next_state = B_THREAD_SUSPENDED;

	thread->sig_block_mask = attributes.signal_mask;

	// init debug structure
	init_thread_debug_info(&thread->debug_info);

	// create the kernel stack
	char stackName[B_OS_NAME_LENGTH];
	snprintf(stackName, B_OS_NAME_LENGTH, "%s_%ld_kstack", thread->name,
		thread->id);
	thread->kernel_stack_area = create_area(stackName,
		(void **)&thread->kernel_stack_base, B_ANY_KERNEL_ADDRESS,
		KERNEL_STACK_SIZE + KERNEL_STACK_GUARD_PAGES  * B_PAGE_SIZE,
		B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_KERNEL_STACK_AREA);

	if (thread->kernel_stack_area < 0) {
		// we're not yet part of a team, so we can just bail out
		status = thread->kernel_stack_area;

		dprintf("create_thread: error creating kernel stack: %s!\n",
			strerror(status));

		return status;
	}

	thread->kernel_stack_top = thread->kernel_stack_base + KERNEL_STACK_SIZE
		+ KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE;

	if (kernel) {
		// Init the thread's kernel stack. It will start executing
		// common_thread_entry() with the arguments we prepare here.
		ThreadEntryArguments entryArgs;
		entryArgs.kernelFunction = attributes.kernelEntry;
		entryArgs.argument = attributes.kernelArgument;
		entryArgs.enterUserland = false;

		init_thread_kernel_stack(thread, &entryArgs, sizeof(entryArgs));
	} else {
		// create the userland stack, if the thread doesn't have one yet
		if (thread->user_stack_base == 0) {
			status = create_thread_user_stack(team, thread,
				attributes.stack_address, attributes.stack_size,
				attributes.additional_stack_size, stackName);
			if (status != B_OK)
				return status;
		}

		// Init the thread's kernel stack. It will start executing
		// common_thread_entry() with the arguments we prepare here.
		UserThreadEntryArguments entryArgs;
		entryArgs.kernelFunction = attributes.kernelEntry;
		entryArgs.argument = attributes.kernelArgument;
		entryArgs.enterUserland = true;
		entryArgs.userlandEntry = (addr_t)attributes.entry;
		entryArgs.userlandArgument1 = attributes.args1;
		entryArgs.userlandArgument2 = attributes.args2;
		entryArgs.pthread = attributes.pthread;
		entryArgs.forkArgs = attributes.forkArgs;
		entryArgs.flags = attributes.flags;

		init_thread_kernel_stack(thread, &entryArgs, sizeof(entryArgs));

		// create the pre-defined thread timers
		status = user_timer_create_thread_timers(team, thread);
		if (status != B_OK)
			return status;
	}

	// lock the team and see, if it is still alive
	TeamLocker teamLocker(team);
	if (team->state >= TEAM_STATE_SHUTDOWN)
		return B_BAD_TEAM_ID;

	bool debugNewThread = false;
	if (!kernel) {
		// allocate the user_thread structure, if not already allocated
		if (thread->user_thread == NULL) {
			thread->user_thread = team_allocate_user_thread(team);
			if (thread->user_thread == NULL)
				return B_NO_MEMORY;
		}

		// If the new thread belongs to the same team as the current thread, it
		// may inherit some of the thread debug flags.
		Thread* currentThread = thread_get_current_thread();
		if (currentThread != NULL && currentThread->team == team) {
			// inherit all user flags...
			int32 debugFlags = atomic_get(&currentThread->debug_info.flags)
				& B_THREAD_DEBUG_USER_FLAG_MASK;

			// ... save the syscall tracing flags, unless explicitely specified
			if (!(debugFlags & B_THREAD_DEBUG_SYSCALL_TRACE_CHILD_THREADS)) {
				debugFlags &= ~(B_THREAD_DEBUG_PRE_SYSCALL
					| B_THREAD_DEBUG_POST_SYSCALL);
			}

			thread->debug_info.flags = debugFlags;

			// stop the new thread, if desired
			debugNewThread = debugFlags & B_THREAD_DEBUG_STOP_CHILD_THREADS;
		}
	}

	// We're going to make the thread live, now. The thread itself will take
	// over a reference to its Thread object. We acquire another reference for
	// our own use (and threadReference remains armed).
	thread->AcquireReference();

	ThreadLocker threadLocker(thread);
	InterruptsSpinLocker schedulerLocker(gSchedulerLock);
	SpinLocker threadHashLocker(sThreadHashLock);

	// make thread visible in global hash/list
	thread->visible = true;
	sUsedThreads++;
	scheduler_on_thread_init(thread);

	// Debug the new thread, if the parent thread required that (see above),
	// or the respective global team debug flag is set. But only, if a
	// debugger is installed for the team.
	if (!kernel) {
		int32 teamDebugFlags = atomic_get(&team->debug_info.flags);
		debugNewThread |= (teamDebugFlags & B_TEAM_DEBUG_STOP_NEW_THREADS) != 0;
		if (debugNewThread
			&& (teamDebugFlags & B_TEAM_DEBUG_DEBUGGER_INSTALLED) != 0) {
			thread->debug_info.flags |= B_THREAD_DEBUG_STOP;
		}
	}

	// insert thread into team
	insert_thread_into_team(team, thread);

	threadHashLocker.Unlock();
	schedulerLocker.Unlock();
	threadLocker.Unlock();
	teamLocker.Unlock();

	// notify listeners
	sNotificationService.Notify(THREAD_ADDED, thread);

	return thread->id;
}


static status_t
undertaker(void* /*args*/)
{
	while (true) {
		// wait for a thread to bury
		InterruptsSpinLocker schedulerLocker(gSchedulerLock);

		while (sUndertakerEntries.IsEmpty()) {
			ConditionVariableEntry conditionEntry;
			sUndertakerCondition.Add(&conditionEntry);
			schedulerLocker.Unlock();

			conditionEntry.Wait();

			schedulerLocker.Lock();
		}

		UndertakerEntry* _entry = sUndertakerEntries.RemoveHead();
		schedulerLocker.Unlock();

		UndertakerEntry entry = *_entry;
			// we need a copy, since the original entry is on the thread's stack

		// we've got an entry
		Thread* thread = entry.thread;

		// remove this thread from from the kernel team -- this makes it
		// unaccessible
		Team* kernelTeam = team_get_kernel_team();
		TeamLocker kernelTeamLocker(kernelTeam);
		thread->Lock();
		schedulerLocker.Lock();

		remove_thread_from_team(kernelTeam, thread);

		schedulerLocker.Unlock();
		kernelTeamLocker.Unlock();

		// free the thread structure
		thread->UnlockAndReleaseReference();
	}

	// can never get here
	return B_OK;
}


/*!	Returns the semaphore the thread is currently waiting on.

	The return value is purely informative.
	The caller must hold the scheduler lock.

	\param thread The thread.
	\return The ID of the semaphore the thread is currently waiting on or \c -1,
		if it isn't waiting on a semaphore.
*/
static sem_id
get_thread_wait_sem(Thread* thread)
{
	if (thread->state == B_THREAD_WAITING
		&& thread->wait.type == THREAD_BLOCK_TYPE_SEMAPHORE) {
		return (sem_id)(addr_t)thread->wait.object;
	}
	return -1;
}


/*!	Fills the thread_info structure with information from the specified thread.
	The caller must hold the thread's lock and the scheduler lock.
*/
static void
fill_thread_info(Thread *thread, thread_info *info, size_t size)
{
	info->thread = thread->id;
	info->team = thread->team->id;

	strlcpy(info->name, thread->name, B_OS_NAME_LENGTH);

	info->sem = -1;

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
				else
					info->sem = sem;
				break;
			}

			case THREAD_BLOCK_TYPE_CONDITION_VARIABLE:
			default:
				break;
		}
	} else
		info->state = (thread_state)thread->state;

	info->priority = thread->priority;
	info->stack_base = (void *)thread->user_stack_base;
	info->stack_end = (void *)(thread->user_stack_base
		+ thread->user_stack_size);

	InterruptsSpinLocker threadTimeLocker(thread->time_lock);
	info->user_time = thread->user_time;
	info->kernel_time = thread->kernel_time;
}


static status_t
send_data_etc(thread_id id, int32 code, const void *buffer, size_t bufferSize,
	int32 flags)
{
	// get the thread
	Thread *target = Thread::Get(id);
	if (target == NULL)
		return B_BAD_THREAD_ID;
	BReference<Thread> targetReference(target, true);

	// get the write semaphore
	ThreadLocker targetLocker(target);
	sem_id cachedSem = target->msg.write_sem;
	targetLocker.Unlock();

	if (bufferSize > THREAD_MAX_MESSAGE_SIZE)
		return B_NO_MEMORY;

	status_t status = acquire_sem_etc(cachedSem, 1, flags, 0);
	if (status == B_INTERRUPTED) {
		// we got interrupted by a signal
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

	targetLocker.Lock();

	// The target thread could have been deleted at this point.
	if (!target->IsAlive()) {
		targetLocker.Unlock();
		free(data);
		return B_BAD_THREAD_ID;
	}

	// Save message informations
	target->msg.sender = thread_get_current_thread()->id;
	target->msg.code = code;
	target->msg.size = bufferSize;
	target->msg.buffer = data;
	cachedSem = target->msg.read_sem;

	targetLocker.Unlock();

	release_sem(cachedSem);
	return B_OK;
}


static int32
receive_data_etc(thread_id *_sender, void *buffer, size_t bufferSize,
	int32 flags)
{
	Thread *thread = thread_get_current_thread();
	size_t size;
	int32 code;

	status_t status = acquire_sem_etc(thread->msg.read_sem, 1, flags, 0);
	if (status != B_OK) {
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


static status_t
common_snooze_etc(bigtime_t timeout, clockid_t clockID, uint32 flags,
	bigtime_t* _remainingTime)
{
	switch (clockID) {
		case CLOCK_REALTIME:
			// make sure the B_TIMEOUT_REAL_TIME_BASE flag is set and fall
			// through
			flags |= B_TIMEOUT_REAL_TIME_BASE;
		case CLOCK_MONOTONIC:
		{
			// Store the start time, for the case that we get interrupted and
			// need to return the remaining time. For absolute timeouts we can
			// still get he time later, if needed.
			bigtime_t startTime
				= _remainingTime != NULL && (flags & B_RELATIVE_TIMEOUT) != 0
					? system_time() : 0;

			Thread* thread = thread_get_current_thread();

			InterruptsSpinLocker schedulerLocker(gSchedulerLock);

			thread_prepare_to_block(thread, flags, THREAD_BLOCK_TYPE_SNOOZE,
				NULL);
			status_t status = thread_block_with_timeout_locked(flags, timeout);

			if (status == B_TIMED_OUT || status == B_WOULD_BLOCK)
				return B_OK;

			// If interrupted, compute the remaining time, if requested.
			if (status == B_INTERRUPTED && _remainingTime != NULL) {
				if ((flags & B_RELATIVE_TIMEOUT) != 0) {
					*_remainingTime = std::max(
						startTime + timeout - system_time(), (bigtime_t)0);
				} else {
					bigtime_t now = (flags & B_TIMEOUT_REAL_TIME_BASE) != 0
						? real_time_clock_usecs() : system_time();
					*_remainingTime = std::max(timeout - now, (bigtime_t)0);
				}
			}

			return status;
		}

		case CLOCK_THREAD_CPUTIME_ID:
			// Waiting for ourselves to do something isn't particularly
			// productive.
			return B_BAD_VALUE;

		case CLOCK_PROCESS_CPUTIME_ID:
		default:
			// We don't have to support those, but we are allowed to. Could be
			// done be creating a UserTimer on the fly with a custom UserEvent
			// that would just wake us up.
			return ENOTSUP;
	}
}


//	#pragma mark - debugger calls


static int
make_thread_unreal(int argc, char **argv)
{
	int32 id = -1;

	if (argc > 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	if (argc > 1)
		id = strtoul(argv[1], NULL, 0);

	for (ThreadHashTable::Iterator it = sThreadHash.GetIterator();
			Thread* thread = it.Next();) {
		if (id != -1 && thread->id != id)
			continue;

		if (thread->priority > B_DISPLAY_PRIORITY) {
			thread->priority = thread->next_priority = B_NORMAL_PRIORITY;
			kprintf("thread %ld made unreal\n", thread->id);
		}
	}

	return 0;
}


static int
set_thread_prio(int argc, char **argv)
{
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

	bool found = false;
	for (ThreadHashTable::Iterator it = sThreadHash.GetIterator();
			Thread* thread = it.Next();) {
		if (thread->id != id)
			continue;
		thread->priority = thread->next_priority = prio;
		kprintf("thread %ld set to priority %ld\n", id, prio);
		found = true;
		break;
	}
	if (!found)
		kprintf("thread %ld (%#lx) not found\n", id, id);

	return 0;
}


static int
make_thread_suspended(int argc, char **argv)
{
	int32 id;

	if (argc > 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	if (argc == 1)
		id = thread_get_current_thread()->id;
	else
		id = strtoul(argv[1], NULL, 0);

	bool found = false;
	for (ThreadHashTable::Iterator it = sThreadHash.GetIterator();
			Thread* thread = it.Next();) {
		if (thread->id != id)
			continue;

		thread->next_state = B_THREAD_SUSPENDED;
		kprintf("thread %ld suspended\n", id);
		found = true;
		break;
	}
	if (!found)
		kprintf("thread %ld (%#lx) not found\n", id, id);

	return 0;
}


static int
make_thread_resumed(int argc, char **argv)
{
	int32 id;

	if (argc != 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	// force user to enter a thread id, as using
	// the current thread is usually not intended
	id = strtoul(argv[1], NULL, 0);

	bool found = false;
	for (ThreadHashTable::Iterator it = sThreadHash.GetIterator();
			Thread* thread = it.Next();) {
		if (thread->id != id)
			continue;

		if (thread->state == B_THREAD_SUSPENDED) {
			scheduler_enqueue_in_run_queue(thread);
			kprintf("thread %ld resumed\n", thread->id);
		}
		found = true;
		break;
	}
	if (!found)
		kprintf("thread %ld (%#lx) not found\n", id, id);

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
		// TODO: This is a non-trivial syscall doing some locking, so this is
		// really nasty and may go seriously wrong.
	if (err)
		kprintf("drop failed\n");
	else
		kprintf("thread %ld dropped into user debugger\n", id);

	return 0;
}


/*!	Returns a user-readable string for a thread state.
	Only for use in the kernel debugger.
*/
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

	struct thread_death_entry *death = NULL;

	kprintf("THREAD: %p\n", thread);
	kprintf("id:                 %ld (%#lx)\n", thread->id, thread->id);
	kprintf("serial_number:      %" B_PRId64 "\n", thread->serial_number);
	kprintf("name:               \"%s\"\n", thread->name);
	kprintf("hash_next:          %p\nteam_next:          %p\nq_next:             %p\n",
		thread->hash_next, thread->team_next, thread->queue_next);
	kprintf("priority:           %ld (next %ld, I/O: %ld)\n", thread->priority,
		thread->next_priority, thread->io_priority);
	kprintf("state:              %s\n", state_to_text(thread, thread->state));
	kprintf("next_state:         %s\n", state_to_text(thread, thread->next_state));
	kprintf("cpu:                %p ", thread->cpu);
	if (thread->cpu)
		kprintf("(%d)\n", thread->cpu->cpu_num);
	else
		kprintf("\n");
	kprintf("sig_pending:        %#llx (blocked: %#llx"
		", before sigsuspend(): %#llx)\n",
		(long long)thread->ThreadPendingSignals(),
		(long long)thread->sig_block_mask,
		(long long)thread->sigsuspend_original_unblocked_mask);
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
	kprintf("team:               %p, \"%s\"\n", thread->team,
		thread->team->Name());
	kprintf("  exit.sem:         %ld\n", thread->exit.sem);
	kprintf("  exit.status:      %#lx (%s)\n", thread->exit.status, strerror(thread->exit.status));
	kprintf("  exit.waiters:\n");
	while ((death = (struct thread_death_entry*)list_get_next_item(
			&thread->exit.waiters, death)) != NULL) {
		kprintf("\t%p (thread %ld)\n", death, death->thread);
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
		for (ThreadHashTable::Iterator it = sThreadHash.GetIterator();
				Thread* thread = it.Next();) {
			if (!strcmp(name, thread->name) || thread->id == id) {
				_dump_thread_info(thread, shortInfo);
				found = true;
				break;
			}
		}

		if (!found)
			kprintf("thread \"%s\" (%ld) doesn't exist!\n", name, id);
	}

	return 0;
}


static int
dump_thread_list(int argc, char **argv)
{
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

	for (ThreadHashTable::Iterator it = sThreadHash.GetIterator();
			Thread* thread = it.Next();) {
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
	return 0;
}


//	#pragma mark - private kernel API


void
thread_exit(void)
{
	cpu_status state;
	Thread* thread = thread_get_current_thread();
	Team* team = thread->team;
	Team* kernelTeam = team_get_kernel_team();
	thread_id parentID = -1;
	status_t status;
	struct thread_debug_info debugInfo;
	team_id teamID = team->id;

	TRACE(("thread %ld exiting w/return code %#lx\n", thread->id,
		thread->exit.status));

	if (!are_interrupts_enabled())
		panic("thread_exit() called with interrupts disabled!\n");

	// boost our priority to get this over with
	thread->priority = thread->next_priority = B_URGENT_DISPLAY_PRIORITY;

	if (team != kernelTeam) {
		// Cancel previously installed alarm timer, if any. Hold the scheduler
		// lock to make sure that when cancel_timer() returns, the alarm timer
		// hook will not be invoked anymore (since
		// B_TIMER_ACQUIRE_SCHEDULER_LOCK is used).
		InterruptsSpinLocker schedulerLocker(gSchedulerLock);
		cancel_timer(&thread->alarm);
		schedulerLocker.Unlock();

		// Delete all user timers associated with the thread.
		ThreadLocker threadLocker(thread);
		thread->DeleteUserTimers(false);

		// detach the thread's user thread
		user_thread* userThread = thread->user_thread;
		thread->user_thread = NULL;

		threadLocker.Unlock();

		// Delete the thread's user thread, if it's not the main thread. If it
		// is, we can save the work, since it will be deleted with the team's
		// address space.
		if (thread != team->main_thread)
			team_free_user_thread(team, userThread);
	}

	// remember the user stack area -- we will delete it below
	area_id userStackArea = -1;
	if (team->address_space != NULL && thread->user_stack_area >= 0) {
		userStackArea = thread->user_stack_area;
		thread->user_stack_area = -1;
	}

	struct job_control_entry *death = NULL;
	struct thread_death_entry* threadDeathEntry = NULL;
	bool deleteTeam = false;
	port_id debuggerPort = -1;

	if (team != kernelTeam) {
		user_debug_thread_exiting(thread);

		if (team->main_thread == thread) {
			// The main thread is exiting. Shut down the whole team.
			deleteTeam = true;

			// kill off all other threads and the user debugger facilities
			debuggerPort = team_shutdown_team(team);

			// acquire necessary locks, which are: process group lock, kernel
			// team lock, parent team lock, and the team lock
			team->LockProcessGroup();
			kernelTeam->Lock();
			team->LockTeamAndParent(true);
		} else {
			threadDeathEntry
				= (thread_death_entry*)malloc(sizeof(thread_death_entry));

			// acquire necessary locks, which are: kernel team lock and the team
			// lock
			kernelTeam->Lock();
			team->Lock();
		}

		ThreadLocker threadLocker(thread);

		state = disable_interrupts();

		// swap address spaces, to make sure we're running on the kernel's pgdir
		vm_swap_address_space(team->address_space, VMAddressSpace::Kernel());

		SpinLocker schedulerLocker(gSchedulerLock);
			// removing the thread and putting its death entry to the parent
			// team needs to be an atomic operation

		// remember how long this thread lasted
		bigtime_t now = system_time();
		InterruptsSpinLocker threadTimeLocker(thread->time_lock);
		thread->kernel_time += now - thread->last_time;
		thread->last_time = now;
		threadTimeLocker.Unlock();

		team->dead_threads_kernel_time += thread->kernel_time;
		team->dead_threads_user_time += thread->user_time;

		// stop/update thread/team CPU time user timers
		if (thread->HasActiveCPUTimeUserTimers()
			|| team->HasActiveCPUTimeUserTimers()) {
			user_timer_stop_cpu_timers(thread, NULL);
		}

		// deactivate CPU time user timers for the thread
		if (thread->HasActiveCPUTimeUserTimers())
			thread->DeactivateCPUTimeUserTimers();

		// put the thread into the kernel team until it dies
		remove_thread_from_team(team, thread);
		insert_thread_into_team(kernelTeam, thread);

		if (team->death_entry != NULL) {
			if (--team->death_entry->remaining_threads == 0)
				team->death_entry->condition.NotifyOne(true, B_OK);
		}

		if (deleteTeam) {
			Team* parent = team->parent;

			// remember who our parent was so we can send a signal
			parentID = parent->id;

			// Set the team job control state to "dead" and detach the job
			// control entry from our team struct.
			team_set_job_control_state(team, JOB_CONTROL_STATE_DEAD, NULL,
				true);
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
			}

			schedulerLocker.Unlock();
			restore_interrupts(state);

			threadLocker.Unlock();

			// Get a temporary reference to the team's process group
			// -- team_remove_team() removes the team from the group, which
			// might destroy it otherwise and we wouldn't be able to unlock it.
			ProcessGroup* group = team->group;
			group->AcquireReference();

			pid_t foregroundGroupToSignal;
			team_remove_team(team, foregroundGroupToSignal);

			// unlock everything but the parent team
			team->Unlock();
			if (parent != kernelTeam)
				kernelTeam->Unlock();
			group->Unlock();
			group->ReleaseReference();

			// Send SIGCHLD to the parent as long as we still have its lock.
			// This makes job control state change + signalling atomic.
			Signal childSignal(SIGCHLD, team->exit.reason, B_OK, team->id);
			if (team->exit.reason == CLD_EXITED) {
				childSignal.SetStatus(team->exit.status);
			} else {
				childSignal.SetStatus(team->exit.signal);
				childSignal.SetSendingUser(team->exit.signaling_user);
			}
			send_signal_to_team(parent, childSignal, B_DO_NOT_RESCHEDULE);

			// also unlock the parent
			parent->Unlock();

			// If the team was a session leader with controlling TTY, we have
			// to send SIGHUP to the foreground process group.
			if (foregroundGroupToSignal >= 0) {
				Signal groupSignal(SIGHUP, SI_USER, B_OK, team->id);
				send_signal_to_process_group(foregroundGroupToSignal,
					groupSignal, B_DO_NOT_RESCHEDULE);
			}
		} else {
			// The thread is not the main thread. We store a thread death entry
			// for it, unless someone is already waiting for it.
			if (threadDeathEntry != NULL
				&& list_is_empty(&thread->exit.waiters)) {
				threadDeathEntry->thread = thread->id;
				threadDeathEntry->status = thread->exit.status;

				// add entry -- remove an old one, if we hit the limit
				list_add_item(&team->dead_threads, threadDeathEntry);
				team->dead_threads_count++;
				threadDeathEntry = NULL;

				if (team->dead_threads_count > MAX_DEAD_THREADS) {
					threadDeathEntry
						= (thread_death_entry*)list_remove_head_item(
							&team->dead_threads);
					team->dead_threads_count--;
				}
			}

			schedulerLocker.Unlock();
			restore_interrupts(state);

			threadLocker.Unlock();
			team->Unlock();
			kernelTeam->Unlock();
		}

		TRACE(("thread_exit: thread %ld now a kernel thread!\n", thread->id));
	}

	free(threadDeathEntry);

	// delete the team if we're its main thread
	if (deleteTeam) {
		team_delete_team(team, debuggerPort);

		// we need to delete any death entry that made it to here
		delete death;
	}

	ThreadLocker threadLocker(thread);

	state = disable_interrupts();
	SpinLocker schedulerLocker(gSchedulerLock);

	// mark invisible in global hash/list, so it's no longer accessible
	SpinLocker threadHashLocker(sThreadHashLock);
	thread->visible = false;
	sUsedThreads--;
	threadHashLocker.Unlock();

	// Stop debugging for this thread
	SpinLocker threadDebugInfoLocker(thread->debug_info.lock);
	debugInfo = thread->debug_info;
	clear_thread_debug_info(&thread->debug_info, true);
	threadDebugInfoLocker.Unlock();

	// Remove the select infos. We notify them a little later.
	select_info* selectInfos = thread->select_infos;
	thread->select_infos = NULL;

	schedulerLocker.Unlock();
	restore_interrupts(state);

	threadLocker.Unlock();

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

	// fill all death entries and delete the sem that others will use to wait
	// for us
	{
		sem_id cachedExitSem = thread->exit.sem;

		ThreadLocker threadLocker(thread);

		// make sure no one will grab this semaphore again
		thread->exit.sem = -1;

		// fill all death entries
		thread_death_entry* entry = NULL;
		while ((entry = (thread_death_entry*)list_get_next_item(
				&thread->exit.waiters, entry)) != NULL) {
			entry->status = thread->exit.status;
		}

		threadLocker.Unlock();

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
	if (teamID != kernelTeam->id)
		user_debug_thread_deleted(teamID, thread->id);

	// enqueue in the undertaker list and reschedule for the last time
	UndertakerEntry undertakerEntry(thread, teamID);

	disable_interrupts();
	schedulerLocker.Lock();

	sUndertakerEntries.Add(&undertakerEntry);
	sUndertakerCondition.NotifyOne(true);

	thread->next_state = THREAD_STATE_FREE_ON_RESCHED;
	scheduler_reschedule();

	panic("never can get here\n");
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
	SpinLocker threadTimeLocker(thread->time_lock);
	thread->user_time += now - thread->last_time;
	thread->last_time = now;
	thread->in_kernel = true;
	threadTimeLocker.Unlock();
}


/*!	Called whenever a thread exits kernel space to user space.
	Tracks time, handles signals, ...
	Interrupts must be enabled. When the function returns, interrupts will be
	disabled.
	The function may not return. This e.g. happens when the thread has received
	a deadly signal.
*/
void
thread_at_kernel_exit(void)
{
	Thread *thread = thread_get_current_thread();

	TRACE(("thread_at_kernel_exit: exit thread %ld\n", thread->id));

	handle_signals(thread);

	disable_interrupts();

	// track kernel time
	bigtime_t now = system_time();
	SpinLocker threadTimeLocker(thread->time_lock);
	thread->in_kernel = false;
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

	// track kernel time
	bigtime_t now = system_time();
	SpinLocker threadTimeLocker(thread->time_lock);
	thread->in_kernel = false;
	thread->kernel_time += now - thread->last_time;
	thread->last_time = now;
}


void
thread_reset_for_exec(void)
{
	Thread* thread = thread_get_current_thread();

	ThreadLocker threadLocker(thread);

	// delete user-defined timers
	thread->DeleteUserTimers(true);

	// cancel pre-defined timer
	if (UserTimer* timer = thread->UserTimerFor(USER_TIMER_REAL_TIME_ID))
		timer->Cancel();

	// reset user_thread and user stack
	thread->user_thread = NULL;
	thread->user_stack_area = -1;
	thread->user_stack_base = 0;
	thread->user_stack_size = 0;

	// reset signals
	InterruptsSpinLocker schedulerLocker(gSchedulerLock);

	thread->ResetSignalsOnExec();

	// reset thread CPU time clock
	thread->cpu_clock_offset = -thread->CPUTime(false);

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


thread_id
allocate_thread_id()
{
	InterruptsSpinLocker threadHashLocker(sThreadHashLock);

	// find the next unused ID
	thread_id id;
	do {
		id = sNextThreadID++;

		// deal with integer overflow
		if (sNextThreadID < 0)
			sNextThreadID = 2;

		// check whether the ID is already in use
	} while (sThreadHash.Lookup(id, false) != NULL);

	return id;
}


thread_id
peek_next_thread_id()
{
	InterruptsSpinLocker threadHashLocker(sThreadHashLock);
	return sNextThreadID;
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

		InterruptsSpinLocker _(gSchedulerLock);

		// mark the thread as yielded, so it will not be scheduled next
		//thread->was_yielded = true;
		thread->next_priority = B_LOWEST_ACTIVE_PRIORITY;
		scheduler_reschedule();
#endif
	} else {
		Thread *thread = thread_get_current_thread();
		if (thread == NULL)
			return;

		// Don't force the thread off the CPU, just reschedule.
		InterruptsSpinLocker _(gSchedulerLock);
		scheduler_reschedule();
	}
}


/*!	Kernel private thread creation function.
*/
thread_id
spawn_kernel_thread_etc(thread_func function, const char *name, int32 priority,
	void *arg, team_id team)
{
	return thread_create_thread(
		ThreadCreationAttributes(function, name, priority, arg, team),
		true);
}


status_t
wait_for_thread_etc(thread_id id, uint32 flags, bigtime_t timeout,
	status_t *_returnCode)
{
	job_control_entry* freeDeath = NULL;
	status_t status = B_OK;

	if (id < B_OK)
		return B_BAD_THREAD_ID;

	// get the thread, queue our death entry, and fetch the semaphore we have to
	// wait on
	sem_id exitSem = B_BAD_THREAD_ID;
	struct thread_death_entry death;

	Thread* thread = Thread::GetAndLock(id);
	if (thread != NULL) {
		// remember the semaphore we have to wait on and place our death entry
		exitSem = thread->exit.sem;
		list_add_link_to_head(&thread->exit.waiters, &death);

		thread->UnlockAndReleaseReference();
			// Note: We mustn't dereference the pointer afterwards, only check
			// it.
	}

	thread_death_entry* threadDeathEntry = NULL;

	if (thread == NULL) {
		// we couldn't find this thread -- maybe it's already gone, and we'll
		// find its death entry in our team
		Team* team = thread_get_current_thread()->team;
		TeamLocker teamLocker(team);

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
			while ((threadDeathEntry = (thread_death_entry*)list_get_next_item(
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
	}

	if (thread == NULL && status == B_OK) {
		// we found the thread's death entry in our team
		if (_returnCode)
			*_returnCode = death.status;

		delete freeDeath;
		free(threadDeathEntry);
		return B_OK;
	}

	// we need to wait for the death of the thread

	if (exitSem < 0)
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
		// We were probably interrupted or the timeout occurred; we need to
		// remove our death entry now.
		thread = Thread::GetAndLock(id);
		if (thread != NULL) {
			list_remove_link(&death);
			thread->UnlockAndReleaseReference();
		} else {
			// The thread is already gone, so we need to wait uninterruptibly
			// for its exit semaphore to make sure our death entry stays valid.
			// It won't take long, since the thread is apparently already in the
			// middle of the cleanup.
			acquire_sem(exitSem);
			status = B_OK;
		}
	}

	return status;
}


status_t
select_thread(int32 id, struct select_info* info, bool kernel)
{
	// get and lock the thread
	Thread* thread = Thread::GetAndLock(id);
	if (thread == NULL)
		return B_BAD_THREAD_ID;
	BReference<Thread> threadReference(thread, true);
	ThreadLocker threadLocker(thread, true);

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
	// get and lock the thread
	Thread* thread = Thread::GetAndLock(id);
	if (thread == NULL)
		return B_BAD_THREAD_ID;
	BReference<Thread> threadReference(thread, true);
	ThreadLocker threadLocker(thread, true);

	// remove info from list
	select_info** infoLocation = &thread->select_infos;
	while (*infoLocation != NULL && *infoLocation != info)
		infoLocation = &(*infoLocation)->next;

	if (*infoLocation != info)
		return B_OK;

	*infoLocation = info->next;

	threadLocker.Unlock();

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
	InterruptsSpinLocker threadHashLocker(sThreadHashLock);
	return sUsedThreads;
}


/*!	Returns a user-readable string for a thread state.
	Only for use in the kernel debugger.
*/
const char*
thread_state_to_text(Thread* thread, int32 state)
{
	return state_to_text(thread, state);
}


int32
thread_get_io_priority(thread_id id)
{
	Thread* thread = Thread::GetAndLock(id);
	if (thread == NULL)
		return B_BAD_THREAD_ID;
	BReference<Thread> threadReference(thread, true);
	ThreadLocker threadLocker(thread, true);

	int32 priority = thread->io_priority;
	if (priority < 0) {
		// negative I/O priority means using the (CPU) priority
		InterruptsSpinLocker schedulerLocker(gSchedulerLock);
		priority = thread->priority;
	}

	return priority;
}


void
thread_set_io_priority(int32 priority)
{
	Thread* thread = thread_get_current_thread();
	ThreadLocker threadLocker(thread);

	thread->io_priority = priority;
}


status_t
thread_init(kernel_args *args)
{
	TRACE(("thread_init: entry\n"));

	// create the thread hash table
	new(&sThreadHash) ThreadHashTable();
	if (sThreadHash.Init(128) != B_OK)
		panic("thread_init(): failed to init thread hash table!");

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
	for (uint32 i = 0; i < args->num_cpus; i++) {
		Thread *thread;
		area_info info;
		char name[64];

		sprintf(name, "idle thread %lu", i + 1);
		thread = new(&sIdleThreads[i]) Thread(name,
			i == 0 ? team_get_kernel_team_id() : -1, &gCPU[i]);
		if (thread == NULL || thread->Init(true) != B_OK) {
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

		if (get_area_info(thread->kernel_stack_area, &info) != B_OK)
			panic("error finding idle kstack area\n");

		thread->kernel_stack_base = (addr_t)info.address;
		thread->kernel_stack_top = thread->kernel_stack_base + info.size;

		thread->visible = true;
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
	// The timer has been installed with B_TIMER_ACQUIRE_SCHEDULER_LOCK, so
	// we're holding the scheduler lock already. This makes things comfortably
	// easy.

	Thread* thread = (Thread*)timer->user_data;
	thread_unblock_locked(thread, B_TIMED_OUT);

	return B_HANDLED_INTERRUPT;
}


/*!	Blocks the current thread.

	The function acquires the scheduler lock and calls thread_block_locked().
	See there for more information.
*/
status_t
thread_block()
{
	InterruptsSpinLocker _(gSchedulerLock);
	return thread_block_locked(thread_get_current_thread());
}


/*!	Blocks the current thread with a timeout.

	Acquires the scheduler lock and calls thread_block_with_timeout_locked().
	See there for more information.
*/
status_t
thread_block_with_timeout(uint32 timeoutFlags, bigtime_t timeout)
{
	InterruptsSpinLocker _(gSchedulerLock);
	return thread_block_with_timeout_locked(timeoutFlags, timeout);
}


/*!	Blocks the current thread with a timeout.

	The thread is blocked until someone else unblock it or the specified timeout
	occurs. Must be called after a call to thread_prepare_to_block(). If the
	thread has already been unblocked after the previous call to
	thread_prepare_to_block(), this function will return immediately. See
	thread_prepare_to_block() for more details.

	The caller must hold the scheduler lock.

	\param thread The current thread.
	\param timeoutFlags The standard timeout flags:
		- \c B_RELATIVE_TIMEOUT: \a timeout specifies the time to wait.
		- \c B_ABSOLUTE_TIMEOUT: \a timeout specifies the absolute end time when
			the timeout shall occur.
		- \c B_TIMEOUT_REAL_TIME_BASE: Only relevant when \c B_ABSOLUTE_TIMEOUT
			is specified, too. Specifies that \a timeout is a real time, not a
			system time.
		If neither \c B_RELATIVE_TIMEOUT nor \c B_ABSOLUTE_TIMEOUT are
		specified, an infinite timeout is implied and the function behaves like
		thread_block_locked().
	\return The error code passed to the unblocking function. thread_interrupt()
		uses \c B_INTERRUPTED. When the timeout occurred, \c B_TIMED_OUT is
		returned. By convention \c B_OK means that the wait was successful while
		another error code indicates a failure (what that means depends on the
		client code).
*/
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
				timerFlags |= B_TIMER_REAL_TIME_BASE;
		}
		timerFlags |= B_TIMER_ACQUIRE_SCHEDULER_LOCK;

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


/*!	Unblocks a userland-blocked thread.
	The caller must not hold any locks.
*/
static status_t
user_unblock_thread(thread_id threadID, status_t status)
{
	// get the thread
	Thread* thread = Thread::GetAndLock(threadID);
	if (thread == NULL)
		return B_BAD_THREAD_ID;
	BReference<Thread> threadReference(thread, true);
	ThreadLocker threadLocker(thread, true);

	if (thread->user_thread == NULL)
		return B_NOT_ALLOWED;

	InterruptsSpinLocker schedulerLocker(gSchedulerLock);

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
	Team* team = thread->team;

	thread->exit.status = returnValue;

	// if called from a kernel thread, we don't deliver the signal,
	// we just exit directly to keep the user space behaviour of
	// this function
	if (team != team_get_kernel_team()) {
		// If this is its main thread, set the team's exit status.
		if (thread == team->main_thread) {
			TeamLocker teamLocker(team);

			if (!team->exit.initialized) {
				team->exit.reason = CLD_EXITED;
				team->exit.signal = 0;
				team->exit.signaling_user = 0;
				team->exit.status = returnValue;
				team->exit.initialized = true;
			}

			teamLocker.Unlock();
		}

		Signal signal(SIGKILLTHR, SI_USER, B_OK, team->id);
		send_signal_to_thread(thread, signal, B_DO_NOT_RESCHEDULE);
	} else
		thread_exit();
}


status_t
kill_thread(thread_id id)
{
	if (id <= 0)
		return B_BAD_VALUE;

	Thread* currentThread = thread_get_current_thread();

	Signal signal(SIGKILLTHR, SI_USER, B_OK, currentThread->team->id);
	return send_signal_to_thread_id(id, signal, 0);
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
	// TODO: The thread argument is ignored.
	int32 count;

	if (get_sem_count(thread_get_current_thread()->msg.read_sem,
			&count) != B_OK)
		return false;

	return count == 0 ? false : true;
}


status_t
_get_thread_info(thread_id id, thread_info *info, size_t size)
{
	if (info == NULL || size != sizeof(thread_info) || id < B_OK)
		return B_BAD_VALUE;

	// get the thread
	Thread* thread = Thread::GetAndLock(id);
	if (thread == NULL)
		return B_BAD_THREAD_ID;
	BReference<Thread> threadReference(thread, true);
	ThreadLocker threadLocker(thread, true);

	// fill the info -- also requires the scheduler lock to be held
	InterruptsSpinLocker schedulerLocker(gSchedulerLock);

	fill_thread_info(thread, info, size);

	return B_OK;
}


status_t
_get_next_thread_info(team_id teamID, int32 *_cookie, thread_info *info,
	size_t size)
{
	if (info == NULL || size != sizeof(thread_info) || teamID < 0)
		return B_BAD_VALUE;

	int32 lastID = *_cookie;

	// get the team
	Team* team = Team::GetAndLock(teamID);
	if (team == NULL)
		return B_BAD_VALUE;
	BReference<Team> teamReference(team, true);
	TeamLocker teamLocker(team, true);

	Thread* thread = NULL;

	if (lastID == 0) {
		// We start with the main thread
		thread = team->main_thread;
	} else {
		// Find the one thread with an ID greater than ours (as long as the IDs
		// don't wrap they are always sorted from highest to lowest).
		// TODO: That is broken not only when the IDs wrap, but also for the
		// kernel team, to which threads are added when they are dying.
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

	ThreadLocker threadLocker(thread);
	InterruptsSpinLocker schedulerLocker(gSchedulerLock);

	fill_thread_info(thread, info, size);

	return B_OK;
}


thread_id
find_thread(const char* name)
{
	if (name == NULL)
		return thread_get_current_thread_id();

	InterruptsSpinLocker threadHashLocker(sThreadHashLock);

	// TODO: Scanning the whole hash with the thread hash lock held isn't
	// exactly cheap -- although this function is probably used very rarely.

	for (ThreadHashTable::Iterator it = sThreadHash.GetIterator();
			Thread* thread = it.Next();) {
		if (!thread->visible)
			continue;

		if (strcmp(thread->name, name) == 0)
			return thread->id;
	}

	return B_NAME_NOT_FOUND;
}


status_t
rename_thread(thread_id id, const char* name)
{
	if (name == NULL)
		return B_BAD_VALUE;

	// get the thread
	Thread* thread = Thread::GetAndLock(id);
	if (thread == NULL)
		return B_BAD_THREAD_ID;
	BReference<Thread> threadReference(thread, true);
	ThreadLocker threadLocker(thread, true);

	// check whether the operation is allowed
	if (thread->team != thread_get_current_thread()->team)
		return B_NOT_ALLOWED;

	strlcpy(thread->name, name, B_OS_NAME_LENGTH);

	team_id teamID = thread->team->id;

	threadLocker.Unlock();

	// notify listeners
	sNotificationService.Notify(THREAD_NAME_CHANGED, teamID, id);
		// don't pass the thread structure, as it's unsafe, if it isn't ours

	return B_OK;
}


status_t
set_thread_priority(thread_id id, int32 priority)
{
	int32 oldPriority;

	// make sure the passed in priority is within bounds
	if (priority > THREAD_MAX_SET_PRIORITY)
		priority = THREAD_MAX_SET_PRIORITY;
	if (priority < THREAD_MIN_SET_PRIORITY)
		priority = THREAD_MIN_SET_PRIORITY;

	// get the thread
	Thread* thread = Thread::GetAndLock(id);
	if (thread == NULL)
		return B_BAD_THREAD_ID;
	BReference<Thread> threadReference(thread, true);
	ThreadLocker threadLocker(thread, true);

	// check whether the change is allowed
	if (thread_is_idle_thread(thread))
		return B_NOT_ALLOWED;

	InterruptsSpinLocker schedulerLocker(gSchedulerLock);

	if (thread == thread_get_current_thread()) {
		// It's ourself, so we know we aren't in the run queue, and we can
		// manipulate our structure directly.
		oldPriority = thread->priority;
		thread->priority = thread->next_priority = priority;
	} else {
		oldPriority = thread->priority;
		scheduler_set_thread_priority(thread, priority);
	}

	return oldPriority;
}


status_t
snooze_etc(bigtime_t timeout, int timebase, uint32 flags)
{
	return common_snooze_etc(timeout, timebase, flags, NULL);
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

	Thread* currentThread = thread_get_current_thread();

	Signal signal(SIGSTOP, SI_USER, B_OK, currentThread->team->id);
	return send_signal_to_thread_id(id, signal, 0);
}


status_t
resume_thread(thread_id id)
{
	if (id <= 0)
		return B_BAD_VALUE;

	Thread* currentThread = thread_get_current_thread();

	// Using the kernel internal SIGNAL_CONTINUE_THREAD signal retains
	// compatibility to BeOS which documents the combination of suspend_thread()
	// and resume_thread() to interrupt threads waiting on semaphores.
	Signal signal(SIGNAL_CONTINUE_THREAD, SI_USER, B_OK,
		currentThread->team->id);
	return send_signal_to_thread_id(id, signal, 0);
}


thread_id
spawn_kernel_thread(thread_func function, const char *name, int32 priority,
	void *arg)
{
	return thread_create_thread(
		ThreadCreationAttributes(function, name, priority, arg),
		true);
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
	// TODO: Don't allow kernel threads to be killed!
	return kill_thread(thread);
}


status_t
_user_cancel_thread(thread_id threadID, void (*cancelFunction)(int))
{
	// check the cancel function
	if (cancelFunction == NULL || !IS_USER_ADDRESS(cancelFunction))
		return B_BAD_VALUE;

	// get and lock the thread
	Thread* thread = Thread::GetAndLock(threadID);
	if (thread == NULL)
		return B_BAD_THREAD_ID;
	BReference<Thread> threadReference(thread, true);
	ThreadLocker threadLocker(thread, true);

	// only threads of the same team can be canceled
	if (thread->team != thread_get_current_thread()->team)
		return B_NOT_ALLOWED;

	// set the cancel function
	thread->cancel_function = cancelFunction;

	// send the cancellation signal to the thread
	InterruptsSpinLocker schedulerLocker(gSchedulerLock);
	return send_signal_to_thread_locked(thread, SIGNAL_CANCEL_THREAD, NULL, 0);
}


status_t
_user_resume_thread(thread_id thread)
{
	// TODO: Don't allow kernel threads to be resumed!
	return resume_thread(thread);
}


status_t
_user_suspend_thread(thread_id thread)
{
	// TODO: Don't allow kernel threads to be suspended!
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

	// TODO: Don't allow kernel threads to be renamed!
	return rename_thread(thread, name);
}


int32
_user_set_thread_priority(thread_id thread, int32 newPriority)
{
	// TODO: Don't allow setting priority of kernel threads!
	return set_thread_priority(thread, newPriority);
}


thread_id
_user_spawn_thread(thread_creation_attributes* userAttributes)
{
	// copy the userland structure to the kernel
	char nameBuffer[B_OS_NAME_LENGTH];
	ThreadCreationAttributes attributes;
	status_t error = attributes.InitFromUserAttributes(userAttributes,
		nameBuffer);
	if (error != B_OK)
		return error;

	// create the thread
	thread_id threadID = thread_create_thread(attributes, false);

	if (threadID >= 0)
		user_debug_thread_created(threadID);

	return threadID;
}


status_t
_user_snooze_etc(bigtime_t timeout, int timebase, uint32 flags,
	bigtime_t* userRemainingTime)
{
	// We need to store more syscall restart parameters than usual and need a
	// somewhat different handling. Hence we can't use
	// syscall_restart_handle_timeout_pre() but do the job ourselves.
	struct restart_parameters {
		bigtime_t	timeout;
		clockid_t	timebase;
		uint32		flags;
	};

	Thread* thread = thread_get_current_thread();

	if ((thread->flags & THREAD_FLAGS_SYSCALL_RESTARTED) != 0) {
		// The syscall was restarted. Fetch the parameters from the stored
		// restart parameters.
		restart_parameters* restartParameters
			= (restart_parameters*)thread->syscall_restart.parameters;
		timeout = restartParameters->timeout;
		timebase = restartParameters->timebase;
		flags = restartParameters->flags;
	} else {
		// convert relative timeouts to absolute ones
		if ((flags & B_RELATIVE_TIMEOUT) != 0) {
			// not restarted yet and the flags indicate a relative timeout

			// Make sure we use the system time base, so real-time clock changes
			// won't affect our wait.
			flags &= ~(uint32)B_TIMEOUT_REAL_TIME_BASE;
			if (timebase == CLOCK_REALTIME)
				timebase = CLOCK_MONOTONIC;

			// get the current time and make the timeout absolute
			bigtime_t now;
			status_t error = user_timer_get_clock(timebase, now);
			if (error != B_OK)
				return error;

			timeout += now;

			// deal with overflow
			if (timeout < 0)
				timeout = B_INFINITE_TIMEOUT;

			flags = (flags & ~B_RELATIVE_TIMEOUT) | B_ABSOLUTE_TIMEOUT;
		} else
			flags |= B_ABSOLUTE_TIMEOUT;
	}

	// snooze
	bigtime_t remainingTime;
	status_t error = common_snooze_etc(timeout, timebase,
		flags | B_CAN_INTERRUPT | B_CHECK_PERMISSION,
		userRemainingTime != NULL ? &remainingTime : NULL);

	// If interrupted, copy the remaining time back to userland and prepare the
	// syscall restart.
	if (error == B_INTERRUPTED) {
		if (userRemainingTime != NULL
			&& (!IS_USER_ADDRESS(userRemainingTime)
				|| user_memcpy(userRemainingTime, &remainingTime,
					sizeof(remainingTime)) != B_OK)) {
			return B_BAD_ADDRESS;
		}

		// store the normalized values in the restart parameters
		restart_parameters* restartParameters
			= (restart_parameters*)thread->syscall_restart.parameters;
		restartParameters->timeout = timeout;
		restartParameters->timebase = timebase;
		restartParameters->flags = flags;

		// restart the syscall, if possible
		atomic_or(&thread->flags, THREAD_FLAGS_RESTART_SYSCALL);
	}

	return error;
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
	ThreadLocker threadLocker(thread);

	// check, if already done
	if (thread->user_thread->wait_status <= 0)
		return thread->user_thread->wait_status;

	// nope, so wait
	thread_prepare_to_block(thread, flags, THREAD_BLOCK_TYPE_OTHER, "user");

	threadLocker.Unlock();
	InterruptsSpinLocker schedulerLocker(gSchedulerLock);

	status_t status = thread_block_with_timeout_locked(flags, timeout);

	schedulerLocker.Unlock();
	threadLocker.Lock();

	// Interruptions or timeouts can race with other threads unblocking us.
	// Favor a wake-up by another thread, i.e. if someone changed the wait
	// status, use that.
	status_t oldStatus = thread->user_thread->wait_status;
	if (oldStatus > 0)
		thread->user_thread->wait_status = status;
	else
		status = oldStatus;

	threadLocker.Unlock();

	return syscall_restart_handle_timeout_post(status, timeout);
}


status_t
_user_unblock_thread(thread_id threadID, status_t status)
{
	status_t error = user_unblock_thread(threadID, status);

	if (error == B_OK)
		scheduler_reschedule_if_necessary();

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

	for (uint32 i = 0; i < count; i++)
		user_unblock_thread(threads[i], status);

	scheduler_reschedule_if_necessary();

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
