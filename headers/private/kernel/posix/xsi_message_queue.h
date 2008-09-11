/*
 * Copyright 2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_XSI_MESSAGE_QUEUE_H
#define KERNEL_XSI_MESSAGE_QUEUE_H

#include <sys/msg.h>
#include <sys/cdefs.h>

#include <OS.h>

#include <kernel.h>


__BEGIN_DECLS

extern void xsi_msg_init();

/* user calls */
int _user_xsi_msgctl(int messageQueueID, int command, struct msqid_ds *buffer);
int _user_xsi_msgget(key_t key, int messageQueueFlags);
ssize_t _user_xsi_msgrcv(int messageQueueID, void *messagePointer,
	size_t messageSize, long messageType, int messageFlags);
int _user_xsi_msgsnd(int messageQueueID, const void *messagePointer,
	size_t messageSize, int messageFlags);

__END_DECLS

#endif	/* KERNEL_XSI_MESSAGE_QUEUE_H */
