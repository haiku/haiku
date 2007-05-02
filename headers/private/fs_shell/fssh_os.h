/* Kernel specific structures and functions
 *
 * Copyright 2004-2006, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FSSH_SEM_H
#define _FSSH_SEM_H

#include "fssh_types.h"


#ifdef __cplusplus
extern "C" {
#endif 


/*-------------------------------------------------------------*/
/* System constants */

#define FSSH_B_OS_NAME_LENGTH	32
#define FSSH_B_PAGE_SIZE		4096
#define FSSH_B_INFINITE_TIMEOUT	(9223372036854775807LL)


/*-------------------------------------------------------------*/
/* Types */

typedef int32_t fssh_area_id;
typedef int32_t	fssh_port_id;
typedef int32_t	fssh_sem_id;
typedef int32_t	fssh_team_id;
typedef int32_t	fssh_thread_id;


/*-------------------------------------------------------------*/
/* Semaphores */

typedef struct fssh_sem_info {
	fssh_sem_id		sem;
	fssh_team_id	team;
	char			name[FSSH_B_OS_NAME_LENGTH];
	int32_t			count;
	fssh_thread_id	latest_holder;
} fssh_sem_info;

/* semaphore flags */
enum {
	FSSH_B_CAN_INTERRUPT			= 0x01,	// acquisition of the semaphore can be
											// interrupted (system use only)
	FSSH_B_CHECK_PERMISSION			= 0x04,	// ownership will be checked (system use
											// only)
	FSSH_B_KILL_CAN_INTERRUPT		= 0x20,	// acquisition of the semaphore can be
											// interrupted by SIGKILL[THR], even
											// if not B_CAN_INTERRUPT (system use
											// only)

	/* release_sem_etc() only flags */
	FSSH_B_DO_NOT_RESCHEDULE		= 0x02,	// thread is not rescheduled
	FSSH_B_RELEASE_ALL				= 0x08,	// all waiting threads will be woken up,
											// count will be zeroed
	FSSH_B_RELEASE_IF_WAITING_ONLY	= 0x10	// release count only if there are any
											// threads waiting
};

extern fssh_sem_id		fssh_create_sem(int32_t count, const char *name);
extern fssh_status_t	fssh_delete_sem(fssh_sem_id id);
extern fssh_status_t	fssh_acquire_sem(fssh_sem_id id);
extern fssh_status_t	fssh_acquire_sem_etc(fssh_sem_id id, int32_t count,
							uint32_t flags, fssh_bigtime_t timeout);
extern fssh_status_t	fssh_release_sem(fssh_sem_id id);
extern fssh_status_t	fssh_release_sem_etc(fssh_sem_id id, int32_t count,
							uint32_t flags);
extern fssh_status_t	fssh_get_sem_count(fssh_sem_id id,
							int32_t *threadCount);
extern fssh_status_t	fssh_set_sem_owner(fssh_sem_id id, fssh_team_id team);

/* system private, use the macros instead */
extern fssh_status_t	_fssh_get_sem_info(fssh_sem_id id,
							struct fssh_sem_info *info, fssh_size_t infoSize);
extern fssh_status_t	_fssh_get_next_sem_info(fssh_team_id team,
							int32_t *cookie, struct fssh_sem_info *info,
							fssh_size_t infoSize);

#define fssh_get_sem_info(sem, info) \
			_fssh_get_sem_info((sem), (info), sizeof(*(info)))

#define fssh_get_next_sem_info(team, cookie, info) \
			_fssh_get_next_sem_info((team), (cookie), (info), sizeof(*(info)))



enum {
	FSSH_B_TIMEOUT			= 8,	/* relative timeout */
	FSSH_B_RELATIVE_TIMEOUT	= 8,	/* fails after a relative timeout with B_WOULD_BLOCK */
	FSSH_B_ABSOLUTE_TIMEOUT	= 16	/* fails after an absolute timeout with B_WOULD BLOCK */
};


/*-------------------------------------------------------------*/
/* Teams */

#define FSSH_B_CURRENT_TEAM	0
#define FSSH_B_SYSTEM_TEAM	1


/*-------------------------------------------------------------*/
/* Threads */

typedef enum {
	FSSH_B_THREAD_RUNNING	= 1,
	FSSH_B_THREAD_READY,
	FSSH_B_THREAD_RECEIVING,
	FSSH_B_THREAD_ASLEEP,
	FSSH_B_THREAD_SUSPENDED,
	FSSH_B_THREAD_WAITING
} fssh_thread_state;

