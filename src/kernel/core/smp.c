/* Functionality for symetrical multi-processors */

/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel.h>
#include <thread.h>
#include <console.h>
#include <debug.h>
#include <int.h>
#include <arch/int.h>
#include <smp_priv.h>
#include <smp.h>
#include <memheap.h>
#include <Errors.h>
#include <atomic.h>

#include <cpu.h>
#include <arch/cpu.h>
#include <arch/smp.h>
//#include <arch/pmap.h>

#include <string.h>

#define MSG_POOL_SIZE (SMP_MAX_CPUS * 4)

struct smp_msg {
	struct smp_msg *next;
	int            message;
	unsigned long   data;
	unsigned long   data2;
	unsigned long   data3;
	void           *data_ptr;
	int            flags;
	int            ref_count;
	volatile bool  done;
	unsigned int   proc_bitmap;
	int            lock;
};

#define MAILBOX_LOCAL 1
#define MAILBOX_BCAST 2

static spinlock_t boot_cpu_spin[SMP_MAX_CPUS] = { 0, };

static struct smp_msg *free_msgs = NULL;
static volatile int free_msg_count = 0;
static spinlock_t free_msg_spinlock = 0;

static struct smp_msg *smp_msgs[SMP_MAX_CPUS] = { NULL, };
static spinlock_t cpu_msg_spinlock[SMP_MAX_CPUS] = { 0, };

static struct smp_msg *smp_broadcast_msgs = NULL;
static spinlock_t broadcast_msg_spinlock = 0;

static bool ici_enabled = false;

static int smp_num_cpus = 1;

static int smp_process_pending_ici(int curr_cpu);

void acquire_spinlock(spinlock_t *lock)
{
	if(smp_num_cpus > 1) {
		int curr_cpu = smp_get_current_cpu();
		if(int_is_interrupts_enabled())
			panic("acquire_spinlock: attempt to acquire lock %p with interrupts enabled\n", lock);
		while(1) {
			while(*lock != 0)
				smp_process_pending_ici(curr_cpu);
			if(atomic_set(lock, 1) == 0)
				break;
		}
	}
}

static void acquire_spinlock_nocheck(spinlock_t *lock)
{
	if(smp_num_cpus > 1) {
		while(1) {
			while(*lock != 0)
				;
			if(atomic_set(lock, 1) == 0)
				break;
		}
	}
}

void release_spinlock(spinlock_t *lock)
{
	*lock = 0;
}

// finds a free message and gets it
// NOTE: has side effect of disabling interrupts
// return value is interrupt state
static int find_free_message(struct smp_msg **msg)
{
	int state;

//	dprintf("find_free_message: entry\n");

retry:
	while(free_msg_count <= 0)
		;
	state = int_disable_interrupts();
	acquire_spinlock(&free_msg_spinlock);

	if(free_msg_count <= 0) {
		// someone grabbed one while we were getting the lock,
		// go back to waiting for it
		release_spinlock(&free_msg_spinlock);
		int_restore_interrupts(state);
		goto retry;
	}

	*msg = free_msgs;
	free_msgs = (*msg)->next;
	free_msg_count--;

	release_spinlock(&free_msg_spinlock);

//	dprintf("find_free_message: returning msg 0x%x\n", *msg);

	return state;
}

static void return_free_message(struct smp_msg *msg)
{
//	dprintf("return_free_message: returning msg 0x%x\n", msg);
	acquire_spinlock_nocheck(&free_msg_spinlock);
	msg->next = free_msgs;
	free_msgs = msg;
	free_msg_count++;
	release_spinlock(&free_msg_spinlock);
}

static struct smp_msg *smp_check_for_message(int curr_cpu, int *source_mailbox)
{
	struct smp_msg *msg;

	acquire_spinlock_nocheck(&cpu_msg_spinlock[curr_cpu]);
	msg = smp_msgs[curr_cpu];
	if(msg != NULL) {
		smp_msgs[curr_cpu] = msg->next;
		release_spinlock(&cpu_msg_spinlock[curr_cpu]);
//		dprintf(" found msg 0x%x in cpu mailbox\n", msg);
		*source_mailbox = MAILBOX_LOCAL;
	} else {
		// try getting one from the broadcast mailbox

		release_spinlock(&cpu_msg_spinlock[curr_cpu]);
		acquire_spinlock_nocheck(&broadcast_msg_spinlock);

		msg = smp_broadcast_msgs;
		while(msg != NULL) {
			if(CHECK_BIT(msg->proc_bitmap, curr_cpu) != 0) {
				// we have handled this one already
				msg = msg->next;
				continue;
			}

			// mark it so we wont try to process this one again
			msg->proc_bitmap = SET_BIT(msg->proc_bitmap, curr_cpu);
			*source_mailbox = MAILBOX_BCAST;
			break;
		}
		release_spinlock(&broadcast_msg_spinlock);
//		dprintf(" found msg 0x%x in broadcast mailbox\n", msg);
	}
	return msg;
}

