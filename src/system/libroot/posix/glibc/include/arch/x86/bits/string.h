/* Optimized, inlined string functions.  i386 version.
   Copyright (C) 1997,1998,1999,2000,2003 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef _STRING_H
# error "Never use <bits/string.h> directly; include <string.h> instead."
#endif

/* The ix86 processors can access unaligned multi-byte variables.  */
#define _STRING_ARCH_unaligned	1


/* We only provide optimizations if the user selects them and if
   GNU CC is used.  */
#if !defined __NO_STRING_INLINES && defined __USE_STRING_INLINES \
    && defined __GNUC__ && __GNUC__ >= 2 && !__BOUNDED_POINTERS__

#ifndef __STRING_INLINE
# ifdef __cplusplus
#  define __STRING_INLINE inline
# else
#  define __STRING_INLINE extern __inline
# endif
#endif


/* Copy N bytes of SRC to DEST.  */
#define _HAVE_STRING_ARCH_memcpy 1
#define memcpy(dest, src, n) \
  (__extension__ (__builtin_constant_p (n)				      \
		  ? __memcpy_c ((dest), (src), (n))			      \
		  : memcpy ((dest), (src), (n))))
/* This looks horribly ugly, but the compiler can optimize it totally,
   as the count is constant.  */
__STRING_INLINE void *__memcpy_c (void *__dest, __const void *__src,
				  size_t __n);

__STRING_INLINE void *
__memcpy_c (void *__dest, __const void *__src, size_t __n)
{
  register unsigned long int __d0, __d1, __d2;
  union {
    unsigned int __ui;
    unsigned short int __usi;
    unsigned char __uc;
  } *__u = __dest;
  switch (__n)
    {
    case 0:
      return __dest;
    case 1:
      __u->__uc = *(const unsigned char *) __src;
      return __dest;
    case 2:
      __u->__usi = *(const unsigned short int *) __src;
      return __dest;
    case 3:
      __u->__usi = *(const unsigned short int *) __src;
      __u = (void *) __u + 2;
      __u->__uc = *(2 + (const unsigned char *) __src);
      return __dest;
    case 4:
      __u->__ui = *(const unsigned int *) __src;
      return __dest;
    case 6:
      __u->__ui = *(const unsigned int *) __src;
      __u = (void *) __u + 4;
      __u->__usi = *(2 + (const unsigned short int *) __src);
      return __dest;
    case 8:
      __u->__ui = *(const unsigned int *) __src;
      __u = (void *) __u + 4;
      __u->__ui = *(1 + (const unsigned int *) __src);
      return __dest;
    case 12:
      __u->__ui = *(const unsigned int *) __src;
      __u = (void *) __u + 4;
      __u->__ui = *(1 + (const unsigned int *) __src);
      __u = (void *) __u + 4;
      __u->__ui = *(2 + (const unsigned int *) __src);
      return __dest;
    case 16:
      __u->__ui = *(const unsigned int *) __src;
      __u = (void *) __u + 4;
      __u->__ui = *(1 + (const unsigned int *) __src);
      __u = (void *) __u + 4;
      __u->__ui = *(2 + (const unsigned int *) __src);
      __u = (void *) __u + 4;
      __u->__ui = *(3 + (const unsigned int *) __src);
      return __dest;
    case 20:
      __u->__ui = *(const unsigned int *) __src;
      __u = (void *) __u + 4;
      __u->__ui = *(1 + (const unsigned int *) __src);
      __u = (void *) __u + 4;
      __u->__ui = *(2 + (const unsigned int *) __src);
      __u = (void *) __u + 4;
      __u->__ui = *(3 + (const unsigned int *) __src);
      __u = (void *) __u + 4;
      __u->__ui = *(4 + (const unsigned int *) __src);
      return __dest;
    }
#define __COMMON_CODE(x) \
  __asm__ __volatile__							      \
    ("cld\n\t"								      \
     "rep; movsl"							      \
     x									      \
     : "=&c" (__d0), "=&D" (__d1), "=&S" (__d2)				      \
     : "0" (__n / 4), "1" (&__u->__uc), "2" (__src)			      \
     : "memory");

  switch (__n % 4)
    {
    case 0:
      __COMMON_CODE ("");
      break;
    case 1:
      __COMMON_CODE ("\n\tmovsb");
      break;
    case 2:
      __COMMON_CODE ("\n\tmovsw");
      break;
    case 3:
      __COMMON_CODE ("\n\tmovsw\n\tmovsb");
      break;
  }
  return __dest;
#undef __COMMON_CODE
}


