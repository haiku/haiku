/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef KERNEL_SEM_H
#define KERNEL_SEM_H

#include <OS.h>
#include <thread.h>


struct kernel_args;


/* user calls */
sem_id user_create_sem(int32 count, const char *name);
status_t user_delete_sem(sem_id id);
status_t user_delete_sem_etc(sem_id id, status_t return_code, bool interrupted);
status_t user_acquire_sem(sem_id id);
status_t user_acquire_sem_etc(sem_id id, int32 count, uint32 flags, bigtime_t timeout);
status_t user_release_sem(sem_id id);
status_t user_release_sem_etc(sem_id id, int32 count, uint32 flags);
status_t user_get_sem_count(sem_id id, int32* thread_count);
status_t user_get_sem_info(sem_id, struct sem_info *, size_t);
status_t user_get_next_sem_info(team_id, int32 *, struct sem_info *, size_t);
status_t user_set_sem_owner(sem_id id, team_id team);

/* kernel calls */
extern sem_id	create_sem_etc(int32 count, const char *name, team_id owner);
extern status_t	sem_init(struct kernel_args *ka);
extern int		sem_delete_owned_sems(team_id owner);
extern status_t	sem_interrupt_thread(struct thread *t);
extern int32	sem_used_sems(void);
extern int32	sem_max_sems(void);

#endif	/* KERNEL_SEM_H */
