/*
 * Copyright 2005-2012 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ARCH_SETJMP_H_
#define _ARCH_SETJMP_H_


/* TODO: A jmp_buf size of 12 might not be large enough. Increase to a large size if needed. */
typedef int __jmp_buf[12];

#endif	/* _ARCH_SETJMP_H_ */
