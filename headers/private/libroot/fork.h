#ifndef __FORK_H__
#define __FORK_H__
/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <SupportDefs.h>


#ifdef __cplusplus
extern "C" {
#endif

extern status_t __init_fork(void);
extern status_t __register_atfork(void( *prepare)( void), void( *parent)( void), void( *child)( void));

#ifdef __cplusplus
}
#endif

#endif	/* __FORK_H__ */
