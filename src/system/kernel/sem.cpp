/*
 * Copyright 2008-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


/*! Semaphore code */


#include <sem.h>

#include <stdlib.h>
#include <string.h>

#include <OS.h>

#include <arch/int.h>
#include <boot/kernel_args.h>
#include <cpu.h>
#include <debug.h>
#include <int.h>
#include <kernel.h>
#include <ksignal.h>
#include <kscheduler.h>
#include <listeners.h>
#include <scheduling_analysis.h>
#include <smp.h>
#include <syscall_restart.h>
#include <team.h>
#include <thread.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>
#include <vfs.h>
#include <vm/vm_page.h>
#include <wait_for_objects.h>

#include "kernel_debug_config.h"


//#define TRACE_SEM
#ifdef TRACE_SEM
#	define TRACE(x) dprintf_no_syslog x
#else
#	define TRACE(x) ;
#endif

//#define KTRACE_SEM
#ifdef KTRACE_SEM
#	define KTRACE(x...) ktrace_printf(x)
#else
#	define KTRACE(x...) do {} while (false)
#endif


// Locking:
// * sSemsSpinlock: Protects the semaphore free list (sFreeSemsHead,
//   sFreeSemsTail), Team::sem_list, and together with sem_entry::lock
//   write access to sem_entry::owner/team_link.
// * sem_entry::lock: Protects all sem_entry members. owner, team_link
//   additional need sSemsSpinlock for write access.
//   lock itself doesn't need protection -- sem_entry objects are never deleted.
//
// The locking order is sSemsSpinlock -> sem_entry::lock -> scheduler lock. All
// semaphores are in the sSems array (sem_entry[]). Access by sem_id requires
// computing the object index (id % sMaxSems), locking the respective
// sem_entry::lock and verifying that sem_entry::id matches afterwards.


struct queued_thread : DoublyLinkedListLinkImpl<queued_thread> {
	queued_thread(Thread *thread, int32 count)
		:
		thread(thread),
		count(count),
		queued(false)
	{
	}

	Thread	*thread;
	int32	count;
	bool	queued;
};

typedef DoublyLinkedList<queued_thread> ThreadQueue;

struct sem_entry {
	union {
		// when slot in use
		struct {
			struct list_link	team_link;
			int32				count;
			int32				net_count;
									// count + acquisition count of all blocked
									// threads
			char*				name;
			team_id				owner;
			select_info*		select_infos;
			thread_id			last_acquirer;
#if DEBUG_SEM_LAST_ACQUIRER
			int32				last_acquire_count;
			thread_id			last_releaser;
			int32				last_release_count;
#endif
		} used;

		// when slot unused
		struct {
			sem_id				next_id;
			struct sem_entry*	next;
		} unused;
	} u;

	sem_id				id;
	spinlock			lock;	// protects only the id field when unused
	ThreadQueue			queue;	// should be in u.used, but has a constructor
};

static const int32 kMaxSemaphores = 65536;
static int32 sMaxSems = 4096;
	// Final value is computed based on the amount of available memory
static int32 sUsedSems = 0;

static struct sem_entry *sSems = NULL;
static bool sSemsActive = false;
static struct sem_entry	*sFreeSemsHead = NULL;
static struct sem_entry	*sFreeSemsTail = NULL;

static spinlock sSemsSpinlock = B_SPINLOCK_INITIALIZER;


static int
dump_sem_list(int argc, char** argv)
{
	const char* name = NULL;
	team_id owner = -1;
	thread_id last = -1;
	int32 i;

	if (argc > 2) {
		if (!strcmp(argv[1], "team") || !strcmp(argv[1], "owner"))
			owner = strtoul(argv[2], NULL, 0);
		else if (!strcmp(argv[1], "name"))
			name = argv[2];
		else if (!strcmp(argv[1], "last"))
			last = strtoul(argv[2], NULL, 0);
	} else if (argc > 1)
		owner = strtoul(argv[1], NULL, 0);

	kprintf("%-*s       id count   team   last  name\n", B_PRINTF_POINTER_WIDTH,
		"sem");

	for (i = 0; i < sMaxSems; i++) {
		struct sem_entry* sem = &sSems[i];
		if (sem->id < 0
			|| (last != -1 && sem->u.used.last_acquirer != last)
			|| (name != NULL && strstr(sem->u.used.name, name) == NULL)
			|| (owner != -1 && sem->u.used.owner != owner))
			continue;

		kprintf("%p %6" B_PRId32 " %5" B_PRId32 " %6" B_PRId32 " "
			"%6" B_PRId32 " "
			" %s\n", sem, sem->id, sem->u.used.count,
			sem->u.used.owner,
			sem->u.used.last_acquirer > 0 ? sem->u.used.last_acquirer : 0,
			sem->u.used.name);
	}

	return 0;
}


