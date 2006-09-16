/*
 * Copyright 2002-2006, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002, Angelo Mottola, a.mottola@libero.it.
 *
 * Distributed under the terms of the MIT License.
 */

/* POSIX signals handling routines */

#include <OS.h>
#include <KernelExport.h>

#include <debug.h>
#include <kernel.h>
#include <kscheduler.h>
#include <ksignal.h>
#include <sem.h>
#include <team.h>
#include <thread.h>
#include <user_debugger.h>

#include <stddef.h>
#include <string.h>

//#define TRACE_SIGNAL
#ifdef TRACE_SIGNAL
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#define SIGNAL_TO_MASK(signal)	(1LL << (signal - 1))
#define BLOCKABLE_SIGNALS		(~(KILL_SIGNALS | SIGNAL_TO_MASK(SIGSTOP)))
#define DEFAULT_IGNORE_SIGNALS \
	(SIGNAL_TO_MASK(SIGCHLD) | SIGNAL_TO_MASK(SIGWINCH) | SIGNAL_TO_MASK(SIGCONT))


const char * const sigstr[NSIG] = {
	"NONE", "HUP", "INT", "QUIT", "ILL", "CHLD", "ABRT", "PIPE",
	"FPE", "KILL", "STOP", "SEGV", "CONT", "TSTP", "ALRM", "TERM",
	"TTIN", "TTOU", "USR1", "USR2", "WINCH", "KILLTHR", "TRAP",
	"POLL", "PROF", "SYS", "URG", "VTALRM", "XCPU", "XFSZ"
};


static bool
notify_debugger(struct thread *thread, int signal, struct sigaction *handler,
	bool deadly)
{
	uint64 signalMask = SIGNAL_TO_MASK(signal);

	// first check the ignore signal masks the debugger specified for the thread

	if (atomic_get(&thread->debug_info.ignore_signals_once) & signalMask) {
		atomic_and(&thread->debug_info.ignore_signals_once, ~signalMask);
		return true;
	}

	if (atomic_get(&thread->debug_info.ignore_signals) & signalMask)
		return true;

	// deliver the event
	return user_debug_handle_signal(signal, handler, deadly);
}


/** Actually handles the signal - ie. the thread will exit, a custom signal
 *	handler is prepared, or whatever the signal demands.
 */

