/*
 * Copyright 2002-2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

/* Functionality for symetrical multi-processors */

#include <thread.h>
#include <int.h>
#include <smp.h>
#include <cpu.h>
#include <arch/cpu.h>
#include <arch/smp.h>
#include <arch/int.h>
#include <arch/debug.h>

#include <stdlib.h>
#include <string.h>

#define DEBUG_SPINLOCKS 1
//#define TRACE_SMP

#ifdef TRACE_SMP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#if __INTEL__
  #define PAUSE() asm volatile ("rep; nop;")
#else
  #define PAUSE()
#endif

#define MSG_POOL_SIZE (SMP_MAX_CPUS * 4)

struct smp_msg {
	struct smp_msg	*next;
	int32			message;
	uint32			data;
	uint32			data2;
	uint32			data3;
	void			*data_ptr;
	uint32			flags;
	int32			ref_count;
	volatile bool	done;
	uint32			proc_bitmap;
};

#define MAILBOX_LOCAL 1
#define MAILBOX_BCAST 2

static spinlock boot_cpu_spin[SMP_MAX_CPUS] = { 0, };

static struct smp_msg *free_msgs = NULL;
static volatile int free_msg_count = 0;
static spinlock free_msg_spinlock = 0;

static struct smp_msg *smp_msgs[SMP_MAX_CPUS] = { NULL, };
static spinlock cpu_msg_spinlock[SMP_MAX_CPUS] = { 0, };

static struct smp_msg *smp_broadcast_msgs = NULL;
static spinlock broadcast_msg_spinlock = 0;

static bool sICIEnabled = false;
static int32 sNumCPUs = 1;

static int32 process_pending_ici(int32 currentCPU);


#if DEBUG_SPINLOCKS
#define NUM_LAST_CALLERS	32

static struct {
	void		*caller;
	spinlock	*lock;
} sLastCaller[NUM_LAST_CALLERS];
static int32 sLastIndex = 0;


static void
push_lock_caller(void *caller, spinlock *lock)
{
	sLastCaller[sLastIndex].caller = caller;
	sLastCaller[sLastIndex].lock = lock;	

	if (++sLastIndex >= NUM_LAST_CALLERS)
		sLastIndex = 0;
}


static void *
find_lock_caller(spinlock *lock)
{
	int32 i;

	for (i = 0; i < NUM_LAST_CALLERS; i++) {
		int32 index = (NUM_LAST_CALLERS + sLastIndex - 1 - i) % NUM_LAST_CALLERS;
		if (sLastCaller[index].lock == lock)
			return sLastCaller[index].caller;
	}

	return NULL;
}
#endif	// DEBUG_SPINLOCKS


void
acquire_spinlock(spinlock *lock)
{
	if (sNumCPUs > 1) {
		int currentCPU = smp_get_current_cpu();
		if (are_interrupts_enabled())
			panic("acquire_spinlock: attempt to acquire lock %p with interrupts enabled\n", lock);
		while (1) {
			while (*lock != 0) {
				process_pending_ici(currentCPU);
				PAUSE();
			}
			if (atomic_set((int32 *)lock, 1) == 0)
				break;
		}
	} else {
#if DEBUG_SPINLOCKS
		int32 oldValue;
		if (are_interrupts_enabled())
			panic("acquire_spinlock: attempt to acquire lock %p with interrupts enabled\n", lock);
		oldValue = atomic_set((int32 *)lock, 1);
		if (oldValue != 0) {
			panic("acquire_spinlock: attempt to acquire lock %p twice on non-SMP system (last caller: %p, value %ld)\n", 
				lock, find_lock_caller(lock), oldValue);
		}

		push_lock_caller(arch_debug_get_caller(), lock);
#endif
	}
}
 

