/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/
#ifndef _KERNEL_SYSCALL_PROCESS_INFO_H
#define _KERNEL_SYSCALL_PROCESS_INFO_H

enum which_process_info {
	SESSION_ID = 1,
	GROUP_ID,
	PARENT_ID,
};

#endif	/* _KRENEL_SYSCALL_PROCESS_INFO_H */
