/*
** Copyright 2002-2004, The Haiku Team. All rights reserved.
** Distributed under the terms of the Haiku License.
**
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

/* Semaphore code */

#include <OS.h>

#include <sem.h>
#include <smp.h>
#include <int.h>
#include <arch/int.h>
#include <debug.h>
#include <thread.h>
#include <team.h>
#include <boot/kernel_args.h>

#include <string.h>
#include <stdlib.h>

#define TRACE_SEM 0
#if TRACE_SEM
#	define TRACE(x) dprintf x
#	define TRACE_BLOCK(x) dprintf x
#else
#	define TRACE(x) ;
#	define TRACE_BLOCK(x) ;
#endif

struct sem_entry {
	sem_id		id;
	spinlock	lock;	// protects only the id field when unused
	union {
		// when slot in use
		struct {
			int					count;
			struct thread_queue q;
			char				*name;
			team_id				owner;	// if set to -1, means owned by a port
		} used;

		// when slot unused
		struct {
			sem_id				next_id;
			struct sem_entry	*next;
		} unused;
	} u;
};

// Todo: Compute based on the amount of available memory.
static int32 sMaxSems = 4096;
static int32 sUsedSems = 0;

static struct sem_entry *gSems = NULL;
static region_id         gSemRegion = 0;
static bool              gSemsActive = false;
static struct sem_entry	*gFreeSemsHead = NULL;
static struct sem_entry	*gFreeSemsTail = NULL;

static spinlock sem_spinlock = 0;
#define GRAB_SEM_LIST_LOCK()     acquire_spinlock(&sem_spinlock)
#define RELEASE_SEM_LIST_LOCK()  release_spinlock(&sem_spinlock)
#define GRAB_SEM_LOCK(s)         acquire_spinlock(&(s).lock)
#define RELEASE_SEM_LOCK(s)      release_spinlock(&(s).lock)

static int remove_thread_from_sem(struct thread *t, struct sem_entry *sem,
				struct thread_queue *queue, status_t acquireStatus);

struct sem_timeout_args {
	thread_id blocked_thread;
	sem_id blocked_sem_id;
	int sem_count;
};


static int
dump_sem_list(int argc, char **argv)
{
	int i;

	for (i = 0; i < sMaxSems; i++) {
		if (gSems[i].id >= 0)
			dprintf("%p\tid: 0x%lx\t\tname: '%s'\n", &gSems[i], gSems[i].id,
					gSems[i].u.used.name);
	}
	return 0;
}


static void
dump_sem(struct sem_entry *sem)
{
	dprintf("SEM:     %p\n", sem);
	dprintf("id:      %ld\n", sem->id);
	if (sem->id >= 0) {
		dprintf("name:    '%s'\n", sem->u.used.name);
		dprintf("owner:   0x%lx\n", sem->u.used.owner);
		dprintf("count:   0x%x\n", sem->u.used.count);
		dprintf("queue:   head %p tail %p\n", sem->u.used.q.head,
				sem->u.used.q.tail);
	} else {
		dprintf("next:    %p\n", sem->u.unused.next);
		dprintf("next_id: %ld\n", sem->u.unused.next_id);
	}
}


static int
dump_sem_info(int argc, char **argv)
{
	int i;

	if (argc < 2) {
		dprintf("sem: not enough arguments\n");
		return 0;
	}

	// if the argument looks like a hex number, treat it as such
	if (strlen(argv[1]) > 2 && argv[1][0] == '0' && argv[1][1] == 'x') {
		unsigned long num = strtoul(argv[1], NULL, 16);

		if (num > KERNEL_BASE && num <= (KERNEL_BASE + (KERNEL_SIZE - 1))) {
			// XXX semi-hack
			dump_sem((struct sem_entry *)num);
			return 0;
		} else {
			unsigned slot = num % sMaxSems;
			if (gSems[slot].id != (int)num) {
				dprintf("sem 0x%lx doesn't exist!\n", num);
				return 0;
			}
			dump_sem(&gSems[slot]);
			return 0;
		}
	}

	// walk through the sem list, trying to match name
	for (i = 0; i < sMaxSems; i++) {
		if (gSems[i].u.used.name != NULL
			&& strcmp(argv[1], gSems[i].u.used.name) == 0) {
			dump_sem(&gSems[i]);
			return 0;
		}
	}
	return 0;
}


