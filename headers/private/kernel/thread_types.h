/*
 * Copyright 2004-2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Thread definition and structures
 */
#ifndef _KERNEL_THREAD_TYPES_H
#define _KERNEL_THREAD_TYPES_H


#include <cbuf.h>
#include <smp.h>
#include <signal.h>
#include <timer.h>
#include <user_debugger.h>
#include <util/list.h>
#include <arch/thread_types.h>


extern spinlock thread_spinlock;
#define GRAB_THREAD_LOCK()    acquire_spinlock(&thread_spinlock)
#define RELEASE_THREAD_LOCK() release_spinlock(&thread_spinlock)

extern struct thread_queue dead_q;

extern spinlock team_spinlock;
	// NOTE: TEAM lock can be held over a THREAD lock acquisition,
	// but not the other way (to avoid deadlock)
#define GRAB_TEAM_LOCK()    acquire_spinlock(&team_spinlock)
#define RELEASE_TEAM_LOCK() release_spinlock(&team_spinlock)

enum additional_thread_state {
//	THREAD_STATE_READY = 0,   // ready to run
//	THREAD_STATE_RUNNING, // running right now somewhere
//	THREAD_STATE_WAITING, // blocked on something
//	THREAD_STATE_SUSPENDED, // suspended, not in queue
	THREAD_STATE_FREE_ON_RESCHED = 7, // free the thread structure upon reschedule
//	THREAD_STATE_BIRTH	// thread is being created
};

enum team_state {
	TEAM_STATE_NORMAL,	// normal state
	TEAM_STATE_BIRTH,	// being contructed
	TEAM_STATE_DEATH	// being killed
};

#define THREAD_RETURN_EXIT			0x1
#define THREAD_RETURN_INTERRUPTED	0x2

struct image;
	// defined in image.c

struct death_entry {
	struct list_link	link;
	team_id				team;
	thread_id			thread;
	status_t			status;
	uint32				reason;
};

struct process_session {
	pid_t				id;
	struct list			groups;
};

struct process_group {
	struct list_link	link;
	struct process_session *session;
	pid_t				id;
	sem_id				dead_child_sem;
	int32				wait_for_any;
	struct team			*teams;
};

struct team_loading_info {
	struct thread	*thread;	// the waiting thread
	status_t		result;		// the result of the loading
	bool			done;		// set when loading is done/aborted
};

struct team_watcher {
	struct list_link	link;
	void				(*hook)(team_id team, void *data);
	void				*data;
};

#define MAX_DEAD_CHILDREN	32
	// this is a soft limit for the number of dead entries in a team

struct team {
	struct team		*next;			/* next team in the hash */
	struct team		*siblings_next;
	struct team		*parent;
	struct team		*children;
	struct team		*group_next;
	team_id			id;
	pid_t			group_id;
	pid_t			session_id;
	struct process_group *group;
	char			name[B_OS_NAME_LENGTH];
	int				num_threads;	/* number of threads in this team */
	int				state;			/* current team state, see above */
	int				pending_signals;
	void			*io_context;
	sem_id			death_sem;		/* semaphore to wait on for dying threads */
	struct {
		sem_id		sem;			/* wait for dead child entries */
		struct list	list;
		uint32		count;
		int32		wait_for_any;	/* count of wait_for_child() that wait for any child */
		bigtime_t	kernel_time;
		bigtime_t	user_time;
	} dead_children;
	struct vm_address_space *address_space;
	struct thread	*main_thread;
	struct thread	*thread_list;
	struct team_loading_info *loading_info;
	struct list		image_list;
	struct list		watcher_list;
	struct arch_team arch_info;

	struct team_debug_info debug_info;

	bigtime_t		dead_threads_kernel_time;
	bigtime_t		dead_threads_user_time;
};

typedef int32 (*thread_entry_func)(thread_func, void *);

struct thread {
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
	union cpu_ent	*cpu;

	sigset_t		sig_pending;
	sigset_t		sig_block_mask;
	struct sigaction sig_action[32];

	bool			in_kernel;

	struct {
		sem_id		blocking;
		int32		count;
		int32		acquire_count;
		status_t	acquire_status;
		int32		flags;
	} sem;

	struct {
		sem_id		write_sem;
		sem_id		read_sem;
		thread_id	sender;
		int32		code;
		size_t		size;
		cbuf		*buffer;
	} msg;

	addr_t			fault_handler;
	int32			page_faults_allowed;
		/* this field may only stay in debug builds in the future */

	thread_entry_func entry;
	void			*args1, *args2;
	struct team		*team;

	struct {
		sem_id		sem;
		status_t	status;
		uint32		reason;
		struct list	waiters;
	} exit;

	struct thread_debug_info debug_info;

	// stack
	area_id			kernel_stack_area;
	addr_t			kernel_stack_base;
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

#endif	/* _KERNEL_THREAD_TYPES_H */
