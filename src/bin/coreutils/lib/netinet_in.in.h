/* -*- buffer-read-only: t -*- vi: set ro: */
/* DO NOT EDIT! GENERATED AUTOMATICALLY! */
#line 1
/* Substitute for <netinet/in.h>.
   Copyright (C) 2007-2008 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef _GL_NETINET_IN_H

#if @HAVE_NETINET_IN_H@

# if __GNUC__ >= 3
@PRAGMA_SYSTEM_HEADER@
# endif

/* On many platforms, <netinet/in.h> assumes prior inclusion of
   <sys/types.h>.  */
# include <sys/types.h>

/* The include_next requires a split double-inclusion guard.  */
# @INCLUDE_NEXT@ @NEXT_NETINET_IN_H@

#endif

#ifndef _GL_NETINET_IN_H
#define _GL_NETINET_IN_H

#if !@HAVE_NETINET_IN_H@

/* A platform that lacks <netinet/in.h>.  */

# include <sys/socket.h>

#endif

#endif /* _GL_NETINET_IN_H */
#endif /* _GL_NETINET_IN_H */
