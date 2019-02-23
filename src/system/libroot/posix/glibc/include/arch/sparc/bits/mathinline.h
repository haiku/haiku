#ifndef _MATH_H
# error "Never use <bits/mathinline.h> directly; include <math.h> instead."
#endif

#define __NTH(fct)    __attribute__ ((__nothrow__)) fct

#ifndef __extern_always_inline
# define __MATH_INLINE __inline
#else
# define __MATH_INLINE __extern_always_inline
#endif

#if defined __USE_ISOC99

#  define isgreater(x, y) __builtin_isgreater (x, y)
#  define isgreaterequal(x, y) __builtin_isgreaterequal (x, y)
#  define isless(x, y) __builtin_isless (x, y)
#  define islessequal(x, y) __builtin_islessequal (x, y)
#  define islessgreater(x, y) __builtin_islessgreater (x, y)
#  define isunordered(x, y) __builtin_isunordered (x, y)

#endif
