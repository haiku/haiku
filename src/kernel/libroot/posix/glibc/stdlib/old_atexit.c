#include <shlib-compat.h>

#if SHLIB_COMPAT (libc, GLIBC_2_0, GLIBC_2_2_2)
# define atexit __dyn_atexit
# include "atexit.c"
# undef atexit
compat_symbol (libc, __dyn_atexit, atexit, GLIBC_2_0);
#endif
