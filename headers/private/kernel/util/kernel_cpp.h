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

#if _KERNEL_MODE

using namespace std;
extern const nothrow_t std::nothrow;

// Oh no! C++ in the kernel! Are you nuts?
//
//	- no exceptions
//	- (almost) no virtuals (well, the Query code now uses them)
//	- it's basically only the C++ syntax, and type checking
//	- since one tend to encapsulate everything in classes, it has a slightly
//	  higher memory overhead
//	- nicer code
//	- easier to maintain


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
