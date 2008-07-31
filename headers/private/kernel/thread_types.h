/*
 * Copyright 2004-2008, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Thread definition and structures
 */
#ifndef _KERNEL_THREAD_TYPES_H
#define _KERNEL_THREAD_TYPES_H

#ifndef _ASSEMBLER

#include <cbuf.h>
#include <smp.h>
#include <signal.h>
#include <thread_defs.h>
#include <timer.h>
#include <user_debugger.h>
#include <util/list.h>
#include <arch/thread_types.h>


extern spinlock thread_spinlock;
#define GRAB_THREAD_LOCK()    acquire_spinlock(&thread_spinlock)
#define RELEASE_THREAD_LOCK() release_spinlock(&thread_spinlock)

extern spinlock team_spinlock;
	// NOTE: TEAM lock can be held over a THREAD lock acquisition,
	// but not the other way (to avoid deadlock)
#define GRAB_TEAM_LOCK()    acquire_spinlock(&team_spinlock)
#define RELEASE_TEAM_LOCK() release_spinlock(&team_spinlock)

enum additional_thread_state {
	THREAD_STATE_FREE_ON_RESCHED = 7, // free the thread structure upon reschedule
//	THREAD_STATE_BIRTH	// thread is being created
};

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

// The type of object a thread blocks on (thread::wait::type, set by
// thread_prepare_to_block()).
enum {
	THREAD_BLOCK_TYPE_SEMAPHORE				= 0,
	THREAD_BLOCK_TYPE_CONDITION_VARIABLE	= 1,
	THREAD_BLOCK_TYPE_SNOOZE				= 2,
	THREAD_BLOCK_TYPE_SIGNAL				= 3,
	THREAD_BLOCK_TYPE_MUTEX					= 4,
	THREAD_BLOCK_TYPE_RW_LOCK				= 5,

	THREAD_BLOCK_TYPE_OTHER					= 9999,
	THREAD_BLOCK_TYPE_USER_BASE				= 10000
};

struct image;					// defined in image.c
struct realtime_sem_context;	// defined in realtime_sem.cpp
struct select_info;
struct user_thread;				// defined in libroot/user_thread.h

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


#endif	// __cplusplus


struct free_user_thread {
	struct free_user_thread*	next;
	struct user_thread*			thread;
};

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
	void			*io_context;
	struct realtime_sem_context	*realtime_sem_context;
	int32			xsi_sem_undo_requests;
	sem_id			death_sem;		// semaphore to wait on for dying threads
	struct list		dead_threads;
	int				dead_threads_count;

	team_dead_children *dead_children;
	team_job_control_children *stopped_children;
	team_job_control_children *continued_children;
	struct job_control_entry* job_control_entry;

	struct vm_address_space *address_space;
	struct thread	*main_thread;
	struct thread	*thread_list;
	struct team_loading_info *loading_info;
	struct list		image_list;
	struct list		watcher_list;
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
	int32			state;
	int32			next_state;
	struct cpu_ent	*cpu;

	sigset_t		sig_pending;
	sigset_t		sig_block_mask;
	struct sigaction sig_action[32];
	addr_t			signal_stack_base;
	size_t			signal_stack_size;
	bool			signal_stack_enabled;

	bool			in_kernel;
	bool			was_yielded;

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
		cbuf		*buffer;
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
#define	THREAD_FLAGS_DEBUG_THREAD			0x0002
#define	THREAD_FLAGS_DEBUGGER_INSTALLED		0x0004
#define	THREAD_FLAGS_BREAKPOINTS_DEFINED	0x0008
#define	THREAD_FLAGS_BREAKPOINTS_INSTALLED	0x0010
#define	THREAD_FLAGS_64_BIT_SYSCALL_RETURN	0x0020
#define	THREAD_FLAGS_RESTART_SYSCALL		0x0040
#define	THREAD_FLAGS_DONT_RESTART_SYSCALL	0x0080
#define	THREAD_FLAGS_SYSCALL_RESTARTED		0x0100
#define	THREAD_FLAGS_SYSCALL				0x0200
	// Note: Set only for certain syscalls.


#endif	/* _KERNEL_THREAD_TYPES_H */
