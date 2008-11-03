#ifndef KERNEL_CPP_H
#define KERNEL_CPP_H
/* cpp - C++ in the kernel
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the OpenBeOS License.
*/

#ifdef __cplusplus

#include <new>
#include <stdlib.h>

#if _KERNEL_MODE || _LOADER_MODE

using namespace std;
extern const nothrow_t std::nothrow;

// We need new() versions we can use when also linking against libgcc.
// std::nothrow can't be used since it's defined in both libgcc and
// kernel_cpp.cpp.
typedef struct {} mynothrow_t;
extern const mynothrow_t mynothrow;


inline void *
operator new(size_t size) throw (std::bad_alloc)
{
	// we don't actually throw any exceptions, but we have to
	// keep the prototype as specified in <new>, or else GCC 3
	// won't like us
	return malloc(size);
}


inline void *
operator new[](size_t size) throw (std::bad_alloc)
{
	return malloc(size);
}


inline void *
operator new(size_t size, const std::nothrow_t &) throw ()
{
	return malloc(size);
}


inline void *
operator new[](size_t size, const std::nothrow_t &) throw ()
{
	return malloc(size);
}


inline void *
operator new(size_t size, const mynothrow_t &) throw ()
{
	return malloc(size);
}


inline void *
operator new[](size_t size, const mynothrow_t &) throw ()
{
	return malloc(size);
}


inline void
operator delete(void *ptr) throw ()
{
	free(ptr);
}


inline void
operator delete[](void *ptr) throw ()
{
	free(ptr);
}

#endif	// #if _KERNEL_MODE

#endif	// __cplusplus

#endif	/* KERNEL_CPP_H */
