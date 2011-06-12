/*
 * Copyright 2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Salvatore Benedetto <salvatore.benedetto@gmail.com>
 */

#include <sys/msg.h>

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>

#include <OS.h>

#include <syscall_utils.h>
#include <syscalls.h>


int
msgctl(int messageQueueID, int command, struct msqid_ds *buffer)
{
	RETURN_AND_SET_ERRNO(_kern_xsi_msgctl(messageQueueID, command, buffer));
}


int
msgget(key_t key, int messageQueueFlags)
{
	RETURN_AND_SET_ERRNO(_kern_xsi_msgget(key, messageQueueFlags));
}


ssize_t
msgrcv(int messageQueueID, void *messagePointer, size_t messageSize,
	long messageType, int messageFlags)
{
	RETURN_AND_SET_ERRNO_TEST_CANCEL(_kern_xsi_msgrcv(messageQueueID,
		messagePointer, messageSize, messageType, messageFlags));
}


int
msgsnd(int messageQueueID, const void *messagePointer, size_t messageSize,
	int messageFlags)
{
	RETURN_AND_SET_ERRNO_TEST_CANCEL(_kern_xsi_msgsnd(messageQueueID,
		messagePointer, messageSize, messageFlags));
}
