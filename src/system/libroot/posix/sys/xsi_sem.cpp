/*
 * Copyright 2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Salvatore Benedetto <salvatore.benedetto@gmail.com>
 */

#include <sys/sem.h>

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>

#include <OS.h>

#include <errno_private.h>
#include <posix/xsi_semaphore_defs.h>
#include <syscall_utils.h>
#include <syscalls.h>


int
semget(key_t key, int numSems, int semFlags)
{
	RETURN_AND_SET_ERRNO(_kern_xsi_semget(key, numSems, semFlags));
}


int
semctl(int semID, int semNum, int command, ...)
{
	union semun arg;
	va_list args;

	switch (command) {
		case GETVAL:
		case GETPID:
		case GETNCNT:
		case GETZCNT:
		case IPC_RMID:
			RETURN_AND_SET_ERRNO(_kern_xsi_semctl(semID, semNum, command, 0));

		case SETVAL:
		case GETALL:
		case SETALL:
		case IPC_STAT:
		case IPC_SET:
			va_start(args, command);
			arg = va_arg(args, union semun);
			va_end(args);
			RETURN_AND_SET_ERRNO(_kern_xsi_semctl(semID, semNum, command,
				&arg));

		default:
			return EINVAL;
	}
}


int
semop(int semID, struct sembuf *semOps, size_t numSemOps)
{
	RETURN_AND_SET_ERRNO(_kern_xsi_semop(semID, semOps, numSemOps));
}
