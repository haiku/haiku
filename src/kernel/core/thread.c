/*
** Copyright 2002-2004, The Haiku Team. All rights reserved.
** Distributed under the terms of the Haiku License.
**
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

/* Threading routines */

#include <OS.h>

#include <thread.h>
#include <team.h>
#include <khash.h>
#include <int.h>
#include <smp.h>
#include <cpu.h>
#include <arch/vm.h>
#include <user_runtime.h>
#include <boot/kernel_args.h>
#include <kimage.h>
#include <ksignal.h>
#include <syscalls.h>
#include <tls.h>

#include <sys/resource.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//#define TRACE_THREAD
#ifdef TRACE_THREAD
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#define THREAD_MAX_MESSAGE_SIZE		65536

static status_t receive_data_etc(thread_id *_sender, void *buffer, size_t bufferSize, int32 flags);

struct thread_key {
	thread_id id;
};

// global
spinlock thread_spinlock = 0;

// thread list
static struct thread *sIdleThreads[B_MAX_CPU_COUNT];
static void *sThreadHash = NULL;
static thread_id sNextThreadID = 1;

static sem_id sSnoozeSem = -1;

// death stacks - used temporarily as a thread cleans itself up
struct death_stack {
	region_id area;
	addr_t address;
	bool in_use;
};
static struct death_stack *sDeathStacks;
static unsigned int sNumDeathStacks;
static unsigned int volatile sDeathStackBitmap;
static sem_id sDeathStackSem;

// The dead queue is used as a pool from which to retrieve and reuse previously
// allocated thread structs when creating a new thread. It should be gone once
// the slab allocator is in.
struct thread_queue dead_q;

static void thread_kthread_entry(void);
static void thread_kthread_exit(void);


/**	Inserts a thread into a team.
 *	You must hold the team lock when you call this function.
 */

static void
insert_thread_into_team(struct team *p, struct thread *t)
{
	t->team_next = p->thread_list;
	p->thread_list = t;
	p->num_threads++;
	if (p->num_threads == 1) {
		// this was the first thread
		p->main_thread = t;
	}
	t->team = p;
}


/**	Removes a thread from a team.
 *	You must hold the team lock when you call this function.
 */

static void
remove_thread_from_team(struct team *p, struct thread *t)
{
	struct thread *temp, *last = NULL;

	for (temp = p->thread_list; temp != NULL; temp = temp->team_next) {
		if (temp == t) {
			if (last == NULL)
				p->thread_list = temp->team_next;
			else
				last->team_next = temp->team_next;

			p->num_threads--;
			break;
		}
		last = temp;
	}
}


static int
thread_struct_compare(void *_t, const void *_key)
{
	struct thread *t = _t;
	const struct thread_key *key = _key;

	if (t->id == key->id)
		return 0;

	return 1;
}


static uint32
thread_struct_hash(void *_t, const void *_key, uint32 range)
{
	struct thread *t = _t;
	const struct thread_key *key = _key;

	if (t != NULL)
		return t->id % range;

	return key->id % range;
}


static struct thread *
create_thread_struct(const char *name)
{
	struct thread *t;
	cpu_status state;
	char temp[64];

	state = disable_interrupts();
	GRAB_THREAD_LOCK();
	t = thread_dequeue(&dead_q);
	RELEASE_THREAD_LOCK();
	restore_interrupts(state);

	if (t == NULL) {
		t = (struct thread *)malloc(sizeof(struct thread));
		if (t == NULL)
			return NULL;
	}

	strlcpy(t->name, name, B_OS_NAME_LENGTH);

	t->id = atomic_add(&sNextThreadID, 1);
	t->team = NULL;
	t->cpu = NULL;
	t->sem.blocking = -1;
	t->fault_handler = 0;
	t->page_faults_allowed = 1;
	t->kernel_stack_region_id = -1;
	t->kernel_stack_base = 0;
	t->user_stack_region_id = -1;
	t->user_stack_base = 0;
	t->user_local_storage = 0;
	t->kernel_errno = 0;
	t->team_next = NULL;
	t->queue_next = NULL;
	t->priority = -1;
	t->args1 = NULL;  t->args2 = NULL;
	t->sig_pending = 0;
	t->sig_block_mask = 0;
	memset(t->sig_action, 0, 32 * sizeof(struct sigaction));
	t->in_kernel = true;
	t->user_time = 0;
	t->kernel_time = 0;
	t->last_time = 0;
	t->last_time_type = KERNEL_TIME;
	t->exit.status = 0;
	t->exit.reason = 0;
	list_init(&t->exit.waiters);

	sprintf(temp, "thread_0x%lx_retcode_sem", t->id);
	t->exit.sem = create_sem(0, temp);
	if (t->exit.sem < B_OK)
		goto err1;

	sprintf(temp, "%s send", t->name);
	t->msg.write_sem = create_sem(1, temp);
	if (t->msg.write_sem < B_OK)
		goto err2;

	sprintf(temp, "%s receive", t->name);
	t->msg.read_sem = create_sem(0, temp);
	if (t->msg.read_sem < B_OK)
		goto err3;

	if (arch_thread_init_thread_struct(t) < B_OK)
		goto err4;

	return t;

err4:
	delete_sem(t->msg.read_sem);
err3:
	delete_sem(t->msg.write_sem);
err2:
	delete_sem(t->exit.sem);
err1:
	// ToDo: put them in the dead queue instead?
	free(t);
	return NULL;
}


static void
delete_thread_struct(struct thread *thread)
{
	delete_sem(thread->exit.sem);
	delete_sem(thread->msg.write_sem);
	delete_sem(thread->msg.read_sem);

	// ToDo: put them in the dead queue instead?
	free(thread);
}


// this function gets run by a new thread before anything else

static void
thread_kthread_entry(void)
{
	// simulates the thread spinlock release that would occur if the thread had been
	// rescheded from. The resched didn't happen because the thread is new.
	RELEASE_THREAD_LOCK();
	enable_interrupts(); // this essentially simulates a return-from-interrupt
}


