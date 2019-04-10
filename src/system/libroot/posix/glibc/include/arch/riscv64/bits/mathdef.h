#if !defined _MATH_H && !defined _COMPLEX_H
# error "Never use <bits/mathdef.h> directly; include <math.h> instead"
#endif

#if defined __USE_ISOC99 && defined _MATH_H && !defined _MATH_H_MATHDEF
# define _MATH_H_MATHDEF	1

typedef float float_t;
typedef double double_t;

/* The values returned by `ilogb' for 0 and NaN respectively.  */
# define FP_ILOGB0       (-2147483647)
# define FP_ILOGBNAN     (2147483647)

# define INFINITY   (__builtin_inff())

/* The GCC 4.6 compiler will define __FP_FAST_FMA{,F,L} if the fma{,f,l}
   builtins are supported.  */
# if __FP_FAST_FMA
#  define FP_FAST_FMA 1
# endif

# if __FP_FAST_FMAF
#  define FP_FAST_FMAF 1
# endif

# if __FP_FAST_FMAL
#  define FP_FAST_FMAL 1
# endif

#endif	/* ISO C99 */
