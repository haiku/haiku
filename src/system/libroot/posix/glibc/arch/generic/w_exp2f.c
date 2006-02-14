/*
 * wrapper exp2f(x)
 */

#include <float.h>
#include "math.h"
#include "math_private.h"

static const float o_threshold= (float) FLT_MAX_EXP;
static const float u_threshold= (float) (FLT_MIN_EXP - FLT_MANT_DIG - 1);

float
__exp2f (float x)		/* wrapper exp2f */
{
#ifdef _IEEE_LIBM
  return __ieee754_exp2f (x);
#else
  float z;
  z = __ieee754_exp2f (x);
  if (_LIB_VERSION != _IEEE_ && __finitef (x))
    {
      if (x > o_threshold)
	/* exp2 overflow */
	return (float) __kernel_standard ((double) x, (double) x, 144);
      else if (x <= u_threshold)
	/* exp2 underflow */
	return (float) __kernel_standard ((double) x, (double) x, 145);
    }
  return z;
#endif
}
weak_alias (__exp2f, exp2f)
