/* cpp - C++ in the kernel
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the OpenBeOS License.
*/


#include "util/kernel_cpp.h"


#if __GNUC__ == 2

extern "C" void
__pure_virtual()
{
	//printf("pure virtual function call");
}

#elif __GNUC__ >= 3

extern "C" void
__cxa_pure_virtual()
{
}

#endif
