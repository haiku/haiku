/* cpp - C++ in the kernel
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the OpenBeOS License.
*/

#if _KERNEL_MODE

#include "util/kernel_cpp.h"

const nothrow_t std::nothrow = {};

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

#endif	// _#if KERNEL_MODE
