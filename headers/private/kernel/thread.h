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


struct arch_fork_arg;
struct kernel_args;
struct select_info;
struct thread_creation_attributes;


// thread notifications
#define THREAD_MONITOR		'_tm_'
#define THREAD_ADDED		0x01
#define THREAD_REMOVED		0x02
#define THREAD_NAME_CHANGED	0x04


namespace BKernel {


struct ThreadCreationAttributes : thread_creation_attributes {
	// when calling from kernel only
	team_id			team;
	Thread*			thread;
	sigset_t		signal_mask;
	size_t			additional_stack_size;	// additional space in the stack
											// area after the TLS region, not
											// used as thread stack
	thread_func		kernelEntry;
	void*			kernelArgument;
	arch_fork_arg*	forkArgs;				// If non-NULL, the userland thread
											// will be started with this
											// register context.

public:
								ThreadCreationAttributes() {}
									// no-init constructor
								ThreadCreationAttributes(
									thread_func function, const char* name,
									int32 priority, void* arg,
									team_id team = -1, Thread* thread = NULL);

			status_t			InitFromUserAttributes(
									const thread_creation_attributes*
										userAttributes,
									char* nameBuffer);
};


}	// namespace BKernel

using BKernel::ThreadCreationAttributes;


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
	return thread->priority == B_IDLE_PRIORITY;
}

thread_id allocate_thread_id();
thread_id peek_next_thread_id();

status_t thread_enter_userspace_new_team(Thread* thread, addr_t entryFunction,
	void* argument1, void* argument2);
status_t thread_create_user_stack(Team* team, Thread* thread, void* stackBase,
	size_t stackSize, size_t additionalSize);
thread_id thread_create_thread(const ThreadCreationAttributes& attributes,
	bool kernel);

thread_id spawn_kernel_thread_etc(thread_func, const char *name, int32 priority,
	void *args, team_id team);
status_t wait_for_thread_etc(thread_id id, uint32 flags, bigtime_t timeout,
	status_t *_returnCode);

status_t select_thread(int32 object, struct select_info *info, bool kernel);
status_t deselect_thread(int32 object, struct select_info *info, bool kernel);

#define syscall_64_bit_return_value() arch_syscall_64_bit_return_value()

status_t thread_block();
status_t thread_block_with_timeout(uint32 timeoutFlags, bigtime_t timeout);
status_t thread_block_with_timeout_locked(uint32 timeoutFlags,
			bigtime_t timeout);

// used in syscalls.c
status_t _user_set_thread_priority(thread_id thread, int32 newPriority);
status_t _user_rename_thread(thread_id thread, const char *name);
status_t _user_suspend_thread(thread_id thread);
status_t _user_resume_thread(thread_id thread);
status_t _user_rename_thread(thread_id thread, const char *name);
thread_id _user_spawn_thread(struct thread_creation_attributes* attributes);
status_t _user_wait_for_thread(thread_id id, status_t *_returnCode);
status_t _user_snooze_etc(bigtime_t timeout, int timebase, uint32 flags,
	bigtime_t* _remainingTime);
status_t _user_kill_thread(thread_id thread);
status_t _user_cancel_thread(thread_id threadID, void (*cancelFunction)(int));
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


/*!	Checks whether the current thread would immediately be interrupted when
	blocking it with the given wait/interrupt flags.

	The caller must hold the scheduler lock.

	\param thread The current thread.
	\param flags Wait/interrupt flags to be considered. Relevant are:
		- \c B_CAN_INTERRUPT: The thread can be interrupted by any non-blocked
			signal. Implies \c B_KILL_CAN_INTERRUPT (specified or not).
		- \c B_KILL_CAN_INTERRUPT: The thread can be interrupted by a kill
			signal.
	\return \c true, if the thread would be interrupted, \c false otherwise.
*/
static inline bool
thread_is_interrupted(Thread* thread, uint32 flags)
{
	sigset_t pendingSignals = thread->AllPendingSignals();
	return ((flags & B_CAN_INTERRUPT) != 0
			&& (pendingSignals & ~thread->sig_block_mask) != 0)
		|| ((flags & B_KILL_CAN_INTERRUPT) != 0
			&& (pendingSignals & KILL_SIGNALS) != 0);
}


