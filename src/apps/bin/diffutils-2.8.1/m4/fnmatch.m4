# Check for fnmatch.

# This is a modified version of autoconf's AC_FUNC_FNMATCH;
# it also checks for FNM_CASEFOLD or FNM_IGNORECASE.

# Copyright (C) 2000, 2001 Free Software Foundation, Inc.

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

# AC_FUNC_FNMATCH
# ---------------
# We look for fnmatch.h to avoid that the test fails in C++.
AC_DEFUN([AC_FUNC_FNMATCH],
[AC_CACHE_CHECK([for working GNU-style fnmatch],
                [ac_cv_func_fnmatch_works],
# Some versions of Solaris, SCO, and the GNU C Library
# have a broken or incompatible fnmatch.
# So we run a test program.  If we are cross-compiling, take no chance.
# Thanks to John Oleynick, Franc,ois Pinard, and Paul Eggert for this test.
[AC_RUN_IFELSE([AC_LANG_PROGRAM([@%:@include <fnmatch.h>],
 [exit (fnmatch ("a*", "abc", 0) != 0
	|| fnmatch ("xxXX", "xXxX", FNM_CASEFOLD) != 0
	|| fnmatch ("d*/*1", "d/s/1", FNM_FILE_NAME) != FNM_NOMATCH
	|| fnmatch ("*", "x", FNM_FILE_NAME | FNM_LEADING_DIR) != 0
	|| fnmatch ("x*", "x/y/z", FNM_FILE_NAME | FNM_LEADING_DIR) != 0
	|| fnmatch ("*c*", "c/x", FNM_FILE_NAME | FNM_LEADING_DIR) != 0);])],
               [ac_cv_func_fnmatch_works=yes],
               [ac_cv_func_fnmatch_works=no],
               [ac_cv_func_fnmatch_works=no])])
if test $ac_cv_func_fnmatch_works = yes; then
  AC_DEFINE(HAVE_FNMATCH, 1,
            [Define to 1 if your system has a working `fnmatch' function.])
fi
])# AC_FUNC_FNMATCH
