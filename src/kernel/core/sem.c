/* Semaphore code. Lots of "todo" items*/

/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel.h>
#include <OS.h>
#include <sem.h>
#include <smp.h>
#include <int.h>
#include <arch/int.h>
#include <timer.h>
#include <debug.h>
#include <memheap.h>
#include <thread.h>
#include <Errors.h>
#include <kerrors.h>

#include <stage2.h>

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
	int			count;
	struct thread_queue q;
	char		*name;
	spinlock	lock;
	team_id		owner;		 // if set to -1, means owned by a port
};

#define MAX_SEMS 4096

static struct sem_entry *gSems = NULL;
static region_id         gSemRegion = 0;
static bool              gSemsActive = false;
static sem_id            gNextSemID = 0;

static spinlock sem_spinlock = 0;
#define GRAB_SEM_LIST_LOCK()     acquire_spinlock(&sem_spinlock)
#define RELEASE_SEM_LIST_LOCK()  release_spinlock(&sem_spinlock)
#define GRAB_SEM_LOCK(s)         acquire_spinlock(&(s).lock)
#define RELEASE_SEM_LOCK(s)      release_spinlock(&(s).lock)

// used in functions that may put a bunch of threads in the run q at once
#define READY_THREAD_CACHE_SIZE 16

static int remove_thread_from_sem(struct thread *t, struct sem_entry *sem, struct thread_queue *queue, int sem_errcode);

struct sem_timeout_args {
	thread_id blocked_thread;
	sem_id blocked_sem_id;
	int sem_count;
};


static int
dump_sem_list(int argc, char **argv)
{
	int i;

	for (i = 0; i < MAX_SEMS; i++) {
		if (gSems[i].id >= 0)
			dprintf("%p\tid: 0x%lx\t\tname: '%s'\n", &gSems[i], gSems[i].id, gSems[i].name);
	}
	return 0;
}


static void
dump_sem(struct sem_entry *sem)
{
	dprintf("SEM:   %p\n", sem);
	dprintf("name:  '%s'\n", sem->name);
	dprintf("owner: 0x%lx\n", sem->owner);
	dprintf("count: 0x%x\n", sem->count);
	dprintf("queue: head %p tail %p\n", sem->q.head, sem->q.tail);
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
		unsigned long num = atoul(argv[1]);

		if (num > KERNEL_BASE && num <= (KERNEL_BASE + (KERNEL_SIZE - 1))) {
			// XXX semi-hack
			dump_sem((struct sem_entry *)num);
			return 0;
		} else {
			unsigned slot = num % MAX_SEMS;
			if (gSems[slot].id != (int)num) {
				dprintf("sem 0x%lx doesn't exist!\n", num);
				return 0;
			}
			dump_sem(&gSems[slot]);
			return 0;
		}
	}

	// walk through the sem list, trying to match name
	for (i = 0; i < MAX_SEMS; i++) {
		if (gSems[i].name != NULL
			&& strcmp(argv[1], gSems[i].name) == 0) {
			dump_sem(&gSems[i]);
			return 0;
		}
	}
	return 0;
}


status_t
sem_init(kernel_args *ka)
{
	int i;

	TRACE(("sem_init: entry\n"));

	// create and initialize semaphore table
	gSemRegion = vm_create_anonymous_region(vm_get_kernel_aspace_id(), "sem_table",
		(void **)&gSems, REGION_ADDR_ANY_ADDRESS, sizeof(struct sem_entry) * MAX_SEMS,
		REGION_WIRING_WIRED, LOCK_RW | LOCK_KERNEL);
	if (gSemRegion < 0)
		panic("unable to allocate semaphore table!\n");

	memset(gSems, 0, sizeof(struct sem_entry) * MAX_SEMS);
	for (i = 0; i < MAX_SEMS; i++)
		gSems[i].id = -1;

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
	int i;
	int state;
	sem_id retval = B_NO_MORE_SEMS;
	char *temp_name;
	int name_len;

	if (gSemsActive == false)
		return B_NO_MORE_SEMS;

	if (name == NULL)
		name = "unnamed semaphore";

	name_len = strlen(name) + 1;
	name_len = min(name_len, SYS_MAX_OS_NAME_LEN);
	temp_name = (char *)kmalloc(name_len);
	if (temp_name == NULL)
		return B_NO_MEMORY;
	strlcpy(temp_name, name, name_len);

	state = disable_interrupts();
	GRAB_SEM_LIST_LOCK();

	// find the first empty spot
	for (i = 0; i < MAX_SEMS; i++) {
		if (gSems[i].id == -1) {
			// make the sem id be a multiple of the slot it's in
			if (i >= gNextSemID % MAX_SEMS)
				gNextSemID += i - gNextSemID % MAX_SEMS;
			else
				gNextSemID += MAX_SEMS - (gNextSemID % MAX_SEMS - i);
			gSems[i].id = gNextSemID++;

			// Set the owner while the sem list lock is hold, or else it
			// might get lost if we have a sem_delete_owned_sems() running
			// (although this should be impossible (if create_sem_etc() is
			// just properly, I just feel better with it).
			gSems[i].owner = owner;

			gSems[i].lock = 0;
			GRAB_SEM_LOCK(gSems[i]);
			RELEASE_SEM_LIST_LOCK();

			gSems[i].q.tail = NULL;
			gSems[i].q.head = NULL;
			gSems[i].count = count;
			gSems[i].name = temp_name;
			retval = gSems[i].id;

			RELEASE_SEM_LOCK(gSems[i]);
			goto out;
		}
	}

	RELEASE_SEM_LIST_LOCK();
	kfree(temp_name);

out:
	restore_interrupts(state);

	return retval;
}


