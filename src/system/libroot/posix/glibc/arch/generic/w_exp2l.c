/*
 * wrapper exp2l(x)
 */

#include <float.h>
#include "math.h"
#include "math_private.h"

static const long double o_threshold = (long double) LDBL_MAX_EXP;
static const long double u_threshold
  = (long double) (LDBL_MIN_EXP - LDBL_MANT_DIG - 1);

long double
__exp2l (long double x)			/* wrapper exp2l */
{
#ifdef _IEEE_LIBM
  return __ieee754_exp2l (x);
#else
  long double z;
  z = __ieee754_exp2l (x);
  if (_LIB_VERSION != _IEEE_ && __finitel (x))
    {
      if (x > o_threshold)
	return __kernel_standard (x, x, 244); /* exp2l overflow */
      else if (x <= u_threshold)
	return __kernel_standard (x, x, 245); /* exp2l underflow */
    }
  return z;
#endif
}
weak_alias (__exp2l, exp2l)