bool
handle_signals(struct thread *thread)
{
	uint32 signalMask = atomic_get(&thread->sig_pending)
		& ~atomic_get(&thread->sig_block_mask);
	struct sigaction *handler;
	bool reschedule = false;
	int32 i;

	// If SIGKILL[THR] are pending, we ignore other signals.
	// Otherwise check, if the thread shall stop for debugging.
	if (signalMask & KILL_SIGNALS) {
		signalMask &= KILL_SIGNALS;
	} else if (thread->debug_info.flags & B_THREAD_DEBUG_STOP) {
		user_debug_stop_thread();
	}

	if (signalMask == 0)
		return 0;

	for (i = 0; i < NSIG; i++) {
		bool debugSignal;
		int32 signal = i + 1;

		if ((signalMask & SIGNAL_TO_MASK(signal)) == 0)
			continue;

		// clear the signal that we will handle
		atomic_and(&thread->sig_pending, ~SIGNAL_TO_MASK(signal));

		debugSignal = !(~atomic_get(&thread->team->debug_info.flags)
				& (B_TEAM_DEBUG_SIGNALS | B_TEAM_DEBUG_DEBUGGER_INSTALLED));

		// ToDo: since sigaction_etc() could clobber the fields at any time,
		//		we should actually copy the relevant fields atomically before
		//		accessing them (only the debugger is calling sigaction_etc()
		//		right now).
		handler = &thread->sig_action[i];

		TRACE(("Thread 0x%lx received signal %s\n", thread->id, sigstr[signal]));

		if (handler->sa_handler == SIG_IGN) {
			// signal is to be ignored
			// ToDo: apply zombie cleaning on SIGCHLD

			// notify the debugger
			if (debugSignal)
				notify_debugger(thread, signal, handler, false);
			continue;
		}
		if (handler->sa_handler == SIG_DFL) {
			// default signal behaviour
			switch (signal) {
				case SIGCHLD:
				case SIGWINCH:
				case SIGTSTP:
				case SIGTTIN:
				case SIGTTOU:
				case SIGCONT:
				case SIGURG:
					// notify the debugger
					if (debugSignal)
						notify_debugger(thread, signal, handler, false);
					continue;

				case SIGSTOP:
					// notify the debugger
					if (debugSignal
						&& !notify_debugger(thread, signal, handler, false))
						continue;

					thread->next_state = B_THREAD_SUSPENDED;
					reschedule = true;
					continue;

				case SIGQUIT:
				case SIGILL:
				case SIGTRAP:
				case SIGABRT:
				case SIGFPE:
				case SIGSEGV:
				case SIGPOLL:
				case SIGPROF:
				case SIGSYS:
				case SIGVTALRM:
				case SIGXCPU:
				case SIGXFSZ:
					TRACE(("Shutting down thread 0x%lx due to signal #%d\n",
						thread->id, signal));
				case SIGKILL:
				case SIGKILLTHR:
				default:
					// if the thread exited normally, the exit reason is already set
					if (thread->exit.reason != THREAD_RETURN_EXIT) {
						thread->exit.reason = THREAD_RETURN_INTERRUPTED;
						thread->exit.signal = (uint16)signal;
					}

					// notify the debugger
					if (debugSignal && signal != SIGKILL && signal != SIGKILLTHR
						&& !notify_debugger(thread, signal, handler, true))
						continue;

					thread_exit();
						// won't return
			}
		}

		// notify the debugger
		if (debugSignal && !notify_debugger(thread, signal, handler, false))
			continue;

		// User defined signal handler
		TRACE(("### Setting up custom signal handler frame...\n"));
		arch_setup_signal_frame(thread, handler, signal, atomic_get(&thread->sig_block_mask));

		if (handler->sa_flags & SA_ONESHOT)
			handler->sa_handler = SIG_DFL;
		if ((handler->sa_flags & SA_NOMASK) == 0) {
			// Update the block mask while the signal handler is running - it
			// will be automatically restored when the signal frame is left.
			atomic_or(&thread->sig_block_mask,
				(handler->sa_mask | SIGNAL_TO_MASK(signal)) & BLOCKABLE_SIGNALS);
		}

		return reschedule;
	}

	arch_check_syscall_restart(thread);
	return reschedule;
}


bool
is_kill_signal_pending(void)
{
	return (atomic_get(&thread_get_current_thread()->sig_pending) & KILL_SIGNALS) != 0;
}


/**	Delivers the \a signal to the \a thread, but doesn't handle the signal -
 *	it just makes sure the thread gets the signal, ie. unblocks it if needed.
 *	This function must be called with interrupts disabled and the
 *	thread lock held.
 */

static status_t
deliver_signal(struct thread *thread, uint signal, uint32 flags)
{
	if (flags & B_CHECK_PERMISSION) {
		// ToDo: introduce euid & uid fields to the team and check permission
	}
	
	if (signal == 0)
		return B_OK;

	if (thread->team == team_get_kernel_team()) {
		// Signals to kernel threads will only wake them up
		if (thread->state == B_THREAD_SUSPENDED) {
			thread->state = thread->next_state = B_THREAD_READY;
			scheduler_enqueue_in_run_queue(thread);
		}
		return B_OK;
	}

	atomic_or(&thread->sig_pending, SIGNAL_TO_MASK(signal));

	switch (signal) {
		case SIGKILL:
		{
			struct thread *mainThread = thread->team->main_thread;
			// Forward KILLTHR to the main thread of the team

			mainThread->sig_pending |= SIGNAL_TO_MASK(SIGKILLTHR);
			// Wake up main thread
			if (mainThread->state == B_THREAD_SUSPENDED) {
				mainThread->state = mainThread->next_state = B_THREAD_READY;
				scheduler_enqueue_in_run_queue(mainThread);
			} else if (mainThread->state == B_THREAD_WAITING)
				sem_interrupt_thread(mainThread);

			// Supposed to fall through
		}
		case SIGKILLTHR:
			// Wake up suspended threads and interrupt waiting ones
			if (thread->state == B_THREAD_SUSPENDED) {
				thread->state = thread->next_state = B_THREAD_READY;
				scheduler_enqueue_in_run_queue(thread);
			} else if (thread->state == B_THREAD_WAITING)
				sem_interrupt_thread(thread);
			break;
		case SIGCONT:
			// Wake up thread if it was suspended
			if (thread->state == B_THREAD_SUSPENDED) {
				thread->state = thread->next_state = B_THREAD_READY;
				scheduler_enqueue_in_run_queue(thread);
			}
			break;

		default:
			if (thread->sig_pending & (~thread->sig_block_mask | SIGNAL_TO_MASK(SIGCHLD))) {
				// Interrupt thread if it was waiting
				if (thread->state == B_THREAD_WAITING)
					sem_interrupt_thread(thread);
			}
			break;
	}
	return B_OK;
}


