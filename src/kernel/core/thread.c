/* Threading routines */

/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <kernel.h>
#include <debug.h>
#include <console.h>
#include <thread.h>
#include <arch/thread.h>
#include <khash.h>
#include <int.h>
#include <smp.h>
#include <timer.h>
#include <cpu.h>
#include <arch/int.h>
#include <arch/cpu.h>
#include <arch/vm.h>
#include <OS.h>
#include <sem.h>
#include <port.h>
#include <vfs.h>
#include <elf.h>
#include <memheap.h>
#include <user_runtime.h>
#include <Errors.h>
#include <stage2.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <resource.h>
#include <atomic.h>
#include <kerrors.h>
#include <syscalls.h>

#define THREAD_MAX_MESSAGE_SIZE		65536

struct thread_key {
	thread_id id;
};

// global
spinlock thread_spinlock = 0;

// thread list
static struct thread *idle_threads[MAX_BOOT_CPUS];
static void *thread_hash = NULL;
static thread_id next_thread_id = 1;

static sem_id snooze_sem = -1;

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

#ifndef NEW_SCHEDULER
// thread queues
// Thread priority has a granularity of 2; this means that we have 61 real
// priority levels: 60 to map BeOS priorities 1-120, plus the idle priority (0).
static struct thread_queue run_q[(B_MAX_PRIORITY / 2) + 1] = { { NULL, NULL }, };
#else /* NEW_SCHEDULER */
// The dead queue is used as a pool from which to retrieve and reuse previously
// allocated thread structs when creating a new thread. It should be gone once
// the slab allocator is in.
#endif /* NEW_SCHEDULER */
struct thread_queue dead_q;

static void thread_kthread_entry(void);
static void thread_kthread_exit(void);
//static void deliver_signal(struct thread *t, int signal);


// insert a thread onto the tail of a queue
void
thread_enqueue(struct thread *t, struct thread_queue *q)
{
	t->q_next = NULL;
	if(q->head == NULL) {
		q->head = t;
		q->tail = t;
	} else {
		q->tail->q_next = t;
		q->tail = t;
	}
}


struct thread *
thread_lookat_queue(struct thread_queue *q)
{
	return q->head;
}


struct thread *
thread_dequeue(struct thread_queue *q)
{
	struct thread *t;

	t = q->head;
	if (t != NULL) {
		q->head = t->q_next;
		if(q->tail == t)
			q->tail = NULL;
	}
	return t;
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
				q->head = t->q_next;
			else
				last->q_next = t->q_next;

			if (q->tail == t)
				q->tail = last;
			break;
		}
		last = t;
		t = t->q_next;
	}
	return t;
}

#ifndef NEW_SCHEDULER
struct thread *
thread_lookat_run_q(int priority)
{
	return thread_lookat_queue(&run_q[(priority + 1) >> 1]);
}


void
thread_enqueue_run_q(struct thread *t)
{
	// these shouldn't exist
	if (t->priority > B_MAX_PRIORITY)
		t->priority = B_MAX_PRIORITY;
	else if (t->priority < B_MIN_PRIORITY)
		t->priority = B_MIN_PRIORITY;

	thread_enqueue(t, &run_q[(t->priority + 1) >> 1]);
}


struct thread *
thread_dequeue_run_q(int priority)
{
	return thread_dequeue(&run_q[(priority + 1) >> 1]);
}
#endif /* not NEW_SCHEDULER */

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


static unsigned int
thread_struct_hash(void *_t, const void *_key, unsigned int range)
{
	struct thread *t = _t;
	const struct thread_key *key = _key;

	if (t != NULL)
		return (t->id % range);

	return (key->id % range);
}


