/* Threading and process information */

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

struct proc_key {
	proc_id id;
};

struct thread_key {
	thread_id id;
};

struct proc_arg {
	char *path;
	char **args;
	unsigned int argc;
};

static struct proc *create_proc_struct(const char *name, bool kernel);
static int proc_struct_compare(void *_p, const void *_key);
static unsigned int proc_struct_hash(void *_p, const void *_key, unsigned int range);

// global
spinlock_t thread_spinlock = 0;

// proc list
static void *proc_hash = NULL;
static struct proc *kernel_proc = NULL;
static proc_id next_proc_id = 0;
static spinlock_t proc_spinlock = 0;
	// NOTE: PROC lock can be held over a THREAD lock acquisition,
	// but not the other way (to avoid deadlock)
#define GRAB_PROC_LOCK() acquire_spinlock(&proc_spinlock)
#define RELEASE_PROC_LOCK() release_spinlock(&proc_spinlock)

// thread list
static struct thread *idle_threads[MAX_BOOT_CPUS];
static void *thread_hash = NULL;
static thread_id next_thread_id = 0;

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

// thread queues
static struct thread_queue run_q[THREAD_NUM_PRIORITY_LEVELS] = { { NULL, NULL }, };
static struct thread_queue dead_q;

static int _rand(void);
static void thread_entry(void);
static struct thread *thread_get_thread_struct_locked(thread_id id);
/* XXX - not currently used, so commented out
static struct proc *proc_get_proc_struct(proc_id id);
 */
static struct proc *proc_get_proc_struct_locked(proc_id id);
static void thread_kthread_exit(void);
static void deliver_signal(struct thread *t, int signal);

// insert a thread onto the tail of a queue
void thread_enqueue(struct thread *t, struct thread_queue *q)
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

struct thread *thread_lookat_queue(struct thread_queue *q)
{
	return q->head;
}

struct thread *thread_dequeue(struct thread_queue *q)
{
	struct thread *t;

	t = q->head;
	if(t != NULL) {
		q->head = t->q_next;
		if(q->tail == t)
			q->tail = NULL;
	}
	return t;
}

struct thread *thread_dequeue_id(struct thread_queue *q, thread_id thr_id)
{
	struct thread *t;
	struct thread *last = NULL;

	t = q->head;
	while(t != NULL) {
		if(t->id == thr_id) {
			if(last == NULL) {
				q->head = t->q_next;
			} else {
				last->q_next = t->q_next;
			}
			if(q->tail == t)
				q->tail = last;
			break;
		}
		last = t;
		t = t->q_next;
	}
	return t;
}

struct thread *thread_lookat_run_q(int priority)
{
	return thread_lookat_queue(&run_q[priority]);
}

void thread_enqueue_run_q(struct thread *t)
{
	// these shouldn't exist
	if(t->priority > THREAD_MAX_PRIORITY)
		t->priority = THREAD_MAX_PRIORITY;
	if(t->priority < 0)
		t->priority = 0;

	thread_enqueue(t, &run_q[t->priority]);
}

struct thread *thread_dequeue_run_q(int priority)
{
	return thread_dequeue(&run_q[priority]);
}

static void insert_thread_into_proc(struct proc *p, struct thread *t)
{
	t->proc_next = p->thread_list;
	p->thread_list = t;
	p->num_threads++;
	if(p->num_threads == 1) {
		// this was the first thread
		p->main_thread = t;
	}
	t->proc = p;
}

static void remove_thread_from_proc(struct proc *p, struct thread *t)
{
	struct thread *temp, *last = NULL;

	for(temp = p->thread_list; temp != NULL; temp = temp->proc_next) {
		if(temp == t) {
			if(last == NULL) {
				p->thread_list = temp->proc_next;
			} else {
				last->proc_next = temp->proc_next;
			}
			p->num_threads--;
			break;
		}
		last = temp;
	}
}

static int thread_struct_compare(void *_t, const void *_key)
{
	struct thread *t = _t;
	const struct thread_key *key = _key;

	if(t->id == key->id) return 0;
	else return 1;
}

// Frees the argument list
// Parameters
// 	args  argument list.
//  args  number of arguments

static void free_arg_list(char **args, int argc)
{
	int  cnt = argc;

	if(args != NULL) {
		for(cnt = 0; cnt < argc; cnt++){
			kfree(args[cnt]);
		}

	    kfree(args);
	}
}

// Copy argument list from  userspace to kernel space
// Parameters
//			args   userspace parameters
//       argc   number of parameters
//       kargs  usespace parameters
//			return < 0 on error and **kargs = NULL