static void
acquire_spinlock_nocheck(spinlock *lock)
{
	if (sNumCPUs > 1) {
#if DEBUG_SPINLOCKS
		if (are_interrupts_enabled())
			panic("acquire_spinlock_nocheck: attempt to acquire lock %p with interrupts enabled\n", lock);
#endif
		while (1) {
			while(*lock != 0)
				PAUSE();
			if (atomic_set((int32 *)lock, 1) == 0)
				break;
		}
	} else {
#if DEBUG_SPINLOCKS
		if (are_interrupts_enabled())
			panic("acquire_spinlock_nocheck: attempt to acquire lock %p with interrupts enabled\n", lock);
		if (atomic_set((int32 *)lock, 1) != 0)
			panic("acquire_spinlock_nocheck: attempt to acquire lock %p twice on non-SMP system\n", lock);
#endif
	}
}


void
release_spinlock(spinlock *lock)
{
	if (sNumCPUs > 1) {
		if (are_interrupts_enabled())
			panic("release_spinlock: attempt to release lock %p with interrupts enabled\n", lock);
		if (atomic_set((int32 *)lock, 0) != 1)
			panic("release_spinlock: lock %p was already released\n", lock);
	} else {
		#if DEBUG_SPINLOCKS
			if (are_interrupts_enabled())
				panic("release_spinlock: attempt to release lock %p with interrupts enabled\n", lock);
			if (atomic_set((int32 *)lock, 0) != 1)
				panic("release_spinlock: lock %p was already released\n", lock);
		#endif
	}
}


/** Finds a free message and gets it.
 *	NOTE: has side effect of disabling interrupts
 *	return value is the former interrupt state
 */

static cpu_status
find_free_message(struct smp_msg **msg)
{
	cpu_status state;

	TRACE(("find_free_message: entry\n"));

retry:
	while (free_msg_count <= 0) 
		PAUSE();
	state = disable_interrupts();
	acquire_spinlock(&free_msg_spinlock);

	if (free_msg_count <= 0) {
		// someone grabbed one while we were getting the lock,
		// go back to waiting for it
		release_spinlock(&free_msg_spinlock);
		restore_interrupts(state);
		goto retry;
	}

	*msg = free_msgs;
	free_msgs = (*msg)->next;
	free_msg_count--;

	release_spinlock(&free_msg_spinlock);

	TRACE(("find_free_message: returning msg %p\n", *msg));

	return state;
}


static void
return_free_message(struct smp_msg *msg)
{
	TRACE(("return_free_message: returning msg %p\n", msg));

	acquire_spinlock_nocheck(&free_msg_spinlock);
	msg->next = free_msgs;
	free_msgs = msg;
	free_msg_count++;
	release_spinlock(&free_msg_spinlock);
}


static struct smp_msg *
check_for_message(int currentCPU, int *source_mailbox)
{
	struct smp_msg *msg;

	if (!sICIEnabled)
		return NULL;

	acquire_spinlock_nocheck(&cpu_msg_spinlock[currentCPU]);
	msg = smp_msgs[currentCPU];
	if (msg != NULL) {
		smp_msgs[currentCPU] = msg->next;
		release_spinlock(&cpu_msg_spinlock[currentCPU]);
		TRACE((" found msg %p in cpu mailbox\n", msg));
		*source_mailbox = MAILBOX_LOCAL;
	} else {
		// try getting one from the broadcast mailbox

		release_spinlock(&cpu_msg_spinlock[currentCPU]);
		acquire_spinlock_nocheck(&broadcast_msg_spinlock);

		msg = smp_broadcast_msgs;
		while (msg != NULL) {
			if (CHECK_BIT(msg->proc_bitmap, currentCPU) != 0) {
				// we have handled this one already
				msg = msg->next;
				continue;
			}

			// mark it so we wont try to process this one again
			msg->proc_bitmap = SET_BIT(msg->proc_bitmap, currentCPU);
			*source_mailbox = MAILBOX_BCAST;
			break;
		}
		release_spinlock(&broadcast_msg_spinlock);
		TRACE((" found msg %p in broadcast mailbox\n", msg));
	}
	return msg;
}


