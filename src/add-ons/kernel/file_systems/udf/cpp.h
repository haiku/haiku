#ifndef CPP_H
#define CPP_H
/* cpp - C++ in the kernel
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the OpenBeOS License.
*/


#include <new>
#include <stdlib.h>

// I'm not entirely sure if this is such a great idea or not, but
// I can't find the standard definition of nothrow anywhere (in
// order to copy it properly here for kernel use), so this will
// do for the moment so things compile and I can go to bed. :-)
// ToDo: declare and define this thing properly
#ifndef nothrow
#	define nothrow 0
#endif

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
operator new(size_t size)
{
	return malloc(size);
} 


inline void *
operator new(size_t size, const nothrow_t&) throw()
{
	return malloc(size);
}


inline void *
operator new[](size_t size)
{
	return malloc(size);
}
 

inline void *
operator new[](size_t size, const nothrow_t&) throw()
{
	return malloc(size);
}


inline void
operator delete(void *ptr)
{
	free(ptr);
} 


inline void
operator delete[](void *ptr)
{
	free(ptr);
}

// we're using virtuals
extern "C" void __pure_virtual();


#endif	/* CPP_H */
