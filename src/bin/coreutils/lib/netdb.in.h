/* -*- buffer-read-only: t -*- vi: set ro: */
/* DO NOT EDIT! GENERATED AUTOMATICALLY! */
#line 1
/* Provide a netdb.h header file for systems lacking it (read: MinGW).
   Copyright (C) 2008 Free Software Foundation, Inc.
   Written by Simon Josefsson.

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

/* This file is supposed to be used on platforms that lack <netdb.h>.
   It is intended to provide definitions and prototypes needed by an
   application.  */

#ifndef _GL_NETDB_H

#if @HAVE_NETDB_H@

# if __GNUC__ >= 3
@PRAGMA_SYSTEM_HEADER@
# endif

/* The include_next requires a split double-inclusion guard.  */
# @INCLUDE_NEXT@ @NEXT_NETDB_H@

#endif

#ifndef _GL_NETDB_H
#define _GL_NETDB_H

/* Get netdb.h definitions such as struct hostent for MinGW.  */
#include <sys/socket.h>

/* Declarations for a platform that lacks <netdb.h>, or where it is
   incomplete.  */

#if @GNULIB_GETADDRINFO@

# if !@HAVE_STRUCT_ADDRINFO@

/* Structure to contain information about address of a service provider.  */
struct addrinfo
{
  int ai_flags;			/* Input flags.  */
  int ai_family;		/* Protocol family for socket.  */
  int ai_socktype;		/* Socket type.  */
  int ai_protocol;		/* Protocol for socket.  */
  socklen_t ai_addrlen;		/* Length of socket address.  */
  struct sockaddr *ai_addr;	/* Socket address for socket.  */
  char *ai_canonname;		/* Canonical name for service location.  */
  struct addrinfo *ai_next;	/* Pointer to next in list.  */
};
# endif

/* Possible values for `ai_flags' field in `addrinfo' structure.  */
# ifndef AI_PASSIVE
#  define AI_PASSIVE	0x0001	/* Socket address is intended for `bind'.  */
# endif
# ifndef AI_CANONNAME
#  define AI_CANONNAME	0x0002	/* Request for canonical name.  */
# endif
# ifndef AI_NUMERICSERV
#  define AI_NUMERICSERV	0x0400	/* Don't use name resolution.  */
# endif

# if 0
/* The commented out definitions below are not yet implemented in the
   GNULIB getaddrinfo() replacement, so are not yet needed and may, in fact,
   cause conflicts on systems with a getaddrinfo() function which does not
   define them.

   If they are restored, be sure to protect the definitions with #ifndef.  */
#  define AI_NUMERICHOST	0x0004	/* Don't use name resolution.  */
#  define AI_V4MAPPED	0x0008	/* IPv4 mapped addresses are acceptable.  */
#  define AI_ALL		0x0010	/* Return IPv4 mapped and IPv6 addresses.  */
#  define AI_ADDRCONFIG	0x0020	/* Use configuration of this host to choose
				   returned address type..  */
# endif /* 0 */

/* Error values for `getaddrinfo' function.  */
# ifndef EAI_BADFLAGS
#  define EAI_BADFLAGS	  -1	/* Invalid value for `ai_flags' field.  */
#  define EAI_NONAME	  -2	/* NAME or SERVICE is unknown.  */
#  define EAI_AGAIN	  -3	/* Temporary failure in name resolution.  */
#  define EAI_FAIL	  -4	/* Non-recoverable failure in name res.  */
#  define EAI_NODATA	  -5	/* No address associated with NAME.  */
#  define EAI_FAMILY	  -6	/* `ai_family' not supported.  */
#  define EAI_SOCKTYPE	  -7	/* `ai_socktype' not supported.  */
#  define EAI_SERVICE	  -8	/* SERVICE not supported for `ai_socktype'.  */
#  define EAI_MEMORY	  -10	/* Memory allocation failure.  */
# endif

/* Since EAI_NODATA is deprecated by RFC3493, some systems (at least
   FreeBSD, which does define EAI_BADFLAGS) have removed the definition
   in favor of EAI_NONAME.  */
# if !defined EAI_NODATA && defined EAI_NONAME
#  define EAI_NODATA EAI_NONAME
# endif

# ifndef EAI_OVERFLOW
/* Not defined on mingw32 and Haiku. */
#  define EAI_OVERFLOW	  -12	/* Argument buffer overflow.  */
# endif
# ifndef EAI_ADDRFAMILY
/* Not defined on mingw32. */
#  define EAI_ADDRFAMILY  -9	/* Address family for NAME not supported.  */
# endif
# ifndef EAI_SYSTEM
/* Not defined on mingw32. */
#  define EAI_SYSTEM	  -11	/* System error returned in `errno'.  */
# endif

# if 0
/* The commented out definitions below are not yet implemented in the
   GNULIB getaddrinfo() replacement, so are not yet needed.

   If they are restored, be sure to protect the definitions with #ifndef.  */
#  ifndef EAI_INPROGRESS
#   define EAI_INPROGRESS	-100	/* Processing request in progress.  */
#   define EAI_CANCELED		-101	/* Request canceled.  */
#   define EAI_NOTCANCELED	-102	/* Request not canceled.  */
#   define EAI_ALLDONE		-103	/* All requests done.  */
#   define EAI_INTR		-104	/* Interrupted by a signal.  */
#   define EAI_IDN_ENCODE	-105	/* IDN encoding failed.  */
#  endif
# endif

# if !@HAVE_DECL_GETADDRINFO@
/* Translate name of a service location and/or a service name to set of
   socket addresses.
   For more details, see the POSIX:2001 specification
   <http://www.opengroup.org/susv3xsh/getaddrinfo.html>.  */
extern int getaddrinfo (const char *restrict nodename,
			const char *restrict servname,
			const struct addrinfo *restrict hints,
			struct addrinfo **restrict res);
# endif

# if !@HAVE_DECL_FREEADDRINFO@
/* Free `addrinfo' structure AI including associated storage.
   For more details, see the POSIX:2001 specification
   <http://www.opengroup.org/susv3xsh/getaddrinfo.html>.  */
extern void freeaddrinfo (struct addrinfo *ai);
# endif

# if !@HAVE_DECL_GAI_STRERROR@
/* Convert error return from getaddrinfo() to a string.
   For more details, see the POSIX:2001 specification
   <http://www.opengroup.org/susv3xsh/gai_strerror.html>.  */
extern const char *gai_strerror (int ecode);
# endif

# if !@HAVE_DECL_GETNAMEINFO@
/* Convert socket address to printable node and service names.
   For more details, see the POSIX:2001 specification
   <http://www.opengroup.org/susv3xsh/getnameinfo.html>.  */
extern int getnameinfo(const struct sockaddr *restrict sa, socklen_t salen,
		       char *restrict node, socklen_t nodelen,
		       char *restrict service, socklen_t servicelen,
		       int flags);
# endif

/* Possible flags for getnameinfo.  */
# ifndef NI_NUMERICHOST
#  define NI_NUMERICHOST 1
# endif
# ifndef NI_NUMERICSERV
#  define NI_NUMERICSERV 2
# endif

#endif /* @GNULIB_GETADDRINFO@ */

#endif /* _GL_NETDB_H */
#endif /* _GL_NETDB_H */
