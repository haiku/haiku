/*
 * Copyright 2002-2005, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002, Angelo Mottola, a.mottola@libero.it.
 *
 * Distributed under the terms of the MIT License.
 */

/* POSIX signals handling routines */

#include <OS.h>
#include <KernelExport.h>
#include <debug.h>
#include <debugger.h>
#include <thread.h>
#include <team.h>
#include <int.h>
#include <sem.h>
#include <ksignal.h>
#include <syscalls.h>
#include <user_debugger.h>

#include <stddef.h>
#include <string.h>

//#define TRACE_SIGNAL
#ifdef TRACE_SIGNAL
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#define SIGNAL_TO_MASK(signal) (1LL << (signal - 1))


const char * const sigstr[NSIG] = {
	"NONE", "HUP", "INT", "QUIT", "ILL", "CHLD", "ABRT", "PIPE",
	"FPE", "KILL", "STOP", "SEGV", "CONT", "TSTP", "ALRM", "TERM",
	"TTIN", "TTOU", "USR1", "USR2", "WINCH", "KILLTHR", "TRAP"
};


static bool
notify_debugger(struct thread *thread, int signal, struct sigaction *handler,
	bool deadly, cpu_status *state)
{
	bool result;
	uint64 signalMask = SIGNAL_TO_MASK(signal);

	// first check the ignore signal masks the debugger specified for the thread

	if (thread->debug_info.ignore_signals_once & signalMask) {
		thread->debug_info.ignore_signals_once &= ~signalMask;
		return true;
	}

	if (thread->debug_info.ignore_signals & signalMask)
		return true;

	// deliver the event

	RELEASE_THREAD_LOCK();
	restore_interrupts(*state);

	result = user_debug_handle_signal(signal, handler, deadly);

	*state = disable_interrupts();
	GRAB_THREAD_LOCK();

	return result;
}


/**
 *	Expects interrupts off and thread lock held.
 *	The function may release the lock and enable interrupts temporarily, so the
 *	caller must be aware that operations before calling this function and after
 *	its return might not belong to the same atomic section.
 */

int
handle_signals(struct thread *thread, cpu_status *state)
{
	uint32 signalMask = thread->sig_pending & (~thread->sig_block_mask);
	int i, signal, global_resched = 0;
	struct sigaction *handler;

	// If SIGKILL[THR] are pending, we ignore other signals.
	// Otherwise check, if the thread shall stop for debugging.
	if (signalMask & KILL_SIGNALS) {
		signalMask &= KILL_SIGNALS;
	} else if (thread->debug_info.flags & B_THREAD_DEBUG_STOP) {
		RELEASE_THREAD_LOCK();
		restore_interrupts(*state);

		user_debug_stop_thread();

		*state = disable_interrupts();
		GRAB_THREAD_LOCK();

		signalMask = thread->sig_pending & (~thread->sig_block_mask);
	}

	if (signalMask == 0)
		return 0;

	for (i = 0; i < NSIG; i++) {
		if (signalMask & 0x1) {
			bool debugSignal = !(~atomic_get(&thread->team->debug_info.flags)
				& (B_TEAM_DEBUG_SIGNALS | B_TEAM_DEBUG_DEBUGGER_INSTALLED));

			signal = i + 1;
			handler = &thread->sig_action[i];
			signalMask >>= 1;
			thread->sig_pending &= ~SIGNAL_TO_MASK(signal);

			TRACE(("Thread 0x%lx received signal %s\n", thread->id, sigstr[signal]));

			if (handler->sa_handler == SIG_IGN) {
				// signal is to be ignored
				// ToDo: apply zombie cleaning on SIGCHLD

				// notify the debugger
				if (debugSignal)
					notify_debugger(thread, signal, handler, false, state);
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
						// notify the debugger
						if (debugSignal) {
							notify_debugger(thread, signal, handler, false, state);
						}
						continue;

					case SIGSTOP:
						// notify the debugger
						if (debugSignal) {
							if (!notify_debugger(thread, signal, handler, false,
								state)) {
								continue;
							}
						}

						thread->next_state = B_THREAD_SUSPENDED;
						global_resched = 1;
						continue;

					case SIGQUIT:
					case SIGILL:
					case SIGTRAP:
					case SIGABRT:
					case SIGFPE:
					case SIGSEGV:
						TRACE(("Shutting down thread 0x%lx due to signal #%d\n", thread->id, signal));
					case SIGKILL:
					case SIGKILLTHR:
					default:
						if (thread->exit.reason != THREAD_RETURN_EXIT)
							thread->exit.reason = THREAD_RETURN_INTERRUPTED;

						// notify the debugger
						if (debugSignal && signal != SIGKILL
								&& signal != SIGKILLTHR) {
							if (!notify_debugger(thread, signal, handler, true,
								state)) {
								continue;
							}
						}

						RELEASE_THREAD_LOCK();
						restore_interrupts(*state);

						// ToDo: when we have more than a thread per process,
						// it can likely happen (for any thread other than the first)
						// that here, interrupts are still disabled.
						// Changing the above line with "enable_interrupts()" fixes
						// the problem, though we should find its cause.
						// We absolutely need interrupts enabled when we enter
						// thread_exit().

						thread_exit();
				}
			}

			// notify the debugger
			if (debugSignal) {
				if (!notify_debugger(thread, signal, handler, false, state))
					continue;
			}

			// ToDo: it's not safe to call arch_setup_signal_frame with
			//		interrupts disabled since it writes to the user stack
			//		and may page fault.

			// User defined signal handler
			TRACE(("### Setting up custom signal handler frame...\n"));
			arch_setup_signal_frame(thread, handler, signal, thread->sig_block_mask);

			if (handler->sa_flags & SA_ONESHOT)
				handler->sa_handler = SIG_DFL;
			if (!(handler->sa_flags & SA_NOMASK))
				thread->sig_block_mask |= (handler->sa_mask | SIGNAL_TO_MASK(signal)) & BLOCKABLE_SIGS;

			return global_resched;
		} else
			signalMask >>= 1;
	}
	arch_check_syscall_restart(thread);

	return global_resched;
}


