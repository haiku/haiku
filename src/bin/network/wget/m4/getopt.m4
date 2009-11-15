# getopt.m4 serial 21
dnl Copyright (C) 2002-2006, 2008-2009 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# Request a POSIX compliant getopt function.
AC_DEFUN([gl_FUNC_GETOPT_POSIX],
[
  m4_divert_text([DEFAULTS], [gl_getopt_required=POSIX])
  AC_REQUIRE([gl_UNISTD_H_DEFAULTS])
  gl_GETOPT_IFELSE([
    gl_REPLACE_GETOPT
  ],
  [])
])

# Request a POSIX compliant getopt function with GNU extensions (such as
# options with optional arguments) and the functions getopt_long,
# getopt_long_only.
AC_DEFUN([gl_FUNC_GETOPT_GNU],
[
  m4_divert_text([INIT_PREPARE], [gl_getopt_required=GNU])

  AC_REQUIRE([gl_FUNC_GETOPT_POSIX])
])

# Request the gnulib implementation of the getopt functions unconditionally.
# argp.m4 uses this.
AC_DEFUN([gl_REPLACE_GETOPT],
[
  dnl Arrange for getopt.h to be created.
  gl_GETOPT_SUBSTITUTE_HEADER
  dnl Arrange for unistd.h to include getopt.h.
  GNULIB_UNISTD_H_GETOPT=1
  dnl Arrange to compile the getopt implementation.
  AC_LIBOBJ([getopt])
  AC_LIBOBJ([getopt1])
  gl_PREREQ_GETOPT
])

# emacs' configure.in uses this.
AC_DEFUN([gl_GETOPT_IFELSE],
[
  AC_REQUIRE([gl_GETOPT_CHECK_HEADERS])
  AS_IF([test -n "$gl_replace_getopt"], [$1], [$2])
])

