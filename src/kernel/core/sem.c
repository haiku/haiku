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

#include <stage2.h>

#include <string.h>
#include <stdlib.h>

struct sem_entry {
	sem_id    id;
	int       count;
	struct thread_queue q;
	char      *name;
	int       lock;
	proc_id   owner;		 // if set to -1, means owned by a port
};

#define MAX_SEMS 4096

static struct sem_entry *sems = NULL;
static region_id sem_region = 0;
static bool sems_active = false;

static sem_id next_sem = 0;

static int sem_spinlock = 0;
#define GRAB_SEM_LIST_LOCK() acquire_spinlock(&sem_spinlock)
#define RELEASE_SEM_LIST_LOCK() release_spinlock(&sem_spinlock)
#define GRAB_SEM_LOCK(s) acquire_spinlock(&(s).lock)
#define RELEASE_SEM_LOCK(s) release_spinlock(&(s).lock)

// used in functions that may put a bunch of threads in the run q at once
#define READY_THREAD_CACHE_SIZE 16

static int remove_thread_from_sem(struct thread *t, struct sem_entry *sem, struct thread_queue *queue, int sem_errcode);

struct sem_timeout_args {
	thread_id blocked_thread;
	sem_id blocked_sem_id;
	int sem_count;
};

static void dump_sem_list(int argc, char **argv)
{
	int i;

	for(i=0; i<MAX_SEMS; i++) {
		if(sems[i].id >= 0) {
			dprintf("%p\tid: 0x%x\t\tname: '%s'\n", &sems[i], sems[i].id, sems[i].name);
		}
	}
}

static void _dump_sem_info(struct sem_entry *sem)
{
	dprintf("SEM:   %p\n", sem);
	dprintf("name:  '%s'\n", sem->name);
	dprintf("owner: 0x%x\n", sem->owner);
	dprintf("count: 0x%x\n", sem->count);
	dprintf("queue: head %p tail %p\n", sem->q.head, sem->q.tail);
}

static void dump_sem_info(int argc, char **argv)
{
	int i;

	if(argc < 2) {
		dprintf("sem: not enough arguments\n");
		return;
	}

	// if the argument looks like a hex number, treat it as such
	if(strlen(argv[1]) > 2 && argv[1][0] == '0' && argv[1][1] == 'x') {
		unsigned long num = atoul(argv[1]);

		if(num > KERNEL_BASE && num <= (KERNEL_BASE + (KERNEL_SIZE - 1))) {
			// XXX semi-hack
			_dump_sem_info((struct sem_entry *)num);
			return;
		} else {
			unsigned slot = num % MAX_SEMS;
			if(sems[slot].id != (int)num) {
				dprintf("sem 0x%lx doesn't exist!\n", num);
				return;
			}
			_dump_sem_info(&sems[slot]);
			return;
		}
	}

	// walk through the sem list, trying to match name
	for(i=0; i<MAX_SEMS; i++) {
		if (sems[i].name != NULL)
			if(strcmp(argv[1], sems[i].name) == 0) {
				_dump_sem_info(&sems[i]);
				return;
			}
	}
}

int sem_init(kernel_args *ka)
{
	int i;

	dprintf("sem_init: entry\n");

	// create and initialize semaphore table
	sem_region = vm_create_anonymous_region(vm_get_kernel_aspace_id(), "sem_table", (void **)&sems,
		REGION_ADDR_ANY_ADDRESS, sizeof(struct sem_entry) * MAX_SEMS, REGION_WIRING_WIRED, LOCK_RW|LOCK_KERNEL);
	if(sem_region < 0) {
		panic("unable to allocate semaphore table!\n");
	}

	memset(sems, 0, sizeof(struct sem_entry) * MAX_SEMS);
	for(i=0; i<MAX_SEMS; i++)
		sems[i].id = -1;

	// add debugger commands
	dbg_add_command(&dump_sem_list, "sems", "Dump a list of all active semaphores");
	dbg_add_command(&dump_sem_info, "sem", "Dump info about a particular semaphore");

	dprintf("sem_init: exit\n");

	sems_active = true;

	return 0;
}

