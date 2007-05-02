#ifndef _FBSD_COMPAT_SYS_KERNEL_H_
#define _FBSD_COMPAT_SYS_KERNEL_H_

#include <stddef.h>
#include <string.h>

#include <KernelExport.h>

#include <sys/module.h>

#define hz	1000000LL

#define bootverbose		1

#define KASSERT(cond,msg) do { \
	if (!cond) \
		panic msg; \
} while(0)


#define __printflike(a, b) __attribute__ ((format (__printf__, a, b)))

int printf(const char *format, ...) __printflike(1, 2);

#endif