static void
finish_message_processing(int currentCPU, struct smp_msg *msg, int source_mailbox)
{
	int old_refcount;

	old_refcount = atomic_add(&msg->ref_count, -1);
	if (old_refcount == 1) {
		// we were the last one to decrement the ref_count
		// it's our job to remove it from the list & possibly clean it up
		struct smp_msg **mbox = NULL;
		spinlock *spinlock = NULL;

		// clean up the message from one of the mailboxes
		switch (source_mailbox) {
			case MAILBOX_BCAST:
				mbox = &smp_broadcast_msgs;
				spinlock = &broadcast_msg_spinlock;
				break;
			case MAILBOX_LOCAL:
				mbox = &smp_msgs[currentCPU];
				spinlock = &cpu_msg_spinlock[currentCPU];
				break;
		}

		acquire_spinlock_nocheck(spinlock);

		TRACE(("cleaning up message %p\n", msg));

		if (msg == *mbox) {
			(*mbox) = msg->next;
		} else {
			// we need to walk to find the message in the list.
			// we can't use any data found when previously walking through
			// the list, since the list may have changed. But, we are guaranteed
			// to at least have msg in it.
			struct smp_msg *last = NULL;
			struct smp_msg *msg1;

			msg1 = *mbox;
			while (msg1 != NULL && msg1 != msg) {
				last = msg1;
				msg1 = msg1->next;
			}

			// by definition, last must be something
			if (msg1 == msg && last != NULL)
				last->next = msg->next;
			else
				dprintf("last == NULL or msg != msg1!!!\n");
		}

		release_spinlock(spinlock);

		if ((msg->flags & SMP_MSG_FLAG_FREE_ARG) != 0 && msg->data_ptr != NULL)
			free(msg->data_ptr);

		if (msg->flags & SMP_MSG_FLAG_SYNC) {
			msg->done = true;
			// the caller cpu should now free the message
		} else {
			// in the !SYNC case, we get to free the message
			return_free_message(msg);
		}
	}
}


static int32
process_pending_ici(int32 currentCPU)
{
	struct smp_msg *msg;
	bool halt = false;
	int source_mailbox = 0;
	int retval = B_HANDLED_INTERRUPT;

	msg = check_for_message(currentCPU, &source_mailbox);
	if (msg == NULL)
		return retval;

	TRACE(("  cpu %d message = %d\n", currentCPU, msg->message));

	switch (msg->message) {
		case SMP_MSG_INVALIDATE_PAGE_RANGE:
			arch_cpu_invalidate_TLB_range((addr_t)msg->data, (addr_t)msg->data2);
			break;
		case SMP_MSG_INVALIDATE_PAGE_LIST:
			arch_cpu_invalidate_TLB_list((addr_t *)msg->data, (int)msg->data2);
			break;
		case SMP_MSG_USER_INVALIDATE_PAGES:
			arch_cpu_user_TLB_invalidate();
			break;
		case SMP_MSG_GLOBAL_INVALIDATE_PAGES:
			arch_cpu_global_TLB_invalidate();
			break;
		case SMP_MSG_RESCHEDULE:
			retval = B_INVOKE_SCHEDULER;
			break;
		case SMP_MSG_CPU_HALT:
			halt = true;
			dprintf("cpu %ld halted!\n", currentCPU);
			break;
		case SMP_MSG_CALL_FUNCTION:
		{
			smp_call_func func = (smp_call_func)msg->data_ptr;
			func(msg->data, currentCPU, msg->data2, msg->data3);
			break;
		}

		default:
			dprintf("smp_intercpu_int_handler: got unknown message %ld\n", msg->message);
	}

	// finish dealing with this message, possibly removing it from the list
	finish_message_processing(currentCPU, msg, source_mailbox);

	// special case for the halt message
	// we otherwise wouldn't have gotten the opportunity to clean up
	if (halt) {
		disable_interrupts();
		for(;;);
	}

	return retval;
}