bool
is_kill_signal_pending()
{
	bool result;
	struct thread *thread = thread_get_current_thread();
	
	cpu_status state = disable_interrupts();
	GRAB_THREAD_LOCK();

	result = (thread->sig_pending & KILL_SIGNALS);

	RELEASE_THREAD_LOCK();
	restore_interrupts(state);

	return result;
}


static status_t
deliver_signal(struct thread *thread, uint signal, uint32 flags)
{
	if (flags & B_CHECK_PERMISSION) {
		// ToDo: introduce euid & uid fields to the team and check permission
	}

	if (thread->team == team_get_kernel_team()) {
		// Signals to kernel threads will only wake them up
		if (thread->state == B_THREAD_SUSPENDED) {
			thread->state = thread->next_state = B_THREAD_READY;
			scheduler_enqueue_in_run_queue(thread);
		}
		return B_OK;
	}

	thread->sig_pending |= SIGNAL_TO_MASK(signal);

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

	if (signal < 1 || signal > MAX_SIGNO)
		return B_BAD_VALUE;

	if (id == 0) {
		// send a signal to the current team
		id = thread_get_current_thread()->team->main_thread->id;
	}

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

		GRAB_THREAD_LOCK();

		thread = thread_get_thread_struct_locked(-id);
		if (thread != NULL) {
			struct process_group *group;
			struct team *team, *next;

			// we need a safe way to get from the thread to the process group
			id = thread->team->id;

			RELEASE_THREAD_LOCK();
			GRAB_TEAM_LOCK();

			// get a pointer to the process group
			team = team_get_team_struct_locked(id);
			group = team->group;

			for (team = group->teams; team != NULL; team = next) {
				// ToDo: there is a *big* race condition here on SMP machines;
				// the team pointer will probably have gone bad in the mean time
				next = team->group_next;
				id = team->main_thread->id;

				RELEASE_TEAM_LOCK();
				GRAB_THREAD_LOCK();

				thread = thread_get_thread_struct_locked(id);
				if (thread != NULL) {
					// we don't stop because of an error sending the signal; we
					// rather want to send as much signals as possible
					status = deliver_signal(thread, signal, flags);
				}

				RELEASE_THREAD_LOCK();
				GRAB_TEAM_LOCK();
			}
			
			RELEASE_TEAM_LOCK();
			GRAB_THREAD_LOCK();
		}
	}		

	// ToDo: maybe the scheduler should only be invoked is there is reason to do it?
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

	return thread->sig_pending & ~thread->sig_block_mask;
}


