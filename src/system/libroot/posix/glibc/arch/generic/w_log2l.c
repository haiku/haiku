/*
 * wrapper log2l(X)
 */

#include "math.h"
#include "math_private.h"

long double
__log2l (long double x)	/* wrapper log2l */
{
#ifdef _IEEE_LIBM
  return __ieee754_log2l (x);
#else
  long double z;
  z = __ieee754_log2l (x);
  if (_LIB_VERSION == _IEEE_ || __isnanl (x)) return z;
  if (x <= 0.0)
    {
      if (x == 0.0)
	return __kernel_standard (x, x, 248); /* log2l (0) */
      else
	return __kernel_standard (x, x, 249); /* log2l (x < 0) */
    }
  else
    return z;
#endif
}
weak_alias (__log2l, log2l)
