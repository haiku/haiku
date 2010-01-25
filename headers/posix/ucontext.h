/*
 * Copyright 2008, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef _UCONTEXT_H_
#define _UCONTEXT_H_

#include <signal.h>


typedef struct vregs mcontext_t;


typedef struct ucontext_t {
	ucontext_t	*uc_link;
	sigset_t	uc_sigmask;
	stack_t		uc_stack;
	mcontext_t	uc_mcontext;
} ucontext_t;


#endif /* _UCONTEXT_H_ */
