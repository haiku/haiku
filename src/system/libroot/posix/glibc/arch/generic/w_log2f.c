/*
 * wrapper log2(X)
 */

#include "math.h"
#include "math_private.h"

float
__log2f (float x)	/* wrapper log2f */
{
#ifdef _IEEE_LIBM
  return __ieee754_log2f (x);
#else
  float z;
  z = __ieee754_log2f (x);
  if (_LIB_VERSION == _IEEE_ || __isnanf (x)) return z;
  if (x <= 0.0f)
    {
      if (x == 0.0f)
	/* log2f (0) */
	return __kernel_standard ((double) x, (double) x, 148);
      else
	/* log2f (x < 0) */
	return __kernel_standard ((double) x, (double) x, 149);
    }
  else
    return z;
#endif
}
weak_alias (__log2f, log2f)
