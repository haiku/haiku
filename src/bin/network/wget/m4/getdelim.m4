# getdelim.m4 serial 5

dnl Copyright (C) 2005, 2006, 2007 Free Software dnl Foundation, Inc.
dnl
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_PREREQ([2.59])

AC_DEFUN([gl_FUNC_GETDELIM],
[
  AC_REQUIRE([gl_STDIO_H_DEFAULTS])

  dnl Persuade glibc <stdio.h> to declare getdelim().
  AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])

  AC_REPLACE_FUNCS([getdelim])
  AC_CHECK_DECLS_ONCE([getdelim])

  if test $ac_cv_func_getdelim = no; then
    gl_PREREQ_GETDELIM
  fi

  if test $ac_cv_have_decl_getdelim = no; then
    HAVE_DECL_GETDELIM=0
  fi
])

# Prerequisites of lib/getdelim.c.
AC_DEFUN([gl_PREREQ_GETDELIM],
[
  AC_CHECK_FUNCS([flockfile funlockfile])
  AC_CHECK_DECLS([getc_unlocked])
])
