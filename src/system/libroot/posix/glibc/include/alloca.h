#ifndef _GLIBC_ALLOCA_H
#define _GLIBC_ALLOCA_H

#include_next <alloca.h>


#define __MAX_ALLOCA_CUTOFF	65536

static inline int
__libc_use_alloca (size_t size)
{
	return size <= __MAX_ALLOCA_CUTOFF;
}


#endif
