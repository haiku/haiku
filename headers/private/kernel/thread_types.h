/*
 * Copyright 2004-2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Thread definition and structures
 */
#ifndef _KERNEL_THREAD_TYPES_H
#define _KERNEL_THREAD_TYPES_H


#ifndef _ASSEMBLER

#include <pthread.h>

#include <arch/thread_types.h>
#include <condition_variable.h>
#include <heap.h>
#include <ksignal.h>
#include <lock.h>
#include <smp.h>
#include <thread_defs.h>
#include <timer.h>
#include <UserTimer.h>
#include <user_debugger.h>
#include <util/DoublyLinkedList.h>
#include <util/KernelReferenceable.h>
#include <util/list.h>


enum additional_thread_state {
	THREAD_STATE_FREE_ON_RESCHED = 7, // free the thread structure upon reschedule
//	THREAD_STATE_BIRTH	// thread is being created
};

#define THREAD_MIN_SET_PRIORITY				B_LOWEST_ACTIVE_PRIORITY
#define THREAD_MAX_SET_PRIORITY				B_REAL_TIME_PRIORITY

enum team_state {
	TEAM_STATE_NORMAL,		// normal state
	TEAM_STATE_BIRTH,		// being constructed
	TEAM_STATE_SHUTDOWN,	// still lives, but is going down
	TEAM_STATE_DEATH		// only the Team object still exists, threads are
							// gone
};

#define	TEAM_FLAG_EXEC_DONE	0x01

typedef enum job_control_state {
	JOB_CONTROL_STATE_NONE,
	JOB_CONTROL_STATE_STOPPED,
	JOB_CONTROL_STATE_CONTINUED,
	JOB_CONTROL_STATE_DEAD
} job_control_state;


struct cpu_ent;
struct image;					// defined in image.c
struct io_context;
struct realtime_sem_context;	// defined in realtime_sem.cpp
struct scheduler_thread_data;
struct select_info;
struct user_thread;				// defined in libroot/user_thread.h
struct VMAddressSpace;
struct xsi_sem_context;			// defined in xsi_semaphore.cpp

namespace BKernel {
	struct Team;
	struct Thread;
	struct ProcessGroup;
}


struct thread_death_entry {
	struct list_link	link;
	thread_id			thread;
	status_t			status;
};

struct team_loading_info {
	Thread*				thread;	// the waiting thread
	status_t			result;		// the result of the loading
	bool				done;		// set when loading is done/aborted
};

struct team_watcher {
	struct list_link	link;
	void				(*hook)(team_id team, void *data);
	void				*data;
};


#define MAX_DEAD_CHILDREN	32
	// this is a soft limit for the number of child death entries in a team
#define MAX_DEAD_THREADS	32
	// this is a soft limit for the number of thread death entries in a team


struct job_control_entry : DoublyLinkedListLinkImpl<job_control_entry> {
	job_control_state	state;		// current team job control state
	thread_id			thread;		// main thread ID == team ID
	uint16				signal;		// signal causing the current state
	bool				has_group_ref;
	uid_t				signaling_user;

	// valid while state != JOB_CONTROL_STATE_DEAD
	BKernel::Team*		team;

	// valid when state == JOB_CONTROL_STATE_DEAD
	pid_t				group_id;
	status_t			status;
	uint16				reason;		// reason for the team's demise, one of the
									// CLD_* values defined in <signal.h>

	job_control_entry();
	~job_control_entry();

	void InitDeadState();

	job_control_entry& operator=(const job_control_entry& other);
};

typedef DoublyLinkedList<job_control_entry> JobControlEntryList;

struct team_job_control_children {
	JobControlEntryList		entries;
};

struct team_dead_children : team_job_control_children {
	ConditionVariable	condition_variable;
	uint32				count;
	bigtime_t			kernel_time;
	bigtime_t			user_time;
};


struct team_death_entry {
	int32				remaining_threads;
	ConditionVariable	condition;
};


struct free_user_thread {
	struct free_user_thread*	next;
	struct user_thread*			thread;
};


