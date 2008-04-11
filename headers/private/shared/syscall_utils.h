/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSCALL_UTILS_H
#define _SYSCALL_UTILS_H

#define RETURN_AND_SET_ERRNO(err)			\
	do {									\
		__typeof(err) raseResult = (err);	\
		if (raseResult < 0) {				\
			errno = raseResult;				\
			return -1;						\
		}									\
		return raseResult;					\
	} while (false)

#endif	// _SYSCALL_UTILS_H
