# fseeko.m4 serial 4
dnl Copyright (C) 2007, 2008, 2009 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_FSEEKO],
[
  AC_REQUIRE([gl_STDIO_H_DEFAULTS])
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([gl_STDIN_LARGE_OFFSET])

  dnl Persuade glibc <stdio.h> to declare fseeko().
  AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])

  AC_CACHE_CHECK([for fseeko], [gl_cv_func_fseeko],
    [
      AC_TRY_LINK([#include <stdio.h>], [fseeko (stdin, 0, 0);],
	[gl_cv_func_fseeko=yes], [gl_cv_func_fseeko=no])
    ])
  if test $gl_cv_func_fseeko = no; then
    HAVE_FSEEKO=0
    gl_REPLACE_FSEEKO
  elif test $gl_cv_var_stdin_large_offset = no; then
    gl_REPLACE_FSEEKO
  fi
])

AC_DEFUN([gl_REPLACE_FSEEKO],
[
  AC_LIBOBJ([fseeko])
  AC_REQUIRE([gl_STDIO_H_DEFAULTS])
  REPLACE_FSEEKO=1
])
