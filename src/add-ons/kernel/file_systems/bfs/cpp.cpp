/* cpp - C++ in the kernel
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the OpenBeOS License.
*/


#include "cpp.h"


extern "C" void
__pure_virtual()
{
	//printf("pure virtual function call");
}

#if __MWERKS__

//--- These functions are supposed to throw exceptions. Obviously, we don't want them to.

extern "C" void __destroy_new_array( void *, int) {
	//printf("__destroy_new_array called");
}

extern "C" void __construct_new_array( void *, int) {
	//printf("__construct_new_array called");
}

#endif
