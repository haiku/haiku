/* sem.h
 *
 * Definitions here are for kernel use ONLY!
 *
 * For the actual definitions of the calls for sems please look in
 * OS.h
 */

#ifndef _SEM_H
#define _SEM_H

#include <OS.h>
#include <thread.h>
#include <stage2.h>

/* #ifdef _KERNEL_ */

sem_id user_create_sem(int32 count, const char *name);
status_t user_delete_sem(sem_id id);
status_t user_delete_sem_etc(sem_id id, status_t return_code);
status_t user_acquire_sem(sem_id id);
status_t user_acquire_sem_etc(sem_id id, int32 count, uint32 flags, bigtime_t timeout);
status_t user_release_sem(sem_id id);
status_t user_release_sem_etc(sem_id id, int32 count, uint32 flags);
status_t user_get_sem_count(sem_id id, int32* thread_count);
status_t user_get_sem_info(sem_id, struct sem_info *, size_t);
status_t user_get_next_sem_info(team_id, int32 *, struct sem_info *, size_t);
status_t user_set_sem_owner(sem_id id, team_id team);


status_t sem_init(kernel_args *ka);
int sem_delete_owned_sems(team_id owner);
status_t sem_interrupt_thread(struct thread *t);
/* #endif */

#endif /* _KERNEL_SEM_H */

