#ifndef _CTYPE_H

extern int __isctype (int __c, int __mask);

# ifndef NOT_IN_libc

/* These accessors are used by the optimized macros to find the
   thread-local cache of ctype information from the current thread's
   locale.  For inside libc, define them as inlines using the _NL_CURRENT
   accessors.  We don't use _NL_CURRENT_LOCALE->__ctype_b here because we
   want to cause a link-time ref to _nl_current_LC_CTYPE under
   NL_CURRENT_INDIRECT.  */

#  include "../locale/localeinfo.h"
#  include <bits/libc-tsd.h>

#  ifndef CTYPE_EXTERN_INLINE	/* Used by ctype/ctype-info.c, which see.  */
#   define CTYPE_EXTERN_INLINE static inline
#  endif

__libc_tsd_define (extern, CTYPE_B)
__libc_tsd_define (extern, CTYPE_TOUPPER)
__libc_tsd_define (extern, CTYPE_TOLOWER)

CTYPE_EXTERN_INLINE const uint16_t ** __attribute__ ((const))
__ctype_b_loc (void)
{
  const uint16_t **tablep =
    (const uint16_t **) __libc_tsd_address (CTYPE_B);
  if (__builtin_expect (*tablep == NULL, 0))
    *tablep = (const uint16_t *) _NL_CURRENT (LC_CTYPE, _NL_CTYPE_CLASS) + 128;
  return tablep;
}

CTYPE_EXTERN_INLINE const int32_t ** __attribute__ ((const))
__ctype_toupper_loc (void)
{
  const int32_t **tablep =
    (const int32_t **) __libc_tsd_address (CTYPE_TOUPPER);
  if (__builtin_expect (*tablep == NULL, 0))
    *tablep = ((int32_t *) _NL_CURRENT (LC_CTYPE, _NL_CTYPE_TOUPPER) + 128);
  return tablep;
}

CTYPE_EXTERN_INLINE const int32_t ** __attribute__ ((const))
__ctype_tolower_loc (void)
{
  const int32_t **tablep =
    (const int32_t **) __libc_tsd_address (CTYPE_TOLOWER);
  if (__builtin_expect (*tablep == NULL, 0))
    *tablep = ((int32_t *) _NL_CURRENT (LC_CTYPE, _NL_CTYPE_TOLOWER) + 128);
  return tablep;
}

# endif	/* Not NOT_IN_libc.  */

# include <ctype/ctype.h>

#endif /* ctype.h */
