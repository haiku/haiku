/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_REALTIME_SEM_H
#define KERNEL_REALTIME_SEM_H

#include <semaphore.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <OS.h>

#include <posix/realtime_sem_defs.h>


struct realtime_sem_context;


__BEGIN_DECLS

void		realtime_sem_init();
void		delete_realtime_sem_context(struct realtime_sem_context* context);
struct realtime_sem_context* clone_realtime_sem_context(
					struct realtime_sem_context* context);

status_t	_user_realtime_sem_open(const char* name, int openFlagsOrShared,
					mode_t mode, uint32 semCount, sem_t* userSem,
					sem_t** _usedUserSem);
status_t	_user_realtime_sem_close(sem_id semID, sem_t** _deleteUserSem);
status_t	_user_realtime_sem_unlink(const char* name);

status_t	_user_realtime_sem_get_value(sem_id semID, int* value);
status_t	_user_realtime_sem_post(sem_id semID);
status_t	_user_realtime_sem_wait(sem_id semID, bigtime_t timeout);

__END_DECLS


#endif	// KERNEL_REALTIME_SEM_H