/* Copy N bytes of SRC to DEST, guaranteeing
   correct behavior for overlapping strings.  */
#define _HAVE_STRING_ARCH_memmove 1
#ifndef _FORCE_INLINES
__STRING_INLINE void *
memmove (void *__dest, __const void *__src, size_t __n)
{
  register unsigned long int __d0, __d1, __d2;
  if (__dest < __src)
    __asm__ __volatile__
      ("cld\n\t"
       "rep\n\t"
       "movsb"
       : "=&c" (__d0), "=&S" (__d1), "=&D" (__d2)
       : "0" (__n), "1" (__src), "2" (__dest)
       : "memory");
  else
    __asm__ __volatile__
      ("std\n\t"
       "rep\n\t"
       "movsb\n\t"
       "cld"
       : "=&c" (__d0), "=&S" (__d1), "=&D" (__d2)
       : "0" (__n), "1" (__n - 1 + (const char *) __src),
	 "2" (__n - 1 + (char *) __dest)
       : "memory");
  return __dest;
}
#endif

/* Set N bytes of S to C.  */
#define _HAVE_STRING_ARCH_memset 1
#define _USE_STRING_ARCH_memset 1
#define memset(s, c, n) \
  (__extension__ (__builtin_constant_p (c)				      \
		  ? (__builtin_constant_p (n)				      \
		     ? __memset_cc (s, 0x01010101UL * (unsigned char) (c), n) \
		     : __memset_cg (s, 0x01010101UL * (unsigned char) (c), n))\
		  : __memset_gg (s, c, n)))

__STRING_INLINE void *__memset_cc (void *__s, unsigned long int __pattern,
				   size_t __n);

__STRING_INLINE void *
__memset_cc (void *__s, unsigned long int __pattern, size_t __n)
{
  register unsigned long int __d0, __d1;
  union {
    unsigned int __ui;
    unsigned short int __usi;
    unsigned char __uc;
  } *__u = __s;
  switch (__n)
    {
    case 0:
      return __s;
    case 1:
      __u->__uc = __pattern;
      return __s;
    case 2:
      __u->__usi = __pattern;
      return __s;
    case 3:
      __u->__usi = __pattern;
      __u = __extension__ ((void *) __u + 2);
      __u->__uc = __pattern;
      return __s;
    case 4:
      __u->__ui = __pattern;
      return __s;
	}
#define __COMMON_CODE(x) \
  __asm__ __volatile__							      \
    ("cld\n\t"								      \
     "rep; stosl"							      \
     x									      \
     : "=&c" (__d0), "=&D" (__d1)					      \
     : "a" (__pattern), "0" (__n / 4), "1" (&__u->__uc)			      \
     : "memory")

  switch (__n % 4)
    {
    case 0:
      __COMMON_CODE ("");
      break;
    case 1:
      __COMMON_CODE ("\n\tstosb");
      break;
    case 2:
      __COMMON_CODE ("\n\tstosw");
      break;
    case 3:
      __COMMON_CODE ("\n\tstosw\n\tstosb");
      break;
    }
  return __s;
#undef __COMMON_CODE
}

__STRING_INLINE void *__memset_cg (void *__s, unsigned long __c, size_t __n);

