/*
 * wrapper exp2(x)
 */

#include <float.h>
#include "math.h"
#include "math_private.h"

static const double o_threshold= (double) DBL_MAX_EXP;
static const double u_threshold= (double) (DBL_MIN_EXP - DBL_MANT_DIG - 1);

double
__exp2 (double x)		/* wrapper exp2 */
{
#ifdef _IEEE_LIBM
  return __ieee754_exp2 (x);
#else
  double z;
  z = __ieee754_exp2 (x);
  if (_LIB_VERSION != _IEEE_ && __finite (x))
    {
      if (x > o_threshold)
	/* exp2 overflow */
	return __kernel_standard (x, x, 44);
      else if (x <= u_threshold)
	/* exp2 underflow */
	return __kernel_standard (x, x, 45);
    }
  return z;
#endif
}
weak_alias (__exp2, exp2)
#ifdef NO_LONG_DOUBLE
strong_alias (__exp2, __expl2)
weak_alias (__exp2, expl2)
#endif
