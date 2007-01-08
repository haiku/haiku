/* Declare a replacement for lchown on hosts that lack it.

   Copyright (C) 2006 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* Written by Jim Meyering.  */

#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#if HAVE_DECL_LCHOWN
# if ! HAVE_LCHOWN
#  undef lchown
#  define lchown rpl_chown
# endif
#else
int lchown (char const *, uid_t, gid_t);
#endif

/* Some systems don't have EOPNOTSUPP.  */
#ifndef EOPNOTSUPP
# ifdef ENOTSUP
#  define EOPNOTSUPP ENOTSUP
# else
/* Some systems don't have ENOTSUP either.  */
#  define EOPNOTSUPP EINVAL
# endif
#endif