sem_id create_sem_etc(int count, const char *name, proc_id owner)
{
	int i;
	int state;
	sem_id retval = B_NO_MORE_SEMS;
	char *temp_name;

	if(sems_active == false)
		return B_NO_MORE_SEMS;

	if(name) {
		int name_len = strlen(name);

		temp_name = (char *)kmalloc(min(name_len + 1, SYS_MAX_OS_NAME_LEN));
		if(temp_name == NULL)
			return ENOMEM;
		strncpy(temp_name, name, SYS_MAX_OS_NAME_LEN-1);
		temp_name[SYS_MAX_OS_NAME_LEN-1] = 0;
	} else {
		temp_name = (char *)kmalloc(sizeof("default_sem_name")+1);
		if(temp_name == NULL)
			return ENOMEM;
		strcpy(temp_name, "default_sem_name");
	}

	state = int_disable_interrupts();
	GRAB_SEM_LIST_LOCK();

	// find the first empty spot
	for(i=0; i<MAX_SEMS; i++) {
		if(sems[i].id == -1) {
			// make the sem id be a multiple of the slot it's in
			if(i >= next_sem % MAX_SEMS) {
				next_sem += i - next_sem % MAX_SEMS;
			} else {
				next_sem += MAX_SEMS - (next_sem % MAX_SEMS - i);
			}
			sems[i].id = next_sem++;

			sems[i].lock = 0;
			GRAB_SEM_LOCK(sems[i]);
			RELEASE_SEM_LIST_LOCK();

			sems[i].q.tail = NULL;
			sems[i].q.head = NULL;
			sems[i].count = count;
			sems[i].name = temp_name;
			sems[i].owner = owner;
			retval = sems[i].id;

			RELEASE_SEM_LOCK(sems[i]);
			goto out;
		}
	}

//err:
	RELEASE_SEM_LIST_LOCK();
	kfree(temp_name);

out:
	int_restore_interrupts(state);

	return retval;
}

sem_id create_sem(int count, const char *name)
{
	return create_sem_etc(count, name, proc_get_kernel_proc_id());
}

int delete_sem(sem_id id)
{
	return delete_sem_etc(id, 0);
}

int delete_sem_etc(sem_id id, int return_code)
{
	int slot;
	int state;
	int err = B_NO_ERROR;
	struct thread *t;
	int released_threads;
	char *old_name;
	struct thread_queue release_queue;

	if(sems_active == false)
		return B_NO_MORE_SEMS;
	if(id < 0)
		return B_BAD_SEM_ID;

	slot = id % MAX_SEMS;

	state = int_disable_interrupts();
	GRAB_SEM_LOCK(sems[slot]);

	if(sems[slot].id != id) {
		RELEASE_SEM_LOCK(sems[slot]);
		int_restore_interrupts(state);
		dprintf("delete_sem: invalid sem_id %d\n", id);
		return B_BAD_SEM_ID;
	}

	released_threads = 0;
	release_queue.head = release_queue.tail = NULL;

	// free any threads waiting for this semaphore
	while((t = thread_dequeue(&sems[slot].q)) != NULL) {
		t->state = THREAD_STATE_READY;
		t->sem_errcode = B_BAD_SEM_ID;
		t->sem_deleted_retcode = return_code;
		t->sem_count = 0;
		thread_enqueue(t, &release_queue);
		released_threads++;
	}

	sems[slot].id = -1;
	old_name = sems[slot].name;
	sems[slot].name = NULL;

	RELEASE_SEM_LOCK(sems[slot]);

	if(released_threads > 0) {
		GRAB_THREAD_LOCK();
		while((t = thread_dequeue(&release_queue)) != NULL) {
			thread_enqueue_run_q(t);
		}
		thread_resched();
		RELEASE_THREAD_LOCK();
	}

	int_restore_interrupts(state);

	kfree(old_name);

	return err;
}

