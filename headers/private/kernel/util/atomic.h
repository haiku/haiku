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
atomic_pointer_test_and_set(PointerType** _pointer, const PointerType* set,
	const PointerType* test)
{
#if LONG_MAX == INT_MAX
	return (PointerType*)atomic_test_and_set((vint32*)_pointer, (int32)set,
		(int32)test);
#else
	return (PointerType*)atomic_test_and_set64((vint64*)_pointer, (int64)set,
		(int64)test);
#endif
}


template<typename PointerType> PointerType*
atomic_pointer_set(PointerType** _pointer, const PointerType* set)
{
#if LONG_MAX == INT_MAX
	return (PointerType*)atomic_set((vint32*)_pointer, (int32)set);
#else
	return (PointerType*)atomic_set64((vint64*)_pointer, (int64)set);
#endif
}


template<typename PointerType> PointerType*
atomic_pointer_get(PointerType** _pointer)
{
#if LONG_MAX == INT_MAX
	return (PointerType*)atomic_get((vint32*)_pointer);
#else
	return (PointerType*)atomic_get64((vint64*)_pointer);
#endif
}

#endif	// __cplusplus

#endif	/* _KERNEL_UTIL_ATOMIC_H */
