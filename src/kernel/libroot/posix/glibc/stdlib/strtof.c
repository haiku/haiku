/* The actual implementation for all floating point sizes is in strtod.c.
   These macros tell it to produce the `float' version, `strtof'.  */

#define	FLOAT		float
#define	FLT		FLT
#ifdef USE_IN_EXTENDED_LOCALE_MODEL
# define STRTOF		__strtof_l
#else
# define STRTOF		strtof
#endif
#define	MPN2FLOAT	__mpn_construct_float
#define	FLOAT_HUGE_VAL	HUGE_VALF
#define SET_MANTISSA(flt, mant) \
  do { union ieee754_float u;						      \
       u.f = (flt);							      \
       if ((mant & 0x7fffff) == 0)					      \
	 mant = 0x400000;						      \
       u.ieee.mantissa = (mant) & 0x7fffff;				      \
       (flt) = u.f;							      \
  } while (0)

#include "strtod.c"
