/* cpp - C++ in the kernel
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the MIT License.
*/


#include "kernel-cpp.h"

#include <KernelExport.h>
#include <stdio.h>

FILE * stderr = NULL;

extern "C" int fprintf(FILE *f, const char *format, ...) { return 0; }
extern "C" void abort() { panic("abort() called!"); }
