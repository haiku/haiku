#ifndef _KERNEL_THREAD_TYPES_H
#define _KERNEL_THREAD_TYPES_H

/**
 * @file kernel/thread_types.h
 * @brief Definitions, structures and functions for threads.
 */

/**
 * @defgroup Kernel_Threads Threads
 * @ingroup OpenBeOS_Kernel
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stage2.h>
#include <ktypes.h>
#include <vm.h>
#include <smp.h>
#include <arch/thread_struct.h>


#define THREAD_IDLE_PRIORITY 0

#define THREAD_NUM_PRIORITY_LEVELS 64
#define THREAD_MIN_PRIORITY    (THREAD_IDLE_PRIORITY + 1)
#define THREAD_MAX_PRIORITY    (THREAD_NUM_PRIORITY_LEVELS - THREAD_NUM_RT_PRIORITY_LEVELS - 1)

#define THREAD_NUM_RT_PRIORITY_LEVELS 16
#define THREAD_MIN_RT_PRIORITY (THREAD_MAX_PRIORITY + 1)
#define THREAD_MAX_RT_PRIORITY (THREAD_NUM_PRIORITY_LEVELS - 1)

#define THREAD_LOWEST_PRIORITY    THREAD_MIN_PRIORITY
#define THREAD_LOW_PRIORITY       12
#define THREAD_MEDIUM_PRIORITY    24
#define THREAD_HIGH_PRIORITY      36
#define THREAD_HIGHEST_PRIORITY   THREAD_MAX_PRIORITY

#define THREAD_RT_LOW_PRIORITY    THREAD_MIN_RT_PRIORITY
#define THREAD_RT_HIGH_PRIORITY   THREAD_MAX_RT_PRIORITY

extern spinlock_t thread_spinlock;
#define GRAB_THREAD_LOCK()    acquire_spinlock(&thread_spinlock)
#define RELEASE_THREAD_LOCK() release_spinlock(&thread_spinlock)

enum {
	THREAD_STATE_READY = 0,   // ready to run
	THREAD_STATE_RUNNING, // running right now somewhere
	THREAD_STATE_WAITING, // blocked on something
	THREAD_STATE_SUSPENDED, // suspended, not in queue
	THREAD_STATE_FREE_ON_RESCHED, // free the thread structure upon reschedule
	THREAD_STATE_BIRTH	// thread is being created
};

enum {
	TEAM_STATE_NORMAL,	// normal state
	TEAM_STATE_BIRTH,	// being contructed
	TEAM_STATE_DEATH	// being killed
};

#define SIG_NONE 0
#define SIG_SUSPEND	1
#define SIG_KILL 2

/**
 * The team structure; equivalent to a common process.
 * @note This is available only within the kernel.
 */
struct team {
	/** Pointer to next team structure */
	struct team *next;
	/** The team_id for this process. */
	team_id      id;
	/** name of the process
	 * @note The maximum length is current SYS_MAX_OS_NAME_LEN chars.
	 */
	char         name[SYS_MAX_OS_NAME_LEN];
	/** How many threads does this process have? */
	int          num_threads;
	/** Current state of the process. */
	int          state;
	/** Signals pending for the process? */
	int          pending_signals;
	/** Pointer to the I/O context for this process.
	 * @note see fd.h for definition of ioctx
	 */
	void        *ioctx;
	/** Path ??? */
	char         path[SYS_MAX_PATH_LEN];
	/** Semaphore to wait on for dying threads */
	sem_id       death_sem;
	/** Our address space pointer */
	aspace_id    _aspace_id;
	vm_address_space *aspace;
	vm_address_space *kaspace;
	addr         user_env_base;
	struct thread *main_thread;
	struct thread *thread_list;
	struct arch_team arch_info;
};

struct thread {
	struct thread *all_next;
	struct thread *team_next;
	struct thread *q_next;
	thread_id id;
	char name[SYS_MAX_OS_NAME_LEN];
	int priority;
	int state;
	int next_state;
	union cpu_ent *cpu;
	int pending_signals;
	bool in_kernel;
	sem_id sem_blocking;
	int sem_count;
	int sem_acquire_count;
	int sem_deleted_retcode;
	int sem_errcode;
	int sem_flags;
	addr fault_handler;
	addr entry;
	void *args;
	struct team *team;
	sem_id return_code_sem;
	region_id kernel_stack_region_id;
	addr kernel_stack_base;
	region_id user_stack_region_id;
	addr user_stack_base;

	bigtime_t user_time;
	bigtime_t kernel_time;
	bigtime_t last_time;

	// architecture dependant section
	struct arch_thread arch_info;
};

struct thread_queue {
	struct thread *head;
	struct thread *tail;
};

#if 1
/**
 * Test routine for threads.
 * @note This function will be removed as is intended for test and
 *       development purposes only.
 */
int thread_test(void);
#endif

#ifdef __cplusplus
}
#endif
/** @} */

#endif /* _KERNEL_THREAD_TYPES_H */

