/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_KSYSCALLS_H
#define _KERNEL_KSYSCALLS_H


#include <SupportDefs.h>


#ifdef __cplusplus
extern "C" {
#endif

int32 syscall_dispatcher(uint32 function, void *argBuffer, uint64 *_returnValue);
status_t generic_syscall_init(void);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_KSYSCALLS_H */
