/*
** Copyright 2004, Oliver Tappe. All rights reserved.
** Distributed under the terms of the Haiku License.
**
** This file represents a haiku-specific implementation of errno.h.
** It's main purpose is to define the E_...-error constants.
** In a full glibc-setup, this file would live under sysdeps, but since
** we do not have/need this abstraction, I placed it here.
*/

#ifndef _ERRNO_H_
#define _ERRNO_H_

#include <Errors.h>

#define ENOERR          0
#define EOK 			ENOERR  /* some code assumes EOK exists */

#ifdef  __cplusplus
extern "C" {
#endif

extern int *_errnop(void);

#ifdef  __cplusplus
}
#endif

#define errno (*(_errnop()))

#endif /* _ERRNO_H_ */