// Called from a timer handler. Wakes up a semaphore
static int sem_timeout(void *data)
{
	struct sem_timeout_args *args = (struct sem_timeout_args *)data;
	struct thread *t;
	int slot;
	int state;
	struct thread_queue wakeup_queue;

	t = thread_get_thread_struct(args->blocked_thread);
	if(t == NULL)
		return INT_NO_RESCHEDULE;
	slot = args->blocked_sem_id % MAX_SEMS;

	state = int_disable_interrupts();
	GRAB_SEM_LOCK(sems[slot]);

//	dprintf("sem_timeout: called on 0x%x sem %d, tid %d\n", to, to->sem_id, to->thread_id);

	if(sems[slot].id != args->blocked_sem_id) {
		// this thread was not waiting on this semaphore
		panic("sem_timeout: thid %d was trying to wait on sem %d which doesn't exist!\n",
			args->blocked_thread, args->blocked_sem_id);
	}

	wakeup_queue.head = wakeup_queue.tail = NULL;
	remove_thread_from_sem(t, &sems[slot], &wakeup_queue, B_TIMED_OUT);

	RELEASE_SEM_LOCK(sems[slot]);

	GRAB_THREAD_LOCK();
	// put the threads in the run q here to make sure we dont deadlock in sem_interrupt_thread
	while((t = thread_dequeue(&wakeup_queue)) != NULL) {
		thread_enqueue_run_q(t);
	}
	RELEASE_THREAD_LOCK();

	int_restore_interrupts(state);

	return INT_RESCHEDULE;
}


int acquire_sem(sem_id id)
{
	return acquire_sem_etc(id, 1, 0, 0);
}

int acquire_sem_etc(sem_id id, int count, int flags, bigtime_t timeout)
{
	int slot = id % MAX_SEMS;
	int state;
	int err = 0;

	if(sems_active == false)
		return B_NO_MORE_SEMS;

	if(id < 0) {
		dprintf("acquire_sem_etc: invalid sem handle %d\n", id);
		return B_BAD_SEM_ID;
	}

	if(count <= 0)
		return ERR_INVALID_ARGS;

	state = int_disable_interrupts();
	GRAB_SEM_LOCK(sems[slot]);

	if(sems[slot].id != id) {
		dprintf("acquire_sem_etc: bad sem_id %d\n", id);
		err = B_BAD_SEM_ID;
		goto err;
	}

	if(sems[slot].count - count < 0 && (flags & B_TIMEOUT) != 0 && timeout <= 0) {
		// immediate timeout
		err = ERR_SEM_TIMED_OUT;
		goto err;
	}

	if((sems[slot].count -= count) < 0) {
		// we need to block
		struct thread *t = thread_get_current_thread();
		struct timer_event timer; // stick it on the stack, since we may be blocking here
		struct sem_timeout_args args;

		// do a quick check to see if the thread has any pending kill signals
		// this should catch most of the cases where the thread had a signal
		if((flags & B_CAN_INTERRUPT) && (t->pending_signals & SIG_KILL)) {
			sems[slot].count += count;
			err = EINTR;
			goto err;
		}

		t->next_state = THREAD_STATE_WAITING;
		t->sem_flags = flags;
		t->sem_blocking = id;
		t->sem_acquire_count = count;
		t->sem_count = min(-sems[slot].count, count); // store the count we need to restore upon release
		t->sem_deleted_retcode = 0;
		t->sem_errcode = B_NO_ERROR;
		thread_enqueue(t, &sems[slot].q);

		if((flags & (B_TIMEOUT | B_ABSOLUTE_TIMEOUT)) != 0) {
			int the_timeout = timeout;
//			dprintf("sem_acquire_etc: setting timeout sem for %d %d usecs, semid %d, tid %d\n",
//				timeout, sem_id, t->id);
			// set up an event to go off with the thread struct as the data
			if (flags & B_ABSOLUTE_TIMEOUT)
				the_timeout -= system_time();

			args.blocked_sem_id = id;
			args.blocked_thread = t->id;
			args.sem_count = count;
			
			timer_setup_timer(&sem_timeout, &args, &timer);
			timer_set_event(the_timeout, TIMER_MODE_ONESHOT, &timer);
		}

		RELEASE_SEM_LOCK(sems[slot]);
		GRAB_THREAD_LOCK();
		// check again to see if a kill signal is pending.
		// it may have been delivered while setting up the sem, though it's pretty unlikely
		if((flags & B_CAN_INTERRUPT) && (t->pending_signals & SIG_KILL)) {
			struct thread_queue wakeup_queue;
			// ok, so a tiny race happened where a signal was delivered to this thread while
			// it was setting up the sem. We can only be sure a signal wasn't delivered
			// here, since the threadlock is held. The previous check would have found most
			// instances, but there was a race, so we have to handle it. It'll be more messy...
			wakeup_queue.head = wakeup_queue.tail = NULL;
			GRAB_SEM_LOCK(sems[slot]);
			if(sems[slot].id == id) {
				remove_thread_from_sem(t, &sems[slot], &wakeup_queue, EINTR);
			}
			RELEASE_SEM_LOCK(sems[slot]);
			while((t = thread_dequeue(&wakeup_queue)) != NULL) {
				thread_enqueue_run_q(t);
			}
			// fall through and reschedule since another thread with a higher priority may have been woken up
		}
		thread_resched();
		RELEASE_THREAD_LOCK();

		if((flags & B_TIMEOUT) != 0) {
			if(t->sem_errcode != ERR_SEM_TIMED_OUT) {
				// cancel the timer event, the sem may have been deleted or interrupted
				// with the timer still active
				timer_cancel_event(&timer);
			}
		}

		int_restore_interrupts(state);

		return t->sem_errcode;
	}

err:
	RELEASE_SEM_LOCK(sems[slot]);
	int_restore_interrupts(state);

	return err;
}

