/*  GNU SED, a batch stream editor.
    Copyright (C) 1998 Free Software Foundation, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

/* some basic #defines to support pre-ANSI compilers */

#ifndef BASICDEFS_H
#define BASICDEFS_H

/* Can the compiler grok function prototypes? */
#if defined __STDC__ && __STDC__
# define P_(s)		s
#else
# define P_(s)		()
#endif

/* (VOID *) is the generic pointer type; some ancient compilers
   don't know about (void *), and typically use (char *) instead.
   VCAST() is used to cast to and from (VOID *)s --- but if the
   compiler *does* support (void *) make this a no-op, so that
   the compiler can detect if we omitted an essential function
   declaration somewhere.
 */
#ifndef VOID
# define VOID		void
# define VCAST(t)	
#else
# define VCAST(t)	(t)
#endif

/* some basic definitions to avoid undue promulgating of VCAST ugliness */
#define MALLOC(n,t)	(VCAST(t *)ck_malloc((n)*sizeof(t)))
#define REALLOC(x,n,t)	(VCAST(t *)ck_realloc(VCAST(VOID *)(x),(n)*sizeof(t)))
#define MEMDUP(x,n,t)	(VCAST(t *)ck_memdup(VCAST(VOID *)(x),(n)*sizeof(t)))
#define FREE(x)		(ck_free(VCAST(VOID *)x))


#ifdef HAVE_MEMORY_H
# include <memory.h>
#endif

#ifndef HAVE_MEMMOVE
# ifndef memmove
   /* ../lib/libsed.a provides a memmove() if the system doesn't.
      Here is where we declare its return type; we don't prototype
      it because that sometimes causes problems when we're running in
      bootstrap mode on a system which really does support memmove(). */
   extern VOID *memmove();
# endif
#endif

#ifndef HAVE_MEMCPY
# ifndef memcpy
#  define memcpy(d, s, n)	memmove(d, s, n)
# endif
#endif

#ifndef HAVE_STRERROR
 extern char *strerror P_((int e));
#endif


/* handle misdesigned <ctype.h> macros (snarfed from lib/regex.c) */
/* Jim Meyering writes:

   "... Some ctype macros are valid only for character codes that
   isascii says are ASCII (SGI's IRIX-4.0.5 is one such system --when
   using /bin/cc or gcc but without giving an ansi option).  So, all
   ctype uses should be through macros like ISPRINT...  If
   STDC_HEADERS is defined, then autoconf has verified that the ctype
   macros don't need to be guarded with references to isascii. ...
   Defining isascii to 1 should let any compiler worth its salt
   eliminate the && through constant folding."
   Solaris defines some of these symbols so we must undefine them first.  */

#undef ISASCII
#if defined STDC_HEADERS || (!defined isascii && !defined HAVE_ISASCII)
# define ISASCII(c) 1
#else
# define ISASCII(c) isascii(c)
#endif

#ifdef isblank
# define ISBLANK(c) (ISASCII (c) && isblank (c))
#else
# define ISBLANK(c) ((c) == ' ' || (c) == '\t')
#endif

#undef ISPRINT
#define ISPRINT(c) (ISASCII (c) && isprint (c))
#define ISDIGIT(c) (ISASCII (c) && isdigit (c))
#define ISALNUM(c) (ISASCII (c) && isalnum (c))
#define ISALPHA(c) (ISASCII (c) && isalpha (c))
#define ISCNTRL(c) (ISASCII (c) && iscntrl (c))
#define ISLOWER(c) (ISASCII (c) && islower (c))
#define ISPUNCT(c) (ISASCII (c) && ispunct (c))
#define ISSPACE(c) (ISASCII (c) && isspace (c))
#define ISUPPER(c) (ISASCII (c) && isupper (c))
#define ISXDIGIT(c) (ISASCII (c) && isxdigit (c))


#endif /*!BASICDEFS_H*/