__STRING_INLINE void *
__memset_cg (void *__s, unsigned long __c, size_t __n)
{
  register unsigned long int __d0, __d1;
  __asm__ __volatile__
    ("cld\n\t"
     "rep; stosl\n\t"
     "testb	$2,%b3\n\t"
     "je	1f\n\t"
     "stosw\n"
     "1:\n\t"
     "testb	$1,%b3\n\t"
     "je	2f\n\t"
     "stosb\n"
     "2:"
     : "=&c" (__d0), "=&D" (__d1)
     : "a" (__c), "q" (__n), "0" (__n / 4), "1" (__s)
     : "memory");
  return __s;
}

__STRING_INLINE void *__memset_gg (void *__s, char __c, size_t __n);

__STRING_INLINE void *
__memset_gg (void *__s, char __c, size_t __n)
{
  register unsigned long int __d0, __d1;
  __asm__ __volatile__
    ("cld\n\t"
     "rep; stosb"
     : "=&D" (__d0), "=&c" (__d1)
     : "a" (__c), "0" (__s), "1" (__n)
     : "memory");
  return __s;
}




/* Search N bytes of S for C.  */
#define _HAVE_STRING_ARCH_memchr 1
#ifndef _FORCE_INLINES
__STRING_INLINE void *
memchr (__const void *__s, int __c, size_t __n)
{
  register unsigned long int __d0;
  register void *__res;
  if (__n == 0)
    return NULL;
  __asm__ __volatile__
    ("cld\n\t"
     "repne; scasb\n\t"
     "je 1f\n\t"
     "movl $1,%0\n"
     "1:"
     : "=D" (__res), "=&c" (__d0)
     : "a" (__c), "0" (__s), "1" (__n),
       "m" ( *(struct { __extension__ char __x[__n]; } *)__s)
     : "cc");
  return __res - 1;
}
#endif

#define _HAVE_STRING_ARCH_memrchr 1
#ifndef _FORCE_INLINES
__STRING_INLINE void *
__memrchr (__const void *__s, int __c, size_t __n)
{
  register unsigned long int __d0;
  register void *__res;
  if (__n == 0)
    return NULL;
  __asm__ __volatile__
    ("std\n\t"
     "repne; scasb\n\t"
     "je 1f\n\t"
     "orl $-1,%0\n"
     "1:\tcld\n\t"
     "incl %0"
     : "=D" (__res), "=&c" (__d0)
     : "a" (__c), "0" (__s + __n - 1), "1" (__n),
       "m" ( *(struct { __extension__ char __x[__n]; } *)__s)
     : "cc");
  return __res;
}
# ifdef __USE_GNU
#  define memrchr(s, c, n) __memrchr (s, c, n)
# endif
#endif

/* Return the length of S.  */
#define _HAVE_STRING_ARCH_strlen 1
#ifndef _FORCE_INLINES
__STRING_INLINE size_t
strlen (__const char *__str)
{
  register unsigned long int __d0;
  register size_t __res;
  __asm__ __volatile__
    ("cld\n\t"
     "repne; scasb\n\t"
     "notl %0"
     : "=c" (__res), "=&D" (__d0)
     : "1" (__str), "a" (0), "0" (0xffffffff),
       "m" ( *(struct { char __x[0xfffffff]; } *)__str)
     : "cc");
  return __res - 1;
}
#endif

/* Copy SRC to DEST.  */
#define _HAVE_STRING_ARCH_strcpy 1
#ifndef _FORCE_INLINES
__STRING_INLINE char *
strcpy (char *__dest, __const char *__src)
{
  register unsigned long int __d0, __d1;
  __asm__ __volatile__
    ("cld\n"
     "1:\n\t"
     "lodsb\n\t"
     "stosb\n\t"
     "testb	%%al,%%al\n\t"
     "jne	1b"
     : "=&S" (__d0), "=&D" (__d1)
     : "0" (__src), "1" (__dest)
     : "ax", "memory", "cc");
  return __dest;
}
#endif

