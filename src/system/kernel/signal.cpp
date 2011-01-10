/*
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002, Angelo Mottola, a.mottola@libero.it.
 *
 * Distributed under the terms of the MIT License.
 */


/*! POSIX signals handling routines */


#include <ksignal.h>

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <OS.h>
#include <KernelExport.h>

#include <cpu.h>
#include <debug.h>
#include <kernel.h>
#include <kscheduler.h>
#include <sem.h>
#include <syscall_restart.h>
#include <syscall_utils.h>
#include <team.h>
#include <thread.h>
#include <tracing.h>
#include <user_debugger.h>
#include <user_thread.h>
#include <util/AutoLock.h>


//#define TRACE_SIGNAL
#ifdef TRACE_SIGNAL
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#define BLOCKABLE_SIGNALS		(~(KILL_SIGNALS | SIGNAL_TO_MASK(SIGSTOP)))
#define STOP_SIGNALS \
	(SIGNAL_TO_MASK(SIGSTOP) | SIGNAL_TO_MASK(SIGTSTP) \
	| SIGNAL_TO_MASK(SIGTTIN) | SIGNAL_TO_MASK(SIGTTOU))
#define DEFAULT_IGNORE_SIGNALS \
	(SIGNAL_TO_MASK(SIGCHLD) | SIGNAL_TO_MASK(SIGWINCH) \
	| SIGNAL_TO_MASK(SIGCONT))
#define NON_DEFERRABLE_SIGNALS	\
	(KILL_SIGNALS				\
	| SIGNAL_TO_MASK(SIGILL)	\
	| SIGNAL_TO_MASK(SIGFPE)	\
	| SIGNAL_TO_MASK(SIGSEGV))


const char * const sigstr[NSIG] = {
	"NONE", "HUP", "INT", "QUIT", "ILL", "CHLD", "ABRT", "PIPE",
	"FPE", "KILL", "STOP", "SEGV", "CONT", "TSTP", "ALRM", "TERM",
	"TTIN", "TTOU", "USR1", "USR2", "WINCH", "KILLTHR", "TRAP",
	"POLL", "PROF", "SYS", "URG", "VTALRM", "XCPU", "XFSZ"
};


static status_t deliver_signal(Thread *thread, uint signal, uint32 flags);



// #pragma mark - signal tracing


#if SIGNAL_TRACING

namespace SignalTracing {


class HandleSignals : public AbstractTraceEntry {
	public:
		HandleSignals(uint32 signals)
			:
			fSignals(signals)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("signal handle:  0x%lx", fSignals);
		}

	private:
		uint32		fSignals;
};


class ExecuteSignalHandler : public AbstractTraceEntry {
	public:
		ExecuteSignalHandler(int signal, struct sigaction* handler)
			:
			fSignal(signal),
			fHandler((void*)handler->sa_handler)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("signal exec handler: signal: %d, handler: %p",
				fSignal, fHandler);
		}

	private:
		int		fSignal;
		void*	fHandler;
};


class SendSignal : public AbstractTraceEntry {
	public:
		SendSignal(pid_t target, uint32 signal, uint32 flags)
			:
			fTarget(target),
			fSignal(signal),
			fFlags(flags)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("signal send: target: %ld, signal: %lu (%s), "
				"flags: 0x%lx", fTarget, fSignal,
				(fSignal < NSIG ? sigstr[fSignal] : "invalid"), fFlags);
		}

	private:
		pid_t	fTarget;
		uint32	fSignal;
		uint32	fFlags;
};


class SigAction : public AbstractTraceEntry {
	public:
		SigAction(Thread* thread, uint32 signal, const struct sigaction* act)
			:
			fThread(thread->id),
			fSignal(signal),
			fAction(*act)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("signal action: thread: %ld, signal: %lu (%s), "
				"action: {handler: %p, flags: 0x%x, mask: 0x%lx}",
				fThread, fSignal,
				(fSignal < NSIG ? sigstr[fSignal] : "invalid"),
				fAction.sa_handler, fAction.sa_flags, fAction.sa_mask);
		}

	private:
		thread_id			fThread;
		uint32				fSignal;
		struct sigaction	fAction;
};