class AssociatedDataOwner;

class AssociatedData : public BReferenceable,
	public DoublyLinkedListLinkImpl<AssociatedData> {
public:
								AssociatedData();
	virtual						~AssociatedData();

			AssociatedDataOwner* Owner() const
									{ return fOwner; }
			void				SetOwner(AssociatedDataOwner* owner)
									{ fOwner = owner; }

	virtual	void				OwnerDeleted(AssociatedDataOwner* owner);

private:
			AssociatedDataOwner* fOwner;
};


class AssociatedDataOwner {
public:
								AssociatedDataOwner();
								~AssociatedDataOwner();

			bool				AddData(AssociatedData* data);
			bool				RemoveData(AssociatedData* data);

			void				PrepareForDeletion();

private:
			typedef DoublyLinkedList<AssociatedData> DataList;

private:

			mutex				fLock;
			DataList			fList;
};


typedef int32 (*thread_entry_func)(thread_func, void *);

typedef bool (*page_fault_callback)(addr_t address, addr_t faultAddress,
	bool isWrite);


namespace BKernel {


template<typename IDType>
struct TeamThreadIteratorEntry
	: DoublyLinkedListLinkImpl<TeamThreadIteratorEntry<IDType> > {
	typedef IDType	id_type;
	typedef TeamThreadIteratorEntry<id_type> iterator_type;

	id_type	id;			// -1 for iterator entries, >= 0 for actual elements
	bool	visible;	// the entry is publicly visible
};


struct Team : TeamThreadIteratorEntry<team_id>, KernelReferenceable,
		AssociatedDataOwner {
	DoublyLinkedListLink<Team>	global_list_link;
	Team			*hash_next;		// next in hash
	Team			*siblings_next;	// next in parent's list; protected by
									// parent's fLock
	Team			*parent;		// write-protected by both parent (if any)
									// and this team's fLock
	Team			*children;		// protected by this team's fLock;
									// adding/removing a child also requires the
									// child's fLock
	Team			*group_next;	// protected by the group's lock

	int64			serial_number;	// immutable after adding team to hash

	// process group info -- write-protected by both the group's lock, the
	// team's lock, and the team's parent's lock
	pid_t			group_id;
	pid_t			session_id;
	ProcessGroup	*group;

	int				num_threads;	// number of threads in this team
	int				state;			// current team state, see above
	int32			flags;
	struct io_context *io_context;
	struct realtime_sem_context	*realtime_sem_context;
	struct xsi_sem_context *xsi_sem_context;
	struct team_death_entry *death_entry;	// protected by fLock
	struct list		dead_threads;
	int				dead_threads_count;

	// protected by the team's fLock
	team_dead_children dead_children;
	team_job_control_children stopped_children;
	team_job_control_children continued_children;

	// protected by the parent team's fLock
	struct job_control_entry* job_control_entry;

	VMAddressSpace	*address_space;
	Thread			*main_thread;	// protected by fLock and the scheduler
									// lock (and the thread's lock), immutable
									// after first set
	Thread			*thread_list;	// protected by fLock and the scheduler lock
	struct team_loading_info *loading_info;	// protected by fLock
	struct list		image_list;		// protected by sImageMutex
	struct list		watcher_list;
	struct list		sem_list;		// protected by sSemsSpinlock
	struct list		port_list;		// protected by sPortsLock
	struct arch_team arch_info;

	addr_t			user_data;
	area_id			user_data_area;
	size_t			user_data_size;
	size_t			used_user_data;
	struct free_user_thread* free_user_threads;

	struct team_debug_info debug_info;

	// protected by scheduler lock
	bigtime_t		dead_threads_kernel_time;
	bigtime_t		dead_threads_user_time;
	bigtime_t		cpu_clock_offset;

	// user group information; protected by fLock, the *_uid/*_gid fields also
	// by the scheduler lock
	uid_t			saved_set_uid;
	uid_t			real_uid;
	uid_t			effective_uid;
	gid_t			saved_set_gid;
	gid_t			real_gid;
	gid_t			effective_gid;
	gid_t*			supplementary_groups;
	int				supplementary_group_count;

	// Exit status information. Set when the first terminal event occurs,
	// immutable afterwards. Protected by fLock.
	struct {
		uint16		reason;			// reason for the team's demise, one of the
									// CLD_* values defined in <signal.h>
		uint16		signal;			// signal killing the team
		uid_t		signaling_user;	// real UID of the signal sender
		status_t	status;			// exit status, if normal team exit
		bool		initialized;	// true when the state has been initialized
	} exit;

public:
								~Team();

	static	Team*				Create(team_id id, const char* name,
									bool kernel);
	static	Team*				Get(team_id id);
	static	Team*				GetAndLock(team_id id);

			bool				Lock()
									{ mutex_lock(&fLock); return true; }
			bool				TryLock()
									{ return mutex_trylock(&fLock) == B_OK; }
			void				Unlock()
									{ mutex_unlock(&fLock); }

			void				UnlockAndReleaseReference()
									{ Unlock(); ReleaseReference(); }

			void				LockTeamAndParent(bool dontLockParentIfKernel);
			void				UnlockTeamAndParent();
			void				LockTeamAndProcessGroup();
			void				UnlockTeamAndProcessGroup();
			void				LockTeamParentAndProcessGroup();
			void				UnlockTeamParentAndProcessGroup();
			void				LockProcessGroup()
									{ LockTeamAndProcessGroup(); Unlock(); }

			const char*			Name() const	{ return fName; }
			void				SetName(const char* name);

			const char*			Args() const	{ return fArgs; }
			void				SetArgs(const char* args);
			void				SetArgs(const char* path,
									const char* const* otherArgs,
									int otherArgCount);

			BKernel::QueuedSignalsCounter* QueuedSignalsCounter() const
									{ return fQueuedSignalsCounter; }
			sigset_t			PendingSignals() const
									{ return fPendingSignals.AllSignals(); }

			void				AddPendingSignal(int signal)
									{ fPendingSignals.AddSignal(signal); }
			void				AddPendingSignal(Signal* signal)
									{ fPendingSignals.AddSignal(signal); }
			void				RemovePendingSignal(int signal)
									{ fPendingSignals.RemoveSignal(signal); }
			void				RemovePendingSignal(Signal* signal)
									{ fPendingSignals.RemoveSignal(signal); }
			void				RemovePendingSignals(sigset_t mask)
									{ fPendingSignals.RemoveSignals(mask); }
			void				ResetSignalsOnExec();

	inline	int32				HighestPendingSignalPriority(
									sigset_t nonBlocked) const;
	inline	Signal*				DequeuePendingSignal(sigset_t nonBlocked,
									Signal& buffer);

			struct sigaction&	SignalActionFor(int32 signal)
									{ return fSignalActions[signal - 1]; }
			void				InheritSignalActions(Team* parent);

			// user timers -- protected by fLock
			UserTimer*			UserTimerFor(int32 id) const
									{ return fUserTimers.TimerFor(id); }
			status_t			AddUserTimer(UserTimer* timer);
			void				RemoveUserTimer(UserTimer* timer);
			void				DeleteUserTimers(bool userDefinedOnly);

			bool				CheckAddUserDefinedTimer();
			void				UserDefinedTimersRemoved(int32 count);

			void				UserTimerActivated(TeamTimeUserTimer* timer)
									{ fCPUTimeUserTimers.Add(timer); }
			void				UserTimerActivated(TeamUserTimeUserTimer* timer)
									{ fUserTimeUserTimers.Add(timer); }
			void				UserTimerDeactivated(TeamTimeUserTimer* timer)
									{ fCPUTimeUserTimers.Remove(timer); }
			void				UserTimerDeactivated(
									TeamUserTimeUserTimer* timer)
									{ fUserTimeUserTimers.Remove(timer); }
			void				DeactivateCPUTimeUserTimers();
									// both total and user CPU timers
			bool				HasActiveCPUTimeUserTimers() const
									{ return !fCPUTimeUserTimers.IsEmpty(); }
			bool				HasActiveUserTimeUserTimers() const
									{ return !fUserTimeUserTimers.IsEmpty(); }
			TeamTimeUserTimerList::ConstIterator
									CPUTimeUserTimerIterator() const
									{ return fCPUTimeUserTimers.GetIterator(); }
	inline	TeamUserTimeUserTimerList::ConstIterator
									UserTimeUserTimerIterator() const;

			bigtime_t			CPUTime(bool ignoreCurrentRun) const;
			bigtime_t			UserCPUTime() const;

private:
								Team(team_id id, bool kernel);

private:
			mutex				fLock;
			char				fName[B_OS_NAME_LENGTH];
			char				fArgs[64];
									// contents for the team_info::args field

			BKernel::QueuedSignalsCounter* fQueuedSignalsCounter;
			BKernel::PendingSignals	fPendingSignals;
									// protected by scheduler lock
			struct sigaction 	fSignalActions[MAX_SIGNAL_NUMBER];
									// indexed signal - 1, protected by fLock

			UserTimerList		fUserTimers;			// protected by fLock
			TeamTimeUserTimerList fCPUTimeUserTimers;
									// protected by scheduler lock
			TeamUserTimeUserTimerList fUserTimeUserTimers;
			vint32				fUserDefinedTimerCount;	// accessed atomically
};


struct Thread : TeamThreadIteratorEntry<thread_id>, KernelReferenceable {
	int32			flags;			// summary of events relevant in interrupt
									// handlers (signals pending, user debugging
									// enabled, etc.)
	int64			serial_number;	// immutable after adding thread to hash
	Thread			*hash_next;		// protected by thread hash lock
	Thread			*team_next;		// protected by team lock and fLock
	Thread			*queue_next;	// protected by scheduler lock
	timer			alarm;			// protected by scheduler lock
	char			name[B_OS_NAME_LENGTH];	// protected by fLock
	int32			priority;		// protected by scheduler lock
	int32			next_priority;	// protected by scheduler lock
	int32			io_priority;	// protected by fLock
	int32			state;			// protected by scheduler lock
	int32			next_state;		// protected by scheduler lock
	struct cpu_ent	*cpu;			// protected by scheduler lock
	struct cpu_ent	*previous_cpu;	// protected by scheduler lock
	int32			pinned_to_cpu;	// only accessed by this thread or in the
									// scheduler, when thread is not running

