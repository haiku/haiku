/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
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


#define BLOCKABLE_SIGNALS	\
	(~(KILL_SIGNALS | SIGNAL_TO_MASK(SIGSTOP)	\
	| SIGNAL_TO_MASK(SIGNAL_CONTINUE_THREAD)	\
	| SIGNAL_TO_MASK(SIGNAL_CANCEL_THREAD)))
#define STOP_SIGNALS \
	(SIGNAL_TO_MASK(SIGSTOP) | SIGNAL_TO_MASK(SIGTSTP) \
	| SIGNAL_TO_MASK(SIGTTIN) | SIGNAL_TO_MASK(SIGTTOU))
#define CONTINUE_SIGNALS \
	(SIGNAL_TO_MASK(SIGCONT) | SIGNAL_TO_MASK(SIGNAL_CONTINUE_THREAD))
#define DEFAULT_IGNORE_SIGNALS \
	(SIGNAL_TO_MASK(SIGCHLD) | SIGNAL_TO_MASK(SIGWINCH) \
	| SIGNAL_TO_MASK(SIGCONT) \
	| SIGNAL_RANGE_TO_MASK(SIGNAL_REALTIME_MIN, SIGNAL_REALTIME_MAX))
#define NON_DEFERRABLE_SIGNALS	\
	(KILL_SIGNALS				\
	| SIGNAL_TO_MASK(SIGILL)	\
	| SIGNAL_TO_MASK(SIGFPE)	\
	| SIGNAL_TO_MASK(SIGSEGV))


static const struct {
	const char*	name;
	int32		priority;
} kSignalInfos[__MAX_SIGNO + 1] = {
	{"NONE",			-1},
	{"HUP",				0},
	{"INT",				0},
	{"QUIT",			0},
	{"ILL",				0},
	{"CHLD",			0},
	{"ABRT",			0},
	{"PIPE",			0},
	{"FPE",				0},
	{"KILL",			100},
	{"STOP",			0},
	{"SEGV",			0},
	{"CONT",			0},
	{"TSTP",			0},
	{"ALRM",			0},
	{"TERM",			0},
	{"TTIN",			0},
	{"TTOU",			0},
	{"USR1",			0},
	{"USR2",			0},
	{"WINCH",			0},
	{"KILLTHR",			100},
	{"TRAP",			0},
	{"POLL",			0},
	{"PROF",			0},
	{"SYS",				0},
	{"URG",				0},
	{"VTALRM",			0},
	{"XCPU",			0},
	{"XFSZ",			0},
	{"SIGBUS",			0},
	{"SIGRESERVED1",	0},
	{"SIGRESERVED2",	0},
	{"SIGRT1",			8},
	{"SIGRT2",			7},
	{"SIGRT3",			6},
	{"SIGRT4",			5},
	{"SIGRT5",			4},
	{"SIGRT6",			3},
	{"SIGRT7",			2},
	{"SIGRT8",			1},
	{"invalid 41",		0},
	{"invalid 42",		0},
	{"invalid 43",		0},
	{"invalid 44",		0},
	{"invalid 45",		0},
	{"invalid 46",		0},
	{"invalid 47",		0},
	{"invalid 48",		0},
	{"invalid 49",		0},
	{"invalid 50",		0},
	{"invalid 51",		0},
	{"invalid 52",		0},
	{"invalid 53",		0},
	{"invalid 54",		0},
	{"invalid 55",		0},
	{"invalid 56",		0},
	{"invalid 57",		0},
	{"invalid 58",		0},
	{"invalid 59",		0},
	{"invalid 60",		0},
	{"invalid 61",		0},
	{"invalid 62",		0},
	{"CANCEL_THREAD",	0},
	{"CONTINUE_THREAD",	0}	// priority must be <= that of SIGSTOP
};


static inline const char*
signal_name(uint32 number)
{
	return number <= __MAX_SIGNO ? kSignalInfos[number].name : "invalid";
}


// #pragma mark - SignalHandledCaller


struct SignalHandledCaller {
	SignalHandledCaller(Signal* signal)
		:
		fSignal(signal)
	{
	}

	~SignalHandledCaller()
	{
		Done();
	}

	void Done()
	{
		if (fSignal != NULL) {
			fSignal->Handled();
			fSignal = NULL;
		}
	}

private:
	Signal*	fSignal;
};


// #pragma mark - QueuedSignalsCounter


/*!	Creates a counter with the given limit.
	The limit defines the maximum the counter may reach. Since the
	BReferenceable's reference count is used, it is assumed that the owning
	team holds a reference and the reference count is one greater than the
	counter value.
	\param limit The maximum allowed value the counter may have. When
		\code < 0 \endcode, the value is not limited.
*/
QueuedSignalsCounter::QueuedSignalsCounter(int32 limit)
	:
	fLimit(limit)
{
}


/*!	Increments the counter, if the limit allows that.
	\return \c true, if incrementing the counter succeeded, \c false otherwise.
*/
bool
QueuedSignalsCounter::Increment()
{
	// no limit => no problem
	if (fLimit < 0) {
		AcquireReference();
		return true;
	}

	// Increment the reference count manually, so we can check atomically. We
	// compare the old value > fLimit, assuming that our (primary) owner has a
	// reference, we don't want to count.
	if (atomic_add(&fReferenceCount, 1) > fLimit) {
		ReleaseReference();
		return false;
	}

	return true;
}


// #pragma mark - Signal


Signal::Signal()
	:
	fCounter(NULL),
	fPending(false)
{
}


Signal::Signal(const Signal& other)
	:
	fCounter(NULL),
	fNumber(other.fNumber),
	fSignalCode(other.fSignalCode),
	fErrorCode(other.fErrorCode),
	fSendingProcess(other.fSendingProcess),
	fSendingUser(other.fSendingUser),
	fStatus(other.fStatus),
	fPollBand(other.fPollBand),
	fAddress(other.fAddress),
	fUserValue(other.fUserValue),
	fPending(false)
{
}


Signal::Signal(uint32 number, int32 signalCode, int32 errorCode,
	pid_t sendingProcess)
	:
	fCounter(NULL),
	fNumber(number),
	fSignalCode(signalCode),
	fErrorCode(errorCode),
	fSendingProcess(sendingProcess),
	fSendingUser(getuid()),
	fStatus(0),
	fPollBand(0),
	fAddress(NULL),
	fPending(false)
{
	fUserValue.sival_ptr = NULL;
}


Signal::~Signal()
{
	if (fCounter != NULL)
		fCounter->ReleaseReference();
}


/*!	Creates a queuable clone of the given signal.
	Also enforces the current team's signal queuing limit.

	\param signal The signal to clone.
	\param queuingRequired If \c true, the function will return an error code
		when creating the clone fails for any reason. Otherwise, the function
		will set \a _signalToQueue to \c NULL, but still return \c B_OK.
	\param _signalToQueue Return parameter. Set to the clone of the signal.
	\return When \c queuingRequired is \c false, always \c B_OK. Otherwise
		\c B_OK, when creating the signal clone succeeds, another error code,
		when it fails.
*/
/*static*/ status_t
Signal::CreateQueuable(const Signal& signal, bool queuingRequired,
	Signal*& _signalToQueue)
{
	_signalToQueue = NULL;

	// If interrupts are disabled, we can't allocate a signal.
	if (!are_interrupts_enabled())
		return queuingRequired ? B_BAD_VALUE : B_OK;

	// increment the queued signals counter
	QueuedSignalsCounter* counter
		= thread_get_current_thread()->team->QueuedSignalsCounter();
	if (!counter->Increment())
		return queuingRequired ? EAGAIN : B_OK;

	// allocate the signal
	Signal* signalToQueue = new(std::nothrow) Signal(signal);
	if (signalToQueue == NULL) {
		counter->Decrement();
		return queuingRequired ? B_NO_MEMORY : B_OK;
	}

	signalToQueue->fCounter = counter;

	_signalToQueue = signalToQueue;
	return B_OK;
}

void
Signal::SetTo(uint32 number)
{
	Team* team = thread_get_current_thread()->team;

	fNumber = number;
	fSignalCode = SI_USER;
	fErrorCode = 0;
	fSendingProcess = team->id;
	fSendingUser = team->effective_uid;
		// assuming scheduler lock is being held
	fStatus = 0;
	fPollBand = 0;
	fAddress = NULL;
	fUserValue.sival_ptr = NULL;
}


int32
Signal::Priority() const
{
	return kSignalInfos[fNumber].priority;
}


void
Signal::Handled()
{
	ReleaseReference();
}


void
Signal::LastReferenceReleased()
{
	if (are_interrupts_enabled())
		delete this;
	else
		deferred_delete(this);
}


// #pragma mark - PendingSignals