static struct thread *
create_thread_struct(const char *name)
{
	struct thread *t;
	int state;
	char temp[64];

	state = disable_interrupts();
	GRAB_THREAD_LOCK();
	t = thread_dequeue(&dead_q);
	RELEASE_THREAD_LOCK();
	restore_interrupts(state);

	if (t == NULL) {
		t = (struct thread *)kmalloc(sizeof(struct thread));
		if (t == NULL)
			goto err;
	}

	strncpy(&t->name[0], name, SYS_MAX_OS_NAME_LEN-1);
	t->name[SYS_MAX_OS_NAME_LEN-1] = 0;

	t->id = atomic_add(&next_thread_id, 1);
	t->team = NULL;
	t->cpu = NULL;
	t->sem_blocking = -1;
	t->fault_handler = 0;
	t->kernel_stack_region_id = -1;
	t->kernel_stack_base = 0;
	t->user_stack_region_id = -1;
	t->user_stack_base = 0;
	t->team_next = NULL;
	t->q_next = NULL;
	t->priority = -1;
	t->args = NULL;
	t->sig_pending = 0;
	t->sig_block_mask = 0;
	memset(t->sig_action, 0, 32 * sizeof(struct sigaction));
	t->in_kernel = true;
	t->user_time = 0;
	t->kernel_time = 0;
	t->last_time = 0;
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
	kfree(t);
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
	kfree(t);
}


static int
_create_user_thread_kentry(void)
{
	struct thread *t;

	t = thread_get_current_thread();

	// a signal may have been delivered here
//	thread_atkernel_exit();
	t->last_time = system_time();
	t->in_kernel = false;

	// jump to the entry point in user space
	arch_thread_enter_uspace((addr)t->entry, t->args, t->user_stack_base + STACK_SIZE);

	// never get here
	return 0;
}


static int
_create_kernel_thread_kentry(void)
{
	int (*func)(void *args);
	struct thread *t;

	t = thread_get_current_thread();
	
	t->last_time = system_time();
	
	// call the entry function with the appropriate args
	func = (void *)t->entry;

	return func(t->args);
}


static thread_id
_create_thread(const char *name, team_id pid, addr entry, void *args, int priority, bool kernel)
{
	struct thread *t;
	struct team *p;
	int state;
	char stack_name[64];
	bool abort = false;

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
	p = team_get_team_struct_locked(pid);
	if (p != NULL && p->state != TEAM_STATE_DEATH)
		insert_thread_into_team(p, t);
	else
		abort = true;

	RELEASE_TEAM_LOCK();
	if (abort) {
		GRAB_THREAD_LOCK();
		hash_remove(thread_hash, t);
		RELEASE_THREAD_LOCK();
	}
	restore_interrupts(state);
	if (abort) {
		delete_thread_struct(t);
		return ERR_TASK_PROC_DELETED;
	}

	sprintf(stack_name, "%s_kstack", name);
	t->kernel_stack_region_id = vm_create_anonymous_region(vm_get_kernel_aspace_id(), stack_name,
		(void **)&t->kernel_stack_base, REGION_ADDR_ANY_ADDRESS, KSTACK_SIZE,
		REGION_WIRING_WIRED, LOCK_RW|LOCK_KERNEL);
	if (t->kernel_stack_region_id < 0)
		panic("_create_thread: error creating kernel stack!\n");

	t->args = args;
	t->entry = entry;

	if (kernel) {
		// this sets up an initial kthread stack that runs the entry
		arch_thread_initialize_kthread_stack(t, &_create_kernel_thread_kentry, &thread_kthread_entry, &thread_kthread_exit);
	} else {
		// create user stack
		// XXX make this better. For now just keep trying to create a stack
		// until we find a spot.
		t->user_stack_base = (USER_STACK_REGION - STACK_SIZE - ENV_SIZE) + USER_STACK_REGION_SIZE;
		while (t->user_stack_base > USER_STACK_REGION) {
			sprintf(stack_name, "%s_stack%ld", p->name, t->id);
			t->user_stack_region_id = vm_create_anonymous_region(p->_aspace_id, stack_name,
				(void **)&t->user_stack_base,
				REGION_ADDR_ANY_ADDRESS, STACK_SIZE, REGION_WIRING_LAZY, LOCK_RW);
			if (t->user_stack_region_id < 0) {
				t->user_stack_base -= STACK_SIZE;
			} else {
				// we created a region
				break;
			}
		}
		if (t->user_stack_region_id < 0)
			panic("_create_thread: unable to create user stack!\n");

		// copy the user entry over to the args field in the thread struct
		// the function this will call will immediately switch the thread into
		// user space.
		arch_thread_initialize_kthread_stack(t, &_create_user_thread_kentry, &thread_kthread_entry, &thread_kthread_exit);
	}

	t->state = B_THREAD_SUSPENDED;

	return t->id;
}


