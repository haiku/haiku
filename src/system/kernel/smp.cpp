/*
 * Copyright 2008-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


/*! Functionality for symetrical multi-processors */


#include <smp.h>

#include <stdlib.h>
#include <string.h>

#include <arch/cpu.h>
#include <arch/debug.h>
#include <arch/int.h>
#include <arch/smp.h>
#include <boot/kernel_args.h>
#include <cpu.h>
#include <generic_syscall.h>
#include <int.h>
#include <spinlock_contention.h>
#include <thread.h>
#if DEBUG_SPINLOCK_LATENCIES
#	include <safemode.h>
#endif

#include "kernel_debug_config.h"


//#define TRACE_SMP
#ifdef TRACE_SMP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#undef try_acquire_spinlock
#undef acquire_spinlock
#undef release_spinlock


#define MSG_POOL_SIZE (SMP_MAX_CPUS * 4)

// These macros define the number of unsuccessful iterations in
// acquire_spinlock() and acquire_spinlock_nocheck() after which the functions
// panic(), assuming a deadlock.
#define SPINLOCK_DEADLOCK_COUNT				100000000
#define SPINLOCK_DEADLOCK_COUNT_NO_CHECK	2000000000


struct smp_msg {
	struct smp_msg	*next;
	int32			message;
	addr_t			data;
	addr_t			data2;
	addr_t			data3;
	void			*data_ptr;
	uint32			flags;
	int32			ref_count;
	volatile bool	done;
	uint32			proc_bitmap;
};

enum mailbox_source {
	MAILBOX_LOCAL,
	MAILBOX_BCAST,
};

static vint32 sBootCPUSpin = 0;

static vint32 sEarlyCPUCall = 0;
static void (*sEarlyCPUCallFunction)(void*, int);
void* sEarlyCPUCallCookie;

static struct smp_msg* sFreeMessages = NULL;
static volatile int sFreeMessageCount = 0;
static spinlock sFreeMessageSpinlock = B_SPINLOCK_INITIALIZER;

static struct smp_msg* sCPUMessages[SMP_MAX_CPUS] = { NULL, };
static spinlock sCPUMessageSpinlock[SMP_MAX_CPUS];

static struct smp_msg* sBroadcastMessages = NULL;
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

static vint32 sLastIndex = 0;
	// Is incremented atomically. Must be % NUM_LAST_CALLERS before being used
	// as index into sLastCaller. Note, that it has to be casted to uint32
	// before applying the modulo operation, since otherwise after overflowing
	// that would yield negative indices.


static void
push_lock_caller(void* caller, spinlock* lock)
{
	int32 index = (uint32)atomic_add(&sLastIndex, 1) % NUM_LAST_CALLERS;

	sLastCaller[index].caller = caller;
	sLastCaller[index].lock = lock;
}


static void*
find_lock_caller(spinlock* lock)
{
	int32 lastIndex = (uint32)sLastIndex % NUM_LAST_CALLERS;

	for (int32 i = 0; i < NUM_LAST_CALLERS; i++) {
		int32 index = (NUM_LAST_CALLERS + lastIndex - 1 - i) % NUM_LAST_CALLERS;
		if (sLastCaller[index].lock == lock)
			return sLastCaller[index].caller;
	}

	return NULL;
}


