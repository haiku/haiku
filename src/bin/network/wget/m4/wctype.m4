# wctype.m4 serial 2

dnl A placeholder for ISO C99 <wctype.h>, for platforms that lack it.

dnl Copyright (C) 2006, 2007, 2008, 2009 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Written by Paul Eggert.

AC_DEFUN([gl_WCTYPE_H],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_CHECK_FUNCS_ONCE([iswcntrl])
  if test $ac_cv_func_iswcntrl = yes; then
    HAVE_ISWCNTRL=1
  else
    HAVE_ISWCNTRL=0
  fi
  AC_SUBST([HAVE_ISWCNTRL])
  AC_CHECK_HEADERS_ONCE([wctype.h])
  AC_REQUIRE([AC_C_INLINE])

  AC_REQUIRE([gt_TYPE_WINT_T])
  if test $gt_cv_c_wint_t = yes; then
    HAVE_WINT_T=1
  else
    HAVE_WINT_T=0
  fi
  AC_SUBST([HAVE_WINT_T])

  WCTYPE_H=wctype.h
  if test $ac_cv_header_wctype_h = yes; then
    if test $ac_cv_func_iswcntrl = yes; then
      dnl Linux libc5 has an iswprint function that returns 0 for all arguments.
      dnl The other functions are likely broken in the same way.
      AC_CACHE_CHECK([whether iswcntrl works], [gl_cv_func_iswcntrl_works],
        [
          AC_TRY_RUN([#include <stddef.h>
                      #include <stdio.h>
                      #include <time.h>
                      #include <wchar.h>
                      #include <wctype.h>
                      int main () { return iswprint ('x') == 0; }],
            [gl_cv_func_iswcntrl_works=yes], [gl_cv_func_iswcntrl_works=no],
            [AC_TRY_COMPILE([#include <stdlib.h>
                          #if __GNU_LIBRARY__ == 1
                          Linux libc5 i18n is broken.
                          #endif], [],
              [gl_cv_func_iswcntrl_works=yes], [gl_cv_func_iswcntrl_works=no])
            ])
        ])
      if test $gl_cv_func_iswcntrl_works = yes; then
        WCTYPE_H=
      fi
    fi
    dnl Compute NEXT_WCTYPE_H even if WCTYPE_H is empty,
    dnl for the benefit of builds from non-distclean directories.
    gl_CHECK_NEXT_HEADERS([wctype.h])
    HAVE_WCTYPE_H=1
  else
    HAVE_WCTYPE_H=0
  fi
  AC_SUBST([HAVE_WCTYPE_H])
  AC_SUBST([WCTYPE_H])

  if test "$gl_cv_func_iswcntrl_works" = no; then
    REPLACE_ISWCNTRL=1
  else
    REPLACE_ISWCNTRL=0
  fi
  AC_SUBST([REPLACE_ISWCNTRL])
])