static void smp_finish_message_processing(int curr_cpu, struct smp_msg *msg, int source_mailbox)
{
	int old_refcount;

	old_refcount = atomic_add(&msg->ref_count, -1);
	if(old_refcount == 1) {
		// we were the last one to decrement the ref_count
		// it's our job to remove it from the list & possibly clean it up
		struct smp_msg **mbox = NULL;
		spinlock_t *spinlock = NULL;

		// clean up the message from one of the mailboxes
		switch(source_mailbox) {
			case MAILBOX_BCAST:
				mbox = &smp_broadcast_msgs;
				spinlock = &broadcast_msg_spinlock;
				break;
			case MAILBOX_LOCAL:
				mbox = &smp_msgs[curr_cpu];
				spinlock = &cpu_msg_spinlock[curr_cpu];
				break;
		}

		acquire_spinlock_nocheck(spinlock);

//		dprintf("cleaning up message 0x%x\n", msg);

		if(msg == *mbox) {
			(*mbox) = msg->next;
		} else {
			// we need to walk to find the message in the list.
			// we can't use any data found when previously walking through
			// the list, since the list may have changed. But, we are guaranteed
			// to at least have msg in it.
			struct smp_msg *last = NULL;
			struct smp_msg *msg1;

			msg1 = *mbox;
			while(msg1 != NULL && msg1 != msg) {
				last = msg1;
				msg1 = msg1->next;
			}

			// by definition, last must be something
			if(msg1 == msg && last != NULL) {
				last->next = msg->next;
			} else {
				dprintf("last == NULL or msg != msg1!!!\n");
			}
		}

		release_spinlock(spinlock);

		if(msg->data_ptr != NULL)
			kfree(msg->data_ptr);

		if(msg->flags == SMP_MSG_FLAG_SYNC) {
			msg->done = true;
			// the caller cpu should now free the message
		} else {
			// in the !SYNC case, we get to free the message
			return_free_message(msg);
		}
	}
}

static int smp_process_pending_ici(int curr_cpu)
{
	struct smp_msg *msg;
	bool halt = false;
	int source_mailbox = 0;
	int retval = INT_NO_RESCHEDULE;

	msg = smp_check_for_message(curr_cpu, &source_mailbox);
	if(msg == NULL)
		return retval;

//	dprintf("  message = %d\n", msg->message);
	switch(msg->message) {
		case SMP_MSG_INVL_PAGE_RANGE:
			arch_cpu_invalidate_TLB_range((addr)msg->data, (addr)msg->data2);
			break;
		case SMP_MSG_INVL_PAGE_LIST:
			arch_cpu_invalidate_TLB_list((addr *)msg->data, (int)msg->data2);
			break;
		case SMP_MSG_GLOBAL_INVL_PAGE:
			arch_cpu_global_TLB_invalidate();
			break;
		case SMP_MSG_RESCHEDULE:
			retval = INT_RESCHEDULE;
			break;
		case SMP_MSG_CPU_HALT:
			halt = true;
			dprintf("cpu %d halted!\n", curr_cpu);
			break;
		case SMP_MSG_1:
		default:
			dprintf("smp_intercpu_int_handler: got unknown message %d\n", msg->message);
	}

	// finish dealing with this message, possibly removing it from the list
	smp_finish_message_processing(curr_cpu, msg, source_mailbox);

	// special case for the halt message
	// we otherwise wouldn't have gotten the opportunity to clean up
	if(halt) {
		int_disable_interrupts();
		for(;;);
	}

	return retval;
}

int smp_intercpu_int_handler(void)
{
	int retval;
	int curr_cpu = smp_get_current_cpu();

//	dprintf("smp_intercpu_int_handler: entry on cpu %d\n", curr_cpu);

	retval = smp_process_pending_ici(curr_cpu);

//	dprintf("smp_intercpu_int_handler: done\n");

	return retval;
}

void smp_send_ici(int target_cpu, int message, unsigned long data, unsigned long data2, unsigned long data3, void *data_ptr, int flags)
{
	struct smp_msg *msg;

//	dprintf("smp_send_ici: target 0x%x, mess 0x%x, data 0x%x, data2 0x%x, data3 0x%x, ptr 0x%x, flags 0x%x\n",
//		target_cpu, message, data, data2, data3, data_ptr, flags);

	if(ici_enabled) {
		int state;
		int curr_cpu;

		// find_free_message leaves interrupts disabled
		state = find_free_message(&msg);

		curr_cpu = smp_get_current_cpu();
		if(target_cpu == curr_cpu) {
			return_free_message(msg);
			int_restore_interrupts(state);
			return; // nope, cant do that
		}

		// set up the message
		msg->message = message;
		msg->data = data;
		msg->data = data2;
		msg->data = data3;
		msg->data_ptr = data_ptr;
		msg->ref_count = 1;
		msg->flags = flags;
		msg->done = false;

		// stick it in the appropriate cpu's mailbox
		acquire_spinlock_nocheck(&cpu_msg_spinlock[target_cpu]);
		msg->next = smp_msgs[target_cpu];
		smp_msgs[target_cpu] = msg;
		release_spinlock(&cpu_msg_spinlock[target_cpu]);

		arch_smp_send_ici(target_cpu);

		if(flags == SMP_MSG_FLAG_SYNC) {
			// wait for the other cpu to finish processing it
			// the interrupt handler will ref count it to <0
			// if the message is sync after it has removed it from the mailbox
			while(msg->done == false)
				smp_process_pending_ici(curr_cpu);
			// for SYNC messages, it's our responsibility to put it
			// back into the free list
			return_free_message(msg);
		}

		int_restore_interrupts(state);
	}
}

