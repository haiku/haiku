/*
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

/* Functionality for symetrical multi-processors */

#include <smp.h>

#include <stdlib.h>
#include <string.h>

#include <arch/cpu.h>
#include <arch/debug.h>
#include <arch/int.h>
#include <arch/smp.h>
#include <cpu.h>
#include <generic_syscall.h>
#include <int.h>
#include <spinlock_contention.h>
#include <thread.h>


#define DEBUG_SPINLOCKS 1
//#define TRACE_SMP

#ifdef TRACE_SMP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#if __INTEL__
#	define PAUSE() asm volatile ("pause;")
#else
#	define PAUSE()
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

static spinlock boot_cpu_spin[SMP_MAX_CPUS] = { };

static struct smp_msg *sFreeMessages = NULL;
static volatile int sFreeMessageCount = 0;
static spinlock sFreeMessageSpinlock = B_SPINLOCK_INITIALIZER;

static struct smp_msg *sCPUMessages[SMP_MAX_CPUS] = { NULL, };
static spinlock sCPUMessageSpinlock[SMP_MAX_CPUS];

static struct smp_msg *sBroadcastMessages = NULL;
static spinlock sBroadcastMessageSpinlock = B_SPINLOCK_INITIALIZER;

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
#if B_DEBUG_SPINLOCK_CONTENTION
		while (atomic_add(&lock->lock, 1) != 0)
			process_pending_ici(currentCPU);
#else
		while (1) {
			while (*lock != 0) {
				process_pending_ici(currentCPU);
				PAUSE();
			}
			if (atomic_set((int32 *)lock, 1) == 0)
				break;
		}
#endif
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
#if B_DEBUG_SPINLOCK_CONTENTION
		while (atomic_add(&lock->lock, 1) != 0) {
		}
#else
		while (1) {
			while(*lock != 0)
				PAUSE();
			if (atomic_set((int32 *)lock, 1) == 0)
				break;
		}
#endif
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
#if B_DEBUG_SPINLOCK_CONTENTION
		{
			int32 count = atomic_set(&lock->lock, 0) - 1;
			if (count < 0) {
				panic("release_spinlock: lock %p was already released\n", lock);
			} else {
				// add to the total count -- deal with carry manually
				if ((uint32)atomic_add(&lock->count_low, count) + count
						< (uint32)count) {
					atomic_add(&lock->count_high, 1);
				}
			}
		}
#else
		if (atomic_set((int32 *)lock, 0) != 1)
			panic("release_spinlock: lock %p was already released\n", lock);
#endif
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
	while (sFreeMessageCount <= 0)
		PAUSE();
	state = disable_interrupts();
	acquire_spinlock(&sFreeMessageSpinlock);

	if (sFreeMessageCount <= 0) {
		// someone grabbed one while we were getting the lock,
		// go back to waiting for it
		release_spinlock(&sFreeMessageSpinlock);
		restore_interrupts(state);
		goto retry;
	}

	*msg = sFreeMessages;
	sFreeMessages = (*msg)->next;
	sFreeMessageCount--;

	release_spinlock(&sFreeMessageSpinlock);

	TRACE(("find_free_message: returning msg %p\n", *msg));

	return state;
}


static void
return_free_message(struct smp_msg *msg)
{
	TRACE(("return_free_message: returning msg %p\n", msg));

	acquire_spinlock_nocheck(&sFreeMessageSpinlock);
	msg->next = sFreeMessages;
	sFreeMessages = msg;
	sFreeMessageCount++;
	release_spinlock(&sFreeMessageSpinlock);
}