int
dump_spinlock(int argc, char** argv)
{
	if (argc != 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	uint64 address;
	if (!evaluate_debug_expression(argv[1], &address, false))
		return 0;

	spinlock* lock = (spinlock*)(addr_t)address;
	kprintf("spinlock %p:\n", lock);
	bool locked = B_SPINLOCK_IS_LOCKED(lock);
	if (locked) {
		kprintf("  locked from %p\n", find_lock_caller(lock));
	} else
		kprintf("  not locked\n");

	return 0;
}


#endif	// DEBUG_SPINLOCKS


#if DEBUG_SPINLOCK_LATENCIES


#define NUM_LATENCY_LOCKS	4
#define DEBUG_LATENCY		200


static struct {
	spinlock	*lock;
	bigtime_t	timestamp;
} sLatency[B_MAX_CPU_COUNT][NUM_LATENCY_LOCKS];

static int32 sLatencyIndex[B_MAX_CPU_COUNT];
static bool sEnableLatencyCheck;


static void
push_latency(spinlock* lock)
{
	if (!sEnableLatencyCheck)
		return;

	int32 cpu = smp_get_current_cpu();
	int32 index = (++sLatencyIndex[cpu]) % NUM_LATENCY_LOCKS;

	sLatency[cpu][index].lock = lock;
	sLatency[cpu][index].timestamp = system_time();
}


static void
test_latency(spinlock* lock)
{
	if (!sEnableLatencyCheck)
		return;

	int32 cpu = smp_get_current_cpu();

	for (int32 i = 0; i < NUM_LATENCY_LOCKS; i++) {
		if (sLatency[cpu][i].lock == lock) {
			bigtime_t diff = system_time() - sLatency[cpu][i].timestamp;
			if (diff > DEBUG_LATENCY && diff < 500000) {
				panic("spinlock %p were held for %lld usecs (%d allowed)\n",
					lock, diff, DEBUG_LATENCY);
			}

			sLatency[cpu][i].lock = NULL;
		}
	}
}


#endif	// DEBUG_SPINLOCK_LATENCIES


int
dump_ici_messages(int argc, char** argv)
{
	// count broadcast messages
	int32 count = 0;
	int32 doneCount = 0;
	int32 unreferencedCount = 0;
	smp_msg* message = sBroadcastMessages;
	while (message != NULL) {
		count++;
		if (message->done)
			doneCount++;
		if (message->ref_count <= 0)
			unreferencedCount++;
		message = message->next;
	}

	kprintf("ICI broadcast messages: %" B_PRId32 ", first: %p\n", count,
		sBroadcastMessages);
	kprintf("  done:         %" B_PRId32 "\n", doneCount);
	kprintf("  unreferenced: %" B_PRId32 "\n", unreferencedCount);

	// count per-CPU messages
	for (int32 i = 0; i < sNumCPUs; i++) {
		count = 0;
		message = sCPUMessages[i];
		while (message != NULL) {
			count++;
			message = message->next;
		}

		kprintf("CPU %" B_PRId32 " messages: %" B_PRId32 ", first: %p\n", i,
			count, sCPUMessages[i]);
	}

	return 0;
}


int
dump_ici_message(int argc, char** argv)
{
	if (argc != 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	uint64 address;
	if (!evaluate_debug_expression(argv[1], &address, false))
		return 0;

	smp_msg* message = (smp_msg*)(addr_t)address;
	kprintf("ICI message %p:\n", message);
	kprintf("  next:        %p\n", message->next);
	kprintf("  message:     %" B_PRId32 "\n", message->message);
	kprintf("  data:        0x%lx\n", message->data);
	kprintf("  data2:       0x%lx\n", message->data2);
	kprintf("  data3:       0x%lx\n", message->data3);
	kprintf("  data_ptr:    %p\n", message->data_ptr);
	kprintf("  flags:       %" B_PRIx32 "\n", message->flags);
	kprintf("  ref_count:   %" B_PRIx32 "\n", message->ref_count);
	kprintf("  done:        %s\n", message->done ? "true" : "false");
	kprintf("  proc_bitmap: %" B_PRIx32 "\n", message->proc_bitmap);

	return 0;
}


static inline void
process_all_pending_ici(int32 currentCPU)
{
	while (process_pending_ici(currentCPU) != B_ENTRY_NOT_FOUND)
		;
}


bool
try_acquire_spinlock(spinlock* lock)
{
#if DEBUG_SPINLOCKS
	if (are_interrupts_enabled()) {
		panic("try_acquire_spinlock: attempt to acquire lock %p with "
			"interrupts enabled", lock);
	}
#endif

#if B_DEBUG_SPINLOCK_CONTENTION
	if (atomic_add(&lock->lock, 1) != 0)
		return false;
#else
	if (atomic_or((int32*)lock, 1) != 0)
		return false;

#	if DEBUG_SPINLOCKS
	push_lock_caller(arch_debug_get_caller(), lock);
#	endif
#endif

	return true;
}


void
acquire_spinlock(spinlock* lock)
{
#if DEBUG_SPINLOCKS
	if (are_interrupts_enabled()) {
		panic("acquire_spinlock: attempt to acquire lock %p with interrupts "
			"enabled", lock);
	}
#endif

	if (sNumCPUs > 1) {
		int currentCPU = smp_get_current_cpu();
#if B_DEBUG_SPINLOCK_CONTENTION
		while (atomic_add(&lock->lock, 1) != 0)
			process_all_pending_ici(currentCPU);
#else
		while (1) {
			uint32 count = 0;
			while (*lock != 0) {
				if (++count == SPINLOCK_DEADLOCK_COUNT) {
					panic("acquire_spinlock(): Failed to acquire spinlock %p "
						"for a long time!", lock);
					count = 0;
				}

				process_all_pending_ici(currentCPU);
				PAUSE();
			}
			if (atomic_or((int32*)lock, 1) == 0)
				break;
		}

#	if DEBUG_SPINLOCKS
		push_lock_caller(arch_debug_get_caller(), lock);
#	endif
#endif
	} else {
#if DEBUG_SPINLOCKS
		int32 oldValue;
		oldValue = atomic_or((int32*)lock, 1);
		if (oldValue != 0) {
			panic("acquire_spinlock: attempt to acquire lock %p twice on "
				"non-SMP system (last caller: %p, value %" B_PRId32 ")", lock,
				find_lock_caller(lock), oldValue);
		}

		push_lock_caller(arch_debug_get_caller(), lock);
#endif
	}
#if DEBUG_SPINLOCK_LATENCIES
	push_latency(lock);
#endif
}


static void
acquire_spinlock_nocheck(spinlock *lock)
{
#if DEBUG_SPINLOCKS
	if (are_interrupts_enabled()) {
		panic("acquire_spinlock_nocheck: attempt to acquire lock %p with "
			"interrupts enabled", lock);
	}
#endif

	if (sNumCPUs > 1) {
#if B_DEBUG_SPINLOCK_CONTENTION
		while (atomic_add(&lock->lock, 1) != 0) {
		}
#else
		while (1) {
			uint32 count = 0;
			while (*lock != 0) {
				if (++count == SPINLOCK_DEADLOCK_COUNT_NO_CHECK) {
					panic("acquire_spinlock(): Failed to acquire spinlock %p "
						"for a long time!", lock);
					count = 0;
				}

				PAUSE();
			}

			if (atomic_or((int32*)lock, 1) == 0)
				break;
		}
#endif
	} else {
#if DEBUG_SPINLOCKS
		if (atomic_or((int32*)lock, 1) != 0) {
			panic("acquire_spinlock_nocheck: attempt to acquire lock %p twice "
				"on non-SMP system\n", lock);
		}
#endif
	}
}


/*!	Equivalent to acquire_spinlock(), save for currentCPU parameter. */
static void
acquire_spinlock_cpu(int32 currentCPU, spinlock *lock)
{
#if DEBUG_SPINLOCKS
	if (are_interrupts_enabled()) {
		panic("acquire_spinlock_cpu: attempt to acquire lock %p with "
			"interrupts enabled", lock);
	}
#endif

	if (sNumCPUs > 1) {
#if B_DEBUG_SPINLOCK_CONTENTION
		while (atomic_add(&lock->lock, 1) != 0)
			process_all_pending_ici(currentCPU);
#else
		while (1) {
			uint32 count = 0;
			while (*lock != 0) {
				if (++count == SPINLOCK_DEADLOCK_COUNT) {
					panic("acquire_spinlock_cpu(): Failed to acquire spinlock "
						"%p for a long time!", lock);
					count = 0;
				}

				process_all_pending_ici(currentCPU);
				PAUSE();
			}
			if (atomic_or((int32*)lock, 1) == 0)
				break;
		}

#	if DEBUG_SPINLOCKS
		push_lock_caller(arch_debug_get_caller(), lock);
#	endif
#endif
	} else {
#if DEBUG_SPINLOCKS
		int32 oldValue;
		oldValue = atomic_or((int32*)lock, 1);
		if (oldValue != 0) {
			panic("acquire_spinlock_cpu(): attempt to acquire lock %p twice on "
				"non-SMP system (last caller: %p, value %" B_PRId32 ")", lock,
				find_lock_caller(lock), oldValue);
		}

		push_lock_caller(arch_debug_get_caller(), lock);
#endif
	}
}


void
release_spinlock(spinlock *lock)
{
#if DEBUG_SPINLOCK_LATENCIES
	test_latency(lock);
#endif

	if (sNumCPUs > 1) {
		if (are_interrupts_enabled())
			panic("release_spinlock: attempt to release lock %p with "
				"interrupts enabled\n", lock);
#if B_DEBUG_SPINLOCK_CONTENTION
		{
			int32 count = atomic_and(&lock->lock, 0) - 1;
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
		if (atomic_and((int32*)lock, 0) != 1)
			panic("release_spinlock: lock %p was already released\n", lock);
#endif
	} else {
#if DEBUG_SPINLOCKS
		if (are_interrupts_enabled()) {
			panic("release_spinlock: attempt to release lock %p with "
				"interrupts enabled\n", lock);
		}
		if (atomic_and((int32*)lock, 0) != 1)
			panic("release_spinlock: lock %p was already released\n", lock);
#endif
#if DEBUG_SPINLOCK_LATENCIES
		test_latency(lock);
#endif
	}
}


/*!	Finds a free message and gets it.
	NOTE: has side effect of disabling interrupts
	return value is the former interrupt state
*/
static cpu_status
find_free_message(struct smp_msg** msg)
{
	cpu_status state;

	TRACE(("find_free_message: entry\n"));

retry:
	while (sFreeMessageCount <= 0) {
		state = disable_interrupts();
		process_all_pending_ici(smp_get_current_cpu());
		restore_interrupts(state);
		PAUSE();
	}
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


/*!	Similar to find_free_message(), but expects the interrupts to be disabled
	already.
*/
static void
find_free_message_interrupts_disabled(int32 currentCPU,
	struct smp_msg** _message)
{
	TRACE(("find_free_message_interrupts_disabled: entry\n"));

	acquire_spinlock_cpu(currentCPU, &sFreeMessageSpinlock);
	while (sFreeMessageCount <= 0) {
		release_spinlock(&sFreeMessageSpinlock);
		process_all_pending_ici(currentCPU);
		PAUSE();
		acquire_spinlock_cpu(currentCPU, &sFreeMessageSpinlock);
	}

	*_message = sFreeMessages;
	sFreeMessages = (*_message)->next;
	sFreeMessageCount--;

	release_spinlock(&sFreeMessageSpinlock);

	TRACE(("find_free_message_interrupts_disabled: returning msg %p\n",
		*_message));
}


static void
return_free_message(struct smp_msg* msg)
{
	TRACE(("return_free_message: returning msg %p\n", msg));

	acquire_spinlock_nocheck(&sFreeMessageSpinlock);
	msg->next = sFreeMessages;
	sFreeMessages = msg;
	sFreeMessageCount++;
	release_spinlock(&sFreeMessageSpinlock);
}


static struct smp_msg*
check_for_message(int currentCPU, mailbox_source& sourceMailbox)
{
	if (!sICIEnabled)
		return NULL;

	acquire_spinlock_nocheck(&sCPUMessageSpinlock[currentCPU]);

	struct smp_msg* msg = sCPUMessages[currentCPU];
	if (msg != NULL) {
		sCPUMessages[currentCPU] = msg->next;
		release_spinlock(&sCPUMessageSpinlock[currentCPU]);
		TRACE((" cpu %d: found msg %p in cpu mailbox\n", currentCPU, msg));
		sourceMailbox = MAILBOX_LOCAL;
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
			sourceMailbox = MAILBOX_BCAST;
			break;
		}
		release_spinlock(&sBroadcastMessageSpinlock);

		TRACE((" cpu %d: found msg %p in broadcast mailbox\n", currentCPU,
			msg));
	}
	return msg;
}


static void
finish_message_processing(int currentCPU, struct smp_msg* msg,
	mailbox_source sourceMailbox)
{
	if (atomic_add(&msg->ref_count, -1) != 1)
		return;

	// we were the last one to decrement the ref_count
	// it's our job to remove it from the list & possibly clean it up
	struct smp_msg** mbox;
	spinlock* spinlock;

	// clean up the message from one of the mailboxes
	if (sourceMailbox == MAILBOX_BCAST) {
		mbox = &sBroadcastMessages;
		spinlock = &sBroadcastMessageSpinlock;
	} else {
		mbox = &sCPUMessages[currentCPU];
		spinlock = &sCPUMessageSpinlock[currentCPU];
	}

	acquire_spinlock_nocheck(spinlock);

	TRACE(("cleaning up message %p\n", msg));

	if (sourceMailbox != MAILBOX_BCAST) {
		// local mailbox -- the message has already been removed in
		// check_for_message()
	} else if (msg == *mbox) {
		*mbox = msg->next;
	} else {
		// we need to walk to find the message in the list.
		// we can't use any data found when previously walking through
		// the list, since the list may have changed. But, we are guaranteed
		// to at least have msg in it.
		struct smp_msg* last = NULL;
		struct smp_msg* msg1;

		msg1 = *mbox;
		while (msg1 != NULL && msg1 != msg) {
			last = msg1;
			msg1 = msg1->next;
		}

		// by definition, last must be something
		if (msg1 == msg && last != NULL)
			last->next = msg->next;
		else
			panic("last == NULL or msg != msg1");
	}

	release_spinlock(spinlock);

	if ((msg->flags & SMP_MSG_FLAG_FREE_ARG) != 0 && msg->data_ptr != NULL)
		free(msg->data_ptr);

	if ((msg->flags & SMP_MSG_FLAG_SYNC) != 0) {
		msg->done = true;
		// the caller cpu should now free the message
	} else {
		// in the !SYNC case, we get to free the message
		return_free_message(msg);
	}
}


static status_t
process_pending_ici(int32 currentCPU)
{
	mailbox_source sourceMailbox;
	struct smp_msg* msg = check_for_message(currentCPU, sourceMailbox);
	if (msg == NULL)
		return B_ENTRY_NOT_FOUND;

	TRACE(("  cpu %ld message = %ld\n", currentCPU, msg->message));

	bool haltCPU = false;

	switch (msg->message) {
		case SMP_MSG_INVALIDATE_PAGE_RANGE:
			arch_cpu_invalidate_TLB_range(msg->data, msg->data2);
			break;
		case SMP_MSG_INVALIDATE_PAGE_LIST:
			arch_cpu_invalidate_TLB_list((addr_t*)msg->data, (int)msg->data2);
			break;
		case SMP_MSG_USER_INVALIDATE_PAGES:
			arch_cpu_user_TLB_invalidate();
			break;
		case SMP_MSG_GLOBAL_INVALIDATE_PAGES:
			arch_cpu_global_TLB_invalidate();
			break;
		case SMP_MSG_CPU_HALT:
			haltCPU = true;
			break;
		case SMP_MSG_CALL_FUNCTION:
		{
			smp_call_func func = (smp_call_func)msg->data_ptr;
			func(msg->data, currentCPU, msg->data2, msg->data3);
			break;
		}
		case SMP_MSG_RESCHEDULE:
		{
			cpu_ent* cpu = thread_get_current_thread()->cpu;
			cpu->invoke_scheduler = true;
			cpu->invoke_scheduler_if_idle = false;
			break;
		}
		case SMP_MSG_RESCHEDULE_IF_IDLE:
		{
			cpu_ent* cpu = thread_get_current_thread()->cpu;
			if (!cpu->invoke_scheduler) {
				cpu->invoke_scheduler = true;
				cpu->invoke_scheduler_if_idle = true;
			}
			break;
		}

		default:
			dprintf("smp_intercpu_int_handler: got unknown message %" B_PRId32 "\n",
				msg->message);
			break;
	}

	// finish dealing with this message, possibly removing it from the list
	finish_message_processing(currentCPU, msg, sourceMailbox);

	// special case for the halt message
	if (haltCPU)
		debug_trap_cpu_in_kdl(currentCPU, false);

	return B_OK;
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


static void
process_early_cpu_call(int32 cpu)
{
	sEarlyCPUCallFunction(sEarlyCPUCallCookie, cpu);
	atomic_and(&sEarlyCPUCall, ~(uint32)(1 << cpu));
}


static void
call_all_cpus_early(void (*function)(void*, int), void* cookie)
{
	if (sNumCPUs > 1) {
		sEarlyCPUCallFunction = function;
		sEarlyCPUCallCookie = cookie;

		uint32 cpuMask = (1 << sNumCPUs) - 2;
			// all CPUs but the boot cpu

		sEarlyCPUCall = cpuMask;

		// wait for all CPUs to finish
		while ((sEarlyCPUCall & cpuMask) != 0)
			PAUSE();
	}

	function(cookie, 0);
}


//	#pragma mark -


int
smp_intercpu_int_handler(int32 cpu)
{
	TRACE(("smp_intercpu_int_handler: entry on cpu %ld\n", cpu));

	process_all_pending_ici(cpu);

	TRACE(("smp_intercpu_int_handler: done\n"));

	return B_HANDLED_INTERRUPT;
}


void
smp_send_ici(int32 targetCPU, int32 message, addr_t data, addr_t data2,
	addr_t data3, void* dataPointer, addr_t flags)
{
	struct smp_msg *msg;

	TRACE(("smp_send_ici: target 0x%lx, mess 0x%lx, data 0x%lx, data2 0x%lx, "
		"data3 0x%lx, ptr %p, flags 0x%lx\n", targetCPU, message, data, data2,
		data3, dataPointer, flags));

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
		msg->data_ptr = dataPointer;
		msg->ref_count = 1;
		msg->flags = flags;
		msg->done = false;

		// stick it in the appropriate cpu's mailbox
		acquire_spinlock_nocheck(&sCPUMessageSpinlock[targetCPU]);
		msg->next = sCPUMessages[targetCPU];
		sCPUMessages[targetCPU] = msg;
		release_spinlock(&sCPUMessageSpinlock[targetCPU]);

		arch_smp_send_ici(targetCPU);

		if ((flags & SMP_MSG_FLAG_SYNC) != 0) {
			// wait for the other cpu to finish processing it
			// the interrupt handler will ref count it to <0
			// if the message is sync after it has removed it from the mailbox
			while (msg->done == false) {
				process_all_pending_ici(currentCPU);
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
smp_send_multicast_ici(cpu_mask_t cpuMask, int32 message, addr_t data,
	addr_t data2, addr_t data3, void *dataPointer, uint32 flags)
{
	if (!sICIEnabled)
		return;

	int currentCPU = smp_get_current_cpu();
	cpuMask &= ~((cpu_mask_t)1 << currentCPU)
		& (((cpu_mask_t)1 << sNumCPUs) - 1);
	if (cpuMask == 0) {
		panic("smp_send_multicast_ici(): 0 CPU mask");
		return;
	}

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
	msg->data_ptr = dataPointer;
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

	if ((flags & SMP_MSG_FLAG_SYNC) != 0) {
		// wait for the other cpus to finish processing it
		// the interrupt handler will ref count it to <0
		// if the message is sync after it has removed it from the mailbox
		while (msg->done == false) {
			process_all_pending_ici(currentCPU);
			PAUSE();
		}

		// for SYNC messages, it's our responsibility to put it
		// back into the free list
		return_free_message(msg);
	}

	restore_interrupts(state);
}


void
smp_send_broadcast_ici(int32 message, addr_t data, addr_t data2, addr_t data3,
	void *dataPointer, uint32 flags)
{
	struct smp_msg *msg;

	TRACE(("smp_send_broadcast_ici: cpu %ld mess 0x%lx, data 0x%lx, data2 "
		"0x%lx, data3 0x%lx, ptr %p, flags 0x%lx\n", smp_get_current_cpu(),
		message, data, data2, data3, dataPointer, flags));

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
		msg->data_ptr = dataPointer;
		msg->ref_count = sNumCPUs - 1;
		msg->flags = flags;
		msg->proc_bitmap = SET_BIT(0, currentCPU);
		msg->done = false;

		TRACE(("smp_send_broadcast_ici%d: inserting msg %p into broadcast "
			"mbox\n", currentCPU, msg));

		// stick it in the appropriate cpu's mailbox
		acquire_spinlock_nocheck(&sBroadcastMessageSpinlock);
		msg->next = sBroadcastMessages;
		sBroadcastMessages = msg;
		release_spinlock(&sBroadcastMessageSpinlock);

		arch_smp_send_broadcast_ici();

		TRACE(("smp_send_broadcast_ici: sent interrupt\n"));

		if ((flags & SMP_MSG_FLAG_SYNC) != 0) {
			// wait for the other cpus to finish processing it
			// the interrupt handler will ref count it to <0
			// if the message is sync after it has removed it from the mailbox
			TRACE(("smp_send_broadcast_ici: waiting for ack\n"));

			while (msg->done == false) {
				process_all_pending_ici(currentCPU);
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


void
smp_send_broadcast_ici_interrupts_disabled(int32 currentCPU, int32 message,
	addr_t data, addr_t data2, addr_t data3, void *dataPointer, uint32 flags)
{
	if (!sICIEnabled)
		return;

	TRACE(("smp_send_broadcast_ici_interrupts_disabled: cpu %ld mess 0x%lx, "
		"data 0x%lx, data2 0x%lx, data3 0x%lx, ptr %p, flags 0x%lx\n",
		currentCPU, message, data, data2, data3, dataPointer, flags));

	struct smp_msg *msg;
	find_free_message_interrupts_disabled(currentCPU, &msg);

	msg->message = message;
	msg->data = data;
	msg->data2 = data2;
	msg->data3 = data3;
	msg->data_ptr = dataPointer;
	msg->ref_count = sNumCPUs - 1;
	msg->flags = flags;
	msg->proc_bitmap = SET_BIT(0, currentCPU);
	msg->done = false;

	TRACE(("smp_send_broadcast_ici_interrupts_disabled %ld: inserting msg %p "
		"into broadcast mbox\n", currentCPU, msg));

	// stick it in the appropriate cpu's mailbox
	acquire_spinlock_nocheck(&sBroadcastMessageSpinlock);
	msg->next = sBroadcastMessages;
	sBroadcastMessages = msg;
	release_spinlock(&sBroadcastMessageSpinlock);

	arch_smp_send_broadcast_ici();

	TRACE(("smp_send_broadcast_ici_interrupts_disabled %ld: sent interrupt\n",
		currentCPU));

	if ((flags & SMP_MSG_FLAG_SYNC) != 0) {
		// wait for the other cpus to finish processing it
		// the interrupt handler will ref count it to <0
		// if the message is sync after it has removed it from the mailbox
		TRACE(("smp_send_broadcast_ici_interrupts_disabled %ld: waiting for "
			"ack\n", currentCPU));

		while (msg->done == false) {
			process_all_pending_ici(currentCPU);
			PAUSE();
		}

		TRACE(("smp_send_broadcast_ici_interrupts_disabled %ld: returning "
			"message to free list\n", currentCPU));

		// for SYNC messages, it's our responsibility to put it
		// back into the free list
		return_free_message(msg);
	}

	TRACE(("smp_send_broadcast_ici_interrupts_disabled: done\n"));
}


/*!	Spin on non-boot CPUs until smp_wake_up_non_boot_cpus() has been called.

	\param cpu The index of the calling CPU.
	\param rendezVous A rendez-vous variable to make sure that the boot CPU
		does not return before all other CPUs have started waiting.
	\return \c true on the boot CPU, \c false otherwise.
*/
bool
smp_trap_non_boot_cpus(int32 cpu, uint32* rendezVous)
{
	if (cpu == 0) {
		smp_cpu_rendezvous(rendezVous, cpu);
		return true;
	}

	smp_cpu_rendezvous(rendezVous, cpu);

	while (sBootCPUSpin == 0) {
		if ((sEarlyCPUCall & (1 << cpu)) != 0)
			process_early_cpu_call(cpu);

		PAUSE();
	}

	return false;
}


void
smp_wake_up_non_boot_cpus()
{
	// ICIs were previously being ignored
	if (sNumCPUs > 1)
		sICIEnabled = true;

	// resume non boot CPUs
	sBootCPUSpin = 1;
}


/*!	Spin until all CPUs have reached the rendez-vous point.

	The rendez-vous variable \c *var must have been initialized to 0 before the
	function is called. The variable will be non-null when the function returns.

	Note that when the function returns on one CPU, it only means that all CPU
	have already entered the function. It does not mean that the variable can
	already be reset. Only when all CPUs have returned (which would have to be
	ensured via another rendez-vous) the variable can be reset.
*/
void
smp_cpu_rendezvous(volatile uint32* var, int current_cpu)
{
	atomic_or((vint32*)var, 1 << current_cpu);

	while (*var != (((uint32)1 << sNumCPUs) - 1))
		PAUSE();
}


status_t
smp_init(kernel_args* args)
{
	TRACE(("smp_init: entry\n"));

#if DEBUG_SPINLOCK_LATENCIES
	sEnableLatencyCheck
		= !get_safemode_boolean(B_SAFEMODE_DISABLE_LATENCY_CHECK, false);
#endif

#if DEBUG_SPINLOCKS
	add_debugger_command_etc("spinlock", &dump_spinlock,
		"Dump info on a spinlock",
		"\n"
		"Dumps info on a spinlock.\n", 0);
#endif
	add_debugger_command_etc("ici", &dump_ici_messages,
		"Dump info on pending ICI messages",
		"\n"
		"Dumps info on pending ICI messages.\n", 0);
	add_debugger_command_etc("ici_message", &dump_ici_message,
		"Dump info on an ICI message",
		"\n"
		"Dumps info on an ICI message.\n", 0);

	if (args->num_cpus > 1) {
		sFreeMessages = NULL;
		sFreeMessageCount = 0;
		for (int i = 0; i < MSG_POOL_SIZE; i++) {
			struct smp_msg* msg
				= (struct smp_msg*)malloc(sizeof(struct smp_msg));
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
smp_per_cpu_init(kernel_args* args, int32 cpu)
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


// #pragma mark - public exported functions


void
call_all_cpus(void (*func)(void*, int), void* cookie)
{
	cpu_status state = disable_interrupts();
	
	// if inter-CPU communication is not yet enabled, use the early mechanism
	if (!sICIEnabled) {
		call_all_cpus_early(func, cookie);
		restore_interrupts(state);
		return;
	}

	if (smp_get_num_cpus() > 1) {
		smp_send_broadcast_ici(SMP_MSG_CALL_FUNCTION, (addr_t)cookie,
			0, 0, (void*)func, SMP_MSG_FLAG_ASYNC);
	}

	// we need to call this function ourselves as well
	func(cookie, smp_get_current_cpu());

	restore_interrupts(state);
}


void
call_all_cpus_sync(void (*func)(void*, int), void* cookie)
{
	cpu_status state = disable_interrupts();
	
	// if inter-CPU communication is not yet enabled, use the early mechanism
	if (!sICIEnabled) {
		call_all_cpus_early(func, cookie);
		restore_interrupts(state);
		return;
	}

	if (smp_get_num_cpus() > 1) {
		smp_send_broadcast_ici(SMP_MSG_CALL_FUNCTION, (addr_t)cookie,
			0, 0, (void*)func, SMP_MSG_FLAG_SYNC);
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
