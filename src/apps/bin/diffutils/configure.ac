# Configure template for GNU Diffutils.

# Copyright (C) 1994, 1995, 1998, 2001, 2002 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

AC_PREREQ(2.53)

AC_INIT(GNU diffutils, 2.8.1, bug-gnu-utils@gnu.org)
AC_CONFIG_SRCDIR(src/diff.c)

AC_CONFIG_AUX_DIR(config)
AM_CONFIG_HEADER([config.h:config.hin])
AM_INIT_AUTOMAKE([gnits])

AC_PROG_AWK
AC_PROG_CC
AC_ISC_POSIX
AM_PROG_CC_STDC
AC_PROG_CPP
AM_MISSING_PROG(HELP2MAN, help2man)
AC_PROG_INSTALL
AC_PROG_RANLIB

AC__GNU_SOURCE
AC_DEFINE([__EXTENSIONS__], 1, [Enable Solaris extensions.])
AC_SYS_LARGEFILE

AC_C_CONST
AC_C_INLINE
AC_C_VARARRAYS

AC_DEFINE(DEFAULT_DIFF_PROGRAM, "diff",
  [Name of "diff" program, unless overridden.])

AC_DEFINE(DEFAULT_EDITOR_PROGRAM, "ed",
  [Name of editor program, unless overridden.])

AC_PATH_PROG(PR_PROGRAM, pr, "")
AC_DEFINE_UNQUOTED(PR_PROGRAM, "$PR_PROGRAM", [Name of "pr" program.])

AC_HEADER_STDBOOL
AC_HEADER_STDC
AC_CHECK_HEADERS(fcntl.h libintl.h limits.h locale.h \
  stdlib.h string.h sys/file.h time.h unistd.h)
AC_CHECK_MEMBERS([struct stat.st_blksize])
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_CHECK_TYPE(ptrdiff_t, int)
AC_CHECK_TYPE(ssize_t, int)
jm_AC_TYPE_UINTMAX_T
AM_GNU_GETTEXT([external])
XGETTEXT="AWK='$AWK' \$(SHELL) \$(top_srcdir)/exgettext $XGETTEXT"
AC_HEADER_DIRENT
AC_HEADER_STAT
AC_HEADER_SYS_WAIT
AC_STRUCT_ST_MTIM_NSEC
AC_TYPE_PID_T
AC_TYPE_SIGNAL

AC_CHECK_FUNCS(diraccess dup2 gettimeofday \
  sigaction sigprocmask strchr strerror tmpnam)
AC_REPLACE_FUNCS(memchr mkstemp strcasecmp waitpid)
if test $ac_cv_func_mkstemp != yes; then
  AC_LIBOBJ(tempname)
fi
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_FORK
AC_FUNC_VPRINTF
jm_FUNC_GLIBC_UNLOCKED_IO
jm_FUNC_GNU_STRFTIME
# No need for AC_FUNC_MEMCMP, since memcmp is used only to test for equality.
jm_FUNC_MALLOC
jm_FUNC_REALLOC
jm_PREREQ_C_STACK
jm_PREREQ_ERROR
jm_PREREQ_HARD_LOCALE
jm_PREREQ_QUOTEARG
jm_PREREQ_REGEX
AC_DEFINE([REGEX_MALLOC], 1,
  [Define to 1 to avoid alloca in the regular-expression implementation.])
jm_PREREQ_TEMPNAME
jm_AC_PREREQ_XSTRTOUMAX

# Check for clock_gettime and its library.  Solaris puts it in -lrt or
# -lposix4, but we don't want to link that library unless we have to.
diff_saved_libs=$LIBS
AC_SEARCH_LIBS(clock_gettime, [rt posix4])
case $ac_cv_search_clock_gettime in
no | 'none required')
  LIB_CLOCK_GETTIME=;;
*)
  LIB_CLOCK_GETTIME=$ac_cv_search_clock_gettime;;
esac
AC_SUBST([LIB_CLOCK_GETTIME])
AC_CHECK_FUNCS(clock_gettime)
LIBS=$diff_saved_libs

# fnmatch
AC_FUNC_FNMATCH
if test $ac_cv_func_fnmatch_works = yes; then
  rm -f lib/fnmatch.h
else
  AC_LIBOBJ(fnmatch)
  AC_CONFIG_LINKS(lib/fnmatch.h:lib/fnmatch.hin)
fi

# regex
jm_INCLUDED_REGEX([lib/regex.c])
if test "$jm_with_regex" = yes; then
  AC_CONFIG_LINKS(lib/regex.h:lib/regex.hin)
else
  rm -f lib/regex.h
fi

jm_AC_DOS
AC_FUNC_SETMODE_DOS

AC_CONFIG_FILES([Makefile doc/Makefile \
		 lib/Makefile lib/posix/Makefile man/Makefile \
		 m4/Makefile ms/Makefile po/Makefile.in src/Makefile])
AC_CONFIG_COMMANDS([default], [date > stamp-h])
AC_OUTPUT