int release_sem(sem_id id)
{
	return release_sem_etc(id, 1, 0);
}


int release_sem_etc(sem_id id, int count, int flags)
{
	int slot = id % MAX_SEMS;
	int state;
	int released_threads = 0;
	int err = 0;
	struct thread_queue release_queue;

	if(sems_active == false)
		return B_NO_MORE_SEMS;

	if(id < 0)
		return B_BAD_SEM_ID;

	if(count <= 0)
		return ERR_INVALID_ARGS;

	state = int_disable_interrupts();
	GRAB_SEM_LOCK(sems[slot]);

	if(sems[slot].id != id) {
		dprintf("sem_release_etc: invalid sem_id %d\n", id);
		err = B_BAD_SEM_ID;
		goto err;
	}

	// clear out a queue we will use to hold all of the threads that we will have to
	// put back into the run list. This is done so the thread lock wont be held
	// while this sems lock is held since the two locks are grabbed in the other
	// order in sem_interrupt_thread.
	release_queue.head = release_queue.tail = NULL;

	while(count > 0) {
		int delta = count;
		if(sems[slot].count < 0) {
			struct thread *t = thread_lookat_queue(&sems[slot].q);

			delta = min(count, t->sem_count);
			t->sem_count -= delta;
			if(t->sem_count <= 0) {
				// release this thread
				t = thread_dequeue(&sems[slot].q);
				thread_enqueue(t, &release_queue);
				t->state = THREAD_STATE_READY;
				released_threads++;
				t->sem_count = 0;
				t->sem_deleted_retcode = 0;
			}
		}

		sems[slot].count += delta;
		count -= delta;
	}
	RELEASE_SEM_LOCK(sems[slot]);

	// pull off any items in the release queue and put them in the run queue
	if(released_threads > 0) {
		struct thread *t;
		GRAB_THREAD_LOCK();
		while((t = thread_dequeue(&release_queue)) != NULL) {
			thread_enqueue_run_q(t);
		}
		if((flags & B_DO_NOT_RESCHEDULE) == 0) {
			thread_resched();
		}
		RELEASE_THREAD_LOCK();
	}
	goto outnolock;

err:
	RELEASE_SEM_LOCK(sems[slot]);
outnolock:
	int_restore_interrupts(state);

	return err;
}

