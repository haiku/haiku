/* A GNU-like <math.h>.

   Copyright (C) 2002-2003, 2007-2009 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef _GL_MATH_H

#if __GNUC__ >= 3
@PRAGMA_SYSTEM_HEADER@
#endif

/* The include_next requires a split double-inclusion guard.  */
#@INCLUDE_NEXT_AS_FIRST_DIRECTIVE@ @NEXT_AS_FIRST_DIRECTIVE_MATH_H@

#ifndef _GL_MATH_H
#define _GL_MATH_H


/* The definition of GL_LINK_WARNING is copied here.  */


#ifdef __cplusplus
extern "C" {
#endif


/* POSIX allows platforms that don't support NAN.  But all major
   machines in the past 15 years have supported something close to
   IEEE NaN, so we define this unconditionally.  We also must define
   it on platforms like Solaris 10, where NAN is present but defined
   as a function pointer rather than a floating point constant.  */
#if !defined NAN || @REPLACE_NAN@
# undef NAN
  /* The Compaq (ex-DEC) C 6.4 compiler chokes on the expression 0.0 / 0.0.  */
# ifdef __DECC
static float
_NaN ()
{
  static float zero = 0.0f;
  return zero / zero;
}
#  define NAN (_NaN())
# else
#  define NAN (0.0f / 0.0f)
# endif
#endif

/* Solaris 10 defines HUGE_VAL, but as a function pointer rather
   than a floating point constant.  */
#if @REPLACE_HUGE_VAL@
# undef HUGE_VAL
# define HUGE_VAL (1.0 / 0.0)
#endif

/* Write x as
     x = mantissa * 2^exp
   where
     If x finite and nonzero: 0.5 <= |mantissa| < 1.0.
     If x is zero: mantissa = x, exp = 0.
     If x is infinite or NaN: mantissa = x, exp unspecified.
   Store exp in *EXPPTR and return mantissa.  */
#if @GNULIB_FREXP@
# if @REPLACE_FREXP@
#  define frexp rpl_frexp
extern double frexp (double x, int *expptr);
# endif
#elif defined GNULIB_POSIXCHECK
# undef frexp
# define frexp(x,e) \
    (GL_LINK_WARNING ("frexp is unportable - " \
                      "use gnulib module frexp for portability"), \
     frexp (x, e))
#endif


#if @GNULIB_MATHL@ || !@HAVE_DECL_ACOSL@
extern long double acosl (long double x);
#endif
#if !@GNULIB_MATHL@ && defined GNULIB_POSIXCHECK
# undef acosl
# define acosl(x) \
    (GL_LINK_WARNING ("acosl is unportable - " \
                      "use gnulib module mathl for portability"), \
     acosl (x))
#endif


#if @GNULIB_MATHL@ || !@HAVE_DECL_ASINL@
extern long double asinl (long double x);
#endif
#if !@GNULIB_MATHL@ && defined GNULIB_POSIXCHECK
# undef asinl
# define asinl(x) \
    (GL_LINK_WARNING ("asinl is unportable - " \
                      "use gnulib module mathl for portability"), \
     asinl (x))
#endif


#if @GNULIB_MATHL@ || !@HAVE_DECL_ATANL@
extern long double atanl (long double x);
#endif
#if !@GNULIB_MATHL@ && defined GNULIB_POSIXCHECK
# undef atanl
# define atanl(x) \
    (GL_LINK_WARNING ("atanl is unportable - " \
                      "use gnulib module mathl for portability"), \
     atanl (x))
#endif


#if @GNULIB_CEILF@
# if @REPLACE_CEILF@
#  define ceilf rpl_ceilf
extern float ceilf (float x);
# endif
#elif defined GNULIB_POSIXCHECK
# undef ceilf
# define ceilf(x) \
    (GL_LINK_WARNING ("ceilf is unportable - " \
                      "use gnulib module ceilf for portability"), \
     ceilf (x))
#endif

#if @GNULIB_CEILL@
# if @REPLACE_CEILL@
#  define ceill rpl_ceill
extern long double ceill (long double x);
# endif
#elif defined GNULIB_POSIXCHECK
# undef ceill
# define ceill(x) \
    (GL_LINK_WARNING ("ceill is unportable - " \
                      "use gnulib module ceill for portability"), \
     ceill (x))
#endif


#if @GNULIB_MATHL@ || (!@HAVE_DECL_COSL@ && !defined cosl)
# undef cosl
extern long double cosl (long double x);
#endif
#if !@GNULIB_MATHL@ && defined GNULIB_POSIXCHECK
# undef cosl
# define cosl(x) \
    (GL_LINK_WARNING ("cosl is unportable - " \
                      "use gnulib module mathl for portability"), \
     cosl (x))
#endif


#if @GNULIB_MATHL@ || !@HAVE_DECL_EXPL@
extern long double expl (long double x);
#endif
#if !@GNULIB_MATHL@ && defined GNULIB_POSIXCHECK
# undef expl
# define expl(x) \
    (GL_LINK_WARNING ("expl is unportable - " \
                      "use gnulib module mathl for portability"), \
     expl (x))
#endif


#if @GNULIB_FLOORF@
# if @REPLACE_FLOORF@
#  define floorf rpl_floorf
extern float floorf (float x);
# endif
#elif defined GNULIB_POSIXCHECK
# undef floorf
# define floorf(x) \
    (GL_LINK_WARNING ("floorf is unportable - " \
                      "use gnulib module floorf for portability"), \
     floorf (x))
#endif

#if @GNULIB_FLOORL@
# if @REPLACE_FLOORL@
#  define floorl rpl_floorl
extern long double floorl (long double x);
# endif
#elif defined GNULIB_POSIXCHECK
# undef floorl
# define floorl(x) \
    (GL_LINK_WARNING ("floorl is unportable - " \
                      "use gnulib module floorl for portability"), \
     floorl (x))
#endif


/* Write x as
     x = mantissa * 2^exp
   where
     If x finite and nonzero: 0.5 <= |mantissa| < 1.0.
     If x is zero: mantissa = x, exp = 0.
     If x is infinite or NaN: mantissa = x, exp unspecified.
   Store exp in *EXPPTR and return mantissa.  */
#if @GNULIB_FREXPL@ && @REPLACE_FREXPL@
# define frexpl rpl_frexpl
#endif
#if (@GNULIB_FREXPL@ && @REPLACE_FREXPL@) || !@HAVE_DECL_FREXPL@
extern long double frexpl (long double x, int *expptr);
#endif
#if !@GNULIB_FREXPL@ && defined GNULIB_POSIXCHECK
# undef frexpl
# define frexpl(x,e) \
    (GL_LINK_WARNING ("frexpl is unportable - " \
                      "use gnulib module frexpl for portability"), \
     frexpl (x, e))
#endif


/* Return x * 2^exp.  */
#if @GNULIB_LDEXPL@ && @REPLACE_LDEXPL@
# define ldexpl rpl_ldexpl
#endif
#if (@GNULIB_LDEXPL@ && @REPLACE_LDEXPL@) || !@HAVE_DECL_LDEXPL@
extern long double ldexpl (long double x, int exp);
#endif
#if !@GNULIB_LDEXPL@ && defined GNULIB_POSIXCHECK
# undef ldexpl
# define ldexpl(x,e) \
    (GL_LINK_WARNING ("ldexpl is unportable - " \
                      "use gnulib module ldexpl for portability"), \
     ldexpl (x, e))
#endif


#if @GNULIB_MATHL@ || (!@HAVE_DECL_LOGL@ && !defined logl)
# undef logl
extern long double logl (long double x);
#endif
#if !@GNULIB_MATHL@ && defined GNULIB_POSIXCHECK
# undef logl
# define logl(x) \
    (GL_LINK_WARNING ("logl is unportable - " \
                      "use gnulib module mathl for portability"), \
     logl (x))
#endif


#if @GNULIB_ROUNDF@
# if @REPLACE_ROUNDF@
#  undef roundf
#  define roundf rpl_roundf
extern float roundf (float x);
# endif
#elif defined GNULIB_POSIXCHECK
# undef roundf
# define roundf(x) \
    (GL_LINK_WARNING ("roundf is unportable - " \
                      "use gnulib module roundf for portability"), \
     roundf (x))
#endif

#if @GNULIB_ROUND@
# if @REPLACE_ROUND@
#  undef round
#  define round rpl_round
extern double round (double x);
# endif
#elif defined GNULIB_POSIXCHECK
# undef round
# define round(x) \
    (GL_LINK_WARNING ("round is unportable - " \
                      "use gnulib module round for portability"), \
     round (x))
#endif

#if @GNULIB_ROUNDL@
# if @REPLACE_ROUNDL@
#  undef roundl
#  define roundl rpl_roundl
extern long double roundl (long double x);
# endif
#elif defined GNULIB_POSIXCHECK
# undef roundl
# define roundl(x) \
    (GL_LINK_WARNING ("roundl is unportable - " \
                      "use gnulib module roundl for portability"), \
     roundl (x))
#endif


#if @GNULIB_MATHL@ || (!@HAVE_DECL_SINL@ && !defined sinl)
# undef sinl
extern long double sinl (long double x);
#endif
#if !@GNULIB_MATHL@ && defined GNULIB_POSIXCHECK
# undef sinl
# define sinl(x) \
    (GL_LINK_WARNING ("sinl is unportable - " \
                      "use gnulib module mathl for portability"), \
     sinl (x))
#endif


#if @GNULIB_MATHL@ || !@HAVE_DECL_SQRTL@
extern long double sqrtl (long double x);
#endif
#if !@GNULIB_MATHL@ && defined GNULIB_POSIXCHECK
# undef sqrtl
# define sqrtl(x) \
    (GL_LINK_WARNING ("sqrtl is unportable - " \
                      "use gnulib module mathl for portability"), \
     sqrtl (x))
#endif


#if @GNULIB_MATHL@ || !@HAVE_DECL_TANL@
extern long double tanl (long double x);
#endif
#if !@GNULIB_MATHL@ && defined GNULIB_POSIXCHECK
# undef tanl
# define tanl(x) \
    (GL_LINK_WARNING ("tanl is unportable - " \
                      "use gnulib module mathl for portability"), \
     tanl (x))
#endif


#if @GNULIB_TRUNCF@
# if !@HAVE_DECL_TRUNCF@
#  define truncf rpl_truncf
extern float truncf (float x);
# endif
#elif defined GNULIB_POSIXCHECK
# undef truncf
# define truncf(x) \
    (GL_LINK_WARNING ("truncf is unportable - " \
                      "use gnulib module truncf for portability"), \
     truncf (x))
#endif

#if @GNULIB_TRUNC@
# if !@HAVE_DECL_TRUNC@
#  define trunc rpl_trunc
extern double trunc (double x);
# endif
#elif defined GNULIB_POSIXCHECK
# undef trunc
# define trunc(x) \
    (GL_LINK_WARNING ("trunc is unportable - " \
                      "use gnulib module trunc for portability"), \
     trunc (x))
#endif

#if @GNULIB_TRUNCL@
# if @REPLACE_TRUNCL@
#  undef truncl
#  define truncl rpl_truncl
extern long double truncl (long double x);
# endif
#elif defined GNULIB_POSIXCHECK
# undef truncl
# define truncl(x) \
    (GL_LINK_WARNING ("truncl is unportable - " \
                      "use gnulib module truncl for portability"), \
     truncl (x))
#endif


#if @GNULIB_ISFINITE@
# if @REPLACE_ISFINITE@
extern int gl_isfinitef (float x);
extern int gl_isfinited (double x);
extern int gl_isfinitel (long double x);
#  undef isfinite
#  define isfinite(x) \
   (sizeof (x) == sizeof (long double) ? gl_isfinitel (x) : \
    sizeof (x) == sizeof (double) ? gl_isfinited (x) : \
    gl_isfinitef (x))
# endif
#elif defined GNULIB_POSIXCHECK
  /* How to override a macro?  */
#endif


#if @GNULIB_ISINF@
# if @REPLACE_ISINF@
extern int gl_isinff (float x);
extern int gl_isinfd (double x);
extern int gl_isinfl (long double x);
#  undef isinf
#  define isinf(x) \
   (sizeof (x) == sizeof (long double) ? gl_isinfl (x) : \
    sizeof (x) == sizeof (double) ? gl_isinfd (x) : \
    gl_isinff (x))
# endif
#elif defined GNULIB_POSIXCHECK
  /* How to override a macro?  */
#endif


#if @GNULIB_ISNANF@
/* Test for NaN for 'float' numbers.  */
# if @HAVE_ISNANF@
/* The original <math.h> included above provides a declaration of isnan macro
   or (older) isnanf function.  */
#  include <math.h>
#  if __GNUC__ >= 4
    /* GCC 4.0 and newer provides three built-ins for isnan.  */
#   undef isnanf
#   define isnanf(x) __builtin_isnanf ((float)(x))
#  elif defined isnan
#   undef isnanf
#   define isnanf(x) isnan ((float)(x))
#  endif
# else
/* Test whether X is a NaN.  */
#  undef isnanf
#  define isnanf rpl_isnanf
extern int isnanf (float x);
# endif
#endif

#if @GNULIB_ISNAND@
/* Test for NaN for 'double' numbers.
   This function is a gnulib extension, unlike isnan() which applied only
   to 'double' numbers earlier but now is a type-generic macro.  */
# if @HAVE_ISNAND@
/* The original <math.h> included above provides a declaration of isnan macro.  */
#  include <math.h>
#  if __GNUC__ >= 4
    /* GCC 4.0 and newer provides three built-ins for isnan.  */
#   undef isnand
#   define isnand(x) __builtin_isnan ((double)(x))
#  else
#   undef isnand
#   define isnand(x) isnan ((double)(x))
#  endif
# else
/* Test whether X is a NaN.  */
#  undef isnand
#  define isnand rpl_isnand
extern int isnand (double x);
# endif
#endif

#if @GNULIB_ISNANL@
/* Test for NaN for 'long double' numbers.  */
# if @HAVE_ISNANL@
/* The original <math.h> included above provides a declaration of isnan macro or (older) isnanl function.  */
#  include <math.h>
#  if __GNUC__ >= 4
    /* GCC 4.0 and newer provides three built-ins for isnan.  */
#   undef isnanl
#   define isnanl(x) __builtin_isnanl ((long double)(x))
#  elif defined isnan
#   undef isnanl
#   define isnanl(x) isnan ((long double)(x))
#  endif
# else
/* Test whether X is a NaN.  */
#  undef isnanl
#  define isnanl rpl_isnanl
extern int isnanl (long double x);
# endif
#endif

/* This must come *after* the snippets for GNULIB_ISNANF and GNULIB_ISNANL!  */
#if @GNULIB_ISNAN@
# if @REPLACE_ISNAN@
/* We can't just use the isnanf macro (e.g.) as exposed by
   isnanf.h (e.g.) here, because those may end up being macros
   that recursively expand back to isnan.  So use the gnulib
   replacements for them directly. */
#  if @HAVE_ISNANF@ && __GNUC__ >= 4
#   define gl_isnan_f(x) __builtin_isnan ((float)(x))
#  else
extern int rpl_isnanf (float x);
#   define gl_isnan_f(x) rpl_isnanf (x)
#  endif
#  if @HAVE_ISNAND@ && __GNUC__ >= 4
#   define gl_isnan_d(x) __builtin_isnan ((double)(x))
#  else
extern int rpl_isnand (double x);
#   define gl_isnan_d(x) rpl_isnand (x)
#  endif
#  if @HAVE_ISNANL@ && __GNUC__ >= 4
#   define gl_isnan_l(x) __builtin_isnan ((long double)(x))
#  else
extern int rpl_isnanl (long double x);
#   define gl_isnan_l(x) rpl_isnanl (x)
#  endif
#  undef isnan
#  define isnan(x) \
   (sizeof (x) == sizeof (long double) ? gl_isnan_l (x) : \
    sizeof (x) == sizeof (double) ? gl_isnan_d (x) : \
    gl_isnan_f (x))
# endif
#elif defined GNULIB_POSIXCHECK
  /* How to override a macro?  */
#endif


#if @GNULIB_SIGNBIT@
# if @REPLACE_SIGNBIT_USING_GCC@
#  undef signbit
   /* GCC 4.0 and newer provides three built-ins for signbit.  */
#  define signbit(x) \
   (sizeof (x) == sizeof (long double) ? __builtin_signbitl (x) : \
    sizeof (x) == sizeof (double) ? __builtin_signbit (x) : \
    __builtin_signbitf (x))
# endif
# if @REPLACE_SIGNBIT@
#  undef signbit
extern int gl_signbitf (float arg);
extern int gl_signbitd (double arg);
extern int gl_signbitl (long double arg);
#  if __GNUC__ >= 2 && !__STRICT_ANSI__
#   if defined FLT_SIGNBIT_WORD && defined FLT_SIGNBIT_BIT && !defined gl_signbitf
#    define gl_signbitf_OPTIMIZED_MACRO
#    define gl_signbitf(arg) \
       ({ union { float _value;						\
                  unsigned int _word[(sizeof (float) + sizeof (unsigned int) - 1) / sizeof (unsigned int)]; \
                } _m;							\
          _m._value = (arg);						\
          (_m._word[FLT_SIGNBIT_WORD] >> FLT_SIGNBIT_BIT) & 1;		\
        })
#   endif
#   if defined DBL_SIGNBIT_WORD && defined DBL_SIGNBIT_BIT && !defined gl_signbitd
#    define gl_signbitd_OPTIMIZED_MACRO
#    define gl_signbitd(arg) \
       ({ union { double _value;						\
                  unsigned int _word[(sizeof (double) + sizeof (unsigned int) - 1) / sizeof (unsigned int)]; \
                } _m;							\
          _m._value = (arg);						\
          (_m._word[DBL_SIGNBIT_WORD] >> DBL_SIGNBIT_BIT) & 1;		\
        })
#   endif
#   if defined LDBL_SIGNBIT_WORD && defined LDBL_SIGNBIT_BIT && !defined gl_signbitl
#    define gl_signbitl_OPTIMIZED_MACRO
#    define gl_signbitl(arg) \
       ({ union { long double _value;					\
                  unsigned int _word[(sizeof (long double) + sizeof (unsigned int) - 1) / sizeof (unsigned int)]; \
                } _m;							\
          _m._value = (arg);						\
          (_m._word[LDBL_SIGNBIT_WORD] >> LDBL_SIGNBIT_BIT) & 1;		\
        })
#   endif
#  endif
#  define signbit(x) \
   (sizeof (x) == sizeof (long double) ? gl_signbitl (x) : \
    sizeof (x) == sizeof (double) ? gl_signbitd (x) : \
    gl_signbitf (x))
# endif
#elif defined GNULIB_POSIXCHECK
  /* How to override a macro?  */
#endif


#ifdef __cplusplus
}
#endif

#endif /* _GL_MATH_H */
#endif /* _GL_MATH_H */