/* Copy no more than N characters of SRC to DEST.  */
#define _HAVE_STRING_ARCH_strncpy 1
#ifndef _FORCE_INLINES
__STRING_INLINE char *
strncpy (char *__dest, __const char *__src, size_t __n)
{
  register unsigned long int __d0, __d1, __d2;
  __asm__ __volatile__
    ("cld\n"
     "1:\n\t"
     "decl	%2\n\t"
     "js	2f\n\t"
     "lodsb\n\t"
     "stosb\n\t"
     "testb	%%al,%%al\n\t"
     "jne	1b\n\t"
     "rep; stosb\n"
     "2:"
     : "=&S" (__d0), "=&D" (__d1), "=&c" (__d2)
     : "0" (__src), "1" (__dest), "2" (__n)
     : "ax", "memory", "cc");
  return __dest;
}
#endif

/* Append SRC onto DEST.  */
#define _HAVE_STRING_ARCH_strcat 1
#ifndef _FORCE_INLINES
__STRING_INLINE char *
strcat (char *__dest, __const char *__src)
{
  register unsigned long int __d0, __d1, __d2, __d3;
  __asm__ __volatile__
    ("cld\n\t"
     "repne; scasb\n\t"
     "decl	%1\n"
     "1:\n\t"
     "lodsb\n\t"
     "stosb\n\t"
     "testb	%%al,%%al\n\t"
     "jne	1b"
     : "=&S" (__d0), "=&D" (__d1), "=&c" (__d2), "=&a" (__d3)
     : "0" (__src), "1" (__dest), "2" (0xffffffff), "3" (0)
     : "memory", "cc");
  return __dest;
}
#endif

/* Append no more than N characters from SRC onto DEST.  */
#define _HAVE_STRING_ARCH_strncat 1
#ifndef _FORCE_INLINES
__STRING_INLINE char *
strncat (char *__dest, __const char *__src, size_t __n)
{
  register unsigned long int __d0, __d1, __d2, __d3;
  __asm__ __volatile__
    ("cld\n\t"
     "repne; scasb\n\t"
     "decl	%1\n\t"
     "movl	%4,%2\n"
     "1:\n\t"
     "decl	%2\n\t"
     "js	2f\n\t"
     "lodsb\n\t"
     "stosb\n\t"
     "testb	%%al,%%al\n\t"
     "jne	1b\n\t"
     "jmp	3f\n"
     "2:\n\t"
     "xorl	%3,%3\n\t"
     "stosb\n"
     "3:"
     : "=&S" (__d0), "=&D" (__d1), "=&c" (__d2), "=&a" (__d3)
     : "g" (__n), "0" (__src), "1" (__dest), "2" (0xffffffff), "3" (0)
     : "memory", "cc");
  return __dest;
}
#endif

/* Compare S1 and S2.  */
#define _HAVE_STRING_ARCH_strcmp 1
#ifndef _FORCE_INLINES
__STRING_INLINE int
strcmp (__const char *__s1, __const char *__s2)
{
  register unsigned long int __d0, __d1;
  register int __res;
  __asm__ __volatile__
    ("cld\n"
     "1:\n\t"
     "lodsb\n\t"
     "scasb\n\t"
     "jne	2f\n\t"
     "testb	%%al,%%al\n\t"
     "jne	1b\n\t"
     "xorl	%%eax,%%eax\n\t"
     "jmp	3f\n"
     "2:\n\t"
     "sbbl	%%eax,%%eax\n\t"
     "orb	$1,%%al\n"
     "3:"
     : "=a" (__res), "=&S" (__d0), "=&D" (__d1)
     : "1" (__s1), "2" (__s2),
       "m" ( *(struct { char __x[0xfffffff]; } *)__s1),
       "m" ( *(struct { char __x[0xfffffff]; } *)__s2)
     : "cc");
  return __res;
}
#endif

