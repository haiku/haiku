/*
 * Copyright 2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_XSI_SEMAPHORE_H
#define KERNEL_XSI_SEMAPHORE_H

#include <sys/sem.h>
#include <sys/cdefs.h>

#include <OS.h>

#include <kernel.h>


// TODO: move into a system/posix/xsi_semaphore_defs.h file to be shared with
// xsi_sem.cpp in libroot!
union semun {
	int					val;
	struct semid_ds		*buf;
	unsigned short		*array;
};


__BEGIN_DECLS

extern void xsi_ipc_init();
extern void xsi_sem_undo(team_id teamID, int32 numberOfUndos);

/* user calls */
int _user_xsi_semget(key_t key, int numberOfSemaphores, int flags);
int _user_xsi_semctl(int semaphoreID, int semaphoreNumber, int command,
	union semun* args);
status_t _user_xsi_semop(int semaphoreID, struct sembuf *semOps,
	size_t numSemOps);

__END_DECLS

#endif	/* KERNEL_XSI_SEMAPHORE_H */
