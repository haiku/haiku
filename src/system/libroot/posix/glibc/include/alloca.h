#ifndef _GLIBC_ALLOCA_H
#define _GLIBC_ALLOCA_H

#include_next <alloca.h>

/* Now define the internal interfaces.  */
#define __MAX_ALLOCA_CUTOFF	65536

static inline int
__libc_use_alloca (size_t size)
{
	return size <= __MAX_ALLOCA_CUTOFF;
}

#ifndef stackinfo_alloca_round
# define stackinfo_alloca_round(l) (((l) + 15) & -16)
#endif

#if _STACK_GROWS_DOWN
# define extend_alloca(buf, len, newlen) \
  (__typeof (buf)) ({ size_t __newlen = stackinfo_alloca_round (newlen);      \
		      char *__newbuf = __alloca (__newlen);		      \
		      if (__newbuf + __newlen == (char *) buf)		      \
			len += __newlen;				      \
		      else						      \
			len = __newlen;					      \
		      __newbuf; })
#elif _STACK_GROWS_UP
# define extend_alloca(buf, len, newlen) \
  (__typeof (buf)) ({ size_t __newlen = stackinfo_alloca_round (newlen);      \
		      char *__newbuf = __alloca (__newlen);		      \
		      char *__buf = (buf);				      \
		      if (__buf + len == __newbuf)			      \
			{						      \
			  len += __newlen;				      \
			  __newbuf = __buf;				      \
			}						      \
		      else						      \
			len = __newlen;					      \
		      __newbuf; })
#else
# define extend_alloca(buf, len, newlen) \
  __alloca (((len) = (newlen)))
#endif

#endif