sem_id
create_sem(int32 count, const char *name)
{
	return create_sem_etc(count, name, team_get_kernel_team_id());
}


status_t
delete_sem(sem_id id)
{
	return delete_sem_etc(id, 0, false);
}


status_t
delete_sem_etc(sem_id id, status_t return_code, bool interrupted)
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

	slot = id % MAX_SEMS;

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
	while ((t = thread_dequeue(&gSems[slot].q)) != NULL) {
		t->state = B_THREAD_READY;
		t->sem_errcode = interrupted ? B_INTERRUPTED : B_BAD_SEM_ID;
		t->sem_deleted_retcode = return_code;
		t->sem_count = 0;
		thread_enqueue(t, &release_queue);
		released_threads++;
	}

	gSems[slot].id = -1;
	old_name = gSems[slot].name;
	gSems[slot].name = NULL;

	RELEASE_SEM_LOCK(gSems[slot]);

	if (released_threads > 0) {
		GRAB_THREAD_LOCK();
		while ((t = thread_dequeue(&release_queue)) != NULL) {
			thread_enqueue_run_q(t);
		}
		resched();
		RELEASE_THREAD_LOCK();
	}

	restore_interrupts(state);

	kfree(old_name);

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
	slot = args->blocked_sem_id % MAX_SEMS;

	state = disable_interrupts();
	GRAB_SEM_LOCK(gSems[slot]);

	TRACE(("sem_timeout: called on 0x%x sem %d, tid %d\n", to, to->sem_id, to->thread_id));

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
		thread_enqueue_run_q(t);
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
	int slot = id % MAX_SEMS;
	int state;
	status_t status = B_OK;

	if (gSemsActive == false)
		return B_NO_MORE_SEMS;
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

	if (gSems[slot].count - count < 0 && (flags & B_TIMEOUT) != 0 && timeout <= 0) {
		// immediate timeout
		status = B_TIMED_OUT;
		goto err;
	}

	if ((gSems[slot].count -= count) < 0) {
		// we need to block
		struct thread *t = thread_get_current_thread();
		timer timeout_timer; // stick it on the stack, since we may be blocking here
		struct sem_timeout_args args;

		TRACE_BLOCK(("acquire_sem_etc(id = %ld): block name = %s, thread = %p, name = %s\n", id, gSems[slot].name, t, t->name));

		// do a quick check to see if the thread has any pending signals
		// this should catch most of the cases where the thread had a signal
		if ((flags & B_CAN_INTERRUPT) && t->sig_pending) {
			gSems[slot].count += count;
			status = B_INTERRUPTED;
			goto err;
		}

		t->next_state = B_THREAD_WAITING;
		t->sem_flags = flags;
		t->sem_blocking = id;
		t->sem_acquire_count = count;
		t->sem_count = min(-gSems[slot].count, count); // store the count we need to restore upon release
		t->sem_deleted_retcode = 0;
		t->sem_errcode = B_NO_ERROR;
		thread_enqueue(t, &gSems[slot].q);

		if ((flags & (B_TIMEOUT | B_ABSOLUTE_TIMEOUT)) != 0) {
			TRACE(("sem_acquire_etc: setting timeout sem for %d %d usecs, semid %d, tid %d\n",
				timeout, sem_id, t->id));

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
				thread_enqueue_run_q(t);
			}
			// fall through and reschedule since another thread with a higher priority may have been woken up
		}
		resched();
		RELEASE_THREAD_LOCK();

		if ((flags & (B_TIMEOUT | B_ABSOLUTE_TIMEOUT)) != 0) {
			if (t->sem_errcode != B_TIMED_OUT) {
				// cancel the timer event, the sem may have been deleted or interrupted
				// with the timer still active
				cancel_timer(&timeout_timer);
			}
		}

		restore_interrupts(state);

		TRACE_BLOCK(("acquire_sem_etc(id = %ld): exit block name = %s, thread = %p (%s)\n", id, gSems[slot].name, t, t->name));
		return t->sem_errcode;
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
	int slot = id % MAX_SEMS;
	int state;
	int released_threads = 0;
	struct thread_queue release_queue;
	status_t status = B_OK;

	if (gSemsActive == false)
		return B_NO_MORE_SEMS;
	if (id < 0)
		return B_BAD_SEM_ID;
	if (count <= 0)
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
	release_queue.head = release_queue.tail = NULL;

	while (count > 0) {
		int delta = count;
		if (gSems[slot].count < 0) {
			struct thread *t = thread_lookat_queue(&gSems[slot].q);

			delta = min(count, t->sem_count);
			t->sem_count -= delta;
			if (t->sem_count <= 0) {
				// release this thread
				t = thread_dequeue(&gSems[slot].q);
				thread_enqueue(t, &release_queue);
				t->state = B_THREAD_READY;
				released_threads++;
				t->sem_count = 0;
				t->sem_deleted_retcode = 0;
			}
		}

		gSems[slot].count += delta;
		count -= delta;
	}
	RELEASE_SEM_LOCK(gSems[slot]);

	// pull off any items in the release queue and put them in the run queue
	if (released_threads > 0) {
		struct thread *t;
		int priority;
		GRAB_THREAD_LOCK();
		while ((t = thread_dequeue(&release_queue)) != NULL) {
			// temporarily place thread in a run queue with high priority to boost it up
			priority = t->priority;
			t->priority = t->priority >= B_FIRST_REAL_TIME_PRIORITY ? t->priority : B_FIRST_REAL_TIME_PRIORITY;
			thread_enqueue_run_q(t);
			t->priority = priority;
		}
		if ((flags & B_DO_NOT_RESCHEDULE) == 0) {
			resched();
		}
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

	slot = id % MAX_SEMS;

	state = disable_interrupts();
	GRAB_SEM_LOCK(gSems[slot]);

	if (gSems[slot].id != id) {
		RELEASE_SEM_LOCK(gSems[slot]);
		restore_interrupts(state);
		TRACE(("sem_get_count: invalid sem_id %ld\n", id));
		return B_BAD_SEM_ID;
	}

	*thread_count = gSems[slot].count;

	RELEASE_SEM_LOCK(gSems[slot]);
	restore_interrupts(state);

	return B_NO_ERROR;
}