static void
dump_sem(struct sem_entry* sem)
{
	kprintf("SEM: %p\n", sem);
	kprintf("id:      %" B_PRId32 " (%#" B_PRIx32 ")\n", sem->id, sem->id);
	if (sem->id >= 0) {
		kprintf("name:    '%s'\n", sem->u.used.name);
		kprintf("owner:   %" B_PRId32 "\n", sem->u.used.owner);
		kprintf("count:   %" B_PRId32 "\n", sem->u.used.count);
		kprintf("queue:  ");
		if (!sem->queue.IsEmpty()) {
			ThreadQueue::Iterator it = sem->queue.GetIterator();
			while (queued_thread* entry = it.Next())
				kprintf(" %" B_PRId32, entry->thread->id);
			kprintf("\n");
		} else
			kprintf(" -\n");

		set_debug_variable("_sem", (addr_t)sem);
		set_debug_variable("_semID", sem->id);
		set_debug_variable("_owner", sem->u.used.owner);

#if DEBUG_SEM_LAST_ACQUIRER
		kprintf("last acquired by: %" B_PRId32 ", count: %" B_PRId32 "\n",
			sem->u.used.last_acquirer, sem->u.used.last_acquire_count);
		kprintf("last released by: %" B_PRId32 ", count: %" B_PRId32 "\n",
			sem->u.used.last_releaser, sem->u.used.last_release_count);

		if (sem->u.used.last_releaser != 0)
			set_debug_variable("_releaser", sem->u.used.last_releaser);
		else
			unset_debug_variable("_releaser");
#else
		kprintf("last acquired by: %" B_PRId32 "\n", sem->u.used.last_acquirer);
#endif

		if (sem->u.used.last_acquirer != 0)
			set_debug_variable("_acquirer", sem->u.used.last_acquirer);
		else
			unset_debug_variable("_acquirer");
	} else {
		kprintf("next:    %p\n", sem->u.unused.next);
		kprintf("next_id: %" B_PRId32 "\n", sem->u.unused.next_id);
	}
}