void smp_send_broadcast_ici(int message, unsigned long data, unsigned long data2, unsigned long data3, void *data_ptr, int flags)
{
	struct smp_msg *msg;

//	dprintf("smp_send_broadcast_ici: cpu %d mess 0x%x, data 0x%x, data2 0x%x, data3 0x%x, ptr 0x%x, flags 0x%x\n",
//		smp_get_current_cpu(), message, data, data2, data3, data_ptr, flags);

	if(ici_enabled) {
		int state;
		int curr_cpu;

		// find_free_message leaves interrupts disabled
		state = find_free_message(&msg);

		curr_cpu = smp_get_current_cpu();

		msg->message = message;
		msg->data = data;
		msg->data2 = data2;
		msg->data3 = data3;
		msg->data_ptr = data_ptr;
		msg->ref_count = smp_num_cpus - 1;
		msg->flags = flags;
		msg->proc_bitmap = SET_BIT(0, curr_cpu);
		msg->done = false;

//		dprintf("smp_send_broadcast_ici%d: inserting msg 0x%x into broadcast mbox\n", smp_get_current_cpu(), msg);

		// stick it in the appropriate cpu's mailbox
		acquire_spinlock_nocheck(&broadcast_msg_spinlock);
		msg->next = smp_broadcast_msgs;
		smp_broadcast_msgs = msg;
		release_spinlock(&broadcast_msg_spinlock);

		arch_smp_send_broadcast_ici();

//		dprintf("smp_send_broadcast_ici: sent interrupt\n");

		if(flags == SMP_MSG_FLAG_SYNC) {
			// wait for the other cpus to finish processing it
			// the interrupt handler will ref count it to <0
			// if the message is sync after it has removed it from the mailbox
//			dprintf("smp_send_broadcast_ici: waiting for ack\n");
			while(msg->done == false)
				smp_process_pending_ici(curr_cpu);
//			dprintf("smp_send_broadcast_ici: returning message to free list\n");
			// for SYNC messages, it's our responsibility to put it
			// back into the free list
			return_free_message(msg);
		}

		int_restore_interrupts(state);
	}
//	dprintf("smp_send_broadcast_ici: done\n");
}

int smp_trap_non_boot_cpus(kernel_args *ka, int cpu)
{
	if(cpu > 0) {
		boot_cpu_spin[cpu] = 1;
		acquire_spinlock(&boot_cpu_spin[cpu]);
		return 1;
	} else {
		return 0;
	}
}

void smp_wake_up_all_non_boot_cpus()
{
	int i;
	for(i=1; i < smp_num_cpus; i++) {
		release_spinlock(&boot_cpu_spin[i]);
	}
}

void smp_wait_for_ap_cpus(kernel_args *ka)
{
	unsigned int i;
	int retry;
	do {
		retry = 0;
		for(i=1; i < ka->num_cpus; i++) {
			if(boot_cpu_spin[i] != 1)
				retry = 1;
		}
	} while(retry == 1);
}

int smp_init(kernel_args *ka)
{
	struct smp_msg *msg;
	int i;

	dprintf("smp_init: entry\n");

	if(ka->num_cpus > 1) {
		free_msgs = NULL;
		free_msg_count = 0;
		for(i=0; i<MSG_POOL_SIZE; i++) {
			msg = (struct smp_msg *)kmalloc(sizeof(struct smp_msg));
			if(msg == NULL) {
				panic("error creating smp mailboxes\n");
				return ERR_GENERAL;
			}
			memset(msg, 0, sizeof(struct smp_msg));
			msg->next = free_msgs;
			free_msgs = msg;
			free_msg_count++;
		}
		smp_num_cpus = ka->num_cpus;
	}
	dprintf("smp_init: calling arch_smp_init\n");
	return arch_smp_init(ka);
}

void smp_set_num_cpus(int num_cpus)
{
	smp_num_cpus = num_cpus;
}

int smp_get_num_cpus()
{
	return smp_num_cpus;
}

int smp_get_current_cpu(void)
{
	struct thread *t = thread_get_current_thread();
	if(t)
		return t->cpu->info.cpu_num;
	else
		return 0;
}

int smp_enable_ici()
{
	if(smp_num_cpus > 1) // dont actually do it if we only have one cpu
		ici_enabled = true;
	return B_NO_ERROR;
}

int smp_disable_ici()
{
	ici_enabled = false;
	return B_NO_ERROR;
}
