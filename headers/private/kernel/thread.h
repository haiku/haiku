/*
 * Copyright 2008-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _THREAD_H
#define _THREAD_H


#include <OS.h>
#include <thread_types.h>
#include <arch/thread.h>

// For the thread blocking inline functions only.
#include <kscheduler.h>
#include <ksignal.h>


struct kernel_args;
struct select_info;
struct thread_creation_attributes;


// thread notifications
#define THREAD_MONITOR		'_tm_'
#define THREAD_ADDED		0x01
#define THREAD_REMOVED		0x02
#define THREAD_NAME_CHANGED	0x04


#ifdef __cplusplus
extern "C" {
#endif

void thread_enqueue(Thread *t, struct thread_queue *q);
Thread *thread_lookat_queue(struct thread_queue *q);
Thread *thread_dequeue(struct thread_queue *q);
Thread *thread_dequeue_id(struct thread_queue *q, thread_id id);

void thread_at_kernel_entry(bigtime_t now);
	// called when the thread enters the kernel on behalf of the thread
void thread_at_kernel_exit(void);
void thread_at_kernel_exit_no_signals(void);
void thread_reset_for_exec(void);

status_t thread_init(struct kernel_args *args);
status_t thread_preboot_init_percpu(struct kernel_args *args, int32 cpuNum);
void thread_yield(bool force);
void thread_exit(void);

int32 thread_max_threads(void);
int32 thread_used_threads(void);

const char* thread_state_to_text(Thread* thread, int32 state);

int32 thread_get_io_priority(thread_id id);
void thread_set_io_priority(int32 priority);

#define thread_get_current_thread arch_thread_get_current_thread

Thread *thread_get_thread_struct(thread_id id);
Thread *thread_get_thread_struct_locked(thread_id id);

static thread_id thread_get_current_thread_id(void);
static inline thread_id
thread_get_current_thread_id(void)
{
	Thread *thread = thread_get_current_thread();
	return thread ? thread->id : 0;
}

static inline bool
thread_is_idle_thread(Thread *thread)
{
	return thread->entry == NULL;
}

typedef bool (*thread_iterator_callback)(Thread* thread, void* cookie);
Thread* thread_iterate_through_threads(thread_iterator_callback callback,
	void* cookie);

thread_id allocate_thread_id(void);
thread_id peek_next_thread_id(void);

thread_id spawn_kernel_thread_etc(thread_func, const char *name, int32 priority,
	void *args, team_id team, thread_id threadID);
status_t wait_for_thread_etc(thread_id id, uint32 flags, bigtime_t timeout,
	status_t *_returnCode);

status_t select_thread(int32 object, struct select_info *info, bool kernel);
status_t deselect_thread(int32 object, struct select_info *info, bool kernel);

#define syscall_64_bit_return_value() arch_syscall_64_bit_return_value()

status_t thread_block();
status_t thread_block_with_timeout(uint32 timeoutFlags, bigtime_t timeout);
status_t thread_block_with_timeout_locked(uint32 timeoutFlags,
			bigtime_t timeout);
void thread_unblock(status_t threadID, status_t status);

// used in syscalls.c
status_t _user_set_thread_priority(thread_id thread, int32 newPriority);
status_t _user_rename_thread(thread_id thread, const char *name);
status_t _user_suspend_thread(thread_id thread);
status_t _user_resume_thread(thread_id thread);
status_t _user_rename_thread(thread_id thread, const char *name);
thread_id _user_spawn_thread(struct thread_creation_attributes* attributes);
status_t _user_wait_for_thread(thread_id id, status_t *_returnCode);
status_t _user_snooze_etc(bigtime_t timeout, int timebase, uint32 flags);
status_t _user_kill_thread(thread_id thread);
void _user_thread_yield(void);
void _user_exit_thread(status_t return_value);
bool _user_has_data(thread_id thread);
status_t _user_send_data(thread_id thread, int32 code, const void *buffer, size_t buffer_size);
status_t _user_receive_data(thread_id *_sender, void *buffer, size_t buffer_size);
thread_id _user_find_thread(const char *name);
status_t _user_get_thread_info(thread_id id, thread_info *info);
status_t _user_get_next_thread_info(team_id team, int32 *cookie, thread_info *info);

status_t _user_block_thread(uint32 flags, bigtime_t timeout);
status_t _user_unblock_thread(thread_id thread, status_t status);
status_t _user_unblock_threads(thread_id* threads, uint32 count,
	status_t status);

// ToDo: these don't belong here
struct rlimit;
int _user_getrlimit(int resource, struct rlimit * rlp);
int _user_setrlimit(int resource, const struct rlimit * rlp);

#ifdef __cplusplus
}
#endif


/*!
	\a thread must be the current thread.
	Thread lock can be, but doesn't need to be held.
*/
static inline bool
thread_is_interrupted(Thread* thread, uint32 flags)
{
	return ((flags & B_CAN_INTERRUPT)
			&& (thread->sig_pending & ~thread->sig_block_mask) != 0)
		|| ((flags & B_KILL_CAN_INTERRUPT)
			&& (thread->sig_pending & KILL_SIGNALS));
}


static inline bool
thread_is_blocked(Thread* thread)
{
	return thread->wait.status == 1;
}


/*!
	\a thread must be the current thread.
	Thread lock can be, but doesn't need to be locked.
*/
static inline void
thread_prepare_to_block(Thread* thread, uint32 flags, uint32 type,
	const void* object)
{
	thread->wait.flags = flags;
	thread->wait.type = type;
	thread->wait.object = object;
	atomic_set(&thread->wait.status, 1);
		// Set status last to guarantee that the other fields are initialized
		// when a thread is waiting.
}


static inline status_t
thread_block_locked(Thread* thread)
{
	if (thread->wait.status == 1) {
		// check for signals, if interruptable
		if (thread_is_interrupted(thread, thread->wait.flags)) {
			thread->wait.status = B_INTERRUPTED;
		} else {
			thread->next_state = B_THREAD_WAITING;
			scheduler_reschedule();
		}
	}

	return thread->wait.status;
}


static inline void
thread_unblock_locked(Thread* thread, status_t status)
{
	if (atomic_test_and_set(&thread->wait.status, status, 1) != 1)
		return;

	// wake up the thread, if it is sleeping
	if (thread->state == B_THREAD_WAITING)
		scheduler_enqueue_in_run_queue(thread);
}


static inline status_t
thread_interrupt(Thread* thread, bool kill)
{
	if ((thread->wait.flags & B_CAN_INTERRUPT) != 0
		|| (kill && (thread->wait.flags & B_KILL_CAN_INTERRUPT) != 0)) {
		thread_unblock_locked(thread, B_INTERRUPTED);
		return B_OK;
	}

	return B_NOT_ALLOWED;
}


static inline void
thread_pin_to_current_cpu(Thread* thread)
{
	thread->pinned_to_cpu++;
}


static inline void
thread_unpin_from_current_cpu(Thread* thread)
{
	thread->pinned_to_cpu--;
}


#endif /* _THREAD_H */