int
sigprocmask(int how, const sigset_t *set, sigset_t *oldSet)
{
	struct thread *thread = thread_get_current_thread();
	sigset_t oldMask = thread->sig_block_mask;

	// ToDo: "sig_block_mask" is probably not the right thing to change?
	//	At least it's often changed at other places...

	switch (how) {
		case SIG_BLOCK:
			atomic_or(&thread->sig_block_mask, *set);
			break;
		case SIG_UNBLOCK:
			atomic_and(&thread->sig_block_mask, ~*set);
			break;
		case SIG_SETMASK:
			atomic_set(&thread->sig_block_mask, *set);
			break;

		default:
			return B_BAD_VALUE;
	}

	if (oldSet != NULL)
		*oldSet = oldMask;

	return B_OK;
}


/**	\brief Similar to sigaction(), just for a specified thread.
 *
 *	A \a threadID is < 0 specifies the current thread.
 *
 */

int
sigaction_etc(thread_id threadID, int signal, const struct sigaction *act,
	struct sigaction *oact)
{
	struct thread *thread;
	cpu_status state;
	status_t error = B_OK;

	if (signal < 1 || signal > MAX_SIGNO
		|| signal == SIGKILL || signal == SIGKILLTHR || signal == SIGSTOP)
		return EINVAL;

	state = disable_interrupts();
	GRAB_THREAD_LOCK();

	thread = (threadID < 0
		? thread_get_current_thread()
		: thread_get_thread_struct_locked(threadID));

	if (thread) {
		if (oact) {
			memcpy(oact, &thread->sig_action[signal - 1],
				sizeof(struct sigaction));
		}

		if (act) {
			memcpy(&thread->sig_action[signal - 1], act,
				sizeof(struct sigaction));
		}

		if (act && act->sa_handler == SIG_IGN)
			thread->sig_pending &= ~SIGNAL_TO_MASK(signal);
		else if (act && act->sa_handler == SIG_DFL) {
			if (signal == SIGCONT || signal == SIGCHLD
				|| signal == SIGWINCH) {
				thread->sig_pending &= ~SIGNAL_TO_MASK(signal);
			}
		} /*else
			dprintf("### custom signal handler set\n");*/
	} else
		error = B_BAD_THREAD_ID;

	RELEASE_THREAD_LOCK();
	restore_interrupts(state);

	return error;
}


int
sigaction(int signal, const struct sigaction *act, struct sigaction *oact)
{
	return sigaction_etc(-1, signal, act, oact);
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


bigtime_t
set_alarm(bigtime_t time, uint32 mode)
{
	struct thread *thread = thread_get_current_thread();
	bigtime_t rv = 0;

	ASSERT(B_ONE_SHOT_RELATIVE_ALARM == B_ONE_SHOT_RELATIVE_TIMER);
		// just to be sure no one changes the headers some day

	TRACE(("set_alarm: thread = %p\n", thread));

	if (thread->alarm.period)
		rv = (bigtime_t)thread->alarm.entry.key - system_time();

	cancel_timer(&thread->alarm);

	if (time != B_INFINITE_TIMEOUT)
		add_timer(&thread->alarm, &alarm_event, time, mode);

	return rv;
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

	if (userSet == NULL)
		return B_BAD_VALUE;

	if (user_memcpy(&set, userSet, sizeof(sigset_t)) < B_OK
		|| (userOldSet != NULL && user_memcpy(&oldSet, userOldSet, sizeof(sigset_t)) < B_OK))
		return B_BAD_ADDRESS;

	status = sigprocmask(how, &set, userOldSet ? &oldSet : NULL);

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
_user_sigsuspend(const sigset_t *mask) {
	sigset_t set;
	if (mask == NULL)
		return B_BAD_VALUE;

	if (user_memcpy(&set, mask, sizeof(sigset_t)) < B_OK)
		return B_BAD_ADDRESS;

	// Todo : implement
	return EINVAL;	
}
