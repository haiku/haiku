/* Threading routines */

/*
** Copyright 2002-2004, The OpenBeOS Team. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
**
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

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
static struct thread *idle_threads[MAX_BOOT_CPUS];
static void *thread_hash = NULL;
static thread_id next_thread_id = 1;

static sem_id gSnoozeSem = -1;

// death stacks - used temporarily as a thread cleans itself up
struct death_stack {
	region_id rid;
	addr address;
	bool in_use;
};
static struct death_stack *death_stacks;
static unsigned int num_death_stacks;
static unsigned int volatile death_stack_bitmap;
static sem_id death_stack_sem;

// The dead queue is used as a pool from which to retrieve and reuse previously
// allocated thread structs when creating a new thread. It should be gone once
// the slab allocator is in.
struct thread_queue dead_q;

static void thread_kthread_entry(void);
static void thread_kthread_exit(void);


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
			goto err;
	}

	strlcpy(t->name, name, B_OS_NAME_LENGTH);

	t->id = atomic_add(&next_thread_id, 1);
	t->team = NULL;
	t->cpu = NULL;
	t->sem_blocking = -1;
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
	t->return_code = 0;
	t->return_flags = 0;

	sprintf(temp, "thread_0x%lx_retcode_sem", t->id);
	t->return_code_sem = create_sem(0, temp);
	if (t->return_code_sem < 0)
		goto err1;

	sprintf(temp, "%s data write sem", t->name);
	t->msg.write_sem = create_sem(1, temp);
	if (t->msg.write_sem < 0)
		goto err2;

	sprintf(temp, "%s data read sem", t->name);
	t->msg.read_sem = create_sem(0, temp);
	if (t->msg.read_sem < 0)
		goto err3;

	if (arch_thread_init_thread_struct(t) < 0)
		goto err4;

	return t;

err4:
	delete_sem_etc(t->msg.read_sem, -1, false);
err3:
	delete_sem_etc(t->msg.write_sem, -1, false);
err2:
	delete_sem_etc(t->return_code_sem, -1, false);
err1:
	free(t);
err:
	return NULL;
}


static void
delete_thread_struct(struct thread *t)
{
	if (t->return_code_sem >= 0)
		delete_sem_etc(t->return_code_sem, -1, false);
	if (t->msg.write_sem >= 0)
		delete_sem_etc(t->msg.write_sem, -1, false);
	if (t->msg.read_sem >= 0)
		delete_sem_etc(t->msg.read_sem, -1, false);
	free(t);
}


/** Initializes the thread and jumps to its userspace entry point.
 *	This function is called at creation time of every user thread,
 *	but not for the team's main threads.
 */

