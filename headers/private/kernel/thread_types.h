/* Thread definition and structures
** 
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef _KERNEL_THREAD_TYPES_H
#define _KERNEL_THREAD_TYPES_H


#include <ktypes.h>
#include <cbuf.h>
#include <vm.h>
#include <smp.h>
#include <signal.h>
#include <timer.h>
#include <util/list.h>
#include <arch/thread_struct.h>


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

enum {
	KERNEL_TIME,
	USER_TIME
};

#define THREAD_RETURN_EXIT			0x1
#define THREAD_RETURN_INTERRUPTED	0x2

struct image;
	// defined in image.c

struct team {
	struct team		*next;			/* next team in the hash */
	struct team		*siblings_next;
	struct team		*parent;
	struct team		*children;
	team_id			id;
	team_id			group_id;
	team_id			session_id;
	char			name[B_OS_NAME_LENGTH];
	int				num_threads;	/* number of threads in this team */
	int				state;			/* current team state, see above */
	int				pending_signals;
	void			*io_context;
	sem_id			death_sem;		/* semaphore to wait on for dying threads */
	aspace_id		_aspace_id;		/* address space pointer */
	vm_address_space *aspace;
	vm_address_space *kaspace;
	addr_t			user_env_base;
	struct thread	*main_thread;
	struct thread	*thread_list;
	struct list		image_list;
	struct arch_team arch_info;
};

typedef int32 (*thread_entry_func)(thread_func, void *);

struct thread {
	struct thread	*all_next;
	struct thread	*team_next;
	struct thread	*queue_next;	/* i.e. run queue, release queue, etc. */
	timer			alarm;
	thread_id		id;
	char			name[B_OS_NAME_LENGTH];
	int				priority;
	int				state;
	int				next_state;
	union cpu_ent	*cpu;

	sigset_t		sig_pending;
	sigset_t		sig_block_mask;
	struct sigaction sig_action[32];

	bool			in_kernel;

	sem_id			sem_blocking;
	int				sem_count;
	int				sem_acquire_count;
	int				sem_deleted_retcode;
	int				sem_errcode;
	int				sem_flags;

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
	status_t		return_code;
	sem_id			return_code_sem;
	int				return_flags;

	// stack
	region_id		kernel_stack_region_id;
	addr_t			kernel_stack_base;
	region_id		user_stack_region_id;
	addr_t			user_stack_base;

	addr_t			user_local_storage;
		// usually allocated at the safe side of the stack
	int				kernel_errno;
		// kernel "errno" differs from its userspace alter ego

	bigtime_t		user_time;
	bigtime_t		kernel_time;
	bigtime_t		last_time;
	int				last_time_type;	// KERNEL_TIME or USER_TIME

	// architecture dependant section
	struct arch_thread arch_info;
};

struct thread_queue {
	struct thread *head;
	struct thread *tail;
};

#endif	/* _KERNEL_THREAD_TYPES_H */
