/*
 * Copyright 2004-2010, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Thread definition and structures
 */
#ifndef _KERNEL_THREAD_TYPES_H
#define _KERNEL_THREAD_TYPES_H


#ifndef _ASSEMBLER

#include <arch/thread_types.h>
#include <condition_variable.h>
#include <signal.h>
#include <smp.h>
#include <thread_defs.h>
#include <timer.h>
#include <user_debugger.h>
#include <util/list.h>


extern spinlock gThreadSpinlock;
#define GRAB_THREAD_LOCK()    acquire_spinlock(&gThreadSpinlock)
#define RELEASE_THREAD_LOCK() release_spinlock(&gThreadSpinlock)

extern spinlock gTeamSpinlock;
	// NOTE: TEAM lock can be held over a THREAD lock acquisition,
	// but not the other way (to avoid deadlock)
#define GRAB_TEAM_LOCK()    acquire_spinlock(&gTeamSpinlock)
#define RELEASE_TEAM_LOCK() release_spinlock(&gTeamSpinlock)

enum additional_thread_state {
	THREAD_STATE_FREE_ON_RESCHED = 7, // free the thread structure upon reschedule
//	THREAD_STATE_BIRTH	// thread is being created
};

#define THREAD_MIN_SET_PRIORITY				B_LOWEST_ACTIVE_PRIORITY
#define THREAD_MAX_SET_PRIORITY				B_REAL_TIME_PRIORITY

enum team_state {
	TEAM_STATE_NORMAL,	// normal state
	TEAM_STATE_BIRTH,	// being contructed
	TEAM_STATE_DEATH	// being killed
};

#define	TEAM_FLAG_EXEC_DONE	0x01

typedef enum job_control_state {
	JOB_CONTROL_STATE_NONE,
	JOB_CONTROL_STATE_STOPPED,
	JOB_CONTROL_STATE_CONTINUED,
	JOB_CONTROL_STATE_DEAD
} job_control_state;


struct image;					// defined in image.c
struct io_context;
struct realtime_sem_context;	// defined in realtime_sem.cpp
struct select_info;
struct user_thread;				// defined in libroot/user_thread.h
struct xsi_sem_context;			// defined in xsi_semaphore.cpp

struct death_entry {
	struct list_link	link;
	pid_t				group_id;
	thread_id			thread;
	status_t			status;
	uint16				reason;
	uint16				signal;
};

struct process_session {
	pid_t				id;
	int32				group_count;
	int32				controlling_tty;	// index of the controlling tty,
											// -1 if none
	pid_t				foreground_group;
};

struct process_group {
	struct process_group *next;		// next in hash
	struct process_session *session;
	pid_t				id;
	int32				refs;
	struct team			*teams;
	bool				orphaned;
};

struct team_loading_info {
	struct thread		*thread;	// the waiting thread
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

typedef struct team_dead_children team_dead_children;
typedef struct team_job_control_children  team_job_control_children;
typedef struct job_control_entry job_control_entry;


#ifdef __cplusplus

#include <condition_variable.h>
#include <util/DoublyLinkedList.h>


struct job_control_entry : DoublyLinkedListLinkImpl<job_control_entry> {
	job_control_state	state;		// current team job control state
	thread_id			thread;		// main thread ID == team ID
	bool				has_group_ref;

	// valid while state != JOB_CONTROL_STATE_DEAD
	struct team*		team;

	// valid when state == JOB_CONTROL_STATE_DEAD
	pid_t				group_id;
	status_t			status;
	uint16				reason;
	uint16				signal;

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


#endif	// __cplusplus


struct free_user_thread {
	struct free_user_thread*	next;
	struct user_thread*			thread;
};

struct scheduler_thread_data;

struct team {
	struct team		*next;			// next in hash
	struct team		*siblings_next;
	struct team		*parent;
	struct team		*children;
	struct team		*group_next;
	team_id			id;
	pid_t			group_id;
	pid_t			session_id;
	struct process_group *group;
	char			name[B_OS_NAME_LENGTH];
	char			args[64];		// contents for the team_info::args field
	int				num_threads;	// number of threads in this team
	int				state;			// current team state, see above
	int32			flags;
	struct io_context *io_context;
	struct realtime_sem_context	*realtime_sem_context;
	struct xsi_sem_context *xsi_sem_context;
	struct team_death_entry *death_entry;
	struct list		dead_threads;
	int				dead_threads_count;