/*!	Checks wether the given thread is currently blocked (i.e. still waiting for
	something).

	If a stable answer is required, the caller must hold the scheduler lock.
	Alternatively, if waiting is not interruptible and cannot time out, holding
	the client lock held when calling thread_prepare_to_block() and the
	unblocking functions works as well.

	\param thread The thread in question.
	\return \c true, if the thread is blocked, \c false otherwise.
*/
static inline bool
thread_is_blocked(Thread* thread)
{
	return thread->wait.status == 1;
}


/*!	Prepares the current thread for waiting.

	This is the first of two steps necessary to block the current thread
	(IOW, to let it wait for someone else to unblock it or optionally time out
	after a specified delay). The process consists of two steps to avoid race
	conditions in case a lock other than the scheduler lock is involved.

	Usually the thread waits for some condition to change and this condition is
	something reflected in the caller's data structures which should be
	protected by a client lock the caller knows about. E.g. in the semaphore
	code that lock is a per-semaphore spinlock that protects the semaphore data,
	including the semaphore count and the queue of waiting threads. For certain
	low-level locking primitives (e.g. mutexes) that client lock is the
	scheduler lock itself, which simplifies things a bit.

	If a client lock other than the scheduler lock is used, this function must
	be called with that lock being held. Afterwards that lock should be dropped
	and the function that actually blocks the thread shall be invoked
	(thread_block[_locked]() or thread_block_with_timeout[_locked]()). In
	between these two steps no functionality that uses the thread blocking API
	for this thread shall be used.

	When the caller determines that the condition for unblocking the thread
	occurred, it calls thread_unblock_locked() to unblock the thread. At that
	time one of locks that are held when calling thread_prepare_to_block() must
	be held. Usually that would be the client lock. In two cases it generally
	isn't, however, since the unblocking code doesn't know about the client
	lock: 1. When thread_block_with_timeout[_locked]() had been used and the
	timeout occurs. 2. When thread_prepare_to_block() had been called with one
	or both of the \c B_CAN_INTERRUPT or \c B_KILL_CAN_INTERRUPT flags specified
	and someone calls thread_interrupt() that is supposed to wake up the thread.
	In either of these two cases only the scheduler lock is held by the
	unblocking code. A timeout can only happen after
	thread_block_with_timeout_locked() has been called, but an interruption is
	possible at any time. The client code must deal with those situations.

	Generally blocking and unblocking threads proceed in the following manner:

	Blocking thread:
	- Acquire client lock.
	- Check client condition and decide whether blocking is necessary.
	- Modify some client data structure to indicate that this thread is now
		waiting.
	- Release client lock (unless client lock is the scheduler lock).
	- Block.
	- Acquire client lock (unless client lock is the scheduler lock).
	- Check client condition and compare with block result. E.g. if the wait was
		interrupted or timed out, but the client condition indicates success, it
		may be considered a success after all, since usually that happens when
		another thread concurrently changed the client condition and also tried
		to unblock the waiting thread. It is even necessary when that other
		thread changed the client data structures in a way that associate some
		resource with the unblocked thread, or otherwise the unblocked thread
		would have to reverse that here.
	- If still necessary -- i.e. not already taken care of by an unblocking
		thread -- modify some client structure to indicate that the thread is no
		longer waiting, so it isn't erroneously unblocked later.

	Unblocking thread:
	- Acquire client lock.
	- Check client condition and decide whether a blocked thread can be woken
		up.
	- Check the client data structure that indicates whether one or more threads
		are waiting and which thread(s) need(s) to be woken up.
	- Unblock respective thread(s).
	- Possibly change some client structure, so that an unblocked thread can
		decide whether a concurrent timeout/interruption can be ignored, or
		simply so that it doesn't have to do any more cleanup.

	Note that in the blocking thread the steps after blocking are strictly
	required only if timeouts or interruptions are possible. If they are not,
	the blocking thread can only be woken up explicitly by an unblocking thread,
	which could already take care of all the necessary client data structure
	modifications, so that the blocking thread wouldn't have to do that.

	Note that the client lock can but does not have to be a spinlock.
	A mutex, a semaphore, or anything that doesn't try to use the thread
	blocking API for the calling thread when releasing the lock is fine.
	In particular that means in principle thread_prepare_to_block() can be
	called with interrupts enabled.

	Care must be taken when the wait can be interrupted or can time out,
	especially with a client lock that uses the thread blocking API. After a
	blocked thread has been interrupted or the the time out occurred it cannot
	acquire the client lock (or any other lock using the thread blocking API)
	without first making sure that the thread doesn't still appears to be
	waiting to other client code. Otherwise another thread could try to unblock
	it which could erroneously unblock the thread while already waiting on the
	client lock. So usually when interruptions or timeouts are possible a
	spinlock needs to be involved.

	\param thread The current thread.
	\param flags The blocking flags. Relevant are:
		- \c B_CAN_INTERRUPT: The thread can be interrupted by any non-blocked
			signal. Implies \c B_KILL_CAN_INTERRUPT (specified or not).
		- \c B_KILL_CAN_INTERRUPT: The thread can be interrupted by a kill
			signal.
	\param type The type of object the thread will be blocked at. Informative/
		for debugging purposes. Must be one of the \c THREAD_BLOCK_TYPE_*
		constants. \c THREAD_BLOCK_TYPE_OTHER implies that \a object is a
		string.
	\param object The object the thread will be blocked at.  Informative/for
		debugging purposes.
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


/*!	Blocks the current thread.

	The thread is blocked until someone else unblock it. Must be called after a
	call to thread_prepare_to_block(). If the thread has already been unblocked
	after the previous call to thread_prepare_to_block(), this function will
	return immediately. Cf. the documentation of thread_prepare_to_block() for
	more details.

	The caller must hold the scheduler lock.

	\param thread The current thread.
	\return The error code passed to the unblocking function. thread_interrupt()
		uses \c B_INTERRUPTED. By convention \c B_OK means that the wait was
		successful while another error code indicates a failure (what that means
		depends on the client code).
*/
static inline status_t
thread_block_locked(Thread* thread)
{
	if (thread->wait.status == 1) {
		// check for signals, if interruptible
		if (thread_is_interrupted(thread, thread->wait.flags)) {
			thread->wait.status = B_INTERRUPTED;
		} else {
			thread->next_state = B_THREAD_WAITING;
			scheduler_reschedule();
		}
	}

	return thread->wait.status;
}


