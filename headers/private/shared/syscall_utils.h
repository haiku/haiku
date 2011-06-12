/*
 * Copyright 2008-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSCALL_UTILS_H
#define _SYSCALL_UTILS_H


#define RETURN_AND_SET_ERRNO(err)			\
	do {									\
		__typeof(err) __result = (err);		\
		if (__result < 0) {					\
			errno = __result;				\
			return -1;						\
		}									\
		return __result;					\
	} while (0)

#define RETURN_AND_TEST_CANCEL(err)			\
	do {									\
		__typeof(err) __result = (err);		\
		pthread_testcancel();				\
		return __result;					\
	} while (0)

#define RETURN_AND_SET_ERRNO_TEST_CANCEL(err)	\
	do {										\
		__typeof(err) __result = (err);			\
		pthread_testcancel();					\
		if (__result < 0) {						\
			errno = __result;					\
			return -1;							\
		}										\
		return __result;						\
	} while (0)


#endif	// _SYSCALL_UTILS_H
