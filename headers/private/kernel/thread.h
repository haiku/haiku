/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _THREAD_H
#define _THREAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <thread_types.h>
#include <arch/thread.h>
#include <signal.h>

// Uncomment the line below to compile the single-queue scheduler
//#define NEW_SCHEDULER

void resched(void);
void start_scheduler(void);

#define BLOCKABLE_SIGS	(~((1L << (SIGKILL - 1)) | (1L << (SIGSTOP - 1))))

int handle_signals(struct thread *t, int state);

void thread_enqueue(struct thread *t, struct thread_queue *q);
struct thread *thread_lookat_queue(struct thread_queue *q);
struct thread *thread_dequeue(struct thread_queue *q);
struct thread *thread_dequeue_id(struct thread_queue *q, thread_id thr_id);
#ifndef NEW_SCHEDULER
struct thread *thread_lookat_run_q(int priority);
#endif /* not NEW_SCHEDULER */
void thread_enqueue_run_q(struct thread *t);
#ifndef NEW_SCHEDULER
struct thread *thread_dequeue_run_q(int priority);
#endif /* not NEW_SCHEDULER */
void thread_atkernel_entry(void);
	// called when the thread enters the kernel on behalf of the thread
void thread_atkernel_exit(void);

int thread_suspend_thread(thread_id id);
int thread_resume_thread(thread_id id);
status_t thread_set_priority(thread_id id, int32 priority);
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

int thread_wait_on_thread(thread_id id, int *retcode);

thread_id thread_create_user_thread(char *name, team_id tid, addr entry, void *args);
thread_id thread_create_kernel_thread(const char *name, int (*func)(void *args), void *args);
thread_id thread_create_kernel_thread_etc(const char *name, int (*func)(void *), void *args, struct team *p);

int team_init(kernel_args *ka);
struct team *team_get_kernel_team(void);
team_id team_create_team(const char *path, const char *name, char **args, int argc, char **envp, int envc, int priority);
int team_kill_team(team_id);
int team_wait_on_team(team_id id, int *retcode);
void team_remove_team_from_hash(struct team *team);
team_id team_get_kernel_team_id(void);
team_id team_get_current_team_id(void);
char **user_team_get_arguments(void);
int user_team_get_arg_count(void);
struct team *team_get_team_struct(team_id id);
struct team *team_get_team_struct_locked(team_id id);

// used in syscalls.c
int user_thread_wait_on_thread(thread_id id, int *uretcode);
team_id user_team_create_team(const char *path, const char *name, char **args, int argc, char **envp, int envc, int priority);
int user_team_wait_on_team(team_id id, int *uretcode);

thread_id user_thread_create_user_thread(addr, team_id, const char*, 
                                         int, void *);

status_t user_send_data(thread_id thread, int32 code, const void *buffer, size_t buffer_size);
status_t user_receive_data(thread_id *sender, void *buffer, size_t buffer_size);

status_t user_get_thread_info(thread_id id, thread_info *info);
status_t user_get_next_thread_info(team_id team, int32 *cookie, thread_info *info);
status_t user_get_team_info(team_id id, team_info *info);
status_t user_get_next_team_info(int32 *cookie, team_info *info);

int user_getrlimit(int resource, struct rlimit * rlp);
int user_setrlimit(int resource, const struct rlimit * rlp);

int user_setenv(const char *name, const char *value, int overwrite);
int user_getenv(const char *name, char **value);

int user_sigaction(int sig, const struct sigaction *act, struct sigaction *oact);

#if 1
// XXX remove later
int thread_test(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _THREAD_H */