PendingSignals::PendingSignals()
	:
	fQueuedSignalsMask(0),
	fUnqueuedSignalsMask(0)
{
}


PendingSignals::~PendingSignals()
{
	Clear();
}


/*!	Of the signals in \a nonBlocked returns the priority of that with the
	highest priority.
	\param nonBlocked The mask with the non-blocked signals.
	\return The priority of the highest priority non-blocked signal, or, if all
		signals are blocked, \c -1.
*/
int32
PendingSignals::HighestSignalPriority(sigset_t nonBlocked) const
{
	Signal* queuedSignal;
	int32 unqueuedSignal;
	return _GetHighestPrioritySignal(nonBlocked, queuedSignal, unqueuedSignal);
}


void
PendingSignals::Clear()
{
	// release references of all queued signals
	while (Signal* signal = fQueuedSignals.RemoveHead())
		signal->Handled();

	fQueuedSignalsMask = 0;
	fUnqueuedSignalsMask = 0;
}


/*!	Adds a signal.
	Takes over the reference to the signal from the caller.
*/
void
PendingSignals::AddSignal(Signal* signal)
{
	// queue according to priority
	int32 priority = signal->Priority();
	Signal* otherSignal = NULL;
	for (SignalList::Iterator it = fQueuedSignals.GetIterator();
			(otherSignal = it.Next()) != NULL;) {
		if (priority > otherSignal->Priority())
			break;
	}

	fQueuedSignals.InsertBefore(otherSignal, signal);
	signal->SetPending(true);

	fQueuedSignalsMask |= SIGNAL_TO_MASK(signal->Number());
}


void
PendingSignals::RemoveSignal(Signal* signal)
{
	signal->SetPending(false);
	fQueuedSignals.Remove(signal);
	_UpdateQueuedSignalMask();
}


void
PendingSignals::RemoveSignals(sigset_t mask)
{
	// remove from queued signals
	if ((fQueuedSignalsMask & mask) != 0) {
		for (SignalList::Iterator it = fQueuedSignals.GetIterator();
				Signal* signal = it.Next();) {
			// remove signal, if in mask
			if ((SIGNAL_TO_MASK(signal->Number()) & mask) != 0) {
				it.Remove();
				signal->SetPending(false);
				signal->Handled();
			}
		}

		fQueuedSignalsMask &= ~mask;
	}

	// remove from unqueued signals
	fUnqueuedSignalsMask &= ~mask;
}


/*!	Removes and returns a signal in \a nonBlocked that has the highest priority.
	The caller gets a reference to the returned signal, if any.
	\param nonBlocked The mask of non-blocked signals.
	\param buffer If the signal is not queued this buffer is returned. In this
		case the method acquires a reference to \a buffer, so that the caller
		gets a reference also in this case.
	\return The removed signal or \c NULL, if all signals are blocked.
*/
Signal*
PendingSignals::DequeueSignal(sigset_t nonBlocked, Signal& buffer)
{
	// find the signal with the highest priority
	Signal* queuedSignal;
	int32 unqueuedSignal;
	if (_GetHighestPrioritySignal(nonBlocked, queuedSignal, unqueuedSignal) < 0)
		return NULL;

	// if it is a queued signal, dequeue it
	if (queuedSignal != NULL) {
		fQueuedSignals.Remove(queuedSignal);
		queuedSignal->SetPending(false);
		_UpdateQueuedSignalMask();
		return queuedSignal;
	}

	// it is unqueued -- remove from mask
	fUnqueuedSignalsMask &= ~SIGNAL_TO_MASK(unqueuedSignal);

	// init buffer
	buffer.SetTo(unqueuedSignal);
	buffer.AcquireReference();
	return &buffer;
}


/*!	Of the signals not it \a blocked returns the priority of that with the
	highest priority.
	\param blocked The mask with the non-blocked signals.
	\param _queuedSignal If the found signal is a queued signal, the variable
		will be set to that signal, otherwise to \c NULL.
	\param _unqueuedSignal If the found signal is an unqueued signal, the
		variable is set to that signal's number, otherwise to \c -1.
	\return The priority of the highest priority non-blocked signal, or, if all
		signals are blocked, \c -1.
*/
int32
PendingSignals::_GetHighestPrioritySignal(sigset_t nonBlocked,
	Signal*& _queuedSignal, int32& _unqueuedSignal) const
{
	// check queued signals
	Signal* queuedSignal = NULL;
	int32 queuedPriority = -1;

	if ((fQueuedSignalsMask & nonBlocked) != 0) {
		for (SignalList::ConstIterator it = fQueuedSignals.GetIterator();
				Signal* signal = it.Next();) {
			if ((SIGNAL_TO_MASK(signal->Number()) & nonBlocked) != 0) {
				queuedPriority = signal->Priority();
				queuedSignal = signal;
				break;
			}
		}
	}

	// check unqueued signals
	int32 unqueuedSignal = -1;
	int32 unqueuedPriority = -1;

	sigset_t unqueuedSignals = fUnqueuedSignalsMask & nonBlocked;
	if (unqueuedSignals != 0) {
		int32 signal = 1;
		while (unqueuedSignals != 0) {
			sigset_t mask = SIGNAL_TO_MASK(signal);
			if ((unqueuedSignals & mask) != 0) {
				int32 priority = kSignalInfos[signal].priority;
				if (priority > unqueuedPriority) {
					unqueuedSignal = signal;
					unqueuedPriority = priority;
				}
				unqueuedSignals &= ~mask;
			}

			signal++;
		}
	}

	// Return found queued or unqueued signal, whichever has the higher
	// priority.
	if (queuedPriority >= unqueuedPriority) {
		_queuedSignal = queuedSignal;
		_unqueuedSignal = -1;
		return queuedPriority;
	}

	_queuedSignal = NULL;
	_unqueuedSignal = unqueuedSignal;
	return unqueuedPriority;
}


void
PendingSignals::_UpdateQueuedSignalMask()
{
	sigset_t mask = 0;
	for (SignalList::Iterator it = fQueuedSignals.GetIterator();
			Signal* signal = it.Next();) {
		mask |= SIGNAL_TO_MASK(signal->Number());
	}

	fQueuedSignalsMask = mask;
}


// #pragma mark - signal tracing


#if SIGNAL_TRACING

namespace SignalTracing {


class HandleSignal : public AbstractTraceEntry {
	public:
		HandleSignal(uint32 signal)
			:
			fSignal(signal)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("signal handle:  %" B_PRIu32 " (%s)" , fSignal,
				signal_name(fSignal));
		}

	private:
		uint32		fSignal;
};


class ExecuteSignalHandler : public AbstractTraceEntry {
	public:
		ExecuteSignalHandler(uint32 signal, struct sigaction* handler)
			:
			fSignal(signal),
			fHandler((void*)handler->sa_handler)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("signal exec handler: signal: %" B_PRIu32 " (%s), "
				"handler: %p", fSignal, signal_name(fSignal), fHandler);
		}

	private:
		uint32	fSignal;
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
				"flags: 0x%lx", fTarget, fSignal, signal_name(fSignal), fFlags);
		}

	private:
		pid_t	fTarget;
		uint32	fSignal;
		uint32	fFlags;
};


class SigAction : public AbstractTraceEntry {
	public:
		SigAction(uint32 signal, const struct sigaction* act)
			:
			fSignal(signal),
			fAction(*act)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("signal action: signal: %lu (%s), "
				"action: {handler: %p, flags: 0x%x, mask: 0x%llx}", fSignal,
				signal_name(fSignal), fAction.sa_handler, fAction.sa_flags,
				(long long)fAction.sa_mask);
		}

	private:
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

			out.Print("signal proc mask: %s 0x%llx, old mask: 0x%llx", how,
				(long long)fMask, (long long)fOldMask);
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
			out.Print("signal suspend: %#llx, old mask: %#llx",
				(long long)fMask, (long long)fOldMask);
		}

	private:
		sigset_t	fMask;
		sigset_t	fOldMask;
};