	sigset_t		sig_block_mask;	// protected by scheduler lock,
									// only modified by the thread itself
	sigset_t		sigsuspend_original_unblocked_mask;
		// non-0 after a return from _user_sigsuspend(), containing the inverted
		// original signal mask, reset in handle_signals(); only accessed by
		// this thread
	ucontext_t*		user_signal_context;	// only accessed by this thread
	addr_t			signal_stack_base;		// only accessed by this thread
	size_t			signal_stack_size;		// only accessed by this thread
	bool			signal_stack_enabled;	// only accessed by this thread

	bool			in_kernel;		// protected by time_lock, only written by
									// this thread
	bool			was_yielded;	// protected by scheduler lock
	struct scheduler_thread_data* scheduler_data; // protected by scheduler lock

	struct user_thread*	user_thread;	// write-protected by fLock, only
										// modified by the thread itself and
										// thus freely readable by it

	void 			(*cancel_function)(int);

	struct {
		uint8		parameters[SYSCALL_RESTART_PARAMETER_SIZE];
	} syscall_restart;

	struct {
		status_t	status;				// current wait status
		uint32		flags;				// interrupable flags
		uint32		type;				// type of the object waited on
		const void*	object;				// pointer to the object waited on
		timer		unblock_timer;		// timer for block with timeout
	} wait;

