/*
 * Copyright 2014, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _UCONTEXT_H_
#define _UCONTEXT_H_


typedef struct vregs mcontext_t;

typedef struct __ucontext_t {
	struct __ucontext_t*	uc_link;
	sigset_t				uc_sigmask;
	stack_t					uc_stack;
	mcontext_t				uc_mcontext;
} ucontext_t;


#endif
