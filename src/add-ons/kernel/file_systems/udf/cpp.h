#ifndef CPP_H
#define CPP_H
/* cpp - C++ in the kernel
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the OpenBeOS License.
*/


#include <new>
#include <stdlib.h>

/*!	Looking through the \c &lt;new&gt; header on my Linux distro
	(can't seem	to find it in the R5 headers...), it looks like
	the type of \c nothrow_t is just:
	
	<code>struct nothrow_t {};</code>
	
	Thus, here I'm just declaring an externed \c nothrow_t var called
	\c nothrow, and defining it in cpp.cpp to be initialized to \c {}.
	So far, this seems to work okay.
*/
extern const nothrow_t nothrow;


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
operator new(size_t size, const nothrow_t&) 
{
	return malloc(size);
}


inline void *
operator new[](size_t size)
{
	return malloc(size);
}
 

inline void *
operator new[](size_t size, const nothrow_t&)
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
