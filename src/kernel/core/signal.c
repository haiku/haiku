/* POSIX signals handling routines
** 
** Copyright 2002, Angelo Mottola, a.mottola@libero.it. All rights reserved.
** Copyright 2002-2004, The OpenBeOS Team. All rights reserved.
**
** Distributed under the terms of the OpenBeOS License.
*/

#include <OS.h>
#include <KernelExport.h>
#include <debug.h>
#include <thread.h>
#include <team.h>
#include <int.h>
#include <sem.h>
#include <ksignal.h>
#include <syscalls.h>

#include <stddef.h>
#include <string.h>

#define SIGNAL_TO_MASK(signal) (1LL << (signal - 1))


const char * const sigstr[NSIG] = {
	"NONE", "HUP", "INT", "QUIT", "ILL", "CHLD", "ABRT", "PIPE",
	"FPE", "KILL", "STOP", "SEGV", "CONT", "TSTP", "ALRM", "TERM",
	"TTIN", "TTOU", "USR1", "USR2", "WINCH", "KILLTHR", "TRAP"
};


/** Expects interrupts off and thread lock held. */

int
handle_signals(struct thread *thread, int state)
{
	uint32 signalMask = thread->sig_pending & (~thread->sig_block_mask);
	int i, sig, global_resched = 0;
	struct sigaction *handler;

	if (signalMask == 0)
		return 0;

	for (i = 0; i < NSIG; i++) {
		if (signalMask & 0x1) {
			sig = i + 1;
			handler = &thread->sig_action[i];
			signalMask >>= 1;
			thread->sig_pending &= ~(1L << i);

			dprintf("Thread 0x%lx received signal %s\n", thread->id, sigstr[sig]);

			if (handler->sa_handler == SIG_IGN) {
				// signal is to be ignored
				// ToDo: apply zombie cleaning on SIGCHLD
				continue;
			}
			if (handler->sa_handler == SIG_DFL) {
				// default signal behaviour
				switch (sig) {
					case SIGCHLD:
					case SIGWINCH:
					case SIGTSTP:
					case SIGTTIN:
					case SIGTTOU:
					case SIGCONT:
						continue;

					case SIGSTOP:
						thread->next_state = B_THREAD_SUSPENDED;
						global_resched = 1;
						continue;

					case SIGQUIT:
					case SIGILL:
					case SIGTRAP:
					case SIGABRT:
					case SIGFPE:
					case SIGSEGV:
						dprintf("Shutting down thread 0x%lx due to signal #%d\n", thread->id, sig);
					case SIGKILL:
					case SIGKILLTHR:
					default:
						if (!(thread->return_flags & THREAD_RETURN_EXIT))
							thread->return_flags |= THREAD_RETURN_INTERRUPTED;
						RELEASE_THREAD_LOCK();
						restore_interrupts(state);
						
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

			// ToDo: it's not safe to call arch_setup_signal_frame with
			//		interrupts disabled since it writes to the user stack
			//		and may page fault.

			// User defined signal handler
			dprintf("### Setting up custom signal handler frame...\n");
			arch_setup_signal_frame(thread, handler, sig, thread->sig_block_mask);

			if (handler->sa_flags & SA_ONESHOT)
				handler->sa_handler = SIG_DFL;
			if (!(handler->sa_flags & SA_NOMASK))
				thread->sig_block_mask |= (handler->sa_mask | (1L << (sig - 1))) & BLOCKABLE_SIGS;

			return global_resched;
		} else
			signalMask >>= 1;
	}
	arch_check_syscall_restart(thread);

	return global_resched;
}


int
send_signal_etc(pid_t threadID, uint signal, uint32 flags)
{
	struct thread *thread;
	cpu_status state;

	if (signal < 1 || signal > MAX_SIGNO)
		return B_BAD_VALUE;

	state = disable_interrupts();
	GRAB_THREAD_LOCK();

	thread = thread_get_thread_struct_locked(threadID);
	if (!thread) {
		RELEASE_THREAD_LOCK();
		restore_interrupts(state);
		return B_BAD_THREAD_ID;
	}
	// XXX check permission

	if (thread->team == team_get_kernel_team()) {
		// Signals to kernel threads will only wake them up
		if (thread->state == B_THREAD_SUSPENDED) {
			thread->state = thread->next_state = B_THREAD_READY;
			scheduler_enqueue_in_run_queue(thread);
		}
	} else {
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
	}
	
	if (!(flags & B_DO_NOT_RESCHEDULE))
		scheduler_reschedule();

	RELEASE_THREAD_LOCK();
	restore_interrupts(state);

	return B_OK;
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
sigaction(int signal, const struct sigaction *act, struct sigaction *oact)
{
	struct thread *thread;
	cpu_status state;

	if (signal < 1 || signal > MAX_SIGNO
		|| signal == SIGKILL || signal == SIGKILLTHR || signal == SIGSTOP)
		return EINVAL;

	state = disable_interrupts();
	GRAB_THREAD_LOCK();

	thread = thread_get_current_thread();

	if (oact)
		memcpy(oact, &thread->sig_action[signal - 1], sizeof(struct sigaction));
	if (act)
		memcpy(&thread->sig_action[signal - 1], act, sizeof(struct sigaction));

	if (act && act->sa_handler == SIG_IGN)
		thread->sig_pending &= ~SIGNAL_TO_MASK(signal);
	else if (act && act->sa_handler == SIG_DFL) {
		if (signal == SIGCONT || signal == SIGCHLD || signal == SIGWINCH)
			thread->sig_pending &= ~SIGNAL_TO_MASK(signal);
	} /*else
		dprintf("### custom signal handler set\n");*/

	RELEASE_THREAD_LOCK();
	restore_interrupts(state);

	return 0;
}


// Triggers a SIGALRM to the thread that issued the timer and reschedules

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

	dprintf("alarm_event: thread = %p\n", thread);
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

	dprintf("set_alarm: thread = %p\n", thread);

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
	return send_signal(team, signal);
}


int
_user_sigaction(int sig, const struct sigaction *userAction, struct sigaction *userOldAction)
{
	struct sigaction act, oact;
	int rc;

	if ((userAction != NULL && user_memcpy(&act, userAction, sizeof(struct sigaction)) < B_OK)
		|| (userOldAction != NULL && user_memcpy(&oact, userOldAction, sizeof(struct sigaction)) < B_OK))
		return B_BAD_ADDRESS;

	rc = sigaction(sig, userAction ? &act : NULL, userOldAction ? &oact : NULL);

	// only copy the old action if a pointer has been given
	if (rc >= 0 && userOldAction != NULL
		&& user_memcpy(userOldAction, &oact, sizeof(struct sigaction)) < B_OK)
		return B_BAD_ADDRESS;

	return rc;
}