static int
dump_sem_info(int argc, char **argv)
{
	bool found = false;
	addr_t num;
	int32 i;

	if (argc < 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	char* endptr;
	num = strtoul(argv[1], &endptr, 0);

	if (endptr != argv[1]) {
		if (IS_KERNEL_ADDRESS(num)) {
			dump_sem((struct sem_entry *)num);
			return 0;
		} else {
			uint32 slot = num % sMaxSems;
			if (sSems[slot].id != (int)num) {
				kprintf("sem %ld (%#lx) doesn't exist!\n", num, num);
				return 0;
			}

			dump_sem(&sSems[slot]);
			return 0;
		}
	}

	// walk through the sem list, trying to match name
	for (i = 0; i < sMaxSems; i++) {
		if (sSems[i].u.used.name != NULL
			&& strcmp(argv[1], sSems[i].u.used.name) == 0) {
			dump_sem(&sSems[i]);
			found = true;
		}
	}

	if (!found)
		kprintf("sem \"%s\" doesn't exist!\n", argv[1]);
	return 0;
}


/*!	\brief Appends a semaphore slot to the free list.

	The semaphore list must be locked.
	The slot's id field is not changed. It should already be set to -1.

	\param slot The index of the semaphore slot.
	\param nextID The ID the slot will get when reused. If < 0 the \a slot
		   is used.
*/
static void
free_sem_slot(int slot, sem_id nextID)
{
	struct sem_entry *sem = sSems + slot;
	// set next_id to the next possible value; for sanity check the current ID
	if (nextID < 0)
		sem->u.unused.next_id = slot;
	else
		sem->u.unused.next_id = nextID;
	// append the entry to the list
	if (sFreeSemsTail)
		sFreeSemsTail->u.unused.next = sem;
	else
		sFreeSemsHead = sem;
	sFreeSemsTail = sem;
	sem->u.unused.next = NULL;
}


static inline void
notify_sem_select_events(struct sem_entry* sem, uint16 events)
{
	if (sem->u.used.select_infos)
		notify_select_events_list(sem->u.used.select_infos, events);
}


/*!	Fills the sem_info structure with information from the given semaphore.
	The semaphore's lock must be held when called.
*/
static void
fill_sem_info(struct sem_entry* sem, sem_info* info, size_t size)
{
	info->sem = sem->id;
	info->team = sem->u.used.owner;
	strlcpy(info->name, sem->u.used.name, sizeof(info->name));
	info->count = sem->u.used.count;
	info->latest_holder = sem->u.used.last_acquirer;
}


/*!	You must call this function with interrupts disabled, and the semaphore's
	spinlock held. Note that it will unlock the spinlock itself.
	Since it cannot free() the semaphore's name with interrupts turned off, it
	will return that one in \a name.
*/
static void
uninit_sem_locked(struct sem_entry& sem, char** _name, SpinLocker& locker)
{
	KTRACE("delete_sem(sem: %ld)", sem.u.used.id);

	notify_sem_select_events(&sem, B_EVENT_INVALID);
	sem.u.used.select_infos = NULL;

	// free any threads waiting for this semaphore
	while (queued_thread* entry = sem.queue.RemoveHead()) {
		entry->queued = false;
		thread_unblock(entry->thread, B_BAD_SEM_ID);
	}

	int32 id = sem.id;
	sem.id = -1;
	*_name = sem.u.used.name;
	sem.u.used.name = NULL;

	locker.Unlock();

	// append slot to the free list
	SpinLocker _(&sSemsSpinlock);
	free_sem_slot(id % sMaxSems, id + sMaxSems);
	atomic_add(&sUsedSems, -1);
}


static status_t
delete_sem_internal(sem_id id, bool checkPermission)
{
	if (sSemsActive == false)
		return B_NO_MORE_SEMS;
	if (id < 0)
		return B_BAD_SEM_ID;

	int32 slot = id % sMaxSems;

	InterruptsLocker interruptsLocker;
	SpinLocker listLocker(sSemsSpinlock);
	SpinLocker semLocker(sSems[slot].lock);

	if (sSems[slot].id != id) {
		TRACE(("delete_sem: invalid sem_id %ld\n", id));
		return B_BAD_SEM_ID;
	}

	if (checkPermission
		&& sSems[slot].u.used.owner == team_get_kernel_team_id()) {
		dprintf("thread %" B_PRId32 " tried to delete kernel semaphore "
			"%" B_PRId32 ".\n", thread_get_current_thread_id(), id);
		return B_NOT_ALLOWED;
	}

	if (sSems[slot].u.used.owner >= 0) {
		list_remove_link(&sSems[slot].u.used.team_link);
		sSems[slot].u.used.owner = -1;
	} else
		panic("sem %" B_PRId32 " has no owner", id);

	listLocker.Unlock();

	char* name;
	uninit_sem_locked(sSems[slot], &name, semLocker);

	SpinLocker schedulerLocker(thread_get_current_thread()->scheduler_lock);
	scheduler_reschedule_if_necessary_locked();
	schedulerLocker.Unlock();

	interruptsLocker.Unlock();

	free(name);
	return B_OK;
}


//	#pragma mark - Private Kernel API


// TODO: Name clash with POSIX sem_init()... (we could just use C++)
status_t
haiku_sem_init(kernel_args *args)
{
	area_id area;
	int32 i;

	TRACE(("sem_init: entry\n"));

	// compute maximal number of semaphores depending on the available memory
	// 128 MB -> 16384 semaphores, 448 kB fixed array size
	// 256 MB -> 32768, 896 kB
	// 512 MB and more-> 65536, 1.75 MB
	i = vm_page_num_pages() / 2;
	while (sMaxSems < i && sMaxSems < kMaxSemaphores)
		sMaxSems <<= 1;

	// create and initialize semaphore table
	virtual_address_restrictions virtualRestrictions = {};
	virtualRestrictions.address_specification = B_ANY_KERNEL_ADDRESS;
	physical_address_restrictions physicalRestrictions = {};
	area = create_area_etc(B_SYSTEM_TEAM, "sem_table",
		sizeof(struct sem_entry) * sMaxSems, B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, CREATE_AREA_DONT_WAIT, 0,
		&virtualRestrictions, &physicalRestrictions, (void**)&sSems);
	if (area < 0)
		panic("unable to allocate semaphore table!\n");

	memset(sSems, 0, sizeof(struct sem_entry) * sMaxSems);
	for (i = 0; i < sMaxSems; i++) {
		sSems[i].id = -1;
		free_sem_slot(i, i);
	}

	// add debugger commands
	add_debugger_command_etc("sems", &dump_sem_list,
		"Dump a list of all active semaphores (for team, with name, etc.)",
		"[ ([ \"team\" | \"owner\" ] <team>) | (\"name\" <name>) ]"
			" | (\"last\" <last acquirer>)\n"
		"Prints a list of all active semaphores meeting the given\n"
		"requirement. If no argument is given, all sems are listed.\n"
		"  <team>             - The team owning the semaphores.\n"
		"  <name>             - Part of the name of the semaphores.\n"
		"  <last acquirer>    - The thread that last acquired the semaphore.\n"
		, 0);
	add_debugger_command_etc("sem", &dump_sem_info,
		"Dump info about a particular semaphore",
		"<sem>\n"
		"Prints info about the specified semaphore.\n"
		"  <sem>  - pointer to the semaphore structure, semaphore ID, or name\n"
		"           of the semaphore to print info for.\n", 0);

	TRACE(("sem_init: exit\n"));

	sSemsActive = true;

	return 0;
}


/*!	Creates a semaphore with the given parameters.

	This function is only available from within the kernel, and
	should not be made public - if possible, we should remove it
	completely (and have only create_sem() exported).
*/
sem_id
create_sem_etc(int32 count, const char* name, team_id owner)
{
	struct sem_entry* sem = NULL;
	sem_id id = B_NO_MORE_SEMS;
	char* tempName;
	size_t nameLength;

	if (sSemsActive == false || sUsedSems == sMaxSems)
		return B_NO_MORE_SEMS;

	if (name == NULL)
		name = "unnamed semaphore";

	// get the owning team
	Team* team = Team::Get(owner);
	if (team == NULL)
		return B_BAD_TEAM_ID;
	BReference<Team> teamReference(team, true);

	// clone the name
	nameLength = strlen(name) + 1;
	nameLength = min_c(nameLength, B_OS_NAME_LENGTH);
	tempName = (char*)malloc(nameLength);
	if (tempName == NULL)
		return B_NO_MEMORY;

	strlcpy(tempName, name, nameLength);

	InterruptsSpinLocker _(&sSemsSpinlock);

	// get the first slot from the free list
	sem = sFreeSemsHead;
	if (sem) {
		// remove it from the free list
		sFreeSemsHead = sem->u.unused.next;
		if (!sFreeSemsHead)
			sFreeSemsTail = NULL;

		// init the slot
		SpinLocker semLocker(sem->lock);
		sem->id = sem->u.unused.next_id;
		sem->u.used.count = count;
		sem->u.used.net_count = count;
		new(&sem->queue) ThreadQueue;
		sem->u.used.name = tempName;
		sem->u.used.owner = team->id;
		sem->u.used.select_infos = NULL;
		id = sem->id;

		list_add_item(&team->sem_list, &sem->u.used.team_link);

		semLocker.Unlock();

		atomic_add(&sUsedSems, 1);

		KTRACE("create_sem_etc(count: %ld, name: %s, owner: %ld) -> %ld",
			count, name, owner, id);

		T_SCHEDULING_ANALYSIS(CreateSemaphore(id, name));
		NotifyWaitObjectListeners(&WaitObjectListener::SemaphoreCreated, id,
			name);
	}

	if (sem == NULL)
		free(tempName);

	return id;
}


status_t
select_sem(int32 id, struct select_info* info, bool kernel)
{
	if (id < 0)
		return B_BAD_SEM_ID;

	int32 slot = id % sMaxSems;

	InterruptsSpinLocker _(&sSems[slot].lock);

	if (sSems[slot].id != id) {
		// bad sem ID
		return B_BAD_SEM_ID;
	} else if (!kernel
		&& sSems[slot].u.used.owner == team_get_kernel_team_id()) {
		// kernel semaphore, but call from userland
		return B_NOT_ALLOWED;
	} else {
		info->selected_events &= B_EVENT_ACQUIRE_SEMAPHORE | B_EVENT_INVALID;

		if (info->selected_events != 0) {
			info->next = sSems[slot].u.used.select_infos;
			sSems[slot].u.used.select_infos = info;

			if (sSems[slot].u.used.count > 0)
				notify_select_events(info, B_EVENT_ACQUIRE_SEMAPHORE);
		}
	}

	return B_OK;
}


status_t
deselect_sem(int32 id, struct select_info* info, bool kernel)
{
	if (id < 0)
		return B_BAD_SEM_ID;

	if (info->selected_events == 0)
		return B_OK;

	int32 slot = id % sMaxSems;

	InterruptsSpinLocker _(&sSems[slot].lock);

	if (sSems[slot].id == id) {
		select_info** infoLocation = &sSems[slot].u.used.select_infos;
		while (*infoLocation != NULL && *infoLocation != info)
			infoLocation = &(*infoLocation)->next;

		if (*infoLocation == info)
			*infoLocation = info->next;
	}

	return B_OK;
}


/*!	Forcibly removes a thread from a semaphores wait queue. May have to wake up
	other threads in the process.
	Must be called with semaphore lock held. The thread lock must not be held.
*/
static void
remove_thread_from_sem(queued_thread *entry, struct sem_entry *sem)
{
	if (!entry->queued)
		return;

	sem->queue.Remove(entry);
	entry->queued = false;
	sem->u.used.count += entry->count;

	// We're done with this entry. We only have to check, if other threads
	// need unblocking, too.

	// Now see if more threads need to be woken up. We get the scheduler lock
	// for that time, so the blocking state of threads won't change (due to
	// interruption or timeout). We need that lock anyway when unblocking a
	// thread.
	while ((entry = sem->queue.Head()) != NULL) {
		SpinLocker schedulerLocker(entry->thread->scheduler_lock);
		if (thread_is_blocked(entry->thread)) {
			// The thread is still waiting. If its count is satisfied, unblock
			// it. Otherwise we can't unblock any other thread.
			if (entry->count > sem->u.used.net_count)
				break;

			thread_unblock_locked(entry->thread, B_OK);
			sem->u.used.net_count -= entry->count;
		} else {
			// The thread is no longer waiting, but still queued, which means
			// acquiration failed and we can just remove it.
			sem->u.used.count += entry->count;
		}

		sem->queue.Remove(entry);
		entry->queued = false;
	}

	// select notification, if the semaphore is now acquirable
	if (sem->u.used.count > 0)
		notify_sem_select_events(sem, B_EVENT_ACQUIRE_SEMAPHORE);
}


/*!	This function deletes all semaphores belonging to a particular team.
*/
void
sem_delete_owned_sems(Team* team)
{
	while (true) {
		char* name;

		{
			// get the next semaphore from the team's sem list
			InterruptsLocker _;
			SpinLocker semListLocker(sSemsSpinlock);
			sem_entry* sem = (sem_entry*)list_remove_head_item(&team->sem_list);
			if (sem == NULL)
				break;

			// delete the semaphore
			SpinLocker semLocker(sem->lock);
			semListLocker.Unlock();
			uninit_sem_locked(*sem, &name, semLocker);
		}

		free(name);
	}

	scheduler_reschedule_if_necessary();
}


int32
sem_max_sems(void)
{
	return sMaxSems;
}


int32
sem_used_sems(void)
{
	return sUsedSems;
}


//	#pragma mark - Public Kernel API


sem_id
create_sem(int32 count, const char* name)
{
	return create_sem_etc(count, name, team_get_kernel_team_id());
}


status_t
delete_sem(sem_id id)
{
	return delete_sem_internal(id, false);
}


status_t
acquire_sem(sem_id id)
{
	return switch_sem_etc(-1, id, 1, 0, 0);
}


status_t
acquire_sem_etc(sem_id id, int32 count, uint32 flags, bigtime_t timeout)
{
	return switch_sem_etc(-1, id, count, flags, timeout);
}


status_t
switch_sem(sem_id toBeReleased, sem_id toBeAcquired)
{
	return switch_sem_etc(toBeReleased, toBeAcquired, 1, 0, 0);
}


status_t
switch_sem_etc(sem_id semToBeReleased, sem_id id, int32 count,
	uint32 flags, bigtime_t timeout)
{
	int slot = id % sMaxSems;

	if (gKernelStartup)
		return B_OK;
	if (sSemsActive == false)
		return B_NO_MORE_SEMS;
#if KDEBUG
	if (!are_interrupts_enabled()) {
		panic("switch_sem_etc: called with interrupts disabled for sem "
			"%" B_PRId32 "\n", id);
	}
#endif

	if (id < 0)
		return B_BAD_SEM_ID;
	if (count <= 0
		|| (flags & (B_RELATIVE_TIMEOUT | B_ABSOLUTE_TIMEOUT)) == (B_RELATIVE_TIMEOUT | B_ABSOLUTE_TIMEOUT)) {
		return B_BAD_VALUE;
	}

	InterruptsLocker _;
	SpinLocker semLocker(sSems[slot].lock);

	if (sSems[slot].id != id) {
		TRACE(("switch_sem_etc: bad sem %ld\n", id));
		return B_BAD_SEM_ID;
	}

	// TODO: the B_CHECK_PERMISSION flag should be made private, as it
	//	doesn't have any use outside the kernel
	if ((flags & B_CHECK_PERMISSION) != 0
		&& sSems[slot].u.used.owner == team_get_kernel_team_id()) {
		dprintf("thread %" B_PRId32 " tried to acquire kernel semaphore "
			"%" B_PRId32 ".\n", thread_get_current_thread_id(), id);
#if 0
		_user_debugger("Thread tried to acquire kernel semaphore.");
#endif
		return B_NOT_ALLOWED;
	}

	if (sSems[slot].u.used.count - count < 0) {
		if ((flags & B_RELATIVE_TIMEOUT) != 0 && timeout <= 0) {
			// immediate timeout
			return B_WOULD_BLOCK;
		} else if ((flags & B_ABSOLUTE_TIMEOUT) != 0 && timeout < 0) {
			// absolute negative timeout
			return B_TIMED_OUT;
		}
	}

	KTRACE("switch_sem_etc(semToBeReleased: %ld, sem: %ld, count: %ld, "
		"flags: 0x%lx, timeout: %lld)", semToBeReleased, id, count, flags,
		timeout);

	if ((sSems[slot].u.used.count -= count) < 0) {
		// we need to block
		Thread *thread = thread_get_current_thread();

		TRACE(("switch_sem_etc(id = %ld): block name = %s, thread = %p,"
			" name = %s\n", id, sSems[slot].u.used.name, thread, thread->name));

		// do a quick check to see if the thread has any pending signals
		// this should catch most of the cases where the thread had a signal
		SpinLocker schedulerLocker(thread->scheduler_lock);
		if (thread_is_interrupted(thread, flags)) {
			schedulerLocker.Unlock();
			sSems[slot].u.used.count += count;
			if (semToBeReleased >= B_OK) {
				// we need to still release the semaphore to always
				// leave in a consistent state
				release_sem_etc(semToBeReleased, 1, B_DO_NOT_RESCHEDULE);
			}
			return B_INTERRUPTED;
		}

		schedulerLocker.Unlock();

		if ((flags & (B_RELATIVE_TIMEOUT | B_ABSOLUTE_TIMEOUT)) == 0)
			timeout = B_INFINITE_TIMEOUT;

		// enqueue in the semaphore queue and get ready to wait
		queued_thread queueEntry(thread, count);
		sSems[slot].queue.Add(&queueEntry);
		queueEntry.queued = true;

		thread_prepare_to_block(thread, flags, THREAD_BLOCK_TYPE_SEMAPHORE,
			(void*)(addr_t)id);

		semLocker.Unlock();

		// release the other semaphore, if any
		if (semToBeReleased >= 0) {
			release_sem_etc(semToBeReleased, 1, B_DO_NOT_RESCHEDULE);
			semToBeReleased = -1;
		}

		status_t acquireStatus = timeout == B_INFINITE_TIMEOUT
			? thread_block() : thread_block_with_timeout(flags, timeout);

		semLocker.Lock();

		// If we're still queued, this means the acquiration failed, and we
		// need to remove our entry and (potentially) wake up other threads.
		if (queueEntry.queued)
			remove_thread_from_sem(&queueEntry, &sSems[slot]);

		if (acquireStatus >= B_OK) {
			sSems[slot].u.used.last_acquirer = thread_get_current_thread_id();
#if DEBUG_SEM_LAST_ACQUIRER
			sSems[slot].u.used.last_acquire_count = count;
#endif
		}

		semLocker.Unlock();

		TRACE(("switch_sem_etc(sem %ld): exit block name %s, "
			"thread %ld (%s)\n", id, sSems[slot].u.used.name, thread->id,
			thread->name));
		KTRACE("switch_sem_etc() done: 0x%lx", acquireStatus);
		return acquireStatus;
	} else {
		sSems[slot].u.used.net_count -= count;
		sSems[slot].u.used.last_acquirer = thread_get_current_thread_id();
#if DEBUG_SEM_LAST_ACQUIRER
		sSems[slot].u.used.last_acquire_count = count;
#endif
	}

	KTRACE("switch_sem_etc() done: 0x%lx", status);

	return B_OK;
}


status_t
release_sem(sem_id id)
{
	return release_sem_etc(id, 1, 0);
}


status_t
release_sem_etc(sem_id id, int32 count, uint32 flags)
{
	int32 slot = id % sMaxSems;

	if (gKernelStartup)
		return B_OK;
	if (sSemsActive == false)
		return B_NO_MORE_SEMS;
	if (id < 0)
		return B_BAD_SEM_ID;
	if (count <= 0 && (flags & B_RELEASE_ALL) == 0)
		return B_BAD_VALUE;
#if KDEBUG
	if ((flags & B_DO_NOT_RESCHEDULE) == 0 && !are_interrupts_enabled()) {
		panic("release_sem_etc(): called with interrupts disabled and "
			"rescheduling allowed for sem_id %" B_PRId32, id);
	}
#endif

	InterruptsLocker _;
	SpinLocker semLocker(sSems[slot].lock);

	if (sSems[slot].id != id) {
		TRACE(("sem_release_etc: invalid sem_id %ld\n", id));
		return B_BAD_SEM_ID;
	}

	// ToDo: the B_CHECK_PERMISSION flag should be made private, as it
	//	doesn't have any use outside the kernel
	if ((flags & B_CHECK_PERMISSION) != 0
		&& sSems[slot].u.used.owner == team_get_kernel_team_id()) {
		dprintf("thread %" B_PRId32 " tried to release kernel semaphore.\n",
			thread_get_current_thread_id());
		return B_NOT_ALLOWED;
	}

	KTRACE("release_sem_etc(sem: %ld, count: %ld, flags: 0x%lx)", id, count,
		flags);

	sSems[slot].u.used.last_acquirer = -sSems[slot].u.used.last_acquirer;
#if DEBUG_SEM_LAST_ACQUIRER
	sSems[slot].u.used.last_releaser = thread_get_current_thread_id();
	sSems[slot].u.used.last_release_count = count;
#endif

	if (flags & B_RELEASE_ALL) {
		count = sSems[slot].u.used.net_count - sSems[slot].u.used.count;

		// is there anything to do for us at all?
		if (count == 0)
			return B_OK;

		// Don't release more than necessary -- there might be interrupted/
		// timed out threads in the queue.
		flags |= B_RELEASE_IF_WAITING_ONLY;
	}

	// Grab the scheduler lock, so thread_is_blocked() is reliable (due to
	// possible interruptions or timeouts, it wouldn't be otherwise).
	while (count > 0) {
		queued_thread* entry = sSems[slot].queue.Head();
		if (entry == NULL) {
			if ((flags & B_RELEASE_IF_WAITING_ONLY) == 0) {
				sSems[slot].u.used.count += count;
				sSems[slot].u.used.net_count += count;
			}
			break;
		}

		SpinLocker schedulerLock(entry->thread->scheduler_lock);
		if (thread_is_blocked(entry->thread)) {
			// The thread is still waiting. If its count is satisfied,
			// unblock it. Otherwise we can't unblock any other thread.
			if (entry->count > sSems[slot].u.used.net_count + count) {
				sSems[slot].u.used.count += count;
				sSems[slot].u.used.net_count += count;
				break;
			}

			thread_unblock_locked(entry->thread, B_OK);

			int delta = min_c(count, entry->count);
			sSems[slot].u.used.count += delta;
			sSems[slot].u.used.net_count += delta - entry->count;
			count -= delta;
		} else {
			// The thread is no longer waiting, but still queued, which
			// means acquiration failed and we can just remove it.
			sSems[slot].u.used.count += entry->count;
		}

		sSems[slot].queue.Remove(entry);
		entry->queued = false;
	}

	if (sSems[slot].u.used.count > 0)
		notify_sem_select_events(&sSems[slot], B_EVENT_ACQUIRE_SEMAPHORE);

	// If we've unblocked another thread reschedule, if we've not explicitly
	// been told not to.
	if ((flags & B_DO_NOT_RESCHEDULE) == 0) {
		semLocker.Unlock();

		SpinLocker _(thread_get_current_thread()->scheduler_lock);
		scheduler_reschedule_if_necessary_locked();
	}

	return B_OK;
}


status_t
get_sem_count(sem_id id, int32 *_count)
{
	if (sSemsActive == false)
		return B_NO_MORE_SEMS;
	if (id < 0)
		return B_BAD_SEM_ID;
	if (_count == NULL)
		return B_BAD_VALUE;

	int slot = id % sMaxSems;

	InterruptsSpinLocker _(sSems[slot].lock);

	if (sSems[slot].id != id) {
		TRACE(("sem_get_count: invalid sem_id %ld\n", id));
		return B_BAD_SEM_ID;
	}

	*_count = sSems[slot].u.used.count;

	return B_OK;
}


/*!	Called by the get_sem_info() macro. */
status_t
_get_sem_info(sem_id id, struct sem_info *info, size_t size)
{
	if (!sSemsActive)
		return B_NO_MORE_SEMS;
	if (id < 0)
		return B_BAD_SEM_ID;
	if (info == NULL || size != sizeof(sem_info))
		return B_BAD_VALUE;

	int slot = id % sMaxSems;

	InterruptsSpinLocker _(sSems[slot].lock);

	if (sSems[slot].id != id) {
		TRACE(("get_sem_info: invalid sem_id %ld\n", id));
		return B_BAD_SEM_ID;
	} else
		fill_sem_info(&sSems[slot], info, size);

	return B_OK;
}


/*!	Called by the get_next_sem_info() macro. */
status_t
_get_next_sem_info(team_id teamID, int32 *_cookie, struct sem_info *info,
	size_t size)
{
	if (!sSemsActive)
		return B_NO_MORE_SEMS;
	if (_cookie == NULL || info == NULL || size != sizeof(sem_info))
		return B_BAD_VALUE;
	if (teamID < 0)
		return B_BAD_TEAM_ID;

	Team* team = Team::Get(teamID);
	if (team == NULL)
		return B_BAD_TEAM_ID;
	BReference<Team> teamReference(team, true);

	InterruptsSpinLocker semListLocker(sSemsSpinlock);

	// TODO: find a way to iterate the list that is more reliable
	sem_entry* sem = (sem_entry*)list_get_first_item(&team->sem_list);
	int32 newIndex = *_cookie;
	int32 index = 0;
	bool found = false;

	while (!found) {
		// find the next entry to be returned
		while (sem != NULL && index < newIndex) {
			sem = (sem_entry*)list_get_next_item(&team->sem_list, sem);
			index++;
		}

		if (sem == NULL)
			return B_BAD_VALUE;

		SpinLocker _(sem->lock);

		if (sem->id != -1 && sem->u.used.owner == team->id) {
			// found one!
			fill_sem_info(sem, info, size);
			newIndex = index + 1;
			found = true;
		} else
			newIndex++;
	}

	if (!found)
		return B_BAD_VALUE;

	*_cookie = newIndex;
	return B_OK;
}


status_t
set_sem_owner(sem_id id, team_id newTeamID)
{
	if (sSemsActive == false)
		return B_NO_MORE_SEMS;
	if (id < 0)
		return B_BAD_SEM_ID;
	if (newTeamID < 0)
		return B_BAD_TEAM_ID;

	int32 slot = id % sMaxSems;

	// get the new team
	Team* newTeam = Team::Get(newTeamID);
	if (newTeam == NULL)
		return B_BAD_TEAM_ID;
	BReference<Team> newTeamReference(newTeam, true);

	InterruptsSpinLocker semListLocker(sSemsSpinlock);
	SpinLocker semLocker(sSems[slot].lock);

	if (sSems[slot].id != id) {
		TRACE(("set_sem_owner: invalid sem_id %ld\n", id));
		return B_BAD_SEM_ID;
	}

	list_remove_link(&sSems[slot].u.used.team_link);
	list_add_item(&newTeam->sem_list, &sSems[slot].u.used.team_link);

	sSems[slot].u.used.owner = newTeam->id;
	return B_OK;
}


/*!	Returns the name of the semaphore. The name is not copied, so the caller
	must make sure that the semaphore remains alive as long as the name is used.
*/
const char*
sem_get_name_unsafe(sem_id id)
{
	int slot = id % sMaxSems;

	if (sSemsActive == false || id < 0 || sSems[slot].id != id)
		return NULL;

	return sSems[slot].u.used.name;
}


//	#pragma mark - Syscalls


sem_id
_user_create_sem(int32 count, const char *userName)
{
	char name[B_OS_NAME_LENGTH];

	if (userName == NULL)
		return create_sem_etc(count, NULL, team_get_current_team_id());

	if (!IS_USER_ADDRESS(userName)
		|| user_strlcpy(name, userName, B_OS_NAME_LENGTH) < B_OK)
		return B_BAD_ADDRESS;

	return create_sem_etc(count, name, team_get_current_team_id());
}


status_t
_user_delete_sem(sem_id id)
{
	return delete_sem_internal(id, true);
}


status_t
_user_acquire_sem(sem_id id)
{
	status_t error = switch_sem_etc(-1, id, 1,
		B_CAN_INTERRUPT | B_CHECK_PERMISSION, 0);

	return syscall_restart_handle_post(error);
}


status_t
_user_acquire_sem_etc(sem_id id, int32 count, uint32 flags, bigtime_t timeout)
{
	syscall_restart_handle_timeout_pre(flags, timeout);

	status_t error = switch_sem_etc(-1, id, count,
		flags | B_CAN_INTERRUPT | B_CHECK_PERMISSION, timeout);

	return syscall_restart_handle_timeout_post(error, timeout);
}


status_t
_user_switch_sem(sem_id releaseSem, sem_id id)
{
	status_t error = switch_sem_etc(releaseSem, id, 1,
		B_CAN_INTERRUPT | B_CHECK_PERMISSION, 0);

	if (releaseSem < 0)
		return syscall_restart_handle_post(error);

	return error;
}


status_t
_user_switch_sem_etc(sem_id releaseSem, sem_id id, int32 count, uint32 flags,
	bigtime_t timeout)
{
	if (releaseSem < 0)
		syscall_restart_handle_timeout_pre(flags, timeout);

	status_t error = switch_sem_etc(releaseSem, id, count,
		flags | B_CAN_INTERRUPT | B_CHECK_PERMISSION, timeout);

	if (releaseSem < 0)
		return syscall_restart_handle_timeout_post(error, timeout);

	return error;
}


status_t
_user_release_sem(sem_id id)
{
	return release_sem_etc(id, 1, B_CHECK_PERMISSION);
}


status_t
_user_release_sem_etc(sem_id id, int32 count, uint32 flags)
{
	return release_sem_etc(id, count, flags | B_CHECK_PERMISSION);
}


status_t
_user_get_sem_count(sem_id id, int32 *userCount)
{
	status_t status;
	int32 count;

	if (userCount == NULL || !IS_USER_ADDRESS(userCount))
		return B_BAD_ADDRESS;

	status = get_sem_count(id, &count);
	if (status == B_OK && user_memcpy(userCount, &count, sizeof(int32)) < B_OK)
		return B_BAD_ADDRESS;

	return status;
}


status_t
_user_get_sem_info(sem_id id, struct sem_info *userInfo, size_t size)
{
	struct sem_info info;
	status_t status;

	if (userInfo == NULL || !IS_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;

	status = _get_sem_info(id, &info, size);
	if (status == B_OK && user_memcpy(userInfo, &info, size) < B_OK)
		return B_BAD_ADDRESS;

	return status;
}


status_t
_user_get_next_sem_info(team_id team, int32 *userCookie, struct sem_info *userInfo,
	size_t size)
{
	struct sem_info info;
	int32 cookie;
	status_t status;

	if (userCookie == NULL || userInfo == NULL
		|| !IS_USER_ADDRESS(userCookie) || !IS_USER_ADDRESS(userInfo)
		|| user_memcpy(&cookie, userCookie, sizeof(int32)) < B_OK)
		return B_BAD_ADDRESS;

	status = _get_next_sem_info(team, &cookie, &info, size);

	if (status == B_OK) {
		if (user_memcpy(userInfo, &info, size) < B_OK
			|| user_memcpy(userCookie, &cookie, sizeof(int32)) < B_OK)
			return B_BAD_ADDRESS;
	}

	return status;
}


status_t
_user_set_sem_owner(sem_id id, team_id team)
{
	return set_sem_owner(id, team);
}