/**	\brief Appends a semaphore slot to the free list.
 *
 *	The semaphore list must be locked.
 *	The slot's id field is not changed. It should already be set to -1.
 *
 *	\param slot The index of the semaphore slot.
 *	\param nextID The ID the slot will get when reused. If < 0 the \a slot
 *		   is used.
 */

static void
free_sem_slot(int slot, sem_id nextID)
{
	struct sem_entry *sem = gSems + slot;
	// set next_id to the next possible value; for sanity check the current ID
	if (nextID < 0)
		sem->u.unused.next_id = slot;
	else
		sem->u.unused.next_id = nextID;
	// append the entry to the list
	if (gFreeSemsTail)
		gFreeSemsTail->u.unused.next = sem;
	else
		gFreeSemsHead = sem;
	gFreeSemsTail = sem;
	sem->u.unused.next = NULL;
}


status_t
sem_init(kernel_args *ka)
{
	int i;

	TRACE(("sem_init: entry\n"));

	// create and initialize semaphore table
	gSemRegion = create_area("sem_table", (void **)&gSems, B_ANY_KERNEL_ADDRESS,
		sizeof(struct sem_entry) * sMaxSems, B_FULL_LOCK, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (gSemRegion < 0)
		panic("unable to allocate semaphore table!\n");

	memset(gSems, 0, sizeof(struct sem_entry) * sMaxSems);
	for (i = 0; i < sMaxSems; i++) {
		gSems[i].id = -1;
		free_sem_slot(i, i);
	}

	// add debugger commands
	add_debugger_command("sems", &dump_sem_list, "Dump a list of all active semaphores");
	add_debugger_command("sem", &dump_sem_info, "Dump info about a particular semaphore");

	TRACE(("sem_init: exit\n"));

	gSemsActive = true;

	return 0;
}


/**	Creates a semaphore with the given parameters.
 *	Note, the team_id is not checked, it must be correct, or else
 *	that semaphore might not be deleted.
 *	This function is only available from within the kernel, and
 *	should not be made public - if possible, we should remove it
 *	completely (and have only create_sem() exported).
 */

sem_id
create_sem_etc(int32 count, const char *name, team_id owner)
{
	struct sem_entry *sem = NULL;
	cpu_status state;
	sem_id id = B_NO_MORE_SEMS;
	char *tempName;
	size_t nameLength;

	if (gSemsActive == false || sUsedSems == sMaxSems)
		return B_NO_MORE_SEMS;

	if (name == NULL)
		name = "unnamed semaphore";

	nameLength = strlen(name) + 1;
	nameLength = min(nameLength, B_OS_NAME_LENGTH);
	tempName = (char *)malloc(nameLength);
	if (tempName == NULL)
		return B_NO_MEMORY;
	strlcpy(tempName, name, nameLength);

	state = disable_interrupts();
	GRAB_SEM_LIST_LOCK();

	// get the first slot from the free list
	sem = gFreeSemsHead;
	if (sem) {
		// remove it from the free list
		gFreeSemsHead = sem->u.unused.next;
		if (!gFreeSemsHead)
			gFreeSemsTail = NULL;
		// init the slot
		GRAB_SEM_LOCK(*sem);
		sem->id = sem->u.unused.next_id;
		sem->u.used.count = count;
		sem->u.used.q.tail = NULL;
		sem->u.used.q.head = NULL;
		sem->u.used.name = tempName;
		sem->u.used.owner = owner;
		id = sem->id;
		RELEASE_SEM_LOCK(*sem);

		atomic_add(&sUsedSems, 1);
	}

	RELEASE_SEM_LIST_LOCK();
	restore_interrupts(state);

	if (!sem)
		free(tempName);

	return id;
}


sem_id
create_sem(int32 count, const char *name)
{
	return create_sem_etc(count, name, team_get_kernel_team_id());
}


status_t
delete_sem(sem_id id)
{
	int slot;
	int state;
	struct thread *t;
	int released_threads;
	char *old_name;
	struct thread_queue release_queue;

	if (gSemsActive == false)
		return B_NO_MORE_SEMS;
	if (id < 0)
		return B_BAD_SEM_ID;

	slot = id % sMaxSems;

	state = disable_interrupts();
	GRAB_SEM_LOCK(gSems[slot]);

	if (gSems[slot].id != id) {
		RELEASE_SEM_LOCK(gSems[slot]);
		restore_interrupts(state);
		TRACE(("delete_sem: invalid sem_id %ld\n", id));
		return B_BAD_SEM_ID;
	}

	released_threads = 0;
	release_queue.head = release_queue.tail = NULL;

	// free any threads waiting for this semaphore
	while ((t = thread_dequeue(&gSems[slot].u.used.q)) != NULL) {
		t->state = B_THREAD_READY;
		t->sem.acquire_status = B_BAD_SEM_ID;
		t->sem.count = 0;
		thread_enqueue(t, &release_queue);
		released_threads++;
	}

	gSems[slot].id = -1;
	old_name = gSems[slot].u.used.name;
	gSems[slot].u.used.name = NULL;

	RELEASE_SEM_LOCK(gSems[slot]);

	// append slot to the free list
	GRAB_SEM_LIST_LOCK();
	free_sem_slot(slot, id + sMaxSems);
	atomic_add(&sUsedSems, -1);
	RELEASE_SEM_LIST_LOCK();

	if (released_threads > 0) {
		GRAB_THREAD_LOCK();
		while ((t = thread_dequeue(&release_queue)) != NULL) {
			scheduler_enqueue_in_run_queue(t);
		}
		scheduler_reschedule();
		RELEASE_THREAD_LOCK();
	}

	restore_interrupts(state);

	free(old_name);

	return B_OK;
}


/** Called from a timer handler. Wakes up a semaphore */

static int32
sem_timeout(timer *data)
{
	struct sem_timeout_args *args = (struct sem_timeout_args *)data->entry.prev;
	struct thread *t;
	int slot;
	int state;
	struct thread_queue wakeup_queue;

	t = thread_get_thread_struct(args->blocked_thread);
	if (t == NULL)
		return B_HANDLED_INTERRUPT;
	slot = args->blocked_sem_id % sMaxSems;

	state = disable_interrupts();
	GRAB_SEM_LOCK(gSems[slot]);

	TRACE(("sem_timeout: called on 0x%x sem %ld, tid %ld\n", data, args->blocked_sem_id, args->blocked_thread));

	if (gSems[slot].id != args->blocked_sem_id) {
		// this thread was not waiting on this semaphore
		panic("sem_timeout: thid %ld was trying to wait on sem %ld which doesn't exist!\n",
			args->blocked_thread, args->blocked_sem_id);
	}

	wakeup_queue.head = wakeup_queue.tail = NULL;
	remove_thread_from_sem(t, &gSems[slot], &wakeup_queue, B_TIMED_OUT);
	
	RELEASE_SEM_LOCK(gSems[slot]);

	GRAB_THREAD_LOCK();
	// put the threads in the run q here to make sure we dont deadlock in sem_interrupt_thread
	while ((t = thread_dequeue(&wakeup_queue)) != NULL) {
		scheduler_enqueue_in_run_queue(t);
	}
	RELEASE_THREAD_LOCK();

	restore_interrupts(state);

	return B_INVOKE_SCHEDULER;
}


status_t
acquire_sem(sem_id id)
{
	return acquire_sem_etc(id, 1, 0, 0);
}


status_t
acquire_sem_etc(sem_id id, int32 count, uint32 flags, bigtime_t timeout)
{
	int slot = id % sMaxSems;
	int state;
	status_t status = B_OK;
	
	if (gSemsActive == false)
		return B_NO_MORE_SEMS;

	if (!kernel_startup && !are_interrupts_enabled())
		panic("acquire_sem_etc: called with interrupts disabled for sem %#lx\n", id);

	if (id < 0)
		return B_BAD_SEM_ID;
	if (count <= 0)
		return B_BAD_VALUE;

	state = disable_interrupts();
	GRAB_SEM_LOCK(gSems[slot]);
	
	if (gSems[slot].id != id) {
		TRACE(("acquire_sem_etc: bad sem_id %ld\n", id));
		status = B_BAD_SEM_ID;
		goto err;
	}

	if (gSems[slot].u.used.count - count < 0 && (flags & B_TIMEOUT) != 0
		&& timeout <= 0) {
		// immediate timeout
		status = B_WOULD_BLOCK;
		goto err;
	}

	if ((gSems[slot].u.used.count -= count) < 0) {
		// we need to block
		struct thread *t = thread_get_current_thread();
		timer timeout_timer; // stick it on the stack, since we may be blocking here
		struct sem_timeout_args args;

		TRACE_BLOCK(("acquire_sem_etc(id = %ld): block name = %s, thread = %p,"
					 " name = %s\n", id, gSems[slot].u.used.name, t, t->name));

		// do a quick check to see if the thread has any pending signals
		// this should catch most of the cases where the thread had a signal
		if ((flags & B_CAN_INTERRUPT) && t->sig_pending) {
			gSems[slot].u.used.count += count;
			status = B_INTERRUPTED;
			goto err;
		}

		t->next_state = B_THREAD_WAITING;
		t->sem.flags = flags;
		t->sem.blocking = id;
		t->sem.acquire_count = count;
		t->sem.count = min(-gSems[slot].u.used.count, count);
			// store the count we need to restore upon release
		t->sem.acquire_status = B_NO_ERROR;
		thread_enqueue(t, &gSems[slot].u.used.q);

		if ((flags & (B_TIMEOUT | B_ABSOLUTE_TIMEOUT)) != 0) {
			TRACE(("sem_acquire_etc: setting timeout sem for %Ld usecs, semid %d, tid %d\n",
				timeout, id, t->id));

			// set up an event to go off with the thread struct as the data
			args.blocked_sem_id = id;
			args.blocked_thread = t->id;
			args.sem_count = count;
			
			// another evil hack: pass the args into timer->entry.prev
			timeout_timer.entry.prev = (qent *)&args;
			add_timer(&timeout_timer, &sem_timeout, timeout,
				flags & B_RELATIVE_TIMEOUT ?
					B_ONE_SHOT_RELATIVE_TIMER : B_ONE_SHOT_ABSOLUTE_TIMER);			
		}

		RELEASE_SEM_LOCK(gSems[slot]);
		GRAB_THREAD_LOCK();
		// check again to see if a signal is pending.
		// it may have been delivered while setting up the sem, though it's pretty unlikely
		if ((flags & B_CAN_INTERRUPT) && t->sig_pending) {
			struct thread_queue wakeup_queue;
			// ok, so a tiny race happened where a signal was delivered to this thread while
			// it was setting up the sem. We can only be sure a signal wasn't delivered
			// here, since the threadlock is held. The previous check would have found most
			// instances, but there was a race, so we have to handle it. It'll be more messy...
			wakeup_queue.head = wakeup_queue.tail = NULL;
			GRAB_SEM_LOCK(gSems[slot]);
			if (gSems[slot].id == id) {
				remove_thread_from_sem(t, &gSems[slot], &wakeup_queue, EINTR);
			}
			RELEASE_SEM_LOCK(gSems[slot]);
			while ((t = thread_dequeue(&wakeup_queue)) != NULL) {
				scheduler_enqueue_in_run_queue(t);
			}
			// fall through and reschedule since another thread with a higher priority may have been woken up
		}
		scheduler_reschedule();
		RELEASE_THREAD_LOCK();

		if ((flags & (B_TIMEOUT | B_ABSOLUTE_TIMEOUT)) != 0) {
			if (t->sem.acquire_status != B_TIMED_OUT) {
				// cancel the timer event, the sem may have been deleted or interrupted
				// with the timer still active
				cancel_timer(&timeout_timer);
			}
		}

		restore_interrupts(state);

		TRACE_BLOCK(("acquire_sem_etc(id = %ld): exit block name = %s, "
					 "thread = %p (%s)\n", id, gSems[slot].u.used.name, t,
					 t->name));
		return t->sem.acquire_status;
	}

err:
	RELEASE_SEM_LOCK(gSems[slot]);
	restore_interrupts(state);

	return status;
}


status_t
release_sem(sem_id id)
{
	return release_sem_etc(id, 1, 0);
}


status_t
release_sem_etc(sem_id id, int32 count, uint32 flags)
{
	struct thread_queue releaseQueue;
	int32 slot = id % sMaxSems;
	cpu_status state;
	status_t status = B_OK;

	if (gSemsActive == false)
		return B_NO_MORE_SEMS;
	if (id < 0)
		return B_BAD_SEM_ID;
	if (count <= 0 && (flags & B_RELEASE_ALL) == 0)
		return B_BAD_VALUE;

	state = disable_interrupts();
	GRAB_SEM_LOCK(gSems[slot]);

	if (gSems[slot].id != id) {
		TRACE(("sem_release_etc: invalid sem_id %ld\n", id));
		status = B_BAD_SEM_ID;
		goto err;
	}

	// clear out a queue we will use to hold all of the threads that we will have to
	// put back into the run list. This is done so the thread lock wont be held
	// while this sems lock is held since the two locks are grabbed in the other
	// order in sem_interrupt_thread.
	releaseQueue.head = releaseQueue.tail = NULL;

	if (flags & B_RELEASE_ALL) {
		count = -gSems[slot].u.used.count;

		// is there anything to do for us at all?
		if (count == 0)
			goto err;
	}

	while (count > 0) {
		int delta = count;
		if (gSems[slot].u.used.count < 0) {
			struct thread *thread = thread_lookat_queue(&gSems[slot].u.used.q);

			delta = min(count, thread->sem.count);
			thread->sem.count -= delta;
			if (thread->sem.count <= 0) {
				// release this thread
				thread = thread_dequeue(&gSems[slot].u.used.q);
				thread_enqueue(thread, &releaseQueue);
				thread->state = B_THREAD_READY;
				thread->sem.count = 0;
			}
		} else if (flags & B_RELEASE_IF_WAITING_ONLY)
			break;

		gSems[slot].u.used.count += delta;
		count -= delta;
	}
	RELEASE_SEM_LOCK(gSems[slot]);

	// pull off any items in the release queue and put them in the run queue
	if (releaseQueue.head != NULL) {
		struct thread *thread;
		int32 priority;

		GRAB_THREAD_LOCK();
		while ((thread = thread_dequeue(&releaseQueue)) != NULL) {
			// temporarily place thread in a run queue with high priority to boost it up
			priority = thread->priority;
			thread->priority = thread->priority >= B_FIRST_REAL_TIME_PRIORITY ?
				thread->priority : B_FIRST_REAL_TIME_PRIORITY;
			scheduler_enqueue_in_run_queue(thread);
			thread->priority = priority;
		}
		if ((flags & B_DO_NOT_RESCHEDULE) == 0)
			scheduler_reschedule();

		RELEASE_THREAD_LOCK();
	}
	goto outnolock;

err:
	RELEASE_SEM_LOCK(gSems[slot]);
outnolock:
	restore_interrupts(state);

	return status;
}


status_t
get_sem_count(sem_id id, int32 *thread_count)
{
	int slot;
	int state;

	if (gSemsActive == false)
		return B_NO_MORE_SEMS;
	if (id < 0)
		return B_BAD_SEM_ID;
	if (thread_count == NULL)
		return EINVAL;

	slot = id % sMaxSems;

	state = disable_interrupts();
	GRAB_SEM_LOCK(gSems[slot]);

	if (gSems[slot].id != id) {
		RELEASE_SEM_LOCK(gSems[slot]);
		restore_interrupts(state);
		TRACE(("sem_get_count: invalid sem_id %ld\n", id));
		return B_BAD_SEM_ID;
	}

	*thread_count = gSems[slot].u.used.count;

	RELEASE_SEM_LOCK(gSems[slot]);
	restore_interrupts(state);

	return B_NO_ERROR;
}


/** Fills the thread_info structure with information from the specified
 *	thread.
 *	The thread lock must be held when called.
 */

static void
fill_sem_info(struct sem_entry *sem, sem_info *info, size_t size)
{
	info->sem = sem->id;
	info->team = sem->u.used.owner;
	strlcpy(info->name, sem->u.used.name, sizeof(info->name));
	info->count = sem->u.used.count;
	info->latest_holder	= sem->u.used.q.head->id;
		// ToDo: not sure if this is the latest holder, or the next
		// holder...
}


/** The underscore is needed for binary compatibility with BeOS.
 *	OS.h contains the following macro:
 *	#define get_sem_info(sem, info)                \
 *            _get_sem_info((sem), (info), sizeof(*(info)))
 */

status_t
_get_sem_info(sem_id id, struct sem_info *info, size_t size)
{
	status_t status = B_OK;
	int state;
	int slot;

	if (!gSemsActive)
		return B_NO_MORE_SEMS;
	if (id < 0)
		return B_BAD_SEM_ID;
	if (info == NULL || size != sizeof(sem_info))
		return B_BAD_VALUE;

	slot = id % sMaxSems;

	state = disable_interrupts();
	GRAB_SEM_LOCK(gSems[slot]);

	if (gSems[slot].id != id) {
		status = B_BAD_SEM_ID;
		TRACE(("get_sem_info: invalid sem_id %ld\n", id));
	} else
		fill_sem_info(&gSems[slot], info, size);

	RELEASE_SEM_LOCK(gSems[slot]);
	restore_interrupts(state);

	return status;
}


/** The underscore is needed for binary compatibility with BeOS.
 *	OS.h contains the following macro:
 *	#define get_next_sem_info(team, cookie, info)  \
 *           _get_next_sem_info((team), (cookie), (info), sizeof(*(info)))
 */

status_t
_get_next_sem_info(team_id team, int32 *_cookie, struct sem_info *info, size_t size)
{
	int state;
	int slot;
	bool found = false;

	if (!gSemsActive)
		return B_NO_MORE_SEMS;
	if (_cookie == NULL || info == NULL || size != sizeof(sem_info))
		return B_BAD_VALUE;

	if (team == B_CURRENT_TEAM)
		team = team_get_current_team_id();
	/* prevents gSems[].owner == -1 >= means owned by a port */
	if (team < 0 || !team_is_valid(team))
		return B_BAD_TEAM_ID; 

	slot = *_cookie;
	if (slot >= sMaxSems)
		return B_BAD_VALUE;

	state = disable_interrupts();
	GRAB_SEM_LIST_LOCK();

	while (slot < sMaxSems) {
		if (gSems[slot].id != -1 && gSems[slot].u.used.owner == team) {
			GRAB_SEM_LOCK(gSems[slot]);
			if (gSems[slot].id != -1 && gSems[slot].u.used.owner == team) {
				// found one!
				fill_sem_info(&gSems[slot], info, size);

				RELEASE_SEM_LOCK(gSems[slot]);
				slot++;
				found = true;
				break;
			}
			RELEASE_SEM_LOCK(gSems[slot]);
		}
		slot++;
	}
	RELEASE_SEM_LIST_LOCK();
	restore_interrupts(state);

	if (!found)
		return B_BAD_VALUE;

	*_cookie = slot;
	return B_OK;
}


status_t
set_sem_owner(sem_id id, team_id team)
{
	int state;
	int slot;

	if (gSemsActive == false)
		return B_NO_MORE_SEMS;
	if (id < 0)
		return B_BAD_SEM_ID;
	if (team < 0 || !team_is_valid(team))
		return B_BAD_TEAM_ID;

	slot = id % sMaxSems;

	state = disable_interrupts();
	GRAB_SEM_LOCK(gSems[slot]);

	if (gSems[slot].id != id) {
		RELEASE_SEM_LOCK(gSems[slot]);
		restore_interrupts(state);
		TRACE(("set_sem_owner: invalid sem_id %ld\n", id));
		return B_BAD_SEM_ID;
	}

	// ToDo: this is a small race condition: the team ID could already
	// be invalid at this point - we would lose one semaphore slot in
	// this case!
	// The only safe way to do this is to prevent either team (the new
	// or the old owner) from dying until we leave the spinlock.
	gSems[slot].u.used.owner = team;

	RELEASE_SEM_LOCK(gSems[slot]);
	restore_interrupts(state);

	return B_NO_ERROR;
}


/** Wake up a thread that's blocked on a semaphore
 *	this function must be entered with interrupts disabled and THREADLOCK held
 */

status_t
sem_interrupt_thread(struct thread *t)
{
	int slot;
	struct thread_queue wakeup_queue;

	TRACE(("sem_interrupt_thread: called on thread %p (%d), blocked on sem 0x%x\n",
		t, t->id, t->sem.blocking));

	if (t->state != B_THREAD_WAITING || t->sem.blocking < 0)
		return B_BAD_VALUE;
	if (!(t->sem.flags & B_CAN_INTERRUPT))
		return B_NOT_ALLOWED;

	slot = t->sem.blocking % sMaxSems;

	GRAB_SEM_LOCK(gSems[slot]);

	if (gSems[slot].id != t->sem.blocking) {
		panic("sem_interrupt_thread: thread 0x%lx sez it's blocking on sem 0x%lx, but that sem doesn't exist!\n", t->id, t->sem.blocking);
	}

	wakeup_queue.head = wakeup_queue.tail = NULL;
	if (remove_thread_from_sem(t, &gSems[slot], &wakeup_queue, EINTR) != B_OK)
		panic("sem_interrupt_thread: thread 0x%lx not found in sem 0x%lx's wait queue\n", t->id, t->sem.blocking);

	RELEASE_SEM_LOCK(gSems[slot]);

	while ((t = thread_dequeue(&wakeup_queue)) != NULL) {
		scheduler_enqueue_in_run_queue(t);
	}

	return B_NO_ERROR;
}


/** forcibly removes a thread from a semaphores wait q. May have to wake up other threads in the
 *	process. All threads that need to be woken up are added to the passed in thread_queue.
 *	must be called with sem lock held
 */

static int
remove_thread_from_sem(struct thread *t, struct sem_entry *sem, struct thread_queue *queue,
	status_t acquireStatus)
{
	struct thread *t1;

	// remove the thread from the queue and place it in the supplied queue
	t1 = thread_dequeue_id(&sem->u.used.q, t->id);
	if (t != t1)
		return B_ENTRY_NOT_FOUND;

	sem->u.used.count += t->sem.acquire_count;
	t->state = t->next_state = B_THREAD_READY;
	t->sem.acquire_status = acquireStatus;
	thread_enqueue(t, queue);

	// now see if more threads need to be woken up
	while (sem->u.used.count > 0
		   && (t1 = thread_lookat_queue(&sem->u.used.q))) {
		int delta = min(t->sem.count, sem->u.used.count);

		t->sem.count -= delta;
		if (t->sem.count <= 0) {
			t = thread_dequeue(&sem->u.used.q);
			t->state = t->next_state = B_THREAD_READY;
			thread_enqueue(t, queue);
		}
		sem->u.used.count -= delta;
	}
	return B_NO_ERROR;
}


/** this function cycles through the sem table, deleting all the sems that are owned by
 *	the passed team_id
 */

int
sem_delete_owned_sems(team_id owner)
{
	int state;
	int i;
	int count = 0;

	if (owner < 0)
		return B_BAD_TEAM_ID;

	state = disable_interrupts();
	GRAB_SEM_LIST_LOCK();

	for (i = 0; i < sMaxSems; i++) {
		if (gSems[i].id != -1 && gSems[i].u.used.owner == owner) {
			sem_id id = gSems[i].id;

			RELEASE_SEM_LIST_LOCK();
			restore_interrupts(state);

			delete_sem(id);
			count++;

			state = disable_interrupts();
			GRAB_SEM_LIST_LOCK();
		}
	}

	RELEASE_SEM_LIST_LOCK();
	restore_interrupts(state);

	return count;
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


//	#pragma mark -


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
	return delete_sem(id);
}


status_t
_user_acquire_sem(sem_id id)
{
	return _user_acquire_sem_etc(id, 1, 0, 0);
}


status_t
_user_acquire_sem_etc(sem_id id, int32 count, uint32 flags, bigtime_t timeout)
{
	return acquire_sem_etc(id, count, flags | B_CAN_INTERRUPT, timeout);
}


status_t
_user_release_sem(sem_id id)
{
	return release_sem_etc(id, 1, 0);
}


status_t
_user_release_sem_etc(sem_id id, int32 count, uint32 flags)
{
	return release_sem_etc(id, count, flags);
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