	struct PrivateConditionVariableEntry *condition_variable_entry;

	struct {
		sem_id		write_sem;	// acquired by writers before writing
		sem_id		read_sem;	// release by writers after writing, acquired
								// by this thread when reading
		thread_id	sender;
		int32		code;
		size_t		size;
		void*		buffer;
	} msg;	// write_sem/read_sem are protected by fLock when accessed by
			// others, the other fields are protected by write_sem/read_sem

	union {
		addr_t		fault_handler;
		page_fault_callback fault_callback;
			// TODO: this is a temporary field used for the vm86 implementation
			// and should be removed again when that one is moved into the
			// kernel entirely.
	};
	int32			page_faults_allowed;
		/* this field may only stay in debug builds in the future */

	BKernel::Team	*team;	// protected by team lock, thread lock, scheduler
							// lock

	struct {
		sem_id		sem;		// immutable after thread creation
		status_t	status;		// accessed only by this thread
		struct list	waiters;	// protected by fLock
	} exit;

	struct select_info *select_infos;	// protected by fLock

	struct thread_debug_info debug_info;

	// stack
	area_id			kernel_stack_area;	// immutable after thread creation
	addr_t			kernel_stack_base;	// immutable after thread creation
	addr_t			kernel_stack_top;	// immutable after thread creation
	area_id			user_stack_area;	// protected by thread lock
	addr_t			user_stack_base;	// protected by thread lock
	size_t			user_stack_size;	// protected by thread lock

