/* cpp - C++ in the kernel
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the OpenBeOS License.
*/

// stripped down C++ support for the boot loader
#ifdef _BOOT_MODE

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


// full C++ support in the kernel
#elif _KERNEL_MODE	// #endif _BOOT_MODE

#include <stdio.h>

#include <KernelExport.h>

#include "util/kernel_cpp.h"

FILE *stderr = NULL;

extern "C"
int
fprintf(FILE *f, const char *format, ...)
{
	// TODO: Introduce a vdprintf()...
	dprintf("fprintf(`%s',...)\n", format);
	return 0;
}

extern "C"
void
abort()
{
	panic("abort() called!");
}

extern "C"
void
debugger(const char *message)
{
	kernel_debugger(message);
}

#endif	// _#if KERNEL_MODE
