/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_SYSCALLS_H
#define _KERNEL_SYSCALLS_H

#include <sys/types.h>

int syscall_dispatcher(unsigned long call_num, void *arg_buffer, uint64 *call_ret);

#endif	/* _KERNEL_SYSCALLS_H */