static struct smp_msg *
check_for_message(int currentCPU, int *source_mailbox)
{
	struct smp_msg *msg;

	if (!sICIEnabled)
		return NULL;

	acquire_spinlock_nocheck(&sCPUMessageSpinlock[currentCPU]);
	msg = sCPUMessages[currentCPU];
	if (msg != NULL) {
		sCPUMessages[currentCPU] = msg->next;
		release_spinlock(&sCPUMessageSpinlock[currentCPU]);
		TRACE((" cpu %d: found msg %p in cpu mailbox\n", currentCPU, msg));
		*source_mailbox = MAILBOX_LOCAL;
	} else {
		// try getting one from the broadcast mailbox

		release_spinlock(&sCPUMessageSpinlock[currentCPU]);
		acquire_spinlock_nocheck(&sBroadcastMessageSpinlock);

		msg = sBroadcastMessages;
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
		release_spinlock(&sBroadcastMessageSpinlock);
		TRACE((" cpu %d: found msg %p in broadcast mailbox\n", currentCPU, msg));
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
				mbox = &sBroadcastMessages;
				spinlock = &sBroadcastMessageSpinlock;
				break;
			case MAILBOX_LOCAL:
				mbox = &sCPUMessages[currentCPU];
				spinlock = &sCPUMessageSpinlock[currentCPU];
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
	vint32 *haltValue = NULL;
	int sourceMailbox = 0;
	int retval = B_HANDLED_INTERRUPT;

	msg = check_for_message(currentCPU, &sourceMailbox);
	if (msg == NULL)
		return retval;

	TRACE(("  cpu %ld message = %ld\n", currentCPU, msg->message));

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
			haltValue = (vint32 *)msg->data_ptr;
			dprintf("CPU %ld halted!\n", currentCPU);
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
	finish_message_processing(currentCPU, msg, sourceMailbox);

	// special case for the halt message
	if (haltValue) {
		cpu_status state = disable_interrupts();

		while (*haltValue != 0)
			PAUSE();

		restore_interrupts(state);
	}

	return retval;
}


#if B_DEBUG_SPINLOCK_CONTENTION

static uint64
get_spinlock_counter(spinlock* lock)
{
	uint32 high;
	uint32 low;
	do {
		high = (uint32)atomic_get(&lock->count_high);
		low = (uint32)atomic_get(&lock->count_low);
	} while (high != atomic_get(&lock->count_high));

	return ((uint64)high << 32) | low;
}


static status_t
spinlock_contention_syscall(const char* subsystem, uint32 function,
	void* buffer, size_t bufferSize)
{
	spinlock_contention_info info;

	if (function != GET_SPINLOCK_CONTENTION_INFO)
		return B_BAD_VALUE;

	if (bufferSize < sizeof(spinlock_contention_info))
		return B_BAD_VALUE;

	info.thread_spinlock_counter = get_spinlock_counter(&gThreadSpinlock);
	info.team_spinlock_counter = get_spinlock_counter(&gTeamSpinlock);

	if (!IS_USER_ADDRESS(buffer)
		|| user_memcpy(buffer, &info, sizeof(info)) != B_OK) {
		return B_BAD_ADDRESS;
	}

	return B_OK;
}

#endif	// B_DEBUG_SPINLOCK_CONTENTION


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

	TRACE(("smp_send_ici: target 0x%lx, mess 0x%lx, data 0x%lx, data2 0x%lx, data3 0x%lx, ptr %p, flags 0x%lx\n",
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
		acquire_spinlock_nocheck(&sCPUMessageSpinlock[targetCPU]);
		msg->next = sCPUMessages[targetCPU];
		sCPUMessages[targetCPU] = msg;
		release_spinlock(&sCPUMessageSpinlock[targetCPU]);

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
smp_send_multicast_ici(cpu_mask_t cpuMask, int32 message, uint32 data,
	uint32 data2, uint32 data3, void *data_ptr, uint32 flags)
{
	if (!sICIEnabled)
		return;

	int currentCPU = smp_get_current_cpu();
	cpuMask &= ~((cpu_mask_t)1 << currentCPU);
	if (cpuMask == 0)
		return;

	// count target CPUs
	int32 targetCPUs = 0;
	for (int32 i = 0; i < sNumCPUs; i++) {
		if ((cpuMask & (cpu_mask_t)1 << i) != 0)
			targetCPUs++;
	}

	// find_free_message leaves interrupts disabled
	struct smp_msg *msg;
	int state = find_free_message(&msg);

	msg->message = message;
	msg->data = data;
	msg->data2 = data2;
	msg->data3 = data3;
	msg->data_ptr = data_ptr;
	msg->ref_count = targetCPUs;
	msg->flags = flags;
	msg->proc_bitmap = ~cpuMask;
	msg->done = false;

	// stick it in the broadcast mailbox
	acquire_spinlock_nocheck(&sBroadcastMessageSpinlock);
	msg->next = sBroadcastMessages;
	sBroadcastMessages = msg;
	release_spinlock(&sBroadcastMessageSpinlock);

	arch_smp_send_broadcast_ici();
		// TODO: Introduce a call that only bothers the target CPUs!

	if (flags & SMP_MSG_FLAG_SYNC) {
		// wait for the other cpus to finish processing it
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


void
smp_send_broadcast_ici(int32 message, uint32 data, uint32 data2, uint32 data3,
	void *data_ptr, uint32 flags)
{
	struct smp_msg *msg;

	TRACE(("smp_send_broadcast_ici: cpu %ld mess 0x%lx, data 0x%lx, data2 0x%lx, data3 0x%lx, ptr %p, flags 0x%lx\n",
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
			currentCPU, msg));

		// stick it in the appropriate cpu's mailbox
		acquire_spinlock_nocheck(&sBroadcastMessageSpinlock);
		msg->next = sBroadcastMessages;
		sBroadcastMessages = msg;
		release_spinlock(&sBroadcastMessageSpinlock);

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
#if B_DEBUG_SPINLOCK_CONTENTION
		boot_cpu_spin[cpu].lock = 1;
#else
		boot_cpu_spin[cpu] = 1;
#endif
		acquire_spinlock_nocheck(&boot_cpu_spin[cpu]);
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

/* have all cpus spin until all have run */
void
smp_cpu_rendezvous(volatile uint32 *var, int current_cpu)
{
	atomic_or((vint32*)var, 1 << current_cpu);

	while (*var != (((uint32)1 << sNumCPUs) - 1))
		PAUSE();
}

status_t
smp_init(kernel_args *args)
{
	struct smp_msg *msg;
	int i;

	TRACE(("smp_init: entry\n"));

	if (args->num_cpus > 1) {
		sFreeMessages = NULL;
		sFreeMessageCount = 0;
		for (i = 0; i < MSG_POOL_SIZE; i++) {
			msg = (struct smp_msg *)malloc(sizeof(struct smp_msg));
			if (msg == NULL) {
				panic("error creating smp mailboxes\n");
				return B_ERROR;
			}
			memset(msg, 0, sizeof(struct smp_msg));
			msg->next = sFreeMessages;
			sFreeMessages = msg;
			sFreeMessageCount++;
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


status_t
smp_init_post_generic_syscalls(void)
{
#if B_DEBUG_SPINLOCK_CONTENTION
	return register_generic_syscall(SPINLOCK_CONTENTION,
		&spinlock_contention_syscall, 0, 0);
#else
	return B_OK;
#endif
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


void
memory_read_barrier(void)
{
	arch_cpu_memory_read_barrier();
}


void
memory_write_barrier(void)
{
	arch_cpu_memory_write_barrier();
}