thread_id
thread_create_user_thread(char *name, team_id pid, addr entry, void *args)
{
	return _create_thread(name, pid, entry, args, -1, false);
}


thread_id
thread_create_kernel_thread(const char *name, int (*func)(void *), void *args)
{
	return _create_thread(name, team_get_kernel_team()->id, (addr)func, args, -1, true);
}


thread_id
thread_create_kernel_thread_etc(const char *name, int (*func)(void *), void *args, struct team *p)
{
	return _create_thread(name, p->id, (addr)func, args, -1, true);
}


int
thread_suspend_thread(thread_id id)
{
	int rv;
	
	rv = send_signal_etc(id, SIGSTOP, B_DO_NOT_RESCHEDULE);
	if (rv == B_OK)
		smp_send_broadcast_ici(SMP_MSG_RESCHEDULE, 0, 0, 0, NULL, SMP_MSG_FLAG_SYNC);
	return rv;
}


int
thread_resume_thread(thread_id id)
{
	return send_signal_etc(id, SIGCONT, B_DO_NOT_RESCHEDULE);
}


#ifndef NEW_SCHEDULER
status_t
thread_set_priority(thread_id id, int32 priority)
{
	struct thread *t;
	int retval;

	// make sure the passed in priority is within bounds
	if (priority > B_MAX_PRIORITY)
		priority = B_MAX_PRIORITY;
	if (priority < B_MIN_PRIORITY)
		priority = B_MIN_PRIORITY;

	t = thread_get_current_thread();
	if (t->id == id) {
		// it's ourself, so we know we aren't in a run queue, and we can manipulate
		// our structure directly
		t->priority = priority;
		retval = B_NO_ERROR;
	} else {
		int state = disable_interrupts();
		GRAB_THREAD_LOCK();

		t = thread_get_thread_struct_locked(id);
		if (t) {
			if (t->state == B_THREAD_READY && t->priority != priority) {
				// this thread is in a ready queue right now, so it needs to be reinserted
				thread_dequeue_id(&run_q[(t->priority + 1) >> 1], t->id);
				t->priority = priority;
				thread_enqueue_run_q(t);
			} else
				t->priority = priority;

			retval = B_NO_ERROR;
		} else
			retval = ERR_INVALID_HANDLE;

		RELEASE_THREAD_LOCK();
		restore_interrupts(state);
	}

	return retval;
}
#endif /* not NEW_SCHEDULER */


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
		t->all_next, t->team_next, t->q_next);
	dprintf("priority:    0x%x\n", t->priority);
	dprintf("state:       %s\n", state_to_text(t->state));
	dprintf("next_state:  %s\n", state_to_text(t->next_state));
	dprintf("cpu:         %p ", t->cpu);
	if (t->cpu)
		dprintf("(%d)\n", t->cpu->info.cpu_num);
	else
		dprintf("\n");
	dprintf("sig_pending:  0x%lx\n", t->sig_pending);
	dprintf("in_kernel:   %d\n", t->in_kernel);
	dprintf("sem_blocking:0x%lx\n", t->sem_blocking);
	dprintf("sem_count:   0x%x\n", t->sem_count);
	dprintf("sem_deleted_retcode: 0x%x\n", t->sem_deleted_retcode);
	dprintf("sem_errcode: 0x%x\n", t->sem_errcode);
	dprintf("sem_flags:   0x%x\n", t->sem_flags);
	dprintf("fault_handler: 0x%lx\n", t->fault_handler);
	dprintf("args:        %p\n", t->args);
	dprintf("entry:       0x%lx\n", t->entry);
	dprintf("team:        %p\n", t->team);
	dprintf("return_code_sem: 0x%lx\n", t->return_code_sem);
	dprintf("kernel_stack_region_id: 0x%lx\n", t->kernel_stack_region_id);
	dprintf("kernel_stack_base: 0x%lx\n", t->kernel_stack_base);
	dprintf("user_stack_region_id:   0x%lx\n", t->user_stack_region_id);
	dprintf("user_stack_base:   0x%lx\n", t->user_stack_base);
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
	unsigned long num;
	struct hash_iterator i;

	if (argc < 2) {
		dprintf("thread: not enough arguments\n");
		return 0;
	}

	// if the argument looks like a hex number, treat it as such
	if (strlen(argv[1]) > 2 && argv[1][0] == '0' && argv[1][1] == 'x') {
		num = atoul(argv[1]);
		if(num > vm_get_kernel_aspace()->virtual_map.base) {
			// XXX semi-hack
			_dump_thread_info((struct thread *)num);
			return 0;
		} else
			id = num;
	}

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
	if (t->q_next != NULL)
		_dump_thread_info(t->q_next);
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
	int i;
	unsigned int bit;
	int state;

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

