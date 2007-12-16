/* Internal implementation of access control lists.

   Copyright (C) 2002, 2003, 2005, 2006, 2007 Free Software Foundation, Inc.

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
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

   Written by Paul Eggert and Andreas Gruenbacher.  */

#include <config.h>

#include "acl.h"

#include <stdbool.h>
#include <stdlib.h>

#ifdef HAVE_ACL_LIBACL_H
# include <acl/libacl.h>
#endif

#include "error.h"
#include "quote.h"

#include <errno.h>
#ifndef ENOSYS
# define ENOSYS (-1)
#endif
#ifndef ENOTSUP
# define ENOTSUP (-1)
#endif

#include "gettext.h"
#define _(msgid) gettext (msgid)

#ifndef HAVE_FCHMOD
# define HAVE_FCHMOD false
# define fchmod(fd, mode) (-1)
#endif

#ifndef MIN_ACL_ENTRIES
# define MIN_ACL_ENTRIES 4
#endif

/* POSIX 1003.1e (draft 17) */
#ifndef HAVE_ACL_GET_FD
# define HAVE_ACL_GET_FD false
# define acl_get_fd(fd) (NULL)
#endif

/* POSIX 1003.1e (draft 17) */
#ifndef HAVE_ACL_SET_FD
# define HAVE_ACL_SET_FD false
# define acl_set_fd(fd, acl) (-1)
#endif

/* Linux-specific */
#ifndef HAVE_ACL_EXTENDED_FILE
# define HAVE_ACL_EXTENDED_FILE false
# define acl_extended_file(name) (-1)
#endif

/* Linux-specific */
#ifndef HAVE_ACL_FROM_MODE
# define HAVE_ACL_FROM_MODE false
# define acl_from_mode(mode) (NULL)
#endif

#define ACL_NOT_WELL_SUPPORTED(Errno) \
  ((Errno) == ENOTSUP || (Errno) == ENOSYS || (Errno) == EINVAL)

/* Define a replacement for acl_entries if needed.  */
#if USE_ACL && HAVE_ACL_GET_FILE && HAVE_ACL_FREE && !HAVE_ACL_ENTRIES
# define acl_entries rpl_acl_entries
int acl_entries (acl_t);
#endif
