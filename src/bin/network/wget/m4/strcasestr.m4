# strcasestr.m4 serial 13
dnl Copyright (C) 2005, 2007, 2008, 2009 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Check that strcasestr is present and works.
AC_DEFUN([gl_FUNC_STRCASESTR_SIMPLE],
[
  AC_REQUIRE([gl_HEADER_STRING_H_DEFAULTS])

  dnl Persuade glibc <string.h> to declare strcasestr().
  AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])

  AC_REQUIRE([gl_FUNC_MEMCHR])
  AC_CHECK_FUNCS([strcasestr])
  if test $ac_cv_func_strcasestr = no; then
    HAVE_STRCASESTR=0
  else
    if test "$gl_cv_func_memchr_works" != yes; then
      REPLACE_STRCASESTR=1
    fi
  fi
  if test $HAVE_STRCASESTR = 0 || test $REPLACE_STRCASESTR = 1; then
    AC_LIBOBJ([strcasestr])
    gl_PREREQ_STRCASESTR
  fi
]) # gl_FUNC_STRCASESTR_SIMPLE

dnl Additionally, check that strcasestr is efficient.
AC_DEFUN([gl_FUNC_STRCASESTR],
[
  AC_REQUIRE([gl_FUNC_STRCASESTR_SIMPLE])
  if test $HAVE_STRCASESTR = 1 && test $REPLACE_STRCASESTR = 0; then
    AC_CACHE_CHECK([whether strcasestr works in linear time],
      [gl_cv_func_strcasestr_linear],
      [AC_RUN_IFELSE([AC_LANG_PROGRAM([[
#include <signal.h> /* for signal */
#include <string.h> /* for memmem */
#include <stdlib.h> /* for malloc */
#include <unistd.h> /* for alarm */
]], [[size_t m = 1000000;
    char *haystack = (char *) malloc (2 * m + 2);
    char *needle = (char *) malloc (m + 2);
    void *result = 0;
    /* Failure to compile this test due to missing alarm is okay,
       since all such platforms (mingw) also lack strcasestr.  */
    signal (SIGALRM, SIG_DFL);
    alarm (5);
    /* Check for quadratic performance.  */
    if (haystack && needle)
      {
	memset (haystack, 'A', 2 * m);
	haystack[2 * m] = 'B';
	haystack[2 * m + 1] = 0;
	memset (needle, 'A', m);
	needle[m] = 'B';
	needle[m + 1] = 0;
	result = strcasestr (haystack, needle);
      }
    return !result;]])],
	[gl_cv_func_strcasestr_linear=yes], [gl_cv_func_strcasestr_linear=no],
        [dnl Only glibc >= 2.9 and cygwin >= 1.7.0 are known to have a
         dnl strcasestr that works in linear time.
	 AC_EGREP_CPP([Lucky user],
	   [
#include <features.h>
#ifdef __GNU_LIBRARY__
 #if (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 9) || (__GLIBC__ > 2)
  Lucky user
 #endif
#endif
#ifdef __CYGWIN__
 #include <cygwin/version.h>
 #if CYGWIN_VERSION_DLL_MAJOR >= 1007
  Lucky user
 #endif
#endif
	   ],
	   [gl_cv_func_strcasestr_linear=yes],
	   [gl_cv_func_strcasestr_linear="guessing no"])
	])
      ])
    if test "$gl_cv_func_strcasestr_linear" != yes; then
      REPLACE_STRCASESTR=1
      AC_LIBOBJ([strcasestr])
      gl_PREREQ_STRCASESTR
    fi
  fi
]) # gl_FUNC_STRCASESTR

# Prerequisites of lib/strcasestr.c.
AC_DEFUN([gl_PREREQ_STRCASESTR], [
  :
])