//	#pragma mark -


int
smp_intercpu_int_handler(void)
{
	int retval;
	int currentCPU = smp_get_current_cpu();

	TRACE(("smp_intercpu_int_handler: entry on cpu %d\n", currentCPU));

	retval = process_pending_ici(currentCPU);

	TRACE(("smp_intercpu_int_handler: done\n"));

	return retval;
}


void
smp_send_ici(int32 targetCPU, int32 message, uint32 data, uint32 data2, uint32 data3,
	void *data_ptr, uint32 flags)
{
	struct smp_msg *msg;

	TRACE(("smp_send_ici: target 0x%x, mess 0x%x, data 0x%lx, data2 0x%lx, data3 0x%lx, ptr %p, flags 0x%x\n",
		targetCPU, message, data, data2, data3, data_ptr, flags));

	if (sICIEnabled) {
		int state;
		int currentCPU;

		// find_free_message leaves interrupts disabled
		state = find_free_message(&msg);

		currentCPU = smp_get_current_cpu();
		if (targetCPU == currentCPU) {
			return_free_message(msg);
			restore_interrupts(state);
			return; // nope, cant do that
		}

		// set up the message
		msg->message = message;
		msg->data = data;
		msg->data2 = data2;
		msg->data3 = data3;
		msg->data_ptr = data_ptr;
		msg->ref_count = 1;
		msg->flags = flags;
		msg->done = false;

		// stick it in the appropriate cpu's mailbox
		acquire_spinlock_nocheck(&cpu_msg_spinlock[targetCPU]);
		msg->next = smp_msgs[targetCPU];
		smp_msgs[targetCPU] = msg;
		release_spinlock(&cpu_msg_spinlock[targetCPU]);

		arch_smp_send_ici(targetCPU);

		if (flags & SMP_MSG_FLAG_SYNC) {
			// wait for the other cpu to finish processing it
			// the interrupt handler will ref count it to <0
			// if the message is sync after it has removed it from the mailbox
			while (msg->done == false) {
				process_pending_ici(currentCPU);
				PAUSE();
			}
			// for SYNC messages, it's our responsibility to put it
			// back into the free list
			return_free_message(msg);
		}

		restore_interrupts(state);
	}
}


void
smp_send_broadcast_ici(int32 message, uint32 data, uint32 data2, uint32 data3,
	void *data_ptr, uint32 flags)
{
	struct smp_msg *msg;

	TRACE(("smp_send_broadcast_ici: cpu %d mess 0x%x, data 0x%lx, data2 0x%lx, data3 0x%lx, ptr %p, flags 0x%x\n",
		smp_get_current_cpu(), message, data, data2, data3, data_ptr, flags));

	if (sICIEnabled) {
		int state;
		int currentCPU;

		// find_free_message leaves interrupts disabled
		state = find_free_message(&msg);

		currentCPU = smp_get_current_cpu();

		msg->message = message;
		msg->data = data;
		msg->data2 = data2;
		msg->data3 = data3;
		msg->data_ptr = data_ptr;
		msg->ref_count = sNumCPUs - 1;
		msg->flags = flags;
		msg->proc_bitmap = SET_BIT(0, currentCPU);
		msg->done = false;

		TRACE(("smp_send_broadcast_ici%d: inserting msg %p into broadcast mbox\n",
			smp_get_current_cpu(), msg));

		// stick it in the appropriate cpu's mailbox
		acquire_spinlock_nocheck(&broadcast_msg_spinlock);
		msg->next = smp_broadcast_msgs;
		smp_broadcast_msgs = msg;
		release_spinlock(&broadcast_msg_spinlock);

		arch_smp_send_broadcast_ici();

		TRACE(("smp_send_broadcast_ici: sent interrupt\n"));

		if (flags & SMP_MSG_FLAG_SYNC) {
			// wait for the other cpus to finish processing it
			// the interrupt handler will ref count it to <0
			// if the message is sync after it has removed it from the mailbox
			TRACE(("smp_send_broadcast_ici: waiting for ack\n"));

			while (msg->done == false) {
				process_pending_ici(currentCPU);
				PAUSE();
			}

			TRACE(("smp_send_broadcast_ici: returning message to free list\n"));

			// for SYNC messages, it's our responsibility to put it
			// back into the free list
			return_free_message(msg);
		}

		restore_interrupts(state);
	}

	TRACE(("smp_send_broadcast_ici: done\n"));
}


