/* sem.h
 *
 * Definitions here are for kernel use ONLY!
 *
 * For the actual definitions of the calls for sems please look in
 * OS.h
 */

#ifndef _SEM_H
#define _SEM_H

#include <thread.h>
#include <stage2.h>

/* #ifdef _KERNEL_ */

sem_id user_create_sem(int count, const char *name);
int    user_delete_sem(sem_id id);
int    user_delete_sem_etc(sem_id id, int return_code);
int    user_acquire_sem(sem_id id);
int    user_acquire_sem_etc(sem_id id, int count, int flags, bigtime_t timeout);
int    user_release_sem(sem_id id);
int    user_release_sem_etc(sem_id id, int count, int flags);
int    user_get_sem_count(sem_id id, int32* thread_count);
int    user_get_sem_info(sem_id, struct sem_info *, size_t);
int    user_get_next_sem_info(proc_id, uint32 *, struct sem_info *, size_t);
int    user_set_sem_owner(sem_id id, proc_id proc);


int    sem_init(kernel_args *ka);
int    sem_delete_owned_sems(proc_id owner);
int    sem_interrupt_thread(struct thread *t);
/* #endif */

#endif /* _KERNEL_SEM_H */