int get_sem_count(sem_id id, int32* thread_count)
{
	int slot;
	int state;
//	int count;

	if(sems_active == false)
		return B_NO_MORE_SEMS;
	if(id < 0)
		return B_BAD_SEM_ID;
	if (thread_count == NULL)
		return ERR_INVALID_ARGS;

	slot = id % MAX_SEMS;

	state = int_disable_interrupts();
	GRAB_SEM_LOCK(sems[slot]);

	if(sems[slot].id != id) {
		RELEASE_SEM_LOCK(sems[slot]);
		int_restore_interrupts(state);
		dprintf("sem_get_count: invalid sem_id %d\n", id);
		return B_BAD_SEM_ID;
	}

	*thread_count = sems[slot].count;

	RELEASE_SEM_LOCK(sems[slot]);
	int_restore_interrupts(state);

	return B_NO_ERROR;
}

int _get_sem_info(sem_id id, struct sem_info *info, size_t sz)
{
	int state;
	int slot;

	if(sems_active == false)
		return B_NO_MORE_SEMS;
	if(id < 0)
		return B_BAD_SEM_ID;
	if (info == NULL)
		return ERR_INVALID_ARGS;

	slot = id % MAX_SEMS;

	state = int_disable_interrupts();
	GRAB_SEM_LOCK(sems[slot]);

	if(sems[slot].id != id) {
		RELEASE_SEM_LOCK(sems[slot]);
		int_restore_interrupts(state);
		dprintf("get_sem_info: invalid sem_id %d\n", id);
		return B_BAD_SEM_ID;
	}

	info->sem			= sems[slot].id;
	info->proc			= sems[slot].owner;
	strncpy(info->name, sems[slot].name, SYS_MAX_OS_NAME_LEN-1);
	info->count			= sems[slot].count;
	info->latest_holder	= sems[slot].q.head->id; // XXX not sure if this is correct

	RELEASE_SEM_LOCK(sems[slot]);
	int_restore_interrupts(state);

	return B_NO_ERROR;
}

int _get_next_sem_info(proc_id proc, uint32 *cookie, struct sem_info *info, size_t sz)
{
	int state;
	int slot;

	if(sems_active == false)
		return B_NO_MORE_SEMS;

	if (cookie == NULL)
		return ERR_INVALID_ARGS;
	if (proc < 0)
		return ERR_INVALID_ARGS;	// prevents sems[].owner == -1 >= means owned by a port

	if (*cookie == NULL) {
		// return first found
		slot = 0;
	} else {
		// start at index cookie, but check cookie against MAX_PORTS
		slot = *cookie;
		if (slot >= MAX_SEMS)
			return B_BAD_SEM_ID;
	}
	// spinlock
	state = int_disable_interrupts();
	GRAB_SEM_LIST_LOCK();

	while (slot < MAX_SEMS) {
		GRAB_SEM_LOCK(sems[slot]);
		if (sems[slot].id != -1)
			if (sems[slot].owner == proc) {
				// found one!
				info->sem			= sems[slot].id;
				info->proc			= sems[slot].owner;
				strncpy(info->name, sems[slot].name, SYS_MAX_OS_NAME_LEN-1);
				info->count			= sems[slot].count;
				info->latest_holder	= sems[slot].q.head->id; // XXX not sure if this is the latest holder, or the next holder...

				RELEASE_SEM_LOCK(sems[slot]);
				slot++;
				break;
			}
		RELEASE_SEM_LOCK(sems[slot]);
		slot++;
	}
	RELEASE_SEM_LIST_LOCK();
	int_restore_interrupts(state);

	if (slot == MAX_SEMS)
		return ERR_SEM_NOT_FOUND;
	*cookie = slot;
	return B_NO_ERROR;
}

