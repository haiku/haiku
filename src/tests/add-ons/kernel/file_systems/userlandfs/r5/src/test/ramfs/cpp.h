#ifndef CPP_H
#define CPP_H
/* cpp - C++ in the kernel
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the MIT License.
*/

#ifdef __cplusplus

#include <new>
#include <stdlib.h>


// Oh no! C++ in the kernel! Are you nuts?
//
//	- no exceptions
//	- (almost) no virtuals (well, the Query code now uses them)
//	- it's basically only the C++ syntax, and type checking
//	- since one tend to encapsulate everything in classes, it has a slightly
//	  higher memory overhead
//	- nicer code
//	- easier to maintain


inline void *operator new(size_t size, const nothrow_t&) throw()
{
	return malloc(size);
} 

inline void *operator new[](size_t size, const nothrow_t&) throw()
{
	return malloc(size);
}
 
inline void operator delete(void *ptr)
{
	free(ptr);
} 

inline void operator delete[](void *ptr)
{
	free(ptr);
}

// now we're using virtuals
extern "C" void __pure_virtual();

#endif	// __cplusplus

#endif	/* CPP_H */