static int user_copy_arg_list(char **args, int argc, char ***kargs)
{
	char **largs;
	int err;
	int cnt;
	char *source;
	char buf[SYS_THREAD_ARG_LENGTH_MAX];

	*kargs = NULL;

	if((addr)args >= KERNEL_BASE && (addr)args <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	largs = (char **)kmalloc((argc + 1) * sizeof(char *));
	if(largs == NULL){
		return ERR_NO_MEMORY;
	}

	// scan all parameters and copy to kernel space

	for(cnt = 0; cnt < argc; cnt++) {
		err = user_memcpy(&source, &(args[cnt]), sizeof(char *));
		if(err < 0)
			goto error;

		if((addr)source >= KERNEL_BASE && (addr)source <= KERNEL_TOP){
			err = ERR_VM_BAD_USER_MEMORY;
			goto error;
		}

		err = user_strncpy(buf,source, SYS_THREAD_ARG_LENGTH_MAX - 1);
		if(err < 0)
			goto error;
		buf[SYS_THREAD_ARG_LENGTH_MAX - 1] = 0;

		largs[cnt] = (char *)kstrdup(buf);
		if(largs[cnt] == NULL){
			err = ERR_NO_MEMORY;
			goto error;
		}
	}

	largs[argc] = NULL;

	*kargs = largs;
	return B_NO_ERROR;

error:
	free_arg_list(largs,cnt);
	dprintf("user_copy_arg_list failed %d \n",err);
	return err;
}

static unsigned int thread_struct_hash(void *_t, const void *_key, unsigned int range)
{
	struct thread *t = _t;
	const struct thread_key *key = _key;

	if(t != NULL)
		return (t->id % range);
	else
		return (key->id % range);
}

static struct thread *create_thread_struct(const char *name)
{
	struct thread *t;
	int state;

	state = int_disable_interrupts();
	GRAB_THREAD_LOCK();
	t = thread_dequeue(&dead_q);
	RELEASE_THREAD_LOCK();
	int_restore_interrupts(state);

	if(t == NULL) {
		t = (struct thread *)kmalloc(sizeof(struct thread));
		if(t == NULL)
			goto err;
	}

	strncpy(&t->name[0], name, SYS_MAX_OS_NAME_LEN-1);
	t->name[SYS_MAX_OS_NAME_LEN-1] = 0;

	t->id = atomic_add(&next_thread_id, 1);
	t->proc = NULL;
	t->cpu = NULL;
	t->sem_blocking = -1;
	t->fault_handler = 0;
	t->kernel_stack_region_id = -1;
	t->kernel_stack_base = 0;
	t->user_stack_region_id = -1;
	t->user_stack_base = 0;
	t->proc_next = NULL;
	t->q_next = NULL;
	t->priority = -1;
	t->args = NULL;
	t->pending_signals = SIG_NONE;
	t->in_kernel = true;
	t->user_time = 0;
	t->kernel_time = 0;
	t->last_time = 0;
	{
		char temp[64];

		sprintf(temp, "thread_0x%x_retcode_sem", t->id);
		t->return_code_sem = create_sem(0, temp);
		if(t->return_code_sem < 0)
			goto err1;
	}

	if(arch_thread_init_thread_struct(t) < 0)
		goto err2;

	return t;

err2:
	delete_sem_etc(t->return_code_sem, -1);
err1:
	kfree(t);
err:
	return NULL;
}

static void delete_thread_struct(struct thread *t)
{
	if(t->return_code_sem >= 0)
		delete_sem_etc(t->return_code_sem, -1);
	kfree(t);
}

static int _create_user_thread_kentry(void)
{
	struct thread *t;

	t = thread_get_current_thread();

	// a signal may have been delivered here
	thread_atkernel_exit();

	// jump to the entry point in user space
	arch_thread_enter_uspace((addr)t->entry, t->args, t->user_stack_base + STACK_SIZE);

	// never get here
	return 0;
}

static int _create_kernel_thread_kentry(void)
{
	int (*func)(void *args);
	struct thread *t;

	t = thread_get_current_thread();

	// call the entry function with the appropriate args
	func = (void *)t->entry;

	return func(t->args);
}

static thread_id _create_thread(const char *name, proc_id pid, addr entry, void *args, 
                                int priority, bool kernel)
{
	struct thread *t;
	struct proc *p;
	int state;
	char stack_name[64];
	bool abort = false;

	t = create_thread_struct(name);
	if(t == NULL)
		return ERR_NO_MEMORY;

	t->priority = priority == -1 ? THREAD_MEDIUM_PRIORITY : priority;
	t->state = THREAD_STATE_BIRTH;
	t->next_state = THREAD_STATE_SUSPENDED;

	state = int_disable_interrupts();
	GRAB_THREAD_LOCK();

	// insert into global list
	hash_insert(thread_hash, t);
	RELEASE_THREAD_LOCK();

	GRAB_PROC_LOCK();
	// look at the proc, make sure it's not being deleted
	p = proc_get_proc_struct_locked(pid);
	if(p != NULL && p->state != PROC_STATE_DEATH) {
		insert_thread_into_proc(p, t);
	} else {
		abort = true;
	}
	RELEASE_PROC_LOCK();
	if(abort) {
		GRAB_THREAD_LOCK();
		hash_remove(thread_hash, t);
		RELEASE_THREAD_LOCK();
	}
	int_restore_interrupts(state);
	if(abort) {
		delete_thread_struct(t);
		return ERR_TASK_PROC_DELETED;
	}

	sprintf(stack_name, "%s_kstack", name);
	t->kernel_stack_region_id = vm_create_anonymous_region(vm_get_kernel_aspace_id(), stack_name,
		(void **)&t->kernel_stack_base, REGION_ADDR_ANY_ADDRESS, KSTACK_SIZE,
		REGION_WIRING_WIRED, LOCK_RW|LOCK_KERNEL);
	if(t->kernel_stack_region_id < 0)
		panic("_create_thread: error creating kernel stack!\n");

	t->args = args;
	t->entry = entry;

	if(kernel) {
		// this sets up an initial kthread stack that runs the entry
		arch_thread_initialize_kthread_stack(t, &_create_kernel_thread_kentry, &thread_entry, &thread_kthread_exit);
	} else {
		// create user stack
		// XXX make this better. For now just keep trying to create a stack
		// until we find a spot.
		t->user_stack_base = (USER_STACK_REGION  - STACK_SIZE) + USER_STACK_REGION_SIZE;
		while(t->user_stack_base > USER_STACK_REGION) {
			sprintf(stack_name, "%s_stack%d", p->name, t->id);
			t->user_stack_region_id = vm_create_anonymous_region(p->_aspace_id, stack_name,
				(void **)&t->user_stack_base,
				REGION_ADDR_ANY_ADDRESS, STACK_SIZE, REGION_WIRING_LAZY, LOCK_RW);
			if(t->user_stack_region_id < 0) {
				t->user_stack_base -= STACK_SIZE;
			} else {
				// we created a region
				break;
			}
		}
		if(t->user_stack_region_id < 0)
			panic("_create_thread: unable to create user stack!\n");

		// copy the user entry over to the args field in the thread struct
		// the function this will call will immediately switch the thread into
		// user space.
		arch_thread_initialize_kthread_stack(t, &_create_user_thread_kentry, &thread_entry, &thread_kthread_exit);
	}

	t->state = THREAD_STATE_SUSPENDED;

	return t->id;
}

thread_id user_thread_create_user_thread(addr entry, proc_id pid, const char *uname, int priority,
                                         void *args)
{
	char name[SYS_MAX_OS_NAME_LEN];
	int rc;

	if((addr)uname >= KERNEL_BASE && (addr)uname <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;
	if(entry >= KERNEL_BASE && entry <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	rc = user_strncpy(name, uname, SYS_MAX_OS_NAME_LEN-1);
	if(rc < 0)
		return rc;
	name[SYS_MAX_OS_NAME_LEN-1] = 0;

	return _create_thread(name, pid, entry, args, priority, false);
}

thread_id thread_create_user_thread(char *name, proc_id pid, addr entry, void *args)
{
	return _create_thread(name, pid, entry, args, -1, false);
}

thread_id thread_create_kernel_thread(const char *name, int (*func)(void *), void *args)
{
	return _create_thread(name, proc_get_kernel_proc()->id, (addr)func, args, -1, true);
}

static thread_id thread_create_kernel_thread_etc(const char *name, int (*func)(void *), void *args, struct proc *p)
{
	return _create_thread(name, p->id, (addr)func, args, -1, true);
}

int thread_suspend_thread(thread_id id)
{
	int state;
	struct thread *t;
	int retval;
	bool global_resched = false;

	state = int_disable_interrupts();
	GRAB_THREAD_LOCK();

	t = thread_get_current_thread();
	if(t->id != id) {
		t = thread_get_thread_struct_locked(id);
	}

	if (t != NULL) {
		if (t->proc == kernel_proc) {
			// no way
			retval = ERR_NOT_ALLOWED;
		} else if (t->in_kernel == true) {
			t->pending_signals |= SIG_SUSPEND;
			retval = B_NO_ERROR;
		} else {
			t->next_state = THREAD_STATE_SUSPENDED;
			global_resched = true;
			retval = B_NO_ERROR;
		}
	} else {
		retval = ERR_INVALID_HANDLE;
	}

	RELEASE_THREAD_LOCK();
	int_restore_interrupts(state);

	if(global_resched) {
		smp_send_broadcast_ici(SMP_MSG_RESCHEDULE, 0, 0, 0, NULL, SMP_MSG_FLAG_SYNC);
	}

	return retval;
}

int thread_resume_thread(thread_id id)
{
	int state;
	struct thread *t;
	int retval;

	state = int_disable_interrupts();
	GRAB_THREAD_LOCK();

	t = thread_get_thread_struct_locked(id);
	if(t != NULL && t->state == THREAD_STATE_SUSPENDED) {
		t->state = THREAD_STATE_READY;
		t->next_state = THREAD_STATE_READY;

		thread_enqueue_run_q(t);
		retval = B_NO_ERROR;
	} else {
		retval = ERR_INVALID_HANDLE;
	}

	RELEASE_THREAD_LOCK();
	int_restore_interrupts(state);

	return retval;
}

int thread_set_priority(thread_id id, int priority)
{
	struct thread *t;
	int retval;

	// make sure the passed in priority is within bounds
	if(priority > THREAD_MAX_PRIORITY)
		priority = THREAD_MAX_PRIORITY;
	if(priority < THREAD_MIN_PRIORITY)
		priority = THREAD_MIN_PRIORITY;

	t = thread_get_current_thread();
	if(t->id == id) {
		// it's ourself, so we know we aren't in a run queue, and we can manipulate
		// our structure directly
		t->priority = priority;
		retval = B_NO_ERROR;
	} else {
		int state = int_disable_interrupts();
		GRAB_THREAD_LOCK();

		t = thread_get_thread_struct_locked(id);
		if(t) {
			if(t->state == THREAD_STATE_READY && t->priority != priority) {
				// this thread is in a ready queue right now, so it needs to be reinserted
				thread_dequeue_id(&run_q[t->priority], t->id);
				t->priority = priority;
				thread_enqueue_run_q(t);
			} else {
				t->priority = priority;
			}
			retval = B_NO_ERROR;
		} else {
			retval = ERR_INVALID_HANDLE;
		}

		RELEASE_THREAD_LOCK();
		int_restore_interrupts(state);
	}

	return retval;
}

static void _dump_proc_info(struct proc *p)
{
	dprintf("PROC: %p\n", p);
	dprintf("id:          0x%x\n", p->id);
	dprintf("name:        '%s'\n", p->name);
	dprintf("next:        %p\n", p->next);
	dprintf("num_threads: %d\n", p->num_threads);
	dprintf("state:       %d\n", p->state);
	dprintf("pending_signals: 0x%x\n", p->pending_signals);
	dprintf("ioctx:       %p\n", p->ioctx);
	dprintf("path:        '%s'\n", p->path);
	dprintf("aspace_id:   0x%x\n", p->_aspace_id);
	dprintf("aspace:      %p\n", p->aspace);
	dprintf("kaspace:     %p\n", p->kaspace);
	dprintf("main_thread: %p\n", p->main_thread);
	dprintf("thread_list: %p\n", p->thread_list);
}

static void dump_proc_info(int argc, char **argv)
{
	struct proc *p;
	int id = -1;
	unsigned long num;
	struct hash_iterator i;

	if(argc < 2) {
		dprintf("proc: not enough arguments\n");
		return;
	}

	// if the argument looks like a hex number, treat it as such
	if(strlen(argv[1]) > 2 && argv[1][0] == '0' && argv[1][1] == 'x') {
		num = atoul(argv[1]);
		if(num > vm_get_kernel_aspace()->virtual_map.base) {
			// XXX semi-hack
			_dump_proc_info((struct proc*)num);
			return;
		} else {
			id = num;
		}
	}

	// walk through the thread list, trying to match name or id
	hash_open(proc_hash, &i);
	while((p = hash_next(proc_hash, &i)) != NULL) {
		if((p->name && strcmp(argv[1], p->name) == 0) || p->id == id) {
			_dump_proc_info(p);
			break;
		}
	}
	hash_close(proc_hash, &i, false);
}


static const char *state_to_text(int state)
{
	switch(state) {
		case THREAD_STATE_READY:
			return "READY";
		case THREAD_STATE_RUNNING:
			return "RUNNING";
		case THREAD_STATE_WAITING:
			return "WAITING";
		case THREAD_STATE_SUSPENDED:
			return "SUSPEND";
		case THREAD_STATE_FREE_ON_RESCHED:
			return "DEATH";
		case THREAD_STATE_BIRTH:
			return "BIRTH";
		default:
			return "UNKNOWN";
	}
}

static struct thread *last_thread_dumped = NULL;

static void _dump_thread_info(struct thread *t)
{
	dprintf("THREAD: %p\n", t);
	dprintf("id:          0x%x\n", t->id);
	dprintf("name:        '%s'\n", t->name);
	dprintf("all_next:    %p\nproc_next:  %p\nq_next:     %p\n",
		t->all_next, t->proc_next, t->q_next);
	dprintf("priority:    0x%x\n", t->priority);
	dprintf("state:       %s\n", state_to_text(t->state));
	dprintf("next_state:  %s\n", state_to_text(t->next_state));
	dprintf("cpu:         %p ", t->cpu);
	if(t->cpu)
		dprintf("(%d)\n", t->cpu->info.cpu_num);
	else
		dprintf("\n");
	dprintf("pending_signals:  0x%x\n", t->pending_signals);
	dprintf("in_kernel:   %d\n", t->in_kernel);
	dprintf("sem_blocking:0x%x\n", t->sem_blocking);
	dprintf("sem_count:   0x%x\n", t->sem_count);
	dprintf("sem_deleted_retcode: 0x%x\n", t->sem_deleted_retcode);
	dprintf("sem_errcode: 0x%x\n", t->sem_errcode);
	dprintf("sem_flags:   0x%x\n", t->sem_flags);
	dprintf("fault_handler: 0x%lx\n", t->fault_handler);
	dprintf("args:        %p\n", t->args);
	dprintf("entry:       0x%lx\n", t->entry);
	dprintf("proc:        %p\n", t->proc);
	dprintf("return_code_sem: 0x%x\n", t->return_code_sem);
	dprintf("kernel_stack_region_id: 0x%x\n", t->kernel_stack_region_id);
	dprintf("kernel_stack_base: 0x%lx\n", t->kernel_stack_base);
	dprintf("user_stack_region_id:   0x%x\n", t->user_stack_region_id);
	dprintf("user_stack_base:   0x%lx\n", t->user_stack_base);
	dprintf("kernel_time:       %Ld\n", t->kernel_time);
	dprintf("user_time:         %Ld\n", t->user_time);
	dprintf("architecture dependant section:\n");
	arch_thread_dump_info(&t->arch_info);

	last_thread_dumped = t;
}

static void dump_thread_info(int argc, char **argv)
{
	struct thread *t;
	int id = -1;
	unsigned long num;
	struct hash_iterator i;

	if(argc < 2) {
		dprintf("thread: not enough arguments\n");
		return;
	}

	// if the argument looks like a hex number, treat it as such
	if(strlen(argv[1]) > 2 && argv[1][0] == '0' && argv[1][1] == 'x') {
		num = atoul(argv[1]);
		if(num > vm_get_kernel_aspace()->virtual_map.base) {
			// XXX semi-hack
			_dump_thread_info((struct thread *)num);
			return;
		} else {
			id = num;
		}
	}

	// walk through the thread list, trying to match name or id
	hash_open(thread_hash, &i);
	while((t = hash_next(thread_hash, &i)) != NULL) {
		if((t->name && strcmp(argv[1], t->name) == 0) || t->id == id) {
			_dump_thread_info(t);
			break;
		}
	}
	hash_close(thread_hash, &i, false);
}

static void dump_thread_list(int argc, char **argv)
{
	struct thread *t;
	struct hash_iterator i;

	hash_open(thread_hash, &i);
	while((t = hash_next(thread_hash, &i)) != NULL) {
		dprintf("%p", t);
		if(t->name != NULL)
			dprintf("\t%32s", t->name);
		else
			dprintf("\t%32s", "<NULL>");
		dprintf("\t0x%x", t->id);
		dprintf("\t%16s", state_to_text(t->state));
		if(t->cpu)
			dprintf("\t%d", t->cpu->info.cpu_num);
		else
			dprintf("\tNOCPU");
		dprintf("\t0x%lx\n", t->kernel_stack_base);
	}
	hash_close(thread_hash, &i, false);
}

static void dump_next_thread_in_q(int argc, char **argv)
{
	struct thread *t = last_thread_dumped;

	if(t == NULL) {
		dprintf("no thread previously dumped. Examine a thread first.\n");
		return;
	}

	dprintf("next thread in queue after thread @ %p\n", t);
	if(t->q_next != NULL) {
		_dump_thread_info(t->q_next);
	} else {
		dprintf("NULL\n");
	}
}

static void dump_next_thread_in_all_list(int argc, char **argv)
{
	struct thread *t = last_thread_dumped;

	if(t == NULL) {
		dprintf("no thread previously dumped. Examine a thread first.\n");
		return;
	}

	dprintf("next thread in global list after thread @ %p\n", t);
	if(t->all_next != NULL) {
		_dump_thread_info(t->all_next);
	} else {
		dprintf("NULL\n");
	}
}

static void dump_next_thread_in_proc(int argc, char **argv)
{
	struct thread *t = last_thread_dumped;

	if(t == NULL) {
		dprintf("no thread previously dumped. Examine a thread first.\n");
		return;
	}

	dprintf("next thread in proc after thread @ %p\n", t);
	if(t->proc_next != NULL) {
		_dump_thread_info(t->proc_next);
	} else {
		dprintf("NULL\n");
	}
}

static int get_death_stack(void)
{
	int i;
	unsigned int bit;
	int state;

	acquire_sem(death_stack_sem);

	// grap the thread lock, find a free spot and release
	state = int_disable_interrupts();
	GRAB_THREAD_LOCK();
	bit = death_stack_bitmap;
	bit = (~bit)&~((~bit)-1);
	death_stack_bitmap |= bit;
	RELEASE_THREAD_LOCK();


	// sanity checks
	if( !bit ) {
		panic("get_death_stack: couldn't find free stack!\n");
	}
	if( bit & (bit-1)) {
		panic("get_death_stack: impossible bitmap result!\n");
	}


	// bit to number
	i= -1;
	while(bit) {
		bit >>= 1;
		i += 1;
	}

//	dprintf("get_death_stack: returning 0x%lx\n", death_stacks[i].address);

	return i;
}

static void put_death_stack_and_reschedule(unsigned int index)
{
//	dprintf("put_death_stack...: passed %d\n", index);

	if(index >= num_death_stacks)
		panic("put_death_stack: passed invalid stack index %d\n", index);

	if(!(death_stack_bitmap & (1 << index)))
		panic("put_death_stack: passed invalid stack index %d\n", index);

	int_disable_interrupts();
	GRAB_THREAD_LOCK();

	death_stack_bitmap &= ~(1 << index);

	release_sem_etc(death_stack_sem, 1, B_DO_NOT_RESCHEDULE);

	thread_resched();
}

int thread_init(kernel_args *ka)
{
	struct thread *t;
	unsigned int i;

//	dprintf("thread_init: entry\n");

	// create the process hash table
	proc_hash = hash_init(15, (addr)&kernel_proc->next - (addr)kernel_proc,
		&proc_struct_compare, &proc_struct_hash);

	// create the kernel process
	kernel_proc = create_proc_struct("kernel_proc", true);
	if(kernel_proc == NULL)
		panic("could not create kernel proc!\n");
	kernel_proc->state = PROC_STATE_NORMAL;

	kernel_proc->ioctx = vfs_new_io_context(NULL);
	if(kernel_proc->ioctx == NULL)
		panic("could not create ioctx for kernel proc!\n");

	//XXX should initialize kernel_proc->path here. Set it to "/"?

	// stick it in the process hash
	hash_insert(proc_hash, kernel_proc);

	// create the thread hash table
	thread_hash = hash_init(15, (addr)&t->all_next - (addr)t,
		&thread_struct_compare, &thread_struct_hash);

	// zero out the run queues
	memset(run_q, 0, sizeof(run_q));

	// zero out the dead thread structure q
	memset(&dead_q, 0, sizeof(dead_q));

	// allocate a snooze sem
	snooze_sem = create_sem(0, "snooze sem");
	if(snooze_sem < 0) {
		panic("error creating snooze sem\n");
		return snooze_sem;
	}

	// create an idle thread for each cpu
	for(i=0; i<ka->num_cpus; i++) {
		char temp[64];
		vm_region *region;

		sprintf(temp, "idle_thread%d", i);
		t = create_thread_struct(temp);
		if(t == NULL) {
			panic("error creating idle thread struct\n");
			return ERR_NO_MEMORY;
		}
		t->proc = proc_get_kernel_proc();
		t->priority = THREAD_IDLE_PRIORITY;
		t->state = THREAD_STATE_RUNNING;
		t->next_state = THREAD_STATE_READY;
		sprintf(temp, "idle_thread%d_kstack", i);
		t->kernel_stack_region_id = vm_find_region_by_name(vm_get_kernel_aspace_id(), temp);
		region = vm_get_region_by_id(t->kernel_stack_region_id);
		if(!region) {
			panic("error finding idle kstack region\n");
		}
		t->kernel_stack_base = region->base;
		vm_put_region(region);
		hash_insert(thread_hash, t);
		insert_thread_into_proc(t->proc, t);
		idle_threads[i] = t;
		if(i == 0)
			arch_thread_set_current_thread(t);
		t->cpu = &cpu[i];
	}

	// create a set of death stacks
	num_death_stacks = smp_get_num_cpus();
	if(num_death_stacks > 8*sizeof(death_stack_bitmap)) {
		/*
		 * clamp values for really beefy machines
		 */
		num_death_stacks = 8*sizeof(death_stack_bitmap);
	}
	death_stack_bitmap = 0;
	death_stacks = (struct death_stack *)kmalloc(num_death_stacks * sizeof(struct death_stack));
	if(death_stacks == NULL) {
		panic("error creating death stacks\n");
		return ERR_NO_MEMORY;
	}
	{
		char temp[64];

		for(i=0; i<num_death_stacks; i++) {
			sprintf(temp, "death_stack%d", i);
			death_stacks[i].rid = vm_create_anonymous_region(vm_get_kernel_aspace_id(), temp,
				(void **)&death_stacks[i].address,
				REGION_ADDR_ANY_ADDRESS, KSTACK_SIZE, REGION_WIRING_WIRED, LOCK_RW|LOCK_KERNEL);
			if(death_stacks[i].rid < 0) {
				panic("error creating death stacks\n");
				return death_stacks[i].rid;
			}
			death_stacks[i].in_use = false;
		}
	}
	death_stack_sem = create_sem(num_death_stacks, "death_stack_noavail_sem");

	// set up some debugger commands
	dbg_add_command(dump_thread_list, "threads", "list all threads");
	dbg_add_command(dump_thread_info, "thread", "list info about a particular thread");
	dbg_add_command(dump_next_thread_in_q, "next_q", "dump the next thread in the queue of last thread viewed");
	dbg_add_command(dump_next_thread_in_all_list, "next_all", "dump the next thread in the global list of the last thread viewed");
	dbg_add_command(dump_next_thread_in_proc, "next_proc", "dump the next thread in the process of the last thread viewed");
	dbg_add_command(dump_proc_info, "proc", "list info about a particular process");

	return 0;
}

int thread_init_percpu(int cpu_num)
{
	arch_thread_set_current_thread(idle_threads[cpu_num]);
	return 0;
}

// this starts the scheduler. Must be run under the context of
// the initial idle thread.
void thread_start_threading(void)
{
	int state;

	// XXX may not be the best place for this
	// invalidate all of the other processors' TLB caches
	state = int_disable_interrupts();
	arch_cpu_global_TLB_invalidate();
	smp_send_broadcast_ici(SMP_MSG_GLOBAL_INVL_PAGE, 0, 0, 0, NULL, SMP_MSG_FLAG_SYNC);
	int_restore_interrupts(state);

	// start the other processors
	smp_send_broadcast_ici(SMP_MSG_RESCHEDULE, 0, 0, 0, NULL, SMP_MSG_FLAG_ASYNC);

	state = int_disable_interrupts();
	GRAB_THREAD_LOCK();

	thread_resched();

	RELEASE_THREAD_LOCK();
	int_restore_interrupts(state);
}

int user_thread_snooze(bigtime_t time)
{
	thread_snooze(time);
	return B_NO_ERROR;
}

void thread_snooze(bigtime_t time)
{
	acquire_sem_etc(snooze_sem, 1, B_TIMEOUT, time);
}

// this function gets run by a new thread before anything else
static void thread_entry(void)
{
	// simulates the thread spinlock release that would occur if the thread had been
	// rescheded from. The resched didn't happen because the thread is new.
	RELEASE_THREAD_LOCK();
	int_enable_interrupts(); // this essentially simulates a return-from-interrupt
}

// used to pass messages between thread_exit and thread_exit2
struct thread_exit_args {
	struct thread *t;
	region_id old_kernel_stack;
	int int_state;
	unsigned int death_stack;
};

static void thread_exit2(void *_args)
{
	struct thread_exit_args args;
//	char *temp;

	// copy the arguments over, since the source is probably on the kernel stack we're about to delete
	memcpy(&args, _args, sizeof(struct thread_exit_args));

	// restore the interrupts
	int_restore_interrupts(args.int_state);

//	dprintf("thread_exit2, running on death stack 0x%lx\n", args.t->kernel_stack_base);

	// delete the old kernel stack region
//	dprintf("thread_exit2: deleting old kernel stack id 0x%x for thread 0x%x\n", args.old_kernel_stack, args.t->id);
	vm_delete_region(vm_get_kernel_aspace_id(), args.old_kernel_stack);

//	dprintf("thread_exit2: removing thread 0x%x from global lists\n", args.t->id);

	// remove this thread from all of the global lists
	int_disable_interrupts();
	GRAB_PROC_LOCK();
	remove_thread_from_proc(kernel_proc, args.t);
	RELEASE_PROC_LOCK();
	GRAB_THREAD_LOCK();
	hash_remove(thread_hash, args.t);
	RELEASE_THREAD_LOCK();

//	dprintf("thread_exit2: done removing thread from lists\n");

	// set the next state to be gone. Will return the thread structure to a ready pool upon reschedule
	args.t->next_state = THREAD_STATE_FREE_ON_RESCHED;

	// return the death stack and reschedule one last time
	put_death_stack_and_reschedule(args.death_stack);
	// never get to here
	panic("thread_exit2: made it where it shouldn't have!\n");
}

void thread_exit(int retcode)
{
	int state;
	struct thread *t = thread_get_current_thread();
	struct proc *p = t->proc;
	bool delete_proc = false;
	unsigned int death_stack;

	dprintf("thread 0x%x exiting w/return code 0x%x\n", t->id, retcode);

	// boost our priority to get this over with
	thread_set_priority(t->id, THREAD_HIGH_PRIORITY);

	// delete the user stack region first
	if(p->_aspace_id >= 0 && t->user_stack_region_id >= 0) {
		region_id rid = t->user_stack_region_id;
		t->user_stack_region_id = -1;
		vm_delete_region(p->_aspace_id, rid);
	}

	if(p != kernel_proc) {
		// remove this thread from the current process and add it to the kernel
		// put the thread into the kernel proc until it dies
		state = int_disable_interrupts();
		GRAB_PROC_LOCK();
		remove_thread_from_proc(p, t);
		insert_thread_into_proc(kernel_proc, t);
		if(p->main_thread == t) {
			// this was main thread in this process
			delete_proc = true;
			hash_remove(proc_hash, p);
			p->state = PROC_STATE_DEATH;
		}
		RELEASE_PROC_LOCK();
		// swap address spaces, to make sure we're running on the kernel's pgdir
		vm_aspace_swap(kernel_proc->kaspace);
		int_restore_interrupts(state);

//		dprintf("thread_exit: thread 0x%x now a kernel thread!\n", t->id);
	}

	// delete the process
	if(delete_proc) {
		if(p->num_threads > 0) {
			// there are other threads still in this process,
			// cycle through and signal kill on each of the threads
			// XXX this can be optimized. There's got to be a better solution.
			struct thread *temp_thread;

			state = int_disable_interrupts();
			GRAB_PROC_LOCK();
			// we can safely walk the list because of the lock. no new threads can be created
			// because of the PROC_STATE_DEATH flag on the process
			temp_thread = p->thread_list;
			while(temp_thread) {
				struct thread *next = temp_thread->proc_next;
				thread_kill_thread_nowait(temp_thread->id);
				temp_thread = next;
			}
			RELEASE_PROC_LOCK();
			int_restore_interrupts(state);

			// Now wait for all of the threads to die
			// XXX block on a semaphore
			while((volatile int)p->num_threads > 0) {
				thread_snooze(10000); // 10 ms
			}
		}
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
		delete_sem_etc(s, retcode);
	}

	death_stack = get_death_stack();
	{
		struct thread_exit_args args;

		args.t = t;
		args.old_kernel_stack = t->kernel_stack_region_id;
		args.death_stack = death_stack;

		// disable the interrupts. Must remain disabled until the kernel stack pointer can be officially switched
		args.int_state = int_disable_interrupts();

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

static int _thread_kill_thread(thread_id id, bool wait_on)
{
	int state;
	struct thread *t;
	int rc;

//	dprintf("_thread_kill_thread: id %d, wait_on %d\n", id, wait_on);

	state = int_disable_interrupts();
	GRAB_THREAD_LOCK();

	t = thread_get_thread_struct_locked(id);
	if(t != NULL) {
		if(t->proc == kernel_proc) {
			// can't touch this
			rc = ERR_NOT_ALLOWED;
		} else {
			deliver_signal(t, SIG_KILL);
			rc = B_NO_ERROR;
			if(t->id == thread_get_current_thread()->id)
				wait_on = false; // can't wait on ourself
		}
	} else {
		rc = ERR_INVALID_HANDLE;
	}

	RELEASE_THREAD_LOCK();
	int_restore_interrupts(state);
	if(rc < 0)
		return rc;

	if(wait_on)
		thread_wait_on_thread(id, NULL);

	return rc;
}

int thread_kill_thread(thread_id id)
{
	return _thread_kill_thread(id, true);
}

int thread_kill_thread_nowait(thread_id id)
{
	return _thread_kill_thread(id, false);
}

static void thread_kthread_exit(void)
{
	thread_exit(0);
}

int user_thread_wait_on_thread(thread_id id, int *uretcode)
{
	int retcode;
	int rc, rc2;

	if((addr)uretcode >= KERNEL_BASE && (addr)uretcode <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	rc = thread_wait_on_thread(id, &retcode);

	rc2 = user_memcpy(uretcode, &retcode, sizeof(retcode));
	if(rc2 < 0)
		return rc2;

	return rc;
}

int thread_wait_on_thread(thread_id id, int *retcode)
{
	sem_id sem;
	int state;
	struct thread *t;
	int rc;

	state = int_disable_interrupts();
	GRAB_THREAD_LOCK();

	t = thread_get_thread_struct_locked(id);
	if(t != NULL) {
		sem = t->return_code_sem;
	} else {
		sem = ERR_INVALID_HANDLE;
	}

	RELEASE_THREAD_LOCK();
	int_restore_interrupts(state);

	rc = acquire_sem(sem);

	/* This thread died the way it should, dont ripple a non-error up */
	if (rc == ERR_SEM_DELETED)
		rc = B_NO_ERROR;

	return rc;
}

int user_proc_wait_on_proc(proc_id id, int *uretcode)
{
	int retcode;
	int rc, rc2;

	if((addr)uretcode >= KERNEL_BASE && (addr)uretcode <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	rc = proc_wait_on_proc(id, &retcode);
	if(rc < 0)
		return rc;

	rc2 = user_memcpy(uretcode, &retcode, sizeof(retcode));
	if(rc2 < 0)
		return rc2;

	return rc;
}

int proc_wait_on_proc(proc_id id, int *retcode)
{
	struct proc *p;
	thread_id tid;
	int state;

	state = int_disable_interrupts();
	GRAB_PROC_LOCK();
	p = proc_get_proc_struct_locked(id);
	if(p && p->main_thread) {
		tid = p->main_thread->id;
	} else {
		tid = ERR_INVALID_HANDLE;
	}
	RELEASE_PROC_LOCK();
	int_restore_interrupts(state);

	if(tid < 0)
		return tid;

	return thread_wait_on_thread(tid, retcode);
}

struct thread *thread_get_thread_struct(thread_id id)
{
	struct thread *t;
	int state;

	state = int_disable_interrupts();
	GRAB_THREAD_LOCK();

	t = thread_get_thread_struct_locked(id);

	RELEASE_THREAD_LOCK();
	int_restore_interrupts(state);

	return t;
}

static struct thread *thread_get_thread_struct_locked(thread_id id)
{
	struct thread_key key;

	key.id = id;

	return hash_lookup(thread_hash, &key);
}

/* XXX - static but unused
static struct proc *proc_get_proc_struct(proc_id id)
{
	struct proc *p;
	int state;

	state = int_disable_interrupts();
	GRAB_PROC_LOCK();

	p = proc_get_proc_struct_locked(id);

	RELEASE_PROC_LOCK();
	int_restore_interrupts(state);

	return p;
}
*/

static struct proc *proc_get_proc_struct_locked(proc_id id)
{
	struct proc_key key;

	key.id = id;

	return hash_lookup(proc_hash, &key);
}

static void thread_context_switch(struct thread *t_from, struct thread *t_to)
{
	bigtime_t now;

	// track kernel time
	now = system_time();
	t_from->kernel_time += now - t_from->last_time;
	t_to->last_time = now;

	t_to->cpu = t_from->cpu;
	arch_thread_set_current_thread(t_to);
	t_from->cpu = NULL;
	arch_thread_context_switch(t_from, t_to);
}

static int _rand(void)
{
	static int next = 0;

	if(next == 0)
		next = system_time();

	next = next * 1103515245 + 12345;
	return((next >> 16) & 0x7FFF);
}

static int reschedule_event(void *unused)
{
	// this function is called as a result of the timer event set by the scheduler
	// returning this causes a reschedule on the timer event
	thread_get_current_thread()->cpu->info.preempted= 1;
	return INT_RESCHEDULE;
}

// NOTE: expects thread_spinlock to be held
void thread_resched(void)
{
	struct thread *next_thread = NULL;
	int last_thread_pri = -1;
	struct thread *old_thread = thread_get_current_thread();
	int i;
	bigtime_t quantum;
	struct timer_event *quantum_timer;

//	dprintf("top of thread_resched: cpu %d, cur_thread = 0x%x\n", smp_get_current_cpu(), thread_get_current_thread());

	switch(old_thread->next_state) {
		case THREAD_STATE_RUNNING:
		case THREAD_STATE_READY:
//			dprintf("enqueueing thread 0x%x into run q. pri = %d\n", old_thread, old_thread->priority);
			thread_enqueue_run_q(old_thread);
			break;
		case THREAD_STATE_SUSPENDED:
			dprintf("suspending thread 0x%x\n", old_thread->id);
			break;
		case THREAD_STATE_FREE_ON_RESCHED:
			thread_enqueue(old_thread, &dead_q);
			break;
		default:
//			dprintf("not enqueueing thread 0x%x into run q. next_state = %d\n", old_thread, old_thread->next_state);
			;
	}
	old_thread->state = old_thread->next_state;

	// search the real-time queue
	for(i = THREAD_MAX_RT_PRIORITY; i >= THREAD_MIN_RT_PRIORITY; i--) {
		next_thread = thread_dequeue_run_q(i);
		if(next_thread)
			goto found_thread;
	}

	// search the regular queue
	for(i = THREAD_MAX_PRIORITY; i > THREAD_IDLE_PRIORITY; i--) {
		next_thread = thread_lookat_run_q(i);
		if(next_thread != NULL) {
			// skip it sometimes
			if(_rand() > 0x3000) {
				next_thread = thread_dequeue_run_q(i);
				goto found_thread;
			}
			last_thread_pri = i;
			next_thread = NULL;
		}
	}
	if(next_thread == NULL) {
		if(last_thread_pri != -1) {
			next_thread = thread_dequeue_run_q(last_thread_pri);
			if(next_thread == NULL)
				panic("next_thread == NULL! last_thread_pri = %d\n", last_thread_pri);
		} else {
			next_thread = thread_dequeue_run_q(THREAD_IDLE_PRIORITY);
			if(next_thread == NULL)
				panic("next_thread == NULL! no idle priorities!\n");
		}
	}

found_thread:
	next_thread->state = THREAD_STATE_RUNNING;
	next_thread->next_state = THREAD_STATE_READY;

	// XXX should only reset the quantum timer if we are switching to a new thread,
	// or we got here as a result of a quantum expire.

	// XXX calculate quantum
	quantum = 10000;

	// get the quantum timer for this cpu
	quantum_timer = &old_thread->cpu->info.quantum_timer;
	if(!old_thread->cpu->info.preempted) {
		_local_timer_cancel_event(old_thread->cpu->info.cpu_num, quantum_timer);
	}
	old_thread->cpu->info.preempted= 0;
	timer_setup_timer(&reschedule_event, NULL, quantum_timer);
	timer_set_event(quantum, TIMER_MODE_ONESHOT, quantum_timer);

	if(next_thread != old_thread) {
//		dprintf("thread_resched: cpu %d switching from thread %d to %d\n",
//			smp_get_current_cpu(), old_thread->id, next_thread->id);
		thread_context_switch(old_thread, next_thread);
	}
}

static int proc_struct_compare(void *_p, const void *_key)
{
	struct proc *p = _p;
	const struct proc_key *key = _key;

	if(p->id == key->id) return 0;
	else return 1;
}

static unsigned int proc_struct_hash(void *_p, const void *_key, unsigned int range)
{
	struct proc *p = _p;
	const struct proc_key *key = _key;

	if(p != NULL)
		return (p->id % range);
	else
		return (key->id % range);
}

struct proc *proc_get_kernel_proc(void)
{
	return kernel_proc;
}

proc_id proc_get_kernel_proc_id(void)
{
	if(!kernel_proc)
		return 0;
	else
		return kernel_proc->id;
}

proc_id proc_get_current_proc_id(void)
{
	return thread_get_current_thread()->proc->id;
}

static struct proc *create_proc_struct(const char *name, bool kernel)
{
	struct proc *p;

	p = (struct proc *)kmalloc(sizeof(struct proc));
	if(p == NULL)
		goto error;
	p->id = atomic_add(&next_proc_id, 1);
	strncpy(&p->name[0], name, SYS_MAX_OS_NAME_LEN-1);
	p->name[SYS_MAX_OS_NAME_LEN-1] = 0;
	p->num_threads = 0;
	p->ioctx = NULL;
	p->path[0] = 0;
	p->_aspace_id = -1;
	p->aspace = NULL;
	p->kaspace = vm_get_kernel_aspace();
	vm_put_aspace(p->kaspace);
	p->thread_list = NULL;
	p->main_thread = NULL;
	p->state = PROC_STATE_BIRTH;
	p->pending_signals = SIG_NONE;

	if(arch_proc_init_proc_struct(p, kernel) < 0)
		goto error1;

	return p;

error1:
	kfree(p);
error:
	return NULL;
}

static void delete_proc_struct(struct proc *p)
{
	kfree(p);
}

static int get_arguments_data_size(char **args,int argc)
{
	int cnt;
	int tot_size = 0;

	for(cnt = 0; cnt < argc; cnt++)
		tot_size += strlen(args[cnt]) + 1;
	tot_size += (argc + 1) * sizeof(char *);

	return tot_size + sizeof(struct uspace_prog_args_t);
}

static int proc_create_proc2(void *args)
{
	int err;
	struct thread *t;
	struct proc *p;
	struct proc_arg *pargs = args;
	char *path;
	addr entry;
	char ustack_name[128];
	int tot_top_size;
	char **uargs;
	char *udest;
	struct uspace_prog_args_t *uspa;
	unsigned int  cnt;

	t = thread_get_current_thread();
	p = t->proc;

	dprintf("proc_create_proc2: entry thread %d\n", t->id);

	// create an initial primary stack region

	tot_top_size = STACK_SIZE + PAGE_ALIGN(get_arguments_data_size(pargs->args,pargs->argc));
	t->user_stack_base = ((USER_STACK_REGION  - tot_top_size) + USER_STACK_REGION_SIZE);
	sprintf(ustack_name, "%s_primary_stack", p->name);
	t->user_stack_region_id = vm_create_anonymous_region(p->_aspace_id, ustack_name, (void **)&t->user_stack_base,
		REGION_ADDR_EXACT_ADDRESS, tot_top_size, REGION_WIRING_LAZY, LOCK_RW);
	if(t->user_stack_region_id < 0) {
		panic("proc_create_proc2: could not create default user stack region\n");
		return t->user_stack_region_id;
	}

	uspa  = (struct uspace_prog_args_t *)(t->user_stack_base + STACK_SIZE);
	uargs = (char **)(uspa + 1);
	udest = (char  *)(uargs + pargs->argc + 1);
//	dprintf("addr: stack base=0x%x uargs = 0x%x  udest=0x%x tot_top_size=%d \n\n",t->user_stack_base,uargs,udest,tot_top_size);

	for(cnt = 0;cnt < pargs->argc;cnt++){
		uargs[cnt] = udest;
		user_strcpy(udest, pargs->args[cnt]);
		udest += strlen(pargs->args[cnt]) + 1;
	}
	uargs[cnt] = NULL;

	user_memcpy(uspa->prog_name, p->name, sizeof(uspa->prog_name));
	user_memcpy(uspa->prog_path, pargs->path, sizeof(uspa->prog_path));
	uspa->argc = cnt;
	uspa->argv = uargs;
	uspa->envc = 0;
	uspa->envp = 0;

	if(pargs->args != NULL)
		free_arg_list(pargs->args,pargs->argc);

	path = pargs->path;
	dprintf("proc_create_proc2: loading elf binary '%s'\n", path);

	err = elf_load_uspace("/boot/libexec/rld.so", p, 0, &entry);
	if(err < 0){
		// XXX clean up proc
		return err;
	}

	// free the args
	kfree(pargs->path);
	kfree(pargs);

	dprintf("proc_create_proc2: loaded elf. entry = 0x%lx\n", entry);

	p->state = PROC_STATE_NORMAL;

	// jump to the entry point in user space
	arch_thread_enter_uspace(entry, uspa, t->user_stack_base + STACK_SIZE);

	// never gets here
	return 0;
}

proc_id proc_create_proc(const char *path, const char *name, char **args, int argc, int priority)
{
	struct proc *p;
	thread_id tid;
	proc_id pid;
	int err;
	unsigned int state;
//	int sem_retcode;
	struct proc_arg *pargs;

	dprintf("proc_create_proc: entry '%s', name '%s' args = %p argc = %d\n", path, name, args, argc);

	p = create_proc_struct(name, false);
	if(p == NULL)
		return ERR_NO_MEMORY;

	pid = p->id;

	state = int_disable_interrupts();
	GRAB_PROC_LOCK();
	hash_insert(proc_hash, p);
	RELEASE_PROC_LOCK();
	int_restore_interrupts(state);

	// copy the args over
	pargs = (struct proc_arg *)kmalloc(sizeof(struct proc_arg));
	if(pargs == NULL){
		err = ERR_NO_MEMORY;
		goto err1;
	}
	pargs->path = (char *)kstrdup(path);
	if(pargs->path == NULL){
		err = ERR_NO_MEMORY;
		goto err2;
	}
	pargs->argc = argc;
	pargs->args = args;

	// create a new ioctx for this process
	p->ioctx = vfs_new_io_context(thread_get_current_thread()->proc->ioctx);
	if(!p->ioctx) {
		err = ERR_NO_MEMORY;
		goto err3;
	}

	//XXX should set p->path to path(?) here.

	// create an address space for this process
	p->_aspace_id = vm_create_aspace(p->name, USER_BASE, USER_SIZE, false);
	if (p->_aspace_id < 0) {
		err = p->_aspace_id;
		goto err4;
	}
	p->aspace = vm_get_aspace_by_id(p->_aspace_id);

	// create a kernel thread, but under the context of the new process
	tid = thread_create_kernel_thread_etc(name, proc_create_proc2, pargs, p);
	if (tid < 0) {
		err = tid;
		goto err5;
	}

	thread_resume_thread(tid);

	return pid;

err5:
	vm_put_aspace(p->aspace);
	vm_delete_aspace(p->_aspace_id);
err4:
	vfs_free_io_context(p->ioctx);
err3:
	kfree(pargs->path);
err2:
	kfree(pargs);
err1:
	// remove the proc structure from the proc hash table and delete the proc structure
	state = int_disable_interrupts();
	GRAB_PROC_LOCK();
	hash_remove(proc_hash, p);
	RELEASE_PROC_LOCK();
	int_restore_interrupts(state);
	delete_proc_struct(p);
//err:
	return err;
}

proc_id user_proc_create_proc(const char *upath, const char *uname, char **args, int argc, int priority)
{
	char path[SYS_MAX_PATH_LEN];
	char name[SYS_MAX_OS_NAME_LEN];
	char **kargs;
	int rc;

	dprintf("user_proc_create_proc : argc=%d \n",argc);

	if((addr)upath >= KERNEL_BASE && (addr)upath <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;
	if((addr)uname >= KERNEL_BASE && (addr)uname <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	rc = user_copy_arg_list(args, argc, &kargs);
	if(rc < 0)
		goto error;

	rc = user_strncpy(path, upath, SYS_MAX_PATH_LEN-1);
	if(rc < 0)
		goto error;

	path[SYS_MAX_PATH_LEN-1] = 0;

	rc = user_strncpy(name, uname, SYS_MAX_OS_NAME_LEN-1);
	if(rc < 0)
		goto error;

	name[SYS_MAX_OS_NAME_LEN-1] = 0;

	return proc_create_proc(path, name, kargs, argc, priority);
error:
	free_arg_list(kargs,argc);
	return rc;
}


// used by PS command and anything else interested in a process list
int user_proc_get_table(struct proc_info *pbuf, size_t len)
{
	struct proc *p;
	struct hash_iterator i;
	struct proc_info pi;
	int state;
	int count=0;
	int max = (len / sizeof(struct proc_info));

	if((addr)pbuf >= KERNEL_BASE && (addr)pbuf <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	state = int_disable_interrupts();
	GRAB_PROC_LOCK();

	hash_open(proc_hash, &i);
	while(((p = hash_next(proc_hash, &i)) != NULL) && (count < max)) {
		pi.id = p->id;
		strcpy(pi.name, p->name);
		pi.state = p->state;
		pi.num_threads = p->num_threads;
		count++;
		user_memcpy(pbuf, &pi, sizeof(struct proc_info));
		pbuf=pbuf + sizeof(struct proc_info);
	}
	hash_close(proc_hash, &i, false);

	RELEASE_PROC_LOCK();
	int_restore_interrupts(state);

	if (count < max)
		return count;
	else
		return ERR_NO_MEMORY;
}


int proc_kill_proc(proc_id id)
{
	int state;
	struct proc *p;
//	struct thread *t;
	thread_id tid = -1;
	int retval = 0;

	state = int_disable_interrupts();
	GRAB_PROC_LOCK();

	p = proc_get_proc_struct_locked(id);
	if(p != NULL) {
		tid = p->main_thread->id;
	} else {
		retval = ERR_INVALID_HANDLE;
	}

	RELEASE_PROC_LOCK();
	int_restore_interrupts(state);
	if(retval < 0)
		return retval;

	// just kill the main thread in the process. The cleanup code there will
	// take care of the process
	return thread_kill_thread(tid);
}

// sets the pending signal flag on a thread and possibly does some work to wake it up, etc.
// expects the thread lock to be held
static void deliver_signal(struct thread *t, int signal)
{
//	dprintf("deliver_signal: thread %p (%d), signal %d\n", t, t->id, signal);
	switch(signal) {
		case SIG_KILL:
			t->pending_signals |= SIG_KILL;
			switch(t->state) {
				case THREAD_STATE_SUSPENDED:
					t->state = THREAD_STATE_READY;
					t->next_state = THREAD_STATE_READY;

					thread_enqueue_run_q(t);
					break;
				case THREAD_STATE_WAITING:
					sem_interrupt_thread(t);
					break;
				default:
					;
			}
			break;
		default:
			t->pending_signals |= signal;
	}
}

// expects the thread lock to be held
static void _check_for_thread_sigs(struct thread *t, int state)
{
	if(t->pending_signals == SIG_NONE)
		return;

	if(t->pending_signals & SIG_KILL) {
		t->pending_signals &= ~SIG_KILL;

		RELEASE_THREAD_LOCK();
		int_restore_interrupts(state);
		thread_exit(0);
		// never gets to here
	}
	if(t->pending_signals & SIG_SUSPEND) {
		t->pending_signals &= ~SIG_SUSPEND;
		t->next_state = THREAD_STATE_SUSPENDED;
		// XXX will probably want to delay this
		thread_resched();
	}
}

// called in the int handler code when a thread enters the kernel for any reason
void thread_atkernel_entry(void)
{
	int state;
	struct thread *t;
	bigtime_t now;

//	dprintf("thread_atkernel_entry: entry thread 0x%x\n", t->id);

	t = thread_get_current_thread();

	state = int_disable_interrupts();

	// track user time
	now = system_time();
	t->user_time += now - t->last_time;
	t->last_time = now;

	GRAB_THREAD_LOCK();

	t->in_kernel = true;

	_check_for_thread_sigs(t, state);

	RELEASE_THREAD_LOCK();
	int_restore_interrupts(state);
}

// called when a thread exits kernel space to user space
void thread_atkernel_exit(void)
{
	int state;
	struct thread *t;
	bigtime_t now;

//	dprintf("thread_atkernel_exit: entry\n");

	t = thread_get_current_thread();

	state = int_disable_interrupts();
	GRAB_THREAD_LOCK();

	_check_for_thread_sigs(t, state);

	t->in_kernel = false;

	RELEASE_THREAD_LOCK();

	// track kernel time
	now = system_time();
	t->kernel_time += now - t->last_time;
	t->last_time = now;

	int_restore_interrupts(state);
}

int user_getrlimit(int resource, struct rlimit * urlp)
{
	int				ret;
	struct rlimit	rl;

	if (urlp == NULL) {
		return ERR_INVALID_ARGS;
	}
	if((addr)urlp >= KERNEL_BASE && (addr)urlp <= KERNEL_TOP) {
		return ERR_VM_BAD_USER_MEMORY;
	}

	ret = getrlimit(resource, &rl);

	if (ret == 0) {
		ret = user_memcpy(urlp, &rl, sizeof(struct rlimit));
		if (ret < 0) {
			return ret;
		}
		return 0;
	}

	return ret;
}

int getrlimit(int resource, struct rlimit * rlp)
{
	if (!rlp) {
		return -1;
	}

	switch(resource) {
		case RLIMIT_NOFILE:
			return vfs_getrlimit(resource, rlp);

		default:
			return -1;
	}

	return 0;
}

int user_setrlimit(int resource, const struct rlimit * urlp)
{
	int				err;
	struct rlimit	rl;

	if (urlp == NULL) {
		return ERR_INVALID_ARGS;
	}
	if((addr)urlp >= KERNEL_BASE && (addr)urlp <= KERNEL_TOP) {
		return ERR_VM_BAD_USER_MEMORY;
	}

	err = user_memcpy(&rl, urlp, sizeof(struct rlimit));
	if (err < 0) {
		return err;
	}

	return setrlimit(resource, &rl);
}

int setrlimit(int resource, const struct rlimit * rlp)
{
	if (!rlp) {
		return -1;
	}

	switch(resource) {
		case RLIMIT_NOFILE:
			return vfs_setrlimit(resource, rlp);

		default:
			return -1;
	}

	return 0;
}