/* Compare N characters of S1 and S2.  */
#define _HAVE_STRING_ARCH_strncmp 1
#ifndef _FORCE_INLINES
__STRING_INLINE int
strncmp (__const char *__s1, __const char *__s2, size_t __n)
{
  register unsigned long int __d0, __d1, __d2;
  register int __res;
  __asm__ __volatile__
    ("cld\n"
     "1:\n\t"
     "decl	%3\n\t"
     "js	2f\n\t"
     "lodsb\n\t"
     "scasb\n\t"
     "jne	3f\n\t"
     "testb	%%al,%%al\n\t"
     "jne	1b\n"
     "2:\n\t"
     "xorl	%%eax,%%eax\n\t"
     "jmp	4f\n"
     "3:\n\t"
     "sbbl	%%eax,%%eax\n\t"
     "orb	$1,%%al\n"
     "4:"
     : "=a" (__res), "=&S" (__d0), "=&D" (__d1), "=&c" (__d2)
     : "1" (__s1), "2" (__s2), "3" (__n),
       "m" ( *(struct { __extension__ char __x[__n]; } *)__s1),
       "m" ( *(struct { __extension__ char __x[__n]; } *)__s2)
     : "cc");
  return __res;
}
#endif

/* Find the first occurrence of C in S.  */
#define _HAVE_STRING_ARCH_strchr 1
#define _USE_STRING_ARCH_strchr 1
#define strchr(s, c) \
  (__extension__ (__builtin_constant_p (c)				      \
		  ? __strchr_c (s, ((c) & 0xff) << 8)			      \
		  : __strchr_g (s, c)))

__STRING_INLINE char *__strchr_g (__const char *__s, int __c);

__STRING_INLINE char *
__strchr_g (__const char *__s, int __c)
{
  register unsigned long int __d0;
  register char *__res;
  __asm__ __volatile__
    ("cld\n\t"
     "movb	%%al,%%ah\n"
     "1:\n\t"
     "lodsb\n\t"
     "cmpb	%%ah,%%al\n\t"
     "je	2f\n\t"
     "testb	%%al,%%al\n\t"
     "jne	1b\n\t"
     "movl	$1,%1\n"
     "2:\n\t"
     "movl	%1,%0"
     : "=a" (__res), "=&S" (__d0)
     : "0" (__c), "1" (__s),
       "m" ( *(struct { char __x[0xfffffff]; } *)__s)
     : "cc");
  return __res - 1;
}

__STRING_INLINE char *__strchr_c (__const char *__s, int __c);

__STRING_INLINE char *
__strchr_c (__const char *__s, int __c)
{
  register unsigned long int __d0;
  register char *__res;
  __asm__ __volatile__
    ("cld\n\t"
     "1:\n\t"
     "lodsb\n\t"
     "cmpb	%%ah,%%al\n\t"
     "je	2f\n\t"
     "testb	%%al,%%al\n\t"
     "jne	1b\n\t"
     "movl	$1,%1\n"
     "2:\n\t"
     "movl	%1,%0"
     : "=a" (__res), "=&S" (__d0)
     : "0" (__c), "1" (__s),
       "m" ( *(struct { char __x[0xfffffff]; } *)__s)
     : "cc");
  return __res - 1;
}


/* Find the first occurrence of C in S or the final NUL byte.  */
#define _HAVE_STRING_ARCH_strchrnul 1
#define __strchrnul(s, c) \
  (__extension__ (__builtin_constant_p (c)				      \
		  ? ((c) == '\0'					      \
		     ? (char *) __rawmemchr (s, c)			      \
		     : __strchrnul_c (s, ((c) & 0xff) << 8))		      \
		  : __strchrnul_g (s, c)))

__STRING_INLINE char *__strchrnul_g (__const char *__s, int __c);

__STRING_INLINE char *
__strchrnul_g (__const char *__s, int __c)
{
  register unsigned long int __d0;
  register char *__res;
  __asm__ __volatile__
    ("cld\n\t"
     "movb	%%al,%%ah\n"
     "1:\n\t"
     "lodsb\n\t"
     "cmpb	%%ah,%%al\n\t"
     "je	2f\n\t"
     "testb	%%al,%%al\n\t"
     "jne	1b\n\t"
     "2:\n\t"
     "movl	%1,%0"
     : "=a" (__res), "=&S" (__d0)
     : "0" (__c), "1" (__s),
       "m" ( *(struct { char __x[0xfffffff]; } *)__s)
     : "cc");
  return __res - 1;
}