static int
_create_user_thread_kentry(void)
{
	struct thread *thread = thread_get_current_thread();

	// a signal may have been delivered here
//	thread_atkernel_exit();
	// start tracking kernel & user time
	thread->last_time = system_time();
	thread->last_time_type = KERNEL_TIME;
	thread->in_kernel = false;

	// jump to the entry point in user space
	arch_thread_enter_uspace(thread, (addr)thread->entry, thread->args1, thread->args2);

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
create_thread(const char *name, team_id teamID, thread_func entry, void *args1, void *args2, int32 priority, bool kernel)
{
	struct thread *t;
	struct team *team;
	cpu_status state;
	char stack_name[64];
	bool abort = false;
	addr mainThreadStackBase = USER_STACK_REGION + USER_STACK_REGION_SIZE;

	t = create_thread_struct(name);
	if (t == NULL)
		return ENOMEM;

	t->priority = priority == -1 ? B_NORMAL_PRIORITY : priority;
	// ToDo: revisit this one - the thread might go to early to B_THREAD_SUSPENDED
//	t->state = THREAD_STATE_BIRTH;
	t->state = B_THREAD_SUSPENDED;	// Is this right?
	t->next_state = B_THREAD_SUSPENDED;

	state = disable_interrupts();
	GRAB_THREAD_LOCK();

	// insert into global list
	hash_insert(thread_hash, t);
	RELEASE_THREAD_LOCK();

	GRAB_TEAM_LOCK();
	// look at the team, make sure it's not being deleted
	team = team_get_team_struct_locked(teamID);
	if (team != NULL && team->state != TEAM_STATE_DEATH)
		insert_thread_into_team(team, t);
	else
		abort = true;

	if (!kernel && team->main_thread)
		mainThreadStackBase = team->main_thread->user_stack_base;

	RELEASE_TEAM_LOCK();
	if (abort) {
		GRAB_THREAD_LOCK();
		hash_remove(thread_hash, t);
		RELEASE_THREAD_LOCK();
	}
	restore_interrupts(state);
	if (abort) {
		delete_thread_struct(t);
		return B_BAD_TEAM_ID;
	}

	sprintf(stack_name, "%s_kstack", name);
	t->kernel_stack_region_id = create_area(stack_name, (void **)&t->kernel_stack_base,
		B_ANY_KERNEL_ADDRESS, KSTACK_SIZE, B_FULL_LOCK, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

	if (t->kernel_stack_region_id < 0)
		panic("_create_thread: error creating kernel stack!\n");

	t->args1 = args1;
	t->args2 = args2;
	t->entry = entry;

	if (kernel) {
		// this sets up an initial kthread stack that runs the entry

		// Note: whatever function wants to set up a user stack later for this thread
		// must initialize the TLS for it
		arch_thread_init_kthread_stack(t, &_create_kernel_thread_kentry, &thread_kthread_entry, &thread_kthread_exit);
	} else {
		// create user stack

		// ToDo: make this better. For now just keep trying to create a stack
		//		until we find a spot.
		//		-> implement B_BASE_ADDRESS for create_area() and use that one!
		//		(we can then also eliminate the mainThreadStackBase variable)

		// ToDo: should we move the stack creation to _create_user_thread_kentry()?

		// the stack will be between USER_STACK_REGION and the main thread stack area
		// (the user stack of the main thread is created in team_create_team())
		t->user_stack_base = USER_STACK_REGION;

		while (t->user_stack_base < mainThreadStackBase) {
			sprintf(stack_name, "%s_stack%ld", team->name, t->id);
			t->user_stack_region_id = create_area_etc(team, stack_name,
				(void **)&t->user_stack_base, B_EXACT_ADDRESS,
				STACK_SIZE + TLS_SIZE, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
			if (t->user_stack_region_id >= 0)
				break;

			t->user_stack_base += STACK_SIZE + TLS_SIZE;
		}
		if (t->user_stack_region_id < 0)
			panic("_create_thread: unable to create user stack!\n");

		// now that the TLS area is allocated, initialize TLS
		arch_thread_init_tls(t);

		// copy the user entry over to the args field in the thread struct
		// the function this will call will immediately switch the thread into
		// user space.
		arch_thread_init_kthread_stack(t, &_create_user_thread_kentry, &thread_kthread_entry, &thread_kthread_exit);
	}

	t->state = B_THREAD_SUSPENDED;

	return t->id;
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
	dprintf("priority:    0x%x\n", t->priority);
	dprintf("state:       %s\n", state_to_text(t->state));
	dprintf("next_state:  %s\n", state_to_text(t->next_state));
	dprintf("cpu:         %p ", t->cpu);
	if (t->cpu)
		dprintf("(%d)\n", t->cpu->info.cpu_num);
	else
		dprintf("\n");
	dprintf("sig_pending:  0x%lx\n", t->sig_pending);
	dprintf("in_kernel:    %d\n", t->in_kernel);
	dprintf("sem_blocking: 0x%lx\n", t->sem_blocking);
	dprintf("sem_count:    0x%x\n", t->sem_count);
	dprintf("sem_deleted_retcode: 0x%x\n", t->sem_deleted_retcode);
	dprintf("sem_errcode:  0x%x\n", t->sem_errcode);
	dprintf("sem_flags:    0x%x\n", t->sem_flags);
	dprintf("fault_handler: %p\n", (void *)t->fault_handler);
	dprintf("args:         %p %p\n", t->args1, t->args2);
	dprintf("entry:        %p\n", (void *)t->entry);
	dprintf("team:         %p\n", t->team);
	dprintf("return_code_sem: 0x%lx\n", t->return_code_sem);
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
	struct thread *t;
	int id = -1;
	struct hash_iterator i;

	if (argc < 2) {
		dprintf("thread: not enough arguments\n");
		return 0;
	}

	// if the argument looks like a hex number, treat it as such
	if (strlen(argv[1]) > 2 && argv[1][0] == '0' && argv[1][1] == 'x')
		id = strtoul(argv[1], NULL, 16);
	else
		id = atoi(argv[1]);

	// walk through the thread list, trying to match name or id
	hash_open(thread_hash, &i);
	while ((t = hash_next(thread_hash, &i)) != NULL) {
		if ((t->name && strcmp(argv[1], t->name) == 0) || t->id == id) {
			_dump_thread_info(t);
			break;
		}
	}
	hash_close(thread_hash, &i, false);
	return 0;
}


static int
dump_thread_list(int argc, char **argv)
{
	struct thread *t;
	struct hash_iterator i;

	hash_open(thread_hash, &i);
	while ((t = hash_next(thread_hash, &i)) != NULL) {
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
	hash_close(thread_hash, &i, false);
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


static int
get_death_stack(void)
{
	cpu_status state;
	unsigned int bit;
	int i;

	acquire_sem(death_stack_sem);

	// grap the thread lock, find a free spot and release
	state = disable_interrupts();
	GRAB_THREAD_LOCK();
	bit = death_stack_bitmap;
	bit = (~bit)&~((~bit)-1);
	death_stack_bitmap |= bit;
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

	TRACE(("get_death_stack: returning 0x%lx\n", death_stacks[i].address));

	return i;
}


static void
put_death_stack_and_reschedule(unsigned int index)
{
	TRACE(("put_death_stack...: passed %d\n", index));

	if (index >= num_death_stacks)
		panic("put_death_stack: passed invalid stack index %d\n", index);

	if (!(death_stack_bitmap & (1 << index)))
		panic("put_death_stack: passed invalid stack index %d\n", index);

	disable_interrupts();
	GRAB_THREAD_LOCK();

	death_stack_bitmap &= ~(1 << index);

	release_sem_etc(death_stack_sem, 1, B_DO_NOT_RESCHEDULE);

	scheduler_reschedule();
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


// used to pass messages between thread_exit and thread_exit2

struct thread_exit_args {
	struct thread *t;
	region_id old_kernel_stack;
	int int_state;
	unsigned int death_stack;
	sem_id death_sem;
};


static void
thread_exit2(void *_args)
{
	struct thread_exit_args args;

	// copy the arguments over, since the source is probably on the kernel stack we're about to delete
	memcpy(&args, _args, sizeof(struct thread_exit_args));

	// restore the interrupts
	enable_interrupts();
	// ToDo: was:
	// restore_interrupts(args.int_state);
	// we probably don't want to let the interrupts disabled at this point??

	TRACE(("thread_exit2, running on death stack 0x%lx\n", args.t->kernel_stack_base));

	// delete the old kernel stack region
	TRACE(("thread_exit2: deleting old kernel stack id 0x%lx for thread 0x%lx\n",
		args.old_kernel_stack, args.t->id));

	delete_area(args.old_kernel_stack);

	// remove this thread from all of the global lists
	TRACE(("thread_exit2: removing thread 0x%lx from global lists\n", args.t->id));

	disable_interrupts();
	GRAB_TEAM_LOCK();
	remove_thread_from_team(team_get_kernel_team(), args.t);
	RELEASE_TEAM_LOCK();
	GRAB_THREAD_LOCK();
	hash_remove(thread_hash, args.t);
	RELEASE_THREAD_LOCK();

	// ToDo: is this correct at this point?
	//restore_interrupts(args.int_state);

	TRACE(("thread_exit2: done removing thread from lists\n"));

	if (args.death_sem >= 0)
		release_sem_etc(args.death_sem, 1, B_DO_NOT_RESCHEDULE);	

	// set the next state to be gone. Will return the thread structure to a ready pool upon reschedule
	args.t->next_state = THREAD_STATE_FREE_ON_RESCHED;

	// return the death stack and reschedule one last time
	put_death_stack_and_reschedule(args.death_stack);
	// never get to here
	panic("thread_exit2: made it where it shouldn't have!\n");
}


void
thread_exit(void)
{
	cpu_status state;
	struct thread *t = thread_get_current_thread();
	struct team *team = t->team;
	thread_id mainParentThread = -1;
	bool delete_team = false;
	unsigned int death_stack;
	sem_id cached_death_sem;
	status_t status;

	if (!are_interrupts_enabled())
		dprintf("thread_exit() called with interrupts disabled!\n");

	TRACE(("thread 0x%lx exiting %s w/return code 0x%x\n", t->id,
		t->return_flags & THREAD_RETURN_INTERRUPTED ? "due to signal" : "normally",
		(int)t->return_code));

	// boost our priority to get this over with
	t->priority = B_FIRST_REAL_TIME_PRIORITY;
		// ToDo: is it really that urgent?

	// shutdown the thread messaging

	status = acquire_sem_etc(t->msg.write_sem, 1, B_RELATIVE_TIMEOUT, 0);
	if (status == B_WOULD_BLOCK) {
		// there is data waiting for us, so let us eat it
		thread_id sender;

		delete_sem(t->msg.write_sem);
			// first, let's remove all possibly waiting writers
		receive_data_etc(&sender, NULL, 0, B_RELATIVE_TIMEOUT);
	} else {
		// we probably own the semaphore here, and we're the last to do so
		delete_sem(t->msg.write_sem);
	}
	// now we can safely remove the msg.read_sem
	delete_sem(t->msg.read_sem);

	// Cancel previously installed alarm timer, if any
	cancel_timer(&t->alarm);
	
	// delete the user stack region first
	if (team->_aspace_id >= 0 && t->user_stack_region_id >= 0) {
		region_id rid = t->user_stack_region_id;
		t->user_stack_region_id = -1;
		delete_area_etc(team, rid);
	}

	if (team != team_get_kernel_team()) {
		// remove this thread from the current team and add it to the kernel
		// put the thread into the kernel team until it dies
		state = disable_interrupts();
		GRAB_TEAM_LOCK();

		remove_thread_from_team(team, t);
		insert_thread_into_team(team_get_kernel_team(), t);

		if (team->main_thread == t) {
			// this was main thread in this team
			delete_team = true;

			// remember who our parent was so we can send a signal
			mainParentThread = team->parent->main_thread->id;

			team_remove_team(team);
		}
		RELEASE_TEAM_LOCK();
		// swap address spaces, to make sure we're running on the kernel's pgdir
		vm_aspace_swap(team_get_kernel_team()->kaspace);
		restore_interrupts(state);

		TRACE(("thread_exit: thread 0x%lx now a kernel thread!\n", t->id));
	}

	// delete the team if we're its main thread
	if (delete_team) {
		team_delete_team(team);

		send_signal_etc(mainParentThread, SIGCHLD, B_DO_NOT_RESCHEDULE);
		cached_death_sem = -1;
	} else
		cached_death_sem = team->death_sem;

	// delete the sem that others will use to wait on us and get the retcode
	{
		sem_id s = t->return_code_sem;

		t->return_code_sem = -1;
		delete_sem_etc(s, t->return_code, t->return_flags & THREAD_RETURN_INTERRUPTED ? true : false);
	}

	death_stack = get_death_stack();
	{
		struct thread_exit_args args;

		args.t = t;
		args.old_kernel_stack = t->kernel_stack_region_id;
		args.death_stack = death_stack;
		args.death_sem = cached_death_sem;

		// disable the interrupts. Must remain disabled until the kernel stack pointer can be officially switched
		args.int_state = disable_interrupts();

		// set the new kernel stack officially to the death stack, wont be really switched until
		// the next function is called. This bookkeeping must be done now before a context switch
		// happens, or the processor will interrupt to the old stack
		t->kernel_stack_region_id = death_stacks[death_stack].rid;
		t->kernel_stack_base = death_stacks[death_stack].address;

		// we will continue in thread_exit2(), on the new stack
		arch_thread_switch_kstack_and_call(t, t->kernel_stack_base + KSTACK_SIZE, thread_exit2, &args);
	}

	panic("never can get here\n");
}


int
thread_kill_thread_nowait(thread_id id)
{
	return send_signal_etc(id, SIGKILLTHR, B_DO_NOT_RESCHEDULE);
}


static void
thread_kthread_exit(void)
{
	struct thread *t = thread_get_current_thread();
	
	t->return_flags = THREAD_RETURN_EXIT;
	thread_exit();
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

	return hash_lookup(thread_hash, &key);
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
	int global_resched;
	struct thread *t;
	bigtime_t now;

	TRACE(("thread_atkernel_exit: entry\n"));

	t = thread_get_current_thread();

	state = disable_interrupts();
	GRAB_THREAD_LOCK();

	global_resched = handle_signals(t, state);

	t->in_kernel = false;

	// track kernel time
	now = system_time();
	t->kernel_time += now - t->last_time;
	t->last_time = now;
	t->last_time_type = USER_TIME;

	RELEASE_THREAD_LOCK();
	restore_interrupts(state);

	if (global_resched)
		smp_send_broadcast_ici(SMP_MSG_RESCHEDULE, 0, 0, 0, NULL, SMP_MSG_FLAG_SYNC);
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


int
thread_init(kernel_args *ka)
{
	struct thread *t;
	unsigned int i;

	TRACE(("thread_init: entry\n"));

	// create the thread hash table
	thread_hash = hash_init(15, (addr)&t->all_next - (addr)t,
		&thread_struct_compare, &thread_struct_hash);

	// zero out the dead thread structure q
	memset(&dead_q, 0, sizeof(dead_q));

	// allocate snooze sem
	gSnoozeSem = create_sem(0, "snooze sem");
	if (gSnoozeSem < 0) {
		panic("error creating snooze sem\n");
		return gSnoozeSem;
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
		hash_insert(thread_hash, t);
		insert_thread_into_team(t->team, t);
		idle_threads[i] = t;
		if (i == 0)
			arch_thread_set_current_thread(t);
		t->cpu = &cpu[i];
	}

	// create a set of death stacks
	num_death_stacks = smp_get_num_cpus();
	if (num_death_stacks > 8*sizeof(death_stack_bitmap)) {
		/*
		 * clamp values for really beefy machines
		 */
		num_death_stacks = 8*sizeof(death_stack_bitmap);
	}
	death_stack_bitmap = 0;
	death_stacks = (struct death_stack *)malloc(num_death_stacks * sizeof(struct death_stack));
	if (death_stacks == NULL) {
		panic("error creating death stacks\n");
		return ENOMEM;
	}
	{
		char temp[64];

		for (i = 0; i < num_death_stacks; i++) {
			sprintf(temp, "death_stack%d", i);
			death_stacks[i].rid = create_area(temp, (void **)&death_stacks[i].address,
				B_ANY_KERNEL_ADDRESS, KSTACK_SIZE, B_FULL_LOCK, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
			if (death_stacks[i].rid < 0) {
				panic("error creating death stacks\n");
				return death_stacks[i].rid;
			}
			death_stacks[i].in_use = false;
		}
	}
	death_stack_sem = create_sem(num_death_stacks, "death_stack_noavail_sem");

	// set up some debugger commands
	add_debugger_command("threads", &dump_thread_list, "list all threads");
	add_debugger_command("thread", &dump_thread_info, "list info about a particular thread");
	add_debugger_command("next_q", &dump_next_thread_in_q, "dump the next thread in the queue of last thread viewed");
	add_debugger_command("next_all", &dump_next_thread_in_all_list, "dump the next thread in the global list of the last thread viewed");
	add_debugger_command("next_team", &dump_next_thread_in_team, "dump the next thread in the team of the last thread viewed");

	return 0;
}


int
thread_init_percpu(int cpu_num)
{
	arch_thread_set_current_thread(idle_threads[cpu_num]);
	return 0;
}


thread_id
spawn_kernel_thread_etc(thread_func function, const char *name, int32 priority, void *arg, team_id team)
{
	return create_thread(name, team, function, arg, NULL, priority, true);
}


//	#pragma mark -
//	public kernel exported functions


status_t
kill_thread(thread_id id)
{
	status_t status = send_signal_etc(id, SIGKILLTHR, B_DO_NOT_RESCHEDULE);
	if (status < 0)
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
		if (thread->sem_blocking == gSnoozeSem)
			info->state = B_THREAD_ASLEEP;
		else if (thread->sem_blocking == thread->msg.read_sem)
			info->state = B_THREAD_RECEIVING;
		else
			info->state = B_THREAD_WAITING;
	} else
		info->state = thread->state;

	info->priority = thread->priority;
	info->sem = thread->sem_blocking;
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

	if (slot >= next_thread_id)
		goto err;

	while (slot < next_thread_id
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
	if (name == NULL)
		return thread_get_current_thread_id();

	// ToDo: implement me!

	return B_ENTRY_NOT_FOUND;
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

	status = acquire_sem_etc(gSnoozeSem, 1, B_ABSOLUTE_TIMEOUT | flags, timeout);
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
	sem_id sem;
	cpu_status state;
	struct thread *t;
	int rc;

	rc = send_signal_etc(id, SIGCONT, 0);
	if (rc != B_OK)
		return rc;
	
	state = disable_interrupts();
	GRAB_THREAD_LOCK();

	t = thread_get_thread_struct_locked(id);
	sem = t != NULL ? t->return_code_sem : B_BAD_THREAD_ID;

	RELEASE_THREAD_LOCK();
	restore_interrupts(state);

	rc = acquire_sem(sem);

	/* This thread died the way it should, dont ripple a non-error up */
	if (rc == B_BAD_SEM_ID) {
		rc = B_NO_ERROR;

		if (_returnCode) {
			t = thread_get_current_thread();
			TRACE(("wait_for_thread: thread %ld got return code 0x%x\n",
				t->id, t->sem_deleted_retcode));
			*_returnCode = t->sem_deleted_retcode;
		}
	}

	return rc;
}


status_t
suspend_thread(thread_id id)
{
	return send_signal_etc(id, SIGSTOP, B_DO_NOT_RESCHEDULE);
}


status_t
resume_thread(thread_id id)
{
	return send_signal_etc(id, SIGCONT, B_DO_NOT_RESCHEDULE);
}


thread_id
spawn_kernel_thread(thread_func function, const char *name, int32 priority, void *arg)
{
	return create_thread(name, team_get_kernel_team()->id, function, arg, NULL, priority, true);
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
_user_exit_thread(status_t return_value)
{
	struct thread *thread = thread_get_current_thread();

	thread->return_code = return_value;
	thread->return_flags = THREAD_RETURN_EXIT;

	send_signal_etc(thread->id, SIGKILLTHR, B_DO_NOT_RESCHEDULE);
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


int32
_user_set_thread_priority(thread_id thread, int32 newPriority)
{
	return set_thread_priority(thread, newPriority);
}


thread_id
_user_spawn_thread(thread_func entry, const char *userName, int32 priority, void *data1, void *data2)
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

	if (userReturnCode && user_memcpy(userReturnCode, &returnCode, sizeof(status_t)) < B_OK)
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