int
send_signal_etc(pid_t id, uint signal, uint32 flags)
{
	status_t status = B_BAD_THREAD_ID;
	struct thread *thread;
	cpu_status state;

	if (signal < 0 || signal > MAX_SIGNO)
		return B_BAD_VALUE;

	state = disable_interrupts();

	if (id > 0) {
		// send a signal to the specified thread

		GRAB_THREAD_LOCK();

		thread = thread_get_thread_struct_locked(id);
		if (thread != NULL)
			status = deliver_signal(thread, signal, flags);
	} else {
		// send a signal to the specified process group
		// (the absolute value of the id)

		struct team *team = thread_get_current_thread()->team;
		struct process_group *group;

		// TODO: handle -1 correctly
		if (id == 0 || id == -1) {
			// send a signal to the current team
			id = thread_get_current_thread()->team->main_thread->id;
		} else
			id = -id;

		GRAB_TEAM_LOCK();

		group = team_get_process_group_locked(team, id);
		if (group != NULL) {
			struct team *team, *next;

			// we need a safe way to get from the thread to the process group
			// XXX whats this? id = thread->team->id;

			for (team = group->teams; team != NULL; team = next) {
				next = team->group_next;
				id = team->main_thread->id;

				GRAB_THREAD_LOCK();

				thread = thread_get_thread_struct_locked(id);
				if (thread != NULL) {
					// we don't stop because of an error sending the signal; we
					// rather want to send as much signals as possible
					status = deliver_signal(thread, signal, flags);
				}

				RELEASE_THREAD_LOCK();
			}
			
		}
		RELEASE_TEAM_LOCK();
		GRAB_THREAD_LOCK();
	}		

	// ToDo: maybe the scheduler should only be invoked if there is reason to do it?
	//	(ie. deliver_signal() moved some threads in the running queue?)
	if ((flags & B_DO_NOT_RESCHEDULE) == 0)
		scheduler_reschedule();

	RELEASE_THREAD_LOCK();
	restore_interrupts(state);

	return status;
}


int
send_signal(pid_t threadID, uint signal)
{
	// The BeBook states that this function wouldn't be exported
	// for drivers, but, of course, it's wrong.
	return send_signal_etc(threadID, signal, 0);
}


int
has_signals_pending(void *_thread)
{
	struct thread *thread = (struct thread *)_thread;
	if (thread == NULL)
		thread = thread_get_current_thread();

	return atomic_get(&thread->sig_pending) & ~atomic_get(&thread->sig_block_mask);
}


int
sigprocmask(int how, const sigset_t *set, sigset_t *oldSet)
{
	struct thread *thread = thread_get_current_thread();
	sigset_t oldMask = atomic_get(&thread->sig_block_mask);

	if (set != NULL) {
		switch (how) {
			case SIG_BLOCK:
				atomic_or(&thread->sig_block_mask, *set & BLOCKABLE_SIGNALS);
				break;
			case SIG_UNBLOCK:
				atomic_and(&thread->sig_block_mask, ~*set);
				break;
			case SIG_SETMASK:
				atomic_set(&thread->sig_block_mask, *set & BLOCKABLE_SIGNALS);
				break;
			default:
				return B_BAD_VALUE;
		}
	}

	if (oldSet != NULL)
		*oldSet = oldMask;

	return B_OK;
}


