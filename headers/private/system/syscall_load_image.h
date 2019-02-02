/*
 * Copyright 2019, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_SYSCALL_LOAD_IMAGE_H
#define _SYSTEM_SYSCALL_LOAD_IMAGE_H


enum {
	B_WAIT_TILL_LOADED	= 0x01,
		/* Wait till the loader has loaded and relocated (but not yet
		   initialized) the application image and all dependencies. If not
		   supplied, the function returns before the loader started to do
		   anything at all, i.e. it returns success, even if the executable
		   doesn't exist. */
};


#endif	/* _SYSTEM_SYSCALL_LOAD_IMAGE_H */