/** The underscore is needed for binary compatibility with BeOS.
 *	OS.h contains the following macro:
 *	#define get_sem_info(sem, info)                \
 *            _get_sem_info((sem), (info), sizeof(*(info)))
 */

status_t
_get_sem_info(sem_id id, struct sem_info *info, size_t sz)
{
	int state;
	int slot;

	if (gSemsActive == false)
		return B_NO_MORE_SEMS;
	if (id < 0)
		return B_BAD_SEM_ID;
	if (info == NULL)
		return EINVAL;

	slot = id % MAX_SEMS;

	state = disable_interrupts();
	GRAB_SEM_LOCK(gSems[slot]);

	if (gSems[slot].id != id) {
		RELEASE_SEM_LOCK(gSems[slot]);
		restore_interrupts(state);
		TRACE(("get_sem_info: invalid sem_id %ld\n", id));
		return B_BAD_SEM_ID;
	}

	info->sem			= gSems[slot].id;
	info->team			= gSems[slot].owner;
	strncpy(info->name, gSems[slot].name, SYS_MAX_OS_NAME_LEN-1);
	info->count			= gSems[slot].count;
	info->latest_holder	= gSems[slot].q.head->id; // XXX not sure if this is correct

	RELEASE_SEM_LOCK(gSems[slot]);
	restore_interrupts(state);

	return B_NO_ERROR;
}