class SigSuspendDone : public AbstractTraceEntry {
	public:
		SigSuspendDone()
			:
			fSignals(thread_get_current_thread()->ThreadPendingSignals())
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


/*!	Updates the given thread's Thread::flags field according to what signals are
	pending.
	The caller must hold the scheduler lock.
*/
static void
update_thread_signals_flag(Thread* thread)
{
	sigset_t mask = ~thread->sig_block_mask;
	if ((thread->AllPendingSignals() & mask) != 0)
		atomic_or(&thread->flags, THREAD_FLAGS_SIGNALS_PENDING);
	else
		atomic_and(&thread->flags, ~THREAD_FLAGS_SIGNALS_PENDING);
}


/*!	Updates the current thread's Thread::flags field according to what signals
	are pending.
	The caller must hold the scheduler lock.
*/
static void
update_current_thread_signals_flag()
{
	update_thread_signals_flag(thread_get_current_thread());
}


/*!	Updates all of the given team's threads' Thread::flags fields according to
	what signals are pending.
	The caller must hold the scheduler lock.
*/
static void
update_team_threads_signal_flag(Team* team)
{
	for (Thread* thread = team->thread_list; thread != NULL;
			thread = thread->team_next) {
		update_thread_signals_flag(thread);
	}
}


/*!	Notifies the user debugger about a signal to be handled.

	The caller must not hold any locks.

	\param thread The current thread.
	\param signal The signal to be handled.
	\param handler The installed signal handler for the signal.
	\param deadly Indicates whether the signal is deadly.
	\return \c true, if the signal shall be handled, \c false, if it shall be
		ignored.
*/
static bool
notify_debugger(Thread* thread, Signal* signal, struct sigaction& handler,
	bool deadly)
{
	uint64 signalMask = SIGNAL_TO_MASK(signal->Number());

	// first check the ignore signal masks the debugger specified for the thread
	InterruptsSpinLocker threadDebugInfoLocker(thread->debug_info.lock);

	if ((thread->debug_info.ignore_signals_once & signalMask) != 0) {
		thread->debug_info.ignore_signals_once &= ~signalMask;
		return true;
	}

	if ((thread->debug_info.ignore_signals & signalMask) != 0)
		return true;

	threadDebugInfoLocker.Unlock();

	// deliver the event
	return user_debug_handle_signal(signal->Number(), &handler, deadly);
}


/*!	Removes and returns a signal with the highest priority in \a nonBlocked that
	is pending in the given thread or its team.
	After dequeuing the signal the Thread::flags field of the affected threads
	are updated.
	The caller gets a reference to the returned signal, if any.
	The caller must hold the scheduler lock.
	\param thread The thread.
	\param nonBlocked The mask of non-blocked signals.
	\param buffer If the signal is not queued this buffer is returned. In this
		case the method acquires a reference to \a buffer, so that the caller
		gets a reference also in this case.
	\return The removed signal or \c NULL, if all signals are blocked.
*/
static Signal*
dequeue_thread_or_team_signal(Thread* thread, sigset_t nonBlocked,
	Signal& buffer)
{
	Team* team = thread->team;
	Signal* signal;
	if (team->HighestPendingSignalPriority(nonBlocked)
			> thread->HighestPendingSignalPriority(nonBlocked)) {
		signal = team->DequeuePendingSignal(nonBlocked, buffer);
		update_team_threads_signal_flag(team);
	} else {
		signal = thread->DequeuePendingSignal(nonBlocked, buffer);
		update_thread_signals_flag(thread);
	}

	return signal;
}


static status_t
setup_signal_frame(Thread* thread, struct sigaction* action, Signal* signal,
	sigset_t signalMask)
{
	// prepare the data, we need to copy onto the user stack
	signal_frame_data frameData;

	// signal info
	frameData.info.si_signo = signal->Number();
	frameData.info.si_code = signal->SignalCode();
	frameData.info.si_errno = signal->ErrorCode();
	frameData.info.si_pid = signal->SendingProcess();
	frameData.info.si_uid = signal->SendingUser();
	frameData.info.si_addr = signal->Address();
	frameData.info.si_status = signal->Status();
	frameData.info.si_band = signal->PollBand();
	frameData.info.si_value = signal->UserValue();

	// context
	frameData.context.uc_link = thread->user_signal_context;
	frameData.context.uc_sigmask = signalMask;
	// uc_stack and uc_mcontext are filled in by the architecture specific code.

	// user data
	frameData.user_data = action->sa_userdata;

	// handler function
	frameData.siginfo_handler = (action->sa_flags & SA_SIGINFO) != 0;
	frameData.handler = frameData.siginfo_handler
		? (void*)action->sa_sigaction : (void*)action->sa_handler;

	// thread flags -- save the and clear the thread's syscall restart related
	// flags
	frameData.thread_flags = atomic_and(&thread->flags,
		~(THREAD_FLAGS_RESTART_SYSCALL | THREAD_FLAGS_64_BIT_SYSCALL_RETURN));

	// syscall restart related fields
	memcpy(frameData.syscall_restart_parameters,
		thread->syscall_restart.parameters,
		sizeof(frameData.syscall_restart_parameters));
	// syscall_restart_return_value is filled in by the architecture specific
	// code.

	return arch_setup_signal_frame(thread, action, &frameData);
}


/*! Actually handles pending signals -- i.e. the thread will exit, a custom
	signal handler is prepared, or whatever the signal demands.
	The function will not return, when a deadly signal is encountered. The
	function will suspend the thread indefinitely, when a stop signal is
	encountered.
	Interrupts must be enabled.
	\param thread The current thread.
*/
void
handle_signals(Thread* thread)
{
	Team* team = thread->team;

	TeamLocker teamLocker(team);
	InterruptsSpinLocker schedulerLocker(gSchedulerLock);

	// If userland requested to defer signals, we check now, if this is
	// possible.
	sigset_t nonBlockedMask = ~thread->sig_block_mask;
	sigset_t signalMask = thread->AllPendingSignals() & nonBlockedMask;

	if (thread->user_thread->defer_signals > 0
		&& (signalMask & NON_DEFERRABLE_SIGNALS) == 0
		&& thread->sigsuspend_original_unblocked_mask == 0) {
		thread->user_thread->pending_signals = signalMask;
		return;
	}

	thread->user_thread->pending_signals = 0;

	// determine syscall restart behavior
	uint32 restartFlags = atomic_and(&thread->flags,
		~THREAD_FLAGS_DONT_RESTART_SYSCALL);
	bool alwaysRestart
		= (restartFlags & THREAD_FLAGS_ALWAYS_RESTART_SYSCALL) != 0;
	bool restart = alwaysRestart
		|| (restartFlags & THREAD_FLAGS_DONT_RESTART_SYSCALL) == 0;

	// Loop until we've handled all signals.
	bool initialIteration = true;
	while (true) {
		if (initialIteration) {
			initialIteration = false;
		} else {
			teamLocker.Lock();
			schedulerLocker.Lock();

			signalMask = thread->AllPendingSignals() & nonBlockedMask;
		}

		// Unless SIGKILL[THR] are pending, check, if the thread shall stop for
		// debugging.
		if ((signalMask & KILL_SIGNALS) == 0
			&& (atomic_get(&thread->debug_info.flags) & B_THREAD_DEBUG_STOP)
				!= 0) {
			schedulerLocker.Unlock();
			teamLocker.Unlock();

			user_debug_stop_thread();
			continue;
		}

		// We're done, if there aren't any pending signals anymore.
		if ((signalMask & nonBlockedMask) == 0)
			break;

		// get pending non-blocked thread or team signal with the highest
		// priority
		Signal stackSignal;
		Signal* signal = dequeue_thread_or_team_signal(thread, nonBlockedMask,
			stackSignal);
		ASSERT(signal != NULL);
		SignalHandledCaller signalHandledCaller(signal);

		schedulerLocker.Unlock();

		// get the action for the signal
		struct sigaction handler;
		if (signal->Number() <= MAX_SIGNAL_NUMBER) {
			handler = team->SignalActionFor(signal->Number());
		} else {
			handler.sa_handler = SIG_DFL;
			handler.sa_flags = 0;
		}

		if ((handler.sa_flags & SA_ONESHOT) != 0
			&& handler.sa_handler != SIG_IGN && handler.sa_handler != SIG_DFL) {
			team->SignalActionFor(signal->Number()).sa_handler = SIG_DFL;
		}

		T(HandleSignal(signal->Number()));

		teamLocker.Unlock();

		// debug the signal, if a debugger is installed and the signal debugging
		// flag is set
		bool debugSignal = (~atomic_get(&team->debug_info.flags)
				& (B_TEAM_DEBUG_DEBUGGER_INSTALLED | B_TEAM_DEBUG_SIGNALS))
			== 0;

		// handle the signal
		TRACE(("Thread %" B_PRId32 " received signal %s\n", thread->id,
			kSignalInfos[signal->Number()].name));

		if (handler.sa_handler == SIG_IGN) {
			// signal is to be ignored
			// TODO: apply zombie cleaning on SIGCHLD

			// notify the debugger
			if (debugSignal)
				notify_debugger(thread, signal, handler, false);
			continue;
		} else if (handler.sa_handler == SIG_DFL) {
			// default signal behaviour

			// realtime signals are ignored by default
			if (signal->Number() >= SIGNAL_REALTIME_MIN
				&& signal->Number() <= SIGNAL_REALTIME_MAX) {
				// notify the debugger
				if (debugSignal)
					notify_debugger(thread, signal, handler, false);
				continue;
			}

			bool killTeam = false;
			switch (signal->Number()) {
				case SIGCHLD:
				case SIGWINCH:
				case SIGURG:
					// notify the debugger
					if (debugSignal)
						notify_debugger(thread, signal, handler, false);
					continue;

				case SIGNAL_CANCEL_THREAD:
					// set up the signal handler
					handler.sa_handler = thread->cancel_function;
					handler.sa_flags = 0;
					handler.sa_mask = 0;
					handler.sa_userdata = NULL;

					restart = false;
						// we always want to interrupt
					break;

				case SIGNAL_CONTINUE_THREAD:
					// prevent syscall restart, but otherwise ignore
					restart = false;
					atomic_and(&thread->flags, ~THREAD_FLAGS_RESTART_SYSCALL);
					continue;

				case SIGCONT:
					// notify the debugger
					if (debugSignal
						&& !notify_debugger(thread, signal, handler, false))
						continue;

					// notify threads waiting for team state changes
					if (thread == team->main_thread) {
						team->LockTeamAndParent(false);

						team_set_job_control_state(team,
							JOB_CONTROL_STATE_CONTINUED, signal, false);

						team->UnlockTeamAndParent();

						// The standard states that the system *may* send a
						// SIGCHLD when a child is continued. I haven't found
						// a good reason why we would want to, though.
					}
					continue;

				case SIGSTOP:
				case SIGTSTP:
				case SIGTTIN:
				case SIGTTOU:
				{
					// notify the debugger
					if (debugSignal
						&& !notify_debugger(thread, signal, handler, false))
						continue;

					// The terminal-sent stop signals are allowed to stop the
					// process only, if it doesn't belong to an orphaned process
					// group. Otherwise the signal must be discarded.
					team->LockProcessGroup();
					AutoLocker<ProcessGroup> groupLocker(team->group, true);
					if (signal->Number() != SIGSTOP
						&& team->group->IsOrphaned()) {
						continue;
					}

					// notify threads waiting for team state changes
					if (thread == team->main_thread) {
						team->LockTeamAndParent(false);

						team_set_job_control_state(team,
							JOB_CONTROL_STATE_STOPPED, signal, false);

						// send a SIGCHLD to the parent (if it does have
						// SA_NOCLDSTOP defined)
						Team* parentTeam = team->parent;

						struct sigaction& parentHandler
							= parentTeam->SignalActionFor(SIGCHLD);
						if ((parentHandler.sa_flags & SA_NOCLDSTOP) == 0) {
							Signal childSignal(SIGCHLD, CLD_STOPPED, B_OK,
								team->id);
							childSignal.SetStatus(signal->Number());
							childSignal.SetSendingUser(signal->SendingUser());
							send_signal_to_team(parentTeam, childSignal, 0);
						}

						team->UnlockTeamAndParent();
					}

					groupLocker.Unlock();

					// Suspend the thread, unless there's already a signal to
					// continue or kill pending.
					InterruptsSpinLocker schedulerLocker(gSchedulerLock);
					if ((thread->AllPendingSignals()
							& (CONTINUE_SIGNALS | KILL_SIGNALS)) == 0) {
						thread->next_state = B_THREAD_SUSPENDED;
						scheduler_reschedule();
					}
					schedulerLocker.Unlock();

					continue;
				}

				case SIGSEGV:
				case SIGBUS:
				case SIGFPE:
				case SIGILL:
				case SIGTRAP:
				case SIGABRT:
				case SIGKILL:
				case SIGQUIT:
				case SIGPOLL:
				case SIGPROF:
				case SIGSYS:
				case SIGVTALRM:
				case SIGXCPU:
				case SIGXFSZ:
				default:
					TRACE(("Shutting down team %" B_PRId32 " due to signal %"
						B_PRIu32 " received in thread %" B_PRIu32 " \n",
						team->id, signal->Number(), thread->id));

					// This signal kills the team regardless which thread
					// received it.
					killTeam = true;

					// fall through
				case SIGKILLTHR:
					// notify the debugger
					if (debugSignal && signal->Number() != SIGKILL
						&& signal->Number() != SIGKILLTHR
						&& !notify_debugger(thread, signal, handler, true)) {
						continue;
					}

					if (killTeam || thread == team->main_thread) {
						// The signal is terminal for the team or the thread is
						// the main thread. In either case the team is going
						// down. Set its exit status, if that didn't happen yet.
						teamLocker.Lock();

						if (!team->exit.initialized) {
							team->exit.reason = CLD_KILLED;
							team->exit.signal = signal->Number();
							team->exit.signaling_user = signal->SendingUser();
							team->exit.status = 0;
							team->exit.initialized = true;
						}

						teamLocker.Unlock();

						// If this is not the main thread, send it a SIGKILLTHR
						// so that the team terminates.
						if (thread != team->main_thread) {
							Signal childSignal(SIGKILLTHR, SI_USER, B_OK,
								team->id);
							send_signal_to_thread_id(team->id, childSignal, 0);
						}
					}

					// explicitly get rid of the signal reference, since
					// thread_exit() won't return
					signalHandledCaller.Done();

					thread_exit();
						// won't return
			}
		}

		// User defined signal handler

		// notify the debugger
		if (debugSignal && !notify_debugger(thread, signal, handler, false))
			continue;

		if (!restart
				|| (!alwaysRestart && (handler.sa_flags & SA_RESTART) == 0)) {
			atomic_and(&thread->flags, ~THREAD_FLAGS_RESTART_SYSCALL);
		}

		T(ExecuteSignalHandler(signal->Number(), &handler));

		TRACE(("### Setting up custom signal handler frame...\n"));

		// save the old block mask -- we may need to adjust it for the handler
		schedulerLocker.Lock();

		sigset_t oldBlockMask = thread->sigsuspend_original_unblocked_mask != 0
			? ~thread->sigsuspend_original_unblocked_mask
			: thread->sig_block_mask;

		// Update the block mask while the signal handler is running -- it
		// will be automatically restored when the signal frame is left.
		thread->sig_block_mask |= handler.sa_mask & BLOCKABLE_SIGNALS;

		if ((handler.sa_flags & SA_NOMASK) == 0) {
			thread->sig_block_mask
				|= SIGNAL_TO_MASK(signal->Number()) & BLOCKABLE_SIGNALS;
		}

		update_current_thread_signals_flag();

		schedulerLocker.Unlock();

		setup_signal_frame(thread, &handler, signal, oldBlockMask);

		// Reset sigsuspend_original_unblocked_mask. It would have been set by
		// sigsuspend_internal(). In that case, above we set oldBlockMask
		// accordingly so that after the handler returns the thread's signal
		// mask is reset.
		thread->sigsuspend_original_unblocked_mask = 0;

		return;
	}

	// We have not handled any signal (respectively only ignored ones).

	// If sigsuspend_original_unblocked_mask is non-null, we came from a
	// sigsuspend_internal(). Not having handled any signal, we should restart
	// the syscall.
	if (thread->sigsuspend_original_unblocked_mask != 0) {
		restart = true;
		atomic_or(&thread->flags, THREAD_FLAGS_RESTART_SYSCALL);
	} else if (!restart) {
		// clear syscall restart thread flag, if we're not supposed to restart
		// the syscall
		atomic_and(&thread->flags, ~THREAD_FLAGS_RESTART_SYSCALL);
	}
}


/*!	Checks whether the given signal is blocked for the given team (i.e. all of
	its threads).
	The caller must hold the team's lock and the scheduler lock.
*/
bool
is_team_signal_blocked(Team* team, int signal)
{
	sigset_t mask = SIGNAL_TO_MASK(signal);

	for (Thread* thread = team->thread_list; thread != NULL;
			thread = thread->team_next) {
		if ((thread->sig_block_mask & mask) == 0)
			return false;
	}

	return true;
}


/*!	Gets (guesses) the current thread's currently used stack from the given
	stack pointer.
	Fills in \a stack with either the signal stack or the thread's user stack.
	\param address A stack pointer address to be used to determine the used
		stack.
	\param stack Filled in by the function.
*/
void
signal_get_user_stack(addr_t address, stack_t* stack)
{
	// If a signal stack is enabled for the stack and the address is within it,
	// return the signal stack. In all other cases return the thread's user
	// stack, even if the address doesn't lie within it.
	Thread* thread = thread_get_current_thread();
	if (thread->signal_stack_enabled && address >= thread->signal_stack_base
		&& address < thread->signal_stack_base + thread->signal_stack_size) {
		stack->ss_sp = (void*)thread->signal_stack_base;
		stack->ss_size = thread->signal_stack_size;
	} else {
		stack->ss_sp = (void*)thread->user_stack_base;
		stack->ss_size = thread->user_stack_size;
	}

	stack->ss_flags = 0;
}


/*!	Checks whether any non-blocked signal is pending for the current thread.
	The caller must hold the scheduler lock.
	\param thread The current thread.
*/
static bool
has_signals_pending(Thread* thread)
{
	return (thread->AllPendingSignals() & ~thread->sig_block_mask) != 0;
}


/*!	Checks whether the current user has permission to send a signal to the given
	target team.

	The caller must hold the scheduler lock or \a team's lock.

	\param team The target team.
	\param schedulerLocked \c true, if the caller holds the scheduler lock,
		\c false otherwise.
*/
static bool
has_permission_to_signal(Team* team, bool schedulerLocked)
{
	// get the current user
	uid_t currentUser = schedulerLocked
		? thread_get_current_thread()->team->effective_uid
		: geteuid();

	// root is omnipotent -- in the other cases the current user must match the
	// target team's
	return currentUser == 0 || currentUser == team->effective_uid;
}


/*!	Delivers a signal to the \a thread, but doesn't handle the signal -- it just
	makes sure the thread gets the signal, i.e. unblocks it if needed.

	The caller must hold the scheduler lock.

	\param thread The thread the signal shall be delivered to.
	\param signalNumber The number of the signal to be delivered. If \c 0, no
		actual signal will be delivered. Only delivery checks will be performed.
	\param signal If non-NULL the signal to be queued (has number
		\a signalNumber in this case). The caller transfers an object reference
		to this function. If \c NULL an unqueued signal will be delivered to the
		thread.
	\param flags A bitwise combination of any number of the following:
		- \c B_CHECK_PERMISSION: Check the caller's permission to send the
			target thread the signal.
	\return \c B_OK, when the signal was delivered successfully, another error
		code otherwise.
*/
status_t
send_signal_to_thread_locked(Thread* thread, uint32 signalNumber,
	Signal* signal, uint32 flags)
{
	ASSERT(signal == NULL || signalNumber == signal->Number());

	T(SendSignal(thread->id, signalNumber, flags));

	// The caller transferred a reference to the signal to us.
	BReference<Signal> signalReference(signal, true);

	if ((flags & B_CHECK_PERMISSION) != 0) {
		if (!has_permission_to_signal(thread->team, true))
			return EPERM;
	}

	if (signalNumber == 0)
		return B_OK;

	if (thread->team == team_get_kernel_team()) {
		// Signals to kernel threads will only wake them up
		if (thread->state == B_THREAD_SUSPENDED)
			scheduler_enqueue_in_run_queue(thread);
		return B_OK;
	}

	if (signal != NULL)
		thread->AddPendingSignal(signal);
	else
		thread->AddPendingSignal(signalNumber);

	// the thread has the signal reference, now
	signalReference.Detach();

	switch (signalNumber) {
		case SIGKILL:
		{
			// If sent to a thread other than the team's main thread, also send
			// a SIGKILLTHR to the main thread to kill the team.
			Thread* mainThread = thread->team->main_thread;
			if (mainThread != NULL && mainThread != thread) {
				mainThread->AddPendingSignal(SIGKILLTHR);

				// wake up main thread
				if (mainThread->state == B_THREAD_SUSPENDED)
					scheduler_enqueue_in_run_queue(mainThread);
				else
					thread_interrupt(mainThread, true);

				update_thread_signals_flag(mainThread);
			}

			// supposed to fall through
		}
		case SIGKILLTHR:
			// Wake up suspended threads and interrupt waiting ones
			if (thread->state == B_THREAD_SUSPENDED)
				scheduler_enqueue_in_run_queue(thread);
			else
				thread_interrupt(thread, true);
			break;

		case SIGNAL_CONTINUE_THREAD:
			// wake up thread, and interrupt its current syscall
			if (thread->state == B_THREAD_SUSPENDED)
				scheduler_enqueue_in_run_queue(thread);

			atomic_or(&thread->flags, THREAD_FLAGS_DONT_RESTART_SYSCALL);
			break;

		case SIGCONT:
			// Wake up thread if it was suspended, otherwise interrupt it, if
			// the signal isn't blocked.
			if (thread->state == B_THREAD_SUSPENDED)
				scheduler_enqueue_in_run_queue(thread);
			else if ((SIGNAL_TO_MASK(SIGCONT) & ~thread->sig_block_mask) != 0)
				thread_interrupt(thread, false);

			// remove any pending stop signals
			thread->RemovePendingSignals(STOP_SIGNALS);
			break;

		default:
			// If the signal is not masked, interrupt the thread, if it is
			// currently waiting (interruptibly).
			if ((thread->AllPendingSignals()
						& (~thread->sig_block_mask | SIGNAL_TO_MASK(SIGCHLD)))
					!= 0) {
				// Interrupt thread if it was waiting
				thread_interrupt(thread, false);
			}
			break;
	}

	update_thread_signals_flag(thread);

	return B_OK;
}


/*!	Sends the given signal to the given thread.

	The caller must not hold the scheduler lock.

	\param thread The thread the signal shall be sent to.
	\param signal The signal to be delivered. If the signal's number is \c 0, no
		actual signal will be delivered. Only delivery checks will be performed.
		The given object will be copied. The caller retains ownership.
	\param flags A bitwise combination of any number of the following:
		- \c B_CHECK_PERMISSION: Check the caller's permission to send the
			target thread the signal.
		- \c B_DO_NOT_RESCHEDULE: If clear and a higher level thread has been
			woken up, the scheduler will be invoked. If set that will not be
			done explicitly, but rescheduling can still happen, e.g. when the
			current thread's time slice runs out.
	\return \c B_OK, when the signal was delivered successfully, another error
		code otherwise.
*/
status_t
send_signal_to_thread(Thread* thread, const Signal& signal, uint32 flags)
{
	// Clone the signal -- the clone will be queued. If something fails and the
	// caller doesn't require queuing, we will add an unqueued signal.
	Signal* signalToQueue = NULL;
	status_t error = Signal::CreateQueuable(signal,
		(flags & SIGNAL_FLAG_QUEUING_REQUIRED) != 0, signalToQueue);
	if (error != B_OK)
		return error;

	InterruptsSpinLocker schedulerLocker(gSchedulerLock);

	error = send_signal_to_thread_locked(thread, signal.Number(), signalToQueue,
		flags);
	if (error != B_OK)
		return error;

	if ((flags & B_DO_NOT_RESCHEDULE) == 0)
		scheduler_reschedule_if_necessary_locked();

	return B_OK;
}


/*!	Sends the given signal to the thread with the given ID.

	The caller must not hold the scheduler lock.

	\param threadID The ID of the thread the signal shall be sent to.
	\param signal The signal to be delivered. If the signal's number is \c 0, no
		actual signal will be delivered. Only delivery checks will be performed.
		The given object will be copied. The caller retains ownership.
	\param flags A bitwise combination of any number of the following:
		- \c B_CHECK_PERMISSION: Check the caller's permission to send the
			target thread the signal.
		- \c B_DO_NOT_RESCHEDULE: If clear and a higher level thread has been
			woken up, the scheduler will be invoked. If set that will not be
			done explicitly, but rescheduling can still happen, e.g. when the
			current thread's time slice runs out.
	\return \c B_OK, when the signal was delivered successfully, another error
		code otherwise.
*/
status_t
send_signal_to_thread_id(thread_id threadID, const Signal& signal, uint32 flags)
{
	Thread* thread = Thread::Get(threadID);
	if (thread == NULL)
		return B_BAD_THREAD_ID;
	BReference<Thread> threadReference(thread, true);

	return send_signal_to_thread(thread, signal, flags);
}


/*!	Sends the given signal to the given team.

	The caller must hold the scheduler lock.

	\param team The team the signal shall be sent to.
	\param signalNumber The number of the signal to be delivered. If \c 0, no
		actual signal will be delivered. Only delivery checks will be performed.
	\param signal If non-NULL the signal to be queued (has number
		\a signalNumber in this case). The caller transfers an object reference
		to this function. If \c NULL an unqueued signal will be delivered to the
		thread.
	\param flags A bitwise combination of any number of the following:
		- \c B_CHECK_PERMISSION: Check the caller's permission to send the
			target thread the signal.
		- \c B_DO_NOT_RESCHEDULE: If clear and a higher level thread has been
			woken up, the scheduler will be invoked. If set that will not be
			done explicitly, but rescheduling can still happen, e.g. when the
			current thread's time slice runs out.
	\return \c B_OK, when the signal was delivered successfully, another error
		code otherwise.
*/
status_t
send_signal_to_team_locked(Team* team, uint32 signalNumber, Signal* signal,
	uint32 flags)
{
	ASSERT(signal == NULL || signalNumber == signal->Number());

	T(SendSignal(team->id, signalNumber, flags));

	// The caller transferred a reference to the signal to us.
	BReference<Signal> signalReference(signal, true);

	if ((flags & B_CHECK_PERMISSION) != 0) {
		if (!has_permission_to_signal(team, true))
			return EPERM;
	}

	if (signalNumber == 0)
		return B_OK;

	if (team == team_get_kernel_team()) {
		// signals to the kernel team are not allowed
		return EPERM;
	}

	if (signal != NULL)
		team->AddPendingSignal(signal);
	else
		team->AddPendingSignal(signalNumber);

	// the team has the signal reference, now
	signalReference.Detach();

	switch (signalNumber) {
		case SIGKILL:
		case SIGKILLTHR:
		{
			// Also add a SIGKILLTHR to the main thread's signals and wake it
			// up/interrupt it, so we get this over with as soon as possible
			// (only the main thread shuts down the team).
			Thread* mainThread = team->main_thread;
			if (mainThread != NULL) {
				mainThread->AddPendingSignal(SIGKILLTHR);

				// wake up main thread
				if (mainThread->state == B_THREAD_SUSPENDED)
					scheduler_enqueue_in_run_queue(mainThread);
				else
					thread_interrupt(mainThread, true);
			}
			break;
		}

		case SIGCONT:
			// Wake up any suspended threads, interrupt the others, if they
			// don't block the signal.
			for (Thread* thread = team->thread_list; thread != NULL;
					thread = thread->team_next) {
				if (thread->state == B_THREAD_SUSPENDED) {
					scheduler_enqueue_in_run_queue(thread);
				} else if ((SIGNAL_TO_MASK(SIGCONT) & ~thread->sig_block_mask)
						!= 0) {
					thread_interrupt(thread, false);
				}

				// remove any pending stop signals
				thread->RemovePendingSignals(STOP_SIGNALS);
			}

			// remove any pending team stop signals
			team->RemovePendingSignals(STOP_SIGNALS);
			break;

		case SIGSTOP:
		case SIGTSTP:
		case SIGTTIN:
		case SIGTTOU:
			// send the stop signal to all threads
			// TODO: Is that correct or should we only target the main thread?
			for (Thread* thread = team->thread_list; thread != NULL;
					thread = thread->team_next) {
				thread->AddPendingSignal(signalNumber);
			}

			// remove the stop signal from the team again
			if (signal != NULL) {
				team->RemovePendingSignal(signal);
				signalReference.SetTo(signal, true);
			} else
				team->RemovePendingSignal(signalNumber);

			// fall through to interrupt threads
		default:
			// Interrupt all interruptibly waiting threads, if the signal is
			// not masked.
			for (Thread* thread = team->thread_list; thread != NULL;
					thread = thread->team_next) {
				sigset_t nonBlocked = ~thread->sig_block_mask
					| SIGNAL_TO_MASK(SIGCHLD);
				if ((thread->AllPendingSignals() & nonBlocked) != 0)
					thread_interrupt(thread, false);
			}
			break;
	}

	update_team_threads_signal_flag(team);

	if ((flags & B_DO_NOT_RESCHEDULE) == 0)
		scheduler_reschedule_if_necessary_locked();

	return B_OK;
}


/*!	Sends the given signal to the given team.

	\param team The team the signal shall be sent to.
	\param signal The signal to be delivered. If the signal's number is \c 0, no
		actual signal will be delivered. Only delivery checks will be performed.
		The given object will be copied. The caller retains ownership.
	\param flags A bitwise combination of any number of the following:
		- \c B_CHECK_PERMISSION: Check the caller's permission to send the
			target thread the signal.
		- \c B_DO_NOT_RESCHEDULE: If clear and a higher level thread has been
			woken up, the scheduler will be invoked. If set that will not be
			done explicitly, but rescheduling can still happen, e.g. when the
			current thread's time slice runs out.
	\return \c B_OK, when the signal was delivered successfully, another error
		code otherwise.
*/
status_t
send_signal_to_team(Team* team, const Signal& signal, uint32 flags)
{
	// Clone the signal -- the clone will be queued. If something fails and the
	// caller doesn't require queuing, we will add an unqueued signal.
	Signal* signalToQueue = NULL;
	status_t error = Signal::CreateQueuable(signal,
		(flags & SIGNAL_FLAG_QUEUING_REQUIRED) != 0, signalToQueue);
	if (error != B_OK)
		return error;

	InterruptsSpinLocker schedulerLocker(gSchedulerLock);

	return send_signal_to_team_locked(team, signal.Number(), signalToQueue,
		flags);
}


/*!	Sends the given signal to the team with the given ID.

	\param teamID The ID of the team the signal shall be sent to.
	\param signal The signal to be delivered. If the signal's number is \c 0, no
		actual signal will be delivered. Only delivery checks will be performed.
		The given object will be copied. The caller retains ownership.
	\param flags A bitwise combination of any number of the following:
		- \c B_CHECK_PERMISSION: Check the caller's permission to send the
			target thread the signal.
		- \c B_DO_NOT_RESCHEDULE: If clear and a higher level thread has been
			woken up, the scheduler will be invoked. If set that will not be
			done explicitly, but rescheduling can still happen, e.g. when the
			current thread's time slice runs out.
	\return \c B_OK, when the signal was delivered successfully, another error
		code otherwise.
*/
status_t
send_signal_to_team_id(team_id teamID, const Signal& signal, uint32 flags)
{
	// get the team
	Team* team = Team::Get(teamID);
	if (team == NULL)
		return B_BAD_TEAM_ID;
	BReference<Team> teamReference(team, true);

	return send_signal_to_team(team, signal, flags);
}


/*!	Sends the given signal to the given process group.

	The caller must hold the process group's lock. Interrupts must be enabled.

	\param group The the process group the signal shall be sent to.
	\param signal The signal to be delivered. If the signal's number is \c 0, no
		actual signal will be delivered. Only delivery checks will be performed.
		The given object will be copied. The caller retains ownership.
	\param flags A bitwise combination of any number of the following:
		- \c B_CHECK_PERMISSION: Check the caller's permission to send the
			target thread the signal.
		- \c B_DO_NOT_RESCHEDULE: If clear and a higher level thread has been
			woken up, the scheduler will be invoked. If set that will not be
			done explicitly, but rescheduling can still happen, e.g. when the
			current thread's time slice runs out.
	\return \c B_OK, when the signal was delivered successfully, another error
		code otherwise.
*/
status_t
send_signal_to_process_group_locked(ProcessGroup* group, const Signal& signal,
	uint32 flags)
{
	T(SendSignal(-group->id, signal.Number(), flags));

	bool firstTeam = true;

	for (Team* team = group->teams; team != NULL; team = team->group_next) {
		status_t error = send_signal_to_team(team, signal,
			flags | B_DO_NOT_RESCHEDULE);
		// If sending to the first team in the group failed, let the whole call
		// fail.
		if (firstTeam) {
			if (error != B_OK)
				return error;
			firstTeam = false;
		}
	}

	if ((flags & B_DO_NOT_RESCHEDULE) == 0)
		scheduler_reschedule_if_necessary();

	return B_OK;
}


/*!	Sends the given signal to the process group specified by the given ID.

	The caller must not hold any process group, team, or thread lock. Interrupts
	must be enabled.

	\param groupID The ID of the process group the signal shall be sent to.
	\param signal The signal to be delivered. If the signal's number is \c 0, no
		actual signal will be delivered. Only delivery checks will be performed.
		The given object will be copied. The caller retains ownership.
	\param flags A bitwise combination of any number of the following:
		- \c B_CHECK_PERMISSION: Check the caller's permission to send the
			target thread the signal.
		- \c B_DO_NOT_RESCHEDULE: If clear and a higher level thread has been
			woken up, the scheduler will be invoked. If set that will not be
			done explicitly, but rescheduling can still happen, e.g. when the
			current thread's time slice runs out.
	\return \c B_OK, when the signal was delivered successfully, another error
		code otherwise.
*/
status_t
send_signal_to_process_group(pid_t groupID, const Signal& signal, uint32 flags)
{
	ProcessGroup* group = ProcessGroup::Get(groupID);
	if (group == NULL)
		return B_BAD_TEAM_ID;
	BReference<ProcessGroup> groupReference(group);

	T(SendSignal(-group->id, signal.Number(), flags));

	AutoLocker<ProcessGroup> groupLocker(group);

	status_t error = send_signal_to_process_group_locked(group, signal,
		flags | B_DO_NOT_RESCHEDULE);
	if (error != B_OK)
		return error;

	groupLocker.Unlock();

	if ((flags & B_DO_NOT_RESCHEDULE) == 0)
		scheduler_reschedule_if_necessary();

	return B_OK;
}


static status_t
send_signal_internal(pid_t id, uint signalNumber, union sigval userValue,
	uint32 flags)
{
	if (signalNumber > MAX_SIGNAL_NUMBER)
		return B_BAD_VALUE;

	Thread* thread = thread_get_current_thread();

	Signal signal(signalNumber,
		(flags & SIGNAL_FLAG_QUEUING_REQUIRED) != 0 ? SI_QUEUE : SI_USER,
		B_OK, thread->team->id);
		// Note: SI_USER/SI_QUEUE is not correct, if called from within the
		// kernel (or a driver), but we don't have any info here.
	signal.SetUserValue(userValue);

	// If id is > 0, send the signal to the respective thread.
	if (id > 0)
		return send_signal_to_thread_id(id, signal, flags);

	// If id == 0, send the signal to the current thread.
	if (id == 0)
		return send_signal_to_thread(thread, signal, flags);

	// If id == -1, send the signal to all teams the calling team has permission
	// to send signals to.
	if (id == -1) {
		// TODO: Implement correctly!
		// currently only send to the current team
		return send_signal_to_team_id(thread->team->id, signal, flags);
	}

	// Send a signal to the specified process group (the absolute value of the
	// id).
	return send_signal_to_process_group(-id, signal, flags);
}


int
send_signal_etc(pid_t id, uint signalNumber, uint32 flags)
{
	// a dummy user value
	union sigval userValue;
	userValue.sival_ptr = NULL;

	return send_signal_internal(id, signalNumber, userValue, flags);
}


int
send_signal(pid_t threadID, uint signal)
{
	// The BeBook states that this function wouldn't be exported
	// for drivers, but, of course, it's wrong.
	return send_signal_etc(threadID, signal, 0);
}


static int
sigprocmask_internal(int how, const sigset_t* set, sigset_t* oldSet)
{
	Thread* thread = thread_get_current_thread();

	InterruptsSpinLocker schedulerLocker(gSchedulerLock);

	sigset_t oldMask = thread->sig_block_mask;

	if (set != NULL) {
		T(SigProcMask(how, *set));

		switch (how) {
			case SIG_BLOCK:
				thread->sig_block_mask |= *set & BLOCKABLE_SIGNALS;
				break;
			case SIG_UNBLOCK:
				thread->sig_block_mask &= ~*set;
				break;
			case SIG_SETMASK:
				thread->sig_block_mask = *set & BLOCKABLE_SIGNALS;
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
sigprocmask(int how, const sigset_t* set, sigset_t* oldSet)
{
	RETURN_AND_SET_ERRNO(sigprocmask_internal(how, set, oldSet));
}


/*!	\brief Like sigaction(), but returning the error instead of setting errno.
*/
static status_t
sigaction_internal(int signal, const struct sigaction* act,
	struct sigaction* oldAction)
{
	if (signal < 1 || signal > MAX_SIGNAL_NUMBER
		|| (SIGNAL_TO_MASK(signal) & ~BLOCKABLE_SIGNALS) != 0)
		return B_BAD_VALUE;

	// get and lock the team
	Team* team = thread_get_current_thread()->team;
	TeamLocker teamLocker(team);

	struct sigaction& teamHandler = team->SignalActionFor(signal);
	if (oldAction) {
		// save previous sigaction structure
		*oldAction = teamHandler;
	}

	if (act) {
		T(SigAction(signal, act));

		// set new sigaction structure
		teamHandler = *act;
		teamHandler.sa_mask &= BLOCKABLE_SIGNALS;
	}

	// Remove pending signal if it should now be ignored and remove pending
	// signal for those signals whose default action is to ignore them.
	if ((act && act->sa_handler == SIG_IGN)
		|| (act && act->sa_handler == SIG_DFL
			&& (SIGNAL_TO_MASK(signal) & DEFAULT_IGNORE_SIGNALS) != 0)) {
		InterruptsSpinLocker schedulerLocker(gSchedulerLock);

		team->RemovePendingSignal(signal);

		for (Thread* thread = team->thread_list; thread != NULL;
				thread = thread->team_next) {
			thread->RemovePendingSignal(signal);
		}
	}

	return B_OK;
}


int
sigaction(int signal, const struct sigaction* act, struct sigaction* oldAction)
{
	RETURN_AND_SET_ERRNO(sigaction_internal(signal, act, oldAction));
}


/*!	Wait for the specified signals, and return the information for the retrieved
	signal in \a info.
	The \c flags and \c timeout combination must either define an infinite
	timeout (no timeout flags set), an absolute timeout (\c B_ABSOLUTE_TIMEOUT
	set), or a relative timeout \code <= 0 \endcode (\c B_RELATIVE_TIMEOUT set).
*/
static status_t
sigwait_internal(const sigset_t* set, siginfo_t* info, uint32 flags,
	bigtime_t timeout)
{
	// restrict mask to blockable signals
	sigset_t requestedSignals = *set & BLOCKABLE_SIGNALS;

	// make always interruptable
	flags |= B_CAN_INTERRUPT;

	// check whether we are allowed to wait at all
	bool canWait = (flags & B_RELATIVE_TIMEOUT) == 0 || timeout > 0;

	Thread* thread = thread_get_current_thread();

	InterruptsSpinLocker schedulerLocker(gSchedulerLock);

	bool timedOut = false;
	status_t error = B_OK;

	while (!timedOut) {
		sigset_t pendingSignals = thread->AllPendingSignals();

		// If a kill signal is pending, just bail out.
		if ((pendingSignals & KILL_SIGNALS) != 0)
			return B_INTERRUPTED;

		if ((pendingSignals & requestedSignals) != 0) {
			// get signal with the highest priority
			Signal stackSignal;
			Signal* signal = dequeue_thread_or_team_signal(thread,
				requestedSignals, stackSignal);
			ASSERT(signal != NULL);

			SignalHandledCaller signalHandledCaller(signal);
			schedulerLocker.Unlock();

			info->si_signo = signal->Number();
			info->si_code = signal->SignalCode();
			info->si_errno = signal->ErrorCode();
			info->si_pid = signal->SendingProcess();
			info->si_uid = signal->SendingUser();
			info->si_addr = signal->Address();
			info->si_status = signal->Status();
			info->si_band = signal->PollBand();
			info->si_value = signal->UserValue();

			return B_OK;
		}

		if (!canWait)
			return B_WOULD_BLOCK;

		sigset_t blockedSignals = thread->sig_block_mask;
		if ((pendingSignals & ~blockedSignals) != 0) {
			// Non-blocked signals are pending -- return to let them be handled.
			return B_INTERRUPTED;
		}

		// No signals yet. Set the signal block mask to not include the
		// requested mask and wait until we're interrupted.
		thread->sig_block_mask = blockedSignals & ~requestedSignals;

		while (!has_signals_pending(thread)) {
			thread_prepare_to_block(thread, flags, THREAD_BLOCK_TYPE_SIGNAL,
				NULL);

			if ((flags & B_ABSOLUTE_TIMEOUT) != 0) {
				error = thread_block_with_timeout_locked(flags, timeout);
				if (error == B_WOULD_BLOCK || error == B_TIMED_OUT) {
					error = B_WOULD_BLOCK;
						// POSIX requires EAGAIN (B_WOULD_BLOCK) on timeout
					timedOut = true;
					break;
				}
			} else
				thread_block_locked(thread);
		}

		// restore the original block mask
		thread->sig_block_mask = blockedSignals;

		update_current_thread_signals_flag();
	}

	// we get here only when timed out
	return error;
}


/*!	Replace the current signal block mask and wait for any event to happen.
	Before returning, the original signal block mask is reinstantiated.
*/
static status_t
sigsuspend_internal(const sigset_t* _mask)
{
	sigset_t mask = *_mask & BLOCKABLE_SIGNALS;

	T(SigSuspend(mask));

	Thread* thread = thread_get_current_thread();

	InterruptsSpinLocker schedulerLocker(gSchedulerLock);

	// Set the new block mask and block until interrupted. We might be here
	// after a syscall restart, in which case sigsuspend_original_unblocked_mask
	// will still be set.
	sigset_t oldMask = thread->sigsuspend_original_unblocked_mask != 0
		? ~thread->sigsuspend_original_unblocked_mask : thread->sig_block_mask;
	thread->sig_block_mask = mask & BLOCKABLE_SIGNALS;

	update_current_thread_signals_flag();

	while (!has_signals_pending(thread)) {
		thread_prepare_to_block(thread, B_CAN_INTERRUPT,
			THREAD_BLOCK_TYPE_SIGNAL, NULL);
		thread_block_locked(thread);
	}

	// Set sigsuspend_original_unblocked_mask (guaranteed to be non-0 due to
	// BLOCKABLE_SIGNALS). This will indicate to handle_signals() that it is
	// called after a _user_sigsuspend(). It will reset the field after invoking
	// a signal handler, or restart the syscall, if there wasn't anything to
	// handle anymore (e.g. because another thread was faster).
	thread->sigsuspend_original_unblocked_mask = ~oldMask;

	T(SigSuspendDone());

	// we're not supposed to actually succeed
	return B_INTERRUPTED;
}


static status_t
sigpending_internal(sigset_t* set)
{
	Thread* thread = thread_get_current_thread();

	if (set == NULL)
		return B_BAD_VALUE;

	InterruptsSpinLocker schedulerLocker(gSchedulerLock);

	*set = thread->AllPendingSignals() & thread->sig_block_mask;

	return B_OK;
}


// #pragma mark - syscalls


/*!	Sends a signal to a thread, process, or process group.
	\param id Specifies the ID of the target:
		- \code id > 0 \endcode: If \a toThread is \c true, the target is the
			thread with ID \a id, otherwise the team with the ID \a id.
		- \code id == 0 \endcode: If toThread is \c true, the target is the
			current thread, otherwise the current team.
		- \code id == -1 \endcode: The target are all teams the current team has
			permission to send signals to. Currently not implemented correctly.
		- \code id < -1 \endcode: The target are is the process group with ID
			\c -id.
	\param signalNumber The signal number. \c 0 to just perform checks, but not
		actually send any signal.
	\param userUserValue A user value to be associated with the signal. Might be
		ignored unless signal queuing is forced. Can be \c NULL.
	\param flags A bitwise or of any number of the following:
		- \c SIGNAL_FLAG_QUEUING_REQUIRED: Signal queuing is required. Fail
			instead of falling back to unqueued signals, when queuing isn't
			possible.
		- \c SIGNAL_FLAG_SEND_TO_THREAD: Interpret the the given ID as a
			\c thread_id rather than a \c team_id. Ignored when the \a id is
			\code < 0 \endcode -- then the target is a process group.
	\return \c B_OK on success, another error code otherwise.
*/
status_t
_user_send_signal(int32 id, uint32 signalNumber,
	const union sigval* userUserValue, uint32 flags)
{
	// restrict flags to the allowed ones and add B_CHECK_PERMISSION
	flags &= SIGNAL_FLAG_QUEUING_REQUIRED | SIGNAL_FLAG_SEND_TO_THREAD;
	flags |= B_CHECK_PERMISSION;

	// Copy the user value from userland. If not given, use a dummy value.
	union sigval userValue;
	if (userUserValue != NULL) {
		if (!IS_USER_ADDRESS(userUserValue)
			|| user_memcpy(&userValue, userUserValue, sizeof(userValue))
				!= B_OK) {
			return B_BAD_ADDRESS;
		}
	} else
		userValue.sival_ptr = NULL;

	// If to be sent to a thread, delegate to send_signal_internal(). Also do
	// that when id < 0, since in this case the semantics is the same as well.
	if ((flags & SIGNAL_FLAG_SEND_TO_THREAD) != 0 || id < 0)
		return send_signal_internal(id, signalNumber, userValue, flags);

	// kill() semantics for id >= 0
	if (signalNumber > MAX_SIGNAL_NUMBER)
		return B_BAD_VALUE;

	Thread* thread = thread_get_current_thread();

	Signal signal(signalNumber,
		(flags & SIGNAL_FLAG_QUEUING_REQUIRED) != 0 ? SI_QUEUE : SI_USER,
		B_OK, thread->team->id);
	signal.SetUserValue(userValue);

	// send to current team for id == 0, otherwise to the respective team
	return send_signal_to_team_id(id == 0 ? team_get_current_team_id() : id,
		signal, flags);
}


status_t
_user_set_signal_mask(int how, const sigset_t *userSet, sigset_t *userOldSet)
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

	status = sigaction_internal(signal, userAction ? &act : NULL,
		userOldAction ? &oact : NULL);

	// only copy the old action if a pointer has been given
	if (status >= B_OK && userOldAction != NULL
		&& user_memcpy(userOldAction, &oact, sizeof(struct sigaction)) < B_OK)
		return B_BAD_ADDRESS;

	return status;
}


status_t
_user_sigwait(const sigset_t *userSet, siginfo_t *userInfo, uint32 flags,
	bigtime_t timeout)
{
	// copy userSet to stack
	sigset_t set;
	if (userSet == NULL || !IS_USER_ADDRESS(userSet)
		|| user_memcpy(&set, userSet, sizeof(sigset_t)) != B_OK) {
		return B_BAD_ADDRESS;
	}

	// userInfo is optional, but must be a user address when given
	if (userInfo != NULL && !IS_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;

	syscall_restart_handle_timeout_pre(flags, timeout);

	flags |= B_CAN_INTERRUPT;

	siginfo_t info;
	status_t status = sigwait_internal(&set, &info, flags, timeout);
	if (status == B_OK) {
		// copy the info back to userland, if userSet is non-NULL
		if (userInfo != NULL)
			status = user_memcpy(userInfo, &info, sizeof(info));
	} else if (status == B_INTERRUPTED) {
		// make sure we'll be restarted
		Thread* thread = thread_get_current_thread();
		atomic_or(&thread->flags, THREAD_FLAGS_ALWAYS_RESTART_SYSCALL);
	}

	return syscall_restart_handle_timeout_post(status, timeout);
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
_user_set_signal_stack(const stack_t* newUserStack, stack_t* oldUserStack)
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
		// determine whether or not the user thread is currently
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


/*!	Restores the environment of a function that was interrupted by a signal
	handler call.
	This syscall is invoked when a signal handler function returns. It
	deconstructs the signal handler frame and restores the stack and register
	state of the function that was interrupted by a signal. The syscall is
	therefore somewhat unusual, since it does not return to the calling
	function, but to someplace else. In case the signal interrupted a syscall,
	it will appear as if the syscall just returned. That is also the reason, why
	this syscall returns an int64, since it needs to return the value the
	interrupted syscall returns, which is potentially 64 bits wide.

	\param userSignalFrameData The signal frame data created for the signal
		handler. Potentially some data (e.g. registers) have been modified by
		the signal handler.
	\return In case the signal interrupted a syscall, the return value of that
		syscall. Otherwise (i.e. in case of a (hardware) interrupt/exception)
		the value might need to be tailored such that after a return to userland
		the interrupted environment is identical to the interrupted one (unless
		explicitly modified). E.g. for x86 to achieve that, the return value
		must contain the eax|edx values of the interrupted environment.
*/
int64
_user_restore_signal_frame(struct signal_frame_data* userSignalFrameData)
{
	syscall_64_bit_return_value();

	Thread *thread = thread_get_current_thread();

	// copy the signal frame data from userland
	signal_frame_data signalFrameData;
	if (userSignalFrameData == NULL || !IS_USER_ADDRESS(userSignalFrameData)
		|| user_memcpy(&signalFrameData, userSignalFrameData,
			sizeof(signalFrameData)) != B_OK) {
		// We failed to copy the signal frame data from userland. This is a
		// serious problem. Kill the thread.
		dprintf("_user_restore_signal_frame(): thread %" B_PRId32 ": Failed to "
			"copy signal frame data (%p) from userland. Killing thread...\n",
			thread->id, userSignalFrameData);
		kill_thread(thread->id);
		return B_BAD_ADDRESS;
	}

	// restore the signal block mask
	InterruptsSpinLocker schedulerLocker(gSchedulerLock);

	thread->sig_block_mask
		= signalFrameData.context.uc_sigmask & BLOCKABLE_SIGNALS;
	update_current_thread_signals_flag();

	schedulerLocker.Unlock();

	// restore the syscall restart related thread flags and the syscall restart
	// parameters
	atomic_and(&thread->flags,
		~(THREAD_FLAGS_RESTART_SYSCALL | THREAD_FLAGS_64_BIT_SYSCALL_RETURN));
	atomic_or(&thread->flags, signalFrameData.thread_flags
		& (THREAD_FLAGS_RESTART_SYSCALL | THREAD_FLAGS_64_BIT_SYSCALL_RETURN));

	memcpy(thread->syscall_restart.parameters,
		signalFrameData.syscall_restart_parameters,
		sizeof(thread->syscall_restart.parameters));

	// restore the previously stored Thread::user_signal_context
	thread->user_signal_context = signalFrameData.context.uc_link;
	if (thread->user_signal_context != NULL
		&& !IS_USER_ADDRESS(thread->user_signal_context)) {
		thread->user_signal_context = NULL;
	}

	// let the architecture specific code restore the registers
	return arch_restore_signal_frame(&signalFrameData);
}
