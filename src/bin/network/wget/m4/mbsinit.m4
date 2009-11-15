# mbsinit.m4 serial 3
dnl Copyright (C) 2008, 2009 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_MBSINIT],
[
  AC_REQUIRE([gl_WCHAR_H_DEFAULTS])

  AC_REQUIRE([AC_TYPE_MBSTATE_T])
  gl_MBSTATE_T_BROKEN
  if test $REPLACE_MBSTATE_T = 1; then
    REPLACE_MBSINIT=1
  fi
  AC_CHECK_FUNCS_ONCE([mbsinit])
  if test $ac_cv_func_mbsinit = no; then
    HAVE_MBSINIT=0
  fi
  if test $HAVE_MBSINIT = 0 || test $REPLACE_MBSINIT = 1; then
    gl_REPLACE_WCHAR_H
    AC_LIBOBJ([mbsinit])
    gl_PREREQ_MBSINIT
  fi
])

# Prerequisites of lib/mbsinit.c.
AC_DEFUN([gl_PREREQ_MBSINIT], [
  :
])