typedef struct {
	fssh_thread_id		thread;
	fssh_team_id		team;
	char				name[FSSH_B_OS_NAME_LENGTH];
	fssh_thread_state	state;
	int32_t				priority;
	fssh_sem_id			sem;
	fssh_bigtime_t		user_time;
	fssh_bigtime_t		kernel_time;
	void				*stack_base;
	void				*stack_end;
} fssh_thread_info;

#define FSSH_B_IDLE_PRIORITY				0
#define FSSH_B_LOWEST_ACTIVE_PRIORITY		1
#define FSSH_B_LOW_PRIORITY					5
#define FSSH_B_NORMAL_PRIORITY				10
#define FSSH_B_DISPLAY_PRIORITY				15
#define	FSSH_B_URGENT_DISPLAY_PRIORITY		20
#define	FSSH_B_REAL_TIME_DISPLAY_PRIORITY	100
#define	FSSH_B_URGENT_PRIORITY				110
#define FSSH_B_REAL_TIME_PRIORITY			120

#define FSSH_B_FIRST_REAL_TIME_PRIORITY		B_REAL_TIME_DISPLAY_PRIORITY
#define FSSH_B_MIN_PRIORITY					B_IDLE_PRIORITY
#define FSSH_B_MAX_PRIORITY					B_REAL_TIME_PRIORITY

#define FSSH_B_SYSTEM_TIMEBASE				0

typedef fssh_status_t (*fssh_thread_func)(void *);
#define fssh_thread_entry fssh_thread_func
	/* thread_entry is for backward compatibility only! Use thread_func */

extern fssh_thread_id	fssh_spawn_thread(fssh_thread_func, const char *name,
							int32_t priority, void *data);
extern fssh_status_t	fssh_kill_thread(fssh_thread_id thread);
extern fssh_status_t	fssh_resume_thread(fssh_thread_id thread);
extern fssh_status_t	fssh_suspend_thread(fssh_thread_id thread);

extern fssh_status_t	fssh_rename_thread(fssh_thread_id thread,
							const char *newName);
extern fssh_status_t	fssh_set_thread_priority (fssh_thread_id thread,
							int32_t newPriority);
extern void				fssh_exit_thread(fssh_status_t status);
extern fssh_status_t	fssh_wait_for_thread (fssh_thread_id thread,
							fssh_status_t *threadReturnValue);
extern fssh_status_t	fssh_on_exit_thread(void (*callback)(void *),
							void *data);

extern fssh_thread_id 	fssh_find_thread(const char *name);

extern fssh_status_t	fssh_send_data(fssh_thread_id thread, int32_t code,
							const void *buffer,
						fssh_size_t bufferSize);
extern int32_t			fssh_receive_data(fssh_thread_id *sender, void *buffer,
							fssh_size_t bufferSize);
extern bool				fssh_has_data(fssh_thread_id thread);

extern fssh_status_t	fssh_snooze(fssh_bigtime_t amount);
extern fssh_status_t	fssh_snooze_etc(fssh_bigtime_t amount, int timeBase,
							uint32_t flags);
extern fssh_status_t	fssh_snooze_until(fssh_bigtime_t time, int timeBase);

/* system private, use macros instead */
extern fssh_status_t	_fssh_get_thread_info(fssh_thread_id id,
							fssh_thread_info *info, fssh_size_t size);
extern fssh_status_t	_fssh_get_next_thread_info(fssh_team_id team,
							int32_t *cookie, fssh_thread_info *info,
							fssh_size_t size);

#define fssh_get_thread_info(id, info) \
			_fssh_get_thread_info((id), (info), sizeof(*(info)))

#define fssh_get_next_thread_info(team, cookie, info) \
			_fssh_get_next_thread_info((team), (cookie), (info), sizeof(*(info)))


/*-------------------------------------------------------------*/
/* Time */

extern uint32_t			fssh_real_time_clock(void);
extern void				fssh_set_real_time_clock(uint32_t secs_since_jan1_1970);
extern fssh_bigtime_t	fssh_real_time_clock_usecs(void);
extern fssh_status_t	fssh_set_timezone(char *timezone);
extern fssh_bigtime_t	fssh_system_time(void);     /* time since booting in microseconds */


#ifdef __cplusplus
}
#endif


#endif	// _FSSH_TYPES_H