class SigProcMask : public AbstractTraceEntry {
	public:
		SigProcMask(int how, sigset_t mask)
			:
			fHow(how),
			fMask(mask),
			fOldMask(thread_get_current_thread()->sig_block_mask)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			const char* how = "invalid";
			switch (fHow) {
				case SIG_BLOCK:
					how = "block";
					break;
				case SIG_UNBLOCK:
					how = "unblock";
					break;
				case SIG_SETMASK:
					how = "set";
					break;
			}

			out.Print("signal proc mask: %s 0x%lx, old mask: 0x%lx", how, fMask,
				fOldMask);
		}

	private:
		int			fHow;
		sigset_t	fMask;
		sigset_t	fOldMask;
};


class SigSuspend : public AbstractTraceEntry {
	public:
		SigSuspend(sigset_t mask)
			:
			fMask(mask),
			fOldMask(thread_get_current_thread()->sig_block_mask)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("signal suspend: %#" B_PRIx32 ", old mask: %#" B_PRIx32,
				fMask, fOldMask);
		}

	private:
		sigset_t	fMask;
		sigset_t	fOldMask;
};


class SigSuspendDone : public AbstractTraceEntry {
	public:
		SigSuspendDone()
			:
			fSignals(thread_get_current_thread()->sig_pending)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("signal suspend done: %#" B_PRIx32, fSignals);
		}

	private:
		uint32		fSignals;
};

}	// namespace SignalTracing

#	define T(x)	new(std::nothrow) SignalTracing::x

#else
#	define T(x)
#endif	// SIGNAL_TRACING


// #pragma mark -


/*!	Updates the thread::flags field according to what signals are pending.
	Interrupts must be disabled and the thread lock must be held.
*/
static void
update_thread_signals_flag(Thread* thread)
{
	sigset_t mask = ~atomic_get(&thread->sig_block_mask)
		| thread->sig_temp_enabled;
	if (atomic_get(&thread->sig_pending) & mask)
		atomic_or(&thread->flags, THREAD_FLAGS_SIGNALS_PENDING);
	else
		atomic_and(&thread->flags, ~THREAD_FLAGS_SIGNALS_PENDING);
}


void
update_current_thread_signals_flag()
{
	InterruptsSpinLocker locker(gThreadSpinlock);

	update_thread_signals_flag(thread_get_current_thread());
}


