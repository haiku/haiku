/*
** Copyright 2002-2004, The OpenBeOS Team. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
**
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _THREAD_H
#define _THREAD_H


#include <OS.h>
#include <thread_types.h>
#include <arch/thread.h>


#ifdef __cplusplus
extern "C" {
#endif

void scheduler_reschedule(void);
void start_scheduler(void);

void thread_enqueue(struct thread *t, struct thread_queue *q);
struct thread *thread_lookat_queue(struct thread_queue *q);
struct thread *thread_dequeue(struct thread_queue *q);
struct thread *thread_dequeue_id(struct thread_queue *q, thread_id thr_id);

void scheduler_enqueue_in_run_queue(struct thread *thread);
void scheduler_remove_from_run_queue(struct thread *thread);

void thread_atkernel_entry(void);
	// called when the thread enters the kernel on behalf of the thread
void thread_atkernel_exit(void);

int thread_init(kernel_args *ka);
int thread_init_percpu(int cpu_num);
void thread_exit(void);
int thread_kill_thread(thread_id id);
int thread_kill_thread_nowait(thread_id id);

#define thread_get_current_thread arch_thread_get_current_thread

struct thread *thread_get_thread_struct(thread_id id);
struct thread *thread_get_thread_struct_locked(thread_id id);

static thread_id thread_get_current_thread_id(void);
static inline thread_id
thread_get_current_thread_id(void)
{
	struct thread *t = thread_get_current_thread();
	return t ? t->id : 0;
}

thread_id spawn_kernel_thread_etc(thread_func, const char *name, int32 priority, void *args, team_id team);

// used in syscalls.c
status_t _user_set_thread_priority(thread_id thread, int32 newPriority);
status_t _user_suspend_thread(thread_id thread);
status_t _user_resume_thread(thread_id thread);
thread_id _user_spawn_thread(thread_func func, const char *name, int32 priority, void *arg1, void *arg2);
status_t _user_wait_for_thread(thread_id id, status_t *_returnCode);
status_t _user_snooze_etc(bigtime_t timeout, int timebase, uint32 flags);
status_t _user_kill_thread(thread_id thread);
void _user_exit_thread(status_t return_value);
bool _user_has_data(thread_id thread);
status_t _user_send_data(thread_id thread, int32 code, const void *buffer, size_t buffer_size);
status_t _user_receive_data(thread_id *_sender, void *buffer, size_t buffer_size);
thread_id _user_find_thread(const char *name);
status_t _user_get_thread_info(thread_id id, thread_info *info);
status_t _user_get_next_thread_info(team_id team, int32 *cookie, thread_info *info);

// ToDo: these don't belong here
int _user_getrlimit(int resource, struct rlimit * rlp);
int _user_setrlimit(int resource, const struct rlimit * rlp);

#if 1
// XXX remove later
int thread_test(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _THREAD_H */
