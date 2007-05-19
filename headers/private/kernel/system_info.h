/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_SYSTEM_INFO_H
#define _KERNEL_SYSTEM_INFO_H


#include <OS.h>

struct kernel_args;


#ifdef __cplusplus
extern "C" {
#endif

extern status_t system_info_init(struct kernel_args *args);
extern uint32	get_haiku_revision(void);

extern status_t _user_get_system_info(system_info *userInfo, size_t size);

#ifdef __cplusplus
}
#endif

#endif	/* _KRENEL_SYSTEM_INFO_H */