/** The underscore is needed for binary compatibility with BeOS.
 *	OS.h contains the following macro:
 *	#define get_next_sem_info(team, cookie, info)  \
 *           _get_next_sem_info((team), (cookie), (info), sizeof(*(info)))
 */

status_t
_get_next_sem_info(team_id team, int32 *cookie, struct sem_info *info, size_t sz)
{
	int state;
	int slot;

	if (gSemsActive == false)
		return B_NO_MORE_SEMS;
	if (cookie == NULL)
		return EINVAL;
	/* prevents gSems[].owner == -1 >= means owned by a port */
	if (team < 0)
		return B_BAD_TEAM_ID; 

	if (*cookie == 0) {
		// return first found
		slot = 0;
	}
	else {
		// start at index cookie, but check cookie against MAX_PORTS
		slot = *cookie;
		if (slot >= MAX_SEMS)
			return B_BAD_SEM_ID;
	}
	// spinlock
	state = disable_interrupts();
	GRAB_SEM_LIST_LOCK();

	while (slot < MAX_SEMS) {
		if (gSems[slot].id != -1 && gSems[slot].owner == team) {
			GRAB_SEM_LOCK(gSems[slot]);
			if (gSems[slot].id != -1 && gSems[slot].owner == team) {
				// found one!
				info->sem			= gSems[slot].id;
				info->team			= gSems[slot].owner;
				strncpy(info->name, gSems[slot].name, SYS_MAX_OS_NAME_LEN-1);
				info->count			= gSems[slot].count;
				info->latest_holder	= gSems[slot].q.head->id; // XXX not sure if this is the latest holder, or the next holder...

				RELEASE_SEM_LOCK(gSems[slot]);
				slot++;
				break;
			}
			RELEASE_SEM_LOCK(gSems[slot]);
		}
		slot++;
	}
	RELEASE_SEM_LIST_LOCK();
	restore_interrupts(state);

	if (slot == MAX_SEMS)
		return B_BAD_SEM_ID;
	*cookie = slot;
	return B_NO_ERROR;
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
	if (team < 0)
		return B_BAD_TEAM_ID;

	// check if the team ID is valid
	if (team_get_team_struct(team) == NULL)
		return B_BAD_TEAM_ID;

	slot = id % MAX_SEMS;

	state = disable_interrupts();
	GRAB_SEM_LOCK(gSems[slot]);

	if (gSems[slot].id != id) {
		RELEASE_SEM_LOCK(gSems[slot]);
		restore_interrupts(state);
		TRACE(("set_sem_owner: invalid sem_id %ld\n", id));
		return B_BAD_SEM_ID;
	}

	// Todo: this is a small race condition: the team ID could already
	// be invalid at this point - we would lose one semaphore slot in
	// this case!
	gSems[slot].owner = team;

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

	TRACE(("sem_interrupt_thread: called on thread %p (%d), blocked on sem 0x%x\n", t, t->id, t->sem_blocking));

	if (t->state != B_THREAD_WAITING || t->sem_blocking < 0)
		return EINVAL;
	if ((t->sem_flags & B_CAN_INTERRUPT) == 0)
		return ERR_SEM_NOT_INTERRUPTABLE;

	t->next_state = B_THREAD_READY;
	
	slot = t->sem_blocking % MAX_SEMS;

	GRAB_SEM_LOCK(gSems[slot]);

	if (gSems[slot].id != t->sem_blocking) {
		panic("sem_interrupt_thread: thread 0x%lx sez it's blocking on sem 0x%lx, but that sem doesn't exist!\n", t->id, t->sem_blocking);
	}

	wakeup_queue.head = wakeup_queue.tail = NULL;
	if (remove_thread_from_sem(t, &gSems[slot], &wakeup_queue, EINTR) == ERR_NOT_FOUND)
		panic("sem_interrupt_thread: thread 0x%lx not found in sem 0x%lx's wait queue\n", t->id, t->sem_blocking);

	RELEASE_SEM_LOCK(gSems[slot]);

	while ((t = thread_dequeue(&wakeup_queue)) != NULL) {
		thread_enqueue_run_q(t);
	}

	return B_NO_ERROR;
}


