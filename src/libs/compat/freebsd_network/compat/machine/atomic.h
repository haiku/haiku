#ifndef _FBSD_COMPAT_MACHINE_ATOMIC_H_
#define _FBSD_COMPAT_MACHINE_ATOMIC_H_

#include <KernelExport.h>

#define atomic_add_int(ptr, value) \
	atomic_add((int32 *)ptr, value)

#define atomic_subtract_int(ptr, value) \
	atomic_add((int32 *)ptr, -value)

#endif
