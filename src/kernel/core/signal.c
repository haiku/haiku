/* POSIX signals handling routines
** 
** Copyright 2002, Angelo Mottola, a.mottola@libero.it. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

#include <OS.h>
#include <KernelExport.h>
#include <kernel.h>
#include <debug.h>
#include <thread.h>
#include <arch/thread.h>
#include <int.h>
#include <sem.h>
#include <string.h>
#include <signal.h>
#include <syscalls.h>



const char * const sigstr[NSIG] = {
	"NONE", "HUP", "INT", "QUIT", "ILL", "CHLD", "ABRT", "PIPE",
	"FPE", "KILL", "STOP", "SEGV", "CONT", "TSTP", "ALRM", "TERM",
	"TTIN", "TTOU", "USR1", "USR2", "WINCH", "KILLTHR", "TRAP"
};


// Expects interrupts off and thread lock held.
int
handle_signals(struct thread *t, int state)
{
	uint32 sig_mask = t->sig_pending & (~t->sig_block_mask);
	int i, sig, global_resched = 0;
	struct sigaction *handler;
	
	if (sig_mask) {
		for (i = 0; i < NSIG; i++) {
			if (sig_mask & 0x1) {
				
				sig = i + 1;
				handler = &t->sig_action[i];
				sig_mask >>= 1;
				t->sig_pending &= ~(1L << i);
				
				dprintf("Thread 0x%lx received signal %s\n", t->id, sigstr[sig]);
				
				if (handler->sa_handler == SIG_IGN) {
					// signal is to be ignored
					// XXX apply zombie cleaning on SIGCHLD
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
							t->next_state = B_THREAD_SUSPENDED;
							global_resched = 1;
							continue;
													
						case SIGQUIT:
						case SIGILL:
						case SIGTRAP:
						case SIGABRT:
						case SIGFPE:
						case SIGSEGV:
							dprintf("Shutting down thread 0x%lx due to signal #%d\n", t->id, sig);
						case SIGKILL:
						case SIGKILLTHR:
						default:
							if (!(t->return_flags & THREAD_RETURN_EXIT))
								t->return_flags |= THREAD_RETURN_INTERRUPTED;
							RELEASE_THREAD_LOCK();
							restore_interrupts(state);
							thread_exit();
					}
				}
				
				// User defined signal handler
				dprintf("### Setting up custom signal handler frame...\n");
				arch_setup_signal_frame(t, handler, sig, t->sig_block_mask);
				
				if (handler->sa_flags & SA_ONESHOT)
					handler->sa_handler = SIG_DFL;
				if (!(handler->sa_flags & SA_NOMASK))
					t->sig_block_mask |= (handler->sa_mask | (1L << sig)) & BLOCKABLE_SIGS;
				
				return global_resched;
			} else
				sig_mask >>= 1;
		}
		arch_check_syscall_restart(t);
	}
	return global_resched;
}


int
send_signal_etc(pid_t tid, uint sig, uint32 flags)
{
	int state;
	struct thread *t, *main_t;
	
	if ((sig < 1) || (sig > MAX_SIGNO))
		return B_BAD_VALUE;
	
	state = disable_interrupts();
	GRAB_THREAD_LOCK();
	
	t = thread_get_thread_struct_locked(tid);
	if (!t) {
		RELEASE_THREAD_LOCK();
		restore_interrupts(state);
		return B_BAD_THREAD_ID;
	}
	// XXX check permission
	
	// Signals to kernel threads will only wake them up
	if (t->team == team_get_kernel_team()) {
		if (t->state == B_THREAD_SUSPENDED) {
			t->state = t->next_state = B_THREAD_READY;
			thread_enqueue_run_q(t);
		}
	}
	else {
		t->sig_pending |= (1L << (sig - 1));
		
		switch (sig) {
			case SIGKILL:
				// Forward KILLTHR to the main thread of the team
				main_t = t->team->main_thread;
				main_t->sig_pending |= (1L << (SIGKILLTHR - 1));
				// Wake up main thread
				if (main_t->state == B_THREAD_SUSPENDED) {
					main_t->state = main_t->next_state = B_THREAD_READY;
					thread_enqueue_run_q(main_t);
				} else if (main_t->state == B_THREAD_WAITING)
					sem_interrupt_thread(main_t);
				// Fallthrough
			case SIGKILLTHR:
				// Wake up suspended threads and interrupt waiting ones
				if (t->state == B_THREAD_SUSPENDED) {
					t->state = t->next_state = B_THREAD_READY;
					thread_enqueue_run_q(t);
				} else if (t->state == B_THREAD_WAITING)
					sem_interrupt_thread(t);
				break;
			case SIGCONT:
				// Wake up thread if it was suspended
				if (t->state == B_THREAD_SUSPENDED) {
					t->state = t->next_state = B_THREAD_READY;
					thread_enqueue_run_q(t);
				}
				break;
			default:
				if (t->sig_pending & ((~t->sig_block_mask) | (1L << (SIGCHLD - 1)))) {
					// Interrupt thread if it was waiting
					if (t->state == B_THREAD_WAITING)
						sem_interrupt_thread(t);
				}
				break;
		}
	}
	
	if (!(flags & B_DO_NOT_RESCHEDULE))
		resched(); //XXX really here, while interrupts are disabled?
	
	RELEASE_THREAD_LOCK();
	restore_interrupts(state);
	
	return B_OK;
}


int
sys_send_signal(pid_t tid, uint sig)
{
	return send_signal_etc(tid, sig, 0);
}


int
has_signals_pending(void *thr)
{
	struct thread *t = (struct thread *)thr;
	if (!t)
		t = thread_get_current_thread();
	return (t->sig_pending & ~t->sig_block_mask);
}


int
user_sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
	struct sigaction kact;
	struct sigaction koact;
	int rc;
	
	if ((!act) || (!oact))
		return EINVAL;
	
	rc = user_memcpy(&kact, act, sizeof(struct sigaction));
	if (rc < 0)
		return rc;
	rc = user_memcpy(&koact, oact, sizeof(struct sigaction));
	if (rc < 0)
		return rc;
	rc = sys_sigaction(sig, &kact, &koact);
	if (rc < 0)
		return rc;
	rc = user_memcpy(oact, &koact, sizeof(struct sigaction));
	return rc;
}


int
sys_sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
	struct thread *t;
	int state;
	
	if ((sig < 1) || (sig > MAX_SIGNO))
		return EINVAL;
	if ((sig == SIGKILL) || (sig == SIGKILLTHR) || (sig == SIGSTOP))
		return EINVAL;
	
	state = disable_interrupts();
	GRAB_THREAD_LOCK();
	
	t = thread_get_current_thread();
	
	memcpy(oact, &t->sig_action[sig - 1], sizeof(struct sigaction));
	memcpy(&t->sig_action[sig - 1], act, sizeof(struct sigaction));
	
	if (act->sa_handler == SIG_IGN)
		t->sig_pending &= ~(1L << (sig - 1));
	else if (act->sa_handler == SIG_DFL) {
		if ((sig == SIGCONT) || (sig == SIGCHLD) || (sig == SIGWINCH))
			t->sig_pending &= ~(1L << (sig - 1));
	}
	else
		dprintf("### custom signal handler set\n");
	
	RELEASE_THREAD_LOCK();
	restore_interrupts(state);
	
	return 0;
}


// Triggers a SIGALRM to the thread that issued the timer and reschedules
static int32
alarm_event(timer *t)
{
	send_signal_etc(thread_get_current_thread()->id, SIGALRM, B_DO_NOT_RESCHEDULE);
	
	return B_INVOKE_SCHEDULER;
}


bigtime_t
sys_set_alarm(bigtime_t time, uint32 mode)
{
	struct thread *t = thread_get_current_thread();
	int state;
	bigtime_t rv = 0;
	
	state = disable_interrupts(); //XXX really here? and what about a spinlock?

	
	if (t->alarm.period)
		rv = (bigtime_t)t->alarm.entry.key - system_time();
	cancel_timer(&t->alarm);
	if (time != B_INFINITE_TIMEOUT)
		add_timer(&t->alarm, &alarm_event, time, mode);
	
	restore_interrupts(state);
	
	return rv;
}