/**	\brief sigaction() for the specified thread.
 *
 *	A \a threadID is < 0 specifies the current thread.
 *
 */

int
sigaction_etc(thread_id threadID, int signal, const struct sigaction *act,
	struct sigaction *oldAction)
{
	struct thread *thread;
	cpu_status state;
	status_t error = B_OK;

	if (signal < 1 || signal > MAX_SIGNO
		|| (SIGNAL_TO_MASK(signal) & ~BLOCKABLE_SIGNALS) != 0)
		return B_BAD_VALUE;

	state = disable_interrupts();
	GRAB_THREAD_LOCK();

	thread = (threadID < 0
		? thread_get_current_thread()
		: thread_get_thread_struct_locked(threadID));

	if (thread) {
		if (oldAction) {
			// save previous sigaction structure
			memcpy(oldAction, &thread->sig_action[signal - 1],
				sizeof(struct sigaction));
		}

		if (act) {
			// set new sigaction structure
			memcpy(&thread->sig_action[signal - 1], act,
				sizeof(struct sigaction));
			thread->sig_action[signal - 1].sa_mask &= BLOCKABLE_SIGNALS;
		}

		if (act && act->sa_handler == SIG_IGN) {
			// remove pending signal if it should now be ignored
			atomic_and(&thread->sig_pending, ~SIGNAL_TO_MASK(signal));
		} else if (act && act->sa_handler == SIG_DFL
			&& (SIGNAL_TO_MASK(signal) & DEFAULT_IGNORE_SIGNALS) != NULL) {
			// remove pending signal for those signals whose default
			// action is to ignore them
			atomic_and(&thread->sig_pending, ~SIGNAL_TO_MASK(signal));
		}
	} else
		error = B_BAD_THREAD_ID;

	RELEASE_THREAD_LOCK();
	restore_interrupts(state);

	return error;
}


int
sigaction(int signal, const struct sigaction *act, struct sigaction *oldAction)
{
	return sigaction_etc(-1, signal, act, oldAction);
}


/** Triggers a SIGALRM to the thread that issued the timer and reschedules */

static int32
alarm_event(timer *t)
{
	// The hook can be called from any context, but we have to
	// deliver the signal to the thread that originally called
	// set_alarm().
	// Since thread->alarm is this timer structure, we can just
	// cast it back - ugly but it works for now
	struct thread *thread = (struct thread *)((uint8 *)t - offsetof(struct thread, alarm));
		// ToDo: investigate adding one user parameter to the timer structure to fix this hack

	TRACE(("alarm_event: thread = %p\n", thread));
	send_signal_etc(thread->id, SIGALRM, B_DO_NOT_RESCHEDULE);

	return B_INVOKE_SCHEDULER;
}


/** Sets the alarm timer for the current thread. The timer fires at the
 *	specified time in the future, periodically or just once, as determined
 *	by \a mode.
 *	\return the time left until a previous set alarm would have fired.
 */

bigtime_t
set_alarm(bigtime_t time, uint32 mode)
{
	struct thread *thread = thread_get_current_thread();
	bigtime_t remainingTime = 0;

	ASSERT(B_ONE_SHOT_RELATIVE_ALARM == B_ONE_SHOT_RELATIVE_TIMER);
		// just to be sure no one changes the headers some day

	TRACE(("set_alarm: thread = %p\n", thread));

	if (thread->alarm.period)
		remainingTime = (bigtime_t)thread->alarm.entry.key - system_time();

	cancel_timer(&thread->alarm);

	if (time != B_INFINITE_TIMEOUT)
		add_timer(&thread->alarm, &alarm_event, time, mode);
	else {
		// this marks the alarm as canceled (for returning the remaining time)
		thread->alarm.period = 0;
	}

	return remainingTime;
}


