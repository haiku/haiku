# realloc.m4 serial 9
dnl Copyright (C) 2007, 2009 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# gl_FUNC_REALLOC_POSIX
# ---------------------
# Test whether 'realloc' is POSIX compliant (sets errno to ENOMEM when it
# fails), and replace realloc if it is not.
AC_DEFUN([gl_FUNC_REALLOC_POSIX],
[
  AC_REQUIRE([gl_CHECK_MALLOC_POSIX])
  if test $gl_cv_func_malloc_posix = yes; then
    HAVE_REALLOC_POSIX=1
    AC_DEFINE([HAVE_REALLOC_POSIX], [1],
      [Define if the 'realloc' function is POSIX compliant.])
  else
    AC_LIBOBJ([realloc])
    HAVE_REALLOC_POSIX=0
  fi
  AC_SUBST([HAVE_REALLOC_POSIX])
])