# Determine whether to replace the entire getopt facility.
AC_DEFUN([gl_GETOPT_CHECK_HEADERS],
[
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles

  dnl Persuade Solaris <unistd.h> to declare optarg, optind, opterr, optopt.
  AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])

  gl_replace_getopt=

  dnl Test whether <getopt.h> is available.
  if test -z "$gl_replace_getopt" && test $gl_getopt_required = GNU; then
    AC_CHECK_HEADERS([getopt.h], [], [gl_replace_getopt=yes])
  fi

  dnl Test whether the function getopt_long is available.
  if test -z "$gl_replace_getopt" && test $gl_getopt_required = GNU; then
    AC_CHECK_FUNCS([getopt_long_only], [], [gl_replace_getopt=yes])
  fi

  dnl BSD getopt_long uses an incompatible method to reset option processing,
  dnl but the testsuite does not show a need to use this 'optreset' variable.
  if false && test -z "$gl_replace_getopt" && test $gl_getopt_required = GNU; then
    AC_CHECK_DECL([optreset], [gl_replace_getopt=yes], [],
      [#include <getopt.h>])
  fi

  dnl mingw's getopt (in libmingwex.a) does weird things when the options
  dnl strings starts with '+' and it's not the first call.  Some internal state
  dnl is left over from earlier calls, and neither setting optind = 0 nor
  dnl setting optreset = 1 get rid of this internal state.
  if test -z "$gl_replace_getopt"; then
    AC_CACHE_CHECK([whether getopt is POSIX compatible],
      [gl_cv_func_getopt_posix],
      [
        dnl This test fails on mingw and succeeds on all other platforms.
        AC_TRY_RUN([
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/* The glibc implementation of getopt supports setting optind = 0 as a means
   of clearing the internal state, but other implementations don't.  */
#if (__GLIBC__ >= 2)
# define OPTIND_MIN 0
#else
# define OPTIND_MIN 1
#endif

int
main ()
{
  {
    int argc = 0;
    char *argv[10];
    int c;

    argv[argc++] = "program";
    argv[argc++] = "-a";
    argv[argc++] = "foo";
    argv[argc++] = "bar";
    optind = OPTIND_MIN;
    opterr = 0;

    c = getopt (argc, argv, "ab");
    if (!(c == 'a'))
      return 1;
    c = getopt (argc, argv, "ab");
    if (!(c == -1))
      return 2;
    if (!(optind == 2))
      return 3;
  }
  /* Some internal state exists at this point.  */
  {
    int argc = 0;
    char *argv[10];
    int c;

    argv[argc++] = "program";
    argv[argc++] = "donald";
    argv[argc++] = "-p";
    argv[argc++] = "billy";
    argv[argc++] = "duck";
    argv[argc++] = "-a";
    argv[argc++] = "bar";
    optind = OPTIND_MIN;
    opterr = 0;

    c = getopt (argc, argv, "+abp:q:");
    if (!(c == -1))
      return 4;
    if (!(strcmp (argv[0], "program") == 0))
      return 5;
    if (!(strcmp (argv[1], "donald") == 0))
      return 6;
    if (!(strcmp (argv[2], "-p") == 0))
      return 7;
    if (!(strcmp (argv[3], "billy") == 0))
      return 8;
    if (!(strcmp (argv[4], "duck") == 0))
      return 9;
    if (!(strcmp (argv[5], "-a") == 0))
      return 10;
    if (!(strcmp (argv[6], "bar") == 0))
      return 11;
    if (!(optind == 1))
      return 12;
  }

  return 0;
}
],
          [gl_cv_func_getopt_posix=yes], [gl_cv_func_getopt_posix=no],
          [case "$host_os" in
             mingw*) gl_cv_func_getopt_posix="guessing no";;
             *)      gl_cv_func_getopt_posix="guessing yes";;
           esac
          ])
      ])
    case "$gl_cv_func_getopt_posix" in
      *no) gl_replace_getopt=yes ;;
    esac
  fi

  if test -z "$gl_replace_getopt" && test $gl_getopt_required = GNU; then
    AC_CACHE_CHECK([for working GNU getopt function], [gl_cv_func_getopt_gnu],
      [AC_RUN_IFELSE(
	[AC_LANG_PROGRAM([[#include <getopt.h>
			   #include <stddef.h>
			   #include <string.h>]],
	   [[
             /* This code succeeds on glibc 2.8, OpenBSD 4.0, Cygwin, mingw,
                and fails on MacOS X 10.5, AIX 5.2, HP-UX 11, IRIX 6.5,
                OSF/1 5.1, Solaris 10.  */
             {
               char *myargv[3];
               myargv[0] = "conftest";
               myargv[1] = "-+";
               myargv[2] = 0;
               opterr = 0;
               if (getopt (2, myargv, "+a") != '?')
                 return 1;
             }
             /* This code succeeds on glibc 2.8, mingw,
                and fails on MacOS X 10.5, OpenBSD 4.0, AIX 5.2, HP-UX 11,
                IRIX 6.5, OSF/1 5.1, Solaris 10, Cygwin.  */
             {
               char *argv[] = { "program", "-p", "foo", "bar" };

               optind = 1;
               if (getopt (4, argv, "p::") != 'p')
                 return 2;
               if (optarg != NULL)
                 return 3;
               if (getopt (4, argv, "p::") != -1)
                 return 4;
               if (optind != 2)
                 return 5;
             }
             return 0;
	   ]])],
	[gl_cv_func_getopt_gnu=yes],
	[gl_cv_func_getopt_gnu=no],
	[dnl Cross compiling. Guess based on host and declarations.
         case "$host_os" in
           *-gnu* | mingw*) gl_cv_func_getopt_gnu=no;;
           *)               gl_cv_func_getopt_gnu=yes;;
         esac
        ])
      ])
    if test "$gl_cv_func_getopt_gnu" = "no"; then
      gl_replace_getopt=yes
    fi
  fi
])

# emacs' configure.in uses this.
AC_DEFUN([gl_GETOPT_SUBSTITUTE_HEADER],
[
  GETOPT_H=getopt.h
  AC_DEFINE([__GETOPT_PREFIX], [[rpl_]],
    [Define to rpl_ if the getopt replacement functions and variables
     should be used.])
  AC_SUBST([GETOPT_H])
])

# Prerequisites of lib/getopt*.
# emacs' configure.in uses this.
AC_DEFUN([gl_PREREQ_GETOPT],
[
  AC_CHECK_DECLS_ONCE([getenv])
])
