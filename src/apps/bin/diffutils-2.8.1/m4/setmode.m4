# Check for setmode, DOS style.

# Copyright (C) 2001, 2002 Free Software Foundation, Inc.

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

AC_DEFUN([AC_FUNC_SETMODE_DOS],
  [AC_CHECK_HEADERS(fcntl.h unistd.h)
   AC_CACHE_CHECK([for DOS-style setmode],
     [ac_cv_func_setmode_dos],
     [AC_TRY_LINK(
	[#include <io.h>
	 #if HAVE_FCNTL_H
	 # include <fcntl.h>
	 #endif
	 #if HAVE_UNISTD_H
	 # include <unistd.h>
	 #endif],
	[int ret = setmode && setmode (1, O_BINARY);],
	[ac_cv_func_setmode_dos=yes],
	[ac_cv_func_setmode_dos=no])])
   if test $ac_cv_func_setmode_dos = yes; then
     AC_DEFINE(HAVE_SETMODE_DOS, 1,
       [Define to 1 if you have the DOS-style `setmode' function.])
   fi])