	team_dead_children *dead_children;
	team_job_control_children *stopped_children;
	team_job_control_children *continued_children;
	struct job_control_entry* job_control_entry;

	struct VMAddressSpace *address_space;
	struct thread	*main_thread;
	struct thread	*thread_list;
	struct team_loading_info *loading_info;
	struct list		image_list;
	struct list		watcher_list;
	struct list		sem_list;
	struct list		port_list;
	struct arch_team arch_info;

	addr_t			user_data;
	area_id			user_data_area;
	size_t			user_data_size;
	size_t			used_user_data;
	struct free_user_thread* free_user_threads;

	struct team_debug_info debug_info;

	bigtime_t		dead_threads_kernel_time;
	bigtime_t		dead_threads_user_time;

	uid_t			saved_set_uid;
	uid_t			real_uid;
	uid_t			effective_uid;
	gid_t			saved_set_gid;
	gid_t			real_gid;
	gid_t			effective_gid;
	gid_t*			supplementary_groups;
	int				supplementary_group_count;
};

typedef int32 (*thread_entry_func)(thread_func, void *);

typedef bool (*page_fault_callback)(addr_t address, addr_t faultAddress,
	bool isWrite);

struct thread {
	int32			flags;			// summary of events relevant in interrupt
									// handlers (signals pending, user debugging
									// enabled, etc.)
	struct thread	*all_next;
	struct thread	*team_next;
	struct thread	*queue_next;	/* i.e. run queue, release queue, etc. */
	timer			alarm;
	thread_id		id;
	char			name[B_OS_NAME_LENGTH];
	int32			priority;
	int32			next_priority;
	int32			io_priority;
	int32			state;
	int32			next_state;
	struct cpu_ent	*cpu;
	struct cpu_ent	*previous_cpu;
	int32			pinned_to_cpu;

	sigset_t		sig_pending;
	sigset_t		sig_block_mask;
	sigset_t		sig_temp_enabled;
	struct sigaction sig_action[32];
	addr_t			signal_stack_base;
	size_t			signal_stack_size;
	bool			signal_stack_enabled;

	bool			in_kernel;
	bool			was_yielded;
	struct scheduler_thread_data* scheduler_data;

	struct user_thread*	user_thread;

	struct {
		uint8		parameters[32];
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
		sem_id		write_sem;
		sem_id		read_sem;
		thread_id	sender;
		int32		code;
		size_t		size;
		void*		buffer;
	} msg;

	union {
		addr_t		fault_handler;
		page_fault_callback fault_callback;
			// TODO: this is a temporary field used for the vm86 implementation
			// and should be removed again when that one is moved into the
			// kernel entirely.
	};
	int32			page_faults_allowed;
		/* this field may only stay in debug builds in the future */

	thread_entry_func entry;
	void			*args1, *args2;
	struct team		*team;

	struct {
		sem_id		sem;
		status_t	status;
		uint16		reason;
		uint16		signal;
		struct list	waiters;
	} exit;

	struct select_info *select_infos;

	struct thread_debug_info debug_info;

	// stack
	area_id			kernel_stack_area;
	addr_t			kernel_stack_base;
	addr_t			kernel_stack_top;
	area_id			user_stack_area;
	addr_t			user_stack_base;
	size_t			user_stack_size;

	addr_t			user_local_storage;
		// usually allocated at the safe side of the stack
	int				kernel_errno;
		// kernel "errno" differs from its userspace alter ego

	bigtime_t		user_time;
	bigtime_t		kernel_time;
	bigtime_t		last_time;

	void			(*post_interrupt_callback)(void*);
	void*			post_interrupt_data;

	// architecture dependant section
	struct arch_thread arch_info;
};

struct thread_queue {
	struct thread *head;
	struct thread *tail;
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