int set_sem_owner(sem_id id, proc_id proc)
{
	int state;
	int slot;

	if(sems_active == false)
		return B_NO_MORE_SEMS;
	if(id < 0)
		return B_BAD_SEM_ID;
	if (proc < NULL)
		return ERR_INVALID_ARGS;

	// XXX: todo check if proc exists
//	if (proc_get_proc_struct(proc) == NULL)
//		return B_BAD_SEM_ID; // proc_id doesn't exist right now

	slot = id % MAX_SEMS;

	state = int_disable_interrupts();
	GRAB_SEM_LOCK(sems[slot]);

	if(sems[slot].id != id) {
		RELEASE_SEM_LOCK(sems[slot]);
		int_restore_interrupts(state);
		dprintf("set_sem_owner: invalid sem_id %d\n", id);
		return B_BAD_SEM_ID;
	}

	sems[slot].owner = proc;

	RELEASE_SEM_LOCK(sems[slot]);
	int_restore_interrupts(state);

	return B_NO_ERROR;
}

// Wake up a thread that's blocked on a semaphore
// this function must be entered with interrupts disabled and THREADLOCK held
int sem_interrupt_thread(struct thread *t)
{
//	struct thread *t1;
	int slot;
//	int state;
	struct thread_queue wakeup_queue;

//	dprintf("sem_interrupt_thread: called on thread %p (%d), blocked on sem 0x%x\n", t, t->id, t->sem_blocking);

	if(t->state != THREAD_STATE_WAITING)
		return ERR_INVALID_ARGS;
	if(t->sem_blocking < 0)
		return ERR_INVALID_ARGS;
	if((t->sem_flags & B_CAN_INTERRUPT) == 0)
		return ERR_SEM_NOT_INTERRUPTABLE;

	slot = t->sem_blocking % MAX_SEMS;

	GRAB_SEM_LOCK(sems[slot]);

	if(sems[slot].id != t->sem_blocking) {
		panic("sem_interrupt_thread: thread 0x%x sez it's blocking on sem 0x%x, but that sem doesn't exist!\n", t->id, t->sem_blocking);
	}

	wakeup_queue.head = wakeup_queue.tail = NULL;
	if(remove_thread_from_sem(t, &sems[slot], &wakeup_queue, EINTR) == ERR_NOT_FOUND)
		panic("sem_interrupt_thread: thread 0x%x not found in sem 0x%x's wait queue\n", t->id, t->sem_blocking);

	RELEASE_SEM_LOCK(sems[slot]);

	while((t = thread_dequeue(&wakeup_queue)) != NULL) {
		thread_enqueue_run_q(t);
	}

	return B_NO_ERROR;
}

// forcibly removes a thread from a semaphores wait q. May have to wake up other threads in the
// process. All threads that need to be woken up are added to the passed in thread_queue.
// must be called with sem lock held
static int remove_thread_from_sem(struct thread *t, struct sem_entry *sem, struct thread_queue *queue, int sem_errcode)
{
	struct thread *t1;

	// remove the thread from the queue and place it in the supplied queue
	t1 = thread_dequeue_id(&sem->q, t->id);
	if(t != t1)
		return ERR_NOT_FOUND;
	sem->count += t->sem_acquire_count;
	t->state = THREAD_STATE_READY;
	t->sem_errcode = sem_errcode;
	thread_enqueue(t, queue);

	// now see if more threads need to be woken up
	while(sem->count > 0 && (t1 = thread_lookat_queue(&sem->q))) {
		int delta = min(t->sem_count, sem->count);

		t->sem_count -= delta;
		if(t->sem_count <= 0) {
			t = thread_dequeue(&sem->q);
			t->state = THREAD_STATE_READY;
			thread_enqueue(t, queue);
		}
		sem->count -= delta;
	}
	return B_NO_ERROR;
}