/*!	Unblocks the specified blocked thread.

	If the thread is no longer waiting (e.g. because thread_unblock_locked() has
	already been called in the meantime), this function does not have any
	effect.

	The caller must hold the scheduler lock and the client lock (might be the
	same).

	\param thread The thread to be unblocked.
	\param status The unblocking status. That's what the unblocked thread's
		call to thread_block_locked() will return.
*/
static inline void
thread_unblock_locked(Thread* thread, status_t status)
{
	if (atomic_test_and_set(&thread->wait.status, status, 1) != 1)
		return;

	// wake up the thread, if it is sleeping
	if (thread->state == B_THREAD_WAITING)
		scheduler_enqueue_in_run_queue(thread);
}


/*!	Interrupts the specified blocked thread, if possible.

	The function checks whether the thread can be interrupted and, if so, calls
	\code thread_unblock_locked(thread, B_INTERRUPTED) \endcode. Otherwise the
	function is a no-op.

	The caller must hold the scheduler lock. Normally thread_unblock_locked()
	also requires the client lock to be held, but in this case the caller
	usually doesn't know it. This implies that the client code needs to take
	special care, if waits are interruptible. See thread_prepare_to_block() for
	more information.

	\param thread The thread to be interrupted.
	\param kill If \c false, the blocked thread is only interrupted, when the
		flag \c B_CAN_INTERRUPT was specified for the blocked thread. If
		\c true, it is only interrupted, when at least one of the flags
		\c B_CAN_INTERRUPT or \c B_KILL_CAN_INTERRUPT was specified for the
		blocked thread.
	\return \c B_OK, if the thread is interruptible and thread_unblock_locked()
		was called, \c B_NOT_ALLOWED otherwise. \c B_OK doesn't imply that the
		thread actually has been interrupted -- it could have been unblocked
		before already.
*/
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