__STRING_INLINE char *__strchrnul_c (__const char *__s, int __c);

__STRING_INLINE char *
__strchrnul_c (__const char *__s, int __c)
{
  register unsigned long int __d0;
  register char *__res;
  __asm__ __volatile__
    ("cld\n\t"
     "1:\n\t"
     "lodsb\n\t"
     "cmpb	%%ah,%%al\n\t"
     "je	2f\n\t"
     "testb	%%al,%%al\n\t"
     "jne	1b\n\t"
     "2:\n\t"
     "movl	%1,%0"
     : "=a" (__res), "=&S" (__d0)
     : "0" (__c), "1" (__s),
       "m" ( *(struct { char __x[0xfffffff]; } *)__s)
     : "cc");
  return __res - 1;
}
#ifdef __USE_GNU
# define strchrnul(s, c) __strchrnul (s, c)
#endif


/* Return the length of the initial segment of S which
   consists entirely of characters not in REJECT.  */
#define _HAVE_STRING_ARCH_strcspn 1
#ifndef _FORCE_INLINES
# ifdef __PIC__
__STRING_INLINE size_t
strcspn (__const char *__s, __const char *__reject)
{
  register unsigned long int __d0, __d1, __d2;
  register char *__res;
  __asm__ __volatile__
    ("pushl	%%ebx\n\t"
     "cld\n\t"
     "movl	%4,%%edi\n\t"
     "repne; scasb\n\t"
     "notl	%%ecx\n\t"
     "decl	%%ecx\n\t"
     "movl	%%ecx,%%ebx\n"
     "1:\n\t"
     "lodsb\n\t"
     "testb	%%al,%%al\n\t"
     "je	2f\n\t"
     "movl	%4,%%edi\n\t"
     "movl	%%ebx,%%ecx\n\t"
     "repne; scasb\n\t"
     "jne	1b\n"
     "2:\n\t"
     "popl	%%ebx"
     : "=&S" (__res), "=&a" (__d0), "=&c" (__d1), "=&D" (__d2)
     : "d" (__reject), "0" (__s), "1" (0), "2" (0xffffffff),
       "m" ( *(struct { char __x[0xfffffff]; } *)__s)
     : "cc");
  return (__res - 1) - __s;
}
# else
__STRING_INLINE size_t
strcspn (__const char *__s, __const char *__reject)
{
  register unsigned long int __d0, __d1, __d2, __d3;
  register char *__res;
  __asm__ __volatile__
    ("cld\n\t"
     "movl	%5,%%edi\n\t"
     "repne; scasb\n\t"
     "notl	%%ecx\n\t"
     "decl	%%ecx\n\t"
     "movl	%%ecx,%%edx\n"
     "1:\n\t"
     "lodsb\n\t"
     "testb	%%al,%%al\n\t"
     "je	2f\n\t"
     "movl	%5,%%edi\n\t"
     "movl	%%edx,%%ecx\n\t"
     "repne; scasb\n\t"
     "jne	1b\n"
     "2:"
     : "=&S" (__res), "=&a" (__d0), "=&c" (__d1), "=&d" (__d2), "=&D" (__d3)
     : "g" (__reject), "0" (__s), "1" (0), "2" (0xffffffff),
       "m" ( *(struct { char __x[0xfffffff]; } *)__s)
     : "cc");
  return (__res - 1) - __s;
}
# endif
#endif


/* Return the length of the initial segment of S which
   consists entirely of characters in ACCEPT.  */