/** forcibly removes a thread from a semaphores wait q. May have to wake up other threads in the
 *	process. All threads that need to be woken up are added to the passed in thread_queue.
 *	must be called with sem lock held
 */

static int
remove_thread_from_sem(struct thread *t, struct sem_entry *sem, struct thread_queue *queue, int sem_errcode)
{
	struct thread *t1;

	// remove the thread from the queue and place it in the supplied queue
	t1 = thread_dequeue_id(&sem->q, t->id);
	if (t != t1)
		return ERR_NOT_FOUND;
	sem->count += t->sem_acquire_count;
	t->state = B_THREAD_READY;
	t->sem_errcode = sem_errcode;
	thread_enqueue(t, queue);

	// now see if more threads need to be woken up
	while (sem->count > 0 && (t1 = thread_lookat_queue(&sem->q))) {
		int delta = min(t->sem_count, sem->count);

		t->sem_count -= delta;
		if (t->sem_count <= 0) {
			t = thread_dequeue(&sem->q);
			t->state = B_THREAD_READY;
			thread_enqueue(t, queue);
		}
		sem->count -= delta;
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

	for (i = 0; i < MAX_SEMS; i++) {
		if (gSems[i].id != -1 && gSems[i].owner == owner) {
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


sem_id
user_create_sem(int32 count, const char *uname)
{
	if (uname != NULL) {
		char name[SYS_MAX_OS_NAME_LEN];
		int rc;

		if ((addr)uname >= KERNEL_BASE && (addr)uname <= KERNEL_TOP)
			return ERR_VM_BAD_USER_MEMORY;

		rc = user_strncpy(name, uname, SYS_MAX_OS_NAME_LEN-1);
		if (rc < 0)
			return rc;
		name[SYS_MAX_OS_NAME_LEN-1] = 0;

		return create_sem_etc(count, name, team_get_current_team_id());
	}

	return create_sem_etc(count, NULL, team_get_current_team_id());
}


status_t
user_delete_sem(sem_id id)
{
	return delete_sem(id);
}


status_t
user_delete_sem_etc(sem_id id, status_t return_code, bool interrupted)
{
	return delete_sem_etc(id, return_code, interrupted);
}


status_t
user_acquire_sem(sem_id id)
{
	return user_acquire_sem_etc(id, 1, 0, 0);
}


status_t
user_acquire_sem_etc(sem_id id, int32 count, uint32 flags, bigtime_t timeout)
{
	flags = flags | B_CAN_INTERRUPT;

	return acquire_sem_etc(id, count, flags, timeout);
}


status_t
user_release_sem(sem_id id)
{
	return release_sem_etc(id, 1, 0);
}


status_t
user_release_sem_etc(sem_id id, int32 count, uint32 flags)
{
	return release_sem_etc(id, count, flags);
}


status_t
user_get_sem_count(sem_id uid, int32* uthread_count)
{
	int32 thread_count;
	status_t rc;
	int rc2;
	rc  = get_sem_count(uid, &thread_count);
	rc2 = user_memcpy(uthread_count, &thread_count, sizeof(int32));
	if (rc2 < 0)
		return rc2;

	return rc;
}


status_t
user_get_sem_info(sem_id uid, struct sem_info *uinfo, size_t sz)
{
	struct sem_info info;
	status_t rc;
	int rc2;

	if ((addr)uinfo >= KERNEL_BASE && (addr)uinfo <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	rc = _get_sem_info(uid, &info, sz);
	rc2 = user_memcpy(uinfo, &info, sz);
	if (rc2 < 0)
		return rc2;

	return rc;
}


status_t
user_get_next_sem_info(team_id uteam, int32 *ucookie, struct sem_info *uinfo, size_t sz)
{
	struct sem_info info;
	int32 cookie;
	status_t rc;
	int rc2;

	if ((addr)uinfo >= KERNEL_BASE && (addr)uinfo <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	rc2 = user_memcpy(&cookie, ucookie, sizeof(int32));
	if (rc2 < 0)
		return rc2;
	rc = _get_next_sem_info(uteam, &cookie, &info, sz);
	rc2 = user_memcpy(uinfo, &info, sz);
	if (rc2 < 0)
		return rc2;
	rc2 = user_memcpy(ucookie, &cookie, sizeof(int32));
	if (rc2 < 0)
		return rc2;

	return rc;
}


status_t
user_set_sem_owner(sem_id uid, team_id uteam)
{
	return set_sem_owner(uid, uteam);
}
