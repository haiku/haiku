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

void thread_enqueue(struct thread *t, struct thread_queue *q);
struct thread *thread_lookat_queue(struct thread_queue *q);
struct thread *thread_dequeue(struct thread_queue *q);
struct thread *thread_dequeue_id(struct thread_queue *q, thread_id thr_id);
struct thread *thread_lookat_run_q(int priority);
void thread_enqueue_run_q(struct thread *t);
struct thread *thread_dequeue_run_q(int priority);
void thread_atkernel_entry(void); // called when the thread enters the kernel on behalf of the thread
void thread_atkernel_exit(void);

int thread_suspend_thread(thread_id id);
int thread_resume_thread(thread_id id);
int thread_set_priority(thread_id id, int priority);
void thread_resched(void);
void thread_start_threading(void);
void thread_snooze(bigtime_t time);
int thread_init(kernel_args *ka);
int thread_init_percpu(int cpu_num);
void thread_exit(int retcode);
int thread_kill_thread(thread_id id);
int thread_kill_thread_nowait(thread_id id);

#define thread_get_current_thread arch_thread_get_current_thread

struct thread *thread_get_thread_struct(thread_id id);
thread_id thread_get_current_thread_id(void);

extern inline thread_id thread_get_current_thread_id(void) {
	struct thread *t = thread_get_current_thread(); return t ? t->id : 0;
}
int thread_wait_on_thread(thread_id id, int *retcode);

thread_id thread_create_user_thread(char *name, proc_id pid, addr entry, void *args);
thread_id thread_create_kernel_thread(const char *name, int (*func)(void *args), void *args);

struct proc *proc_get_kernel_proc(void);
proc_id proc_create_proc(const char *path, const char *name, char **args, int argc, int priority);
int proc_kill_proc(proc_id);
int proc_wait_on_proc(proc_id id, int *retcode);
proc_id proc_get_kernel_proc_id(void);
proc_id proc_get_current_proc_id(void);
char **user_proc_get_arguments(void);
int user_proc_get_arg_count(void);

// used in syscalls.c
int user_thread_wait_on_thread(thread_id id, int *uretcode);
proc_id user_proc_create_proc(const char *path, const char *name, char **args, int argc, int priority);
int user_proc_wait_on_proc(proc_id id, int *uretcode);

thread_id user_thread_create_user_thread(addr, proc_id, const char*, 
                                         int, void *);

int user_thread_snooze(bigtime_t time);
int user_proc_get_table(struct proc_info *pi, size_t len);
int user_getrlimit(int resource, struct rlimit * rlp);
int user_setrlimit(int resource, const struct rlimit * rlp);

#if 1
// XXX remove later
int thread_test(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _THREAD_H */