#define _HAVE_STRING_ARCH_strspn 1
#ifndef _FORCE_INLINES
# ifdef __PIC__
__STRING_INLINE size_t
strspn (__const char *__s, __const char *__accept)
{
  register unsigned long int __d0, __d1, __d2;
  register char *__res;
  __asm__ __volatile__
    ("pushl	%%ebx\n\t"
     "cld\n\t"
     "movl	%4,%%edi\n\t"
     "repne; scasb\n\t"
     "notl	%%ecx\n\t"
     "decl	%%ecx\n\t"
     "movl	%%ecx,%%ebx\n"
     "1:\n\t"
     "lodsb\n\t"
     "testb	%%al,%%al\n\t"
     "je	2f\n\t"
     "movl	%4,%%edi\n\t"
     "movl	%%ebx,%%ecx\n\t"
     "repne; scasb\n\t"
     "je	1b\n"
     "2:\n\t"
     "popl	%%ebx"
     : "=&S" (__res), "=&a" (__d0), "=&c" (__d1), "=&D" (__d2)
     : "r" (__accept), "0" (__s), "1" (0), "2" (0xffffffff),
       "m" ( *(struct { char __x[0xfffffff]; } *)__s)
     : "cc");
  return (__res - 1) - __s;
}
# else
__STRING_INLINE size_t
strspn (__const char *__s, __const char *__accept)
{
  register unsigned long int __d0, __d1, __d2, __d3;
  register char *__res;
  __asm__ __volatile__
    ("cld\n\t"
     "movl	%5,%%edi\n\t"
     "repne; scasb\n\t"
     "notl	%%ecx\n\t"
     "decl	%%ecx\n\t"
     "movl	%%ecx,%%edx\n"
     "1:\n\t"
     "lodsb\n\t"
     "testb	%%al,%%al\n\t"
     "je	2f\n\t"
     "movl	%5,%%edi\n\t"
     "movl	%%edx,%%ecx\n\t"
     "repne; scasb\n\t"
     "je	1b\n"
     "2:"
     : "=&S" (__res), "=&a" (__d0), "=&c" (__d1), "=&d" (__d2), "=&D" (__d3)
     : "g" (__accept), "0" (__s), "1" (0), "2" (0xffffffff),
       "m" ( *(struct { char __x[0xfffffff]; } *)__s)
     : "cc");
  return (__res - 1) - __s;
}
# endif
#endif


/* Find the first occurrence in S of any character in ACCEPT.  */
#define _HAVE_STRING_ARCH_strpbrk 1
#ifndef _FORCE_INLINES
# ifdef __PIC__
__STRING_INLINE char *
strpbrk (__const char *__s, __const char *__accept)
{
  unsigned long int __d0, __d1, __d2;
  register char *__res;
  __asm__ __volatile__
    ("pushl	%%ebx\n\t"
     "cld\n\t"
     "movl	%4,%%edi\n\t"
     "repne; scasb\n\t"
     "notl	%%ecx\n\t"
     "decl	%%ecx\n\t"
     "movl	%%ecx,%%ebx\n"
     "1:\n\t"
     "lodsb\n\t"
     "testb	%%al,%%al\n\t"
     "je	2f\n\t"
     "movl	%4,%%edi\n\t"
     "movl	%%ebx,%%ecx\n\t"
     "repne; scasb\n\t"
     "jne	1b\n\t"
     "decl	%0\n\t"
     "jmp	3f\n"
     "2:\n\t"
     "xorl	%0,%0\n"
     "3:\n\t"
     "popl	%%ebx"
     : "=&S" (__res), "=&a" (__d0), "=&c" (__d1), "=&D" (__d2)
     : "r" (__accept), "0" (__s), "1" (0), "2" (0xffffffff),
       "m" ( *(struct { char __x[0xfffffff]; } *)__s)
     : "cc");
  return __res;
}
# else
__STRING_INLINE char *
strpbrk (__const char *__s, __const char *__accept)
{
  register unsigned long int __d0, __d1, __d2, __d3;
  register char *__res;
  __asm__ __volatile__
    ("cld\n\t"
     "movl	%5,%%edi\n\t"
     "repne; scasb\n\t"
     "notl	%%ecx\n\t"
     "decl	%%ecx\n\t"
     "movl	%%ecx,%%edx\n"
     "1:\n\t"
     "lodsb\n\t"
     "testb	%%al,%%al\n\t"
     "je	2f\n\t"
     "movl	%5,%%edi\n\t"
     "movl	%%edx,%%ecx\n\t"
     "repne; scasb\n\t"
     "jne	1b\n\t"
     "decl	%0\n\t"
     "jmp	3f\n"
     "2:\n\t"
     "xorl	%0,%0\n"
     "3:"
     : "=&S" (__res), "=&a" (__d0), "=&c" (__d1), "=&d" (__d2), "=&D" (__d3)
     : "g" (__accept), "0" (__s), "1" (0), "2" (0xffffffff),
       "m" ( *(struct { char __x[0xfffffff]; } *)__s)
     : "cc");
  return __res;
}
# endif
#endif