//	dprintf("get_death_stack: returning 0x%lx\n", death_stacks[i].address);

	return i;
}


static void
put_death_stack_and_reschedule(unsigned int index)
{
//	dprintf("put_death_stack...: passed %d\n", index);

	if (index >= num_death_stacks)
		panic("put_death_stack: passed invalid stack index %d\n", index);

	if (!(death_stack_bitmap & (1 << index)))
		panic("put_death_stack: passed invalid stack index %d\n", index);

	disable_interrupts();
	GRAB_THREAD_LOCK();

	death_stack_bitmap &= ~(1 << index);

	release_sem_etc(death_stack_sem, 1, B_DO_NOT_RESCHEDULE);

	resched();
}


int
thread_init(kernel_args *ka)
{
	struct thread *t;
	unsigned int i;

//	dprintf("thread_init: entry\n");

	// create the thread hash table
	thread_hash = hash_init(15, (addr)&t->all_next - (addr)t,
		&thread_struct_compare, &thread_struct_hash);

#ifndef NEW_SCHEDULER
	// zero out the run queues
	memset(run_q, 0, sizeof(run_q));

#endif /* not NEW_SCHEDULER */
	// zero out the dead thread structure q
	memset(&dead_q, 0, sizeof(dead_q));

	// allocate a snooze sem
	snooze_sem = create_sem(0, "snooze sem");
	if (snooze_sem < 0) {
		panic("error creating snooze sem\n");
		return snooze_sem;
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
		t->kernel_stack_region_id = vm_find_region_by_name(vm_get_kernel_aspace_id(), temp);
		region = vm_get_region_by_id(t->kernel_stack_region_id);
		if (!region) {
			panic("error finding idle kstack region\n");
		}
		t->kernel_stack_base = region->base;
		vm_put_region(region);
		hash_insert(thread_hash, t);
		insert_thread_into_team(t->team, t);
		idle_threads[i] = t;
		if(i == 0)
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
	death_stacks = (struct death_stack *)kmalloc(num_death_stacks * sizeof(struct death_stack));
	if (death_stacks == NULL) {
		panic("error creating death stacks\n");
		return ENOMEM;
	}
	{
		char temp[64];

		for (i = 0; i < num_death_stacks; i++) {
			sprintf(temp, "death_stack%d", i);
			death_stacks[i].rid = vm_create_anonymous_region(vm_get_kernel_aspace_id(), temp,
				(void **)&death_stacks[i].address,
				REGION_ADDR_ANY_ADDRESS, KSTACK_SIZE, REGION_WIRING_WIRED, LOCK_RW|LOCK_KERNEL);
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


// This snooze is for internal kernel use only; doesn't interrupt on signals.
status_t
snooze(bigtime_t timeout)
{
	return acquire_sem_etc(snooze_sem, 1, B_RELATIVE_TIMEOUT, timeout);
}


status_t
snooze_etc(bigtime_t timeout, int timebase, uint32 flags)
{
	if (timebase != B_SYSTEM_TIMEBASE)
		return B_BAD_VALUE;
	return acquire_sem_etc(snooze_sem, 1, B_ABSOLUTE_TIMEOUT | flags, timeout);
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
//	char *temp;

	// copy the arguments over, since the source is probably on the kernel stack we're about to delete
	memcpy(&args, _args, sizeof(struct thread_exit_args));

	// restore the interrupts
	restore_interrupts(args.int_state);

//	dprintf("thread_exit2, running on death stack 0x%lx\n", args.t->kernel_stack_base);

	// delete the old kernel stack region
//	dprintf("thread_exit2: deleting old kernel stack id 0x%x for thread 0x%x\n", args.old_kernel_stack, args.t->id);
	vm_delete_region(vm_get_kernel_aspace_id(), args.old_kernel_stack);

//	dprintf("thread_exit2: removing thread 0x%x from global lists\n", args.t->id);

	// remove this thread from all of the global lists
	disable_interrupts();
	GRAB_TEAM_LOCK();
	remove_thread_from_team(team_get_kernel_team(), args.t);
	RELEASE_TEAM_LOCK();
	GRAB_THREAD_LOCK();
	hash_remove(thread_hash, args.t);
	RELEASE_THREAD_LOCK();

//	dprintf("thread_exit2: done removing thread from lists\n");

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
	int state;
	struct thread *t = thread_get_current_thread();
	struct team *p = t->team;
	bool delete_team = false;
	unsigned int death_stack;
	sem_id cached_death_sem;

	dprintf("thread 0x%lx exiting %s w/return code 0x%x\n", t->id,
		t->return_flags & THREAD_RETURN_INTERRUPTED ? "due to signal" : "normally",
		(int)t->return_code);

	// boost our priority to get this over with
	thread_set_priority(t->id, B_FIRST_REAL_TIME_PRIORITY);

	// Cancel previously installed alarm timer, if any
	cancel_timer(&t->alarm);
	
	// delete the user stack region first
	if (p->_aspace_id >= 0 && t->user_stack_region_id >= 0) {
		region_id rid = t->user_stack_region_id;
		t->user_stack_region_id = -1;
		vm_delete_region(p->_aspace_id, rid);
	}

	if (p != team_get_kernel_team()) {
		// remove this thread from the current team and add it to the kernel
		// put the thread into the kernel team until it dies
		state = disable_interrupts();
		GRAB_TEAM_LOCK();
		remove_thread_from_team(p, t);
		insert_thread_into_team(team_get_kernel_team(), t);
		if (p->main_thread == t) {
			// this was main thread in this team
			delete_team = true;
			team_remove_team_from_hash(p);
			p->state = TEAM_STATE_DEATH;
		}
		RELEASE_TEAM_LOCK();
		// swap address spaces, to make sure we're running on the kernel's pgdir
		vm_aspace_swap(team_get_kernel_team()->kaspace);
		restore_interrupts(state);

//		dprintf("thread_exit: thread 0x%x now a kernel thread!\n", t->id);
	}

	cached_death_sem = p->death_sem;

	// delete the team
	if (delete_team) {
		if (p->num_threads > 0) {
			// there are other threads still in this team,
			// cycle through and signal kill on each of the threads
			// XXX this can be optimized. There's got to be a better solution.
			struct thread *temp_thread;
			char death_sem_name[SYS_MAX_OS_NAME_LEN];

			sprintf(death_sem_name, "team %ld death sem", p->id);
			p->death_sem = create_sem(0, death_sem_name);
			if (p->death_sem < 0)
				panic("thread_exit: cannot init death sem for team %ld\n", p->id);
			
			state = disable_interrupts();
			GRAB_TEAM_LOCK();
			// we can safely walk the list because of the lock. no new threads can be created
			// because of the TEAM_STATE_DEATH flag on the team
			temp_thread = p->thread_list;
			while(temp_thread) {
				struct thread *next = temp_thread->team_next;
				thread_kill_thread_nowait(temp_thread->id);
				temp_thread = next;
			}
			RELEASE_TEAM_LOCK();
			restore_interrupts(state);
			// wait until all threads in team are dead.
			acquire_sem_etc(p->death_sem, p->num_threads, 0, 0);
			delete_sem(p->death_sem);
		}
		cached_death_sem = -1;
		vm_put_aspace(p->aspace);
		vm_delete_aspace(p->_aspace_id);
		delete_owned_ports(p->id);
		sem_delete_owned_sems(p->id);
		vfs_free_io_context(p->ioctx);
		kfree(p);
	}

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


void
sys_exit_thread(status_t return_value)
{
	struct thread *t = thread_get_current_thread();
	
	t->return_code = return_value;
	t->return_flags = THREAD_RETURN_EXIT;
	send_signal_etc(t->id, SIGKILLTHR, B_DO_NOT_RESCHEDULE);
}


int
thread_kill_thread(thread_id id)
{
	int rv;
	
	rv = send_signal_etc(id, SIGKILLTHR, B_DO_NOT_RESCHEDULE);
	if (rv < 0)
		return rv;
	if (id != thread_get_current_thread()->id)
		thread_wait_on_thread(id, NULL);
	
	return rv;
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


int
thread_wait_on_thread(thread_id id, int *retcode)
{
	sem_id sem;
	int state;
	struct thread *t;
	int rc;

	rc = send_signal_etc(id, SIGCONT, 0);
	if (rc != B_OK)
		return rc;
	
	state = disable_interrupts();
	GRAB_THREAD_LOCK();

	t = thread_get_thread_struct_locked(id);
	if (t != NULL) {
		sem = t->return_code_sem;
	} else {
		sem = B_BAD_THREAD_ID;
	}

	RELEASE_THREAD_LOCK();
	restore_interrupts(state);

	rc = acquire_sem(sem);

	/* This thread died the way it should, dont ripple a non-error up */
	if (rc == B_BAD_SEM_ID) {
		rc = B_NO_ERROR;
		
		if (retcode) {
			state = disable_interrupts();
			GRAB_THREAD_LOCK();
			t = thread_get_current_thread();
			dprintf("thread_wait_on_thread: thread %ld got return code 0x%x\n",
				t->id, t->sem_deleted_retcode);
			*retcode = t->sem_deleted_retcode;
			RELEASE_THREAD_LOCK();
			restore_interrupts(state);
		}
	}

	return rc;
}


struct thread *
thread_get_thread_struct(thread_id id)
{
	struct thread *t;
	int state;

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
	int state;
	struct thread *t;
	bigtime_t now;

//	dprintf("thread_atkernel_entry: entry thread 0x%x\n", t->id);

	t = thread_get_current_thread();

	state = disable_interrupts();

	// track user time
	now = system_time();
	t->user_time += now - t->last_time;
	t->last_time = now;

	t->in_kernel = true;

	restore_interrupts(state);
}


// called when a thread exits kernel space to user space

void
thread_atkernel_exit(void)
{
	int state;
	struct thread *t;
	bigtime_t now;

//	dprintf("thread_atkernel_exit: entry\n");

	t = thread_get_current_thread();

	state = disable_interrupts();
	GRAB_THREAD_LOCK();

	handle_signals(t, state);

	t->in_kernel = false;

	RELEASE_THREAD_LOCK();

	// track kernel time
	now = system_time();
	t->kernel_time += now - t->last_time;
	t->last_time = now;

	restore_interrupts(state);
}


status_t
send_data(thread_id tid, int32 code, const void *buffer, size_t buffer_size)
{
	struct thread *target;
	sem_id cached_sem;
	int state;
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

	rv = acquire_sem_etc(cached_sem, 1, B_CAN_INTERRUPT, 0);
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
receive_data(thread_id *sender, void *buffer, size_t buffer_size)
{
	struct thread *t = thread_get_current_thread();
	status_t rv;
	size_t size;
	int32 code;

	acquire_sem(t->msg.read_sem);

	size = min(buffer_size, t->msg.size);
	rv = cbuf_user_memcpy_from_chain(buffer, t->msg.buffer, 0, size);
	if (rv < 0) {
		cbuf_free_chain(t->msg.buffer);
		release_sem(t->msg.write_sem);
		return rv;
	}

	*sender = t->msg.sender;
	code = t->msg.code;

	cbuf_free_chain(t->msg.buffer);
	release_sem(t->msg.write_sem);

	return code;
}


bool
has_data(thread_id thread)
{
	int32 count;

	if (get_sem_count(thread_get_current_thread()->msg.read_sem, &count) != B_OK)
		return false;
	return count == 0 ? false : true;
}


status_t
_get_thread_info(thread_id id, thread_info *info, size_t size)
{
	int state;
	status_t rc = B_OK;
	struct thread *t;

	state = disable_interrupts();
	GRAB_THREAD_LOCK();

	t = thread_get_thread_struct_locked(id);
	if (!t) {
		rc = B_BAD_VALUE;
		goto err;
	}
	info->thread = t->id;
	info->team = t->team->id;
	strncpy(info->name, t->name, B_OS_NAME_LENGTH);
	info->name[B_OS_NAME_LENGTH - 1] = '\0';
	if (t->state == B_THREAD_WAITING) {
		if (t->sem_blocking == snooze_sem)
			info->state = B_THREAD_ASLEEP;
		else if (t->sem_blocking == t->msg.read_sem)
			info->state = B_THREAD_RECEIVING;
		else
			info->state = B_THREAD_WAITING;
	} else
		info->state = t->state;
	info->priority = t->priority;
	info->sem = t->sem_blocking;
	info->user_time = t->user_time;
	info->kernel_time = t->kernel_time;
	info->stack_base = (void *)t->user_stack_base;
	info->stack_end = (void *)(t->user_stack_base + STACK_SIZE);

err:
	RELEASE_THREAD_LOCK();
	restore_interrupts(state);

	return rc;
}


status_t
_get_next_thread_info(team_id tid, int32 *cookie, thread_info *info, size_t size)
{
	int state;
	int slot;
	status_t rc = B_BAD_VALUE;
	struct team *team;
	struct thread *t = NULL;

	if (tid == 0)
		tid = team_get_current_team_id();
	team = team_get_team_struct(tid);
	if (!team)
		return B_BAD_VALUE;

	state = disable_interrupts();
	GRAB_THREAD_LOCK();

	if (*cookie == 0)
		slot = 0;
	else {
		slot = *cookie;
		if (slot >= next_thread_id)
			goto err;
	}

	while ((slot < next_thread_id) && (!(t = thread_get_thread_struct_locked(slot)) || (t->team->id != tid)))
		slot++;

	if ((t) && (t->team->id == tid)) {
		info->thread = t->id;
		info->team = t->team->id;
		strncpy(info->name, t->name, B_OS_NAME_LENGTH);
		info->name[B_OS_NAME_LENGTH - 1] = '\0';
		if (t->state == B_THREAD_WAITING) {
			if (t->sem_blocking == snooze_sem)
				info->state = B_THREAD_ASLEEP;
			else if (t->sem_blocking == t->msg.read_sem)
				info->state = B_THREAD_RECEIVING;
			else
				info->state = B_THREAD_WAITING;
		} else
			info->state = t->state;
		info->priority = t->priority;
		info->sem = t->sem_blocking;
		info->user_time = t->user_time;
		info->kernel_time = t->kernel_time;
		info->stack_base = (void *)t->user_stack_base;
		info->stack_end = (void *)(t->user_stack_base + STACK_SIZE);
		slot++;
		*cookie = slot;
		rc = B_OK;
	}
err:
	RELEASE_THREAD_LOCK();
	restore_interrupts(state);

	return rc;
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
/* user calls */


thread_id
user_thread_create_user_thread(addr entry, team_id pid, const char *uname, int priority,
	void *args)
{
	char name[SYS_MAX_OS_NAME_LEN];
	int rc;

	if ((addr)uname >= KERNEL_BASE && (addr)uname <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;
	if (entry >= KERNEL_BASE && entry <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	rc = user_strncpy(name, uname, SYS_MAX_OS_NAME_LEN-1);
	if (rc < 0)
		return rc;
	name[SYS_MAX_OS_NAME_LEN-1] = 0;

	return _create_thread(name, pid, entry, args, priority, false);
}


status_t
user_get_thread_info(thread_id id, thread_info *info)
{
	thread_info kinfo;
	status_t rc = B_OK;
	status_t rc2;
	
	if ((addr)info >= KERNEL_BASE && (addr)info <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;
		
	rc = _get_thread_info(id, &kinfo, sizeof(thread_info));
	if (rc != B_OK)
		return rc;
	
	rc2 = user_memcpy(info, &kinfo, sizeof(thread_info));
	if (rc2 < 0)
		return rc2;
	
	return rc;
}


status_t
user_get_next_thread_info(team_id team, int32 *cookie, thread_info *info)
{
	int32 kcookie;
	thread_info kinfo;
	status_t rc = B_OK;
	status_t rc2;
	
	if ((addr)cookie >= KERNEL_BASE && (addr)cookie <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;
	if ((addr)info >= KERNEL_BASE && (addr)info <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;
	
	rc2 = user_memcpy(&kcookie, cookie, sizeof(int32));
	if (rc2 < 0)
		return rc2;
	
	rc = _get_next_thread_info(team, &kcookie, &kinfo, sizeof(thread_info));
	if (rc != B_OK)
		return rc;
	
	rc2 = user_memcpy(cookie, &kcookie, sizeof(int32));
	if (rc2 < 0)
		return rc2;
	
	rc2 = user_memcpy(info, &kinfo, sizeof(thread_info));
	if (rc2 < 0)
		return rc2;
	
	return rc;
}


int
user_thread_wait_on_thread(thread_id id, int *uretcode)
{
	int retcode;
	int rc, rc2;

	if ((addr)uretcode >= KERNEL_BASE && (addr)uretcode <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	rc = thread_wait_on_thread(id, &retcode);

	rc2 = user_memcpy(uretcode, &retcode, sizeof(int));
	if (rc2 < 0)
		return rc2;

	return rc;
}


status_t
user_send_data(thread_id tid, int32 code, const void *buffer, size_t buffer_size)
{
	if (((addr)buffer >= KERNEL_BASE) && ((addr)buffer <= KERNEL_TOP))
		return B_BAD_ADDRESS;
	return send_data(tid, code, buffer, buffer_size);
}


status_t
user_receive_data(thread_id *sender, void *buffer, size_t buffer_size)
{
	thread_id ksender;
	status_t code;
	status_t rv;

	if (((addr)sender >= KERNEL_BASE) && ((addr)sender <= KERNEL_TOP))
		return B_BAD_ADDRESS;
	if (((addr)buffer >= KERNEL_BASE) && ((addr)buffer <= KERNEL_TOP))
		return B_BAD_ADDRESS;
	
	code = receive_data(&ksender, buffer, buffer_size);
	
	rv = user_memcpy(sender, &ksender, sizeof(thread_id));
	if (rv < 0)
		return rv;
	
	return code;
}


int
user_getrlimit(int resource, struct rlimit * urlp)
{
	struct rlimit rl;
	int ret;

	if (urlp == NULL)
		return EINVAL;

	if ((addr)urlp >= KERNEL_BASE && (addr)urlp <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

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
user_setrlimit(int resource, const struct rlimit * urlp)
{
	struct rlimit rl;
	int err;

	if (urlp == NULL)
		return EINVAL;

	if ((addr)urlp >= KERNEL_BASE && (addr)urlp <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	err = user_memcpy(&rl, urlp, sizeof(struct rlimit));
	if (err < 0)
		return err;

	return setrlimit(resource, &rl);
}


