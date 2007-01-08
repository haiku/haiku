/* Duplicate a size-bounded string.
   Copyright (C) 2003, 2006 Free Software Foundation, Inc.

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

/* Get size_t.  */
#include <stddef.h>
/* If HAVE_STRNDUP, get the strndup declaration.
   If !HAVE_STRNDUP, include <string.h> now so that it doesn't cause
   trouble if included later.  */
#include <string.h>

#if !HAVE_STRNDUP
# undef strndup
# define strndup rpl_strndup
# if !HAVE_DECL_STRNDUP  /* Don't risk conflicting declarations.  */
/* Return a newly allocated copy of at most N bytes of STRING.  */
extern char *strndup (const char *string, size_t n);
# endif
#endif