static void
thread_kthread_exit(void)
{
	struct thread *t = thread_get_current_thread();

	t->exit.reason = THREAD_RETURN_EXIT;
	thread_exit();
}


/** Initializes the thread and jumps to its userspace entry point.
 *	This function is called at creation time of every user thread,
 *	but not for a team's main thread.
 */

static int
_create_user_thread_kentry(void)
{
	struct thread *thread = thread_get_current_thread();

	// a signal may have been delivered here
	// ToDo: this looks broken
//	thread_atkernel_exit();
	// start tracking kernel & user time
	thread->last_time = system_time();
	thread->last_time_type = KERNEL_TIME;
	thread->in_kernel = false;
	// jump to the entry point in user space
	arch_thread_enter_uspace(thread, (addr_t)thread->entry, thread->args1, thread->args2);

	// never get here
	return 0;
}


/** Initializes the thread and calls it kernel space entry point.
 */

static int
_create_kernel_thread_kentry(void)
{
	struct thread *thread = thread_get_current_thread();
	int (*func)(void *args);

	// start tracking kernel & user time
	thread->last_time = system_time();
	thread->last_time_type = KERNEL_TIME;

	// call the entry function with the appropriate args
	func = (void *)thread->entry;

	return func(thread->args1);
}


static thread_id
create_thread(const char *name, team_id teamID, thread_entry_func entry,
	void *args1, void *args2, int32 priority, bool kernel)
{
	struct thread *t;
	struct team *team;
	cpu_status state;
	char stack_name[B_OS_NAME_LENGTH];
	status_t status;
	bool abort = false;

	t = create_thread_struct(name);
	if (t == NULL)
		return B_NO_MEMORY;

	t->priority = priority == -1 ? B_NORMAL_PRIORITY : priority;
	// ToDo: this could be dangerous in case someone calls resume_thread() on us
	t->state = B_THREAD_SUSPENDED;
	t->next_state = B_THREAD_SUSPENDED;

	snprintf(stack_name, B_OS_NAME_LENGTH, "%s_%lx_kstack", name, t->id);
	t->kernel_stack_region_id = create_area(stack_name, (void **)&t->kernel_stack_base,
		B_ANY_KERNEL_ADDRESS, KSTACK_SIZE, B_FULL_LOCK, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

	if (t->kernel_stack_region_id < 0) {
		// we're not yet part of a team, so we can just bail out
		dprintf("create_thread: error creating kernel stack!\n");

		status = t->kernel_stack_region_id;
		delete_thread_struct(t);
		return status;
	}

	state = disable_interrupts();
	GRAB_THREAD_LOCK();

	// insert into global list
	hash_insert(sThreadHash, t);
	RELEASE_THREAD_LOCK();

	GRAB_TEAM_LOCK();
	// look at the team, make sure it's not being deleted
	team = team_get_team_struct_locked(teamID);
	if (team != NULL && team->state != TEAM_STATE_DEATH)
		insert_thread_into_team(team, t);
	else
		abort = true;

	RELEASE_TEAM_LOCK();
	if (abort) {
		GRAB_THREAD_LOCK();
		hash_remove(sThreadHash, t);
		RELEASE_THREAD_LOCK();
	}
	restore_interrupts(state);
	if (abort) {
		delete_area(t->kernel_stack_region_id);
		delete_thread_struct(t);
		return B_BAD_TEAM_ID;
	}

	t->args1 = args1;
	t->args2 = args2;
	t->entry = entry;
	status = t->id;

	if (kernel) {
		// this sets up an initial kthread stack that runs the entry

		// Note: whatever function wants to set up a user stack later for this thread
		// must initialize the TLS for it
		arch_thread_init_kthread_stack(t, &_create_kernel_thread_kentry, &thread_kthread_entry, &thread_kthread_exit);
	} else {
		// create user stack

		// the stack will be between USER_STACK_REGION and the main thread stack area
		// (the user stack of the main thread is created in team_create_team())
		t->user_stack_base = USER_STACK_REGION;
		t->user_stack_size = STACK_SIZE;

		snprintf(stack_name, B_OS_NAME_LENGTH, "%s_%lx_stack", name, t->id);
		t->user_stack_region_id = create_area_etc(team, stack_name,
				(void **)&t->user_stack_base, B_BASE_ADDRESS,
				t->user_stack_size + TLS_SIZE, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
		if (t->user_stack_region_id < 0) {
			// great, we have a fully running thread without a stack
			dprintf("create_thread: unable to create user stack!\n");
			status = t->user_stack_region_id;
			kill_thread(t->id);
		} else {
			// now that the TLS area is allocated, initialize TLS
			arch_thread_init_tls(t);
		}

		// copy the user entry over to the args field in the thread struct
		// the function this will call will immediately switch the thread into
		// user space.
		arch_thread_init_kthread_stack(t, &_create_user_thread_kentry, &thread_kthread_entry, &thread_kthread_exit);
	}

	return status;
}


static const char *
state_to_text(int state)
{
	switch (state) {
		case B_THREAD_READY:
			return "READY";
		case B_THREAD_RUNNING:
			return "RUNNING";
		case B_THREAD_WAITING:
			return "WAITING";
		case B_THREAD_SUSPENDED:
			return "SUSPEND";
		case THREAD_STATE_FREE_ON_RESCHED:
			return "DEATH";
		default:
			return "UNKNOWN";
	}
}


static struct thread *last_thread_dumped = NULL;

static void
_dump_thread_info(struct thread *t)
{
	dprintf("THREAD: %p\n", t);
	dprintf("id:          0x%lx\n", t->id);
	dprintf("name:        '%s'\n", t->name);
	dprintf("all_next:    %p\nteam_next:  %p\nq_next:     %p\n",
		t->all_next, t->team_next, t->queue_next);
	dprintf("priority:    0x%lx\n", t->priority);
	dprintf("state:       %s\n", state_to_text(t->state));
	dprintf("next_state:  %s\n", state_to_text(t->next_state));
	dprintf("cpu:         %p ", t->cpu);
	if (t->cpu)
		dprintf("(%d)\n", t->cpu->info.cpu_num);
	else
		dprintf("\n");
	dprintf("sig_pending:  0x%lx\n", t->sig_pending);
	dprintf("in_kernel:    %d\n", t->in_kernel);
	dprintf("sem.blocking: 0x%lx\n", t->sem.blocking);
	dprintf("sem.count:    0x%lx\n", t->sem.count);
	dprintf("sem.acquire_status: 0x%lx\n", t->sem.acquire_status);
	dprintf("sem.flags:    0x%lx\n", t->sem.flags);
	dprintf("fault_handler: %p\n", (void *)t->fault_handler);
	dprintf("args:         %p %p\n", t->args1, t->args2);
	dprintf("entry:        %p\n", (void *)t->entry);
	dprintf("team:         %p\n", t->team);
	dprintf("exit.sem:     0x%lx\n", t->exit.sem);
	dprintf("kernel_stack_region_id: 0x%lx\n", t->kernel_stack_region_id);
	dprintf("kernel_stack_base: %p\n", (void *)t->kernel_stack_base);
	dprintf("user_stack_region_id:   0x%lx\n", t->user_stack_region_id);
	dprintf("user_stack_base:    %p\n", (void *)t->user_stack_base);
	dprintf("user_local_storage: %p\n", (void *)t->user_local_storage);
	dprintf("kernel_errno:      %d\n", t->kernel_errno);
	dprintf("kernel_time:       %Ld\n", t->kernel_time);
	dprintf("user_time:         %Ld\n", t->user_time);
	dprintf("architecture dependant section:\n");
	arch_thread_dump_info(&t->arch_info);

	last_thread_dumped = t;
}


static int
dump_thread_info(int argc, char **argv)
{
	const char *name = NULL;
	struct thread *t;
	int id = -1;
	struct hash_iterator i;

	if (argc > 2) {
		dprintf("usage: thread [id/name]\n");
		return 0;
	}

	if (argc == 1) {
		name = NULL;
		id = thread_get_current_thread()->id;
	} else {
		name = argv[1];

		// if the argument looks like a hex number, treat it as such
		if (strlen(argv[1]) > 2 && argv[1][0] == '0' && argv[1][1] == 'x')
			id = strtoul(argv[1], NULL, 16);
		else
			id = atoi(argv[1]);
	}

	// walk through the thread list, trying to match name or id
	hash_open(sThreadHash, &i);
	while ((t = hash_next(sThreadHash, &i)) != NULL) {
		if ((t->name && strcmp(name, t->name) == 0) || t->id == id) {
			_dump_thread_info(t);
			break;
		}
	}
	hash_close(sThreadHash, &i, false);
	return 0;
}


static int
dump_thread_list(int argc, char **argv)
{
	struct thread *t;
	struct hash_iterator i;

	hash_open(sThreadHash, &i);
	while ((t = hash_next(sThreadHash, &i)) != NULL) {
		dprintf("%p", t);
		if (t->name != NULL)
			dprintf("\t%32s", t->name);
		else
			dprintf("\t%32s", "<NULL>");
		dprintf("\t0x%lx", t->id);
		dprintf("\t%16s", state_to_text(t->state));
		if (t->cpu)
			dprintf("\t%d", t->cpu->info.cpu_num);
		else
			dprintf("\tNOCPU");
		dprintf("\t0x%lx\n", t->kernel_stack_base);
	}
	hash_close(sThreadHash, &i, false);
	return 0;
}


static int
dump_next_thread_in_q(int argc, char **argv)
{
	struct thread *t = last_thread_dumped;

	if (t == NULL) {
		dprintf("no thread previously dumped. Examine a thread first.\n");
		return 0;
	}

	dprintf("next thread in queue after thread @ %p\n", t);
	if (t->queue_next != NULL)
		_dump_thread_info(t->queue_next);
	else
		dprintf("NULL\n");

	return 0;
}


static int
dump_next_thread_in_all_list(int argc, char **argv)
{
	struct thread *t = last_thread_dumped;

	if (t == NULL) {
		dprintf("no thread previously dumped. Examine a thread first.\n");
		return 0;
	}

	dprintf("next thread in global list after thread @ %p\n", t);
	if (t->all_next != NULL)
		_dump_thread_info(t->all_next);
	else
		dprintf("NULL\n");

	return 0;
}


static int
dump_next_thread_in_team(int argc, char **argv)
{
	struct thread *t = last_thread_dumped;

	if (t == NULL) {
		dprintf("no thread previously dumped. Examine a thread first.\n");
		return 0;
	}

	dprintf("next thread in team after thread @ %p\n", t);
	if (t->team_next != NULL)
		_dump_thread_info(t->team_next);
	else
		dprintf("NULL\n");

	return 0;
}


/** Finds a free death stack for us and allocates it.
 *	Leaves interrupts disabled and returns the former interrupt state.
 */

static cpu_status
get_death_stack(uint32 *_stack)
{
	cpu_status state;
	uint32 bit;
	int32 i;

	acquire_sem(sDeathStackSem);

	state = disable_interrupts();

	// grap the thread lock, find a free spot and release
	GRAB_THREAD_LOCK();
	bit = sDeathStackBitmap;
	bit = (~bit) & ~((~bit) - 1);
	sDeathStackBitmap |= bit;
	RELEASE_THREAD_LOCK();

	// sanity checks
	if (!bit)
		panic("get_death_stack: couldn't find free stack!\n");

	if (bit & (bit - 1))
		panic("get_death_stack: impossible bitmap result!\n");

	// bit to number
	for (i = -1; bit; i++) {
		bit >>= 1;
	}

	TRACE(("get_death_stack: returning 0x%lx\n", sDeathStacks[i].address));

	*_stack = (uint32)i;
	return state;
}


static void
put_death_stack_and_reschedule(uint32 index)
{
	TRACE(("put_death_stack...: passed %lu\n", index));

	if (index >= sNumDeathStacks)
		panic("put_death_stack: passed invalid stack index %d\n", index);

	if (!(sDeathStackBitmap & (1 << index)))
		panic("put_death_stack: passed invalid stack index %d\n", index);

	disable_interrupts();
	GRAB_THREAD_LOCK();

	sDeathStackBitmap &= ~(1 << index);

	release_sem_etc(sDeathStackSem, 1, B_DO_NOT_RESCHEDULE);

	scheduler_reschedule();
		// requires thread lock to be held
}


// used to pass messages between thread_exit and thread_exit2

struct thread_exit_args {
	struct thread	*thread;
	area_id			old_kernel_stack;
	cpu_status		int_state;
	uint32			death_stack;
	sem_id			death_sem;
};


static void
thread_exit2(void *_args)
{
	struct thread_exit_args args;

	// copy the arguments over, since the source is probably on the kernel stack we're about to delete
	memcpy(&args, _args, sizeof(struct thread_exit_args));

	// we can't let the interrupts disabled at this point
	enable_interrupts();

	TRACE(("thread_exit2, running on death stack 0x%lx\n", args.t->kernel_stack_base));

	// delete the old kernel stack region
	TRACE(("thread_exit2: deleting old kernel stack id 0x%lx for thread 0x%lx\n",
		args.old_kernel_stack, args.thread->id));

	delete_area(args.old_kernel_stack);

	// remove this thread from all of the global lists
	TRACE(("thread_exit2: removing thread 0x%lx from global lists\n", args.thread->id));

	disable_interrupts();
	GRAB_TEAM_LOCK();
	remove_thread_from_team(team_get_kernel_team(), args.thread);
	RELEASE_TEAM_LOCK();
	GRAB_THREAD_LOCK();
	hash_remove(sThreadHash, args.thread);
	RELEASE_THREAD_LOCK();

	// restore former thread interrupts (doesn't matter much at this point anyway)
	restore_interrupts(args.int_state);

	TRACE(("thread_exit2: done removing thread from lists\n"));

	if (args.death_sem >= 0)
		release_sem_etc(args.death_sem, 1, B_DO_NOT_RESCHEDULE);	

	// set the next state to be gone. Will return the thread structure to a ready pool upon reschedule
	args.thread->next_state = THREAD_STATE_FREE_ON_RESCHED;

	// return the death stack and reschedule one last time
	put_death_stack_and_reschedule(args.death_stack);

	// never get to here
	panic("thread_exit2: made it where it shouldn't have!\n");
}


void
thread_exit(void)
{
	cpu_status state;
	struct thread *thread = thread_get_current_thread();
	struct process_group *freeGroup = NULL;
	struct team *team = thread->team;
	struct death_entry *death = NULL;
	thread_id mainParentThread = -1;
	bool deleteTeam = false;
	uint32 death_stack;
	sem_id cachedDeathSem, parentDeadSem = -1, groupDeadSem = -1;
	status_t status;

	if (!are_interrupts_enabled())
		dprintf("thread_exit() called with interrupts disabled!\n");

	TRACE(("thread 0x%lx exiting %s w/return code 0x%x\n", thread->id,
		thread->exit.reason == THREAD_RETURN_INTERRUPTED ? "due to signal" : "normally",
		(int)thread->exit.status));

	// boost our priority to get this over with
	thread->priority = B_URGENT_DISPLAY_PRIORITY;

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

	// Cancel previously installed alarm timer, if any
	cancel_timer(&thread->alarm);

	// delete the user stack region first, we won't need it anymore
	if (team->aspace != NULL && thread->user_stack_region_id >= 0) {
		region_id rid = thread->user_stack_region_id;
		thread->user_stack_region_id = -1;
		delete_area_etc(team, rid);
	}

	if (team != team_get_kernel_team()) {
		if (team->main_thread == thread) {
			// this was the main thread in this team, so we will delete that as well
			deleteTeam = true;

			// put a death entry into the dead children list of our parent
			death = (struct death_entry *)malloc(sizeof(struct death_entry));
			if (death != NULL) {
				death->team = team->id;
				death->thread = thread->id;
				death->status = thread->exit.status;
				death->reason = thread->exit.reason;
			}
		}

		// remove this thread from the current team and add it to the kernel
		// put the thread into the kernel team until it dies
		state = disable_interrupts();
		GRAB_TEAM_LOCK();

		remove_thread_from_team(team, thread);
		insert_thread_into_team(team_get_kernel_team(), thread);

		if (deleteTeam) {
			struct team *parent = team->parent;

			// remember who our parent was so we can send a signal
			mainParentThread = parent->main_thread->id;

			if (death != NULL) {
				// insert death entry into the parent's list

				list_add_link_to_tail(&parent->dead_children.list, death);
				if (++parent->dead_children.count > MAX_DEAD_CHILDREN) {
					death = list_remove_head_item(&parent->dead_children.list);
					parent->dead_children.count--;
				} else
					death = NULL;

				parentDeadSem = parent->dead_children.sem;
				groupDeadSem = team->group->dead_child_sem;
			}

			team_remove_team(team, &freeGroup);
		}
		RELEASE_TEAM_LOCK();
		// swap address spaces, to make sure we're running on the kernel's pgdir
		vm_aspace_swap(team_get_kernel_team()->kaspace);
		restore_interrupts(state);

		TRACE(("thread_exit: thread 0x%lx now a kernel thread!\n", thread->id));
	}

	// delete the team if we're its main thread
	if (deleteTeam) {
		// notify listeners that a new death entry is available
		// ToDo: should that be moved to handle_signal() (for SIGCHLD)?
		release_sem_etc(parentDeadSem, 0, B_RELEASE_ALL | B_DO_NOT_RESCHEDULE);
		release_sem_etc(groupDeadSem, 0, B_RELEASE_ALL | B_DO_NOT_RESCHEDULE);

		team_delete_process_group(freeGroup);
		team_delete_team(team);

		// we need to remove any death entry that made it to here
		if (death != NULL)
			free(death);

		send_signal_etc(mainParentThread, SIGCHLD, B_DO_NOT_RESCHEDULE);
		cachedDeathSem = -1;
	} else
		cachedDeathSem = team->death_sem;

	// fill all death entries and delete the sem that others will use to wait on us
	{
		sem_id cachedExitSem = thread->exit.sem;
		cpu_status state;

		state = disable_interrupts();
		GRAB_THREAD_LOCK();

		// make sure no one will grab this semaphore again
		thread->exit.sem = -1;

		// fill all death entries
		death = NULL;
		while ((death = list_get_next_item(&thread->exit.waiters, death)) != NULL) {
			death->status = thread->exit.status;
			death->reason = thread->exit.reason;
		}

		RELEASE_THREAD_LOCK();
		restore_interrupts(state);

		delete_sem(cachedExitSem);
	}

	{
		struct thread_exit_args args;

		args.int_state = get_death_stack(&death_stack);
			// this disables interrups for us

		args.thread = thread;
		args.old_kernel_stack = thread->kernel_stack_region_id;
		args.death_stack = death_stack;
		args.death_sem = cachedDeathSem;

		// set the new kernel stack officially to the death stack, wont be really switched until
		// the next function is called. This bookkeeping must be done now before a context switch
		// happens, or the processor will interrupt to the old stack
		thread->kernel_stack_region_id = sDeathStacks[death_stack].area;
		thread->kernel_stack_base = sDeathStacks[death_stack].address;

		// we will continue in thread_exit2(), on the new stack
		arch_thread_switch_kstack_and_call(thread, thread->kernel_stack_base + KSTACK_SIZE, thread_exit2, &args);
	}

	panic("never can get here\n");
}


struct thread *
thread_get_thread_struct(thread_id id)
{
	struct thread *t;
	cpu_status state;

	state = disable_interrupts();
	GRAB_THREAD_LOCK();

	t = thread_get_thread_struct_locked(id);

	RELEASE_THREAD_LOCK();
	restore_interrupts(state);

	return t;
}


struct thread *
thread_get_thread_struct_locked(thread_id id)
{
	struct thread_key key;

	key.id = id;

	return hash_lookup(sThreadHash, &key);
}


// called in the int handler code when a thread enters the kernel for any reason

void
thread_atkernel_entry(void)
{
	cpu_status state;
	struct thread *t;
	bigtime_t now;

	t = thread_get_current_thread();

	TRACE(("thread_atkernel_entry: entry thread 0x%lx\n", t->id));

	state = disable_interrupts();

	// track user time
	now = system_time();
	t->user_time += now - t->last_time;
	t->last_time = now;
	t->last_time_type = KERNEL_TIME;

	t->in_kernel = true;

	restore_interrupts(state);
}


// called when a thread exits kernel space to user space

void
thread_atkernel_exit(void)
{
	cpu_status state;
	struct thread *t;
	bigtime_t now;

	TRACE(("thread_atkernel_exit: entry\n"));

	// ToDo: this may be broken (when it is called, and what exactly should it do...)

	t = thread_get_current_thread();

	state = disable_interrupts();
	GRAB_THREAD_LOCK();

	if (handle_signals(t, state))
		scheduler_reschedule();
	// was: smp_send_broadcast_ici(SMP_MSG_RESCHEDULE, 0, 0, 0, NULL, SMP_MSG_FLAG_SYNC);

	t->in_kernel = false;

	RELEASE_THREAD_LOCK();

	// track kernel time
	now = system_time();
	t->kernel_time += now - t->last_time;
	t->last_time = now;
	t->last_time_type = USER_TIME;

	restore_interrupts(state);
}


//	#pragma mark -
//	private kernel exported functions


/** insert a thread onto the tail of a queue
 */

void
thread_enqueue(struct thread *thread, struct thread_queue *queue)
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


struct thread *
thread_lookat_queue(struct thread_queue *queue)
{
	return queue->head;
}


struct thread *
thread_dequeue(struct thread_queue *queue)
{
	struct thread *thread = queue->head;

	if (thread != NULL) {
		queue->head = thread->queue_next;
		if (queue->tail == thread)
			queue->tail = NULL;
	}
	return thread;
}


struct thread *
thread_dequeue_id(struct thread_queue *q, thread_id thr_id)
{
	struct thread *t;
	struct thread *last = NULL;

	t = q->head;
	while (t != NULL) {
		if (t->id == thr_id) {
			if (last == NULL)
				q->head = t->queue_next;
			else
				last->queue_next = t->queue_next;

			if (q->tail == t)
				q->tail = last;
			break;
		}
		last = t;
		t = t->queue_next;
	}
	return t;
}


status_t
thread_init(kernel_args *ka)
{
	struct thread *t;
	unsigned int i;

	TRACE(("thread_init: entry\n"));

	// create the thread hash table
	sThreadHash = hash_init(15, (addr_t)&t->all_next - (addr_t)t,
		&thread_struct_compare, &thread_struct_hash);

	// zero out the dead thread structure q
	memset(&dead_q, 0, sizeof(dead_q));

	// allocate snooze sem
	sSnoozeSem = create_sem(0, "snooze sem");
	if (sSnoozeSem < 0) {
		panic("error creating snooze sem\n");
		return sSnoozeSem;
	}

	// create an idle thread for each cpu
	for (i = 0; i < ka->num_cpus; i++) {
		char temp[64];
		vm_region *region;

		sprintf(temp, "idle_thread%d", i);
		t = create_thread_struct(temp);
		if (t == NULL) {
			panic("error creating idle thread struct\n");
			return ENOMEM;
		}
		t->team = team_get_kernel_team();
		t->priority = B_IDLE_PRIORITY;
		t->state = B_THREAD_RUNNING;
		t->next_state = B_THREAD_READY;
		sprintf(temp, "idle_thread%d_kstack", i);
		t->kernel_stack_region_id = find_area(temp);
		region = vm_get_region_by_id(t->kernel_stack_region_id);
		if (!region)
			panic("error finding idle kstack region\n");

		t->kernel_stack_base = region->base;
		vm_put_region(region);
		hash_insert(sThreadHash, t);
		insert_thread_into_team(t->team, t);
		sIdleThreads[i] = t;
		if (i == 0)
			arch_thread_set_current_thread(t);
		t->cpu = &cpu[i];
	}

	// create a set of death stacks
	sNumDeathStacks = smp_get_num_cpus();
	if (sNumDeathStacks > 8*sizeof(sDeathStackBitmap)) {
		/*
		 * clamp values for really beefy machines
		 */
		sNumDeathStacks = 8*sizeof(sDeathStackBitmap);
	}
	sDeathStackBitmap = 0;
	sDeathStacks = (struct death_stack *)malloc(sNumDeathStacks * sizeof(struct death_stack));
	if (sDeathStacks == NULL) {
		panic("error creating death stacks\n");
		return B_NO_MEMORY;
	}
	{
		char temp[64];

		for (i = 0; i < sNumDeathStacks; i++) {
			sprintf(temp, "death_stack%d", i);
			sDeathStacks[i].area = create_area(temp, (void **)&sDeathStacks[i].address,
				B_ANY_KERNEL_ADDRESS, KSTACK_SIZE, B_FULL_LOCK, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
			if (sDeathStacks[i].area < 0) {
				panic("error creating death stacks\n");
				return sDeathStacks[i].area;
			}
			sDeathStacks[i].in_use = false;
		}
	}
	sDeathStackSem = create_sem(sNumDeathStacks, "death_stack_noavail_sem");

	// set up some debugger commands
	add_debugger_command("threads", &dump_thread_list, "list all threads");
	add_debugger_command("thread", &dump_thread_info, "list info about a particular thread");
	add_debugger_command("next_q", &dump_next_thread_in_q, "dump the next thread in the queue of last thread viewed");
	add_debugger_command("next_all", &dump_next_thread_in_all_list, "dump the next thread in the global list of the last thread viewed");
	add_debugger_command("next_team", &dump_next_thread_in_team, "dump the next thread in the team of the last thread viewed");

	return B_OK;
}


status_t
thread_per_cpu_init(int32 cpu_num)
{
	arch_thread_set_current_thread(sIdleThreads[cpu_num]);
	return B_OK;
}


thread_id
spawn_kernel_thread_etc(thread_func function, const char *name, int32 priority, void *arg, team_id team)
{
	return create_thread(name, team, (thread_entry_func)function, arg, NULL, priority, true);
}


//	#pragma mark -
//	public kernel exported functions


void
exit_thread(status_t returnValue)
{
	struct thread *thread = thread_get_current_thread();

	thread->exit.status = returnValue;
	thread->exit.reason = THREAD_RETURN_EXIT;

	send_signal_etc(thread->id, SIGKILLTHR, B_DO_NOT_RESCHEDULE);
}


status_t
kill_thread(thread_id id)
{
	status_t status = send_signal_etc(id, SIGKILLTHR, B_DO_NOT_RESCHEDULE);
	if (status < B_OK)
		return status;

	if (id != thread_get_current_thread()->id)
		wait_for_thread(id, NULL);

	return status;
}


static status_t
send_data_etc(thread_id tid, int32 code, const void *buffer, size_t buffer_size, int32 flags)
{
	struct thread *target;
	sem_id cached_sem;
	cpu_status state;
	status_t rv;
	cbuf *data;

	state = disable_interrupts();
	GRAB_THREAD_LOCK();
	target = thread_get_thread_struct_locked(tid);
	if (!target) {
		RELEASE_THREAD_LOCK();
		restore_interrupts(state);
		return B_BAD_THREAD_ID;
	}
	cached_sem = target->msg.write_sem;
	RELEASE_THREAD_LOCK();
	restore_interrupts(state);

	if (buffer_size > THREAD_MAX_MESSAGE_SIZE)
		return B_NO_MEMORY;

	rv = acquire_sem_etc(cached_sem, 1, flags, 0);
	if (rv == B_INTERRUPTED)
		// We got interrupted by a signal
		return rv;
	if (rv != B_OK)
		// Any other acquisition problems may be due to thread deletion
		return B_BAD_THREAD_ID;

	if (buffer_size > 0) {
		data = cbuf_get_chain(buffer_size);
		if (!data)
			return B_NO_MEMORY;
		rv = cbuf_user_memcpy_to_chain(data, 0, buffer, buffer_size);
		if (rv < 0) {
			cbuf_free_chain(data);
			return B_NO_MEMORY;
		}
	} else
		data = NULL;

	state = disable_interrupts();
	GRAB_THREAD_LOCK();

	// The target thread could have been deleted at this point
	target = thread_get_thread_struct_locked(tid);
	if (!target) {
		RELEASE_THREAD_LOCK();
		restore_interrupts(state);
		cbuf_free_chain(data);
		return B_BAD_THREAD_ID;
	}

	// Save message informations
	target->msg.sender = thread_get_current_thread()->id;
	target->msg.code = code;
	target->msg.size = buffer_size;
	target->msg.buffer = data;
	cached_sem = target->msg.read_sem;

	RELEASE_THREAD_LOCK();
	restore_interrupts(state);

	release_sem(cached_sem);

	return B_OK;
}


status_t
send_data(thread_id thread, int32 code, const void *buffer, size_t bufferSize)
{
	return send_data_etc(thread, code, buffer, bufferSize, 0);
}


static status_t
receive_data_etc(thread_id *_sender, void *buffer, size_t bufferSize, int32 flags)
{
	struct thread *thread = thread_get_current_thread();
	status_t status;
	size_t size;
	int32 code;

	status = acquire_sem_etc(thread->msg.read_sem, 1, flags, 0);
	if (status < B_OK)
		return status;

	if (buffer != NULL && bufferSize != 0) {
		size = min(bufferSize, thread->msg.size);
		status = cbuf_user_memcpy_from_chain(buffer, thread->msg.buffer, 0, size);
		if (status < B_OK) {
			cbuf_free_chain(thread->msg.buffer);
			release_sem(thread->msg.write_sem);
			return status;
		}
	}

	*_sender = thread->msg.sender;
	code = thread->msg.code;

	cbuf_free_chain(thread->msg.buffer);
	release_sem(thread->msg.write_sem);

	return code;
}


status_t
receive_data(thread_id *sender, void *buffer, size_t bufferSize)
{
	return receive_data_etc(sender, buffer, bufferSize, 0);
}


bool
has_data(thread_id thread)
{
	int32 count;

	if (get_sem_count(thread_get_current_thread()->msg.read_sem, &count) != B_OK)
		return false;

	return count == 0 ? false : true;
}


/** Fills the thread_info structure with information from the specified
 *	thread.
 *	The thread lock must be held when called.
 */

static void
fill_thread_info(struct thread *thread, thread_info *info, size_t size)
{
	info->thread = thread->id;
	info->team = thread->team->id;

	strlcpy(info->name, thread->name, B_OS_NAME_LENGTH);

	if (thread->state == B_THREAD_WAITING) {
		if (thread->sem.blocking == sSnoozeSem)
			info->state = B_THREAD_ASLEEP;
		else if (thread->sem.blocking == thread->msg.read_sem)
			info->state = B_THREAD_RECEIVING;
		else
			info->state = B_THREAD_WAITING;
	} else
		info->state = thread->state;

	info->priority = thread->priority;
	info->sem = thread->sem.blocking;
	info->user_time = thread->user_time;
	info->kernel_time = thread->kernel_time;
	info->stack_base = (void *)thread->user_stack_base;
	info->stack_end = (void *)(thread->user_stack_base + STACK_SIZE);
}


status_t
_get_thread_info(thread_id id, thread_info *info, size_t size)
{
	status_t status = B_OK;
	struct thread *thread;
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
_get_next_thread_info(team_id team, int32 *_cookie, thread_info *info, size_t size)
{
	status_t status = B_BAD_VALUE;
	struct thread *thread = NULL;
	cpu_status state;
	int slot;

	if (info == NULL || size != sizeof(thread_info) || team < B_OK)
		return B_BAD_VALUE;

	if (team == B_CURRENT_TEAM)
		team = team_get_current_team_id();
	else if (!team_is_valid(team))
		return B_BAD_VALUE;

	slot = *_cookie;

	state = disable_interrupts();
	GRAB_THREAD_LOCK();

	if (slot >= sNextThreadID)
		goto err;

	while (slot < sNextThreadID
		&& (!(thread = thread_get_thread_struct_locked(slot)) || thread->team->id != team))
		slot++;

	if (thread != NULL && thread->team->id == team) {
		fill_thread_info(thread, info, size);

		*_cookie = slot + 1;
		status = B_OK;
	}

err:
	RELEASE_THREAD_LOCK();
	restore_interrupts(state);

	return status;
}


thread_id
find_thread(const char *name)
{
	struct hash_iterator iterator;
	struct thread *thread;
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
	while ((thread = hash_next(sThreadHash, &iterator)) != NULL) {
		// Search through hash
		if (thread->name != NULL && !strcmp(thread->name, name)) {
			thread_id id = thread->id;

			RELEASE_THREAD_LOCK();
			restore_interrupts(state);
			return id;
		}
	}

	RELEASE_THREAD_LOCK();
	restore_interrupts(state);

	return B_ENTRY_NOT_FOUND;
}


status_t
rename_thread(thread_id id, const char *name)
{
	status_t status = B_OK;
	struct thread *thread;

	if (name == NULL)
		return B_BAD_VALUE;

	thread = thread_get_current_thread();
	if (thread->id == id) {
		// it's ourself, so we know we aren't in the run queue, and we can manipulate
		// our structure directly
		strlcpy(thread->name, name, B_OS_NAME_LENGTH);
	} else {
		cpu_status state = disable_interrupts();
		GRAB_THREAD_LOCK();

		thread = thread_get_thread_struct_locked(id);
		if (thread) {
			strlcpy(thread->name, name, B_OS_NAME_LENGTH);
		} else
			status = B_BAD_THREAD_ID;

		RELEASE_THREAD_LOCK();
		restore_interrupts(state);
	}

	return status;
}


status_t
set_thread_priority(thread_id id, int32 priority)
{
	struct thread *thread;
	int32 oldPriority;

	// make sure the passed in priority is within bounds
	if (priority > B_MAX_PRIORITY)
		priority = B_MAX_PRIORITY;
	if (priority < B_MIN_PRIORITY)
		priority = B_MIN_PRIORITY;

	thread = thread_get_current_thread();
	if (thread->id == id) {
		// it's ourself, so we know we aren't in the run queue, and we can manipulate
		// our structure directly
		oldPriority = thread->priority;
			// note that this might not return the correct value if we are preempted
			// here, and another thread changes our priority before the next line is
			// executed
		thread->priority = priority;
	} else {
		cpu_status state = disable_interrupts();
		GRAB_THREAD_LOCK();

		thread = thread_get_thread_struct_locked(id);
		if (thread) {
			oldPriority = thread->priority;
			if (thread->state == B_THREAD_READY && thread->priority != priority) {
				// if the thread is in the run queue, we reinsert it at a new position
				// ToDo: this doesn't seem to be necessary: if a thread is already in
				//	the run queue, running it once doesn't hurt, and if the priority
				//	is now very low, it probably wouldn't have been selected to be in
				//	the run queue at all right now, anyway.
				scheduler_remove_from_run_queue(thread);
				thread->priority = priority;
				scheduler_enqueue_in_run_queue(thread);
			} else
				thread->priority = priority;
		} else
			oldPriority = B_BAD_THREAD_ID;

		RELEASE_THREAD_LOCK();
		restore_interrupts(state);
	}

	return oldPriority;
}


status_t
snooze_etc(bigtime_t timeout, int timebase, uint32 flags)
{
	status_t status;

	if (timebase != B_SYSTEM_TIMEBASE)
		return B_BAD_VALUE;

	status = acquire_sem_etc(sSnoozeSem, 1, B_ABSOLUTE_TIMEOUT | flags, timeout);
	if (status == B_TIMED_OUT || status == B_WOULD_BLOCK)
		return B_OK;

	return status;
}


/** snooze() for internal kernel use only; doesn't interrupt on signals.
 */

status_t
snooze(bigtime_t timeout)
{
	return snooze_etc(timeout, B_SYSTEM_TIMEBASE, B_RELATIVE_TIMEOUT);
}


/** snooze_until() for internal kernel use only; doesn't interrupt on signals.
 */

status_t
snooze_until(bigtime_t timeout, int timebase)
{
	return snooze_etc(timeout, timebase, B_ABSOLUTE_TIMEOUT);
}


status_t
wait_for_thread(thread_id id, status_t *_returnCode)
{
	sem_id sem = B_BAD_THREAD_ID;
	struct death_entry death;
	struct thread *thread;
	cpu_status state;
	status_t status;

	// we need to resume the thread we're waiting for first
	status = resume_thread(id);
	if (status != B_OK)
		return status;

	state = disable_interrupts();
	GRAB_THREAD_LOCK();

	thread = thread_get_thread_struct_locked(id);
	if (thread != NULL) {
		// remember the semaphore we have to wait on and place our death entry
		sem = thread->exit.sem;
		list_add_link_to_head(&thread->exit.waiters, &death);
	}

	RELEASE_THREAD_LOCK();
	restore_interrupts(state);

	if (sem < B_OK)
		return B_BAD_THREAD_ID;

	status = acquire_sem(sem);

	if (status == B_OK) {
		// this should never happen as the thread deletes the semaphore on exit
		panic("could acquire exit_sem for thread %lx\n", id);
	} else if (status == B_BAD_SEM_ID) {
		// this is the way the thread normally exits
		status = B_OK;

		if (_returnCode)
			*_returnCode = death.status;
	} else {
		// We were probably interrupted; we need to remove our death entry now.
		// When the thread is already gone, we don't have to care

		state = disable_interrupts();
		GRAB_THREAD_LOCK();

		thread = thread_get_thread_struct_locked(id);
		if (thread != NULL)
			list_remove_link(&death);

		RELEASE_THREAD_LOCK();
		restore_interrupts(state);
	}

	return status;
}


status_t
suspend_thread(thread_id id)
{
	return send_signal(id, SIGSTOP);
}


status_t
resume_thread(thread_id id)
{
	return send_signal(id, SIGCONT);
}


thread_id
spawn_kernel_thread(thread_func function, const char *name, int32 priority, void *arg)
{
	return create_thread(name, team_get_kernel_team()->id, (thread_entry_func)function,
				arg, NULL, priority, true);
}


int
getrlimit(int resource, struct rlimit * rlp)
{
	if (!rlp)
		return -1;

	switch (resource) {
		case RLIMIT_NOFILE:
			return vfs_getrlimit(resource, rlp);

		default:
			return -1;
	}

	return 0;
}


int
setrlimit(int resource, const struct rlimit * rlp)
{
	if (!rlp)
		return -1;

	switch (resource) {
		case RLIMIT_NOFILE:
			return vfs_setrlimit(resource, rlp);

		default:
			return -1;
	}

	return 0;
}


//	#pragma mark -
//	Calls from userland (with extra address checks)


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
_user_spawn_thread(int32 (*entry)(thread_func, void *), const char *userName, int32 priority, void *data1, void *data2)
{
	char name[B_OS_NAME_LENGTH];

	if (!IS_USER_ADDRESS(entry) || entry == NULL
		|| !IS_USER_ADDRESS(userName)
		|| user_strlcpy(name, userName, B_OS_NAME_LENGTH) < B_OK)
		return B_BAD_ADDRESS;

	return create_thread(name, thread_get_current_thread()->team->id, entry, data1, data2, priority, false);
}


status_t
_user_snooze_etc(bigtime_t timeout, int timebase, uint32 flags)
{
	return snooze_etc(timeout, timebase, flags | B_CAN_INTERRUPT);
}


status_t
_user_get_thread_info(thread_id id, thread_info *userInfo)
{
	thread_info info;
	status_t status;

	if (!IS_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;

	status = _get_thread_info(id, &info, sizeof(thread_info));

	if (status >= B_OK && user_memcpy(userInfo, &info, sizeof(thread_info)) < B_OK)
		return B_BAD_ADDRESS;

	return status;
}


status_t
_user_get_next_thread_info(team_id team, int32 *userCookie, thread_info *userInfo)
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

	if (!IS_USER_ADDRESS(userReturnCode))
		return B_BAD_ADDRESS;

	status = wait_for_thread(id, &returnCode);

	if (status == B_OK && userReturnCode != NULL
		&& user_memcpy(userReturnCode, &returnCode, sizeof(status_t)) < B_OK)
		return B_BAD_ADDRESS;

	return status;
}


bool
_user_has_data(thread_id thread)
{
	return has_data(thread);
}


status_t
_user_send_data(thread_id thread, int32 code, const void *buffer, size_t bufferSize)
{
	if (!IS_USER_ADDRESS(buffer))
		return B_BAD_ADDRESS;

	return send_data_etc(thread, code, buffer, bufferSize, B_CAN_INTERRUPT);
		// supports userland buffers
}


status_t
_user_receive_data(thread_id *_userSender, void *buffer, size_t bufferSize)
{
	thread_id sender;
	status_t code;

	if (!IS_USER_ADDRESS(_userSender)
		|| !IS_USER_ADDRESS(buffer))
		return B_BAD_ADDRESS;

	code = receive_data_etc(&sender, buffer, bufferSize, B_CAN_INTERRUPT);
		// supports userland buffers

	if (user_memcpy(_userSender, &sender, sizeof(thread_id)) < B_OK)
		return B_BAD_ADDRESS;

	return code;
}


// ToDo: the following two functions don't belong here


int
_user_getrlimit(int resource, struct rlimit *urlp)
{
	struct rlimit rl;
	int ret;

	if (urlp == NULL)
		return EINVAL;

	if (!IS_USER_ADDRESS(urlp))
		return B_BAD_ADDRESS;

	ret = getrlimit(resource, &rl);

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
		|| user_memcpy(&resourceLimit, userResourceLimit, sizeof(struct rlimit)) < B_OK)
		return B_BAD_ADDRESS;

	return setrlimit(resource, &resourceLimit);
}