bool
smp_trap_non_boot_cpus(int32 cpu)
{
	if (cpu > 0) {
		boot_cpu_spin[cpu] = 1;
		acquire_spinlock_nocheck(&boot_cpu_spin[cpu]);

		// lets make sure we're in sync with the main cpu
		// the boot processor has probably been sending us 
		// tlb sync messages all along the way, but we've 
		// been ignoring them
		arch_cpu_global_TLB_invalidate();

		return false;
	}

	return true;
}


void
smp_wake_up_non_boot_cpus()
{
	int i;

	// ICIs were previously being ignored
	if (sNumCPUs > 1)
		sICIEnabled = true;

	// resume non boot CPUs
	for (i = 1; i < sNumCPUs; i++) {
		release_spinlock(&boot_cpu_spin[i]);
	}
}


void
smp_wait_for_non_boot_cpus(void)
{
	bool retry;
	int32 i;
	do {
		retry = false;
		for (i = 1; i < sNumCPUs; i++) {
			if (boot_cpu_spin[i] != 1)
				retry = true;
		}
	} while (retry == true);
}


status_t
smp_init(kernel_args *args)
{
	struct smp_msg *msg;
	int i;

	TRACE(("smp_init: entry\n"));

	if (args->num_cpus > 1) {
		free_msgs = NULL;
		free_msg_count = 0;
		for (i = 0; i < MSG_POOL_SIZE; i++) {
			msg = (struct smp_msg *)malloc(sizeof(struct smp_msg));
			if (msg == NULL) {
				panic("error creating smp mailboxes\n");
				return B_ERROR;
			}
			memset(msg, 0, sizeof(struct smp_msg));
			msg->next = free_msgs;
			free_msgs = msg;
			free_msg_count++;
		}
		sNumCPUs = args->num_cpus;
	}
	TRACE(("smp_init: calling arch_smp_init\n"));

	return arch_smp_init(args);
}


status_t
smp_per_cpu_init(kernel_args *args, int32 cpu)
{
	return arch_smp_per_cpu_init(args, cpu);
}


void
smp_set_num_cpus(int32 numCPUs)
{
	sNumCPUs = numCPUs;
}


int32
smp_get_num_cpus()
{
	return sNumCPUs;
}


int32
smp_get_current_cpu(void)
{
	return thread_get_current_thread()->cpu->cpu_num;
}


//	#pragma mark -
//	public exported functions


void
call_all_cpus(void (*func)(void *, int), void *cookie)
{
	cpu_status state = disable_interrupts();

	if (smp_get_num_cpus() > 1) {
		smp_send_broadcast_ici(SMP_MSG_CALL_FUNCTION, (uint32)cookie,
			0, 0, (void *)func, SMP_MSG_FLAG_ASYNC);
	}

	// we need to call this function ourselves as well
	func(cookie, smp_get_current_cpu());

	restore_interrupts(state);
}

void
call_all_cpus_sync(void (*func)(void *, int), void *cookie)
{
	cpu_status state = disable_interrupts();

	if (smp_get_num_cpus() > 1) {
		smp_send_broadcast_ici(SMP_MSG_CALL_FUNCTION, (uint32)cookie,
			0, 0, (void *)func, SMP_MSG_FLAG_SYNC);
	}

	// we need to call this function ourselves as well
	func(cookie, smp_get_current_cpu());

	restore_interrupts(state);
}