/* this function cycles through the sem table, deleting all the sems that are owned by
   the passed proc_id */
int sem_delete_owned_sems(proc_id owner)
{
	int state;
	int i;
	int count = 0;

	if (owner < 0)
		return B_BAD_SEM_ID;

	state = int_disable_interrupts();
	GRAB_SEM_LIST_LOCK();

	for(i=0; i<MAX_SEMS; i++) {
		if(sems[i].id != -1 && sems[i].owner == owner) {
			sem_id id = sems[i].id;

			RELEASE_SEM_LIST_LOCK();
			int_restore_interrupts(state);

			delete_sem_etc(id, 0);
			count++;

			state = int_disable_interrupts();
			GRAB_SEM_LIST_LOCK();
		}
	}

	RELEASE_SEM_LIST_LOCK();
	int_restore_interrupts(state);

	return count;
}

sem_id user_create_sem(int count, const char *uname)
{
	if(uname != NULL) {
		char name[SYS_MAX_OS_NAME_LEN];
		int rc;

		if((addr)uname >= KERNEL_BASE && (addr)uname <= KERNEL_TOP)
			return ERR_VM_BAD_USER_MEMORY;

		rc = user_strncpy(name, uname, SYS_MAX_OS_NAME_LEN-1);
		if(rc < 0)
			return rc;
		name[SYS_MAX_OS_NAME_LEN-1] = 0;

		return create_sem_etc(count, name, proc_get_current_proc_id());
	} else {
		return create_sem_etc(count, NULL, proc_get_current_proc_id());
	}
}

int user_delete_sem(sem_id id)
{
	return delete_sem(id);
}

int user_delete_sem_etc(sem_id id, int return_code)
{
	return delete_sem_etc(id, return_code);
}

int user_acquire_sem(sem_id id)
{
	return user_acquire_sem_etc(id, 1, 0, 0);
}

int user_acquire_sem_etc(sem_id id, int count, int flags, bigtime_t timeout)
{
	flags = flags | B_CAN_INTERRUPT;

	return acquire_sem_etc(id, count, flags, timeout);
}

int user_release_sem(sem_id id)
{
	return release_sem_etc(id, 1, 0);
}

int user_release_sem_etc(sem_id id, int count, int flags)
{
	return release_sem_etc(id, count, flags);
}

int user_get_sem_count(sem_id uid, int32* uthread_count)
{
	int32 thread_count;
	int rc, rc2;
	rc  = get_sem_count(uid, &thread_count);
	rc2 = user_memcpy(uthread_count, &thread_count, sizeof(int32));
	if(rc2 < 0)
		return rc2;
	return rc;
}

int user_get_sem_info(sem_id uid, struct sem_info *uinfo, size_t sz)
{
	struct sem_info info;
	int rc, rc2;

	if((addr)uinfo >= KERNEL_BASE && (addr)uinfo <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	rc = _get_sem_info(uid, &info, sz);
	rc2 = user_memcpy(uinfo, &info, sz);
	if(rc2 < 0)
		return rc2;
	return rc;
}

int user_get_next_sem_info(proc_id uproc, uint32 *ucookie, struct sem_info *uinfo, size_t sz)
{
	struct sem_info info;
	uint32 cookie;
	int rc, rc2;

	if((addr)uinfo >= KERNEL_BASE && (addr)uinfo <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	rc2 = user_memcpy(&cookie, ucookie, sizeof(uint32));
	if(rc2 < 0)
		return rc2;
	rc = _get_next_sem_info(uproc, &cookie, &info, sz);
	rc2 = user_memcpy(uinfo, &info, sz);
	if(rc2 < 0)
		return rc2;
	rc2 = user_memcpy(ucookie, &cookie, sizeof(uint32));
	if(rc2 < 0)
		return rc2;
	return rc;
}

int user_set_sem_owner(sem_id uid, proc_id uproc)
{
	return set_sem_owner(uid, uproc);
}