/**	Replace the current signal block mask and wait for any event to happen.
 *	Before returning, the original signal block mask is reinstantiated.
 */

int
sigsuspend(const sigset_t *mask)
{
	struct thread *thread = thread_get_current_thread();
	sigset_t oldMask = atomic_get(&thread->sig_block_mask);
	cpu_status state;

	// set the new block mask and suspend ourselves - we cannot use
	// SIGSTOP for this, as signals are only handled upon kernel exit

	atomic_set(&thread->sig_block_mask, *mask);

	while (true) {
		thread->next_state = B_THREAD_SUSPENDED;

		state = disable_interrupts();
		GRAB_THREAD_LOCK();

		scheduler_reschedule();

		RELEASE_THREAD_LOCK();
		restore_interrupts(state);

		if (has_signals_pending(thread))
			break;
	}

	// restore the original block mask
	atomic_set(&thread->sig_block_mask, oldMask);

	// we're not supposed to actually succeed
	// ToDo: could this get us into trouble with SA_RESTART handlers?
	return B_INTERRUPTED;
}


int
sigpending(sigset_t *set)
{
	struct thread *thread = thread_get_current_thread();

	if (set == NULL)
		return B_BAD_VALUE;

	*set = atomic_get(&thread->sig_pending);	
	return B_OK;
}


//	#pragma mark -


bigtime_t
_user_set_alarm(bigtime_t time, uint32 mode)
{
	return set_alarm(time, mode);
}


int
_user_send_signal(pid_t team, uint signal)
{
	return send_signal_etc(team, signal, B_CHECK_PERMISSION);
}


int
_user_sigprocmask(int how, const sigset_t *userSet, sigset_t *userOldSet)
{
	sigset_t set, oldSet;
	status_t status;

	if ((userSet != NULL && user_memcpy(&set, userSet, sizeof(sigset_t)) < B_OK)
		|| (userOldSet != NULL && user_memcpy(&oldSet, userOldSet, sizeof(sigset_t)) < B_OK))
		return B_BAD_ADDRESS;

	status = sigprocmask(how, userSet ? &set : NULL, userOldSet ? &oldSet : NULL);

	// copy old set if asked for
	if (status >= B_OK && userOldSet != NULL && user_memcpy(userOldSet, &oldSet, sizeof(sigset_t)) < B_OK)
		return B_BAD_ADDRESS;

	return status;
}


int
_user_sigaction(int signal, const struct sigaction *userAction, struct sigaction *userOldAction)
{
	struct sigaction act, oact;
	status_t status;

	if ((userAction != NULL && user_memcpy(&act, userAction, sizeof(struct sigaction)) < B_OK)
		|| (userOldAction != NULL && user_memcpy(&oact, userOldAction, sizeof(struct sigaction)) < B_OK))
		return B_BAD_ADDRESS;

	status = sigaction(signal, userAction ? &act : NULL, userOldAction ? &oact : NULL);

	// only copy the old action if a pointer has been given
	if (status >= B_OK && userOldAction != NULL
		&& user_memcpy(userOldAction, &oact, sizeof(struct sigaction)) < B_OK)
		return B_BAD_ADDRESS;

	return status;
}


int
_user_sigsuspend(const sigset_t *userMask)
{
	sigset_t mask;

	if (userMask == NULL)
		return B_BAD_VALUE;
	if (user_memcpy(&mask, userMask, sizeof(sigset_t)) < B_OK)
		return B_BAD_ADDRESS;

	return sigsuspend(&mask);
}


int
_user_sigpending(sigset_t *userSet)
{
	sigset_t set;
	int status;

	if (userSet == NULL)
		return B_BAD_VALUE;
	if (!IS_USER_ADDRESS(userSet))
		return B_BAD_ADDRESS;

	status = sigpending(&set);
	if (status == B_OK
		&& user_memcpy(userSet, &set, sizeof(sigset_t)) < B_OK)
		return B_BAD_ADDRESS;

	return status;
}