	addr_t			user_local_storage;
		// usually allocated at the safe side of the stack
	int				kernel_errno;
		// kernel "errno" differs from its userspace alter ego

	// user_time, kernel_time, and last_time are only written by the thread
	// itself, so they can be read by the thread without lock. Holding the
	// scheduler lock and checking that the thread does not run also guarantees
	// that the times will not change.
	spinlock		time_lock;
	bigtime_t		user_time;			// protected by time_lock
	bigtime_t		kernel_time;		// protected by time_lock
	bigtime_t		last_time;			// protected by time_lock
	bigtime_t		cpu_clock_offset;	// protected by scheduler lock

	void			(*post_interrupt_callback)(void*);
	void*			post_interrupt_data;

	// architecture dependent section
	struct arch_thread arch_info;

public:
								Thread() {}
									// dummy for the idle threads
								Thread(const char *name, thread_id threadID,
									struct cpu_ent *cpu);
								~Thread();

	static	status_t			Create(const char* name, Thread*& _thread);

	static	Thread*				Get(thread_id id);
	static	Thread*				GetAndLock(thread_id id);
	static	Thread*				GetDebug(thread_id id);
									// in kernel debugger only

	static	bool				IsAlive(thread_id id);

			void*				operator new(size_t size);
			void*				operator new(size_t, void* pointer);
			void				operator delete(void* pointer, size_t size);

			status_t			Init(bool idleThread);

			bool				Lock()
									{ mutex_lock(&fLock); return true; }
			bool				TryLock()
									{ return mutex_trylock(&fLock) == B_OK; }
			void				Unlock()
									{ mutex_unlock(&fLock); }

			void				UnlockAndReleaseReference()
									{ Unlock(); ReleaseReference(); }

			bool				IsAlive() const;

			bool				IsRunning() const
									{ return cpu != NULL; }
									// scheduler lock must be held

			sigset_t			ThreadPendingSignals() const
									{ return fPendingSignals.AllSignals(); }
	inline	sigset_t			AllPendingSignals() const;
			void				AddPendingSignal(int signal)
									{ fPendingSignals.AddSignal(signal); }
			void				AddPendingSignal(Signal* signal)
									{ fPendingSignals.AddSignal(signal); }
			void				RemovePendingSignal(int signal)
									{ fPendingSignals.RemoveSignal(signal); }
			void				RemovePendingSignal(Signal* signal)
									{ fPendingSignals.RemoveSignal(signal); }
			void				RemovePendingSignals(sigset_t mask)
									{ fPendingSignals.RemoveSignals(mask); }
			void				ResetSignalsOnExec();

