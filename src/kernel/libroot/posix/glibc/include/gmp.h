/* Include gmp-mparam.h first, such that definitions of _SHORT_LIMB
   and _LONG_LONG_LIMB in it can take effect into gmp.h.  */
#include <gmp-mparam.h>

#ifndef __GMP_H__

#include <stdlib/gmp.h>

/* Now define the internal interfaces.  */
extern mp_size_t __mpn_extract_double (mp_ptr res_ptr, mp_size_t size,
				       int *expt, int *is_neg,
				       double value);

extern mp_size_t __mpn_extract_long_double (mp_ptr res_ptr, mp_size_t size,
					    int *expt, int *is_neg,
					    long double value);

extern float __mpn_construct_float (mp_srcptr frac_ptr, int expt, int sign);

extern double __mpn_construct_double (mp_srcptr frac_ptr, int expt,
				      int negative);

extern long double __mpn_construct_long_double (mp_srcptr frac_ptr, int expt,
						int sign);


#endif
