/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

#ifndef BUILTIN_FS_H
#define BUILTIN_FS_H

#include <SupportDefs.h>

#ifdef __cplusplus
extern "C" {
#endif

extern status_t bootstrap_rootfs(void);
extern status_t bootstrap_pipefs(void);
extern status_t bootstrap_devfs(void);
extern status_t bootstrap_bootfs(void);

#ifdef __cplusplus
}
#endif

#endif	/* BUILTIN_FS_H */
