#ifndef _ALLOCA_H

#include <stdlib/alloca.h>
#include <stackinfo.h>

#undef	__alloca

/* Now define the internal interfaces.  */
extern void *__alloca (size_t __size);

#ifdef	__GNUC__
# define __alloca(size)	__builtin_alloca (size)
#endif /* GCC.  */

extern int __libc_use_alloca (size_t size) __attribute__ ((const));
extern int __libc_alloca_cutoff (size_t size) __attribute__ ((const));

#define __MAX_ALLOCA_CUTOFF	65536

#include <allocalim.h>

#if _STACK_GROWS_DOWN
# define extend_alloca(buf, len, newlen) \
  (__typeof (buf)) ({ size_t __newlen = (newlen);			      \
		      char *__newbuf = __alloca (__newlen);		      \
		      if (__newbuf + __newlen == (char *) buf)		      \
			len += __newlen;				      \
		      else						      \
			len = __newlen;					      \
		      __newbuf; })
#elif _STACK_GROWS_UP
# define extend_alloca(buf, len, newlen) \
  (__typeof (buf)) ({ size_t __newlen = (newlen);			      \
		      char *__newbuf = __alloca (__newlen);		      \
		      char *__buf = (buf);				      \
		      if (__buf + __newlen == __newbuf)			      \
			{						      \
			  len += __newlen;				      \
			  __newbuf = __buf;				      \
			}						      \
		      else						      \
			len = __newlen;					      \
		      __newbuf; })
#else
# define extern_alloca(buf, len, newlen) \
  __alloca (((len) = (newlen)))
#endif

#endif