/* Find the first occurrence of NEEDLE in HAYSTACK.  */
#define _HAVE_STRING_ARCH_strstr 1
#ifndef _FORCE_INLINES
# ifdef __PIC__
__STRING_INLINE char *
strstr (__const char *__haystack, __const char *__needle)
{
  register unsigned long int __d0, __d1, __d2;
  register char *__res;
  __asm__ __volatile__
    ("pushl	%%ebx\n\t"
     "cld\n\t" \
     "movl	%4,%%edi\n\t"
     "repne; scasb\n\t"
     "notl	%%ecx\n\t"
     "decl	%%ecx\n\t"	/* NOTE! This also sets Z if searchstring='' */
     "movl	%%ecx,%%ebx\n"
     "1:\n\t"
     "movl	%4,%%edi\n\t"
     "movl	%%esi,%%eax\n\t"
     "movl	%%ebx,%%ecx\n\t"
     "repe; cmpsb\n\t"
     "je	2f\n\t"		/* also works for empty string, see above */
     "xchgl	%%eax,%%esi\n\t"
     "incl	%%esi\n\t"
     "cmpb	$0,-1(%%eax)\n\t"
     "jne	1b\n\t"
     "xorl	%%eax,%%eax\n\t"
     "2:\n\t"
     "popl	%%ebx"
     : "=&a" (__res), "=&c" (__d0), "=&S" (__d1), "=&D" (__d2)
     : "r" (__needle), "0" (0), "1" (0xffffffff), "2" (__haystack)
     : "memory", "cc");
  return __res;
}
# else
__STRING_INLINE char *
strstr (__const char *__haystack, __const char *__needle)
{
  register unsigned long int __d0, __d1, __d2, __d3;
  register char *__res;
  __asm__ __volatile__
    ("cld\n\t" \
     "movl	%5,%%edi\n\t"
     "repne; scasb\n\t"
     "notl	%%ecx\n\t"
     "decl	%%ecx\n\t"	/* NOTE! This also sets Z if searchstring='' */
     "movl	%%ecx,%%edx\n"
     "1:\n\t"
     "movl	%5,%%edi\n\t"
     "movl	%%esi,%%eax\n\t"
     "movl	%%edx,%%ecx\n\t"
     "repe; cmpsb\n\t"
     "je	2f\n\t"		/* also works for empty string, see above */
     "xchgl	%%eax,%%esi\n\t"
     "incl	%%esi\n\t"
     "cmpb	$0,-1(%%eax)\n\t"
     "jne	1b\n\t"
     "xorl	%%eax,%%eax\n\t"
     "2:"
     : "=&a" (__res), "=&c" (__d0), "=&S" (__d1), "=&d" (__d2), "=&D" (__d3)
     : "g" (__needle), "0" (0), "1" (0xffffffff), "2" (__haystack)
     : "memory", "cc");
  return __res;
}
# endif
#endif

#ifndef _FORCE_INLINES
# undef __STRING_INLINE
#endif

#endif	/* use string inlines && GNU CC */
