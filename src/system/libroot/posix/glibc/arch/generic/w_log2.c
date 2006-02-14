/*
 * wrapper log2(X)
 */

#include "math.h"
#include "math_private.h"

double
__log2 (double x)	/* wrapper log2 */
{
#ifdef _IEEE_LIBM
  return __ieee754_log2 (x);
#else
  double z;
  z = __ieee754_log2 (x);
  if (_LIB_VERSION == _IEEE_ || __isnan (x)) return z;
  if (x <= 0.0)
    {
      if (x == 0.0)
	return __kernel_standard (x, x, 48); /* log2 (0) */
      else
	return __kernel_standard (x, x, 49); /* log2 (x < 0) */
    }
  else
    return z;
#endif
}
weak_alias (__log2, log2)
#ifdef NO_LONG_DOUBLE
strong_alias (__log2, __log2l)
weak_alias (__log2, log2l)
#endif
