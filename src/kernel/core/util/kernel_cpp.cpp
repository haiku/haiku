/* cpp - C++ in the kernel
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the OpenBeOS License.
*/

#include "util/kernel_cpp.h"

#ifdef _BOOT_MODE
#	include <boot/platform.h>
#elif defined(_KERNEL_MODE)
#	include <KernelExport.h>
#endif

// Always define the symbols needed when not linking against libgcc.a --
// we simply override them.

const nothrow_t std::nothrow = {};

#if __GNUC__ == 2

extern "C" void
__pure_virtual()
{
	panic("pure virtual function call\n");
}

#elif __GNUC__ >= 3

extern "C" void
__cxa_pure_virtual()
{
	panic("pure virtual function call\n");
}

#endif

// full C++ support in the kernel
#if defined(_KERNEL_MODE) && !defined(_BOOT_MODE)

#include <stdio.h>

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
