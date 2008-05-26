/*
 * Copyright 2008, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYS_IPC_H
#define _SYS_IPC_H


#include <sys/types.h>
#error functionality has not yet been implemented


/* Mode bits for msgget(), semget(), and shmget() */
#define IPC_CREAT	01000	/* create key */
#define IPC_EXCL	02000	/* fail if key exists */
#define IPC_NOWAIT	04000	/* do not block */

/* Control commands for msgctl(), semctl(), and shmctl() */
#define IPC_RMID	0		/* remove identifier */
#define IPC_SET		1
#define IPC_STAT	2

/* Private key */
#define IPC_PRIVATE	0


struct ipc_perm {
	key_t	key;
	uid_t	uid;			/* owner's user ID */
	gid_t	gid;			/* owner's group ID */
	uid_t	cuid;			/* creator's user ID */
	gid_t	cgid;			/* creator's group ID */
	mode_t	mode;			/* Read/write permission */
};


#ifdef __cplusplus
extern "C" {
#endif

key_t ftok(const char *path, int id);

#ifdef __cplusplus
}
#endif

#endif	/* _SYS_IPC_H */
