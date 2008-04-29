/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_UTIL_ATOMIC_H
#define _KERNEL_UTIL_ATOMIC_H


#include <limits.h>

#include <SupportDefs.h>


#ifdef __cplusplus

template<typename PointerType> PointerType*
atomic_pointer_test_and_set(PointerType** _pointer, PointerType* set,
	PointerType* test)
{
#if LONG_MAX == INT_MAX
	return (PointerType*)atomic_test_and_set((vint32*)_pointer, (int32)set,
		(int32)test);
#else
	return (PointerType*)atomic_test_and_set64((vint64*)_pointer, (int64)set,
		(int64)test);
#endif
}

#endif	// __cplusplus

#endif	/* _KERNEL_UTIL_ATOMIC_H */