	inline	int32				HighestPendingSignalPriority(
									sigset_t nonBlocked) const;
	inline	Signal*				DequeuePendingSignal(sigset_t nonBlocked,
									Signal& buffer);

			// user timers -- protected by fLock
			UserTimer*			UserTimerFor(int32 id) const
									{ return fUserTimers.TimerFor(id); }
			status_t			AddUserTimer(UserTimer* timer);
			void				RemoveUserTimer(UserTimer* timer);
			void				DeleteUserTimers(bool userDefinedOnly);

			void				UserTimerActivated(ThreadTimeUserTimer* timer)
									{ fCPUTimeUserTimers.Add(timer); }
			void				UserTimerDeactivated(ThreadTimeUserTimer* timer)
									{ fCPUTimeUserTimers.Remove(timer); }
			void				DeactivateCPUTimeUserTimers();
			bool				HasActiveCPUTimeUserTimers() const
									{ return !fCPUTimeUserTimers.IsEmpty(); }
			ThreadTimeUserTimerList::ConstIterator
									CPUTimeUserTimerIterator() const
									{ return fCPUTimeUserTimers.GetIterator(); }

	inline	bigtime_t			CPUTime(bool ignoreCurrentRun) const;

private:
			mutex				fLock;

			BKernel::PendingSignals	fPendingSignals;
									// protected by scheduler lock

			UserTimerList		fUserTimers;			// protected by fLock
			ThreadTimeUserTimerList fCPUTimeUserTimers;
									// protected by scheduler lock
};


struct ProcessSession : BReferenceable {
	pid_t				id;
	int32				controlling_tty;	// index of the controlling tty,
											// -1 if none
	pid_t				foreground_group;

public:
								ProcessSession(pid_t id);
								~ProcessSession();

			bool				Lock()
									{ mutex_lock(&fLock); return true; }
			bool				TryLock()
									{ return mutex_trylock(&fLock) == B_OK; }
			void				Unlock()
									{ mutex_unlock(&fLock); }

private:
			mutex				fLock;
};


struct ProcessGroup : KernelReferenceable {
	struct ProcessGroup *next;		// next in hash
	pid_t				id;
	BKernel::Team		*teams;

public:
								ProcessGroup(pid_t id);
								~ProcessGroup();

	static	ProcessGroup*		Get(pid_t id);

			bool				Lock()
									{ mutex_lock(&fLock); return true; }
			bool				TryLock()
									{ return mutex_trylock(&fLock) == B_OK; }
			void				Unlock()
									{ mutex_unlock(&fLock); }

			ProcessSession*		Session() const
									{ return fSession; }
			void				Publish(ProcessSession* session);
			void				PublishLocked(ProcessSession* session);

			bool				IsOrphaned() const;

			void				ScheduleOrphanedCheck();
			void				UnsetOrphanedCheck();

public:
			SinglyLinkedListLink<ProcessGroup> fOrphanedCheckListLink;

private:
			mutex				fLock;
			ProcessSession*		fSession;
			bool				fInOrphanedCheckList;	// protected by
														// sOrphanedCheckLock
};

typedef SinglyLinkedList<ProcessGroup,
	SinglyLinkedListMemberGetLink<ProcessGroup,
		&ProcessGroup::fOrphanedCheckListLink> > ProcessGroupList;


/*!	\brief Allows to iterate through all teams.
*/
struct TeamListIterator {
								TeamListIterator();
								~TeamListIterator();

			Team*				Next();

private:
			TeamThreadIteratorEntry<team_id> fEntry;
};


/*!	\brief Allows to iterate through all threads.
*/
struct ThreadListIterator {
								ThreadListIterator();
								~ThreadListIterator();