static bool
notify_debugger(Thread *thread, int signal, struct sigaction *handler,
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


/*! Actually handles the signal - ie. the thread will exit, a custom signal
	handler is prepared, or whatever the signal demands.
*/
bool
handle_signals(Thread *thread)
{
	uint32 signalMask = atomic_get(&thread->sig_pending)
		& (~atomic_get(&thread->sig_block_mask) | thread->sig_temp_enabled);
	thread->sig_temp_enabled = 0;

	// If SIGKILL[THR] are pending, we ignore other signals.
	// Otherwise check, if the thread shall stop for debugging.
	if (signalMask & KILL_SIGNALS) {
		signalMask &= KILL_SIGNALS;
	} else if (thread->debug_info.flags & B_THREAD_DEBUG_STOP) {
		user_debug_stop_thread();
	}

	if (signalMask == 0)
		return 0;

	if (thread->user_thread->defer_signals > 0
		&& (signalMask & NON_DEFERRABLE_SIGNALS) == 0) {
		thread->user_thread->pending_signals = signalMask;
		return 0;
	}

	thread->user_thread->pending_signals = 0;

	uint32 restartFlags = atomic_and(&thread->flags,
		~THREAD_FLAGS_DONT_RESTART_SYSCALL);
	bool alwaysRestart
		= (restartFlags & THREAD_FLAGS_ALWAYS_RESTART_SYSCALL) != 0;
	bool restart = alwaysRestart
		|| (restartFlags & THREAD_FLAGS_DONT_RESTART_SYSCALL) == 0;

	T(HandleSignals(signalMask));

	for (int32 i = 0; i < NSIG; i++) {
		bool debugSignal;
		int32 signal = i + 1;

		if ((signalMask & SIGNAL_TO_MASK(signal)) == 0)
			continue;

		// clear the signal that we will handle
		atomic_and(&thread->sig_pending, ~SIGNAL_TO_MASK(signal));

		debugSignal = !(~atomic_get(&thread->team->debug_info.flags)
				& (B_TEAM_DEBUG_SIGNALS | B_TEAM_DEBUG_DEBUGGER_INSTALLED));

		// TODO: since sigaction_etc() could clobber the fields at any time,
		//		we should actually copy the relevant fields atomically before
		//		accessing them (only the debugger is calling sigaction_etc()
		//		right now).
		//		Update: sigaction_etc() is only used by the userland debugger
		//		support. We can just as well restrict getting/setting signal
		//		handlers to work only when the respective thread is stopped.
		//		Then sigaction() could be used instead and we could get rid of
		//		sigaction_etc().
		struct sigaction* handler = &thread->sig_action[i];

		TRACE(("Thread 0x%lx received signal %s\n", thread->id, sigstr[signal]));

		if (handler->sa_handler == SIG_IGN) {
			// signal is to be ignored
			// ToDo: apply zombie cleaning on SIGCHLD

			// notify the debugger
			if (debugSignal)
				notify_debugger(thread, signal, handler, false);
			continue;
		} else if (handler->sa_handler == SIG_DFL) {
			// default signal behaviour
			switch (signal) {
				case SIGCHLD:
				case SIGWINCH:
				case SIGURG:
					// notify the debugger
					if (debugSignal)
						notify_debugger(thread, signal, handler, false);
					continue;

				case SIGCONT:
					// notify the debugger
					if (debugSignal
						&& !notify_debugger(thread, signal, handler, false))
						continue;

					// notify threads waiting for team state changes
					if (thread == thread->team->main_thread) {
						InterruptsSpinLocker locker(gTeamSpinlock);
						team_set_job_control_state(thread->team,
							JOB_CONTROL_STATE_CONTINUED, signal, false);

						// The standard states that the system *may* send a
						// SIGCHLD when a child is continued. I haven't found
						// a good reason why we would want to, though.
					}
					continue;

				case SIGSTOP:
				case SIGTSTP:
				case SIGTTIN:
				case SIGTTOU:
					// notify the debugger
					if (debugSignal
						&& !notify_debugger(thread, signal, handler, false))
						continue;

					thread->next_state = B_THREAD_SUSPENDED;

					// notify threads waiting for team state changes
					if (thread == thread->team->main_thread) {
						InterruptsSpinLocker locker(gTeamSpinlock);
						team_set_job_control_state(thread->team,
							JOB_CONTROL_STATE_STOPPED, signal, false);

						// send a SIGCHLD to the parent (if it does have
						// SA_NOCLDSTOP defined)
						SpinLocker _(gThreadSpinlock);
						Thread* parentThread
							= thread->team->parent->main_thread;
						struct sigaction& parentHandler
							= parentThread->sig_action[SIGCHLD - 1];
						if ((parentHandler.sa_flags & SA_NOCLDSTOP) == 0)
							deliver_signal(parentThread, SIGCHLD, 0);
					}

					return true;

				case SIGSEGV:
				case SIGFPE:
				case SIGILL:
				case SIGTRAP:
				case SIGABRT:
					// If this is the main thread, we just fall through and let
					// this signal kill the team. Otherwise we send a SIGKILL to
					// the main thread first, since the signal will kill this
					// thread only.
					if (thread != thread->team->main_thread)
						send_signal(thread->team->main_thread->id, SIGKILL);
				case SIGQUIT:
				case SIGPOLL:
				case SIGPROF:
				case SIGSYS:
				case SIGVTALRM:
				case SIGXCPU:
				case SIGXFSZ:
					TRACE(("Shutting down thread 0x%lx due to signal #%ld\n",
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

		// User defined signal handler

		// notify the debugger
		if (debugSignal && !notify_debugger(thread, signal, handler, false))
			continue;

		if (!restart
				|| ((!alwaysRestart && handler->sa_flags & SA_RESTART) == 0)) {
			atomic_and(&thread->flags, ~THREAD_FLAGS_RESTART_SYSCALL);
		}

		T(ExecuteSignalHandler(signal, handler));

		TRACE(("### Setting up custom signal handler frame...\n"));
		arch_setup_signal_frame(thread, handler, signal,
			atomic_get(&thread->sig_block_mask));

		if (handler->sa_flags & SA_ONESHOT)
			handler->sa_handler = SIG_DFL;
		if ((handler->sa_flags & SA_NOMASK) == 0) {
			// Update the block mask while the signal handler is running - it
			// will be automatically restored when the signal frame is left.
			atomic_or(&thread->sig_block_mask,
				(handler->sa_mask | SIGNAL_TO_MASK(signal)) & BLOCKABLE_SIGNALS);
		}

		update_current_thread_signals_flag();

		return false;
	}

	// clear syscall restart thread flag, if we're not supposed to restart the
	// syscall
	if (!restart)
		atomic_and(&thread->flags, ~THREAD_FLAGS_RESTART_SYSCALL);

	update_current_thread_signals_flag();

	return false;
}


bool
is_kill_signal_pending(void)
{
	return (atomic_get(&thread_get_current_thread()->sig_pending)
		& KILL_SIGNALS) != 0;
}


bool
is_signal_blocked(int signal)
{
	return (atomic_get(&thread_get_current_thread()->sig_block_mask)
		& SIGNAL_TO_MASK(signal)) != 0;
}


/*!	Delivers the \a signal to the \a thread, but doesn't handle the signal -
	it just makes sure the thread gets the signal, ie. unblocks it if needed.
	This function must be called with interrupts disabled and the
	thread lock held.
*/
static status_t
deliver_signal(Thread *thread, uint signal, uint32 flags)
{
	if (flags & B_CHECK_PERMISSION) {
		// ToDo: introduce euid & uid fields to the team and check permission
	}

	if (signal == 0)
		return B_OK;

	if (thread->team == team_get_kernel_team()) {
		// Signals to kernel threads will only wake them up
		if (thread->state == B_THREAD_SUSPENDED)
			scheduler_enqueue_in_run_queue(thread);
		return B_OK;
	}

	atomic_or(&thread->sig_pending, SIGNAL_TO_MASK(signal));

	switch (signal) {
		case SIGKILL:
		{
			// Forward KILLTHR to the main thread of the team
			Thread *mainThread = thread->team->main_thread;
			atomic_or(&mainThread->sig_pending, SIGNAL_TO_MASK(SIGKILLTHR));

			// Wake up main thread
			if (mainThread->state == B_THREAD_SUSPENDED)
				scheduler_enqueue_in_run_queue(mainThread);
			else
				thread_interrupt(mainThread, true);

			update_thread_signals_flag(mainThread);

			// Supposed to fall through
		}
		case SIGKILLTHR:
			// Wake up suspended threads and interrupt waiting ones
			if (thread->state == B_THREAD_SUSPENDED)
				scheduler_enqueue_in_run_queue(thread);
			else
				thread_interrupt(thread, true);
			break;

		case SIGCONT:
			// Wake up thread if it was suspended
			if (thread->state == B_THREAD_SUSPENDED)
				scheduler_enqueue_in_run_queue(thread);

			if ((flags & SIGNAL_FLAG_DONT_RESTART_SYSCALL) != 0)
				atomic_or(&thread->flags, THREAD_FLAGS_DONT_RESTART_SYSCALL);

			atomic_and(&thread->sig_pending, ~STOP_SIGNALS);
				// remove any pending stop signals
			break;

		default:
			if (thread->sig_pending
				& (~thread->sig_block_mask | SIGNAL_TO_MASK(SIGCHLD))) {
				// Interrupt thread if it was waiting
				thread_interrupt(thread, false);
			}
			break;
	}

	update_thread_signals_flag(thread);

	return B_OK;
}


int
send_signal_etc(pid_t id, uint signal, uint32 flags)
{
	status_t status = B_BAD_THREAD_ID;
	Thread *thread;
	cpu_status state = 0;

	if (signal < 0 || signal > MAX_SIGNO)
		return B_BAD_VALUE;

	T(SendSignal(id, signal, flags));

	if ((flags & SIGNAL_FLAG_TEAMS_LOCKED) == 0)
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

		struct process_group *group;

		// TODO: handle -1 correctly
		if (id == 0 || id == -1) {
			// send a signal to the current team
			id = thread_get_current_thread()->team->id;
		} else
			id = -id;

		if ((flags & SIGNAL_FLAG_TEAMS_LOCKED) == 0)
			GRAB_TEAM_LOCK();

		group = team_get_process_group_locked(NULL, id);
		if (group != NULL) {
			Team *team, *next;

			// Send a signal to all teams in this process group

			for (team = group->teams; team != NULL; team = next) {
				next = team->group_next;
				id = team->id;

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

		if ((flags & SIGNAL_FLAG_TEAMS_LOCKED) == 0)
			RELEASE_TEAM_LOCK();

		GRAB_THREAD_LOCK();
	}

	if ((flags & (B_DO_NOT_RESCHEDULE | SIGNAL_FLAG_TEAMS_LOCKED)) == 0)
		scheduler_reschedule_if_necessary_locked();

	RELEASE_THREAD_LOCK();

	if ((flags & SIGNAL_FLAG_TEAMS_LOCKED) == 0)
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
	Thread *thread = (Thread *)_thread;
	if (thread == NULL)
		thread = thread_get_current_thread();

	return atomic_get(&thread->sig_pending)
		& ~atomic_get(&thread->sig_block_mask);
}


static int
sigprocmask_internal(int how, const sigset_t *set, sigset_t *oldSet)
{
	Thread *thread = thread_get_current_thread();
	sigset_t oldMask = atomic_get(&thread->sig_block_mask);

	if (set != NULL) {
		T(SigProcMask(how, *set));

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

		update_current_thread_signals_flag();
	}

	if (oldSet != NULL)
		*oldSet = oldMask;

	return B_OK;
}


int
sigprocmask(int how, const sigset_t *set, sigset_t *oldSet)
{
	RETURN_AND_SET_ERRNO(sigprocmask_internal(how, set, oldSet));
}


/*!	\brief sigaction() for the specified thread.
	A \a threadID is < 0 specifies the current thread.
*/
static status_t
sigaction_etc_internal(thread_id threadID, int signal, const struct sigaction *act,
	struct sigaction *oldAction)
{
	Thread *thread;
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
			T(SigAction(thread, signal, act));

			// set new sigaction structure
			memcpy(&thread->sig_action[signal - 1], act,
				sizeof(struct sigaction));
			thread->sig_action[signal - 1].sa_mask &= BLOCKABLE_SIGNALS;
		}

		if (act && act->sa_handler == SIG_IGN) {
			// remove pending signal if it should now be ignored
			atomic_and(&thread->sig_pending, ~SIGNAL_TO_MASK(signal));
		} else if (act && act->sa_handler == SIG_DFL
			&& (SIGNAL_TO_MASK(signal) & DEFAULT_IGNORE_SIGNALS) != 0) {
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
sigaction_etc(thread_id threadID, int signal, const struct sigaction *act,
	struct sigaction *oldAction)
{
	RETURN_AND_SET_ERRNO(sigaction_etc_internal(threadID, signal, act,
		oldAction));
}


int
sigaction(int signal, const struct sigaction *act, struct sigaction *oldAction)
{
	return sigaction_etc(-1, signal, act, oldAction);
}


/*! Triggers a SIGALRM to the thread that issued the timer and reschedules */
static int32
alarm_event(timer *t)
{
	// The hook can be called from any context, but we have to
	// deliver the signal to the thread that originally called
	// set_alarm().
	// Since thread->alarm is this timer structure, we can just
	// cast it back - ugly but it works for now
	Thread *thread = (Thread *)((uint8 *)t - offsetof(Thread, alarm));
		// ToDo: investigate adding one user parameter to the timer structure to fix this hack

	TRACE(("alarm_event: thread = %p\n", thread));
	send_signal_etc(thread->id, SIGALRM, B_DO_NOT_RESCHEDULE);

	return B_HANDLED_INTERRUPT;
}


/*!	Sets the alarm timer for the current thread. The timer fires at the
	specified time in the future, periodically or just once, as determined
	by \a mode.
	\return the time left until a previous set alarm would have fired.
*/
bigtime_t
set_alarm(bigtime_t time, uint32 mode)
{
	Thread *thread = thread_get_current_thread();
	bigtime_t remainingTime = 0;

	ASSERT(B_ONE_SHOT_RELATIVE_ALARM == B_ONE_SHOT_RELATIVE_TIMER);
		// just to be sure no one changes the headers some day

	TRACE(("set_alarm: thread = %p\n", thread));

	if (thread->alarm.period)
		remainingTime = (bigtime_t)thread->alarm.schedule_time - system_time();

	cancel_timer(&thread->alarm);

	if (time != B_INFINITE_TIMEOUT)
		add_timer(&thread->alarm, &alarm_event, time, mode);
	else {
		// this marks the alarm as canceled (for returning the remaining time)
		thread->alarm.period = 0;
	}

	return remainingTime;
}


/*!	Wait for the specified signals, and return the signal retrieved in
	\a _signal.
*/
static status_t
sigwait_internal(const sigset_t *set, int *_signal)
{
	sigset_t requestedSignals = *set & BLOCKABLE_SIGNALS;

	Thread* thread = thread_get_current_thread();

	while (true) {
		sigset_t pendingSignals = atomic_get(&thread->sig_pending);
		sigset_t blockedSignals = atomic_get(&thread->sig_block_mask);
		sigset_t pendingRequestedSignals = pendingSignals & requestedSignals;
		if ((pendingRequestedSignals) != 0) {
			// select the lowest pending signal to return in _signal
			for (int signal = 1; signal < NSIG; signal++) {
				if ((SIGNAL_TO_MASK(signal) & pendingSignals) != 0) {
					atomic_and(&thread->sig_pending, ~SIGNAL_TO_MASK(signal));
					*_signal = signal;
					return B_OK;
				}
			}
		}

		if ((pendingSignals & ~blockedSignals) != 0) {
			// Non-blocked signals are pending -- return to let them be handled.
			return B_INTERRUPTED;
		}

		// No signals yet. Set the signal block mask to not include the
		// requested mask and wait until we're interrupted.
		atomic_set(&thread->sig_block_mask,
			blockedSignals & ~(requestedSignals & BLOCKABLE_SIGNALS));

		while (!has_signals_pending(thread)) {
			thread_prepare_to_block(thread, B_CAN_INTERRUPT,
				THREAD_BLOCK_TYPE_SIGNAL, NULL);
			thread_block();
		}

		// restore the original block mask
		atomic_set(&thread->sig_block_mask, blockedSignals);

		update_current_thread_signals_flag();
	}
}


int
sigwait(const sigset_t *set, int *_signal)
{
	RETURN_AND_SET_ERRNO(sigwait_internal(set, _signal));
}


/*!	Replace the current signal block mask and wait for any event to happen.
	Before returning, the original signal block mask is reinstantiated.
*/
static status_t
sigsuspend_internal(const sigset_t *mask)
{
	T(SigSuspend(*mask));

	Thread *thread = thread_get_current_thread();
	sigset_t oldMask = atomic_get(&thread->sig_block_mask);

	// Set the new block mask and block until interrupted.

	atomic_set(&thread->sig_block_mask, *mask & BLOCKABLE_SIGNALS);

	while (!has_signals_pending(thread)) {
		thread_prepare_to_block(thread, B_CAN_INTERRUPT,
			THREAD_BLOCK_TYPE_SIGNAL, NULL);
		thread_block();
	}

	// restore the original block mask
	atomic_set(&thread->sig_block_mask, oldMask);

	thread->sig_temp_enabled = ~*mask;

	update_current_thread_signals_flag();

	T(SigSuspendDone());

	// we're not supposed to actually succeed
	return B_INTERRUPTED;
}


int
sigsuspend(const sigset_t *mask)
{
	RETURN_AND_SET_ERRNO(sigsuspend_internal(mask));
}


static status_t
sigpending_internal(sigset_t *set)
{
	Thread *thread = thread_get_current_thread();

	if (set == NULL)
		return B_BAD_VALUE;

	*set = atomic_get(&thread->sig_pending);
	return B_OK;
}


int
sigpending(sigset_t *set)
{
	RETURN_AND_SET_ERRNO(sigpending_internal(set));
}


// #pragma mark - syscalls


bigtime_t
_user_set_alarm(bigtime_t time, uint32 mode)
{
	syscall_64_bit_return_value();

	return set_alarm(time, mode);
}


status_t
_user_send_signal(pid_t team, uint signal)
{
	return send_signal_etc(team, signal, B_CHECK_PERMISSION);
}


status_t
_user_sigprocmask(int how, const sigset_t *userSet, sigset_t *userOldSet)
{
	sigset_t set, oldSet;
	status_t status;

	if ((userSet != NULL && user_memcpy(&set, userSet, sizeof(sigset_t)) < B_OK)
		|| (userOldSet != NULL && user_memcpy(&oldSet, userOldSet,
				sizeof(sigset_t)) < B_OK))
		return B_BAD_ADDRESS;

	status = sigprocmask_internal(how, userSet ? &set : NULL,
		userOldSet ? &oldSet : NULL);

	// copy old set if asked for
	if (status >= B_OK && userOldSet != NULL
		&& user_memcpy(userOldSet, &oldSet, sizeof(sigset_t)) < B_OK)
		return B_BAD_ADDRESS;

	return status;
}


status_t
_user_sigaction(int signal, const struct sigaction *userAction,
	struct sigaction *userOldAction)
{
	struct sigaction act, oact;
	status_t status;

	if ((userAction != NULL && user_memcpy(&act, userAction,
				sizeof(struct sigaction)) < B_OK)
		|| (userOldAction != NULL && user_memcpy(&oact, userOldAction,
				sizeof(struct sigaction)) < B_OK))
		return B_BAD_ADDRESS;

	status = sigaction(signal, userAction ? &act : NULL,
		userOldAction ? &oact : NULL);

	// only copy the old action if a pointer has been given
	if (status >= B_OK && userOldAction != NULL
		&& user_memcpy(userOldAction, &oact, sizeof(struct sigaction)) < B_OK)
		return B_BAD_ADDRESS;

	return status;
}


status_t
_user_sigwait(const sigset_t *userSet, int *_userSignal)
{
	if (userSet == NULL || _userSignal == NULL)
		return B_BAD_VALUE;

	sigset_t set;
	if (user_memcpy(&set, userSet, sizeof(sigset_t)) < B_OK)
		return B_BAD_ADDRESS;

	int signal;
	status_t status = sigwait_internal(&set, &signal);
	if (status == B_INTERRUPTED) {
		// make sure we'll be restarted
		Thread* thread = thread_get_current_thread();
		atomic_or(&thread->flags,
			THREAD_FLAGS_ALWAYS_RESTART_SYSCALL | THREAD_FLAGS_RESTART_SYSCALL);
		return status;
	}

	return user_memcpy(_userSignal, &signal, sizeof(int));
}


status_t
_user_sigsuspend(const sigset_t *userMask)
{
	sigset_t mask;

	if (userMask == NULL)
		return B_BAD_VALUE;
	if (user_memcpy(&mask, userMask, sizeof(sigset_t)) < B_OK)
		return B_BAD_ADDRESS;

	return sigsuspend_internal(&mask);
}


status_t
_user_sigpending(sigset_t *userSet)
{
	sigset_t set;
	int status;

	if (userSet == NULL)
		return B_BAD_VALUE;
	if (!IS_USER_ADDRESS(userSet))
		return B_BAD_ADDRESS;

	status = sigpending_internal(&set);
	if (status == B_OK
		&& user_memcpy(userSet, &set, sizeof(sigset_t)) < B_OK)
		return B_BAD_ADDRESS;

	return status;
}


status_t
_user_set_signal_stack(const stack_t *newUserStack, stack_t *oldUserStack)
{
	Thread *thread = thread_get_current_thread();
	struct stack_t newStack, oldStack;
	bool onStack = false;

	if ((newUserStack != NULL && user_memcpy(&newStack, newUserStack,
				sizeof(stack_t)) < B_OK)
		|| (oldUserStack != NULL && user_memcpy(&oldStack, oldUserStack,
				sizeof(stack_t)) < B_OK))
		return B_BAD_ADDRESS;

	if (thread->signal_stack_enabled) {
		// determine wether or not the user thread is currently
		// on the active signal stack
		onStack = arch_on_signal_stack(thread);
	}

	if (oldUserStack != NULL) {
		oldStack.ss_sp = (void *)thread->signal_stack_base;
		oldStack.ss_size = thread->signal_stack_size;
		oldStack.ss_flags = (thread->signal_stack_enabled ? 0 : SS_DISABLE)
			| (onStack ? SS_ONSTACK : 0);
	}

	if (newUserStack != NULL) {
		// no flags other than SS_DISABLE are allowed
		if ((newStack.ss_flags & ~SS_DISABLE) != 0)
			return B_BAD_VALUE;

		if ((newStack.ss_flags & SS_DISABLE) == 0) {
			// check if the size is valid
			if (newStack.ss_size < MINSIGSTKSZ)
				return B_NO_MEMORY;
			if (onStack)
				return B_NOT_ALLOWED;
			if (!IS_USER_ADDRESS(newStack.ss_sp))
				return B_BAD_VALUE;

			thread->signal_stack_base = (addr_t)newStack.ss_sp;
			thread->signal_stack_size = newStack.ss_size;
			thread->signal_stack_enabled = true;
		} else
			thread->signal_stack_enabled = false;
	}

	// only copy the old stack info if a pointer has been given
	if (oldUserStack != NULL
		&& user_memcpy(oldUserStack, &oldStack, sizeof(stack_t)) < B_OK)
		return B_BAD_ADDRESS;

	return B_OK;
}