			Thread*				Next();

private:
			TeamThreadIteratorEntry<thread_id> fEntry;
};


inline int32
Team::HighestPendingSignalPriority(sigset_t nonBlocked) const
{
	return fPendingSignals.HighestSignalPriority(nonBlocked);
}


inline Signal*
Team::DequeuePendingSignal(sigset_t nonBlocked, Signal& buffer)
{
	return fPendingSignals.DequeueSignal(nonBlocked, buffer);
}


inline TeamUserTimeUserTimerList::ConstIterator
Team::UserTimeUserTimerIterator() const
{
	return fUserTimeUserTimers.GetIterator();
}


inline sigset_t
Thread::AllPendingSignals() const
{
	return fPendingSignals.AllSignals() | team->PendingSignals();
}


inline int32
Thread::HighestPendingSignalPriority(sigset_t nonBlocked) const
{
	return fPendingSignals.HighestSignalPriority(nonBlocked);
}


inline Signal*
Thread::DequeuePendingSignal(sigset_t nonBlocked, Signal& buffer)
{
	return fPendingSignals.DequeueSignal(nonBlocked, buffer);
}


/*!	Returns the thread's current total CPU time (kernel + user + offset).

	The caller must hold the scheduler lock.

	\param ignoreCurrentRun If \c true and the thread is currently running,
		don't add the time since the last time \c last_time was updated. Should
		be used in "thread unscheduled" scheduler callbacks, since although the
		thread is still running at that time, its time has already been stopped.
	\return The thread's current total CPU time.
*/
inline bigtime_t
Thread::CPUTime(bool ignoreCurrentRun) const
{
	bigtime_t time = user_time + kernel_time + cpu_clock_offset;

	// If currently running, also add the time since the last check, unless
	// requested otherwise.
	if (!ignoreCurrentRun && cpu != NULL)
		time += system_time() - last_time;

	return time;
}


}	// namespace BKernel

using BKernel::Team;
using BKernel::TeamListIterator;
using BKernel::Thread;
using BKernel::ThreadListIterator;
using BKernel::ProcessSession;
using BKernel::ProcessGroup;
using BKernel::ProcessGroupList;


struct thread_queue {
	Thread*	head;
	Thread*	tail;
};


#endif	// !_ASSEMBLER


// bits for the thread::flags field
#define	THREAD_FLAGS_SIGNALS_PENDING		0x0001
	// unblocked signals are pending (computed flag for optimization purposes)
#define	THREAD_FLAGS_DEBUG_THREAD			0x0002
	// forces the thread into the debugger as soon as possible (set by
	// debug_thread())
#define	THREAD_FLAGS_SINGLE_STEP			0x0004
	// indicates that the thread is in single-step mode (in userland)
#define	THREAD_FLAGS_DEBUGGER_INSTALLED		0x0008
	// a debugger is installed for the current team (computed flag for
	// optimization purposes)
#define	THREAD_FLAGS_BREAKPOINTS_DEFINED	0x0010
	// hardware breakpoints are defined for the current team (computed flag for
	// optimization purposes)
#define	THREAD_FLAGS_BREAKPOINTS_INSTALLED	0x0020
	// breakpoints are currently installed for the thread (i.e. the hardware is
	// actually set up to trigger debug events for them)
#define	THREAD_FLAGS_64_BIT_SYSCALL_RETURN	0x0040
	// set by 64 bit return value syscalls
#define	THREAD_FLAGS_RESTART_SYSCALL		0x0080
	// set by handle_signals(), if the current syscall shall be restarted
#define	THREAD_FLAGS_DONT_RESTART_SYSCALL	0x0100
	// explicitly disables automatic syscall restarts (e.g. resume_thread())
#define	THREAD_FLAGS_ALWAYS_RESTART_SYSCALL	0x0200
	// force syscall restart, even if a signal handler without SA_RESTART was
	// invoked (e.g. sigwait())
#define	THREAD_FLAGS_SYSCALL_RESTARTED		0x0400
	// the current syscall has been restarted
#define	THREAD_FLAGS_SYSCALL				0x0800
	// the thread is currently in a syscall; set/reset only for certain
	// functions (e.g. ioctl()) to allow inner functions to discriminate
	// whether e.g. parameters were passed from userland or kernel


#endif	/* _KERNEL_THREAD_TYPES_H */
